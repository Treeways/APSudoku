#pragma once

#include "Main.hpp"
#include "Font.hpp"
inline ALLEGRO_COLOR
	C_TRANS = al_map_rgba(0,0,0,0)
	, C_WHITE = al_map_rgb(255,255,255)
	, C_BLACK = al_map_rgb(0,0,0)
	, C_BLUE = al_map_rgb(30, 107, 229)
	, C_LGRAY = al_map_rgb(192, 192, 192)
	, C_MGRAY = al_map_rgb(128, 128, 128)
	, C_DGRAY = al_map_rgb(64, 64, 64)
	;
extern ALLEGRO_COLOR
	C_BG
	, C_CELL_BG
	, C_CELL_BORDER
	, C_CELL_TEXT
	, C_CELL_GIVEN
	, C_REGION_BORDER
	, C_LBL_TEXT
	, C_LBL_SHADOW
	, C_BUTTON_FG
	, C_BUTTON_BG
	, C_BUTTON_HOVBG
	, C_BUTTON_BORDER
	, C_BUTTON_DISTXT
	, C_RAD_BG
	, C_RAD_HOVBG
	, C_RAD_FG
	, C_RAD_DIS_BG
	, C_RAD_DIS_FG
	, C_RAD_BORDER
	, C_RAD_TXT
	, C_HIGHLIGHT
	, C_HIGHLIGHT2
	;
extern double render_xscale, render_yscale;
extern int render_resx, render_resy;

struct InputObject;
struct GUIObject;
struct InputState : public ALLEGRO_MOUSE_STATE
{
	int oldstate = 0;
	InputObject* hovered = nullptr;
	InputObject* focused = nullptr;
	
	bool lclick() const {return (buttons&0x7) == 0x1 && !(oldstate&0x7);}
	bool rclick() const {return (buttons&0x7) == 0x2 && !(oldstate&0x7);}
	bool mclick() const {return (buttons&0x7) == 0x4 && !(oldstate&0x7);}
	bool lrelease() const {return !(buttons&0x1) && (oldstate&0x7) == 0x1;}
	bool rrelease() const {return !(buttons&0x2) && (oldstate&0x7) == 0x2;}
	bool mrelease() const {return !(buttons&0x4) && (oldstate&0x7) == 0x4;}
	bool shift() const;
	bool ctrl_cmd() const;
	bool alt() const;
};
extern InputState* cur_input;

struct DrawContainer : public vector<shared_ptr<GUIObject>>
{
	GUIObject const* draw_parent = nullptr;
	bool redraw = true;
	std::function<bool()> run_proc;
	virtual void run();
	virtual void draw() const;
	virtual bool mouse();
	virtual void on_disp_resize();
	virtual void key_event(ALLEGRO_EVENT const& ev);
	
	virtual void run_loop();
};

struct Dialog : public DrawContainer
{
	InputState state;
	virtual void run() override;
	virtual void draw() const override;
	virtual bool mouse() override;
	virtual void on_disp_resize() override;
	virtual void key_event(ALLEGRO_EVENT const& ev) override;
	
	virtual void run_loop() override;
};

void clear_a5_bmp(ALLEGRO_COLOR col, ALLEGRO_BITMAP* bmp = nullptr);

void update_scale();
void scale_min(u16& v);
void scale_max(u16& v);
void scale_x(u16& x);
void scale_y(u16& y);
void scale_pos(u16& x, u16& y);
void scale_pos(u16& x, u16& y, u16& w, u16& h);
void unscale_min(u16& v);
void unscale_max(u16& v);
void unscale_x(u16& x);
void unscale_y(u16& y);
void unscale_pos(u16& x, u16& y);
void unscale_pos(u16& x, u16& y, u16& w, u16& h);

enum MouseEvent
{
	MOUSE_HOVER_ENTER,
	MOUSE_HOVER_EXIT,
	MOUSE_LOSTFOCUS,
	MOUSE_LCLICK,
	MOUSE_RCLICK,
	MOUSE_MCLICK,
	MOUSE_LRELEASE,
	MOUSE_RRELEASE,
	MOUSE_MRELEASE,
	MOUSE_LDOWN,
	MOUSE_RDOWN,
	MOUSE_MDOWN,
};

