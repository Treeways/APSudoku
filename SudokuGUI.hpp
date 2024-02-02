#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <string>
#include <set>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
using std::string;
using std::set;
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
	;

#define C_BG C_LGRAY
#define C_TXT C_BLUE

void log(string const& msg);
void fail(string const& msg);

#define FL_SELECTED 0b00000001
#define FL_DISABLED 0b00000010
struct Button
{
	u16 x, y;
	u16 w = 64, h = 32;
	string text;
	ALLEGRO_COLOR c_txt = C_BLACK;
	ALLEGRO_COLOR c_distxt = C_LGRAY;
	ALLEGRO_COLOR c_bg = C_WHITE;
	ALLEGRO_COLOR c_border = C_BLACK;
	u8 flags;
	
	void draw() const;
	
	Button() = default;
	Button(string const& txt, u16 X, u16 Y);
};
