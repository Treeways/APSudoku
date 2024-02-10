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
FontDef fonts[NUM_FONTS];

double render_xscale = 1, render_yscale = 1, render_scale = 1;
int render_resx = CANVAS_W, render_resy = CANVAS_H;
int render_scalew = CANVAS_W, render_scaleh = CANVAS_H;
int render_xoff = 0, render_yoff = 0;

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

bool pop_confirm(string const& msg);

vector<vector<DrawnObject*>*> popups;

bool settings_unsaved = false;

void swap_screen(Screen scr)
{
	if(curscr == scr)
		return;
	if(curscr == SCR_SETTINGS && settings_unsaved)
	{
		if(!pop_confirm("Swap without saving settings?"))
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
InputState* cur_input;
void run(vector<DrawnObject*>& vec)
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
	auto &x = cur_input->x;
	auto &y = cur_input->y;
	x -= render_xoff;
	y -= render_yoff;
	x /= render_xscale;
	y /= render_yscale;
	
	//
	__run_mouse(vec);
	
	cur_input->oldstate = cur_input->buttons;
}

void draw()
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
			run(gui_objects[curscr]);
			draw();
			render();
			redraw = false;
		}
		
		run_events(redraw);
	}
	
	return 0;
}

void DrawnObject::on_disp_resize()
{
	if(onResizeDisplay)
		onResizeDisplay();
}

