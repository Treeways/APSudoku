#include "Archipelago.h"
#include <iostream>
#include <filesystem>
#include "SudokuGUI.hpp"
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

#define CANVAS_W 640
#define CANVAS_H 352
ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* canvas;
ALLEGRO_TIMER* timer;
ALLEGRO_EVENT_QUEUE* events;
ALLEGRO_EVENT_SOURCE event_source;
ALLEGRO_FONT* fonts[NUM_FONTS];
#define GRID_X (32)
#define GRID_Y (32)
Sudoku::Grid grid { GRID_X, GRID_Y };
#define BUTTON_X (CANVAS_W-32-96)
#define BUTTON_Y (32)
Button swap_btns[NUM_SCRS];
Screen curscr = SCR_SUDOKU;

map<Screen,vector<DrawnObject*>> gui_objects;

void clear_a5_bmp(ALLEGRO_COLOR col, ALLEGRO_BITMAP* bmp = nullptr)
{
	if(bmp)
	{
		ALLEGRO_STATE old_state;
		al_store_state(&old_state, ALLEGRO_STATE_TARGET_BITMAP);
		
		al_set_target_bitmap(bmp);
		al_clear_to_color(col);
		
		al_restore_state(&old_state);
	}
	else al_clear_to_color(col);
}

void swap_screen(Screen scr)
{
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
	{ //Swap buttons
		#define SWAP_EVENT(b, scr) \
		b.onMouse = [](MouseEvent e) \
			{ \
				switch(e) \
				{ \
					case MOUSE_LCLICK: \
						if(curscr != scr) swap_screen(scr); \
						return MRET_TAKEFOCUS; \
					case MOUSE_HOVER_ENTER: \
						b.flags |= FL_HOVERED; \
						break; \
					case MOUSE_HOVER_EXIT: \
						b.flags &= ~FL_HOVERED; \
						break; \
				} \
				return MRET_OK; \
			}
		swap_btns[0] = Button("Sudoku", BUTTON_X, BUTTON_Y);
		swap_btns[1] = Button("Archipelago", BUTTON_X, BUTTON_Y+32);
		SWAP_EVENT(swap_btns[0], SCR_SUDOKU);
		SWAP_EVENT(swap_btns[1], SCR_CONNECT);
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
						grid.deselect();
					[[fallthrough]];
					case MOUSE_LDOWN:
						if((ret & MRET_TAKEFOCUS) || mouse_state.focused == &grid)
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
	{ //BG, to allow 'clicking off'
		static MouseObject bg(0,0,CANVAS_W,CANVAS_H);
		bg.onMouse = [](MouseEvent e)
			{
				if(e == MOUSE_LCLICK && mouse_state.focused)
				{
					u32 newret = mouse_state.focused->onMouse(MOUSE_LOSTFOCUS);
					if(!(newret & MRET_TAKEFOCUS))
						mouse_state.focused = nullptr;
				}
				return MRET_OK;
			};
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(&bg);
	}
}

void __run_mouse()
{
	for(DrawnObject* obj : gui_objects[NUM_SCRS])
		if(obj->mouse())
			return;
	for(DrawnObject* obj : gui_objects[curscr])
		if(obj->mouse())
			return;
}
MouseState mouse_state;
void run()
{
	al_get_mouse_state(&mouse_state);
	auto buttons = mouse_state.buttons;
	auto os = mouse_state.oldstate & 0x7;
	
	if(buttons & os) //Remember the held state
		buttons &= os;
	
	mouse_state.buttons &= ~0x7;
	if(buttons & 0x1)
		mouse_state.buttons |= 0x1;
	else if(buttons & 0x2)
		mouse_state.buttons |= 0x2;
	else if(buttons & 0x4)
		mouse_state.buttons |= 0x4;
	
	//!TODO Mouse coords are based on the display, not the canvas. Scale them before running the mouse!
	__run_mouse();
	
	mouse_state.oldstate = mouse_state.buttons;
}

void draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	
	for(DrawnObject* obj : gui_objects[NUM_SCRS])
		obj->draw();
	for(DrawnObject* obj : gui_objects[curscr])
		obj->draw();
	
	al_restore_state(&oldstate);
}
void render()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_backbuffer(display);
	clear_a5_bmp(C_BG);
	
	int resx = al_get_display_width(display);
	int resy = al_get_display_height(display);
	
	double xscale = resx/CANVAS_W, yscale = resy/CANVAS_H;
	double scale = std::min(xscale,yscale);
	int scaled_w = int(CANVAS_W*scale), scaled_h = int(CANVAS_H*scale);
	int xoff = (resx-scaled_w)/2, yoff = (resy-scaled_h)/2;
	
	al_draw_scaled_bitmap(canvas, 0, 0, CANVAS_W, CANVAS_H, xoff, yoff, scaled_w, scaled_h, 0);
	
	al_flip_display();
	
	al_restore_state(&oldstate);
}

