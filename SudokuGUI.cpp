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

double render_xscale = 1, render_yscale = 1, render_scale = 1;
int render_resx = CANVAS_W, render_resy = CANVAS_H;
int render_scalew = CANVAS_W, render_scaleh = CANVAS_H;
int render_xoff = 0, render_yoff = 0;

EntryMode mode = ENT_ANSWER;
EntryMode get_mode()
{
	if(input_state.shift())
		return ENT_CORNER;
	if(input_state.ctrl_cmd())
		return ENT_CENTER;
	if(input_state.alt())
		return ENT_ANSWER;
	return mode;
}
bool InputState::shift() const
{
	ALLEGRO_KEYBOARD_STATE st;
	al_get_keyboard_state(&st);
	return (al_key_down(&st,ALLEGRO_KEY_LSHIFT)
		|| al_key_down(&st,ALLEGRO_KEY_RSHIFT));
}
bool InputState::ctrl_cmd() const
{
	ALLEGRO_KEYBOARD_STATE st;
	al_get_keyboard_state(&st);
	return (al_key_down(&st,ALLEGRO_KEY_LCTRL)
		|| al_key_down(&st,ALLEGRO_KEY_RCTRL)
		|| al_key_down(&st,ALLEGRO_KEY_COMMAND));
}
bool InputState::alt() const
{
	ALLEGRO_KEYBOARD_STATE st;
	al_get_keyboard_state(&st);
	return (al_key_down(&st,ALLEGRO_KEY_ALT)
		|| al_key_down(&st,ALLEGRO_KEY_ALTGR));
}

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


void update_scale()
{
	render_resx = al_get_display_width(display);
	render_resy = al_get_display_height(display);
	
	render_xscale = double(render_resx)/CANVAS_W;
	render_yscale = double(render_resy)/CANVAS_H;
	render_scale = std::min(render_xscale,render_yscale);
}

void scale_x(u16& x)
{
	x *= render_xscale;
}
void scale_y(u16& y)
{
	y *= render_yscale;
}
void scale_pos(u16& x, u16& y)
{
	x *= render_xscale;
	y *= render_yscale;
}
void scale_pos(u16& x, u16& y, u16& w, u16& h)
{
	scale_pos(x,y);
	w *= render_xscale;
	h *= render_yscale;
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
						if(!input_state.shift())
							grid.deselect();
					[[fallthrough]];
					case MOUSE_LDOWN:
						if((ret & MRET_TAKEFOCUS) || input_state.focused == &grid)
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
		static InputObject bg(0,0,CANVAS_W,CANVAS_H);
		bg.onMouse = [](MouseEvent e)
			{
				if(e == MOUSE_LCLICK && input_state.focused)
				{
					u32 newret = input_state.focused->onMouse(MOUSE_LOSTFOCUS);
					if(!(newret & MRET_TAKEFOCUS))
						input_state.focused = nullptr;
				}
				return MRET_OK;
			};
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(&bg);
	}
}

void __run_mouse()
{
	for(DrawnObject* obj : gui_objects[curscr])
		if(obj->mouse())
			return;
}
InputState input_state;
void run()
{
	al_get_mouse_state(&input_state);
	auto buttons = input_state.buttons;
	auto os = input_state.oldstate & 0x7;
	
	if(buttons & os) //Remember the held state
		buttons &= os;
	
	input_state.buttons &= ~0x7;
	if(buttons & 0x1)
		input_state.buttons |= 0x1;
	else if(buttons & 0x2)
		input_state.buttons |= 0x2;
	else if(buttons & 0x4)
		input_state.buttons |= 0x4;
	
	//Mouse coordinates are based on the 'display'
	//Scale down the x/y before running mouse logic:
	auto &x = input_state.x;
	auto &y = input_state.y;
	x -= render_xoff;
	y -= render_yoff;
	x /= render_xscale;
	y /= render_yscale;
	
	//
	__run_mouse();
	
	input_state.oldstate = input_state.buttons;
}

void draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	
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
void on_resize();
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
			case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
				redraw = true;
				break;
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				running = false;
				break;
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
				al_acknowledge_resize(display);
				on_resize();
				redraw = true;
				break;
			case ALLEGRO_EVENT_KEY_DOWN:
			case ALLEGRO_EVENT_KEY_UP:
			case ALLEGRO_EVENT_KEY_CHAR:
				if(input_state.focused)
					input_state.focused->key_event(ev);
				break;
		}
	}
	
	return 0;
}

void InputObject::unhover()
{
	if(!onMouse) return;
	
	if(mouseflags&MFL_HASMOUSE)
	{
		mouseflags &= ~MFL_HASMOUSE;
		process(onMouse(MOUSE_HOVER_EXIT));
	}
	if(input_state.hovered == this)
		input_state.hovered = nullptr;
}
void InputObject::process(u32 retcode)
{
	auto& m = input_state;
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
bool InputObject::mouse()
{
	auto& m = input_state;
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
	u16 X = x, Y = y, W = w, H = h;
	scale_pos(X,Y,W,H);
	
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
	al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, *bg);
	// Draw the border 
	al_draw_rectangle(X, Y, X+W-1, Y+H-1, c_border, 1);
	// Finally, the text
	int tx = (X+W/2);
	int ty = (Y+H/2)-(al_get_font_line_height(fonts[FONT_BUTTON])/2);
	al_draw_text(fonts[FONT_BUTTON], *fg, tx, ty, ALLEGRO_ALIGN_CENTRE, text.c_str());
}

Button::Button()
	: InputObject(0, 0, 96, 32), flags(0)
{}
Button::Button(string const& txt, u16 X, u16 Y)
	: InputObject(X, Y, 96, 32), flags(0), text(txt)
{}
Button::Button(string const& txt, u16 X, u16 Y, u16 W, u16 H)
	: InputObject(X, Y, W, H), flags(0), text(txt)
{}




void init_fonts(double scale)
{
	al_set_new_bitmap_flags(0);
	fonts[FONT_BUTTON] = al_load_ttf_font("assets/font_opensans/OpenSans-Regular.ttf", -20*scale, 0);
	fonts[FONT_ANSWER] = al_load_ttf_font("assets/font_opensans/OpenSans-Regular.ttf", -24*scale, 0);
	fonts[FONT_MARKING5] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -12*scale, 0);
	fonts[FONT_MARKING6] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -11*scale, 0);
	fonts[FONT_MARKING7] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -10*scale, 0);
	fonts[FONT_MARKING8] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -9*scale, 0);
	fonts[FONT_MARKING9] = al_load_ttf_font("assets/font_opensans/OpenSans-Italic.ttf", -7*scale, 0);
}
void scale_fonts()
{
	for(int q = 0; q < NUM_FONTS; ++q)
		al_destroy_font(fonts[q]);
	init_fonts(render_scale);
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
	
	init_fonts(1.0);
}

