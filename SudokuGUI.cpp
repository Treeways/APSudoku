#include "Archipelago.h"
#include <iostream>
#include "SudokuGUI.hpp"
#include "SudokuGrid.hpp"

void log(string const& msg)
{
	std::cout << msg << std::endl;
}
void fail(string const& msg)
{
	log(msg);
	exit(1);
}

#define CANVAS_W 640
#define CANVAS_H 352
ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* canvas;
ALLEGRO_TIMER* timer;
ALLEGRO_EVENT_QUEUE* events;
ALLEGRO_EVENT_SOURCE event_source;
ALLEGRO_FONT* fonts[NUM_FONTS];
#define GRID_X (32)
#define GRID_Y (32)
Sudoku::Grid grid = { GRID_X, GRID_Y };
#define BUTTON_X (CANVAS_W-96)
#define BUTTON_Y (32)
Button swap_btns[NUM_SCRS] =
{
	{ "Sudoku", BUTTON_X, BUTTON_Y },
	{ "Archipelago", BUTTON_X, BUTTON_Y+32 }
};
Screen curscr = SCR_SUDOKU;

void clear_a5_bmp(ALLEGRO_COLOR col, ALLEGRO_BITMAP* bmp = nullptr)
{
	if(bmp)
	{
		ALLEGRO_STATE old_state;
		al_store_state(&old_state, ALLEGRO_STATE_TARGET_BITMAP);
		
		al_set_target_bitmap(bmp);
		al_clear_to_color(col);
		
		al_restore_state(&old_state);
	}
	else al_clear_to_color(col);
}

void draw()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_bitmap(canvas);
	clear_a5_bmp(C_BG);
	for(Button& b : swap_btns)
		b.draw();
	switch(curscr)
	{
		case SCR_SUDOKU:
			grid.draw();
			break;
		case SCR_CONNECT:
			break;
	}
	
	al_restore_state(&oldstate);
}
void render()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_backbuffer(display);
	clear_a5_bmp(C_BG);
	
	int resx = al_get_display_width(display);
	int resy = al_get_display_height(display);
	
	double xscale = resx/CANVAS_W, yscale = resy/CANVAS_H;
	double scale = std::min(xscale,yscale);
	int scaled_w = int(CANVAS_W*scale), scaled_h = int(CANVAS_H*scale);
	int xoff = (resx-scaled_w)/2, yoff = (resy-scaled_h)/2;
	
	al_draw_scaled_bitmap(canvas, 0, 0, CANVAS_W, CANVAS_H, xoff, yoff, scaled_w, scaled_h, 0);
	
	al_flip_display();
	
	al_restore_state(&oldstate);
}

void setup_allegro();
int main(int argc, char **argv)
{
	//!TODO TESTING, REMOVE
	swap_btns[1].flags |= FL_SELECTED;
	//
	log("Program Start");
	setup_allegro();
	
	al_start_timer(timer);
	bool redraw = true;
	bool running = true;
	while(running)
	{
		if(redraw && al_is_event_queue_empty(events))
		{
			draw();
			render();
			redraw = false;
		}
		
		ALLEGRO_EVENT ev;
		al_wait_for_event(events, &ev);
		
		switch(ev.type)
		{
			case ALLEGRO_EVENT_TIMER:
				redraw = true;
				break;
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				running = false;
				break;
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
				al_acknowledge_resize(display);
				break;
		}
	}
	
	return 0;
}

void Button::draw() const
{
	ALLEGRO_COLOR const* c1 = &c_txt;
	ALLEGRO_COLOR const* c2 = &c_bg;
	bool dis = flags&FL_DISABLED;
	bool sel = !dis && (flags&FL_SELECTED);
	if(sel)
		std::swap(c1,c2);
	
	// Fill the button
	al_draw_filled_rectangle(x, y, x+w-1, y+h-1, *c2);
	// Draw the border
	al_draw_rectangle(x, y, x+w-1, y+w-1, c_border, 1);
	// Finally, the text
	
}
Button::Button(string const& txt, u16 X, u16 Y)
	: text(txt), x(X), y(Y)
{}



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
	
	al_set_new_display_flags(ALLEGRO_RESIZABLE);
	display = al_create_display(CANVAS_W, CANVAS_H);
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
	
	al_set_window_constraints(display, CANVAS_W, CANVAS_H, 0, 0);
	al_apply_window_constraints(display, true);
	
	// fonts[FONT_BUTTON] = al_load_font(, -20, 0);
	// fonts[FONT_ANSWER] = al_load_font(, -20, 0);
	// fonts[FONT_MARKING] = al_load_font(, -6, 0);
}