void InputObject::unhover()
{
	if(!onMouse) return;
	
	if(mouseflags&MFL_HASMOUSE)
	{
		mouseflags &= ~MFL_HASMOUSE;
		process(onMouse(MOUSE_HOVER_EXIT));
	}
	if(cur_input->hovered == this)
		cur_input->hovered = nullptr;
}
void InputObject::process(u32 retcode)
{
	auto& m = *cur_input;
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
	if(!onMouse)
		return false;
	auto& m = *cur_input;
	bool inbounds = (m.x >= x && m.x < x+w) && (m.y >= y && m.y < y+h);
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

map<FontDef,ALLEGRO_FONT*> fontmap;
map<FontDef,ALLEGRO_FONT*> fontmap_base;
FontDef::FontDef(i16 h, bool ital, Bold b)
	: tuple<i16,bool,Bold>(h,ital,b)
{}
ALLEGRO_FONT* FontDef::get() const
{
	auto it = fontmap.find(*this);
	if(it != fontmap.end()) //Font already generated
		return it->second;
	return gen();
}
ALLEGRO_FONT* FontDef::gen() const
{
	auto it = fontmap.find(*this);
	if(it != fontmap.end()) //font already exists, don't leak
		al_destroy_font(it->second);
	string weightstr;
	switch(weight())
	{
		case BOLD_NONE:
			weightstr = italic() ? "" : "Regular";
			break;
		case BOLD_LIGHT:
			weightstr = "Light";
			break;
		case BOLD_SEMI:
			weightstr = "Semibold";
			break;
		case BOLD_NORMAL:
			weightstr = "Bold";
			break;
		case BOLD_EXTRA:
			weightstr = "ExtraBold";
			break;
	}
	if(italic())
		weightstr += "Italic";
	string fontstr = "assets/font_opensans/OpenSans-"+weightstr+".ttf";
	al_set_new_bitmap_flags(0);
	ALLEGRO_FONT* f = al_load_ttf_font(fontstr.c_str(), render_yscale*height(), 0);
	fontmap[*this] = f;
	return f;
}
ALLEGRO_FONT* FontDef::get_base() const
{
	auto it = fontmap_base.find(*this);
	if(it != fontmap_base.end()) //Font already generated
		return it->second;
	return gen_base();
}
ALLEGRO_FONT* FontDef::gen_base() const
{
	auto it = fontmap_base.find(*this);
	if(it != fontmap_base.end()) //font already exists, don't leak
		al_destroy_font(it->second);
	string weightstr;
	switch(weight())
	{
		case BOLD_NONE:
			weightstr = italic() ? "" : "Regular";
			break;
		case BOLD_LIGHT:
			weightstr = "Light";
			break;
		case BOLD_SEMI:
			weightstr = "Semibold";
			break;
		case BOLD_NORMAL:
			weightstr = "Bold";
			break;
		case BOLD_EXTRA:
			weightstr = "ExtraBold";
			break;
	}
	if(italic())
		weightstr += "Italic";
	string fontstr = "assets/font_opensans/OpenSans-"+weightstr+".ttf";
	al_set_new_bitmap_flags(0);
	ALLEGRO_FONT* f = al_load_ttf_font(fontstr.c_str(), height(), 0);
	fontmap_base[*this] = f;
	return f;
}
void init_fonts()
{
	fonts[FONT_BUTTON] = FontDef(-20, false, BOLD_NONE);
	fonts[FONT_ANSWER] = FontDef(-24, false, BOLD_NONE);
	fonts[FONT_MARKING5] = FontDef(-12, true, BOLD_NONE);
	fonts[FONT_MARKING6] = FontDef(-11, true, BOLD_NONE);
	fonts[FONT_MARKING7] = FontDef(-10, true, BOLD_NONE);
	fonts[FONT_MARKING8] = FontDef(-9, true, BOLD_NONE);
	fonts[FONT_MARKING9] = FontDef(-7, true, BOLD_NONE);
}
void scale_fonts()
{
	for(auto pair : fontmap)
		pair.first.gen();
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

bool pop_confirm(string const& msg)
{
	vector<DrawnObject*> popup;
	popups.emplace_back(&popup);
	
	bool redraw = true;
	bool running = true;
	bool ret = false;
	
	const uint POP_W = CANVAS_W/2;
	const uint BTN_H = 32, BTN_PAD = 5;
	//resizes automatically
	uint POP_H = CANVAS_H/3;
	uint POP_X, POP_Y;
	
	ShapeRect bg, win;
	Button ok, cancel;
	vector<Label> lbls;
	{ //Text Label (alters popup size, must go first)
		const u16 TXT_PAD = 15, TXT_VSPC = 3;
		u16 new_h = BTN_H + 2*BTN_PAD + 2*TXT_PAD;
		FontDef fd(-20, false, BOLD_NONE);
		ALLEGRO_FONT* f = fd.get_base();
		//!TODO scan through the string, and split it into multiple label lines
		char* buf = new char[msg.size()+1];
		strcpy(buf, msg.c_str());
		size_t pos = 0, anchor = 0, last_space = 0;
		vector<string> texts;
		while(true)
		{
			char c = buf[pos];
			buf[pos] = 0;
			//
			optional<string> add_text;
			if(al_get_text_width(f, buf+anchor) + 2*TXT_PAD >= POP_W)
			{
				char c2 = buf[last_space];
				char c3;
				optional<size_t> c3pos;
				if(pos > 0)
				{
					c3 = buf[pos-1];
					c3pos = pos-1;
				}
				//
				size_t start = anchor;
				if(anchor < last_space)
				{
					buf[last_space] = 0;
					anchor = last_space;
				}
				else if(c3pos)
				{
					buf[*c3pos] = 0;
					anchor = *c3pos;
				}
				else anchor = pos;
				add_text = string(buf+start);
				//Restore stolen chars
				if(c3pos)
					buf[*c3pos] = c3;
				buf[last_space] = c2;
			}
			else if(!c) //end of string
			{
				add_text = string(buf+anchor);
			}
			//
			if(c == ' ')
				last_space = pos;
			buf[pos++] = c;
			
			if(add_text)
				texts.emplace_back(std::move(*add_text));
			if(!c)
				break;
		}
		const u16 fh = al_get_font_line_height(f);
		POP_H = new_h + (u16(texts.size())*(TXT_VSPC+fh)) - TXT_VSPC;
		POP_X = CANVAS_W/2 - POP_W/2;
		POP_Y = CANVAS_H/2 - POP_H/2;
		const u16 TXT_CX = CANVAS_W/2;
		u16 TXT_Y = POP_Y+TXT_PAD;
		for(string const& s : texts)
		{
			lbls.emplace_back(s, TXT_CX, TXT_Y, fd, ALLEGRO_ALIGN_CENTRE, C_BLACK, C_WHITE);
			TXT_Y += fh+TXT_VSPC;
		}
	}
	{ //BG, to allow 'clicking off' / shade the background
		bg = ShapeRect(0,0,CANVAS_W,CANVAS_H,al_map_rgba(0,0,0,128));
		bg.onMouse = mouse_killfocus;
		
		win = ShapeRect(POP_X,POP_Y,POP_W,POP_H,C_BG);
		win.onMouse = mouse_killfocus;
	}
	{ //Swap buttons
		ok = Button("OK");
		ok.x = CANVAS_W/2 - ok.w;
		ok.y = POP_Y+POP_H - BTN_H - BTN_PAD;
		ok.h = BTN_H;
		ok.onMouse = [&ret,&running,&ok](MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						ret = true;
						running = false;
						ok.flags |= FL_SELECTED;
						return MRET_TAKEFOCUS;
				}
				return ok.handle_ev(e);
			};
		cancel = Button("Cancel");
		cancel.x = CANVAS_W/2;
		cancel.y = POP_Y+POP_H - BTN_H - BTN_PAD;
		cancel.h = BTN_H;
		cancel.onMouse = [&ret,&running,&cancel](MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						ret = false;
						running = false;
						cancel.flags |= FL_SELECTED;
						return MRET_TAKEFOCUS;
				}
				return cancel.handle_ev(e);
			};
		
	}
	popup.push_back(&bg);
	popup.push_back(&win);
	for(Label& lbl : lbls)
		popup.push_back(&lbl);
	popup.push_back(&ok);
	popup.push_back(&cancel);
	
	InputState popup_state;
	InputState* old_state = cur_input;
	cur_input = &popup_state;
	on_resize();
	while(running && program_running)
	{
		if(redraw && al_is_event_queue_empty(events))
		{
			run(popup);
			draw();
			render();
			redraw = false;
		}
		
		if(running)
			run_events(redraw);
	}
	cur_input = old_state;
	popups.pop_back();
	return ret;
}

