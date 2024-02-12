#pragma once

#include "Main.hpp"
#include "Font.hpp"
#include "Theme.hpp"
extern ALLEGRO_COLOR
	C_CELL_BG
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
	, C_TF_BG
	, C_TF_FG
	, C_TF_DIS_BG
	, C_TF_DIS_FG
	, C_TF_HOVBG
	, C_TF_BORDER
	, C_TF_CURSOR
	, C_HIGHLIGHT
	, C_HIGHLIGHT2
	;
extern double render_xscale, render_yscale;
extern int render_resx, render_resy;

struct ClipRect
{
	int x,y,w,h;
	void store();
	void load();
	ClipRect();
	static void set(int X, int Y, int W, int H);
};

struct InputObject;
struct GUIObject;
struct InputState : public ALLEGRO_MOUSE_STATE
{
	int oldstate = 0;
	InputObject* hovered = nullptr;
	InputObject* focused = nullptr;
	InputObject* new_focus = nullptr;
	
	bool lclick() const {return (buttons&0x7) == 0x1 && !(oldstate&0x7);}
	bool rclick() const {return (buttons&0x7) == 0x2 && !(oldstate&0x7);}
	bool mclick() const {return (buttons&0x7) == 0x4 && !(oldstate&0x7);}
	bool lrelease() const {return !(buttons&0x1) && (oldstate&0x7) == 0x1;}
	bool rrelease() const {return !(buttons&0x2) && (oldstate&0x7) == 0x2;}
	bool mrelease() const {return !(buttons&0x4) && (oldstate&0x7) == 0x4;}
	bool shift() const;
	bool ctrl_cmd() const;
	bool alt() const;
	bool unfocus();
	bool refocus(InputObject* targ);
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

void clear_a5_bmp(Color col, ALLEGRO_BITMAP* bmp = nullptr);

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
	MOUSE_GOTFOCUS,
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
	//Obj is disabled if this returns true, OR flags&FL_DISABLED
	std::function<bool()> dis_proc;
	//If set, obj is selected iff returns true,
	//ELSE obj is selected if flags&FL_SELECT
	std::function<bool()> sel_proc;
	
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
	virtual bool selected() const;
	virtual void key_event(ALLEGRO_EVENT const& ev) {}
	virtual void realign(size_t start = 0) {}
	virtual void focus(){}
	bool focused() const;
	GUIObject()
		: flags(0), onResizeDisplay(), dis_proc(), sel_proc(), draw_parent(nullptr)
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
	virtual u32 handle_ev(MouseEvent ev);
	virtual void focus() override;
	u32 mouse_event(MouseEvent ev);
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
	Column() : padding(0), spacing(2), align(ALLEGRO_ALIGN_CENTRE) {};
	Column(u16 X, u16 Y, u16 padding = 0,
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

struct RadioButton : public InputObject
{
	u16 radius, fillrad;
	string text;
	FontDef font;
	static const u16 pad = 4;
	static double fill_sel;
	
	virtual void draw() const override;
	virtual u16 xpos() const override;
	virtual u16 ypos() const override;
	virtual u16 width() const override;
	virtual u16 height() const override;
	RadioButton() : text(), font(FontDef(-20, false, BOLD_NONE)),
		radius(4), fillrad(2)
	{}
	RadioButton(string const& txt, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: radius(rad), fillrad(frad), text(txt), font(fnt)
	{}
	RadioButton(u16 X, u16 Y, string const& txt, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: InputObject(X,Y), radius(rad), fillrad(frad), text(txt), font(fnt)
	{}
	
	u32 handle_ev(MouseEvent ev) override;
};
struct RadioSet : public Column
{
	RadioSet(vector<string> opts, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: Column(0,0,0,2,ALLEGRO_ALIGN_LEFT)
	{
		init(opts, fnt, rad, frad);
		realign();
	}
	RadioSet(std::function<optional<u8>()> getter,
		std::function<void(optional<u8>)> setter,
		vector<string> opts, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: Column(0,0,0,2,ALLEGRO_ALIGN_LEFT),
		get_sel_proc(getter), set_sel_proc(setter)
	{
		init(opts, fnt, rad, frad);
		realign();
	}
	optional<u16> get_sel() const;
	void select(optional<u16> ind);
private:
	optional<u16> sel_ind;
	std::function<optional<u8>()> get_sel_proc;
	std::function<void(optional<u8>)> set_sel_proc;
	void init(vector<string>& opts, FontDef fnt, u16 rad, u16 frad);
};
struct CheckBox : public RadioButton
{
	static double fill_sel;
	virtual void draw() const override;
	CheckBox() : RadioButton() {}
	CheckBox(string const& txt, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: RadioButton(txt,fnt,rad,frad)
	{}
	CheckBox(u16 X, u16 Y, string const& txt, FontDef fnt, u16 rad = 4, u16 frad = 2)
		: RadioButton(X,Y,txt,fnt,rad,frad)
	{}
};

struct Button : public InputObject
{
	string text;
	FontDef font;
	
	void draw() const override;
	
	Button();
	Button(string const& txt);
	Button(string const& txt, FontDef fnt);
	Button(string const& txt, FontDef fnt, u16 X, u16 Y);
	Button(string const& txt, FontDef fnt, u16 X, u16 Y, u16 W, u16 H);
	
	u32 handle_ev(MouseEvent ev) override;
};

struct ShapeRect : public InputObject
{
	Color c_fill;
	optional<Color> c_border;
	double brd_thick;
	
	void draw() const override;
	
	ShapeRect();
	ShapeRect(u16 X, u16 Y, u16 W, u16 H);
	ShapeRect(u16 X, u16 Y, u16 W, u16 H, Color c);
	ShapeRect(u16 X, u16 Y, u16 W, u16 H, Color c, Color cb, double border_thick);
};

void draw_text(string const& str, i8 align, FontDef font, Color c_txt, optional<Color> c_shadow = nullopt);
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
	Label(string const& txt, FontDef fd, i8 align = ALLEGRO_ALIGN_CENTRE);
	Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 align = ALLEGRO_ALIGN_CENTRE);
};

struct TextField : public InputObject
{
	string content;
	FontDef font;
	u16 cpos;
	static const u16 pad = 2;
	std::function<bool(string const&,string const&,char)> onValidate;
	
	void draw() const override;
	u16 height() const override;
	
	TextField() : InputObject(0,0,64,0), content(),
		font(FontDef(-20, false, BOLD_NONE))
	{}
	TextField(string const& txt) : InputObject(0,0,64,0), content(txt),
		font(FontDef(-20, false, BOLD_NONE))
	{}
	TextField(string const& txt, FontDef fd)
		: InputObject(0,0,64,0), content(txt), font(fd)
	{}
	TextField(u16 X, u16 Y, u16 W, string const& txt, FontDef fd)
		: InputObject(X,Y,W,0), content(txt), font(fd)
	{}
	
	bool validate(string const& ostr, string const& nstr, char c);
	u32 handle_ev(MouseEvent ev) override;
	void key_event(ALLEGRO_EVENT const& ev) override;
};

u32 mouse_killfocus(InputObject& ref,MouseEvent e);
bool validate_numeric(string const& ostr, string const& nstr, char c);
bool validate_float(string const& ostr, string const& nstr, char c);
bool validate_alphanum(string const& ostr, string const& nstr, char c);

bool pop_okc(string const& title, string const& msg);
bool pop_yn(string const& title, string const& msg);
void pop_inf(string const& title, string const& msg);
