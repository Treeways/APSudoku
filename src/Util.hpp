#pragma once

#include "Main.hpp"

u32 col_to_hex(ALLEGRO_COLOR const& c);
u32 col_to_hex(ALLEGRO_COLOR const* c);
ALLEGRO_COLOR hex_to_col(u32 val);
u32 rgb_to_hex(u8 r, u8 g, u8 b);
u32 rgba_to_hex(u8 r, u8 g, u8 b, u8 a);

template<typename T>
T vbound(T v, T low, T high)
{
	if(low > high)
	{
		T tmp = low;
		low = high;
		high = tmp;
	}
	if(v < low)
		return low;
	if(v > high)
		return high;
	return v;
}
template<typename T>
T vbound(T v, T low, T high, bool& bounded)
{
	if(low > high)
	{
		T tmp = low;
		low = high;
		high = tmp;
	}
	bounded = true;
	if(v < low)
		return low;
	if(v > high)
		return high;
	bounded = false;
	return v;
}

