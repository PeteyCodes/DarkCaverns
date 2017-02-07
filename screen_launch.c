/*
    Launch Screen

    Written by Peter de Tagyos
    Started: 2/6/2017
*/

#define BG_WIDTH	80
#define BG_HEIGHT	45

#define MENU_LEFT	30
#define MENU_TOP	20
#define MENU_WIDTH	24
#define MENU_HEIGHT	13



internal void render_bg_view(Console *console);
internal void render_menu_view(Console *console);
internal void handle_event_launch(UIScreen *activeScreen, SDL_Event event);


// Init / Show screen --

internal UIScreen * 
screen_show_launch() 
{
	List *launchViews = list_new(NULL);

	UIRect menuRect = {(16 * MENU_LEFT), (16 * MENU_TOP), (16 * MENU_WIDTH), (16 * MENU_HEIGHT)};
	UIView *menuView = view_new(menuRect, MENU_WIDTH, MENU_HEIGHT,
								 "./terminal16x16.png", 0, render_menu_view);
	list_insert_after(launchViews, NULL, menuView);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./terminal16x16.png", 0, render_bg_view);
	list_insert_after(launchViews, NULL, bgView);

	UIScreen *launchScreen = malloc(sizeof(UIScreen));
	launchScreen->views = launchViews;
	launchScreen->activeView = menuView;
	launchScreen->handle_event = handle_event_launch;

	return launchScreen;
}


// Render Functions -- 

internal void 
render_bg_view(Console *console) 
{
	UIRect rect = {0, 0, BG_WIDTH, BG_HEIGHT};
	view_draw_rect(console, &rect, 0x222222FF, 1, 0xFF990099);

	console_put_string_at(console, "Dark Caverns", 10, 10, 0xe600e6FF, 0x00000000);
	console_put_string_at(console, "TODO: Add a cool ASCII background here!", 10, 16, 0xe6e600FF, 0x00000000);
}

internal void 
render_menu_view(Console *console) 
{
	UIRect rect = {0, 0, MENU_WIDTH, MENU_HEIGHT};
	view_draw_rect(console, &rect, 0x330033FF, 0, 0xFF990099);

	console_put_string_at(console, "Start a (n)ew game", 2, 3, 0xe6e600FF, 0x00000000);
	console_put_string_at(console, "(L)oad a saved game", 2, 6, 0xe6e600FF, 0x00000000);
	console_put_string_at(console, "Show (C)redits", 2, 9, 0xe6e600FF, 0x00000000);
}


// Event Handling --

internal void
handle_event_launch(UIScreen *activeScreen, SDL_Event event) 
{

	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.sym;

		switch (key) {
			case SDLK_n: {
				// Start a new game and transition to in-game screen
				game_new();
				activeScreen = screen_show_in_game();
				currentlyInGame = true;
			}
			break;

			case SDLK_DOWN: {
			}
			break;

			default:
				break;
		}
	}

}

