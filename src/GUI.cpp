#include "Main.hpp"
#include "GUI.hpp"
#include "Font.hpp"

ALLEGRO_COLOR
	C_BG = C_LGRAY
	, C_CELL_BG = C_WHITE
	, C_CELL_BORDER = C_LGRAY
	, C_CELL_TEXT = C_BLUE
	, C_CELL_GIVEN = C_BLACK
	, C_REGION_BORDER = C_BLACK
	, C_LBL_TEXT = C_BLACK
	, C_LBL_SHADOW = al_map_rgba(0,0,0,0)
	, C_BUTTON_FG = C_BLACK
	, C_BUTTON_BG = C_WHITE
	, C_BUTTON_HOVBG = C_LGRAY
	, C_BUTTON_BORDER = C_BLACK
	, C_BUTTON_DISTXT = C_LGRAY
	, C_HIGHLIGHT = al_map_rgb(76, 164, 255)
	, C_HIGHLIGHT2 = al_map_rgb(255, 164, 76)
	;
InputState* cur_input;
double render_xscale = 1, render_yscale = 1;
int render_resx = CANVAS_W, render_resy = CANVAS_H;

void DrawContainer::run()
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
	mouse();
	
	cur_input->oldstate = cur_input->buttons;
}
void DrawContainer::draw() const
{
	for(DrawnObject const* obj : *this)
		obj->draw();
}
bool DrawContainer::mouse()
{
	for(auto it = rbegin(); it != rend(); ++it)
		if((*it)->mouse())
			return true;
	return false;
}
void DrawContainer::on_disp_resize()
{
	for(DrawnObject* obj : *this)
		obj->on_disp_resize();
}
void DrawContainer::key_event(ALLEGRO_EVENT const& ev)
{
	for(DrawnObject* obj : *this)
		obj->key_event(ev);
}
void DrawContainer::run_loop()
{
	redraw = true;
	while(program_running && (!run_proc || run_proc()))
	{
		if(redraw && events_empty())
		{
			run();
			dlg_draw();
			dlg_render();
			redraw = false;
		}
		else run_events(redraw);
	}
}

void Dialog::run()
{
	InputState* old = cur_input;
	cur_input = &state;
	DrawContainer::run();
	cur_input = old;
}
void Dialog::draw() const
{
	InputState* old = cur_input;
	cur_input = &const_cast<Dialog*>(this)->state;
	DrawContainer::draw();
	cur_input = old;
}
bool Dialog::mouse()
{
	InputState* old = cur_input;
	cur_input = &state;
	bool ret = DrawContainer::mouse();
	cur_input = old;
	return ret;
}
void Dialog::on_disp_resize()
{
	InputState* old = cur_input;
	cur_input = &state;
	DrawContainer::on_disp_resize();
	cur_input = old;
}
void Dialog::key_event(ALLEGRO_EVENT const& ev)
{
	InputState* old = cur_input;
	cur_input = &state;
	DrawContainer::key_event(ev);
	cur_input = old;
}
void Dialog::run_loop()
{
	InputState* old = cur_input;
	cur_input = &state;
	DrawContainer::run_loop();
	cur_input = old;
}

void clear_a5_bmp(ALLEGRO_COLOR col, ALLEGRO_BITMAP* bmp)
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

