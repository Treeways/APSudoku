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
	void Cell::draw(u16 x, u16 y) const
	{
		al_draw_filled_rectangle(x, y, x+CELL_SZ-1, y+CELL_SZ-1, c_bg);
		al_draw_rectangle(x, y, x+CELL_SZ-1, y+CELL_SZ-1, c_border, 0.5);
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
					c->flags |= FL_INVALID;
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
	
	void Grid::clear_invalid()
	{
		for(Cell& c : cells)
		{
			c.flags &= ~FL_INVALID;
		}
	}
	void Grid::draw(u16 x, u16 y) const
	{
		for(u8 q = 0; q < 9*9; ++q)
			cells[q].draw(x + ((q%9)*CELL_SZ), y + ((q/9)*CELL_SZ));
		for(u8 q = 0; q < 9; ++q)
		{
			u16 tx = x + ((q%3)*(CELL_SZ*3));
			u16 ty = y + ((q/3)*(CELL_SZ*3));
			al_draw_rectangle(tx , ty, tx+CELL_SZ*3-1, ty+CELL_SZ*3-1, C_BLACK, 2);
		}
	}
}

