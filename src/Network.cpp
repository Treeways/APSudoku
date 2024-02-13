#include "Main.hpp"
#include "GUI.hpp"
#include "Network.hpp"
#include "../Archipelago.h"

static Json::Reader reader;
deque<string> pending_deaths;

map<int,AP_NetworkItem> missing_progressive, missing_basic;

static bool wait_conn()
{
	return !ap_connected();
}
static void ignore_loc_callback(vector<AP_NetworkItem> vec)
{
	AP_SetLocationInfoCallback(nullptr);
}

static void on_item_clear()
{
	missing_progressive.clear();
	missing_basic.clear();
}
static void on_connected()
{
	set<i64> const& locs = AP_GetMissingLocations();
	AP_SetLocationInfoCallback([](vector<AP_NetworkItem> vec)
		{
			for(AP_NetworkItem const& itm : vec)
			{
				auto* dest = &missing_basic;
				if(itm.flags & ITEMFL_PROGRESSION)
					dest = &missing_progressive;
				(*dest)[itm.location] = itm;
			}
			AP_SetLocationInfoCallback(nullptr);
		});
	AP_SendLocationScouts(locs,0);
}
static void on_item_receive(i64,bool){}
static void on_location_checked(int loc)
{
	missing_progressive.erase(loc);
	missing_basic.erase(loc);
}
static void on_death_recv(string src, string cause)
{
	string death_str = std::format("{} died to {}, bringing you down with them!",src,cause);
	log("DeathLink",death_str);
	pending_deaths.push_back(death_str);
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
	AP_SetLocationInfoCallback(nullptr);
	AP_SetConnectedCallback(on_connected);
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

void grant_hint()
{
	AP_GetServerDataRequest req;
	req.key = std::format("_read_hints_{}_{}", AP_GetPlayerTeam(), AP_GetPlayerID());
	req.type = AP_DataType::Raw;
	req.value = new string();
	AP_GetServerData(&req);
	
	optional<u8> ret;
	bool running = true;
	Dialog popup;
	popups.emplace_back(&popup);
	generate_popup(popup, ret, running, "Please Wait", "Loading hint data...", {});
	std::function<bool()> req_proc = [&req]()
		{
			if(req.status == AP_RequestStatus::Pending && ap_connected())
				return true;
			return false;
		};
	popup.run_proc = req_proc;
	popup.run_loop();
	
	string slotgame = AP_GetSlotGame();
	string const& val = *((string*)(req.value));
	Json::Value data;
	reader.parse(val, data);
	for(Json::Value& v : data)
		on_location_checked(v["location"].asInt());
	if(missing_progressive.empty() && missing_basic.empty())
		pop_inf("Hinted Out!","Nothing left to hint for this slot!");
	else
	{
		int basic = 0, prog = 0;
		switch(diff)
		{
			case DIFF_EASY:
				basic = 90;
				prog = 10;
				break;
			case DIFF_NORMAL:
				basic = 60;
				prog = 40;
				break;
			case DIFF_HARD:
				basic = 20;
				prog = 80;
				break;
		}
		if(missing_basic.empty())
			basic = 0;
		if(missing_progressive.empty())
			prog = 0;
		u64 val = rand(basic+prog);
		map<int,AP_NetworkItem>& valid_locations =
			val < basic ? missing_basic : missing_progressive;
		u64 indx = rand(valid_locations.size());
		auto it = valid_locations.begin();
		std::advance(it, indx);
		auto [loc,item] = *it;
		AP_SetLocationInfoCallback(ignore_loc_callback);
		AP_SendLocationScouts({loc},2);
		Hint h(item);
		log(h);
		pop_inf("Hint Earned:",h);
	}
	
	popups.pop_back();

	delete (string*)req.value;
}


string ap_get_playername(int playerid)
{
	if(playerid == AP_GetPlayerID())
		return "You";
	auto& pmap = AP_GetPlayerMap();
	auto it = pmap.find(playerid);
	if(it != pmap.end())
		return it->second.name_or_alias();
	return "";
}
string ap_get_itemname(int itemid)
{
	string slotgame = AP_GetSlotGame();
	for(auto [p,id] : AP_GetItemMap())
	{
		auto [s,v] = p;
		if(v != itemid)
			continue;
		if(s != slotgame)
			continue;
		return id;
	}
	return "";
}
optional<string> ap_get_itemflagstr(int itemflags)
{
	if(itemflags)
	{
		stringstream ret;
		ret << "(";
		if(itemflags & ITEMFL_PROGRESSION)
			ret << "Progression,";
		if(itemflags & ITEMFL_IMPORTANT)
			ret << "Useful,";
		if(itemflags & ITEMFL_TRAP)
			ret << "Trap,";
		ret.seekp(-1,ret.cur);
		ret << ")";
		return ret.str();
	}
	else return nullopt;
}
string ap_get_locationname(int locid)
{
	string slotgame = AP_GetSlotGame();
	for(auto [p,id] : AP_GetLocationMap())
	{
		auto [s,v] = p;
		if(v != locid)
			continue;
		if(s != slotgame)
			continue;
		return id;
	}
	return "";
}

