/*
    End Game Screen

    Written by Peter de Tagyos
    Started: 2/22/2017
*/

#include "config.h"
#include "game.h"
#include "list.h"
#include "screen_end_game.h"


#define BG_WIDTH	80
#define BG_HEIGHT	45

#define INFO_LEFT	2
#define INFO_TOP	10
#define INFO_WIDTH	50
#define INFO_HEIGHT	35


void render_endgame_bg_view(Console *console);
void render_info_view(Console *console);


// Init / Show screen --

UIScreen * 
screen_show_endgame() 
{
	List *subViews = list_new(NULL);

	UIRect infoRect = {(16 * INFO_LEFT), (16 * INFO_TOP), (16 * INFO_WIDTH), (16 * INFO_HEIGHT)};
	UIView *infoView = view_new(infoRect, INFO_WIDTH, INFO_HEIGHT,
								 "./assets/terminal16x16.png", 0, 0x000000ff, 
								 true, render_info_view);
	list_insert_after(subViews, NULL, infoView);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./assets/terminal16x16.png", 0, 0x000000ff,
							   true, render_endgame_bg_view);
	list_insert_after(subViews, NULL, bgView);

	UIScreen *endScreen = calloc(1, sizeof(UIScreen));
	endScreen->views = subViews;
	endScreen->activeView = infoView;
	endScreen->handle_event = handle_event_endgame;

	if (hofConfig == NULL) {
		hofConfig = config_file_parse("./config/hof.cfg");
	}

	return endScreen;
}


// Internal Render Functions -- 

void 
render_endgame_bg_view(Console *console)  
{
	// We should load and process the bg image only once, not on each render
	local_persist BitmapImage *bgImage = NULL;
	local_persist AsciiImage *aiImage = NULL;
	if (bgImage == NULL) {
		bgImage = image_load_from_file("./assets/gameover.png");
		aiImage = asciify_bitmap(console, bgImage);	
	}

	if (asciiMode) {
		view_draw_ascii_image_at(console, aiImage, 0, 0);
	} else {
		view_draw_image_at(console, bgImage, 0, 0);	
	}
}

void 
render_info_view(Console *console)  
{
	// Stats recap
	console_put_string_at(console, "-== HERO STATS ==-", 17, 0, 0xffd700ff, 0x00000000);

	console_put_string_at(console, playerName, 18, 2, 0xffffffff, 0x00000000);

	char *level = String_Create("Level:%d", currentLevelNumber);
	console_put_string_at(console, level, 18, 4, 0xffd700ff, 0x00000000);
	String_Destroy(level);

	char *gems = String_Create("Gems:%d", gemsFoundTotal);
	console_put_string_at(console, gems, 28, 4, 0x753aabff, 0x00000000);
	String_Destroy(gems);

	// Leaderboard
	console_put_string_at(console, "-== HERO HALL OF FAME ==-", 14, 7, 0xaa0000ff, 0x00000000);
	// Loop through all HoF entries, extract the data into a formatted string, and write to screen
	i32 y = 9;
	ListElement *e = list_head(hofConfig->entities);
	while (e != NULL) {
		ConfigEntity *entity = (ConfigEntity *)e->data;

		char *name = config_entity_value(entity, "name");
		char *level = config_entity_value(entity, "level");
		char *gems = config_entity_value(entity, "gems");
		char *date = config_entity_value(entity, "date");

		char *nameString = String_Create("%20s", name);
		console_put_string_at(console, nameString, 3, y, 0xe2f442ff, 0x00000000);
		char *recordString = String_Create("%10s Level:%s Gems:%s", date, level, gems);
		console_put_string_at(console, recordString, 23, y, 0xeeeeeeff, 0x00000000);

		y += 2;
		e = list_next(e);
	}

	// Instructions for active commands
	console_put_string_at(console, "Start a (N)ew game", 18, 30, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "-or-", 25, 31, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "(ESC) to Quit", 20, 32, 0xbca285FF, 0x00000000);
}


// Event Handling --

void
handle_event_endgame(UIScreen *activeScreen, SDL_Event event) 
{

	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.sym;

		switch (key) {
			case SDLK_n: {
				// Start a new game and transition to in-game screen
				game_new();
				ui_set_active_screen(screen_show_in_game());
				currentlyInGame = true;
			}
			break;

			case SDLK_ESCAPE: {
				game_quit();
			}
			break;

			default:
				break;
		}
	}

}