// Button
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
	ALLEGRO_FONT* f = fonts[FONT_BUTTON].get();
	int tx = (X+W/2);
	int ty = (Y+H/2)-(al_get_font_line_height(f)/2);
	al_draw_text(f, *fg, tx, ty, ALLEGRO_ALIGN_CENTRE, text.c_str());
}

u32 Button::handle_ev(MouseEvent e)
{
	switch(e)
	{
		case MOUSE_HOVER_ENTER:
			flags |= FL_HOVERED;
			break;
		case MOUSE_HOVER_EXIT:
			flags &= ~FL_HOVERED;
			break;
	}
	return MRET_OK;
}
bool Button::mouse()
{
	if(!onMouse)
		onMouse = [this](MouseEvent e){return this->handle_ev(e);};
	return InputObject::mouse();
}

Button::Button()
	: InputObject(0, 0, 96, 32), flags(0)
{}
Button::Button(string const& txt)
	: InputObject(0, 0, 96, 32), flags(0), text(txt)
{}
Button::Button(string const& txt, u16 X, u16 Y)
	: InputObject(X, Y, 96, 32), flags(0), text(txt)
{}
Button::Button(string const& txt, u16 X, u16 Y, u16 W, u16 H)
	: InputObject(X, Y, W, H), flags(0), text(txt)
{}

//ShapeRect
void ShapeRect::draw() const
{
	u16 X = x, Y = y, W = w, H = h;
	scale_pos(X,Y,W,H);
	al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, c_fill);
}

bool ShapeRect::mouse()
{
	if(!onMouse)
		onMouse = mouse_killfocus;
	return InputObject::mouse();
}

ShapeRect::ShapeRect()
	: InputObject()
{}
ShapeRect::ShapeRect(u16 X, u16 Y, u16 W, u16 H)
	: InputObject(X,Y,W,H)
{}
ShapeRect::ShapeRect(u16 X, u16 Y, u16 W, u16 H, ALLEGRO_COLOR c)
	: InputObject(X,Y,W,H), c_fill(c)
{}

// Label
void Label::draw() const
{
	u16 X = x, Y = y;
	scale_pos(X,Y);
	ALLEGRO_FONT* f = font.get();
	if(c_shadow)
		al_draw_text(f, *c_shadow, X+2, Y+2, align, text.c_str());
	al_draw_text(f, c_txt, X, Y, align, text.c_str());
}
u16 Label::xpos() const
{
	u16 lx = InputObject::xpos();
	switch(align)
	{
		case ALLEGRO_ALIGN_LEFT:
			return lx;
		case ALLEGRO_ALIGN_CENTRE:
			return lx - width()/2;
		case ALLEGRO_ALIGN_RIGHT:
			return lx - width();
	}
	return lx;
}
u16 Label::width() const
{
	ALLEGRO_FONT* f = font.get();
	return f ? al_get_text_width(f, text.c_str()) : 0;
}
u16 Label::height() const
{
	ALLEGRO_FONT* f = font.get();
	return f ? al_get_font_line_height(f) : 0;
}
Label::Label()
	: InputObject(), text()
{}
Label::Label(string const& txt)
	: InputObject(), text(txt), align(ALLEGRO_ALIGN_CENTRE)
{}
Label::Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 al)
	: InputObject(X,Y), text(txt), align(al), font(fd)
{}
Label::Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 al, ALLEGRO_COLOR fg, optional<ALLEGRO_COLOR> shd)
	: InputObject(X,Y), text(txt), align(al), font(fd),
	c_txt(fg), c_shadow(shd)
{}

u32 mouse_killfocus(MouseEvent e)
{
	if(e == MOUSE_LCLICK && cur_input->focused)
	{
		u32 newret = cur_input->focused->onMouse(MOUSE_LOSTFOCUS);
		if(!(newret & MRET_TAKEFOCUS))
			cur_input->focused = nullptr;
	}
	return MRET_OK;
}

