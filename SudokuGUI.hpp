#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <optional>
#include <functional>
#include <tuple>
#include <stdint.h>
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;
using std::tuple;
using std::string;
using std::set;
using std::map;
using std::vector;
using std::to_string;
using std::optional;
using std::nullopt;

enum Screen
{
	SCR_SUDOKU,
	SCR_CONNECT,
	SCR_SETTINGS,
	NUM_SCRS
};
enum EntryMode
{
	ENT_ANSWER,
	ENT_CENTER,
	ENT_CORNER,
	NUM_ENT
};
extern EntryMode mode;
EntryMode get_mode();

inline ALLEGRO_COLOR
	C_TRANS = al_map_rgba(0,0,0,0)
	, C_WHITE = al_map_rgb(255,255,255)
	, C_BLACK = al_map_rgb(0,0,0)
	, C_BLUE = al_map_rgb(30, 107, 229)
	, C_LGRAY = al_map_rgb(192, 192, 192)
	, C_MGRAY = al_map_rgb(128, 128, 128)
	, C_DGRAY = al_map_rgb(64, 64, 64)
	, C_HIGHLIGHT = al_map_rgb(76, 164, 255)
	, C_HIGHLIGHT2 = al_map_rgb(255, 164, 76)
	;

enum direction
{
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UPLEFT,
	DIR_UPRIGHT,
	DIR_DOWNLEFT,
	DIR_DOWNRIGHT,
	NUM_DIRS
};

#define C_BG C_LGRAY
#define C_TXT C_BLUE
#define C_GIVEN C_BLACK

void log(string const& msg);
void fail(string const& msg);

enum Bold
{
	BOLD_LIGHT,
	BOLD_NONE,
	BOLD_SEMI,
	BOLD_NORMAL,
	BOLD_EXTRA,
	NUM_BOLDS
};
struct FontDef : public tuple<i16,bool,Bold> // A reference to a font
{
	i16& height() {return std::get<0>(*this);}
	i16 const& height() const {return std::get<0>(*this);}
	bool& italic() {return std::get<1>(*this);}
	bool const& italic() const {return std::get<1>(*this);}
	Bold& weight() {return std::get<2>(*this);}
	Bold const& weight() const {return std::get<2>(*this);}
	
	FontDef(i16 h = -20, bool ital = false, Bold b = BOLD_NONE);
	ALLEGRO_FONT* get() const;
	ALLEGRO_FONT* gen() const;
	//Unscaled, so, canvas-size
	ALLEGRO_FONT* get_base() const;
	ALLEGRO_FONT* gen_base() const;
};
enum Font
{
	FONT_BUTTON,
	FONT_ANSWER,
	FONT_MARKING5,
	FONT_MARKING6,
	FONT_MARKING7,
	FONT_MARKING8,
	FONT_MARKING9,
	NUM_FONTS
};
extern FontDef fonts[NUM_FONTS];

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

void update_scale();
void scale_x(u16& x);
void scale_y(u16& y);
void scale_pos(u16& x, u16& y);
void scale_pos(u16& x, u16& y, u16& w, u16& h);

#define FL_SELECTED 0b00000001
#define FL_DISABLED 0b00000010
#define FL_HOVERED  0b00000100
struct Button : public InputObject
{
	string text;
	ALLEGRO_COLOR c_txt = C_BLACK;
	ALLEGRO_COLOR c_distxt = C_LGRAY;
	ALLEGRO_COLOR c_bg = C_WHITE;
	ALLEGRO_COLOR c_hov_bg = C_LGRAY;
	ALLEGRO_COLOR c_border = C_BLACK;
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
	
	void draw() const override;
	virtual bool mouse() override;
	
	ShapeRect();
	ShapeRect(u16 X, u16 Y, u16 W, u16 H);
	ShapeRect(u16 X, u16 Y, u16 W, u16 H, ALLEGRO_COLOR c);
};

struct Label : public InputObject
{
	ALLEGRO_COLOR c_txt = C_BLACK;
	optional<ALLEGRO_COLOR> c_shadow = nullopt;
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
	Label(string const& txt, u16 X, u16 Y, FontDef fd, i8 align, ALLEGRO_COLOR fg, optional<ALLEGRO_COLOR> shd = nullopt);
};

u32 mouse_killfocus(MouseEvent e);
