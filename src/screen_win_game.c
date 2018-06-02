/*
    Win Screen

    Written by: Peter de Tagyos
    Started: 12/3/2017
*/

#include "game.h"
#include "list.h"
#include "screen_win_game.h"


#define BG_WIDTH	80
#define BG_HEIGHT	45

#define WIN_INFO_LEFT 30
#define WIN_INFO_TOP 12
#define WIN_INFO_WIDTH 50
#define WIN_INFO_HEIGHT 20


// Internal render functions
void render_win_bg_view(Console *console);
void render_win_info_view(Console *console);  


// Init / Show screen --

UIScreen * 
screen_show_win_game() 
{
	List *views = list_new(NULL);

	UIRect infoRect = {(16 * WIN_INFO_LEFT), (16 * WIN_INFO_TOP), (16 * WIN_INFO_WIDTH), (16 * WIN_INFO_HEIGHT)};
	UIView *infoView = view_new(infoRect, WIN_INFO_WIDTH, WIN_INFO_HEIGHT,
								 "./assets/terminal16x16.png", 0, 0x00000000, 
                                 true, render_win_info_view);
	list_insert_after(views, NULL, infoView);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./assets/terminal16x16.png", 0, 0x000000ff,
                               true, render_win_bg_view);
	list_insert_after(views, NULL, bgView);

	UIScreen *winScreen = calloc(1, sizeof(UIScreen));
	winScreen->views = views;
	winScreen->activeView = bgView;
	winScreen->handle_event = handle_event_win;

	return winScreen;
}


// Event Handling --

void
handle_event_win(UIScreen *activeScreen, SDL_Event event) 
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

            case SDLK_h: {
				// Show hall of fame / credits screen
                ui_set_active_screen(screen_show_hof());
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


// Render Functions -- 

void 
render_win_bg_view(Console *console)  
{
	// We should load and process the bg image only once, not on each render
	local_persist BitmapImage *bgImage = NULL;
	local_persist AsciiImage *aiImage = NULL;
	if (bgImage == NULL) {
		bgImage = image_load_from_file("./assets/you_won.png");
		aiImage = asciify_bitmap(console, bgImage);	
	}

	if (asciiMode) {
		view_draw_ascii_image_at(console, aiImage, 0, 0);
	} else {
		view_draw_image_at(console, bgImage, 0, 0);	
	}

    console_put_string_at(console, "Your hero is teleported back to the surface safely!", 3, 10, 0x0000bbff, 0x00000000);

}

void 
render_win_info_view(Console *console)  
{
	// Stats recap
	console_put_string_at(console, "-== HERO STATS ==-", 17, 0, 0xffd700ff, 0x00000000);

	console_put_string_at(console, playerName, 18, 2, 0xffffffff, 0x00000000);

	char *level = String_Create("Level:%d", 20);
	console_put_string_at(console, level, 18, 4, 0xffd700ff, 0x00000000);
	String_Destroy(level);

	char *gems = String_Create("Gems:%d", gemsFoundTotal);
	console_put_string_at(console, gems, 28, 4, 0x753aabff, 0x00000000);
	String_Destroy(gems);

	// Instructions for active commands
	console_put_string_at(console, "View the (H)all of Fame", 16, 9, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "-or-", 25, 11, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "Start a (N)ew game", 18, 13, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "-or-", 25, 15, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "(ESC) to Quit", 20, 17, 0xbca285FF, 0x00000000);
}