void update_scale()
{
	extern ALLEGRO_DISPLAY* display;
	render_resx = al_get_display_width(display);
	render_resy = al_get_display_height(display);
	
	render_xscale = double(render_resx)/CANVAS_W;
	render_yscale = double(render_resy)/CANVAS_H;
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
void unscale_x(u16& x)
{
	x /= render_xscale;
}
void unscale_y(u16& y)
{
	y /= render_yscale;
}
void unscale_pos(u16& x, u16& y)
{
	x /= render_xscale;
	y /= render_yscale;
}
void unscale_pos(u16& x, u16& y, u16& w, u16& h)
{
	unscale_pos(x,y);
	w /= render_xscale;
	h /= render_yscale;
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

// Column
u16 Column::width() const
{
	return w+2*padding;
}
u16 Column::height() const
{
	u16 H = 2*padding;
	for(DrawnObject const* obj : cont)
		H += obj->height() + spacing;
	return H;
}
void Column::realign(size_t start)
{
	u16 H = padding;
	for(size_t q = 0; q < start; ++q)
	{
		DrawnObject* obj = cont[q];
		H += obj->height() + spacing;
	}
	for(size_t q = start; q < cont.size(); ++q)
	{
		DrawnObject* obj = cont[q];
		switch(align)
		{
			default:
				obj->x = x+padding;
				break;
			case ALLEGRO_ALIGN_CENTRE:
				obj->x = x+padding+(w-obj->width())/2;
				break;
			case ALLEGRO_ALIGN_RIGHT:
				obj->x = x+padding+(w-obj->width());
				break;
		}
		obj->y = y + H;
		H += obj->height() + spacing;
	}
	h = H+padding;
}
void Column::add(DrawnObject* obj)
{
	auto objw = obj->width();
	if(objw > w)
	{
		w = objw;
		realign();
	}
	DrawWrapper::add(obj);
	realign(cont.size()-1);
}
Column::Column(u16 X, u16 Y, u16 padding, u16 spacing, i8 align)
	: DrawWrapper(X, Y), padding(padding), spacing(spacing),
	align(align)
{}
// Row
u16 Row::width() const
{
	u16 W = 2*padding;
	for(DrawnObject const* obj : cont)
		W += obj->width() + spacing;
	return W;
}
u16 Row::height() const
{
	return h+2*padding;
}
void Row::realign(size_t start)
{
	u16 W = padding;
	for(size_t q = 0; q < start; ++q)
	{
		DrawnObject* obj = cont[q];
		W += obj->width() + spacing;
	}
	for(size_t q = start; q < cont.size(); ++q)
	{
		DrawnObject* obj = cont[q];
		switch(align)
		{
			default:
				obj->y = y+padding;
				break;
			case ALLEGRO_ALIGN_CENTRE:
				obj->y = y+padding+(h-obj->height())/2;
				break;
			case ALLEGRO_ALIGN_RIGHT:
				obj->y = y+padding+(h-obj->height());
				break;
		}
		obj->x = x + W;
		W += obj->width() + spacing;
	}
	w = W+padding;
}
void Row::add(DrawnObject* obj)
{
	auto objh = obj->height();
	if(objh > h)
	{
		h = objh;
		realign();
	}
	DrawWrapper::add(obj);
	realign(cont.size()-1);
}
Row::Row(u16 X, u16 Y, u16 padding, u16 spacing, i8 align)
	: DrawWrapper(X, Y), padding(padding), spacing(spacing),
	align(align)
{}

// Button
void Button::draw() const
{
	u16 X = x, Y = y, W = w, H = h;
	scale_pos(X,Y,W,H);
	
	ALLEGRO_COLOR const* fg = &C_BUTTON_FG;
	ALLEGRO_COLOR const* bg = &C_BUTTON_BG;
	bool dis = flags&FL_DISABLED;
	bool sel = !dis && (flags&FL_SELECTED);
	bool hov = !dis && (flags&FL_HOVERED);
	if(sel)
		std::swap(fg,bg);
	else if(hov)
		bg = &C_BUTTON_HOVBG;
	else if(dis)
		fg = &C_BUTTON_DISTXT;
	
	// Fill the button
	al_draw_filled_rectangle(X, Y, X+W-1, Y+H-1, *bg);
	// Draw the border 
	al_draw_rectangle(X, Y, X+W-1, Y+H-1, C_BUTTON_BORDER, 1);
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

// ShapeRect
void ShapeRect::draw() const
{
	u16 X = x, Y = y, W = w, H = h;
	scale_pos(X,Y,W,H);
	if(c_border)
		al_draw_rectangle(X, Y, X+W-1, Y+H-1, *c_border, brd_thick);
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
	: InputObject(X,Y,W,H), c_fill(c), brd_thick(0)
{}
ShapeRect::ShapeRect(u16 X, u16 Y, u16 W, u16 H, ALLEGRO_COLOR c, ALLEGRO_COLOR cb, double border_thick)
	: InputObject(X,Y,W,H), c_fill(c), c_border(cb), brd_thick(border_thick)
{}

// Label
void draw_text(u16 X, u16 Y, string const& str, i8 align, FontDef font, ALLEGRO_COLOR c_txt, optional<ALLEGRO_COLOR> c_shadow = nullopt)
{
	ALLEGRO_FONT* f = font.get();
	if(c_shadow)
		al_draw_text(f, *c_shadow, X+2, Y+2, align, str.c_str());
	al_draw_text(f, c_txt, X, Y, align, str.c_str());
}
void Label::draw() const
{
	u16 X = x, Y = y;
	scale_pos(X,Y);
	draw_text(X, Y, text, align, font, C_LBL_TEXT, C_LBL_SHADOW);
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

//COMMON EVENT FUNCS
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

//POPUPS
bool pop_confirm(string const& title, string const& msg, string truestr, string falsestr)
{
	Dialog popup;
	popups.emplace_back(&popup);
	
	bool redraw = true;
	bool running = true;
	bool ret = false;
	
	const uint POP_W = CANVAS_W/2;
	const uint BTN_H = 32, BTN_PAD = 5;
	//resizes automatically
	uint POP_H = CANVAS_H/3;
	uint POP_X, POP_Y;
	const uint TITLE_H = 16, TITLE_VSPC = 2, TITLE_HPAD = 5;
	
	ShapeRect bg, win, titlebar;
	Button ok, cancel;
	vector<Label> lbls;
	{ //Text Label (alters popup size, must go first)
		const u16 TXT_PAD = 15, TXT_VSPC = 3;
		u16 new_h = BTN_H + 2*BTN_PAD + 2*TXT_PAD;
		FontDef fd(-20, false, BOLD_NONE);
		ALLEGRO_FONT* f = fd.get_base();
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
			else if(c == '\n')
			{
				add_text = string(buf+anchor);
				anchor = ++pos;
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
		delete[] buf;
		const u16 fh = al_get_font_line_height(f);
		POP_H = new_h + (u16(texts.size())*(TXT_VSPC+fh)) - TXT_VSPC;
		POP_X = CANVAS_W/2 - POP_W/2;
		POP_Y = CANVAS_H/2 - POP_H/2;
		const u16 TXT_CX = CANVAS_W/2;
		u16 TXT_Y = POP_Y+TXT_PAD;
		for(string const& s : texts)
		{
			lbls.emplace_back(s, TXT_CX, TXT_Y, fd, ALLEGRO_ALIGN_CENTRE);
			TXT_Y += fh+TXT_VSPC;
		}
		
		FontDef title_fd(-i16(TITLE_H-TITLE_VSPC*2), true, BOLD_SEMI);
		lbls.emplace_back(title, POP_X+TITLE_HPAD, POP_Y-TITLE_H+TITLE_VSPC, title_fd, ALLEGRO_ALIGN_LEFT);
	}
	{ //BG, to allow 'clicking off' / shade the background
		bg = ShapeRect(0,0,CANVAS_W,CANVAS_H,al_map_rgba(0,0,0,128));
		bg.onMouse = mouse_killfocus;
		
		win = ShapeRect(POP_X,POP_Y,POP_W,POP_H,C_BG,C_BLACK,4);
		win.onMouse = mouse_killfocus;
		
		titlebar = ShapeRect(POP_X,POP_Y-TITLE_H,POP_W,TITLE_H,C_BG,C_BLACK,4);
		titlebar.onMouse = mouse_killfocus;
	}
	{ //Buttons
		ok = Button(truestr);
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
		cancel = Button(falsestr);
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
	popup.push_back(&titlebar);
	for(Label& lbl : lbls)
		popup.push_back(&lbl);
	popup.push_back(&ok);
	popup.push_back(&cancel);
	
	on_resize();
	popup.run_proc = [&running](){return running;};
	popup.run_loop();
	popups.pop_back();
	return ret;
}
bool pop_okc(string const& title, string const& msg)
{
	return pop_confirm(title, msg, "OK", "Cancel");
}
bool pop_yn(string const& title, string const& msg)
{
	return pop_confirm(title, msg, "Yes", "No");
}

