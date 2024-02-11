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
	, C_HIGHLIGHT
	, C_HIGHLIGHT2
	;
extern double render_xscale, render_yscale;
extern int render_resx, render_resy;

struct InputObject;
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

struct DrawContainer : public vector<DrawnObject*>
{
	bool redraw = true;
	std::function<bool()> run_proc;
	virtual void run();
	virtual void draw();
	virtual bool mouse();
	virtual void on_disp_resize();
	virtual void key_event(ALLEGRO_EVENT const& ev);
	
	virtual void run_loop();
};

struct Dialog : public DrawContainer
{
	InputState state;
	virtual void run() override;
	virtual void draw() override;
	virtual bool mouse() override;
	virtual void on_disp_resize() override;
	virtual void key_event(ALLEGRO_EVENT const& ev) override;
	
	virtual void run_loop() override;
};

void clear_a5_bmp(ALLEGRO_COLOR col, ALLEGRO_BITMAP* bmp = nullptr);

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

struct DrawnObject
{
	u16 x, y;
	std::function<void()> onResizeDisplay;
	
	void on_disp_resize();
	virtual void draw() const {};
	virtual bool mouse() {return false;}
	virtual u16 xpos() const {return x;}
	virtual u16 ypos() const {return y;}
	virtual u16 width() const {return 0;}
	virtual u16 height() const {return 0;}
	virtual void key_event(ALLEGRO_EVENT const& ev) {}
	DrawnObject() = default;
	DrawnObject(u16 x, u16 y) : x(x), y(y) {}
};

struct InputObject : public DrawnObject
{
	u16 w, h;
	u8 mouseflags;
	std::function<u32(MouseEvent)> onMouse;
	virtual bool mouse() override;
	virtual u16 width() const override {return w;}
	virtual u16 height() const override {return h;}
	void unhover();
	InputObject() = default;
	InputObject(u16 x, u16 y)
		: DrawnObject(x, y), w(0), h(0), mouseflags(0), onMouse() {}
	InputObject(u16 x, u16 y, u16 w, u16 h)
		: DrawnObject(x, y), w(w), h(h), mouseflags(0), onMouse() {}
private:
	void process(u32 retcode);
};

void update_scale();
void scale_x(u16& x);
void scale_y(u16& y);
void scale_pos(u16& x, u16& y);
void scale_pos(u16& x, u16& y, u16& w, u16& h);
void unscale_x(u16& x);
void unscale_y(u16& y);
void unscale_pos(u16& x, u16& y);
void unscale_pos(u16& x, u16& y, u16& w, u16& h);

#define FL_SELECTED 0b00000001
#define FL_DISABLED 0b00000010
#define FL_HOVERED  0b00000100
struct Button : public InputObject
{
	string text;
	u8 flags;
	
	void draw() const override;
	bool mouse() override;
	
	Button();
	Button(string const& txt);
	Button(string const& txt, u16 X, u16 Y);
	Button(string const& txt, u16 X, u16 Y, u16 W, u16 H);
	
	u32 handle_ev(MouseEvent ev);
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

u32 mouse_killfocus(MouseEvent e);


bool pop_okc(string const& title, string const& msg);
bool pop_yn(string const& title, string const& msg);

