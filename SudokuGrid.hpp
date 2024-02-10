#pragma once

#include "SudokuGUI.hpp"

namespace Sudoku
{
	// Return results for checking the state of the puzzle
	typedef u8 PuzzleState;
	#define PS_SOLVED     0b0000 //All cells are filled correctly
	#define PS_INCOMPLETE 0b0001 //At least one cell is empty
	#define PS_INVALID    0b0010 //Two cells conflict with one another
	#define PS_WRONG      0b0100 //A cell conflicts with the solution
	
	// Flags for cells
	#define CFL_GIVEN      0b0001
	#define CFL_INVALID    0b0010
	#define CFL_SELECTED   0b0100
	
	struct Cell;
	struct Region;
	struct Grid;
	
	#define CELL_SZ 32
	
	struct Cell
	{
		u8 solution = 0;
		u8 val = 0;
		bool center_marks[9] = {0,0,0,0,0,0,0,0,0};
		bool corner_marks[9] = {0,0,0,0,0,0,0,0,0};
		u8 flags = 0;
		
		ALLEGRO_COLOR c_bg = C_WHITE;
		ALLEGRO_COLOR c_border = C_LGRAY;
		ALLEGRO_COLOR c_text = C_TXT;
		ALLEGRO_COLOR c_sel = C_HIGHLIGHT;
		ALLEGRO_COLOR c_sel2 = C_HIGHLIGHT2;
		
		void clear();
		void clear_marks();
		void clear_marks(EntryMode m);
		EntryMode current_mode() const;
		
		void draw(u16 x, u16 y, u16 w, u16 h) const;
		void draw_sel(u16 x, u16 y, u16 w, u16 h, u8 hlbits, bool special) const;
		
		void enter(EntryMode m, u8 val);
	};
	struct Region
	{
		Cell* cells[9] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};
		PuzzleState check() const;
	private:
		Region() = default;
		friend struct Grid;
	};
	struct Grid : public InputObject
	{
		Cell cells[9*9];
		
		Region get_row(u8 ind);
		Region get_col(u8 ind);
		Region get_box(u8 ind);
		Cell* get(u8 row, u8 col);
		Cell* get_hov();
		optional<u8> find(Cell* c);
		
		void clear_invalid();
		void draw() const override;
		void key_event(ALLEGRO_EVENT const& ev) override;
		
		void deselect();
		void select(Cell* c);
		set<Cell*> get_selected() const {return selected;}
		
		Grid(u16 X, u16 Y);
	private:
		set<Cell*> selected;
		Cell* focus_cell;
	};
	
	class sudoku_exception : public std::exception
	{
	public:
		const char * what() const noexcept override {
			return msg.c_str();
		}
		sudoku_exception(std::string const& msg) : msg(msg)
		{}
	private:
		std::string msg;
	};
}

