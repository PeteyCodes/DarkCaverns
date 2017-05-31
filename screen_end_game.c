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
	// Stats recap
	console_put_string_at(console, "-== HERO STATS ==-", 8, 0, 0xffd700ff, 0x00000000);

	console_put_string_at(console, playerName, 9, 2, 0xffffffff, 0x00000000);

	char *level = String_Create("Level:%d", currentLevelNumber);
	console_put_string_at(console, level, 9, 4, 0xffd700ff, 0x00000000);
	String_Destroy(level);

	char *gems = String_Create("Gems:%d", gemsFoundTotal);
	console_put_string_at(console, gems, 19, 4, 0x753aabff, 0x00000000);
	String_Destroy(gems);

	// Leaderboard
	console_put_string_at(console, "-== HERO HALL OF FAME ==-", 5, 7, 0xaa0000ff, 0x00000000);
	console_put_string_at(console, "Coming Soon!", 11, 9, 0x555555ff, 0x00000000);
	// TODO: leaderboard

	// Instructions for active commands
	console_put_string_at(console, "Start a (N)ew game", 8, 27, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "-or-", 15, 28, 0xbca285FF, 0x00000000);
	console_put_string_at(console, "(ESC) to Quit", 10, 29, 0xbca285FF, 0x00000000);
}


// Event Handling --

internal void
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
				quit_game();
			}
			break;

			default:
				break;
		}
	}

}
