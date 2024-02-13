#pragma once

#include "Main.hpp"

bool process_remote_deaths();
bool do_ap_death(string cause);
void do_ap_disconnect();
void do_ap_connect(string& errtxt, string const& ip, string const& port,
	string const& slot, string const& pwd, optional<int> deathlink);
bool ap_connected();
bool ap_deathlink();
