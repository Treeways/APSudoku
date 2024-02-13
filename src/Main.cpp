#include "../Archipelago.h"
#include <iostream>
#include <filesystem>
#include "GUI.hpp"
#include "Font.hpp"
#include "Config.hpp"
#include "SudokuGrid.hpp"
#include "Network.hpp"

void log(string const& hdr, string const& msg)
{
	std::cout << "[" << hdr << "] " << msg << std::endl;
}
void log(string const& msg)
{
	std::cout << "[LOG] " << msg << std::endl;
}
void error(string const& msg)
{
	std::cerr << "[ERROR] " << msg << std::endl;
}
void fail(string const& msg)
{
	std::cerr << "[FATAL] " << msg << std::endl;
	exit(1);
}

ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* canvas;
ALLEGRO_TIMER* timer;
ALLEGRO_EVENT_QUEUE* events;
ALLEGRO_EVENT_SOURCE event_source;

Difficulty diff = DIFF_NORMAL;
EntryMode mode = ENT_ANSWER;
EntryMode get_mode()
{
	if(cur_input->shift())
		return ENT_CORNER;
	if(cur_input->ctrl_cmd())
		return ENT_CENTER;
	return mode;
}
bool mode_mod()
{
	return cur_input->shift() || cur_input->ctrl_cmd();
}

#define GRID_X (32)
#define GRID_Y (32)
shared_ptr<Sudoku::Grid> grid;
#define BUTTON_X (CANVAS_W-32-96)
#define BUTTON_Y (32)
shared_ptr<Button> swap_btns[NUM_SCRS];
shared_ptr<RadioSet> difficulty, entry_mode;
shared_ptr<Label> connect_error;
shared_ptr<CheckBox> deathlink_cbox;
shared_ptr<TextField> deathlink_amnesty;
vector<shared_ptr<TextField>> ap_fields;

Screen curscr = SCR_SUDOKU;

map<Screen,DrawContainer> gui_objects;
vector<DrawContainer*> popups;
bool settings_unsaved = false;

void swap_screen(Screen scr)
{
	if(curscr == scr)
		return;
	if(curscr == SCR_SETTINGS && settings_unsaved)
	{
		if(!pop_yn("Change pages", "Swap pages without saving your settings?"))
			return;
	}
	curscr = scr;
}