void init_fonts();
void setup_allegro();
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
	
	al_start_timer(timer);
	bool redraw = true;
	bool running = true;
	while(running)
	{
		if(redraw && al_is_event_queue_empty(events))
		{
			run();
			draw();
			render();
			redraw = false;
		}
		
		ALLEGRO_EVENT ev;
		al_wait_for_event(events, &ev);
		
		switch(ev.type)
		{
			case ALLEGRO_EVENT_TIMER:
				redraw = true;
				break;
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				running = false;
				break;
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
				al_acknowledge_resize(display);
				break;
		}
	}
	
	return 0;
}

void MouseObject::unhover()
{
	if(!onMouse) return;
	
	if(mouseflags&MFL_HASMOUSE)
	{
		mouseflags &= ~MFL_HASMOUSE;
		process(onMouse(MOUSE_HOVER_EXIT));
	}
	if(mouse_state.hovered == this)
		mouse_state.hovered = nullptr;
}
void MouseObject::process(u32 retcode)
{
	auto& m = mouse_state;
	if(retcode & MRET_TAKEFOCUS)
	{
		if(m.focused && m.focused != this)
		{
			u32 newret = m.focused->onMouse(MOUSE_LOSTFOCUS);
			if(!(newret & MRET_TAKEFOCUS))
				m.focused = this;
		}
		else m.focused = this;
	}
}
bool MouseObject::mouse()
{
	auto& m = mouse_state;
	bool inbounds = (m.x >= x && m.x < x+w) && (m.y >= y && m.y < y+h);
	if(!onMouse)
		return inbounds;
	if(inbounds)
	{
		if(m.hovered && m.hovered != this)
			m.hovered->unhover();
		m.hovered = this;
		if(!(mouseflags&MFL_HASMOUSE))
		{
			mouseflags |= MFL_HASMOUSE;
			process(onMouse(MOUSE_HOVER_ENTER));
		}
		
		if(m.lrelease())
			process(onMouse(MOUSE_LRELEASE));
		else if(m.lrelease())
			process(onMouse(MOUSE_RRELEASE));
		else if(m.lrelease())
			process(onMouse(MOUSE_MRELEASE));
		
		if(m.lclick())
			process(onMouse(MOUSE_LCLICK));
		else if(m.rclick())
			process(onMouse(MOUSE_RCLICK));
		else if(m.mclick())
			process(onMouse(MOUSE_MCLICK));
		else if(m.buttons & 0x1)
			process(onMouse(MOUSE_LDOWN));
		else if(m.buttons & 0x2)
			process(onMouse(MOUSE_RDOWN));
		else if(m.buttons & 0x4)
			process(onMouse(MOUSE_MDOWN));
		
		return true;
	}
	else
	{
		unhover();
		return false;
	}
}

void Button::draw() const
{
	ALLEGRO_COLOR const* fg = &c_txt;
	ALLEGRO_COLOR const* bg = &c_bg;
	bool dis = flags&FL_DISABLED;
	bool sel = !dis && (flags&FL_SELECTED);
	bool hov = !dis && (flags&FL_HOVERED);
	if(sel)
		std::swap(fg,bg);
	else if(hov)
		bg = &c_hov_bg;
	
	// Fill the button
	al_draw_filled_rectangle(x, y, x+w-1, y+h-1, *bg);
	// Draw the border 
	al_draw_rectangle(x, y, x+w-1, y+h-1, c_border, 1);
	// Finally, the text
	int tx = (x+w/2);
	int ty = (y+h/2)-(al_get_font_line_height(fonts[FONT_BUTTON])/2);
	al_draw_text(fonts[FONT_BUTTON], *fg, tx, ty, ALLEGRO_ALIGN_CENTRE, text.c_str());
}

Button::Button()
	: MouseObject(0, 0, 96, 32)
{}
Button::Button(string const& txt, u16 X, u16 Y)
	: MouseObject(X, Y, 96, 32), text(txt)
{}
Button::Button(string const& txt, u16 X, u16 Y, u16 W, u16 H)
	: MouseObject(X, Y, W, H), text(txt)
{}




void init_fonts()
{
	al_set_new_bitmap_flags(0);
	fonts[FONT_BUTTON] = al_load_ttf_font("assets/font_opensans/OpenSans-Regular.ttf", -20, 0);
	fonts[FONT_ANSWER] = al_load_ttf_font("assets/font_opensans/OpenSans-Regular.ttf", -20, 0);
	fonts[FONT_MARKING] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -6, 0);
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
	
	al_set_window_constraints(display, CANVAS_W, CANVAS_H, 0, 0);
	al_apply_window_constraints(display, true);
	
	init_fonts();
}