#define MFL_HASMOUSE 0x01

#define MRET_OK         (u32(0x00))
#define MRET_TAKEFOCUS  (u32(0x01))

#define FL_SELECTED 0b00000001
#define FL_DISABLED 0b00000010
#define FL_HOVERED  0b00000100

struct DrawWrapper;
struct GUIObject
{
	u8 flags;
	std::function<void()> onResizeDisplay;
	std::function<bool()> dis_proc;
	
	GUIObject const* draw_parent;
	
	void on_disp_resize();
	virtual void draw() const {};
	virtual bool mouse() {return false;}
	virtual u16 xpos() const {return 0;}
	virtual u16 ypos() const {return 0;}
	virtual void xpos(u16 v) {}
	virtual void ypos(u16 v) {}
	virtual u16 width() const {return 0;}
	virtual u16 height() const {return 0;}
	virtual bool disabled() const;
	virtual void key_event(ALLEGRO_EVENT const& ev) {}
	virtual void realign(size_t start = 0) {}
	GUIObject()
		: flags(0), onResizeDisplay(), dis_proc(), draw_parent(nullptr)
	{}
};

struct InputObject : public GUIObject
{
	u16 x, y, w, h;
	u8 mouseflags;
	std::function<u32(InputObject&,MouseEvent)> onMouse;
	virtual bool mouse() override;
	virtual u16 xpos() const override {return x;}
	virtual u16 ypos() const override {return y;}
	virtual void xpos(u16 v) override {x = v + (x-xpos());}
	virtual void ypos(u16 v) override {y = v + (y-ypos());}
	virtual u16 width() const override {return w;}
	virtual u16 height() const override {return h;}
	virtual u32 handle_ev(MouseEvent ev) {return MRET_OK;};
	void unhover();
	InputObject()
		: GUIObject(), x(0), y(0), w(0), h(0), mouseflags(0), onMouse() {}
	InputObject(u16 x, u16 y)
		: GUIObject(), x(x), y(y), w(0), h(0), mouseflags(0), onMouse() {}
	InputObject(u16 x, u16 y, u16 w, u16 h)
		: GUIObject(), x(x), y(y), w(w), h(h), mouseflags(0), onMouse() {}
private:
	void process(u32 retcode);
};

struct DrawWrapper : public InputObject
{
	DrawContainer cont;
	virtual void draw() const override;
	virtual bool mouse() override;
	virtual u16 width() const override = 0;
	virtual u16 height() const override = 0;
	
	virtual void add(shared_ptr<GUIObject> obj) {cont.emplace_back(obj);}
	DrawWrapper() = default;
	DrawWrapper(u16 X, u16 Y) : InputObject(X,Y) {}
};

struct Column : public DrawWrapper
{
	u16 padding, spacing;
	i8 align;
	void realign(size_t start = 0);
	virtual u16 width() const override;
	virtual u16 height() const override;
	virtual void add(shared_ptr<GUIObject> obj) override;
	Column() : padding(4), spacing(2), align(ALLEGRO_ALIGN_CENTRE) {};
	Column(u16 X, u16 Y, u16 padding = 4,
		u16 spacing = 2, i8 align = ALLEGRO_ALIGN_CENTRE);
};
struct Row : public DrawWrapper
{
	u16 padding, spacing;
	i8 align;
	void realign(size_t start = 0);
	virtual u16 width() const override;
	virtual u16 height() const override;
	virtual void add(shared_ptr<GUIObject> obj) override;
	Row() : padding(4), spacing(2), align(ALLEGRO_ALIGN_CENTRE) {};
	Row(u16 X, u16 Y, u16 padding = 4,
		u16 spacing = 2, i8 align = ALLEGRO_ALIGN_CENTRE);
};