void build_gui()
{
	using namespace Sudoku;
	const int GRID_SZ = CELL_SZ*9+2;
	const int GRID_X2 = GRID_X+GRID_SZ;
	const int GRID_Y2 = GRID_Y+GRID_SZ;
	const int RGRID_X = GRID_X2 + 8;
	FontDef font_l(-22, false, BOLD_NONE);
	FontDef font_s(-15,false,BOLD_NONE);
	{ // BG, to allow 'clicking off'
		shared_ptr<ShapeRect> bg = make_shared<ShapeRect>(0,0,CANVAS_W,CANVAS_H,C_BACKGROUND);
		bg->onMouse = mouse_killfocus;
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(bg);
	}
	{ // Swap buttons
		#define ON_SWAP_BTN(b, scr) \
		b->sel_proc = [](GUIObject const& ref) -> bool {return curscr == scr;}; \
		b->onMouse = [](InputObject& ref,MouseEvent e) \
			{ \
				switch(e) \
				{ \
					case MOUSE_LCLICK: \
						if(curscr != scr) swap_screen(scr); \
						return MRET_TAKEFOCUS; \
				} \
				return ref.handle_ev(e); \
			}
		swap_btns[0] = make_shared<Button>("Sudoku", font_l);
		swap_btns[1] = make_shared<Button>("Archipelago", font_l);
		swap_btns[2] = make_shared<Button>("Settings", font_l);
		ON_SWAP_BTN(swap_btns[0], SCR_SUDOKU);
		ON_SWAP_BTN(swap_btns[1], SCR_CONNECT);
		ON_SWAP_BTN(swap_btns[2], SCR_SETTINGS);
		shared_ptr<Column> sb_column = make_shared<Column>(BUTTON_X, BUTTON_Y, 0, 2, ALLEGRO_ALIGN_CENTER);
		for(shared_ptr<Button> const& b : swap_btns)
			sb_column->add(b);
		
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(sb_column);
	}
	{ // Grid
		grid = make_shared<Grid>(GRID_X, GRID_Y);
		grid->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LOSTFOCUS:
						if(cur_input->new_focus == entry_mode.get())
							return MRET_TAKEFOCUS;
						for(shared_ptr<GUIObject>& obj : entry_mode->cont)
							if(cur_input->new_focus == obj.get())
								return MRET_TAKEFOCUS;
						break;
				}
				return ref.handle_ev(e);
			};
		gui_objects[SCR_SUDOKU].push_back(grid);
		
		shared_ptr<Label> lifecnt = make_shared<Label>("", font_l, ALLEGRO_ALIGN_LEFT);
		lifecnt->setx(GRID_X);
		lifecnt->sety(GRID_Y-lifecnt->height()-2);
		lifecnt->text_proc = [](Label& ref) -> string
			{
				return std::format("Lives: {}", AP_GetCurrentDeathAmnesty());
			};
		lifecnt->vis_proc = [](GUIObject const& ref) -> bool
			{
				return grid->active() && ap_deathlink();
			};
		gui_objects[SCR_SUDOKU].push_back(lifecnt);
		
		shared_ptr<Label> nogame = make_shared<Label>("", font_l, ALLEGRO_ALIGN_CENTER);
		nogame->setx(GRID_X + GRID_SZ/2);
		nogame->sety(GRID_Y-nogame->height()-2);
		nogame->text_proc = [](Label& ref) -> string
			{
				if(grid->active())
				{
					ref.type = TYPE_NORMAL;
					return std::format("Playing: {}", *difficulty->get_sel_text());
				}
				else
				{
					ref.type = TYPE_ERROR;
					return "No Active Game";
				}
			};
		gui_objects[SCR_SUDOKU].push_back(nogame);
	}
	{ // Difficulty / game buttons
		shared_ptr<Column> diff_column = make_shared<Column>(RGRID_X,GRID_Y,0,2,ALLEGRO_ALIGN_LEFT);
		
		shared_ptr<Label> diff_lbl = make_shared<Label>("Difficulty:", font_s, ALLEGRO_ALIGN_LEFT);
		diff_column->add(diff_lbl);
		
		difficulty = make_shared<RadioSet>(
			[](){return diff;},
			[](optional<u16> v)
			{
				if(v)
					diff = Difficulty(*v);
			},
			vector<string>({"Easy","Normal","Hard"}),
			FontDef(-20, false, BOLD_NONE));
		difficulty->select(1);
		difficulty->dis_proc = [](GUIObject const& ref) -> bool
			{
				return grid->active();
			};
		diff_column->add(difficulty);
		
		shared_ptr<Button> start_btn = make_shared<Button>("Start", font_l);
		diff_column->add(start_btn);
		start_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(!ap_connected())
						{
							if(!pop_yn("Start Unconnected?",
								"Start a puzzle while not connected to Archipelago?"
								"\nNo hints will be earnable."))
								return MRET_OK;
						}
						grid->generate(diff);
						break;
				}
				return ref.handle_ev(e);
			};
		start_btn->dis_proc = [](GUIObject const& ref) -> bool
			{
				return grid->active();
			};
		
		shared_ptr<Button> forfeit_btn = make_shared<Button>("Forfeit", font_l);
		diff_column->add(forfeit_btn);
		forfeit_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(pop_yn("Forfeit", "Quit solving this puzzle?"
								+ string(ap_deathlink()?"\nThis will count as a DeathLink death!":"")))
						{
							do_ap_death("quit a sudoku puzzle!");
							grid->clear();
						}
						break;
				}
				return ref.handle_ev(e);
			};
		forfeit_btn->dis_proc = [](GUIObject const& ref) -> bool
			{
				return !grid->active();
			};
		
		shared_ptr<Button> check_btn = make_shared<Button>("Check", font_l);
		diff_column->add(check_btn);
		check_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(!grid->filled())
							pop_inf("Unfinished","Not all cells are filled!");
						else if(grid->check())
						{
							pop_inf("Correct","Puzzle solved correctly! WIP!");
							grid->exit();
						}
						else
						{
							pop_inf("Wrong", "Puzzle solution incorrect!");
							if(do_ap_death("solved a sudoku wrong!"))
								grid->exit();
						}
						break;
				}
				return ref.handle_ev(e);
			};
		check_btn->dis_proc = [](GUIObject const& ref) -> bool
			{
				return !grid->active();
			};
		diff_column->sety(GRID_Y2-diff_column->height());
		diff_column->realign();
		gui_objects[SCR_SUDOKU].push_back(diff_column);
	}
	{ // Entry mode toggle
		shared_ptr<Column> entry_col = make_shared<Column>(RGRID_X,GRID_Y,0,2,ALLEGRO_ALIGN_LEFT);
		
		shared_ptr<Label> entry_lbl = make_shared<Label>("Entry Mode:", font_s, ALLEGRO_ALIGN_LEFT);
		entry_col->add(entry_lbl);
		
		entry_mode = make_shared<RadioSet>(
			[]()
			{
				if(grid->focused())
					return get_mode();
				return mode;
			},
			[](optional<u16> v)
			{
				if(v)
					mode = EntryMode(*v);
			},
			vector<string>({"Answer","Center","Corner"}),
			FontDef(-20, false, BOLD_NONE));
		entry_mode->select(ENT_ANSWER);
		entry_mode->dis_proc = [](GUIObject const& ref) -> bool {return grid->focused() && mode_mod();};
		entry_col->add(entry_mode);
		entry_mode->onMouse = [](InputObject& ref,MouseEvent e)
			{
				if(e == MOUSE_GOTFOCUS)
				{
					grid->focus();
					return MRET_OK;
				}
				return ref.handle_ev(e);
			};
		
		gui_objects[SCR_SUDOKU].push_back(entry_col);
	}
	{ // AP connection
		shared_ptr<Column> apc = make_shared<Column>(GRID_X, GRID_Y, 0, 8, ALLEGRO_ALIGN_RIGHT);
		apc->dis_proc = [](GUIObject const& ref) -> bool
			{
				return ap_connected();
			};
		
		vector<tuple<string,string,
			std::function<bool(string const&,string const&,char)>>> p = {
			{"IP:","archipelago.gg", [](string const& o, string const& n, char c)
				{
					return validate_alphanum(o,n,c) || c == '.';
				}},
			{"Port:","57298", [](string const& o, string const& n, char c)
				{
					return validate_numeric(o,n,c) && n.size() <= 5;
				}},
			{"Slot:","EmV",nullptr},
			{"Passwd:","",nullptr}
		};
		ap_fields.clear();
		for(auto [label,defval,valproc] : p)
		{
			shared_ptr<Row> r = make_shared<Row>(0,0,0,4,ALLEGRO_ALIGN_CENTER);
			
			shared_ptr<Label> lbl = make_shared<Label>(label, font_l, ALLEGRO_ALIGN_LEFT);
			shared_ptr<TextField> field = make_shared<TextField>(
				GRID_X, GRID_Y, 256,
				defval, font_l);
			field->onValidate = valproc;
			r->add(lbl);
			r->add(field);
			ap_fields.push_back(field);
			apc->add(r);
		}
		if(!ap_fields.empty())
		{
			for(size_t q = 0; q < ap_fields.size()-1; ++q)
				ap_fields[q]->tab_target = ap_fields[q+1].get();
			ap_fields.back()->tab_target = ap_fields.front().get();
		}
		gui_objects[SCR_CONNECT].push_back(apc);
		
		deathlink_cbox = make_shared<CheckBox>("DeathLink", font_l);
		deathlink_cbox->setx(apc->xpos()+apc->width()+4);
		deathlink_cbox->sety(apc->ypos());
		deathlink_cbox->dis_proc = [](GUIObject const& ref) -> bool
			{
				return ap_connected();
			};
		gui_objects[SCR_CONNECT].push_back(deathlink_cbox);
		
		shared_ptr<Row> amnesty_row = make_shared<Row>();
		amnesty_row->setx(deathlink_cbox->xpos());
		amnesty_row->sety(deathlink_cbox->ypos()+deathlink_cbox->height()+4);
		
		shared_ptr<Label> amnesty_lbl = make_shared<Label>("Lives:",font_s);
		amnesty_row->add(amnesty_lbl);
		deathlink_amnesty = make_shared<TextField>("0", font_l);
		deathlink_amnesty->onValidate = validate_numeric;
		amnesty_row->dis_proc = [](GUIObject const& ref) -> bool
			{
				return ap_connected() || !(deathlink_cbox->selected());
			};
		amnesty_row->add(deathlink_amnesty);
		gui_objects[SCR_CONNECT].push_back(amnesty_row);
		
		shared_ptr<Switcher> sw = make_shared<Switcher>(
			[]()
			{
				return ap_connected() ? 1 : 0;
			},
			[](optional<u16> v){}
		);
		shared_ptr<DrawContainer> cont_on = make_shared<DrawContainer>();
		shared_ptr<DrawContainer> cont_off = make_shared<DrawContainer>();
		
		shared_ptr<Button> connect_btn = make_shared<Button>("Connect", font_l);
		shared_ptr<Button> disconnect_btn = make_shared<Button>("Disconnect", font_l);
		connect_btn->setx(apc->xpos());
		connect_btn->sety(apc->ypos() + apc->height() + 4);
		disconnect_btn->setx(connect_btn->xpos());
		disconnect_btn->sety(connect_btn->ypos());
		connect_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
					{
						if(grid->active())
						{
							if(!pop_yn("Forfeit", "Quit solving current puzzle?"))
								return MRET_OK;
							//do_ap_death("quit a sudoku puzzle!");//can't be connected here
							grid->clear();
						}
						ref.flags |= FL_SELECTED;
						ref.focus();
						string& errtxt = connect_error->text;
						do_ap_connect(errtxt, ap_fields[0]->get_str(),
							ap_fields[1]->get_str(), ap_fields[2]->get_str(),
							ap_fields[3]->get_str(), deathlink_cbox->selected() ? optional<int>(deathlink_amnesty->get_int()) : nullopt);
						
						if(!errtxt.empty())
							return MRET_OK; //failed, error handled
						auto status = AP_GetConnectionStatus();
						
						optional<u8> ret;
						bool running = true;
						
						Dialog popup;
						popups.emplace_back(&popup);
						generate_popup(popup, ret, running, "Please Wait", "Connecting...", {"Cancel"});
						popup.run_proc = [&running]()
							{
								if(!running) //cancelled
								{
									do_ap_disconnect();
									return false;
								}
								switch(AP_GetConnectionStatus())
								{
									case AP_ConnectionStatus::Disconnected:
									case AP_ConnectionStatus::Connected:
										return true;
								}
								return false;
							};
						popup.run_loop();
						if(!running)
							errtxt = "Connection cancelled";
						else switch(AP_GetConnectionStatus())
						{
							case AP_ConnectionStatus::ConnectionRefused:
								errtxt = "Connection Refused: check your connection details";
								break;
						}
						popups.pop_back();
						ref.flags &= ~FL_SELECTED;
						
						return MRET_OK;
					}
				}
				return ref.handle_ev(e);
			};
		disconnect_btn->onMouse = [](InputObject& ref,MouseEvent e)
			{
				switch(e)
				{
					case MOUSE_LCLICK:
						if(grid->active())
						{
							if(!pop_yn("Forfeit", "Quit solving current puzzle?"
								+ string(ap_deathlink()?"\nThis will count as a DeathLink death!":"")))
								return MRET_OK;
							do_ap_death("quit a sudoku puzzle!");
							grid->clear();
						}
						do_ap_disconnect();
						return MRET_TAKEFOCUS;
				}
				return ref.handle_ev(e);
			};
		cont_on->push_back(connect_btn);
		cont_off->push_back(disconnect_btn);
		
		shared_ptr<Label> grid_status = make_shared<Label>(
			"Currently mid-puzzle! Changing connection requires forfeiting!",
			font_s, ALLEGRO_ALIGN_LEFT);
		grid_status->setx(connect_btn->xpos() + connect_btn->width() + 4);
		grid_status->sety(connect_btn->ypos() + (connect_btn->height() - grid_status->height()) / 2);
		grid_status->vis_proc = [](GUIObject const& ref) -> bool {return grid->active();};
		cont_on->push_back(grid_status);
		cont_off->push_back(grid_status);
		
		connect_error = make_shared<Label>("", font_s, ALLEGRO_ALIGN_LEFT);
		connect_error->setx(connect_btn->xpos());
		connect_error->sety(connect_btn->ypos() + connect_btn->height());
		connect_error->type = TYPE_ERROR;
		cont_on->push_back(connect_error);
		
		sw->add(cont_on);
		sw->add(cont_off);
		gui_objects[SCR_CONNECT].push_back(sw);
	}
}

