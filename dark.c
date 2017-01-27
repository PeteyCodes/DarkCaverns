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
#include "pt_console.c"
#include "pt_ui.c"
#include "map.c"
#include "game.c"
#include "fov.c"


void render_screen(SDL_Renderer *renderer, 
				  SDL_Texture *screenTexture, 
				  UIScreen *screen) {

	PT_ConsoleClear(screen->console);

	// Render views from back to front for the current screen
	ListElement *e = list_head(screen->views);
	while (e != NULL) {
		UIView *v = (UIView *)list_data(e);
		v->render(screen->console);
		e = list_next(e);
	}

	SDL_UpdateTexture(screenTexture, NULL, screen->console->pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

internal void gameMapRender(PT_Console *console) {

	// Setup layer render history  
	u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			layerRendered[x][y] = LAYER_UNSET;
		}
	}

	ListElement *e = list_head(visibilityComps);
	while (e != NULL) {
		Visibility *vis = (Visibility *)list_data(e);
		Position *p = (Position *)game_object_get_component(&gameObjects[vis->objectId], COMP_POSITION);
		if (fovMap[p->x][p->y] > 0) {
			vis->hasBeenSeen = true;
			// Don't render if we've already written something to to this cell at a higher layer
			if (p->layer > layerRendered[p->x][p->y]) {
				PT_ConsolePutCharAt(console, vis->glyph, p->x, p->y, vis->fgColor, vis->bgColor);
				layerRendered[p->x][p->y] = p->layer;
			}

		} else if (vis->visibleOutsideFOV && vis->hasBeenSeen) {
			u32 fullColor = vis->fgColor;
			u32 fadedColor = COLOR_FROM_RGBA(RED(fullColor), GREEN(fullColor), BLUE(fullColor), 0x77);
			// Don't render if we've already written something to to this cell at a higher layer
			if (p->layer > layerRendered[p->x][p->y]) {
				PT_ConsolePutCharAt(console, vis->glyph, p->x, p->y, fadedColor, 0x000000FF);
				layerRendered[p->x][p->y] = p->layer;
			}
		}
		e = list_next(e);
	}

}

internal void statsRender(PT_Console *console) {
	
	PT_Rect rect = {0, 40, 20, 5};
	UI_DrawRect(console, &rect, 0x222222FF, 0, 0xFF990099);

	// HP health bar
	Health *playerHealth = game_object_get_component(player, COMP_HEALTH);
	PT_ConsolePutCharAt(console, 'H', 0, 41, 0xFF990099, 0x00000000);
	PT_ConsolePutCharAt(console, 'P', 1, 41, 0xFF990099, 0x00000000);
	i32 leftX = 3;
	i32 barWidth = 16;

	i32 healthCount = ceil(((float)playerHealth->currentHP / (float)playerHealth->maxHP) * barWidth);
	for (i32 x = 0; x < barWidth; x++) {
		if (x < healthCount) {
			PT_ConsolePutCharAt(console, 176, leftX + x, 41, 0x009900FF, 0x00000000);		
			PT_ConsolePutCharAt(console, 3, leftX + x, 41, 0x009900FF, 0x00000000);		
		} else {
			PT_ConsolePutCharAt(console, 176, leftX + x, 41, 0xFF990099, 0x00000000);		
		}
	}

	Combat *playerCombat = game_object_get_component(player, COMP_COMBAT);
	char *att = NULL;
	sasprintf(att, "ATT: %d (%d)", playerCombat->attack, playerCombat->attackModifier);
	PT_ConsolePutStringAt(console, att, 0, 42, 0xe6e600FF, 0x00000000);
	free(att);

	char *def = NULL;
	sasprintf(def, "DEF: %d (%d)", playerCombat->defense, playerCombat->defenseModifier);
	PT_ConsolePutStringAt(console, def, 0, 43, 0xe6e600FF, 0x00000000);
	free(def);

}

internal void messageLogRender(PT_Console *console) {

	PT_Rect rect = {22, 40, 58, 5};
	UI_DrawRect(console, &rect, 0x191919FF, 0, 0xFF990099);

	if (messageLog == NULL) { return; }

	// Get the last 5 messages from the log
	ListElement *e = list_tail(messageLog);
	i32 msgCount = list_size(messageLog);
	u32 row = 44;
	u32 col = 22;

	if (msgCount < 5) {
		row -= (5 - msgCount);
	} else {
		msgCount = 5;
	}

	for (i32 i = 0; i < msgCount; i++) {
		if (e != NULL) {
			Message *m = (Message *)list_data(e);
			PT_Rect rect = {.x = col, .y = row, .w = 58, .h = 1};
			PT_ConsolePutStringInRect(console, m->msg, rect, false, m->fgColor, 0x00000000);
			e = list_prev(e);			
			row -= 1;
		}
	}
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

	// Initialize UI state (screens, view stack, etc)
	// TODO: Move this somewhere else
	UIScreen *activeScreen = NULL;

	PT_Console *igConsole = PT_ConsoleInit(SCREEN_WIDTH, SCREEN_HEIGHT, NUM_ROWS, NUM_COLS);
	PT_ConsoleSetBitmapFont(igConsole, "./terminal16x16.png", 0, 16, 16);
	List *igViews = list_new(NULL);

	UIView *mapView = malloc(sizeof(UIView));
	mapView->render = gameMapRender;
	list_insert_after(igViews, NULL, mapView);

	UIView *statsView = malloc(sizeof(UIView));
	statsView->render = statsRender;
	list_insert_after(igViews, NULL, statsView);

	UIView *logView = malloc(sizeof(UIView));
	logView->render = messageLogRender;
	list_insert_after(igViews, NULL, logView);

	UIScreen *inGameScreen = malloc(sizeof(UIScreen));
	inGameScreen->console = igConsole;
	inGameScreen->views = igViews;

	activeScreen = inGameScreen;


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
