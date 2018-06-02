/*
* dark.c - Dark Caverns Main Game Loop
*/

#include "dark.h"
#include "game.h"
#include "list.h"
#include "screen_launch.h"
#include "screen_in_game.h"
#include "screen_win_game.h"
#include "typedefs.h"
#include "ui.h"


#define FPS_LIMIT		20


void 
render_screen(SDL_Renderer *renderer, 
				  SDL_Texture *screenTexture, 
				  UIScreen *screen) 
{

	// Render views from back to front for the current screen
	ListElement *e = list_head(screen->views);
	while (e != NULL) {
		UIView *v = (UIView *)list_data(e);
		console_clear(v->console);
		v->render(v->console);
		SDL_UpdateTexture(screenTexture, v->pixelRect, v->console->pixels, v->pixelRect->w * sizeof(u32));
		e = list_next(e);
	}

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent(renderer);
}


int main() 
{
	srand((unsigned)time(NULL));

	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window *window = SDL_CreateWindow("Dark Caverns",
		SDL_WINDOWPOS_UNDEFINED, 
		SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		0);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	SDL_Texture *screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize UI state to show launch screen
	ui_set_active_screen(screen_show_launch());

	currentlyInGame = false;

	while (gameIsRunning) {
		playerTookTurn = false;

		SDL_Event event;
		u32 timePerFrame = 1000 / FPS_LIMIT;
		u32 frameStart = 0;
		while (SDL_PollEvent(&event) != 0) {

			frameStart = SDL_GetTicks();

			if (event.type == SDL_QUIT) {
				game_quit(); 
				break;
			}

			// Handle "global" keypresses (those not handled on a screen-by-screen basis)
			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				switch (key) {
					case SDLK_t: {
						asciiMode = !asciiMode;
						if (currentlyInGame) {
							ui_set_active_screen(screen_show_in_game());
						}
					}
					break;

					// // DEBUG - jump straight to win screen
					case SDLK_w: {
						ui_set_active_screen(screen_show_win_game());
					}
					break;

					default:
						break;
				}
			
				// Send the event to the currently active screen for handling
				UIScreen *screenForInput = ui_get_active_screen(); 
				screenForInput->handle_event(screenForInput, event);
			}
		}

		// If we're in-game, have the game update itself
		if (currentlyInGame) {
			game_update();		
		}

		// Render the active screen
		render_screen(renderer, screenTexture, ui_get_active_screen());

		// Limit our FPS
		i32 sleepTime = timePerFrame - (SDL_GetTicks() - frameStart);
		if (sleepTime > 0) {
			SDL_Delay(sleepTime);
		}
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}
