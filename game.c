/*
* game.c
*/

#define UNUSED	-1

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
} GameComponentType;


/* Entity */
typedef struct {
	i32 id;
	void *components[COMPONENT_COUNT];
} GameObject;


/* Components */
typedef struct {
	i32 objectId;
	u8 x, y;	
	u8 layer;				// 1 is bottom layer
} Position;

typedef struct {
	i32 objectId;
	asciiChar glyph;
	u32 fgColor;
	u32 bgColor;
	bool hasBeenSeen;
	bool visibleOutsideFOV;
} Visibility;

typedef struct {
	i32 objectId;
	bool blocksMovement;
	bool blocksSight;
} Physical;

typedef struct {
	i32 objectId;
	i32 speed;				// How many spaces the object can move when it moves.
	i32 frequency;			// How often the object moves. 1=every tick, 2=every other tick, etc.
	i32 ticksUntilNextMove;	// Countdown to next move. Moves when = 0.
	Point destination;
	bool hasDestination;
	bool chasingPlayer;
	i32 turnsSincePlayerSeen;
} Movement;


/* Level Support */

typedef struct {
	bool (*mapWalls)[MAP_HEIGHT];
} DungeonLevel;


/* World State */
#define MAX_GO 	10000
global_variable GameObject gameObjects[MAX_GO];
global_variable List *positionComps;
global_variable List *visibilityComps;
global_variable List *physicalComps;
global_variable List *movementComps;

global_variable DungeonLevel *currentLevel;
global_variable u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
global_variable i32 (*targetMap)[MAP_HEIGHT] = NULL;

/* World State Management */
void world_state_init() {
	for (u32 i = 0; i < MAX_GO; i++) {
		gameObjects[i].id = UNUSED;
	}
	positionComps = list_init(free);
	visibilityComps = list_init(free);
	physicalComps = list_init(free);
	movementComps = list_init(free);
}


/* Game Object Management */
GameObject *game_object_create() {
	// Find the next available object space
	GameObject *go = NULL;
	for (i32 i = 0; i < MAX_GO; i++) {
		if (gameObjects[i].id == UNUSED) {
			go = &gameObjects[i];
			go->id = i;
			break;
		}
	}

	assert(go != NULL);		// Have we run out of game objects?

	for (i32 i = 0; i < COMPONENT_COUNT; i++) {
		go->components[i] = NULL;
	}

	return go;
}

void game_object_update_component(GameObject *obj, 
							  GameComponentType comp,
							  void *compData) {
	assert(obj->id != UNUSED);

	switch (comp) {
		case COMP_POSITION: {
			Position *pos = obj->components[COMP_POSITION];
			if (pos == NULL) {
				pos = (Position *)malloc(sizeof(Position));
			}
			Position *posData = (Position *)compData;
			pos->objectId = obj->id;
			pos->x = posData->x;
			pos->y = posData->y;
			pos->layer = posData->layer;

			list_insert_after(positionComps, NULL, pos);
			obj->components[comp] = pos;

			break;
		}

		case COMP_VISIBILITY: {
			Visibility *vis = obj->components[COMP_VISIBILITY];
			if (vis == NULL)  {
				vis = (Visibility *)malloc(sizeof(Visibility));
			}
			Visibility *visData = (Visibility *)compData;
			vis->objectId = obj->id;
			vis->glyph = visData->glyph;
			vis->fgColor = visData->fgColor;
			vis->bgColor = visData->bgColor;
			vis->hasBeenSeen = visData->hasBeenSeen;
			vis->visibleOutsideFOV = visData->visibleOutsideFOV;

			list_insert_after(visibilityComps, NULL, vis);
			obj->components[comp] = vis;

			break;
		}

		case COMP_PHYSICAL: {
			Physical *phys = obj->components[COMP_PHYSICAL];
			if (phys == NULL) {
				phys = (Physical *)malloc(sizeof(Physical));
			}
			Physical *physData = (Physical *)compData;
			phys->objectId = obj->id;
			phys->blocksSight = physData->blocksSight;
			phys->blocksMovement = physData->blocksMovement;

			list_insert_after(physicalComps, NULL, phys);
			obj->components[comp] = phys;

			break;
		}

		case COMP_MOVEMENT: {
			Movement *mv = obj->components[COMP_MOVEMENT];
			if (mv == NULL) {
				mv = (Movement *)malloc(sizeof(Movement));
			}
			Movement *mvData = (Movement *)compData;
			mv->objectId = obj->id;
			mv->speed = mvData->speed;
			mv->frequency = mvData->frequency;
			mv->ticksUntilNextMove = mvData->ticksUntilNextMove;
			mv->chasingPlayer = mvData->chasingPlayer;
			mv->turnsSincePlayerSeen = mvData->turnsSincePlayerSeen;

			list_insert_after(movementComps, NULL, mv);
			obj->components[comp] = mv;

			break;
		}

		default:
			assert(1 == 0);
	}

}

