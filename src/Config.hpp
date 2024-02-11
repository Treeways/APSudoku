#pragma once

#include "Main.hpp"
#include "Util.hpp"

void set_config_int(char const* sec, char const* key, int val);
optional<int> get_config_int(char const* sec, char const* key);
void set_config_dbl(char const* sec, char const* key, double val);
optional<double> get_config_dbl(char const* sec, char const* key);
void set_config_str(char const* sec, char const* key, char const* val);
void set_config_str(char const* sec, char const* key, string const& val);
optional<string> get_config_str(char const* sec, char const* key);
void set_config_hex(char const* sec, char const* key, u32 val);
optional<u32> get_config_hex(char const* sec, char const* key);
void set_config_col(char const* sec, char const* key, ALLEGRO_COLOR const& val);
optional<ALLEGRO_COLOR> get_config_col(char const* sec, char const* key);

