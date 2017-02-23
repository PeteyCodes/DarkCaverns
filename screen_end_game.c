/*
    End Game Screen

    Written by Peter de Tagyos
    Started: 2/22/2017
*/

#define ENDGAME_BG_WIDTH	80
#define ENDGAME_BG_HEIGHT	45

#define INFO_LEFT	10
#define INFO_TOP	10
#define INFO_WIDTH	30
#define INFO_HEIGHT	30


internal void render_endgame_bg_view(Console *console);
internal void render_info_view(Console *console);
internal void handle_event_endgame(UIScreen *activeScreen, SDL_Event event);


// Init / Show screen --

internal UIScreen * 
screen_show_endgame() 
{
	List *subViews = list_new(NULL);

	UIRect infoRect = {(16 * INFO_LEFT), (16 * INFO_TOP), (16 * INFO_WIDTH), (16 * INFO_HEIGHT)};
	UIView *infoView = view_new(infoRect, INFO_WIDTH, INFO_HEIGHT,
								 "./terminal16x16.png", 0, render_info_view);
	list_insert_after(subViews, NULL, infoView);

	UIRect bgRect = {0, 0, (16 * BG_WIDTH), (16 * BG_HEIGHT)};
	UIView *bgView = view_new(bgRect, BG_WIDTH, BG_HEIGHT, 
							   "./terminal16x16.png", 0, render_endgame_bg_view);
	list_insert_after(subViews, NULL, bgView);

	UIScreen *endScreen = malloc(sizeof(UIScreen));
	endScreen->views = subViews;
	endScreen->activeView = infoView;
	endScreen->handle_event = handle_event_endgame;

	return endScreen;
}


// Render Functions -- 

internal void 
render_endgame_bg_view(Console *console)  
{
	// We should load and process the bg image only once, not on each render
	local_persist BitmapImage *bgImage = NULL;
	local_persist AsciiImage *aiImage = NULL;
	if (bgImage == NULL) {
		bgImage = image_load_from_file("./gameover.png");
		aiImage = asciify_bitmap(console, bgImage);	
	}

	if (asciiMode) {
		view_draw_ascii_image_at(console, aiImage, 0, 0);
	} else {
		view_draw_image_at(console, bgImage, 0, 0);	
	}
}

internal void 
render_info_view(Console *console)  
{
	// TODO
}


// Event Handling --

internal void
handle_event_endgame(UIScreen *activeScreen, SDL_Event event) 
{

	if (event.type == SDL_KEYDOWN) {
		SDL_Keycode key = event.key.keysym.sym;

		switch (key) {
			case SDLK_n: {
				// TODO
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