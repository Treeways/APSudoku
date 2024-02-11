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
	, C_RAD_BG = C_WHITE
	, C_RAD_HOVBG = C_LGRAY
	, C_RAD_FG = C_BLACK
	, C_RAD_DIS_BG = C_LGRAY
	, C_RAD_DIS_FG = C_DGRAY
	, C_RAD_BORDER = C_BLACK
	, C_RAD_TXT = C_LBL_TEXT
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
	for(shared_ptr<GUIObject> const& obj : *this)
	{
		auto old_parent = obj->draw_parent;
		
		obj->draw_parent = draw_parent;
		obj->draw();
		obj->draw_parent = old_parent;
	}
}
bool DrawContainer::mouse()
{
	for(auto it = rbegin(); it != rend(); ++it)
	{
		auto const& obj = (*it);
		auto old_parent = obj->draw_parent;
		
		obj->draw_parent = draw_parent;
		bool ret = obj->mouse();
		obj->draw_parent = old_parent;
		
		if(ret)
			return true;
	}
	return false;
}
void DrawContainer::on_disp_resize()
{
	for(shared_ptr<GUIObject>& obj : *this)
		obj->on_disp_resize();
}
void DrawContainer::key_event(ALLEGRO_EVENT const& ev)
{
	for(shared_ptr<GUIObject>& obj : *this)
		obj->key_event(ev);
}
void DrawContainer::run_loop()
{
	al_get_mouse_state(cur_input);
	cur_input->oldstate = cur_input->buttons;
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

bool InputState::unfocus()
{
	if(!focused) return true;
	u32 newret = focused->mouse_event(MOUSE_LOSTFOCUS);
	if(newret & MRET_TAKEFOCUS)
		return false;
	return true;
}
bool InputState::refocus(InputObject* targ)
{
	new_focus = targ;
	bool ret = unfocus();
	new_focus = nullptr;
	if(ret)
		focused = targ;
	return ret;
}

void update_scale()
{
	extern ALLEGRO_DISPLAY* display;
	render_resx = al_get_display_width(display);
	render_resy = al_get_display_height(display);
	
	render_xscale = double(render_resx)/CANVAS_W;
	render_yscale = double(render_resy)/CANVAS_H;
}

void scale_min(u16& v)
{
	v *= std::min(render_xscale,render_yscale);
}
void scale_max(u16& v)
{
	v *= std::max(render_xscale,render_yscale);
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
void unscale_min(u16& v)
{
	v /= std::min(render_xscale,render_yscale);
}
void unscale_max(u16& v)
{
	v /= std::max(render_xscale,render_yscale);
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

void GUIObject::on_disp_resize()
{
	if(onResizeDisplay)
		onResizeDisplay();
	realign();
}
bool GUIObject::disabled() const
{
	return (flags&FL_DISABLED)
		|| (dis_proc && dis_proc())
		|| (draw_parent && draw_parent->disabled());
}
bool GUIObject::selected() const
{
	return sel_proc ? sel_proc() : (flags&FL_SELECTED);
}
bool GUIObject::focused() const
{
	return cur_input && cur_input->focused == this;
}

void InputObject::focus()
{
	if(cur_input && cur_input->focused != this)
		cur_input->refocus(this);
}
u32 InputObject::handle_ev(MouseEvent ev)
{
	return mouse_killfocus(*this, ev);
}
u32 InputObject::mouse_event(MouseEvent ev)
{
	return onMouse ? onMouse(*this,ev) : handle_ev(ev);
}
void InputObject::unhover()
{
	if(mouseflags&MFL_HASMOUSE)
	{
		mouseflags &= ~MFL_HASMOUSE;
		process(mouse_event(MOUSE_HOVER_EXIT));
	}
	if(cur_input->hovered == this)
		cur_input->hovered = nullptr;
}
void InputObject::process(u32 retcode)
{
	if(retcode & MRET_TAKEFOCUS)
		focus();
}
bool InputObject::mouse()
{
	auto& m = *cur_input;
	u16 X = xpos(), Y = ypos(), W = width(), H = height();
	bool inbounds = (m.x >= X && m.x < X+W) && (m.y >= Y && m.y < Y+H);
	if(inbounds)
	{
		if(m.hovered && m.hovered != this)
			m.hovered->unhover();
		m.hovered = this;
		if(!(mouseflags&MFL_HASMOUSE))
		{
			mouseflags |= MFL_HASMOUSE;
			process(mouse_event(MOUSE_HOVER_ENTER));
		}
		
		if(disabled())
			return true;
		if(m.lrelease())
			process(mouse_event(MOUSE_LRELEASE));
		else if(m.lrelease())
			process(mouse_event(MOUSE_RRELEASE));
		else if(m.lrelease())
			process(mouse_event(MOUSE_MRELEASE));
		
		if(m.lclick())
			process(mouse_event(MOUSE_LCLICK));
		else if(m.rclick())
			process(mouse_event(MOUSE_RCLICK));
		else if(m.mclick())
			process(mouse_event(MOUSE_MCLICK));
		else if(m.buttons & 0x1)
			process(mouse_event(MOUSE_LDOWN));
		else if(m.buttons & 0x2)
			process(mouse_event(MOUSE_RDOWN));
		else if(m.buttons & 0x4)
			process(mouse_event(MOUSE_MDOWN));
		
		return true;
	}
	else
	{
		unhover();
		return false;
	}
}

// DrawWrapper
void DrawWrapper::draw() const
{
	const_cast<DrawContainer*>(&cont)->draw_parent = this;
	cont.draw();
}
bool DrawWrapper::mouse()
{
	cont.draw_parent = this;
	return cont.mouse();
}

// Column
u16 Column::width() const
{
	return w+2*padding;
}
u16 Column::height() const
{
	u16 H = 2*padding;
	for(shared_ptr<GUIObject> const& obj : cont)
		H += obj->height() + spacing;
	return H;
}
void Column::realign(size_t start)
{
	u16 H = padding;
	for(size_t q = 0; q < start; ++q)
	{
		shared_ptr<GUIObject>& obj = cont[q];
		H += obj->height() + spacing;
	}
	for(size_t q = start; q < cont.size(); ++q)
	{
		shared_ptr<GUIObject>& obj = cont[q];
		switch(align)
		{
			default:
				obj->xpos(x+padding);
				break;
			case ALLEGRO_ALIGN_CENTRE:
				obj->xpos(x+padding+(w-obj->width())/2);
				break;
			case ALLEGRO_ALIGN_RIGHT:
				obj->xpos(x+padding+(w-obj->width()));
				break;
		}
		
		obj->ypos(y + H);
		H += obj->height() + spacing;
		
		obj->realign();
	}
	h = H+padding;
}
void Column::add(shared_ptr<GUIObject> obj)
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
	for(shared_ptr<GUIObject> const& obj : cont)
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
		shared_ptr<GUIObject>& obj = cont[q];
		W += obj->width() + spacing;
	}
	for(size_t q = start; q < cont.size(); ++q)
	{
		shared_ptr<GUIObject>& obj = cont[q];
		switch(align)
		{
			default:
				obj->ypos(y+padding);
				break;
			case ALLEGRO_ALIGN_CENTRE:
				obj->ypos(y+padding+(h-obj->height())/2);
				break;
			case ALLEGRO_ALIGN_RIGHT:
				obj->ypos(y+padding+(h-obj->height()));
				break;
		}
		obj->xpos(x + W);
		W += obj->width() + spacing;
		
		obj->realign();
	}
	w = W+padding;
}
void Row::add(shared_ptr<GUIObject> obj)
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

// RadioButton
void RadioButton::draw() const
{
	bool dis = disabled();
	bool sel = selected();
	bool hov = !dis && (flags&FL_HOVERED);
	u16 CX = x, CY = y;
	u16 X = x + radius + pad, Y = CY;
	u16 RAD = radius;
	scale_pos(CX,CY);
	scale_pos(X,Y);
	scale_min(RAD);
	al_draw_filled_circle(CX, CY, RAD+pad/2, C_RAD_BORDER);
	ALLEGRO_COLOR const& bgc = dis ? C_RAD_DIS_BG : (hov ? C_RAD_HOVBG : C_RAD_BG);
	ALLEGRO_COLOR const& fgc = dis ? C_RAD_DIS_FG : C_RAD_FG;
	al_draw_filled_circle(CX, CY, RAD, bgc);
	if(sel)
		al_draw_filled_circle(CX, CY, RAD-4, fgc);
	ALLEGRO_FONT* f = font.get();
	Y = CY - al_get_font_line_height(f) / 2;
	if(!text.empty())
		al_draw_text(f, C_RAD_TXT, X, Y, ALLEGRO_ALIGN_LEFT, text.c_str());
}
u32 RadioButton::handle_ev(MouseEvent e)
{
	switch(e)
	{
		case MOUSE_LCLICK:
			flags ^= FL_SELECTED;
			return MRET_TAKEFOCUS;
		case MOUSE_HOVER_ENTER:
			flags |= FL_HOVERED;
			break;
		case MOUSE_HOVER_EXIT:
			flags &= ~FL_HOVERED;
			break;
	}
	return MRET_OK;
}
u16 RadioButton::xpos() const
{
	return x - radius - pad;
}
u16 RadioButton::ypos() const
{
	return y - height()/2;
}
u16 RadioButton::width() const
{
	ALLEGRO_FONT* f = font.get_base();
	return radius*2 + pad*2 + al_get_text_width(f, text.c_str());
}
u16 RadioButton::height() const
{
	ALLEGRO_FONT* f = font.get_base();
	return std::max(al_get_font_line_height(f), radius*2);
}

// RadioSet
void RadioSet::init(vector<string>& opts, FontDef fnt, u16 rad)
{
	cont.clear();
	u16 ind = 0;
	for(string const& str : opts)
	{
		shared_ptr<RadioButton> rb = make_shared<RadioButton>(str, fnt, rad);
		add(rb);
		rb->sel_proc = [this,ind]()
			{
				return this->get_sel() == ind;
			};
		rb->onMouse = [this,ind](InputObject& ref,MouseEvent e)
			{
				if(e == MOUSE_LCLICK)
				{
					this->select(ind);
					this->focus();
					return MRET_OK;
				}
				return ref.handle_ev(e);
			};
		++ind;
	}
}
optional<u16> RadioSet::get_sel() const
{
	if(get_sel_proc)
		return get_sel_proc();
	return sel_ind;
}
void RadioSet::select(optional<u16> targ)
{
	if(set_sel_proc)
		return set_sel_proc(targ);
	sel_ind = targ;
}

// CheckBox
void CheckBox::draw() const
{
	bool dis = disabled();
	bool sel = selected();
	bool hov = !dis && (flags&FL_HOVERED);
	u16 CX = x, CY = y;
	u16 X = x + radius + pad, Y = CY;
	u16 RAD = radius;
	scale_pos(CX,CY);
	scale_pos(X,Y);
	scale_min(RAD);
	u16 R = RAD+pad/2;
	al_draw_filled_rectangle(CX-R, CY-R, CX+R, CY+R, C_RAD_BORDER);
	ALLEGRO_COLOR const& bgc = dis ? C_RAD_DIS_BG : (hov ? C_RAD_HOVBG : C_RAD_BG);
	ALLEGRO_COLOR const& fgc = dis ? C_RAD_DIS_FG : C_RAD_FG;
	R = RAD;
	al_draw_filled_rectangle(CX-R, CY-R, CX+R, CY+R, bgc);
	if(sel)
	{
		R = RAD-4;
		al_draw_filled_rectangle(CX-R, CY-R, CX+R, CY+R, fgc);
	}
	ALLEGRO_FONT* f = font.get();
	Y = CY - al_get_font_line_height(f) / 2;
	if(!text.empty())
		al_draw_text(f, C_RAD_TXT, X, Y, ALLEGRO_ALIGN_LEFT, text.c_str());
}

// Button
void Button::draw() const
{
	u16 X = x, Y = y, W = w, H = h;
	scale_pos(X,Y,W,H);
	
	ALLEGRO_COLOR const* fg = &C_BUTTON_FG;
	ALLEGRO_COLOR const* bg = &C_BUTTON_BG;
	bool dis = disabled();
	bool sel = !dis && selected();
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
	ALLEGRO_FONT* f = font.get();
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

Button::Button()
	: InputObject(0, 0, 96, 32), font(FontDef(-20, false, BOLD_NONE))
{}
Button::Button(string const& txt)
	: InputObject(0, 0, 96, 32), font(FontDef(-20, false, BOLD_NONE)),
		text(txt)
{}
Button::Button(string const& txt, FontDef fnt)
	: InputObject(0, 0, 96, 32), font(fnt), text(txt)
{}
Button::Button(string const& txt, FontDef fnt, u16 X, u16 Y)
	: InputObject(X, Y, 96, 32), font(fnt), text(txt)
{}
Button::Button(string const& txt, FontDef fnt, u16 X, u16 Y, u16 W, u16 H)
	: InputObject(X, Y, W, H), font(fnt), text(txt)
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
	ALLEGRO_FONT* f = font.get_base();
	return f ? al_get_text_width(f, text.c_str()) : 0;
}
u16 Label::height() const
{
	ALLEGRO_FONT* f = font.get_base();
	return f ? al_get_font_line_height(f) : 0;
}
Label::Label()
	: InputObject(), text()
{}
Label::Label(string const& txt)
	: InputObject(), text(txt), align(ALLEGRO_ALIGN_CENTRE)
{}
Label::Label(string const& txt, FontDef fd, i8 al)
	: InputObject(), text(txt), align(al), font(fd)
{}
Label::Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 al)
	: InputObject(X,Y), text(txt), align(al), font(fd)
{}

