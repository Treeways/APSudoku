#pragma once

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <map>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <format>
#include <optional>
#include <functional>
#include <memory>
#include <tuple>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <random>
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned int uint;
using std::pair;
using std::tuple;
using std::string;
using std::stringstream;
using std::set;
using std::map;
using std::vector;
using std::deque;
using std::to_string;
using std::optional;
using std::nullopt;
using std::shared_ptr;
using std::make_shared;
using std::format;

extern std::mt19937 rng;
u64 rand(u64 range);
u64 rand(u64 min, u64 max);

struct FontDef;
struct GUIObject;
struct InputObject;
struct DrawContainer;
struct AP_NetworkItem;

extern ALLEGRO_DISPLAY* display;

extern vector<DrawContainer*> popups;
extern u64 cur_frame;

void dlg_draw();
void dlg_render();
void run_events(bool& redraw);
bool events_empty();
void on_resize();

extern bool program_running;
extern bool settings_unsaved;

struct Hint
{
	string entrance;
	int finding_player, receiving_player;
	bool found;
	int item, location;
	int item_flags;
	operator string() const;
	Hint(Json::Value const& v);
	Hint(AP_NetworkItem const& itm);
};

enum Screen
{
	SCR_SUDOKU,
	SCR_CONNECT,
	SCR_SETTINGS,
	NUM_SCRS
};
enum EntryMode
{
	ENT_ANSWER,
	ENT_CENTER,
	ENT_CORNER,
	NUM_ENT
};
extern EntryMode mode;
EntryMode get_mode();
bool mode_mod();

enum Difficulty
{
	DIFF_EASY,
	DIFF_NORMAL,
	DIFF_HARD,
	NUM_DIFF
};
extern Difficulty diff;

#define CANVAS_W 640
#define CANVAS_H 352

enum direction
{
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_UPLEFT,
	DIR_UPRIGHT,
	DIR_DOWNLEFT,
	DIR_DOWNRIGHT,
	NUM_DIRS
};

void log(string const& hdr, string const& msg);
void error(string const& hdr, string const& msg);
void fail(string const& hdr, string const& msg);
void log(string const& msg);
void error(string const& msg);
void fail(string const& msg);

class sudoku_exception : public std::exception
{
public:
	virtual const char * what() const noexcept override
	{
		return msg.c_str();
	}
	sudoku_exception(string const& msg) : msg(msg)
	{}
private:
	string msg;
};

#include "Util.hpp"
