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
	void Cell::draw_sel(u16 X, u16 Y, u16 W, u16 H, u8 hlbits) const
	{
		if(!hlbits)
			return;
		u16 HLW = 4, HLH = 4;
		scale_pos(HLW, HLH);
		for(u8 q = 0; q < NUM_DIRS; ++q)
		{
			u16 TX = X, TY = Y, TW = W, TH = H;
			if(hlbits & (1<<q))
			{
				switch(q)
				{
					case DIR_UP:
					case DIR_DOWN:
						if(!(hlbits & (1<<DIR_LEFT)))
						{
							TX -= HLW;
							TW += HLW;
						}
						if(!(hlbits & (1<<DIR_RIGHT)))
						{
							TW += HLW;
						}
						break;
					case DIR_LEFT:
					case DIR_RIGHT:
						if(!(hlbits & (1<<DIR_UP)))
						{
							TY -= HLH;
							TH += HLH;
						}
						if(!(hlbits & (1<<DIR_DOWN)))
						{
							TH += HLH;
						}
						break;
				}
				switch(q)
				{
					case DIR_UP:
						al_draw_filled_rectangle(TX, TY, TX+TW-1, TY+HLH-1, c_sel);
						hlbits &= ~((1<<DIR_UPLEFT)|(1<<DIR_UPRIGHT));
						break;
					case DIR_DOWN:
						al_draw_filled_rectangle(TX, TY+TH-HLH, TX+TW-1, TY+TH-1, c_sel);
						hlbits &= ~((1<<DIR_DOWNLEFT)|(1<<DIR_DOWNRIGHT));
						break;
					case DIR_LEFT:
						al_draw_filled_rectangle(TX, TY, TX+HLW-1, TY+TH-1, c_sel);
						hlbits &= ~((1<<DIR_UPLEFT)|(1<<DIR_DOWNLEFT));
						break;
					case DIR_RIGHT:
						al_draw_filled_rectangle(TX+TW-HLW, TY, TX+TW-1, TY+TH-1, c_sel);
						hlbits &= ~((1<<DIR_UPRIGHT)|(1<<DIR_DOWNRIGHT));
						break;
					case DIR_UPLEFT:
					{
						u16 TX2 = TX-HLW, TWOFF = HLW;
						u16 TY2 = TY-HLH, THOFF = HLH;
						al_draw_filled_rectangle(TX2, TY, TX2+TWOFF+HLW-1, TY+HLH-1, c_sel);
						al_draw_filled_rectangle(TX, TY2, TX+HLW-1, TY2+THOFF+HLH-1, c_sel);
						break;
					}
					case DIR_UPRIGHT:
					{
						u16 TX2 = TX, TWOFF = HLW;
						u16 TY2 = TY-HLH, THOFF = HLH;
						al_draw_filled_rectangle(TX2+TW-HLW, TY, TX2+TWOFF+TW-1, TY+HLH-1, c_sel);
						al_draw_filled_rectangle(TX+TW-HLW, TY2, TX+TW-1, TY2+THOFF+HLH-1, c_sel);
						break;
					}
					case DIR_DOWNLEFT:
					{
						u16 TX2 = TX-HLW, TWOFF = HLW;
						u16 TY2 = TY, THOFF = HLH;
						al_draw_filled_rectangle(TX2, TY+TH-HLH, TX2+TWOFF+HLW-1, TY+TH-1, c_sel);
						al_draw_filled_rectangle(TX, TY2+TH-HLH, TX+HLW-1, TY2+THOFF+TH-1, c_sel);
						break;
					}
					case DIR_DOWNRIGHT:
					{
						u16 TX2 = TX, TWOFF = HLW;
						u16 TY2 = TY, THOFF = HLH;
						al_draw_filled_rectangle(TX2+TW-HLW, TY+TH-HLH, TX2+TWOFF+TW-1, TY+TH-1, c_sel);
						al_draw_filled_rectangle(TX+TW-HLW, TY2+TH-HLH, TX+TW-1, TY2+THOFF+TH-1, c_sel);
						break;
					}
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
		bool sel[9*9] = {0};
		for(u8 q = 0; q < 9*9; ++q) // Map selected
			sel[q] = (cells[q].flags & CFL_SELECTED) != 0;
		//
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
		for(u8 q = 0; q < 9*9; ++q) // Selected cell highlights
		{
			u16 X = x + ((q%9)*CELL_SZ), Y = y + ((q/9)*CELL_SZ),
				W = CELL_SZ, H = CELL_SZ;
			scale_pos(X,Y,W,H);
			u8 hlbits = 0;
			Cell const& c = cells[q];
			if(c.flags & CFL_SELECTED)
			{
				bool u,d,l,r;
				if(!(u = (q >= 9)) || !sel[q-9])
					hlbits |= 1<<DIR_UP;
				if(!(d = (q < 9*9-9)) || !sel[q+9])
					hlbits |= 1<<DIR_DOWN;
				if(!(l = (q % 9)) || !sel[q-1])
					hlbits |= 1<<DIR_LEFT;
				if(!(r = ((q % 9) < 8)) || !sel[q+1])
					hlbits |= 1<<DIR_RIGHT;
				if(!(u&&l) || !sel[q-9-1])
					hlbits |= 1<<DIR_UPLEFT;
				if(!(u&&r) || !sel[q-9+1])
					hlbits |= 1<<DIR_UPRIGHT;
				if(!(d&&l) || !sel[q+9-1])
					hlbits |= 1<<DIR_DOWNLEFT;
				if(!(d&&r) || !sel[q+9+1])
					hlbits |= 1<<DIR_DOWNRIGHT;
			}
			c.draw_sel(X, Y, W, H, hlbits);
		}
	}
	Grid::Grid(u16 X, u16 Y)
		: MouseObject(X,Y,9*CELL_SZ,9*CELL_SZ)
	{}
}

