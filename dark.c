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


void render_screen(SDL_Renderer *renderer, 
				  SDL_Texture *screenTexture, 
				  UIScreen *screen) {

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

	SDL_Texture *screenTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

	// Initialize UI  state (screens, view stack, etc)
    UIScreen *activeScreen = NULL;

	// Show the in-game screen (for now)
	// TODO: Show the launch screen instead
	activeScreen = screen_show_in_game();

	// TODO: Move this somewhere else - group with other game startup code
	world_state_init();

	// Create our player
	player = game_object_create();
	Visibility vis = {.objectId=player->id, .glyph='@', .fgColor=0x00FF00FF, .bgColor=0x00000000, .hasBeenSeen=true};
	game_object_update_component(player, COMP_VISIBILITY, &vis);
	Physical phys = {player->id, true, true};
	game_object_update_component(player, COMP_PHYSICAL, &phys);
	Health hlth = {.objectId = player->id, .currentHP = 20, .maxHP = 20, .recoveryRate = 1};
	game_object_update_component(player, COMP_HEALTH, &hlth);
	Combat com = {.objectId = player->id, .toHit=80, .toHitModifier=0, .attack = 5, .defense = 2, .attackModifier = 0, .defenseModifier = 0, .hitModifier = 0};
	game_object_update_component(player, COMP_COMBAT, &com);

	// Create a level and place our player in it
	// TODO: Hand this fn the actual level number we should generate.
	i32 currentLevelNumber = 1;
	currentLevel = level_init(currentLevelNumber, player);
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

	fov_calculate(playerPos->x, playerPos->y, fovMap);

	generate_target_map(playerPos->x, playerPos->y);

	bool done = false;
	bool recalculateFOV = false;
	bool playerTookTurn = false;

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

			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

				switch (key) {

					// DEBUG
					case SDLK_m:
						// Restart a new level with a new map
						level_init(currentLevelNumber, player);
						playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
						fov_calculate(playerPos->x, playerPos->y, fovMap);
						generate_target_map(playerPos->x, playerPos->y);

						break;
					// END DEBUG

					case SDLK_ESCAPE:
						done = true;
						break;

					case SDLK_UP: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y - 1, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_update_component(player, COMP_POSITION, &newPos);
							recalculateFOV = true;					
							playerTookTurn = true;		

						} else {
							// Check to see what is blocking movement. If NPC - resolve combat!
							List *blockers = game_objects_at_position(playerPos->x, playerPos->y - 1);
							GameObject *blockerObj = NULL;
							ListElement *e = list_head(blockers);
							while (e != NULL) {
								GameObject *go = (GameObject *)list_data(e);
								Combat *cc = (Combat *)game_object_get_component(go, COMP_COMBAT);
								if (cc != NULL) {
									blockerObj = go;
									break;
								}
								e = list_next(e);
							}
							if (blockerObj != NULL) {
								combat_attack(player, blockerObj);
								playerTookTurn = true;		
							}
						}	
					}
					break;

					case SDLK_DOWN: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y + 1, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_update_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerTookTurn = true;		
						} else {
							// Check to see what is blocking movement. If NPC - resolve combat!
							List *blockers = game_objects_at_position(playerPos->x, playerPos->y + 1);
							GameObject *blockerObj = NULL;
							ListElement *e = list_head(blockers);
							while (e != NULL) {
								GameObject *go = (GameObject *)list_data(e);
								Combat *cc = (Combat *)game_object_get_component(go, COMP_COMBAT);
								if (cc != NULL) {
									blockerObj = go;
									break;
								}
								e = list_next(e);
							}
							if (blockerObj != NULL) {
								combat_attack(player, blockerObj);
								playerTookTurn = true;		
							}
						}	
					}
					break;

					case SDLK_LEFT: {
						Position newPos = {playerPos->objectId, playerPos->x - 1, playerPos->y, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_update_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerTookTurn = true;		
						} else {
							// Check to see what is blocking movement. If NPC - resolve combat!
							List *blockers = game_objects_at_position(playerPos->x - 1, playerPos->y);
							GameObject *blockerObj = NULL;
							ListElement *e = list_head(blockers);
							while (e != NULL) {
								GameObject *go = (GameObject *)list_data(e);
								Combat *cc = (Combat *)game_object_get_component(go, COMP_COMBAT);
								if (cc != NULL) {
									blockerObj = go;
									break;
								}
								e = list_next(e);
							}
							if (blockerObj != NULL) {
								combat_attack(player, blockerObj);
								playerTookTurn = true;		
							}
						}	
					}
					break;

					case SDLK_RIGHT: {
						Position newPos = {playerPos->objectId, playerPos->x + 1, playerPos->y, LAYER_TOP};
						if (can_move(newPos)) {
							game_object_update_component(player, COMP_POSITION, &newPos);							
							recalculateFOV = true;							
							playerTookTurn = true;		

						} else {
							// Check to see what is blocking movement. If NPC - resolve combat!
							List *blockers = game_objects_at_position(playerPos->x + 1, playerPos->y);
							GameObject *blockerObj = NULL;
							ListElement *e = list_head(blockers);
							while (e != NULL) {
								GameObject *go = (GameObject *)list_data(e);
								Combat *cc = (Combat *)game_object_get_component(go, COMP_COMBAT);
								if (cc != NULL) {
									blockerObj = go;
									break;
								}
								e = list_next(e);
							}
							if (blockerObj != NULL) {
								combat_attack(player, blockerObj);
								playerTookTurn = true;		
							}
						}	
					}
					break;

					case SDLK_i: {
						show_inventory_overlay(activeScreen);
					}
					break;

					case SDLK_z: {
						// Player rests
						health_recover_player_only();
						playerTookTurn = true;
					}
					break;

					default:
						break;
				}
			}
		}

		// Have things move themselves around the dungeon if the player moved
		if (playerTookTurn) {
			Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
			generate_target_map(playerPos->x, playerPos->y);
			movement_update();			

			health_removal_update();
		}

		if (recalculateFOV) {
			Position *pos = (Position *)game_object_get_component(player, COMP_POSITION);
			fov_calculate(pos->x, pos->y, fovMap);
			recalculateFOV = false;
		}

		// TODO: Determine active screen and render it
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
