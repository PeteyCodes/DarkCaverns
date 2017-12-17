/*
* dark.c - Dark Caverns Main Game Loop
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <SDL2/SDL.h>

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720

#define NUM_COLS		80
#define NUM_ROWS 		45

#define FPS_LIMIT		20


#include <stdbool.h>
#include <stdint.h>

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef int8_t		i8;
typedef int32_t		i32;
typedef int64_t		i64;


void quit_game();


#define internal static
#define local_persist static
#define global_variable static

#include "util.c"
#include "String.c"
#include "list.c"
#include "config.c"
// #define HASHMAP_IMPLEMENTATION
// #include "hashmap.h"
#include "ui.c"
#include "map.c"
#include "game.c"
#include "fov.c"

// Screen files
#include "screen_in_game.c"
#include "screen_launch.c"
#include "screen_hof.c"
#include "screen_end_game.c"
#include "screen_win_game.c"


internal void 
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

global_variable bool gameIsRunning = true;
void quit_game() {
	gameIsRunning = false;
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
				quit_game(); 
				break;
			}

			// Handle "global" keypresses (those not handled on a screen-by-screen basis)
			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				switch (key) {
					case SDLK_t: {
						asciiMode = !asciiMode;
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
