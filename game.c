/*
* game.c
*/

#define UNUSED	100000

#define LAYER_UNSET		0
#define LAYER_GROUND	1
#define LAYER_MID		2
#define LAYER_AIR		3
#define LAYER_TOP		4


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
	u8 layer;				// 1 is bottom layer
} Position;

typedef struct {
	u32 objectId;
	asciiChar glyph;
	u32 fgColor;
	u32 bgColor;
	bool hasBeenSeen;
	bool visibleOutsideFOV;
} Visibility;

typedef struct {
	u32 objectId;
	bool blocksMovement;
	bool blocksSight;
} Physical;

typedef struct {
	u32 objectId;
	u32 speed;				// How many spaces the object can move when it moves.
	u32 frequency;			// How often the object moves. 1=every tick, 2=every other tick, etc.
	u8 ticksUntilNextMove;	// Countdown to next move. Moves when = 0.
	Point destination;
	bool hasDestination;
} Movement;


/* Level Support */

typedef struct {
	bool (*mapWalls)[MAP_HEIGHT];
} DungeonLevel;


/* World State */
#define MAX_GO 	10000
global_variable GameObject gameObjects[MAX_GO];
global_variable Position positionComps[MAX_GO];
global_variable Visibility visibilityComps[MAX_GO];
global_variable Physical physicalComps[MAX_GO];
global_variable Movement movementComps[MAX_GO];

global_variable DungeonLevel *currentLevel;
global_variable i32 (*targetMap)[MAP_HEIGHT] = NULL;




/* World State Management */
void world_state_init() {
	for (u32 i = 0; i < MAX_GO; i++) {
		gameObjects[i].id = UNUSED;
		positionComps[i].objectId = UNUSED;
		visibilityComps[i].objectId = UNUSED;
		physicalComps[i].objectId = UNUSED;
		movementComps[i].objectId = UNUSED;
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
		case COMP_POSITION: {
			Position *pos = &positionComps[obj->id];
			Position *posData = (Position *)compData;
			pos->objectId = obj->id;
			pos->x = posData->x;
			pos->y = posData->y;
			pos->layer = posData->layer;

			obj->components[comp] = pos;

			break;
		}

		case COMP_VISIBILITY: {
			Visibility *vis = &visibilityComps[obj->id];
			Visibility *visData = (Visibility *)compData;
			vis->objectId = obj->id;
			vis->glyph = visData->glyph;
			vis->fgColor = visData->fgColor;
			vis->bgColor = visData->bgColor;
			vis->visibleOutsideFOV = visData->visibleOutsideFOV;

			obj->components[comp] = vis;

			break;
		}

		case COMP_PHYSICAL: {
			Physical *phys = &physicalComps[obj->id];
			Physical *physData = (Physical *)compData;
			phys->objectId = obj->id;
			phys->blocksSight = physData->blocksSight;
			phys->blocksMovement = physData->blocksMovement;

			obj->components[comp] = phys;

			break;
		}

		case COMP_MOVEMENT: {
			Movement *mv = &movementComps[obj->id];
			Movement *mvData = (Movement *)compData;
			mv->objectId = obj->id;
			mv->speed = mvData->speed;
			mv->frequency = mvData->frequency;
			mv->ticksUntilNextMove = mvData->ticksUntilNextMove;

			obj->components[comp] = mv;

			break;
		}

		default:
			assert(1 == 0);
	}

}