//COMMON EVENT FUNCS
u32 mouse_killfocus(InputObject& ref,MouseEvent e)
{
	if(e == MOUSE_LCLICK)
		cur_input->unfocus();
	return MRET_OK;
}

//POPUPS
optional<u8> pop_confirm(string const& title, string const& msg, vector<string> const& strs)
{
	Dialog popup;
	popups.emplace_back(&popup);
	
	bool redraw = true;
	bool running = true;
	optional<u8> ret;
	
	const uint POP_W = CANVAS_W/2;
	const uint BTN_H = 32, BTN_PAD = 5;
	//resizes automatically
	uint POP_H = CANVAS_H/3;
	uint POP_X, POP_Y;
	const uint TITLE_H = 16, TITLE_VSPC = 2, TITLE_HPAD = 5;
	
	shared_ptr<ShapeRect> bg, win, titlebar;
	vector<shared_ptr<Label>> lbls;
	shared_ptr<Row> btnrow;
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
			lbls.emplace_back(make_shared<Label>(s, TXT_CX, TXT_Y, fd, ALLEGRO_ALIGN_CENTRE));
			TXT_Y += fh+TXT_VSPC;
		}
		
		FontDef title_fd(-i16(TITLE_H-TITLE_VSPC*2), true, BOLD_SEMI);
		lbls.emplace_back(make_shared<Label>(title, POP_X+TITLE_HPAD, POP_Y-TITLE_H+TITLE_VSPC, title_fd, ALLEGRO_ALIGN_LEFT));
	}
	{ //BG, to allow 'clicking off' / shade the background
		bg = make_shared<ShapeRect>(0,0,CANVAS_W,CANVAS_H,al_map_rgba(0,0,0,128));
		
		win = make_shared<ShapeRect>(POP_X,POP_Y,POP_W,POP_H,C_BG,C_BLACK,4);
		
		titlebar = make_shared<ShapeRect>(POP_X,POP_Y-TITLE_H,POP_W,TITLE_H,C_BG,C_BLACK,4);
	}
	{ //Buttons
		btnrow = make_shared<Row>(0,0,BTN_PAD,2,ALLEGRO_ALIGN_CENTRE);
		for(size_t q = 0; q < strs.size(); ++q)
		{
			shared_ptr<Button> btn = make_shared<Button>(strs[q]);
			btn->h = BTN_H;
			btn->onMouse = [&ret,&running,q](InputObject& ref,MouseEvent e)
				{
					switch(e)
					{
						case MOUSE_LCLICK:
							ret = u8(q);
							running = false;
							ref.flags |= FL_SELECTED;
							return MRET_TAKEFOCUS;
					}
					return ref.handle_ev(e);
				};
			btnrow->add(btn);
		}
		btnrow->x = (CANVAS_W - btnrow->width()) / 2;
		btnrow->y = POP_Y+POP_H-btnrow->height();
		btnrow->realign();
	}
	popup.push_back(bg);
	popup.push_back(win);
	popup.push_back(titlebar);
	for(shared_ptr<Label>& lbl : lbls)
		popup.push_back(lbl);
	popup.push_back(btnrow);
	
	on_resize();
	popup.run_proc = [&running](){return running;};
	popup.run_loop();
	popups.pop_back();
	return ret;
}
bool pop_okc(string const& title, string const& msg)
{
	return 0==pop_confirm(title, msg, {"OK", "Cancel"});
}
bool pop_yn(string const& title, string const& msg)
{
	return 0==pop_confirm(title, msg, {"Yes", "No"});
}
void pop_inf(string const& title, string const& msg)
{
	pop_confirm(title, msg, {"OK"});
}

