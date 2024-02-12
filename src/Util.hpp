#pragma once

#include "Main.hpp"

u32 col_to_hex(ALLEGRO_COLOR const& c);
u32 col_to_hex(ALLEGRO_COLOR const* c);
ALLEGRO_COLOR hex_to_col(u32 val);
u32 rgb_to_hex(u8 r, u8 g, u8 b);
u32 rgba_to_hex(u8 r, u8 g, u8 b, u8 a);