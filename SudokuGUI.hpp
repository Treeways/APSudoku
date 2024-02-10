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
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
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
	NUM_SCRS
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
enum EntryMode
{
	ENT_ANSWER,
	ENT_CENTER,
	ENT_CORNER,
	NUM_ENT
};
extern EntryMode mode;
EntryMode get_mode();

extern ALLEGRO_FONT* fonts[NUM_FONTS];

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

#define MRET_OK         0x00
#define MRET_TAKEFOCUS  0x01

struct DrawnObject
{
	u16 x, y;
	virtual void draw() const {};
	virtual bool mouse() {return false;}
	virtual void key_event(ALLEGRO_EVENT const& ev) {}
	DrawnObject() = default;
	DrawnObject(u16 x, u16 y) : x(x), y(y) {}
};

struct InputObject : public DrawnObject
{
	u16 w, h;
	u8 mouseflags;
	std::function<u32(MouseEvent)> onMouse;
	bool mouse() override;
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
extern InputState input_state;

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
	
	Button();
	Button(string const& txt, u16 X, u16 Y);
	Button(string const& txt, u16 X, u16 Y, u16 W, u16 H);
};

void update_scale();
void scale_x(u16& x);
void scale_y(u16& y);
void scale_pos(u16& x, u16& y);
void scale_pos(u16& x, u16& y, u16& w, u16& h);