void game_object_destroy(GameObject *obj) {
	ListElement *elementToRemove = list_search(positionComps, obj->components[COMP_POSITION]);
	if (elementToRemove != NULL ) { list_remove(positionComps, elementToRemove); }

	elementToRemove = list_search(visibilityComps, obj->components[COMP_VISIBILITY]);
	if (elementToRemove != NULL ) { list_remove(visibilityComps, elementToRemove); }

	elementToRemove = list_search(physicalComps, obj->components[COMP_PHYSICAL]);
	if (elementToRemove != NULL ) { list_remove(physicalComps, elementToRemove); }

	elementToRemove = list_search(movementComps, obj->components[COMP_MOVEMENT]);
	if (elementToRemove != NULL ) { list_remove(movementComps, elementToRemove); }

	// TODO: Clean up other components used by this object

	obj->id = UNUSED;
}


void *game_object_get_component(GameObject *obj, 
								GameComponentType comp) {
	return obj->components[comp];
}


GameObject *game_object_at_position(u32 x, u32 y) {
	ListElement *e = list_head(positionComps);
	while (e != NULL) {
		Position *p = (Position *)list_data(e);
		if (p->x == x && p->y == y) {
			return &gameObjects[p->objectId];
		}		
		e = list_next(e);
	}

	return NULL;
}


/* Game objects */

