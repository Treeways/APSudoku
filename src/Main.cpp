#include "../Archipelago.h"
#include <iostream>
#include <filesystem>
#include "GUI.hpp"
#include "Font.hpp"
#include "Config.hpp"
#include "SudokuGrid.hpp"

extern ALLEGRO_CONFIG* config;

void log(string const& msg)
{
	std::cout << "[LOG] " << msg << std::endl;
}
void error(string const& msg)
{
	std::cerr << "[ERROR] " << msg << std::endl;
}
void fail(string const& msg)
{
	std::cerr << "[FATAL] " << msg << std::endl;
	exit(1);
}

ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* canvas;
ALLEGRO_TIMER* timer;
ALLEGRO_EVENT_QUEUE* events;
ALLEGRO_EVENT_SOURCE event_source;

EntryMode mode = ENT_ANSWER;
EntryMode get_mode()
{
	if(cur_input->shift())
		return ENT_CORNER;
	if(cur_input->ctrl_cmd())
		return ENT_CENTER;
	if(cur_input->alt())
		return ENT_ANSWER;
	return mode;
}

#define GRID_X (32)
#define GRID_Y (32)
shared_ptr<Sudoku::Grid> grid;
#define BUTTON_X (CANVAS_W-32-96)
#define BUTTON_Y (32)
shared_ptr<Button> swap_btns[NUM_SCRS];
shared_ptr<RadioSet> difficulty;


Screen curscr = SCR_SUDOKU;

map<Screen,DrawContainer> gui_objects;
vector<DrawContainer*> popups;
bool settings_unsaved = false;

void swap_screen(Screen scr)
{
	if(curscr == scr)
		return;
	if(curscr == SCR_SETTINGS && settings_unsaved)
	{
		if(!pop_yn("Change pages", "Swap pages without saving your settings?"))
			return;
	}
	curscr = scr;
	for(auto q = 0; q < NUM_SCRS; ++q)
	{
		if(q == scr)
			swap_btns[q]->flags |= FL_SELECTED;
		else swap_btns[q]->flags &= ~FL_SELECTED;
	}
}

