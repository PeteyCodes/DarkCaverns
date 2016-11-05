/*
* dark.c - Dark Caverns Main Game Loop
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

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


#include "list.c"
#define HASHMAP_IMPLEMENTATION
#include "hashmap.h"
#include "pt_console.c"
#include "map.c"
#include "game.c"
#include "fov.c"


void render_screen(SDL_Renderer *renderer, 
				  SDL_Texture *screen, 
				  PT_Console *console) {

	PT_ConsoleClear(console);

	// Setup layer render history  
	u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			layerRendered[x][y] = LAYER_UNSET;
		}
	}

	for (u32 i = 0; i < MAX_GO; i++) {
		if (visibilityComps[i].objectId != UNUSED) {
			Position *p = (Position *)game_object_get_component(&gameObjects[i], COMP_POSITION);
			if (fovMap[p->x][p->y] > 0) {
				visibilityComps[i].hasBeenSeen = true;
				// Don't render if we've already written something to to this cell at a higher layer
				if (p->layer > layerRendered[p->x][p->y]) {
					// i32 val = targetMap[p->x][p->y] % 10;
					// asciiChar ch = 48 + val;
					// PT_ConsolePutCharAt(console, ch, p->x, p->y, 
					// 					visibilityComps[i].fgColor, visibilityComps[i].bgColor);

					PT_ConsolePutCharAt(console, visibilityComps[i].glyph, p->x, p->y, 
										visibilityComps[i].fgColor, visibilityComps[i].bgColor);
					layerRendered[p->x][p->y] = p->layer;
				}

			} else if (visibilityComps[i].visibleOutsideFOV && visibilityComps[i].hasBeenSeen) {
				u32 fullColor = visibilityComps[i].fgColor;
				u32 fadedColor = COLOR_FROM_RGBA(RED(fullColor), GREEN(fullColor), BLUE(fullColor), 0x77);
				// Don't render if we've already written something to to this cell at a higher layer
				if (p->layer > layerRendered[p->x][p->y]) {
					PT_ConsolePutCharAt(console, visibilityComps[i].glyph, p->x, p->y, 
										fadedColor, 0x000000FF);
					layerRendered[p->x][p->y] = p->layer;
				}
			}
		}
	}

	SDL_UpdateTexture(screen, NULL, console->pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {

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

	SDL_Texture *screen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	PT_Console *console = PT_ConsoleInit(SCREEN_WIDTH, SCREEN_HEIGHT, 
										 NUM_ROWS, NUM_COLS);

	PT_ConsoleSetBitmapFont(console, "./terminal16x16.png", 0, 16, 16);


	world_state_init();

	GameObject *player = game_object_create();
	Visibility vis = {player->id, '@', 0x00FF00FF, 0x00000000};
	game_object_add_component(player, COMP_VISIBILITY, &vis);
	Physical phys = {player->id, true, true};
	game_object_add_component(player, COMP_PHYSICAL, &phys);


	// Create a level and place our player in it
	currentLevel = level_init(player);
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

	fov_calculate(playerPos->x, playerPos->y, fovMap);

	generate_target_map(playerPos->x, playerPos->y);

	bool done = false;
	bool recalculateFOV = false;
	bool playerMoved = false;
	while (!done) {

		playerMoved = false;

		SDL_Event event;
		u32 timePerFrame = 1000 / FPS_LIMIT;
		u32 frameStart = 0;
		while (SDL_PollEvent(&event) != 0) {

			frameStart = SDL_GetTicks();

			if (event.type == SDL_QUIT) {
				done = true; 
				break;
			}

			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

				switch (key) {

					// DEBUG
					case SDLK_m:
						level_init(player);
						break;
					// END DEBUG

					case SDLK_ESCAPE:
						done = true;
						break;

					case SDLK_UP: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y - 1, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_add_component(player, COMP_POSITION, &newPos);
							recalculateFOV = true;					
							playerMoved = true;		
						}
					}
					break;

					case SDLK_DOWN: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y + 1, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_add_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerMoved = true;		
						}
					}
					break;

					case SDLK_LEFT: {
						Position newPos = {playerPos->objectId, playerPos->x - 1, playerPos->y, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_add_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerMoved = true;		
						}
					}
					break;

					case SDLK_RIGHT: {
						Position newPos = {playerPos->objectId, playerPos->x + 1, playerPos->y, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_add_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerMoved = true;		
						}
					}
					break;

					default:
						break;
				}
			}
		}

		// Have things move themselves around the dungeon if the player moved
		if (playerMoved) {
			if (targetMap != NULL) {
				free(targetMap);
			}
			
			Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
			generate_target_map(playerPos->x, playerPos->y);
			movement_update();			
		}

		if (recalculateFOV) {
			Position *pos = (Position *)game_object_get_component(player, COMP_POSITION);
			fov_calculate(pos->x, pos->y, fovMap);
			recalculateFOV = false;
		}

		render_screen(renderer, screen, console);

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