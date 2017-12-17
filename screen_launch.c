/*
    Launch Screen

    Written by Peter de Tagyos
    Started: 2/6/2017
*/

#define BG_WIDTH	80
#define BG_HEIGHT	45

#define MENU_LEFT	50
#define MENU_TOP	28
#define MENU_WIDTH	24
#define MENU_HEIGHT	10



internal void render_bg_view(Console *console);
internal void render_menu_view(Console *console);
internal void handle_event_launch(UIScreen *activeScreen, SDL_Event event);

// Forward declares for other screens
internal UIScreen * screen_show_hof();


// Init / Show screen --

internal UIScreen * 
screen_show_launch() 
{
	List *launchViews = list_new(NULL);

	UIRect menuRect = {(16 * MENU_LEFT), (16 * MENU_TOP), (16 * MENU_WIDTH), (16 * MENU_HEIGHT)};
	UIView *menuView = view_new(menuRect, MENU_WIDTH, MENU_HEIGHT,
								 "./terminal16x16.png", 0, 0x000000ff,
								 render_menu_view);
	list_insert_after(launchViews, NULL, menuView);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./terminal16x16.png", 0, 0x000000ff,
							   render_bg_view);
	list_insert_after(launchViews, NULL, bgView);

	UIScreen *launchScreen = calloc(1, sizeof(UIScreen));
	launchScreen->views = launchViews;
	launchScreen->activeView = menuView;
	launchScreen->handle_event = handle_event_launch;

	return launchScreen;
}


// Render Functions -- 

internal void 
render_bg_view(Console *console)  
{
	// We should load and process the bg image only once, not on each render
	local_persist BitmapImage *bgImage = NULL;
	local_persist AsciiImage *aiImage = NULL;
	if (bgImage == NULL) {
		bgImage = image_load_from_file("./launch.png");
		aiImage = asciify_bitmap(console, bgImage);	
	}

	if (asciiMode) {
		view_draw_ascii_image_at(console, aiImage, 0, 0);
	} else {
		view_draw_image_at(console, bgImage, 0, 0);	
	}

	console_put_string_at(console, "Dark Caverns", 52, 18, 0x556d76FF, 0x00000000);
}

internal void 
render_menu_view(Console *console) 
{
	UIRect rect = {0, 0, MENU_WIDTH, MENU_HEIGHT};
	view_draw_rect(console, &rect, 0x363247FF, 0, 0xFFFFFFFF);

	console_put_string_at(console, "Start a (N)ew game", 2, 3, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "View (H)all of Fame", 2, 6, 0xbca285FF, 0x00000000);
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
				quit_game();
			}
			break;

			default:
				break;
		}
	}

}