void game_object_destroy(GameObject *obj) {
	positionComps[obj->id].objectId = UNUSED;
	visibilityComps[obj->id].objectId = UNUSED;
	physicalComps[obj->id].objectId = UNUSED;
	movementComps[obj->id].objectId = UNUSED;
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

void floor_add(u8 x, u8 y) {
	GameObject *floor = game_object_create();
	Position floorPos = {.objectId = floor->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_add_component(floor, COMP_POSITION, &floorPos);
	Visibility floorVis = {.objectId = floor->id, .glyph = '.', .fgColor = 0x3e3c3cFF, .bgColor = 0x00000000, .visibleOutsideFOV = true};
	game_object_add_component(floor, COMP_VISIBILITY, &floorVis);
	Physical floorPhys = {.objectId = floor->id, .blocksMovement = false, .blocksSight = false};
	game_object_add_component(floor, COMP_PHYSICAL, &floorPhys);
}

void npc_add(u8 x, u8 y, u8 layer, asciiChar glyph, u32 fgColor, u32 speed, u32 frequency) {
	GameObject *npc = game_object_create();
	Position pos = {.objectId = npc->id, .x = x, .y = y, .layer = layer};
	game_object_add_component(npc, COMP_POSITION, &pos);
	Visibility vis = {.objectId = npc->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000, .visibleOutsideFOV = false};
	game_object_add_component(npc, COMP_VISIBILITY, &vis);
	Physical phys = {.objectId = npc->id, .blocksMovement = true, .blocksSight = false};
	game_object_add_component(npc, COMP_PHYSICAL, &phys);
	Movement mv = {.objectId = npc->id, .speed = speed, .frequency = frequency, .ticksUntilNextMove = frequency};
	game_object_add_component(npc, COMP_MOVEMENT, &mv);
}

void wall_add(u8 x, u8 y) {
	GameObject *wall = game_object_create();
	Position wallPos = {.objectId = wall->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_add_component(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0x675644FF, 0x00000000, .visibleOutsideFOV = true};
	game_object_add_component(wall, COMP_VISIBILITY, &wallVis);
	Physical wallPhys = {wall->id, true, true};
	game_object_add_component(wall, COMP_PHYSICAL, &wallPhys);
}


/* Level Management */

Point level_get_open_point(bool (*mapCells)[MAP_HEIGHT]) {
	// Return a random position within the level that is open
	for (;;) {
		u32 x = rand() % MAP_WIDTH;
		u32 y = rand() % MAP_HEIGHT;
		if (mapCells[x][y] == false) {
			return (Point) {x, y};
		}
	}
}

DungeonLevel * level_init(GameObject *player) {
	// Clear the previous level data from the world state
	for (u32 i = 0; i < MAX_GO; i++) {
		if ((gameObjects[i].id != player->id) && 
			(gameObjects[i].id != UNUSED)) {
			game_object_destroy(&gameObjects[i]);
		}
	}

	// Generate a level map into the world state
	bool (*mapCells)[MAP_HEIGHT] = malloc(MAP_WIDTH * MAP_HEIGHT);

	map_generate(mapCells);

	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			if (mapCells[x][y]) {
				wall_add(x, y);
			} else {
				floor_add(x, y);
			}
		}
	}

	// Create DungeonLevel Object and store relevant info
	DungeonLevel *level = malloc(sizeof(DungeonLevel));
	level->mapWalls = mapCells;

	// Generate some monsters and drop them randomly in the level
	// TODO: Remove hard-coding
	Point pt = level_get_open_point(mapCells);
	npc_add(pt.x, pt.y, LAYER_TOP, 'g', 0xaa0000ff, 1, 1);
	pt = level_get_open_point(mapCells);
	npc_add(pt.x, pt.y, LAYER_TOP, 's', 0x009900ff, 1, 1);
	pt = level_get_open_point(mapCells);
	npc_add(pt.x, pt.y, LAYER_TOP, 'k', 0x000077ff, 1, 1);

	// Place our player in a random position in the level
	pt = level_get_open_point(mapCells);
	Position pos = {.objectId = player->id, .x = pt.x, .y = pt.y, .layer = LAYER_TOP};
	game_object_add_component(player, COMP_POSITION, &pos);

	return level;
}

// TODO: Need a level cleanup function 


/* Movement System */

bool can_move(Position pos) {
	bool moveAllowed = true;

	// TODO: Make this function more performant - it's very slow

	if ((pos.x >= 0) && (pos.x < NUM_COLS) && (pos.y >= 0) && (pos.y < NUM_ROWS)) {
		for (u32 i = 0; i < MAX_GO; i++) {
			Position p = positionComps[i];
			if ((p.objectId != UNUSED) && (p.x == pos.x) && (p.y == pos.y)) {
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


typedef struct {
	Point target;
	i32 weight;
} TargetPoint;

bool is_wall(i32 x, i32 y) {
	return currentLevel->mapWalls[x][y];
}

void generate_target_map(i32 targetX, i32 targetY) { // List *targetPoints) {
	i32 (* dmap)[MAP_HEIGHT] = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(i32));
	i32 UNSET = 9999;

	for (i32 x = 0; x < MAP_WIDTH; x++) {
		for (i32 y = 0; y < MAP_HEIGHT; y++) {
			dmap[x][y] = UNSET;
		}
	}

	// Set our target point(s)
	dmap[targetX][targetY] = 0;

	// Calculate our target map
	bool changesMade = true;
	while (changesMade) {
		changesMade = false;

		for (i32 x = 0; x < MAP_WIDTH; x++) {
			for (i32 y = 0; y < MAP_HEIGHT; y++) {
				i32 currCellValue = dmap[x][y];
				if (currCellValue != UNSET) {
					// Check cells around this one and update them if warranted
					if ((!is_wall(x+1, y)) && (dmap[x+1][y] > currCellValue + 1)) { 
						dmap[x+1][y] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x-1, y)) && (dmap[x-1][y] > currCellValue + 1)) { 
						dmap[x-1][y] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x, y-1)) && (dmap[x][y-1] > currCellValue + 1)) { 
						dmap[x][y-1] = currCellValue + 1;
						changesMade = true;
					}
					if ((!is_wall(x, y+1)) && (dmap[x][y+1] > currCellValue + 1)) { 
						dmap[x][y+1] = currCellValue + 1;
						changesMade = true;
					}
				}
			}
		}
	}

	targetMap = dmap;
}

void movement_update() {

	for (u32 i = 0; i < MAX_GO; i++) {
		if (movementComps[i].objectId != UNUSED) {
			Movement *mv = &movementComps[i];

			// Determine if the object is going to move this tick
			mv->ticksUntilNextMove -= 1;
			if (mv->ticksUntilNextMove <= 0) {
				// The object is moving, so determine new position based on destination and speed
				Position *p = (Position *)game_object_get_component(&gameObjects[i], COMP_POSITION);
				Position newPos = {.objectId = p->objectId, .x = p->x, .y = p->y, .layer = p->layer};

				// TODO: A monster should only move toward the player if they have seen the player

				// TODO: Evaluate all cardinal direction cells and pick randomly between optimal moves

				// Determine new position based on our target map
				i32 currTargetValue = targetMap[p->x][p->y];
				if (targetMap[p->x - 1][p->y] < currTargetValue) { newPos.x -= 1; }
				else if (targetMap[p->x][p->y - 1] < currTargetValue) { newPos.y -= 1; }
				else if (targetMap[p->x + 1][p->y] < currTargetValue) { newPos.x += 1; }
				else if (targetMap[p->x][p->y + 1] < currTargetValue) { newPos.y += 1; }

				// Test to see if the new position can be moved to
				if (can_move(newPos)) {
					game_object_add_component(&gameObjects[i], COMP_POSITION, &newPos);
					mv->ticksUntilNextMove = mv->frequency;				
				} else {
					mv->ticksUntilNextMove += 1;
				}
			}

		}
	}

}