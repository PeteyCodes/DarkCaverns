/*
    Hall of Fame/Credits Screen

    Written by Peter de Tagyos
    Started: 5/17/2017
*/

#define BG_WIDTH	80
#define BG_HEIGHT	45


internal void render_hof_bg_view(Console *console);
internal void render_hof_view(Console *console);
internal void handle_event_hof(UIScreen *activeScreen, SDL_Event event);


// Init / Show screen --

internal UIScreen * 
screen_show_hof() 
{
	List *views = list_new(NULL);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./terminal16x16.png", 0, render_hof_bg_view);
	list_insert_after(views, NULL, bgView);

	UIScreen *hofScreen = malloc(sizeof(UIScreen));
	hofScreen->views = views;
	hofScreen->activeView = bgView;
	hofScreen->handle_event = handle_event_hof;

	return hofScreen;
}


// Render Functions -- 

internal void 
render_hof_bg_view(Console *console)  
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

	UIRect rect = {15, 5, 50, 34};
	view_draw_rect(console, &rect, 0x363247dd, 2, 0xaad700ff);

	console_put_string_at(console, "-== HALL OF FAME ==-", 30, 7, 0xaad700ff, 0x00000000);

    console_put_string_at(console, "-== CREDITS ==-", 33, 32, 0xaad700ff, 0x00000000);

}


// Event Handling --

internal void
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