void build_gui()
{
	using namespace Sudoku;
	FontDef fd(-22, false, BOLD_NONE);
	{ // BG, to allow 'clicking off'
		shared_ptr<ShapeRect> bg = make_shared<ShapeRect>(0,0,CANVAS_W,CANVAS_H,C_BG);
		bg->onMouse = mouse_killfocus;
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(bg);
	}
	{ // Swap buttons
		#define ON_SWAP_BTN(b, scr) \
		b->onMouse = [](InputObject& ref,MouseEvent e) \
			{ \
				switch(e) \
				{ \
					case MOUSE_LCLICK: \
						if(curscr != scr) swap_screen(scr); \
						return MRET_TAKEFOCUS; \
				} \
				return ref.handle_ev(e); \
			}
		swap_btns[0] = make_shared<Button>("Sudoku", fd);
		swap_btns[1] = make_shared<Button>("Archipelago", fd);
		swap_btns[2] = make_shared<Button>("Settings", fd);
		ON_SWAP_BTN(swap_btns[0], SCR_SUDOKU);
		ON_SWAP_BTN(swap_btns[1], SCR_CONNECT);
		ON_SWAP_BTN(swap_btns[2], SCR_SETTINGS);
		swap_btns[curscr]->flags |= FL_SELECTED;
		shared_ptr<Column> sb_column = make_shared<Column>(BUTTON_X, BUTTON_Y);
		for(shared_ptr<Button> const& b : swap_btns)
			sb_column->add(b);
		
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(sb_column);
	}
	{ // Grid
		grid = make_shared<Grid>(GRID_X, GRID_Y);
		grid->onMouse = [](InputObject& ref,MouseEvent e)
			{
				u32 ret = MRET_OK;
				switch(e)
				{
					case MOUSE_LCLICK:
						ret |= MRET_TAKEFOCUS;
						if(!(cur_input->shift() || cur_input->ctrl_cmd()))
							grid->deselect();
					[[fallthrough]];
					case MOUSE_LDOWN:
						if((ret & MRET_TAKEFOCUS) || cur_input->focused == grid.get())
						{
							if(Cell* c = grid->get_hov())
								grid->select(c);
						}
						break;
					case MOUSE_LOSTFOCUS:
						grid->deselect();
						break;
				}
				return ret;
			};
		gui_objects[SCR_SUDOKU].push_back(grid);
	}
	const int R_GRID_X = GRID_X+(CELL_SZ*9)+2;
	{ // Difficulty / game buttons
		shared_ptr<Column> diff_column = make_shared<Column>(R_GRID_X+8,GRID_Y);
		diff_column->align = ALLEGRO_ALIGN_LEFT;
		
		difficulty = make_shared<RadioSet>(vector<string>({"Easy","Medium","Hard"}), FontDef(-20, false, BOLD_NONE));
		difficulty->select(0);
		difficulty->dis_proc = []()
			{
				return grid->active();
			};
		diff_column->add(difficulty);
		
		shared_ptr<Button> start_btn = make_shared<Button>("Start", fd);
		diff_column->add(start_btn);
		start_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(optional<u8> diff = difficulty->get_sel())
							grid->generate(*diff);
						break;
				}
				return ref.handle_ev(e);
			};
		start_btn->dis_proc = []()
			{
				return grid->active();
			};
		
		shared_ptr<Button> forfeit_btn = make_shared<Button>("Forfeit", fd);
		diff_column->add(forfeit_btn);
		forfeit_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(pop_yn("Forfeit", "Quit solving this puzzle?"))
							grid->clear();
						break;
				}
				return ref.handle_ev(e);
			};
		forfeit_btn->dis_proc = []()
			{
				return !grid->active();
			};
		
		shared_ptr<Button> check_btn = make_shared<Button>("Check", fd);
		diff_column->add(check_btn);
		check_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(!grid->filled())
							pop_inf("Unfinished","Not all cells are filled!");
						else if(grid->check())
						{
							pop_inf("Correct","Puzzle solved correctly! WIP!");
							grid->exit();
						}
						else pop_inf("Wrong", "Puzzle solution incorrect!");
						break;
				}
				return ref.handle_ev(e);
			};
		check_btn->dis_proc = []()
			{
				return !grid->active();
			};
		
		gui_objects[SCR_SUDOKU].push_back(diff_column);
	}
}

void dlg_draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	
	gui_objects[curscr].draw();
	for(DrawContainer* p : popups)
		p->draw();
	
	al_restore_state(&oldstate);
}

void dlg_render()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_backbuffer(display);
	clear_a5_bmp(C_BG);
	
	al_draw_bitmap(canvas, 0, 0, 0);
	
	al_flip_display();
	
	update_scale();
	
	al_restore_state(&oldstate);
}

void init_grid()
{
	//!TODO this is just a test grid inserted on launch
	for(int q = 0; q < 9; ++q)
	{
		Sudoku::Cell* c = grid->get(0,q);
		c->val = q+1;
		if(q%2)
			c->flags |= CFL_GIVEN;
		
		c = grid->get(1,q);
		for(int p = 0; p <= q; ++p)
			c->center_marks[p] = true;
		
		c = grid->get(2,q);
		for(int p = 0; p <= q; ++p)
			c->corner_marks[p] = true;
	}
}
void setup_allegro();
bool program_running = true;
void run_events(bool& redraw)
{
	ALLEGRO_EVENT ev;
	al_wait_for_event(events, &ev);
	
	switch(ev.type)
	{
		case ALLEGRO_EVENT_TIMER:
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			redraw = true;
			break;
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			program_running = false;
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			al_acknowledge_resize(display);
			on_resize();
			redraw = true;
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
		case ALLEGRO_EVENT_KEY_UP:
		case ALLEGRO_EVENT_KEY_CHAR:
			if(cur_input && cur_input->focused)
				cur_input->focused->key_event(ev);
			break;
	}
}
bool events_empty()
{
	return al_is_event_queue_empty(events);
}
int main(int argc, char **argv)
{
	log("Program Start");
	//
	string exepath(argv[0]);
	string wdir;
	auto ind = exepath.find_last_of("/\\");
	if(ind == string::npos)
		wdir = "";
	else wdir = exepath.substr(0,1+ind);
	std::filesystem::current_path(wdir);
	log("Running in dir: \"" + wdir + "\"");
	//
	setup_allegro();
	log("Allegro initialized successfully");
	log("Building GUI...");
	build_gui();
	log("GUI built successfully, launch complete");
	
	init_grid();
	
	InputState input_state;
	cur_input = &input_state;
	al_start_timer(timer);
	bool redraw = true;
	program_running = true;
	while(program_running)
	{
		if(redraw && events_empty())
		{
			gui_objects[curscr].run();
			dlg_draw();
			dlg_render();
			redraw = false;
		}
		run_events(redraw);
	}
	
	return 0;
}

