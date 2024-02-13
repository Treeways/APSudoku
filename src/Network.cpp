#include "Main.hpp"
#include "GUI.hpp"
#include "Network.hpp"
#include "../Archipelago.h"

deque<string> pending_deaths;
static void on_item_clear(){}
static void on_item_receive(i64,bool){}
static void on_location_checked(i64){}
static void on_death_recv(string src, string cause)
{
	string death_str = std::format("{} died to {}, bringing you down with them!",src,cause);
	log("DeathLink",death_str);
	pending_deaths.push_back(death_str);
}

void on_location_scouted(vector<AP_NetworkItem> vec)
{
	
}
bool process_remote_deaths()
{
	static bool mid_processing = false;
	if(mid_processing)
		return false;
	if(!ap_deathlink())
	{
		pending_deaths.clear();
		return false;
	}
	if(pending_deaths.empty())
		return false;
	mid_processing = true;
	while(!pending_deaths.empty())
	{
		string const& str = pending_deaths.front();
		pop_inf("DeathLink", str);
		pending_deaths.pop_front();
	}
	mid_processing = false;
	return true;
}

bool do_ap_death(string cause)
{
	bool ret = false;
	if(AP_DeathLinkSend(cause))
	{
		log("DeathLink", std::format("Sent death from: You {}", cause));
		pop_inf("You Died!", std::format("You perished because:\nYou {}", cause));
		ret = true;
	}
	return ret;
}
void do_ap_disconnect()
{
	AP_Disconnect();
}
void do_ap_connect(string& errtxt, string const& ip, string const& port,
	string const& slot, string const& pwd, optional<int> deathlink)
{
	if(port.empty())
	{
		errtxt = "Port is required!";
		return;
	}
	if(port.size() != 5 || port.find_first_not_of("0123456789") != string::npos)
	{
		errtxt = std::format("Port '{}' is invalid!", port);
		return;
	}
	if(ip.empty())
	{
		errtxt = "IP is required!";
		return;
	}
	if(slot.empty())
	{
		errtxt = "Slot is required!";
		return;
	}
	
	log(std::format("Connecting: '{}:{}', '{}', '{}'", ip,port,slot,pwd));
	AP_SetDeathLinkAlias(slot + "_APSudoku");
	
	string ip_port = std::format("{}:{}",ip,port);
	AP_Init(ip_port.c_str(), "", slot.c_str(), pwd.c_str());
	AP_SetDeathLinkForced(deathlink.has_value());
	AP_SetDeathAmnestyForced(deathlink.value_or(0));
	set<string> tags = {"Tracker","HintGame"};
	AP_SetTags(tags);
	
	log(std::format("\t with tags: {}", set_string(AP_GetTags())));
	
	AP_SetItemClearCallback(on_item_clear);
	AP_SetItemRecvCallback(on_item_receive);
	AP_SetLocationCheckedCallback(on_location_checked);
	AP_SetDeathLinkRecvCallback(on_death_recv);
	AP_SetLocationInfoCallback(on_location_scouted);
	AP_Start();
	errtxt.clear();
}

bool ap_connected()
{
	return AP_GetConnectionStatus() == AP_ConnectionStatus::Authenticated;
}
bool ap_deathlink()
{
	return ap_connected() && AP_GetTags().contains("DeathLink");
}

