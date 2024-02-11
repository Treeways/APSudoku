#pragma once

#include "Main.hpp"

u32 col_to_hex(ALLEGRO_COLOR const& c);
u32 col_to_hex(ALLEGRO_COLOR const* c);
ALLEGRO_COLOR hex_to_col(u32 val);