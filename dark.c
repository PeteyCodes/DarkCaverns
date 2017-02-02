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


#define internal static
#define local_persist static
#define global_variable static

#include "util.c"
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

int main(int argc, char *argv[]) 
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

	// Initialize UI state (screens, view stack, etc)
	// TODO: Move this to ui.c
    UIScreen *activeScreen = NULL;


	// Show the in-game screen (for now)
	// TODO: Show the launch screen instead
	activeScreen = screen_show_in_game();

	// TODO: Move this to somewhere more relevant
	game_new();
	currentlyInGame = true;

	bool done = false;

	while (!done) {
		playerTookTurn = false;

		SDL_Event event;
		u32 timePerFrame = 1000 / FPS_LIMIT;
		u32 frameStart = 0;
		while (SDL_PollEvent(&event) != 0) {

			frameStart = SDL_GetTicks();

			if (event.type == SDL_QUIT) {
				done = true; 
				break;
			}

			// Handle "global" keypresses (those not handled on a screen-by-screen basis)
			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				switch (key) {
					case SDLK_ESCAPE:
						done = true;
						break;

					default:
						break;
				}
			
				// Send the event to the currently active screen for handling
				activeScreen->handle_event(activeScreen, event);
			}
		}

		// If we're in-game, have the game update itself
		if (currentlyInGame) {
			game_update();		
		}

		// Render the active screen
		render_screen(renderer, screenTexture, activeScreen);

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
