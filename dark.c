/*
* dark.c - Dark Caverns Main Game Loop
*/

#include "assert.h"

#include <SDL2/SDL.h>

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720

#define NUM_COLS		80
#define NUM_ROWS 		45

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t		u8;
typedef uint32_t	u32;
typedef uint64_t	u64;
typedef int32_t		i32;
typedef int64_t		i64;


#define internal static
#define local_persist static
#define global_variable static


#include "pt_console.c"
#include "game.c"


bool canMove(Position pos) {
	bool moveAllowed = true;

	if ((pos.x >= 0) && (pos.x < NUM_COLS) && (pos.y >= 0) && (pos.y < NUM_ROWS)) {
		for (u32 i = 1; i < MAX_GO; i++) {
			Position p = positionComps[i];
			if ((p.objectId > 0) && (p.x == pos.x) && (p.y == pos.y)) {
				if (physicalComps[i].blocksMovement == true) {
					moveAllowed = false;
				}
			}
		}

	} else {
		moveAllowed = false;
	}

	return moveAllowed;
}

void renderScreen(SDL_Renderer *renderer, 
				  SDL_Texture *screen, 
				  PT_Console *console) {

	PT_ConsoleClear(console);

	for (u32 i = 1; i < MAX_GO; i++) {
		if (visibilityComps[i].objectId > 0) {
			Position *p = (Position *)getComponentForGameObject(&gameObjects[i], COMP_POSITION);
			PT_ConsolePutCharAt(console, visibilityComps[i].glyph, p->x, p->y, 
								visibilityComps[i].fgColor, visibilityComps[i].bgColor);
		}
	}

	SDL_UpdateTexture(screen, NULL, console->pixels, SCREEN_WIDTH * sizeof(u32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);
	SDL_RenderPresent(renderer);
}

int main() {

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


	GameObject *player = createGameObject();
	Position pos = {player->id, 25, 25};
	addComponentToGameObject(player, COMP_POSITION, &pos);
	Visibility vis = {player->id, '@', 0x00FF00FF, 0x000000FF};
	addComponentToGameObject(player, COMP_VISIBILITY, &vis);
	Physical phys = {player->id, true, true};
	addComponentToGameObject(player, COMP_PHYSICAL, &phys);

	GameObject *wall = createGameObject();
	Position wallPos = {wall->id, 30, 25};
	addComponentToGameObject(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0xFFFFFFFF, 0x000000FF};
	addComponentToGameObject(wall, COMP_VISIBILITY, &wallVis);
	Physical wallPhys = {wall->id, true, true};
	addComponentToGameObject(wall, COMP_PHYSICAL, &wallPhys);


	bool done = false;
	while (!done) {

		SDL_Event event;
		while (SDL_PollEvent(&event) != 0) {

			if (event.type == SDL_QUIT) {
				done = true;
				break;
			}

			if (event.type == SDL_KEYDOWN) {
				SDL_Keycode key = event.key.keysym.sym;

				Position *playerPos = (Position *)getComponentForGameObject(player, COMP_POSITION);

				switch (key) {
					case SDLK_ESCAPE:
						done = true;
						break;

					case SDLK_UP: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y - 1};
						if (canMove(newPos)) {
							addComponentToGameObject(player, COMP_POSITION, &newPos);							
						}
					}
					break;

					case SDLK_DOWN: {
						Position newPos = {playerPos->objectId, playerPos->x, playerPos->y + 1};
						if (canMove(newPos)) {
							addComponentToGameObject(player, COMP_POSITION, &newPos);							
						}
					}
					break;

					case SDLK_LEFT: {
						Position newPos = {playerPos->objectId, playerPos->x - 1, playerPos->y};
						if (canMove(newPos)) {
							addComponentToGameObject(player, COMP_POSITION, &newPos);							
						}
					}
					break;

					case SDLK_RIGHT: {
						Position newPos = {playerPos->objectId, playerPos->x + 1, playerPos->y};
						if (canMove(newPos)) {
							addComponentToGameObject(player, COMP_POSITION, &newPos);							
						}
					}
					break;

					default:
						break;
				}
			}
		}

		renderScreen(renderer, screen, console);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	SDL_Quit();

	return 0;
}