struct RadioSet;
struct RadioButton : public InputObject
{
	u16 radius;
	string text;
	FontDef font;
	RadioSet* parent;
	static const u16 pad = 4;
	
	virtual void draw() const override;
	virtual bool mouse() override;
	virtual u16 xpos() const override;
	virtual u16 ypos() const override;
	virtual u16 width() const override;
	virtual u16 height() const override;
	virtual bool disabled() const override;
	RadioButton() : text(), font(FontDef(-20, false, BOLD_NONE)),
		radius(4), parent(nullptr)
	{}
	RadioButton(string const& txt, FontDef fnt, u16 rad = 4)
		: radius(rad), text(txt), font(fnt), parent(nullptr)
	{}
	RadioButton(u16 X, u16 Y, string const& txt, FontDef fnt, u16 rad = 4)
		: InputObject(X,Y), radius(rad), text(txt), font(fnt), parent(nullptr)
	{}
	
	u32 handle_ev(MouseEvent ev) override;
};
struct RadioSet : public Column
{
	RadioSet(vector<string> opts, FontDef fnt, u16 rad = 4)
		: Column()
	{
		init(opts, fnt, rad);
		align = ALLEGRO_ALIGN_LEFT;
		realign();
	}
	RadioSet(u16 X, u16 Y, vector<string> opts, FontDef fnt,
		u16 rad = 4, u16 padding = 4, u16 spacing = 2)
		: Column(X,Y,padding,spacing)
	{
		init(opts, fnt, rad);
		align = ALLEGRO_ALIGN_LEFT;
		realign();
	}
	optional<u16> get_sel();
	void select(u16 ind);
private:
	void init(vector<string>& opts, FontDef fnt, u16 rad);
	friend struct RadioButton;
	void deselect();
};
struct CheckBox : public RadioButton
{
	virtual void draw() const override;
	CheckBox() : RadioButton() {}
	CheckBox(string const& txt, FontDef fnt, u16 rad = 4)
		: RadioButton(txt,fnt,rad)
	{}
	CheckBox(u16 X, u16 Y, string const& txt, FontDef fnt, u16 rad = 4)
		: RadioButton(X,Y,txt,fnt,rad)
	{}
};

struct Button : public InputObject
{
	string text;
	FontDef font;
	
	void draw() const override;
	bool mouse() override;
	
	Button();
	Button(string const& txt);
	Button(string const& txt, FontDef fnt);
	Button(string const& txt, FontDef fnt, u16 X, u16 Y);
	Button(string const& txt, FontDef fnt, u16 X, u16 Y, u16 W, u16 H);
	
	u32 handle_ev(MouseEvent ev) override;
};

struct ShapeRect : public InputObject
{
	ALLEGRO_COLOR c_fill = C_BG;
	optional<ALLEGRO_COLOR> c_border = nullopt;
	double brd_thick;
	
	void draw() const override;
	virtual bool mouse() override;
	
	ShapeRect();
	ShapeRect(u16 X, u16 Y, u16 W, u16 H);
	ShapeRect(u16 X, u16 Y, u16 W, u16 H, ALLEGRO_COLOR c);
	ShapeRect(u16 X, u16 Y, u16 W, u16 H, ALLEGRO_COLOR c, ALLEGRO_COLOR cb, double border_thick);
};

void draw_text(string const& str, i8 align, FontDef font, ALLEGRO_COLOR c_txt, optional<ALLEGRO_COLOR> c_shadow = nullopt);
struct Label : public InputObject
{
	string text;
	i8 align;
	FontDef font;
	
	void draw() const override;
	u16 xpos() const override;
	//u16 ypos() const override; //add valign?
	u16 width() const override;
	u16 height() const override;
	
	Label();
	Label(string const& txt);
	Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 align = ALLEGRO_ALIGN_CENTRE);
};

u32 mouse_killfocus(InputObject& ref,MouseEvent e);


bool pop_okc(string const& title, string const& msg);
bool pop_yn(string const& title, string const& msg);
void pop_inf(string const& title, string const& msg);

