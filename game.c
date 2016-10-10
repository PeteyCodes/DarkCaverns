/*
* game.c
*/

#define UNUSED	100000

typedef enum {
	COMP_POSITION = 0,
	COMP_VISIBILITY,
	COMP_PHYSICAL,
	COMP_HEALTH,
	COMP_MOVEMENT,

	/* Define other components above here */
	COMPONENT_COUNT
} GameComponent;


/* Entity */
typedef struct {
	u32 id;
	void *components[COMPONENT_COUNT];
} GameObject;


/* Components */
typedef struct {
	u32 objectId;
	u8 x, y;	
} Position;

typedef struct {
	u32 objectId;
	asciiChar glyph;
	u32 fgColor;
	u32 bgColor;
	bool hasBeenSeen;
} Visibility;

typedef struct {
	u32 objectId;
	bool blocksMovement;
	bool blocksSight;
} Physical;


/* World State */
#define MAX_GO 	10000
global_variable GameObject gameObjects[MAX_GO];
global_variable Position positionComps[MAX_GO];
global_variable Visibility visibilityComps[MAX_GO];
global_variable Physical physicalComps[MAX_GO];


/* World State Management */
void world_state_init() {
	for (u32 i = 0; i < MAX_GO; i++) {
		gameObjects[i].id = UNUSED;
		positionComps[i].objectId = UNUSED;
		visibilityComps[i].objectId = UNUSED;
		physicalComps[i].objectId = UNUSED;
	}
}


/* Game Object Management */
GameObject *game_object_create() {
	// Find the next available object space
	GameObject *go = NULL;
	for (u32 i = 0; i < MAX_GO; i++) {
		if (gameObjects[i].id == UNUSED) {
			go = &gameObjects[i];
			go->id = i;
			break;
		}
	}

	assert(go != NULL);		// Have we run out of game objects?

	for (u32 i = 0; i < COMPONENT_COUNT; i++) {
		go->components[i] = NULL;
	}

	return go;
}

void game_object_add_component(GameObject *obj, 
							  GameComponent comp,
							  void *compData) {
	assert(obj->id != UNUSED);

	switch (comp) {
		case COMP_POSITION:
			Position *pos = &positionComps[obj->id];
			Position *posData = (Position *)compData;
			pos->objectId = obj->id;
			pos->x = posData->x;
			pos->y = posData->y;

			obj->components[comp] = pos;

			break;

		case COMP_VISIBILITY:
			Visibility *vis = &visibilityComps[obj->id];
			Visibility *visData = (Visibility *)compData;
			vis->objectId = obj->id;
			vis->glyph = visData->glyph;
			vis->fgColor = visData->fgColor;
			vis->bgColor = visData->bgColor;

			obj->components[comp] = vis;

			break;

		case COMP_PHYSICAL:
			Physical *phys = &physicalComps[obj->id];
			Physical *physData = (Physical *)compData;
			phys->objectId = obj->id;
			phys->blocksSight = physData->blocksSight;
			phys->blocksMovement = physData->blocksMovement;

			obj->components[comp] = phys;

			break;

		default:
			assert(1 == 0);
	}

}

void game_object_destroy(GameObject *obj) {
	positionComps[obj->id].objectId = UNUSED;
	visibilityComps[obj->id].objectId = UNUSED;
	physicalComps[obj->id].objectId = UNUSED;
	// TODO: Clean up other components used by this object

	obj->id = UNUSED;
}


void *game_object_get_component(GameObject *obj, 
								GameComponent comp) {
	return obj->components[comp];
}

GameObject *game_object_at_position(u32 x, u32 y) {
	for (u32 i = 0; i < MAX_GO; i++) {
		Position p = positionComps[i];
		if (p.objectId != UNUSED && p.x == x && p.y == y) {
			return &gameObjects[i];
		}
	}
	return NULL;
}


/* Game objects */

void wall_add(u8 x, u8 y) {
	GameObject *wall = game_object_create();
	Position wallPos = {wall->id, x, y};
	game_object_add_component(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0x675644FF, 0x000000FF};
	game_object_add_component(wall, COMP_VISIBILITY, &wallVis);
	Physical wallPhys = {wall->id, true, true};
	game_object_add_component(wall, COMP_PHYSICAL, &wallPhys);
}


/* Level Management */

void level_init(GameObject *player) {
	// Clear the previous level data from the world state
	for (u32 i = 0; i < MAX_GO; i++) {
		if ((gameObjects[i].id != player->id) && 
			(gameObjects[i].id != UNUSED)) {
			game_object_destroy(&gameObjects[i]);
		}
	}

	// Generate a level map into the world state
	map_generate();
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			if (mapCells[x][y]) {
				wall_add(x, y);
			}
		}
	}

	// Place the player in a random position within the level
	for (;;) {
		u32 x = rand() % MAP_WIDTH;
		u32 y = rand() % MAP_HEIGHT;
		if (mapCells[x][y] == false) {
			Position pos = {player->id, x, y};
			game_object_add_component(player, COMP_POSITION, &pos);
			break;
		}
	}
}
