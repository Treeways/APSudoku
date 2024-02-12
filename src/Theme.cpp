#include "Main.hpp"
#include "Config.hpp"
#include "Theme.hpp"

using namespace Theme;

ALLEGRO_COLOR pal[PAL_SZ];

ALLEGRO_COLOR const& Color::get() const
{
	if(c) return *c;
	if(indx && unsigned(*indx) < PAL_SZ) return pal[*indx];
	
	static ALLEGRO_COLOR def_ret = al_map_rgb(255,0,255);
	error("INVALID COLOR ACCESS! Returning default color.");
	return def_ret;
}
Color::Color(ALLEGRO_COLOR const& c)
	: c(c), indx()
{}
Color::Color(const PalIndex indx)
	: c(), indx(indx)
{}

namespace Theme
{

static void set_color(char const* name, ALLEGRO_COLOR const& c)
{
	set_config_col("Color", name, c);
}
static void set_color(char const* name, const u32 c)
{
	set_config_hex("Color", name, c);
}
static void reset_palette()
{
	ConfigStash stash;
	set_cfg(CFG_THEME);
	
	#define X(name,capsname,def_c) \
	set_color(#name,def_c);
	#include "Theme.xtable"
	#undef X
}
void reset() // Resets the theme config to default
{
	ConfigStash stash;
	set_cfg(CFG_THEME);
	
	al_add_config_section(config, "Style");
	al_add_config_section(config, "Color");
	al_add_config_comment(config, "Color", "Colors given as 0xRRGGBBAA");
	set_config_dbl("Style", "fill_radiobubbles", 0.6);
	set_config_dbl("Style", "fill_cboxes", 0.6);
	
	reset_palette();
}

void write_palette()
{
	ConfigStash stash;
	set_cfg(CFG_THEME);
	
	#define X(name,capsname,def_c) \
	set_color(#name,pal[C_##capsname]);
	#include "Theme.xtable"
	#undef X
}

void read_palette()
{
	ConfigStash stash;
	set_cfg(CFG_THEME);
	
	#define X(name,capsname,def_c) \
	if(auto c = get_config_col("Color", #name)) \
		pal[C_##capsname] = *c;
	#include "Theme.xtable"
	#undef X
}

string name(PalIndex ind)
{
	switch(ind)
	{
		#define X(name,capsname,def_c) \
		case C_##capsname: \
			return #name;
		#include "Theme.xtable"
		#undef X
		default:
			return "<ERROR>";
	}
}

}

