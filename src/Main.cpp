#include "../Archipelago.h"
#include <iostream>
#include <filesystem>
#include "GUI.hpp"
#include "Font.hpp"
#include "SudokuGrid.hpp"

void log(string const& msg)
{
	std::cout << msg << std::endl;
}
void fail(string const& msg)
{
	log(msg);
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
Sudoku::Grid grid { GRID_X, GRID_Y };
#define BUTTON_X (CANVAS_W-32-96)
#define BUTTON_Y (32)
Button swap_btns[NUM_SCRS];
Screen curscr = SCR_SUDOKU;

map<Screen,vector<DrawnObject*>> gui_objects;
vector<vector<DrawnObject*>*> popups;
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
			swap_btns[q].flags |= FL_SELECTED;
		else swap_btns[q].flags &= ~FL_SELECTED;
	}
}

void build_gui()
{
	using namespace Sudoku;
	{ //BG, to allow 'clicking off'
		static ShapeRect bg(0,0,CANVAS_W,CANVAS_H,C_BG);
		bg.onMouse = mouse_killfocus;
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(&bg);
	}
	{ //Swap buttons
		#define ON_SWAP_BTN(b, scr) \
		b.onMouse = [](MouseEvent e) \
			{ \
				switch(e) \
				{ \
					case MOUSE_LCLICK: \
						if(curscr != scr) swap_screen(scr); \
						return MRET_TAKEFOCUS; \
				} \
				return b.handle_ev(e); \
			}
		swap_btns[0] = Button("Sudoku", BUTTON_X, BUTTON_Y);
		swap_btns[1] = Button("Archipelago", BUTTON_X, BUTTON_Y+32);
		swap_btns[2] = Button("Settings", BUTTON_X, BUTTON_Y+64);
		ON_SWAP_BTN(swap_btns[0], SCR_SUDOKU);
		ON_SWAP_BTN(swap_btns[1], SCR_CONNECT);
		ON_SWAP_BTN(swap_btns[2], SCR_SETTINGS);
		swap_btns[curscr].flags |= FL_SELECTED;
		for(int q = 0; q < NUM_SCRS; ++q)
			for(Button& b : swap_btns)
				gui_objects[static_cast<Screen>(q)].push_back(&b);
	}
	{ // Grid
		grid = Grid(GRID_X, GRID_Y);
		grid.onMouse = [](MouseEvent e)
			{
				u32 ret = MRET_OK;
				switch(e)
				{
					case MOUSE_LCLICK:
						ret |= MRET_TAKEFOCUS;
						if(!cur_input->shift())
							grid.deselect();
					[[fallthrough]];
					case MOUSE_LDOWN:
						if((ret & MRET_TAKEFOCUS) || cur_input->focused == &grid)
						{
							if(Cell* c = grid.get_hov())
								grid.select(c);
						}
						break;
					case MOUSE_LOSTFOCUS:
						grid.deselect();
						break;
				}
				return ret;
			};
		gui_objects[SCR_SUDOKU].push_back(&grid);
	}
}

void __run_mouse(vector<DrawnObject*>& vec)
{
	for (auto it = vec.rbegin(); it != vec.rend(); ++it)
		if((*it)->mouse())
			return;
}
InputState input_state;
void dlg_run(vector<DrawnObject*>& vec)
{
	al_get_mouse_state(cur_input);
	auto buttons = cur_input->buttons;
	auto os = cur_input->oldstate & 0x7;
	
	if(buttons & os) //Remember the held state
		buttons &= os;
	
	cur_input->buttons &= ~0x7;
	if(buttons & 0x1)
		cur_input->buttons |= 0x1;
	else if(buttons & 0x2)
		cur_input->buttons |= 0x2;
	else if(buttons & 0x4)
		cur_input->buttons |= 0x4;
	
	//Mouse coordinates are based on the 'display'
	//Scale down the x/y before running mouse logic:
	u16 mx = cur_input->x, my = cur_input->y;
	unscale_pos(mx,my);
	cur_input->x = mx;
	cur_input->y = my;
	//
	__run_mouse(vec);
	
	cur_input->oldstate = cur_input->buttons;
}

void dlg_draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	
	for(DrawnObject* obj : gui_objects[curscr])
		obj->draw();
	
	for(vector<DrawnObject*>* p : popups)
		for(DrawnObject* obj : *p)
			obj->draw();
	
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
		Sudoku::Cell* c = grid.get(0,q);
		c->val = q+1;
		if(q%2)
			c->flags |= CFL_GIVEN;
		
		c = grid.get(1,q);
		for(int p = 0; p <= q; ++p)
			c->center_marks[p] = true;
		
		c = grid.get(2,q);
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
			if(cur_input->focused)
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
	swap_btns[0].flags |= FL_SELECTED;
	//
	setup_allegro();
	log("Allegro initialized successfully");
	log("Building GUI...");
	build_gui();
	log("GUI built successfully, launch complete");
	
	init_grid();
	
	cur_input = &input_state;
	al_start_timer(timer);
	bool redraw = true;
	program_running = true;
	while(program_running)
	{
		if(redraw && al_is_event_queue_empty(events))
		{
			dlg_run(gui_objects[curscr]);
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
	fonts[FONT_BUTTON] = FontDef(-20, false, BOLD_NONE);
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
	{
		for(DrawnObject* obj : gui_objects[Screen(scr)])
			obj->on_disp_resize();
	}
	for(vector<DrawnObject*>* popup : popups)
		for(DrawnObject* obj : *popup)
			obj->on_disp_resize();
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
	display = al_create_display(CANVAS_W, CANVAS_H);
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
	
	init_fonts();
}

