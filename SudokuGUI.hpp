#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <map>
#include <vector>
#include <set>
#include <string>
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
	FONT_MARKING,
	NUM_FONTS
};

extern ALLEGRO_FONT* fonts[NUM_FONTS];

inline ALLEGRO_COLOR
	C_WHITE = al_map_rgb(255,255,255)
	, C_BLACK = al_map_rgb(0,0,0)
	, C_BLUE = al_map_rgb(30, 107, 229)
	, C_LGRAY = al_map_rgb(192, 192, 192)
	, C_MGRAY = al_map_rgb(128, 128, 128)
	, C_DGRAY = al_map_rgb(64, 64, 64)
	, C_HIGHLIGHT = al_map_rgba(64, 64, 192, 128);
	;

#define C_BG C_LGRAY
#define C_TXT C_BLUE

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
	DrawnObject() = default;
	DrawnObject(u16 x, u16 y) : x(x), y(y) {}
};

struct MouseObject : public DrawnObject
{
	u16 w, h;
	u8 mouseflags;
	std::function<u32(MouseEvent)> onMouse;
	bool mouse() override;
	void unhover();
	MouseObject() = default;
	MouseObject(u16 x, u16 y)
		: DrawnObject(x, y), w(0), h(0), mouseflags(0), onMouse() {}
	MouseObject(u16 x, u16 y, u16 w, u16 h)
		: DrawnObject(x, y), w(w), h(h), mouseflags(0), onMouse() {}
private:
	void process(u32 retcode);
};
struct MouseState : public ALLEGRO_MOUSE_STATE
{
	int oldstate = 0;
	MouseObject* hovered = nullptr;
	MouseObject* focused = nullptr;
	
	bool lclick() const {return (buttons&0x7) == 0x1 && !(oldstate&0x7);}
	bool rclick() const {return (buttons&0x7) == 0x2 && !(oldstate&0x7);}
	bool mclick() const {return (buttons&0x7) == 0x4 && !(oldstate&0x7);}
	bool lrelease() const {return !(buttons&0x1) && (oldstate&0x7) == 0x1;}
	bool rrelease() const {return !(buttons&0x2) && (oldstate&0x7) == 0x2;}
	bool mrelease() const {return !(buttons&0x4) && (oldstate&0x7) == 0x4;}
};
extern MouseState mouse_state;

#define FL_SELECTED 0b00000001
#define FL_DISABLED 0b00000010
#define FL_HOVERED  0b00000100
struct Button : public MouseObject
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
