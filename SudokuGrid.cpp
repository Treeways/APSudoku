#include "SudokuGrid.hpp"

namespace Sudoku
{
	void Cell::clear()
	{
		*this = Cell();
	}
	void Cell::clear_marks()
	{
		if(val)
			val = 0;
		else
		{
			memset(center_marks, 0, sizeof(center_marks));
			memset(corner_marks, 0, sizeof(corner_marks));
		}
	}
	void Cell::draw(u16 X, u16 Y, u16 W, u16 H) const
	{
		al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, c_bg);
		if(flags & CFL_SELECTED)
			al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, c_sel);
		al_draw_rectangle(X, Y, X+W-1, Y+H-1, c_border, 0.5);
		
		bool given = (flags & CFL_GIVEN);
		if(val)
		{
			string text = to_string(val);
			int tx = (X+W/2);
			int ty = (Y+H/2)-(al_get_font_line_height(fonts[FONT_ANSWER])/2);
			al_draw_text(fonts[FONT_ANSWER], given ? C_GIVEN : c_text, tx, ty, ALLEGRO_ALIGN_CENTRE, text.c_str());
		}
		else if(!given)
		{
			string cen_marks, crn_marks;
			for(u8 q = 0; q < 9; ++q)
			{
				if(center_marks[q])
					cen_marks += to_string(q+1);
				if(corner_marks[q])
					crn_marks += to_string(q+1);
			}
			if(!cen_marks.empty())
			{
				auto font = FONT_MARKING5;
				if(cen_marks.size() > 5)
					font = Font(FONT_MARKING5 + cen_marks.size()-5);
				int tx = (X+W/2);
				int ty = (Y+H/2)-(al_get_font_line_height(fonts[font])/2);
				al_draw_text(fonts[font], c_text, tx, ty, ALLEGRO_ALIGN_CENTRE, cen_marks.c_str());
			}
			if(!crn_marks.empty())
			{
				auto font = FONT_MARKING5;
				u16 vx = 6, vy = 6;
				scale_pos(vx, vy);
				u16 xs[] = {vx,W-vx,vx,W-vx,W/2,W/2,vx,W-vx,W/2-4};
				u16 ys[] = {vy,vy,H-vy,H-vy,vy,H-vy,H/2,H/2,H/2-4};
				auto fh = al_get_font_line_height(fonts[font]);
				char buf[2] = {0,0};
				for(int q = 0; q < 9 && q < crn_marks.size(); ++q)
				{
					buf[0] = crn_marks[q];
					al_draw_text(fonts[font], c_text, X+xs[q], Y+ys[q] - fh/2, ALLEGRO_ALIGN_CENTRE, buf);
				}
			}
		}
	}
	
	PuzzleState Region::check() const
	{
		PuzzleState ret = PS_SOLVED;
		set<u8> found;
		set<u8> dupes;
		for(Cell* c : cells)
		{
			if(!c)
				throw sudoku_exception("Region error!");
			if(!c->val)
				ret |= PS_INCOMPLETE;
			else
			{
				if(c->val != c->solution)
					ret |= PS_WRONG;
				if(found.contains(c->val))
				{
					ret |= PS_INVALID;
					dupes.insert(c->val);
				}
				else found.insert(c->val);
			}
		}
		if(ret & PS_INVALID)
		{
			for(Cell* c : cells)
			{
				if(dupes.contains(c->val))
					c->flags |= CFL_INVALID;
			}
		}
		return ret;
	}

	Region Grid::get_row(u8 ind)
	{
		if(ind >= 9)
			throw sudoku_exception("Index error!");
		Region reg;
		for(u8 q = 0; q < 9; ++q)
			reg.cells[q] = &(cells[9*ind + q]);
		return reg;
	}
	Region Grid::get_col(u8 ind)
	{
		if(ind >= 9)
			throw sudoku_exception("Index error!");
		Region reg;
		for(u8 q = 0; q < 9; ++q)
			reg.cells[q] = &(cells[9*q + ind]);
		return reg;
	}
	Region Grid::get_box(u8 ind)
	{
		if(ind >= 9)
			throw sudoku_exception("Index error!");
		Region reg;
		for(u8 q = 0; q < 9; ++q)
			reg.cells[q] = &(cells[9*(3*(ind/3) + (q/3)) + (3*(ind%3) + (q%3))]);
		return reg;
	}
	Cell* Grid::get(u8 row, u8 col)
	{
		if(row >= 9 || col >= 9)
			return nullptr;
		return &cells[9*row + col];
	}
	Cell* Grid::get_hov()
	{
		u8 col = (mouse_state.x - x) / CELL_SZ;
		u8 row = (mouse_state.y - y) / CELL_SZ;
		return get(row,col);
	}
	
	
	void Grid::deselect()
	{
		for(Cell* c : selected)
			c->flags &= ~CFL_SELECTED;
		selected.clear();
	}
	void Grid::select(Cell* c)
	{
		bool found = false;
		for(int q = 0; q < 9*9; ++q)
			if((found = (c == &cells[q])))
				break;
		if(!found)
			throw sudoku_exception("Cannot select cell not from this grid!");
		selected.insert(c);
		c->flags |= CFL_SELECTED;
	}
	void Grid::clear_invalid()
	{
		for(Cell& c : cells)
		{
			c.flags &= ~CFL_INVALID;
		}
	}
	void Grid::draw() const
	{
		for(u8 q = 0; q < 9*9; ++q) // Cell draws
		{
			u16 X = x + ((q%9)*CELL_SZ), Y = y + ((q/9)*CELL_SZ),
				W = CELL_SZ, H = CELL_SZ;
			scale_pos(X,Y,W,H);
			cells[q].draw(X, Y, W, H);
		}
		for(u8 q = 0; q < 9; ++q) // 3x3 box thicker borders
		{
			u16 X = x + ((q%3)*(CELL_SZ*3)), Y = y + ((q/3)*(CELL_SZ*3)),
				W = CELL_SZ*3, H = CELL_SZ*3;
			scale_pos(X,Y,W,H);
			al_draw_rectangle(X, Y, X+W-1, Y+H-1, C_BLACK, 2);
		}
	}
	Grid::Grid(u16 X, u16 Y)
		: MouseObject(X,Y,9*CELL_SZ,9*CELL_SZ)
	{}
}

