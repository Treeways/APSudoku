#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
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

inline ALLEGRO_COLOR
	C_WHITE = al_map_rgb(255,255,255)
	, C_BLACK = al_map_rgb(0,0,0)
	, C_BLUE = al_map_rgb(30, 107, 229)
	, C_LGRAY = al_map_rgb(200, 200, 200)
	;

#define C_BG C_LGRAY
#define C_TXT C_BLUE

void log(string const& msg);
void fail(string const& msg);