void init_fonts()
{
	fonts[FONT_ANSWER] = FontDef(-32, false, BOLD_NONE);
	fonts[FONT_MARKING5] = FontDef(-12, true, BOLD_NONE);
	fonts[FONT_MARKING6] = FontDef(-11, true, BOLD_NONE);
	fonts[FONT_MARKING7] = FontDef(-10, true, BOLD_NONE);
	fonts[FONT_MARKING8] = FontDef(-9, true, BOLD_NONE);
	fonts[FONT_MARKING9] = FontDef(-7, true, BOLD_NONE);
}

void on_resize()
{
	int w = render_resx, h = render_resy;
	update_scale();
	if(w == render_resx && h == render_resy)
		return; // no resize
	al_destroy_bitmap(canvas);
	canvas = al_create_bitmap(render_resx, render_resy);
	if(!canvas)
		fail("Failed to recreate canvas bitmap!");
	scale_fonts();
	
	for(int scr = 0; scr < NUM_SCRS; ++scr)
		gui_objects[Screen(scr)].on_disp_resize();
	for(DrawContainer* popup : popups)
		popup->on_disp_resize();
}

void save_cfg()
{
	if(!al_save_config_file("APSudoku.cfg", config))
		error("Failed to save config file! Settings may not be savable!");
}
void default_config()
{
	set_config_col("Test", "BGC", C_LGRAY);
}
void refresh_configs()
{
	if(auto c = get_config_col("Test", "BGC"))
		C_BG = *c;
}
void setup_allegro()
{
	if(!al_init())
		fail("Failed to initialize Allegro!");
	
	if(!al_init_font_addon())
		fail("Failed to initialize Allegro Fonts!");
	
	if(!al_init_ttf_addon())
		fail("Failed to initialize Allegro TTF!");
	
	if(!al_init_primitives_addon())
		fail("Failed to initialize Allegro Primitives!");
	
	al_install_mouse();
	al_install_keyboard();
	
	al_set_new_display_flags(ALLEGRO_RESIZABLE);
	display = al_create_display(CANVAS_W*2, CANVAS_H*2);
	if(!display)
		fail("Failed to create display!");
	
	al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE);
	canvas = al_create_bitmap(CANVAS_W, CANVAS_H);
	if(!canvas)
		fail("Failed to create canvas bitmap!");
	
	timer = al_create_timer(ALLEGRO_BPS_TO_SECS(60));
	if(!timer)
		fail("Failed to create timer!");
	
	events = al_create_event_queue();
	if(!events)
		fail("Failed to create event queue!");
	al_init_user_event_source(&event_source);
	al_register_event_source(events, &event_source);
	al_register_event_source(events, al_get_display_event_source(display));
	al_register_event_source(events, al_get_timer_event_source(timer));
	al_register_event_source(events, al_get_keyboard_event_source());
	
	al_set_window_constraints(display, CANVAS_W, CANVAS_H, 0, 0);
	al_apply_window_constraints(display, true);
	
	config = al_create_config();
	default_config();
	if(ALLEGRO_CONFIG* cfg = al_load_config_file("APSudoku.cfg"))
	{
		al_merge_config_into(config, cfg);
		al_destroy_config(cfg);
	}
	save_cfg();
	refresh_configs();
	on_resize();
	init_fonts();
}

