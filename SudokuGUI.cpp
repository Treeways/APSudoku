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

ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* canvas;
ALLEGRO_TIMER *timer;
ALLEGRO_EVENT_QUEUE *events;
ALLEGRO_EVENT_SOURCE event_source;
Sudoku::Grid grid;
#define CANVAS_W 640
#define CANVAS_H 480

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
	grid.draw(32,32);
	
	al_restore_state(&oldstate);
}
void render()
{
	ALLEGRO_STATE oldstate;
	al_store_state(&oldstate, ALLEGRO_STATE_TARGET_BITMAP);
	
	al_set_target_backbuffer(display);
	
	int resx = al_get_display_width(display);
	int resy = al_get_display_height(display);
	
	al_draw_scaled_bitmap(canvas, 0, 0, CANVAS_W, CANVAS_H, 0, 0, resx, resy, 0);
	
	al_flip_display();
	
	al_restore_state(&oldstate);
}

int main(int argc, char **argv)
{
	log("Program Start");
	if(!al_init())
		fail("Failed to initialize Allegro!");
	
	if(!al_init_font_addon())
		fail("Failed to initialize Allegro Fonts!");
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
	
	//
	
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
		}
	}
	
	return 0;
}

