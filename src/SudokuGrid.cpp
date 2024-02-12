#include "SudokuGrid.hpp"

namespace Sudoku
{
	void Cell::clear()
	{
		*this = Cell();
	}
	void Cell::clear_marks()
	{
		clear_marks(current_mode());
	}
	void Cell::clear_marks(EntryMode m)
	{
		switch(m)
		{
			case ENT_ANSWER:
				val = 0;
				break;
			case ENT_CENTER:
				memset(center_marks, 0, sizeof(center_marks));
				break;
			case ENT_CORNER:
				memset(corner_marks, 0, sizeof(corner_marks));
				break;
		}
	}
	EntryMode Cell::current_mode() const
	{
		if(val)
			return ENT_ANSWER;
		for(int q = 0; q < 9; ++q)
			if(center_marks[q])
				return ENT_CENTER;
		return ENT_CORNER;
	}
	void Cell::draw(u16 X, u16 Y, u16 W, u16 H) const
	{
		al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, Color(C_CELL_BG));
		al_draw_rectangle(X, Y, X+W-1, Y+H-1, Color(C_CELL_BORDER), 0.5);
		
		bool given = (flags & CFL_GIVEN);
		if(val)
		{
			string text = to_string(val);
			ALLEGRO_FONT* f = fonts[FONT_ANSWER].get();
			int tx = (X+W/2);
			int ty = (Y+H/2)-(al_get_font_line_height(f)/2);
			al_draw_text(f, Color(given ? C_CELL_GIVEN : C_CELL_TEXT), tx, ty, ALLEGRO_ALIGN_CENTRE, text.c_str());
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
				ALLEGRO_FONT* f = fonts[font].get();
				int tx = (X+W/2);
				int ty = (Y+H/2)-(al_get_font_line_height(f)/2);
				al_draw_text(f, Color(C_CELL_TEXT), tx, ty, ALLEGRO_ALIGN_CENTRE, cen_marks.c_str());
			}
			if(!crn_marks.empty())
			{
				auto font = FONT_MARKING5;
				ALLEGRO_FONT* f = fonts[font].get();
				u16 vx = 6, vy = 6;
				scale_pos(vx, vy);
				int xs[] = {vx,W-vx,vx,W-vx,W/2,W/2,vx,W-vx,W/2-4};
				int ys[] = {vy,vy,H-vy,H-vy,vy,H-vy,H/2,H/2,H/2-4};
				auto fh = al_get_font_line_height(f);
				char buf[2] = {0,0};
				for(int q = 0; q < 9 && q < crn_marks.size(); ++q)
				{
					buf[0] = crn_marks[q];
					al_draw_text(f, Color(C_CELL_TEXT), X+xs[q], Y+ys[q] - fh/2, ALLEGRO_ALIGN_CENTRE, buf);
				}
			}
		}
	}
	void Cell::draw_sel(u16 X, u16 Y, u16 W, u16 H, u8 hlbits, bool special) const
	{
		u16 HLW = 4, HLH = 4;
		Color col = C_HIGHLIGHT;
		if(special)
		{
			col = C_HIGHLIGHT2;
			hlbits = ~0;
		}
		if(!hlbits)
			return;
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
						al_draw_filled_rectangle(TX, TY, TX+TW-1, TY+HLH-1, col);
						hlbits &= ~((1<<DIR_UPLEFT)|(1<<DIR_UPRIGHT));
						break;
					case DIR_DOWN:
						al_draw_filled_rectangle(TX, TY+TH-HLH, TX+TW-1, TY+TH-1, col);
						hlbits &= ~((1<<DIR_DOWNLEFT)|(1<<DIR_DOWNRIGHT));
						break;
					case DIR_LEFT:
						al_draw_filled_rectangle(TX, TY, TX+HLW-1, TY+TH-1, col);
						hlbits &= ~((1<<DIR_UPLEFT)|(1<<DIR_DOWNLEFT));
						break;
					case DIR_RIGHT:
						al_draw_filled_rectangle(TX+TW-HLW, TY, TX+TW-1, TY+TH-1, col);
						hlbits &= ~((1<<DIR_UPRIGHT)|(1<<DIR_DOWNRIGHT));
						break;
					case DIR_UPLEFT:
					{
						u16 TX2 = TX-HLW, TWOFF = HLW;
						u16 TY2 = TY-HLH, THOFF = HLH;
						al_draw_filled_rectangle(TX2, TY, TX2+TWOFF+HLW-1, TY+HLH-1, col);
						al_draw_filled_rectangle(TX, TY2, TX+HLW-1, TY2+THOFF+HLH-1, col);
						break;
					}
					case DIR_UPRIGHT:
					{
						u16 TX2 = TX, TWOFF = HLW;
						u16 TY2 = TY-HLH, THOFF = HLH;
						al_draw_filled_rectangle(TX2+TW-HLW, TY, TX2+TWOFF+TW-1, TY+HLH-1, col);
						al_draw_filled_rectangle(TX+TW-HLW, TY2, TX+TW-1, TY2+THOFF+HLH-1, col);
						break;
					}
					case DIR_DOWNLEFT:
					{
						u16 TX2 = TX-HLW, TWOFF = HLW;
						u16 TY2 = TY, THOFF = HLH;
						al_draw_filled_rectangle(TX2, TY+TH-HLH, TX2+TWOFF+HLW-1, TY+TH-1, col);
						al_draw_filled_rectangle(TX, TY2+TH-HLH, TX+HLW-1, TY2+THOFF+TH-1, col);
						break;
					}
					case DIR_DOWNRIGHT:
					{
						u16 TX2 = TX, TWOFF = HLW;
						u16 TY2 = TY, THOFF = HLH;
						al_draw_filled_rectangle(TX2+TW-HLW, TY+TH-HLH, TX2+TWOFF+TW-1, TY+TH-1, col);
						al_draw_filled_rectangle(TX+TW-HLW, TY2+TH-HLH, TX+TW-1, TY2+THOFF+TH-1, col);
						break;
					}
				}
			}
		}
	}
	
	void Cell::enter(EntryMode m, u8 v)
	{
		if(flags & CFL_GIVEN)
			return;
		switch(m)
		{
			case ENT_ANSWER:
				if(val == v)
					val = 0;
				else val = v;
				break;
			case ENT_CENTER:
				center_marks[v-1] = !center_marks[v-1];
				break;
			case ENT_CORNER:
				corner_marks[v-1] = !corner_marks[v-1];
				break;
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

	int Grid::sel_style = STYLE_OVER;
	
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
		u16 X = x, Y = y, W = CELL_SZ, H = CELL_SZ;
		scale_pos(X,Y,W,H);
		u8 col = (cur_input->x - X) / W;
		u8 row = (cur_input->y - Y) / H;
		return get(row,col);
	}
	optional<u8> Grid::find(Cell* c)
	{
		for(u8 q = 0; q < 9*9; ++q)
			if(c == &cells[q])
				return q;
		return nullopt;
	}
	
	bool Grid::filled() const
	{
		for(Cell const& c : cells)
			if(!c.val)
				return false;
		return true;
	}
	bool Grid::check() const
	{
		for(Cell const& c : cells)
			if(c.solution != c.val)
				return false;
		return true;
	}
	
	void Grid::clear()
	{
		for(Cell& c : cells)
			c.clear();
	}
	void Grid::exit()
	{
		for(Cell& c : cells)
			c.solution = 0;
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
		optional<u8> focus_ind;
		for(u8 q = 0; q < 9*9; ++q) // Map selected
			sel[q] = (cells[q].flags & CFL_SELECTED) != 0;
		//
		#define DRAW_FOCUS() \
		if(focus_ind) \
		{ \
			u8 q = *focus_ind; \
			u16 X = x + ((q%9)*CELL_SZ), \
				Y = y + ((q/9)*CELL_SZ), \
				W = CELL_SZ, H = CELL_SZ; \
			scale_pos(X,Y,W,H); \
			focus_cell->draw_sel(X, Y, W, H, 0, true); \
		}
		//
		for(u8 q = 0; q < 9*9; ++q) // Cell draws
		{
			u16 X = x + ((q%9)*CELL_SZ),
				Y = y + ((q/9)*CELL_SZ),
				W = CELL_SZ, H = CELL_SZ;
			scale_pos(X,Y,W,H);
			Cell const& c = cells[q];
			if(&c == focus_cell)
				focus_ind = q;
			c.draw(X, Y, W, H);
		}
		for(u8 q = 0; q < 9; ++q) // 3x3 box thicker borders
		{
			u16 X = x + ((q%3)*(CELL_SZ*3)),
				Y = y + ((q/3)*(CELL_SZ*3)),
				W = CELL_SZ*3, H = CELL_SZ*3;
			scale_pos(X,Y,W,H);
			al_draw_rectangle(X, Y, X+W-1, Y+H-1, Color(C_REGION_BORDER), 2);
		}
		if(sel_style == STYLE_UNDER) DRAW_FOCUS()
		for(u8 q = 0; q < 9*9; ++q) // Selected cell highlights
		{
			u16 X = x + ((q%9)*CELL_SZ),
				Y = y + ((q/9)*CELL_SZ),
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
			c.draw_sel(X, Y, W, H, hlbits, false);
		}
		if(sel_style == STYLE_OVER) DRAW_FOCUS()
	}
	void Grid::key_event(ALLEGRO_EVENT const& ev)
	{
		bool shift = cur_input->shift();
		bool ctrl_cmd = cur_input->ctrl_cmd();
		bool alt = cur_input->alt();
		switch(ev.type)
		{
			case ALLEGRO_EVENT_KEY_DOWN:
			{
				u8 input_num = 0;
				switch(ev.keyboard.keycode)
				{
					case ALLEGRO_KEY_1:
					case ALLEGRO_KEY_2:
					case ALLEGRO_KEY_3:
					case ALLEGRO_KEY_4:
					case ALLEGRO_KEY_5:
					case ALLEGRO_KEY_6:
					case ALLEGRO_KEY_7:
					case ALLEGRO_KEY_8:
					case ALLEGRO_KEY_9:
					{
						input_num = ev.keyboard.keycode-ALLEGRO_KEY_0;
						break;
					}
					case ALLEGRO_KEY_PAD_1:
					case ALLEGRO_KEY_PAD_2:
					case ALLEGRO_KEY_PAD_3:
					case ALLEGRO_KEY_PAD_4:
					case ALLEGRO_KEY_PAD_5:
					case ALLEGRO_KEY_PAD_6:
					case ALLEGRO_KEY_PAD_7:
					case ALLEGRO_KEY_PAD_8:
					case ALLEGRO_KEY_PAD_9:
					{
						input_num = ev.keyboard.keycode-ALLEGRO_KEY_PAD_0;
						break;
					}
					case ALLEGRO_KEY_UP: case ALLEGRO_KEY_W:
					case ALLEGRO_KEY_DOWN: case ALLEGRO_KEY_S:
					case ALLEGRO_KEY_LEFT: case ALLEGRO_KEY_A:
					case ALLEGRO_KEY_RIGHT: case ALLEGRO_KEY_D:
						if(optional<u8> o_ind = find(focus_cell))
						{
							u8 ind = *o_ind;
							if(!shift)
								deselect();
							switch(ev.keyboard.keycode)
							{
								case ALLEGRO_KEY_UP: case ALLEGRO_KEY_W:
									if(ind >= 9)
										ind -= 9;
									break;
								case ALLEGRO_KEY_DOWN: case ALLEGRO_KEY_S:
									if(ind <= 9*9-9)
										ind += 9;
									break;
								case ALLEGRO_KEY_LEFT: case ALLEGRO_KEY_A:
									if(ind % 9)
										--ind;
									break;
								case ALLEGRO_KEY_RIGHT: case ALLEGRO_KEY_D:
									if((ind % 9) < 8)
										++ind;
									break;
							}
							select(&cells[ind]);
						}
						break;
					case ALLEGRO_KEY_PAD_MINUS:
						//!TODO remove cheat
						for(Cell& c : cells)
							c.val = c.solution;
						break;
					case ALLEGRO_KEY_TAB:
						if(!mode_mod())
							mode = EntryMode((mode+1)%NUM_ENT);
						break;
					case ALLEGRO_KEY_DELETE:
					case ALLEGRO_KEY_BACKSPACE:
						if(!selected.empty())
						{
							EntryMode m = NUM_ENT;
							for(Cell* c : selected)
							{
								if(c->flags & CFL_GIVEN)
									continue;
								EntryMode m2 = c->current_mode();
								if(m2 < m)
									m = m2;
							}
							for(Cell* c : selected)
							{
								if(c->flags & CFL_GIVEN)
									continue;
								c->clear_marks(m);
							}
						}
						break;
				}
				if(input_num && !selected.empty())
				{
					auto m = get_mode();
					for(Cell* c : selected)
						c->enter(m, input_num);
				}
				break;
			}
			case ALLEGRO_EVENT_KEY_UP:
				break;
			case ALLEGRO_EVENT_KEY_CHAR:
				break;
		}
	}
	
	void Grid::deselect()
	{
		for(Cell* c : selected)
			c->flags &= ~CFL_SELECTED;
		selected.clear();
		focus_cell = nullptr;
	}
	void Grid::select(Cell* c)
	{
		if(!find(c))
			throw sudoku_exception("Cannot select cell not from this grid!");
		selected.insert(c);
		c->flags |= CFL_SELECTED;
		focus_cell = c;
	}
	
	bool Grid::active() const
	{
		return cells[0].solution != 0;
	}
	void Grid::generate(u8 diff)
	{
		//!TODO Generate random puzzles
		//Placeholder: test puzzle for each difficulty
		u8 const* s1;
		u8 const* s2;
		switch(diff)
		{
			case DIFF_EASY:
			{
				static u8 sol[] =
				{
					9, 8, 5,  2, 7, 1,  4, 3, 6,
					3, 4, 2,  9, 6, 5,  8, 1, 7,
					1, 7, 6,  3, 4, 8,  9, 5, 2,

					5, 3, 1,  8, 2, 4,  7, 6, 9,
					7, 9, 4,  5, 3, 6,  2, 8, 1,
					2, 6, 8,  1, 9, 7,  3, 4, 5,

					6, 2, 7,  4, 1, 3,  5, 9, 8,
					4, 5, 9,  6, 8, 2,  1, 7, 3,
					8, 1, 3,  7, 5, 9,  6, 2, 4,
				};
				static u8 start[] =
				{
					9, 0, 0,  2, 7, 1,  4, 3, 6,
					0, 4, 2,  9, 6, 0,  8, 1, 7,
					1, 7, 6,  0, 4, 8,  0, 0, 0,

					5, 0, 1,  0, 0, 4,  0, 0, 9,
					0, 9, 4,  0, 0, 6,  0, 8, 1,
					2, 0, 8,  0, 0, 0,  0, 0, 5,

					6, 0, 7,  4, 0, 3,  5, 9, 0,
					4, 5, 0,  6, 8, 0,  1, 0, 0,
					8, 0, 3,  7, 0, 9,  6, 0, 4,
				};
				s1 = sol; s2 = start;
				break;
			}
			case DIFF_NORMAL:
			{
				static u8 sol[] =
				{
					9, 8, 6,  2, 5, 3,  4, 7, 1,
					7, 3, 5,  1, 9, 4,  6, 2, 8,
					1, 2, 4,  6, 8, 7,  5, 9, 3,

					8, 5, 1,  9, 3, 2,  7, 6, 4,
					2, 6, 3,  7, 4, 5,  8, 1, 9,
					4, 7, 9,  8, 1, 6,  2, 3, 5,

					3, 1, 2,  4, 6, 8,  9, 5, 7,
					6, 9, 8,  5, 7, 1,  3, 4, 2,
					5, 4, 7,  3, 2, 9,  1, 8, 6,
				};
				static u8 start[] =
				{
					9, 8, 0,  0, 5, 3,  0, 0, 1,
					0, 0, 0,  0, 9, 4,  0, 0, 0,
					0, 0, 4,  6, 8, 7,  0, 0, 3,

					0, 5, 1,  0, 0, 2,  0, 6, 4,
					2, 0, 0,  7, 0, 5,  0, 1, 9,
					0, 7, 0,  0, 0, 0,  0, 0, 0,

					3, 1, 0,  4, 0, 8,  9, 0, 0,
					0, 0, 8,  5, 0, 1,  0, 4, 2,
					0, 0, 7,  0, 0, 0,  0, 0, 6,
				};
				s1 = sol; s2 = start;
				break;
			}
			case DIFF_HARD:
			{
				static u8 sol[] =
				{
					6, 4, 8,  3, 5, 9,  7, 1, 2,
					2, 7, 1,  4, 8, 6,  9, 5, 3,
					9, 3, 5,  7, 1, 2,  6, 4, 8,

					1, 6, 2,  5, 7, 4,  3, 8, 9,
					3, 5, 9,  8, 2, 1,  4, 7, 6,
					4, 8, 7,  6, 9, 3,  5, 2, 1,

					7, 2, 6,  9, 4, 8,  1, 3, 5,
					8, 9, 4,  1, 3, 5,  2, 6, 7,
					5, 1, 3,  2, 6, 7,  8, 9, 4,
				};
				static u8 start[] =
				{
					0, 0, 0,  0, 0, 9,  0, 1, 0,
					0, 0, 0,  0, 0, 0,  9, 5, 0,
					0, 3, 0,  7, 1, 0,  0, 0, 0,

					0, 6, 2,  0, 0, 0,  3, 8, 0,
					0, 0, 0,  0, 0, 0,  0, 0, 0,
					4, 0, 7,  6, 9, 0,  0, 0, 0,

					7, 0, 0,  9, 4, 0,  1, 0, 0,
					0, 0, 0,  1, 0, 0,  0, 0, 7,
					0, 0, 3,  0, 6, 0,  0, 0, 4,
				};
				s1 = sol; s2 = start;
				break;
			}
		}
		for(u8 ind = 0; ind < 9*9; ++ind)
		{
			cells[ind].clear();
			cells[ind].solution = s1[ind];
			cells[ind].val = s2[ind];
			if(s2[ind])
				cells[ind].flags |= CFL_GIVEN;
		}
	}
	
	u32 Grid::handle_ev(MouseEvent e)
	{
		u32 ret = MRET_OK;
		switch(e)
		{
			case MOUSE_LCLICK:
				ret |= MRET_TAKEFOCUS;
				if(!(cur_input->shift() || cur_input->ctrl_cmd()))
					deselect();
			[[fallthrough]];
			case MOUSE_LDOWN:
				if((ret & MRET_TAKEFOCUS) || focused())
				{
					if(Cell* c = get_hov())
						select(c);
				}
				break;
			case MOUSE_LOSTFOCUS:
				deselect();
				break;
		}
		return ret;
	}
	
	Grid::Grid(u16 X, u16 Y)
		: InputObject(X,Y,9*CELL_SZ,9*CELL_SZ)
	{}
}