void dlg_draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BACKGROUND);
	
	gui_objects[curscr].draw();
	for(DrawContainer* p : popups)
		p->draw();
	
	al_restore_state(&oldstate);
}

void dlg_render()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_backbuffer(display);
	clear_a5_bmp(C_BACKGROUND);
	
	al_draw_bitmap(canvas, 0, 0, 0);
	
	al_flip_display();
	
	update_scale();
	
	al_restore_state(&oldstate);
}

void init_grid()
{
	//!TODO this is just a test grid inserted on launch
	for(int q = 0; q < 9; ++q)
	{
		Sudoku::Cell* c = grid->get(0,q);
		c->val = q+1;
		if(q%2)
			c->flags |= CFL_GIVEN;
		
		c = grid->get(1,q);
		for(int p = 0; p <= q; ++p)
			c->center_marks[p] = true;
		
		c = grid->get(2,q);
		for(int p = 0; p <= q; ++p)
			c->corner_marks[p] = true;
	}
}
void setup_allegro();
void save_cfg();
bool program_running = true;
u64 cur_frame = 0;
void run_events(bool& redraw)
{
	ALLEGRO_EVENT ev;
	al_wait_for_event(events, &ev);
	
	switch(ev.type)
	{
		case ALLEGRO_EVENT_TIMER:
			++cur_frame;
		[[fallthrough]];
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			redraw = true;
			break;
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			program_running = false;
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			al_acknowledge_resize(display);
			on_resize();
			redraw = true;
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
		case ALLEGRO_EVENT_KEY_UP:
		case ALLEGRO_EVENT_KEY_CHAR:
			if(cur_input && cur_input->focused)
				cur_input->focused->key_event(ev);
			break;
	}
	if(process_remote_deaths())
		grid->exit();
}
bool events_empty()
{
	return al_is_event_queue_empty(events);
}
int main(int argc, char **argv)
{
	log("Program Start");
	//
	string exepath(argv[0]);
	string wdir;
	auto ind = exepath.find_last_of("/\\");
	if(ind == string::npos)
		wdir = "";
	else wdir = exepath.substr(0,1+ind);
	std::filesystem::current_path(wdir);
	log("Running in dir: \"" + wdir + "\"");
	//
	setup_allegro();
	log("Allegro initialized successfully");
	log("Building GUI...");
	build_gui();
	log("GUI built successfully, launch complete");
	
	init_grid();
	
	InputState input_state;
	cur_input = &input_state;
	al_start_timer(timer);
	bool redraw = true;
	program_running = true;
	while(program_running)
	{
		if(redraw && events_empty())
		{
			gui_objects[curscr].run();
			dlg_draw();
			dlg_render();
			redraw = false;
		}
		run_events(redraw);
	}
	return 0;
}

