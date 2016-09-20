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


/* Function Declarations */
bool map_carve_room(u32 x, u32 y, u32 w, u32 h);





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



/* Map Management */

#define MAP_WIDTH	80
#define MAP_HEIGHT	40

bool mapCells[MAP_WIDTH][MAP_HEIGHT];

void map_generate() {

	// Mark all the map cells as "filled"
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			mapCells[x][y] = true;
		}
	}

	// Carve out non-overlapping rooms that are randomly placed, and of 
	// random size. 
	bool roomsDone = false;
	PT_Rect rooms[100];
	u32 cellsUsed = 0;
	u32 roomCount = 0;
	while (!roomsDone) {
		// Generate a random width/height for a room
		u32 w = (rand() % 17) + 3;
		u32 h = (rand() % 17) + 3;
		u32 x = rand() % MAP_WIDTH - w - 1;
		u32 y = rand() % MAP_HEIGHT - h - 1;

		bool success = map_carve_room(x, y, w, h);
		if (success) {
			PT_Rect r = {x, y, w, h};
			rooms[roomCount] = r;
			roomCount += 1;
			cellsUsed += (w * h);
		}

		// Exit condition - more that desired % of cells in use
		if (((float)cellsUsed / (float)(MAP_HEIGHT * MAP_WIDTH)) > 0.65) {
			roomsDone = true;
		}
	}

	// Join all rooms with corridors, so that all rooms are reachable

}

bool map_carve_room(u32 x, u32 y, u32 w, u32 h) {
	// Determine if all the cells within the given rectangle are filled
	for (u8 i = x; i < x + w; i++) {
		for (u8 j = y; j < y + h; j++) {
			if (mapCells[i][j] == false) {
				return false;
			}
		}
	}

	// Carve out the room
	for (u8 i = x; i < x + w; i++) {
		for (u8 j = y; j < y + h; j++) {
			mapCells[i][j] = false;
		}
	}

	return true;
}


void wall_add(u8 x, u8 y) {
	GameObject *wall = game_object_create();
	Position wallPos = {wall->id, x, y};
	game_object_add_component(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0xFFFFFFFF, 0x000000FF};
	game_object_add_component(wall, COMP_VISIBILITY, &wallVis);
	Physical wallPhys = {wall->id, true, true};
	game_object_add_component(wall, COMP_PHYSICAL, &wallPhys);
}

