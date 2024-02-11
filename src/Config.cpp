#include "Main.hpp"
#include "Config.hpp"

ALLEGRO_CONFIG* config = nullptr;

void set_config_int(char const* sec, char const* key, int val)
{
	set_config_str(sec, key, to_string(val));
}
optional<int> get_config_int(char const* sec, char const* key)
{
	auto str = get_config_str(sec,key);
	if(!str) return nullopt;
	return std::stoi(*str);
}
void set_config_dbl(char const* sec, char const* key, double val)
{
	set_config_str(sec, key, to_string(val));
}
optional<double> get_config_dbl(char const* sec, char const* key)
{
	auto str = get_config_str(sec,key);
	if(!str) return nullopt;
	return std::stod(*str);
}
void set_config_str(char const* sec, char const* key, char const* val)
{
	al_set_config_value(config, sec, key, val);
}
void set_config_str(char const* sec, char const* key, string const& val)
{
	set_config_str(sec, key, val.c_str());
}
optional<string> get_config_str(char const* sec, char const* key)
{
	char const* str = al_get_config_value(config,sec,key);
	if(!str) return nullopt;
	return string(str);
}
void set_config_hex(char const* sec, char const* key, u32 val)
{
	set_config_str(sec, key, std::format("0x{:08X}", val));
}
optional<u32> get_config_hex(char const* sec, char const* key)
{
	auto str = get_config_str(sec,key);
	if(!str) return nullopt;
	string& s = *str;
	if(s.size() > 1 && s[0] == '0' && s[1] == 'x')
		s = s.substr(2);
	return std::stoul(s, nullptr, 16);
}
void set_config_col(char const* sec, char const* key, ALLEGRO_COLOR const& val)
{
	set_config_hex(sec, key, col_to_hex(val));
}
optional<ALLEGRO_COLOR> get_config_col(char const* sec, char const* key)
{
	auto v = get_config_hex(sec,key);
	if(!v) return nullopt;
	return hex_to_col(*v);
}