void init_fonts()
{
	fonts[FONT_ANSWER] = FontDef(-32, false, BOLD_NONE);
	fonts[FONT_MARKING5] = FontDef(-12, true, BOLD_NONE);
	fonts[FONT_MARKING6] = FontDef(-11, true, BOLD_NONE);
	fonts[FONT_MARKING7] = FontDef(-10, true, BOLD_NONE);
	fonts[FONT_MARKING8] = FontDef(-9, true, BOLD_NONE);
	fonts[FONT_MARKING9] = FontDef(-7, true, BOLD_NONE);
}

void on_resize()
{
	int w = render_resx, h = render_resy;
	update_scale();
	if(w == render_resx && h == render_resy)
		return; // no resize
	al_destroy_bitmap(canvas);
	canvas = al_create_bitmap(render_resx, render_resy);
	if(!canvas)
		fail("Failed to recreate canvas bitmap!");
	scale_fonts();
	
	for(int scr = 0; scr < NUM_SCRS; ++scr)
		gui_objects[Screen(scr)].on_disp_resize();
	for(DrawContainer* popup : popups)
		popup->on_disp_resize();
}

void default_configs() // Resets configs to default
{
	ConfigStash stash;
	
	set_cfg(CFG_ROOT);
	set_config_dbl("GUI", "start_scale", 2.0);
	Theme::reset();
}
void refresh_configs() // Uses values in the loaded configs to change the program
{
	using namespace Sudoku;
	ConfigStash stash;
	
	bool wrote_any = false;
	bool b;
	#define DBL_BOUND(var, low, high, sec, key) \
	if(auto val = get_config_dbl(sec, key)) \
	{ \
		var = vbound(*val,low,high,b); \
		if(b) \
			set_config_dbl(sec, key, var); \
		wrote_any = wrote_any || b; \
	}
	#define INT_BOUND(var, low, high, sec, key) \
	if(auto val = get_config_int(sec, key)) \
	{ \
		var = vbound(*val,low,high,b); \
		if(b) \
			set_config_dbl(sec, key, var); \
		wrote_any = wrote_any || b; \
	}
	#define BOOL_READ(var, sec, key) \
	if(auto val = get_config_bool(sec, key)) \
	{ \
		var = *val; \
	}
	
	set_cfg(CFG_THEME);
	DBL_BOUND(RadioButton::fill_sel,0.5,1.0,"Style", "RadioButton Fill %")
	DBL_BOUND(CheckBox::fill_sel,0.5,1.0,"Style", "CheckBox Fill %")
	INT_BOUND(Grid::sel_style,0,NUM_STYLE-1,"Style","Grid: Cursor 2 Style")
	Theme::read_palette();
	set_cfg(CFG_ROOT);
	
	if(wrote_any)
		save_cfg();
	#undef DBL_BOUND
}
void setup_allegro()
{
	if(!al_init())
		fail("Failed to initialize Allegro!");
	
	if(!al_init_font_addon())
		fail("Failed to initialize Allegro Fonts!");
	
	if(!al_init_ttf_addon())
		fail("Failed to initialize Allegro TTF!");
	
	if(!al_init_primitives_addon())
		fail("Failed to initialize Allegro Primitives!");
	
	al_install_mouse();
	al_install_keyboard();
	
	configs[CFG_ROOT] = al_create_config();
	configs[CFG_THEME] = al_create_config();
	set_cfg(CFG_ROOT);
	default_configs();
	load_cfg();
	save_cfg();
	
	double start_scale = get_config_dbl("GUI", "start_scale").value_or(2.0);
	
	al_set_new_display_flags(ALLEGRO_RESIZABLE);
	display = al_create_display(CANVAS_W*start_scale, CANVAS_H*start_scale);
	if(!display)
		fail("Failed to create display!");
	
	al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE);
	canvas = al_create_bitmap(CANVAS_W, CANVAS_H);
	if(!canvas)
		fail("Failed to create canvas bitmap!");
	
	timer = al_create_timer(ALLEGRO_BPS_TO_SECS(60));
	if(!timer)
		fail("Failed to create timer!");
	
	events = al_create_event_queue();
	if(!events)
		fail("Failed to create event queue!");
	al_init_user_event_source(&event_source);
	al_register_event_source(events, &event_source);
	al_register_event_source(events, al_get_display_event_source(display));
	al_register_event_source(events, al_get_timer_event_source(timer));
	al_register_event_source(events, al_get_keyboard_event_source());
	
	al_set_window_constraints(display, CANVAS_W, CANVAS_H, 0, 0);
	al_apply_window_constraints(display, true);
	
	refresh_configs();
	on_resize();
	init_fonts();
}

