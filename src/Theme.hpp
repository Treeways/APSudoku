#pragma once

#include "Main.hpp"

inline ALLEGRO_COLOR
	C_TRANS = al_map_rgba(0,0,0,0)
	, C_WHITE = al_map_rgb(255,255,255)
	, C_BLACK = al_map_rgb(0,0,0)
	, C_BLUE = al_map_rgb(30, 107, 229)
	, C_LLGRAY = al_map_rgb(224, 224, 224)
	, C_LGRAY = al_map_rgb(192, 192, 192)
	, C_MGRAY = al_map_rgb(128, 128, 128)
	, C_DGRAY = al_map_rgb(64, 64, 64)
	;

enum PalIndex
{
#define X(name,capsname,...) \
	C_##capsname,
#include "Theme.xtable"
#undef X
	PAL_SZ
};

struct Color
{
	ALLEGRO_COLOR const& get() const;
	Color(ALLEGRO_COLOR const& c);
	Color(const PalIndex indx);
private:
	optional<ALLEGRO_COLOR> c;
	optional<PalIndex> indx;
};

namespace Theme
{
	void reset(); // Resets the theme config to default
	void write_palette();
	void read_palette();
	string name(PalIndex ind);
}

