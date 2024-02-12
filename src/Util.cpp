#include "Main.hpp"
#include "Util.hpp"

u32 col_to_hex(ALLEGRO_COLOR const& c)
{
	u8 r,g,b,a;
	al_unmap_rgba(c, &r,&g,&b,&a);
	return rgba_to_hex(r,g,b,a);
}
u32 col_to_hex(ALLEGRO_COLOR const* c)
{
	return c ? col_to_hex(*c) : 0;
}
ALLEGRO_COLOR hex_to_col(u32 val)
{
	u8 r = (val>>24)&0xFF;
	u8 g = (val>>16)&0xFF;
	u8 b = (val>>8)&0xFF;
	u8 a = (val>>0)&0xFF;
	return al_map_rgba(r,g,b,a);
}

