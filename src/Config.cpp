#include "Main.hpp"
#include "Config.hpp"

ALLEGRO_CONFIG* config = nullptr;

ALLEGRO_CONFIG* configs[NUM_CFGS];
void set_cfg(Config c)
{
	config = configs[c];
}

void set_config_bool(char const* sec, char const* key, bool val)
{
	set_config_str(sec, key, val ? "true" : "false");
}
optional<bool> get_config_bool(char const* sec, char const* key)
{
	optional<string> optstr = get_config_str(sec,key);
	if(!optstr) return nullopt;
	string& str = *optstr;
	for(char& c : str)
		c = tolower(c);
	if(str == "true")
		return true;
	if(str == "false")
		return false;
	//write it back to the config as 'true' or 'false'...
	bool ret = false;
	if(str == "yes")
		ret = true;
	else if(str == "no")
		ret = false;
	else if(str.empty())
		ret = false;
	else if(str[0] >= '0' && str[0] <= '9')
		ret = std::stoi(str) != 0;
	set_config_bool(sec,key,ret);
	return ret;
}

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
	string dblstr = to_string(val);
	size_t dotpos = dblstr.find_first_of('.');
	if(dotpos != string::npos) //trim extra 0s
	{
		size_t nzpos = dblstr.find_last_not_of('0');
		if(nzpos == dotpos) ++nzpos;
		dblstr = dblstr.substr(0,nzpos+1);
	}
	set_config_str(sec, key, dblstr);
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