void floor_add(u8 x, u8 y) {
	GameObject *floor = game_object_create();
	Position floorPos = {.objectId = floor->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_update_component(floor, COMP_POSITION, &floorPos);
	Visibility floorVis = {.objectId = floor->id, .glyph = '.', .fgColor = 0x3e3c3cFF, .bgColor = 0x00000000, .visibleOutsideFOV = true};
	game_object_update_component(floor, COMP_VISIBILITY, &floorVis);
	Physical floorPhys = {.objectId = floor->id, .blocksMovement = false, .blocksSight = false};
	game_object_update_component(floor, COMP_PHYSICAL, &floorPhys);
}

void npc_add(u8 x, u8 y, u8 layer, asciiChar glyph, u32 fgColor, u32 speed, u32 frequency) {
	GameObject *npc = game_object_create();
	Position pos = {.objectId = npc->id, .x = x, .y = y, .layer = layer};
	game_object_update_component(npc, COMP_POSITION, &pos);
	Visibility vis = {.objectId = npc->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000, .visibleOutsideFOV = false};
	game_object_update_component(npc, COMP_VISIBILITY, &vis);
	Physical phys = {.objectId = npc->id, .blocksMovement = true, .blocksSight = false};
	game_object_update_component(npc, COMP_PHYSICAL, &phys);
	Movement mv = {.objectId = npc->id, .speed = speed, .frequency = frequency, .ticksUntilNextMove = frequency, 
		.chasingPlayer = false, .turnsSincePlayerSeen = 0};
	game_object_update_component(npc, COMP_MOVEMENT, &mv);
}

void wall_add(u8 x, u8 y) {
	GameObject *wall = game_object_create();
	Position wallPos = {.objectId = wall->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_update_component(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0x675644FF, 0x00000000, .visibleOutsideFOV = true};
	game_object_update_component(wall, COMP_VISIBILITY, &wallVis);
	Physical wallPhys = {wall->id, true, true};
	game_object_update_component(wall, COMP_PHYSICAL, &wallPhys);
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
	game_object_update_component(player, COMP_POSITION, &pos);

	return level;
}

// TODO: Need a level cleanup function 


/* Movement System */

bool can_move(Position pos) {
	bool moveAllowed = true;

	if ((pos.x >= 0) && (pos.x < NUM_COLS) && (pos.y >= 0) && (pos.y < NUM_ROWS)) {
		ListElement *e = list_head(positionComps);
		while (e != NULL) {
			Position *p = (Position *)list_data(e);
			if (p->x == pos.x && p->y == pos.y) {
				Physical *phys = (Physical *)game_object_get_component(&gameObjects[p->objectId], COMP_PHYSICAL);
				if (phys->blocksMovement == true) {
					moveAllowed = false;
					break;
				}
			}		
			e = list_next(e);
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

	ListElement *e = list_head(movementComps);
	while (e != NULL) {
		Movement *mv = (Movement *)list_data(e);

		// Determine if the object is going to move this tick
		mv->ticksUntilNextMove -= 1;
		if (mv->ticksUntilNextMove <= 0) {
			// The object is moving, so determine new position based on destination and speed
			Position *p = (Position *)game_object_get_component(&gameObjects[mv->objectId], COMP_POSITION);
			Position newPos = {.objectId = p->objectId, .x = p->x, .y = p->y, .layer = p->layer};

			// A monster should only move toward the player if they have seen the player
			// Should give chase if player is currently in view or has been in view in the last 5 turns

			// If the player can see the monster, the monster can see the player
			bool giveChase = false;
			if (fovMap[p->x][p->y] > 0) {
				// Player is visible
				giveChase = true;
				mv->chasingPlayer = true;
				mv->turnsSincePlayerSeen = 0;
			} else {
				// Player is not visible - see if monster should still chase
				giveChase = mv->chasingPlayer;
				mv->turnsSincePlayerSeen += 1;
				if (mv->turnsSincePlayerSeen > 5) {
					mv->chasingPlayer = false;
				}
			}

			// Determine new position based on our target map
			if (giveChase) {
				// Evaluate all cardinal direction cells and pick randomly between optimal moves 
				Position moves[4];
				i32 moveCount = 0;
				i32 currTargetValue = targetMap[p->x][p->y];
				if (targetMap[p->x - 1][p->y] < currTargetValue) {
					Position np = newPos;
					np.x -= 1;	
					moves[moveCount] = np;					
					moveCount += 1;
				}
				if (targetMap[p->x][p->y - 1] < currTargetValue) { 
					Position np = newPos;
					np.y -= 1;						
					moves[moveCount] = np;					
					moveCount += 1;
				}
				if (targetMap[p->x + 1][p->y] < currTargetValue) { 
					Position np = newPos;
					np.x += 1;						
					moves[moveCount] = np;					
					moveCount += 1;
				}
				if (targetMap[p->x][p->y + 1] < currTargetValue) { 
					Position np = newPos;
					np.y += 1;						
					moves[moveCount] = np;					
					moveCount += 1;
				}

				u32 moveIdx = rand() % moveCount;
				newPos = moves[moveIdx];

			} else {
				// Move randomly?
				u32 dir = rand() % 4;
				switch (dir) {
					case 0:
						newPos.x -= 1;
						break;
					case 1:
						newPos.y -= 1;
						break;
					case 2:
						newPos.x += 1;
						break;
					default: 
						newPos.y += 1;
				}
			}

			// Test to see if the new position can be moved to
			if (can_move(newPos)) {
				game_object_update_component(&gameObjects[mv->objectId], COMP_POSITION, &newPos);
				mv->ticksUntilNextMove = mv->frequency;				
			} else {
				mv->ticksUntilNextMove += 1;
			}
		}

		e = list_next(e);
	}


}