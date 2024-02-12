#include "../Archipelago.h"
#include <iostream>
#include <filesystem>
#include "GUI.hpp"
#include "Font.hpp"
#include "Config.hpp"
#include "SudokuGrid.hpp"

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
shared_ptr<RadioSet> difficulty;
shared_ptr<RadioSet> entry_mode;


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
	FontDef font_l(-22, false, BOLD_NONE);
	FontDef font_s(-15,false,BOLD_NONE);
	{ // BG, to allow 'clicking off'
		shared_ptr<ShapeRect> bg = make_shared<ShapeRect>(0,0,CANVAS_W,CANVAS_H,C_BG);
		bg->onMouse = mouse_killfocus;
		for(int q = 0; q < NUM_SCRS; ++q)
			gui_objects[static_cast<Screen>(q)].push_back(bg);
	}
	{ // Swap buttons
		#define ON_SWAP_BTN(b, scr) \
		b->sel_proc = [](){return curscr == scr;}; \
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
	}
	const int GRID_X2 = GRID_X+(CELL_SZ*9)+2;
	const int GRID_Y2 = GRID_Y+(CELL_SZ*9)+2;
	const int RGRID_X = GRID_X2 + 8;
	{ // Difficulty / game buttons
		shared_ptr<Column> diff_column = make_shared<Column>(RGRID_X,GRID_Y,0,2,ALLEGRO_ALIGN_LEFT);
		
		shared_ptr<Label> diff_lbl = make_shared<Label>("Difficulty:", font_s, ALLEGRO_ALIGN_LEFT);
		diff_column->add(diff_lbl);
		
		difficulty = make_shared<RadioSet>(
			[](){return diff;},
			[](optional<u8> v)
			{
				if(v)
					diff = Difficulty(*v);
			},
			vector<string>({"Easy","Normal","Hard"}),
			FontDef(-20, false, BOLD_NONE));
		difficulty->select(1);
		difficulty->dis_proc = []()
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
						grid->generate(diff);
						break;
				}
				return ref.handle_ev(e);
			};
		start_btn->dis_proc = []()
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
						if(pop_yn("Forfeit", "Quit solving this puzzle?"))
							grid->clear();
						break;
				}
				return ref.handle_ev(e);
			};
		forfeit_btn->dis_proc = []()
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
						else pop_inf("Wrong", "Puzzle solution incorrect!");
						break;
				}
				return ref.handle_ev(e);
			};
		check_btn->dis_proc = []()
			{
				return !grid->active();
			};
		diff_column->ypos(GRID_Y2-diff_column->height());
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
			[](optional<u8> v)
			{
				if(v)
					mode = EntryMode(*v);
			},
			vector<string>({"Answer","Center","Corner"}),
			FontDef(-20, false, BOLD_NONE));
		entry_mode->select(ENT_ANSWER);
		entry_mode->dis_proc = [](){return grid->focused() && mode_mod();};
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
		
		vector<tuple<string,string,
			std::function<bool(string const&,string const&,char)>>> p = {
			{"IP:","archipelago.gg", [](string const& o, string const& n, char c)
				{
					return validate_alphanum(o,n,c) || c == '.';
				}},
			{"Port:","", [](string const& o, string const& n, char c)
				{
					return validate_numeric(o,n,c) && n.size() <= 5;
				}},
			{"Slot:","",nullptr},
			{"Passwd:","",nullptr}
		};
		for(auto [label,defval,valproc] : p)
		{
			shared_ptr<Row> r = make_shared<Row>(0,0,0,4,ALLEGRO_ALIGN_CENTER);
			
			shared_ptr<Label> lbl = make_shared<Label>(label, font_l, ALLEGRO_ALIGN_LEFT);
			shared_ptr<TextField> field = make_shared<TextField>(
				GRID_X, GRID_Y, 256,
				defval, FontDef(-22, false, BOLD_NONE));
			field->onValidate = valproc;
			r->add(lbl);
			r->add(field);
			apc->add(r);
		}
		gui_objects[SCR_CONNECT].push_back(apc);
	}
}

void dlg_draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	
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
	clear_a5_bmp(C_BG);
	
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

void save_cfg()
{
	if(!al_save_config_file("APSudoku.cfg", configs[CFG_ROOT]))
		error("Failed to save config file! Settings may not be savable!");
	if(!al_save_config_file("Theme.cfg", configs[CFG_THEME]))
		error("Failed to save theme file! Theme may not be savable!");
}
void default_theme()
{
	set_cfg(CFG_THEME);
	al_add_config_section(config, "Style");
	al_add_config_section(config, "Color");
	al_add_config_comment(config, "Color", "Colors given as 0xRRGGBBAA");
	set_config_dbl("Style", "fill_radiobubbles", 0.6);
	set_config_dbl("Style", "fill_cboxes", 0.6);
	set_config_col("Color", "Background", C_LGRAY);
	set_cfg(CFG_ROOT);
}
void default_configs()
{
	set_cfg(CFG_ROOT);
	set_config_dbl("GUI", "start_scale", 2.0);
	default_theme();
}
void refresh_configs()
{
	set_cfg(CFG_THEME);
	if(auto d = get_config_dbl("Style", "fill_radiobubbles"))
		RadioButton::fill_sel = vbound(*d,0.0,1.0);
	if(auto d = get_config_dbl("Style", "fill_cboxes"))
		CheckBox::fill_sel = vbound(*d,0.0,1.0);
	if(auto c = get_config_col("Color", "Background"))
		C_BG = *c;
	set_cfg(CFG_ROOT);
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
	default_configs();
	if(ALLEGRO_CONFIG* cfg = al_load_config_file("APSudoku.cfg"))
	{
		al_merge_config_into(configs[CFG_ROOT], cfg);
		al_destroy_config(cfg);
	}
	if(ALLEGRO_CONFIG* cfg = al_load_config_file("Theme.cfg"))
	{
		al_merge_config_into(configs[CFG_THEME], cfg);
		al_destroy_config(cfg);
	}
	save_cfg();
	set_cfg(CFG_ROOT);
	
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

