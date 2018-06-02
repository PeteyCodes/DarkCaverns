/*
    Hall of Fame/Credits Screen

    Written by Peter de Tagyos
    Started: 5/17/2017
*/

#define BG_WIDTH	80
#define BG_HEIGHT	45

#include "config.h"
#include "game.h"
#include "list.h"
#include "screen_hof.h"
#include "screen_launch.h"
#include "String.h"


// Internal Render Functions 
void render_hof_bg_view(Console *console);
void render_hof_view(Console *console);


// Init / Show screen --

UIScreen * 
screen_show_hof() 
{
	List *views = list_new(NULL);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./assets/terminal16x16.png", 0, 0x000000ff, 
							   true, render_hof_bg_view);
	list_insert_after(views, NULL, bgView);

	UIScreen *hofScreen = calloc(1, sizeof(UIScreen));
	hofScreen->views = views;
	hofScreen->activeView = bgView;
	hofScreen->handle_event = handle_event_hof;

	if (hofConfig == NULL) {
		hofConfig = config_file_parse("./config/hof.cfg");
	}

	return hofScreen;
}


// Event Handling --

void
handle_event_hof(UIScreen *activeScreen, SDL_Event event) 
{

	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.sym;

		switch (key) {
			case SDLK_ESCAPE: {
				ui_set_active_screen(screen_show_launch());
			}
			break;

			default:
				break;
		}
	}

}



// Internal Render Functions -- 

void
render_hof_entries(Console *console) {

	// Loop through all HoF entries, extract the data into a formatted string, and write to screen
	i32 y = 10;
	ListElement *e = list_head(hofConfig->entities);
	while (e != NULL) {
		ConfigEntity *entity = (ConfigEntity *)e->data;

		char *name = config_entity_value(entity, "name");
		char *level = config_entity_value(entity, "level");
		char *gems = config_entity_value(entity, "gems");
		char *date = config_entity_value(entity, "date");

		char *nameString = String_Create("%20s", name);
		console_put_string_at(console, nameString, 16, y, 0xe2f442ff, 0x00000000);
		char *dateString = String_Create("%10s", date);
		console_put_string_at(console, dateString, 36, y, 0xeeeeeeff, 0x00000000);
		char *levelString = String_Create("Level:%2s", level);
		console_put_string_at(console, levelString, 47, y, 0xffd700ff, 0x00000000);
		char *gemString = String_Create("Gems:%s", gems);
		console_put_string_at(console, gemString, 56, y, 0xdb99fcff, 0x00000000);

		y += 2;
		e = list_next(e);
	}
}

void 
render_hof_bg_view(Console *console)  
{
	// We should load and process the bg image only once, not on each render
	local_persist BitmapImage *bgImage = NULL;
	local_persist AsciiImage *aiImage = NULL;
	if (bgImage == NULL) {
		bgImage = image_load_from_file("./assets/launch.png");
		aiImage = asciify_bitmap(console, bgImage);	
	}

	if (asciiMode) {
		view_draw_ascii_image_at(console, aiImage, 0, 0);
	} else {
		view_draw_image_at(console, bgImage, 0, 0);	
	}

	UIRect rect = {10, 5, 60, 34};
	view_draw_rect(console, &rect, 0x363247dd, 2, 0xaad700ff);

	console_put_string_at(console, "-== HALL OF FAME ==-", 28, 7, 0xaad700ff, 0x00000000);

	render_hof_entries(console);

    console_put_string_at(console, "-== CREDITS ==-", 31, 32, 0xaad700ff, 0x00000000);
	console_put_string_at(console, "Coded live at twitch.tv/peteycodes by PeteyCodes", 15, 34, 0xffffffff, 0x00000000);

}


