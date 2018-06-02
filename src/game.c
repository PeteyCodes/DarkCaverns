/*
* game.c
*/

#include <time.h>

#include "config.h"
#include "dark.h"
#include "fov.h"
#include "game.h"
#include "list.h"
#include "screen_end_game.h"
#include "screen_win_game.h"
#include "String.h"
#include "ui.h"
#include "util.h"


#define EQUIP_LIFETIME 500


/* Game State */

bool gameIsRunning = true;

char* playerName = NULL;

List *carriedItems;
i32 maxWeightAllowed = 20;

i32 gemsFoundThisLevel = 0;
i32 gemsFoundTotal = 0;

bool currentlyInGame = false;
bool recalculateFOV = false;
bool playerTookTurn = false;

i32 currentLevelNumber;
DungeonLevel *currentLevel;
u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
i32 (*targetMap)[MAP_HEIGHT] = NULL;
Config *monsterConfig = NULL;
i32 monsterProbability[MONSTER_TYPE_COUNT][MAX_DUNGEON_LEVEL];		// TODO: dynamically size this based on actual count of monsters in config file
Config *itemConfig = NULL;
i32 itemProbability[ITEM_TYPE_COUNT][MAX_DUNGEON_LEVEL];		// TODO: dynamically size this based on actual count of monsters in config file
Config *levelConfig = NULL;
i32 maxMonsters[MAX_DUNGEON_LEVEL];
i32 maxItems[MAX_DUNGEON_LEVEL];
List *messageLog = NULL;
Config *hofConfig = NULL;


/* Necessary function declarations */

void add_message(char *msg, u32 color);
void generate_target_map(i32 targetX, i32 targetY);
void combat_attack(GameObject *attacker, GameObject *defender);
void item_toggle_equip(GameObject *item);
void animateGem(u32 gameObjectId);


/* World State Management */

void get_appearance_prob(Config *config, i32 (*probabilities)[MAX_DUNGEON_LEVEL]) {
	ListElement *e = list_head(config->entities);
	while (e != NULL) {
		ConfigEntity *entity = (ConfigEntity *)e->data;
		char *appearance_prob = config_entity_value(entity, "appearance_prob");
		char *copy = (char *)calloc(strlen(appearance_prob) + 1, sizeof(char));
		strcpy(copy, appearance_prob);

		char *idString = config_entity_value(entity, "id");
		i32 id = atoi(idString);

		char *lvl = strtok(copy, ",");
		if (lvl != NULL) {
			i32 lastLvl = 0;
			bool probData = true;
			while (probData) {
				char *prob = strtok(NULL, ",");
				i32 lvlNum = atoi(lvl);
				i32 probNum = atoi(prob);	

				// Fill in the probabilities from our last filled level to the current level
				for (i32 i = lastLvl; i < lvlNum; i++) {
					probabilities[id-1][i] = probNum;							
				}				

				lastLvl = lvlNum;
				lvl = strtok(NULL, ",");
				if (lvl == NULL) {
					probData = false;
				}
			}
		}

		free(copy);
		e = list_next(e);
	}

}

void get_max_counts(ConfigEntity *entity, char *propertyName, i32 *maxCounts) {
	char *countsString = config_entity_value(entity, propertyName);
	char *copy = (char *)calloc(strlen(countsString) + 1, sizeof(char));
	strcpy(copy, countsString);

	char *lvl = strtok(copy, ",");
	if (lvl != NULL) {
		i32 lastLvl = 0;
		bool lvlCountData = true;
		while (lvlCountData) {
			char *sCount = strtok(NULL, ",");
			i32 lvlNum = atoi(lvl);
			i32 maxCount = atoi(sCount);

			// Fill in the probabilities from our last filled level to the current level
			for (i32 i = lastLvl; i < lvlNum; i++) {
				maxCounts[i] = maxCount;
			}				

			lastLvl = lvlNum;
			lvl = strtok(NULL, ",");
			if (lvl == NULL) {
				lvlCountData = false;
			}
		}
	}

	free(copy);
}

void world_state_init() {
	for (u32 i = 0; i < MAX_GO; i++) {
		gameObjects[i].id = UNUSED;
	}
	positionComps = list_new(free);
	visibilityComps = list_new(free);
	physicalComps = list_new(free);
	movementComps = list_new(free);
	healthComps = list_new(free);
	combatComps = list_new(free);
	equipmentComps = list_new(free);
	treasureComps = list_new(free);
	animationComps = list_new(free);

	carriedItems = list_new(free);
	gemsFoundTotal = 0;

	// Parse necessary config files into memory
	monsterConfig = config_file_parse("./config/monsters.cfg");
	itemConfig = config_file_parse("./config/items.cfg");
	levelConfig = config_file_parse("./config/levels.cfg");

	// Generate our monster appearance probability data
	get_appearance_prob(monsterConfig, monsterProbability);

	// Do the same for item probabilities
	get_appearance_prob(itemConfig, itemProbability);

	// Get level generation config information
	ListElement *e = list_head(levelConfig->entities);
	if (e != NULL) {
		ConfigEntity *levelEntity = (ConfigEntity *)e->data;
		get_max_counts(levelEntity, "max_monsters", maxMonsters);
		get_max_counts(levelEntity, "max_items", maxItems);
	}

	// Clear our message log if necessary
	if (messageLog != NULL) {
		list_destroy(messageLog);
		messageLog = NULL;
	}
	
	// TODO: Other one-time data generation using config data?

}




/* Game objects */

void floor_add(u8 x, u8 y) {
	GameObject *floor = game_object_create();
	Position floorPos = {.objectId = floor->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_update_component(floor, COMP_POSITION, &floorPos);
	Visibility floorVis = {.objectId = floor->id, .glyph = '.', .fgColor = 0x3e3c3cFF, .bgColor = 0x00000000, .visibleOutsideFOV = true, .name="Floor"};
	game_object_update_component(floor, COMP_VISIBILITY, &floorVis);
	Physical floorPhys = {.objectId = floor->id, .blocksMovement = false, .blocksSight = false};
	game_object_update_component(floor, COMP_PHYSICAL, &floorPhys);
}

void item_add(char *name, u8 x, u8 y, u8 layer, asciiChar glyph, u32 fgColor, 
	i32 hitMod, i32 attMod, i32 defMod, i32 quantity, i32 weight, char *slot) {

	GameObject *item = game_object_create();
	Position pos = {.objectId = item->id, .x = x, .y = y, .layer = layer};
	game_object_update_component(item, COMP_POSITION, &pos);
	Physical phys = {.objectId = item->id, .blocksMovement = false, .blocksSight = false};
	game_object_update_component(item, COMP_PHYSICAL, &phys);
	Visibility vis = {.objectId = item->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000, .visibleOutsideFOV = false, .name = name};
	game_object_update_component(item, COMP_VISIBILITY, &vis);
	Combat com = {.objectId = item->id, .toHitModifier = hitMod, .attackModifier = attMod, .defenseModifier = defMod };
	game_object_update_component(item, COMP_COMBAT, &com);
	Equipment eq = {.objectId = item->id, .quantity = quantity, .weight = weight, .slot = slot, .lifetime = EQUIP_LIFETIME};
	game_object_update_component(item, COMP_EQUIPMENT, &eq);
}

void npc_add(char *name, u8 x, u8 y, u8 layer, asciiChar glyph, u32 fgColor, 
	u32 speed, u32 frequency, i32 maxHP, i32 hpRecRate, 
	i32 toHit, i32 hitMod, i32 attack, i32 defense, i32 attMod, i32 defMod) {
	
	GameObject *npc = game_object_create();
	Position pos = {.objectId = npc->id, .x = x, .y = y, .layer = layer};
	game_object_update_component(npc, COMP_POSITION, &pos);
	Visibility vis = {.objectId = npc->id, .glyph = glyph, .fgColor = fgColor, .bgColor = 0x00000000, .visibleOutsideFOV = false, .name = name};
	game_object_update_component(npc, COMP_VISIBILITY, &vis);
	Physical phys = {.objectId = npc->id, .blocksMovement = true, .blocksSight = false};
	game_object_update_component(npc, COMP_PHYSICAL, &phys);
	Movement mv = {.objectId = npc->id, .speed = speed, .frequency = frequency, .ticksUntilNextMove = frequency, 
		.chasingPlayer = false, .turnsSincePlayerSeen = 0};
	game_object_update_component(npc, COMP_MOVEMENT, &mv);
	Health hlth = {.objectId = npc->id, .currentHP = maxHP, .maxHP = maxHP, .recoveryRate = hpRecRate};
	game_object_update_component(npc, COMP_HEALTH, &hlth);
	Combat com = {.objectId = npc->id, .attack = attack, .defense = defense, .toHit = toHit, .toHitModifier = hitMod, .attackModifier = attMod, .defenseModifier = defMod};
	game_object_update_component(npc, COMP_COMBAT, &com);
}

void wall_add(u8 x, u8 y) {
	GameObject *wall = game_object_create();
	Position wallPos = {.objectId = wall->id, .x = x, .y = y, .layer = LAYER_GROUND};
	game_object_update_component(wall, COMP_POSITION, &wallPos);
	Visibility wallVis = {wall->id, '#', 0x675644FF, 0x00000000, .visibleOutsideFOV = true, .name="Wall"};
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
			bool isOccupied = false;
			List *objs = game_objects_at_position(x, y);
			ListElement *le = list_head(objs);
			while (le != NULL) {
				Equipment *eq = (Equipment *)game_object_get_component(le->data, COMP_EQUIPMENT);
				Health *h = (Health *)game_object_get_component(le->data, COMP_HEALTH);
				Treasure *t = (Treasure *)game_object_get_component(le->data, COMP_TREASURE);
				if ((eq != NULL) || (h != NULL) || (t != NULL)) { 
					isOccupied = true;
					break; 
				}
				le = list_next(le);
			}
			if (!isOccupied) {
				return (Point) {x, y};
			}
		}
	}
}

i32 item_for_level(i32 level) {
	u32 r = rand() % 100;
	u32 accum = 0;
	for (int i = 0; i < ITEM_TYPE_COUNT; i++) {
		accum += itemProbability[i][level-1];
		if (accum >= r) {
			return i + 1;
		}
	}
	return 1;
}

i32 monster_for_level(i32 level) {
	u32 r = rand() % 100;
	u32 accum = 0;
	for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
		accum += monsterProbability[i][level-1];
		if (accum >= r) {
			return i + 1;
		}
	}
	return 1;
}

ConfigEntity * get_entity_with_id(Config *config, i32 id) {
	ListElement *e = list_head(config->entities);
	while (e != NULL) {
		ConfigEntity *entity = (ConfigEntity *)e->data;
		i32 eId = atoi(config_entity_value(entity, "id"));
		if (eId == id) {
			return entity;
		}

		e = list_next(e);
	}

	return NULL;
}


DungeonLevel * level_init(i32 levelToGenerate, GameObject *player) {
	// Clear the previous level data from the world state
	// Note: We start at index 1 because the player is at index 0 and we want to keep them!
	for (u32 i = 1; i < MAX_GO; i++) {
		if ((gameObjects[i].id != player->id) && 
			(gameObjects[i].id != UNUSED) &&
			list_search(carriedItems, &gameObjects[i]) == NULL) {

			game_object_destroy(&gameObjects[i]);
		}
	}

	// Check for game win scenario
	if (levelToGenerate == 21) {
		game_over();
		ui_set_active_screen(screen_show_win_game());
		return NULL;
	}

	// Generate a level map into the world state
	bool (*mapCells)[MAP_HEIGHT] = calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(bool));

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
	DungeonLevel *level = calloc(1, sizeof(DungeonLevel));
	level->level = levelToGenerate;
	level->mapWalls = mapCells;

	// Grab the number of monsters to generate for this level from level config
	i32 monstersToAdd = maxMonsters[levelToGenerate-1];
	for (i32 i = 0; i < monstersToAdd; i++) {
		// Consult our monster appearance data to determine what monster to generate.
		i32 monsterId = monster_for_level(levelToGenerate);
		ConfigEntity *monsterEntity = get_entity_with_id(monsterConfig, monsterId);

		if (monsterEntity != NULL) {
			// Add the monster		
			Point pt = level_get_open_point(mapCells);
			char *name = config_entity_value(monsterEntity, "name");
			char *glyph = config_entity_value(monsterEntity, "vis_glyph");
			asciiChar g = atoi(glyph);
			char *color = config_entity_value(monsterEntity, "vis_color");
			u32 c = xtoi(color);
			char *speed = config_entity_value(monsterEntity, "mv_speed");
			u32 s = atoi(speed);
			char *freq = config_entity_value(monsterEntity, "mv_frequency");
			u32 f = atoi(freq);
			char *maxHP = config_entity_value(monsterEntity, "h_maxHP");
			i32 hp = atoi(maxHP);
			char *recRate = config_entity_value(monsterEntity, "h_recRate");
			i32 rr = atoi(recRate);
			i32 hit = atoi(config_entity_value(monsterEntity, "com_toHit"));
			i32 att = atoi(config_entity_value(monsterEntity, "com_attack"));
			i32 def = atoi(config_entity_value(monsterEntity, "com_defense"));

			npc_add(name, pt.x, pt.y, LAYER_TOP, g, c, s, f, hp, rr, hit, 0, att, def, 0, 0);
		}
	}

	// Sprinkle some items throughout the level
	i32 itemsToAdd = maxItems[levelToGenerate-1];
	for (i32 i = 0; i < itemsToAdd; i++) {
		// Consult our item appearance data to determine what item to generate.
		i32 itemId = item_for_level(levelToGenerate);
		ConfigEntity *entity = get_entity_with_id(itemConfig, itemId);

		if (entity != NULL) {
			// Add the item		
			Point pt = level_get_open_point(mapCells);
			char *name = config_entity_value(entity, "name");
			char *glyph = config_entity_value(entity, "vis_glyph");
			asciiChar g = atoi(glyph);
			char *color = config_entity_value(entity, "vis_color");
			u32 c = xtoi(color);

			i32 toHitMod = atoi(config_entity_value(entity, "com_toHitModifier"));
			i32 attMod = atoi(config_entity_value(entity, "com_attackModifier"));
			i32 defMod = atoi(config_entity_value(entity, "com_defenseModifier"));

			i32 qty = atoi(config_entity_value(entity, "eq_quantity"));
			char *slot = config_entity_value(entity, "eq_slot");
			i32 weight = atoi(config_entity_value(entity, "eq_weight"));

			item_add(name, pt.x, pt.y, LAYER_MID, g, c, toHitMod, attMod, defMod, qty, weight, slot);
		}
	}
	
	// Place gems in random positions around the level
	gemsFoundThisLevel = 0;
	for (i32 i = 0; i < GEMS_PER_LEVEL; i++) {
		GameObject *gem = game_object_create();
		Point ptGem = level_get_open_point(mapCells);
		Position gemPos = {.objectId = gem->id, .x = ptGem.x, .y = ptGem.y, .layer = LAYER_MID};
		game_object_update_component(gem, COMP_POSITION, &gemPos);
		Visibility vis = {.objectId = gem->id, .glyph = 4, .fgColor = 0x753aabff, .bgColor = 0x00000000, .visibleOutsideFOV = false, .name="Gem"};
		game_object_update_component(gem, COMP_VISIBILITY, &vis);
		Physical phys = {.objectId = gem->id, .blocksMovement = false, .blocksSight = false};
		game_object_update_component(gem, COMP_PHYSICAL, &phys);
		Treasure treas = {.objectId = gem->id, .value = 1};
		game_object_update_component(gem, COMP_TREASURE, &treas);
		Animation anim = {.objectId = gem->id, .keyFrameInterval = 3, .ticksUntilKeyframe = 3, .finished = false, .keyframeAnimation = animateGem, .value1 = 0};
		game_object_update_component(gem, COMP_ANIMATION, &anim);
	}

	// Place a staircase in a random position in the level
	GameObject *stairs = game_object_create();
	Point ptStairs = level_get_open_point(mapCells);
	Position stairPos = {.objectId = stairs->id, .x = ptStairs.x, .y = ptStairs.y, .layer = LAYER_MID};
	game_object_update_component(stairs, COMP_POSITION, &stairPos);
	if (levelToGenerate < 20) {
		Visibility vis = {.objectId = stairs->id, .glyph = '>', .fgColor = 0xffd700ff, .bgColor = 0x00000000, .visibleOutsideFOV = true, .name="Stairs"};
		game_object_update_component(stairs, COMP_VISIBILITY, &vis);
	} else {
		Visibility vis = {.objectId = stairs->id, .glyph = 15, .fgColor = 0x80ff80ff, .bgColor = 0x00000000, .visibleOutsideFOV = true, .name="Stairs"};
		game_object_update_component(stairs, COMP_VISIBILITY, &vis);
	}
	Physical phys = {.objectId = stairs->id, .blocksMovement = false, .blocksSight = false};
	game_object_update_component(stairs, COMP_PHYSICAL, &phys);

	// Place our player in a random position in the level
	Point pt = level_get_open_point(mapCells);
	Position pos = {.objectId = player->id, .x = pt.x, .y = pt.y, .layer = LAYER_TOP};
	game_object_update_component(player, COMP_POSITION, &pos);

	return level;
}

void level_descend() {
	// Make sure that the player is on a staircase (or the end portal)
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

	// Get objects at player's current position
	List *objects = game_objects_at_position(playerPos->x, playerPos->y);
	ListElement *e = list_head(objects);
	bool foundStairs = false;
	while (e != NULL) {
		GameObject *go = (GameObject *)list_data(e);
		Visibility *v = (Visibility *)game_object_get_component(go, COMP_VISIBILITY);
		if ((v != NULL) && (String_Equals(v->name, "Stairs"))) {
			foundStairs = true;
			break;
		}
		e = list_next(e);
	}

	if (foundStairs) {
		currentLevelNumber += 1;
		currentLevel = level_init(currentLevelNumber, player);

		if (currentLevelNumber <= 20) {
			Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
			fov_calculate(playerPos->x, playerPos->y, fovMap);
			generate_target_map(playerPos->x, playerPos->y);

			char *msg = String_Create("You descend further, and are now on level %d.", currentLevelNumber);
			add_message("---------------------------------------------------", 0x555555ff);
			add_message(msg, 0x990000ff);
			String_Destroy(msg);
		}

		// Buff the player's base attack and defense
		Combat *combatStats = (Combat *)game_object_get_component(player, COMP_COMBAT);
		combatStats->attack += 2;
		combatStats->defense += 1;

	} else {
		add_message("There are no stairs here, you silly person.", 0x555555ff);
	}

}

// TODO: Need a level cleanup function 


/* Message */
void add_message(char *msg, u32 color) {

	Message *m = calloc(1, sizeof(Message));
	if (msg != NULL) {
		m->msg = calloc(strlen(msg) + 1, sizeof(char));
		strcpy(m->msg, msg);		
	} else {
		m->msg = "";
	}
	m->fgColor = color;

	// Add message to log
	if (messageLog == NULL) {
		messageLog = list_new(NULL);
	}
	list_insert_after(messageLog, list_tail(messageLog), m);

	// If our log has exceeded 20 messages, cull the older messages
	if (list_size(messageLog) > 20) {
		list_remove(messageLog, NULL);  // Remove the oldest message
	}

}

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

// TODO: Allow for a list of target points to be provided, with differing starting weights/priorities?
void generate_target_map(i32 targetX, i32 targetY) { // List *targetPoints) {
	// Clean up any previously generated target map
	if (targetMap != NULL) {
		free(targetMap);
	}

	i32 (* dmap)[MAP_HEIGHT] = calloc(MAP_WIDTH * MAP_HEIGHT, sizeof(i32));
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

			i32 speedCounter = mv->speed;
			while (speedCounter > 0) {
				// Determine if we're currently in combat range of the player
				if ((fovMap[p->x][p->y] > 0) && (targetMap[p->x][p->y] == 1)) {
					// Combat range - so attack the player
					combat_attack(&gameObjects[mv->objectId], player);

				} else {
					// Out of combat range, so determine new position based on our target map
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

						if (moveCount > 0) {
							u32 moveIdx = rand() % moveCount;
							newPos = moves[moveIdx];
						}

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

				speedCounter -= 1;
			}
		}

		e = list_next(e);
	}


}


/* Health Routines */

void health_check_death(GameObject *go) {
	Health *h = (Health *)game_object_get_component(go, COMP_HEALTH);
	if (h->currentHP <= 0) {
		// Death!
		if (go == player) {
			char *msg = String_Create("You have died.");
			add_message(msg, 0xCC0000FF);
			String_Destroy(msg);
			game_over();
			ui_set_active_screen(screen_show_endgame());
			currentlyInGame = false;			
		
		} else {

			Visibility *vis = (Visibility *)game_object_get_component(go, COMP_VISIBILITY);
			vis->glyph = '%';
			vis->fgColor = 0x990000FF;

			char *msg = String_Create("You killed the %s.", vis->name);
			add_message(msg, 0xff9900FF);
			String_Destroy(msg);

			Position *pos = (Position *)game_object_get_component(go, COMP_POSITION);
			pos->layer = LAYER_GROUND;

			Physical *phys = (Physical *)game_object_get_component(go, COMP_PHYSICAL);
			phys->blocksMovement = false;
			phys->blocksSight = false;

			// Remove the movement component - no more moving!
			game_object_update_component(go, COMP_MOVEMENT, NULL);

			h->ticksUntilRemoval = 5;
		}
	}
}

void health_recover_player_only() {
	Health *h = game_object_get_component(player, COMP_HEALTH);
	if (h->currentHP > 0) {
		h->currentHP += h->recoveryRate;
		if (h->currentHP > h->maxHP) { 
			h->currentHP = h->maxHP;
		}			
	}
}

void health_recover() {
	// Loop through all our health components and apply recovery HP (only if object is not already dead)
	ListElement *e = list_head(healthComps);
	while (e != NULL) {
		Health *h = (Health *)list_data(e);
		if (h->currentHP > 0) {
			h->currentHP += h->recoveryRate;
			if (h->currentHP > h->maxHP) { 
				h->currentHP = h->maxHP;
			}			
		}
		e = list_next(e);
	}
}

void health_removal_update() {
	// Loop through all our health components and remove any objects that have been dead for awhile. 
	// Decrement counters for newly-dead objects
	ListElement *e = list_head(healthComps);
	while (e != NULL) {
		Health *h = (Health *)list_data(e);
		if (h->currentHP <= 0) {
			if (h->ticksUntilRemoval <= 0) {
				// Remove object and all related components from world state
				GameObject goToDestroy = gameObjects[h->objectId];
				e = list_next(e);	// Grab the next element in the list, because we're about to destroy this one
				game_object_destroy(&goToDestroy);

			} else {
				h->ticksUntilRemoval -= 1;
	 			e = list_next(e);
			}

		} else {
			e = list_next(e);
		}
	}
}


/* Combat Routines */

void combat_deal_damage(GameObject *attacker, GameObject *defender) {
	Combat *att = (Combat *)game_object_get_component(attacker, COMP_COMBAT);
	Combat *def = (Combat *)game_object_get_component(defender, COMP_COMBAT);
	Health *defHealth = (Health *)game_object_get_component(defender, COMP_HEALTH);

	i32 totAtt = att->attack + att->attackModifier;
	i32 totDef = def->defense + def->defenseModifier;
	i32 monsterMod = 1 + (currentLevelNumber / 3);
	if (attacker != player) {
		totAtt += monsterMod;
	} else {
		totDef += monsterMod;		
	}

	if (totDef >= totAtt) {
		if (attacker == player) {
			add_message("Your attack didn't do any damage.", 0xCCCCCCFF);
		} else {
			add_message("The creature's pathetic attack didn't do any damage.", 0xCCCCCCFF);
		}

	} else {
		if (attacker == player) {
			Visibility *vis = game_object_get_component(defender, COMP_VISIBILITY);
			char *msg = String_Create("You hit the %s for %d damage.", vis->name, (totAtt - totDef));
			add_message(msg, 0xCCCCCCFF);
			String_Destroy(msg);

		} else {
			Visibility *vis = game_object_get_component(attacker, COMP_VISIBILITY);
			char *msg = String_Create("The %s hits you for %d damage.", vis->name, (totAtt - totDef));
			add_message(msg, 0xCCCCCCFF);
			String_Destroy(msg);
		}


		defHealth->currentHP -= (totAtt - totDef);

		health_check_death(defender);
	}
}

void combat_attack(GameObject *attacker, GameObject *defender) {
	Combat *att = (Combat *)game_object_get_component(attacker, COMP_COMBAT);

	i32 hitRoll = (rand() % 100) + 1;
	i32 hitWindow = (att->toHit + att->toHitModifier);
	if ((hitRoll < hitWindow) || (hitRoll == 100)) {
		// We have a hit
		combat_deal_damage(attacker, defender);
	} else {
		// Missed
		if (attacker == player) {
			add_message("Your attack misses.", 0xCCCCCCFF);

		} else {
			Visibility *vis = game_object_get_component(attacker, COMP_VISIBILITY);
			char *msg = String_Create("The %s misses you.", vis->name);
			add_message(msg, 0xCCCCCCFF);
			String_Destroy(msg);
		}
	}
}



/* Item Management routines */

i32 item_get_weight_carried() {
	i32 totalWeight = 0;

	ListElement *e = list_head(carriedItems);
	while (e != NULL) {
		GameObject *go = (GameObject *)list_data(e);
		Equipment *eq = game_object_get_component(go, COMP_EQUIPMENT);
		totalWeight += eq->weight;
		e = list_next(e);
	}
	
	return totalWeight;
}

void item_get() {
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
	// Get the item at player's current position
	List *objects = game_objects_at_position(playerPos->x, playerPos->y);
	GameObject *itemObj = NULL;
	Equipment *eq = NULL;
	Treasure *t = NULL;
	ListElement *e = list_head(objects);
	while (e != NULL) {
		GameObject *go = (GameObject *)list_data(e);
		eq = (Equipment *)game_object_get_component(go, COMP_EQUIPMENT);
		if (eq != NULL) {
			itemObj = go;
			break;
		}
		t = (Treasure *)game_object_get_component(go, COMP_TREASURE);
		if (t != NULL) {
			itemObj = go;
			break;
		}
		e = list_next(e);
	}

	if (itemObj != NULL && t != NULL) {
		gemsFoundThisLevel += 1;
		gemsFoundTotal += 1;

		Visibility *v = (Visibility *)game_object_get_component(itemObj, COMP_VISIBILITY);
		if (v != NULL) {
			char *msg = String_Create("You picked up the %s. Gems left on level:%d", v->name, GEMS_PER_LEVEL - gemsFoundThisLevel);
			add_message(msg, 0x753aabff);
			String_Destroy(msg);
		}

		// Destroy the gem game object - we don't need it anymore.
		game_object_update_component(itemObj, COMP_POSITION, NULL);		// remove it from position tracking lists
		game_object_destroy(itemObj);
	}

	if (itemObj != NULL && eq != NULL) {
		// Can the player pick it up (are they carrying too much already?)
		i32 carriedWeight = item_get_weight_carried();
		if (carriedWeight + eq->weight <= maxWeightAllowed) {
			// Add the item to the player's carried items
			list_insert_after(carriedItems, NULL, itemObj);

			// Remove the item from the map (take away its Position comp)
			game_object_update_component(itemObj, COMP_POSITION, NULL);

			// Write an appropriate message to the log
			Visibility *v = (Visibility *)game_object_get_component(itemObj, COMP_VISIBILITY);
			if (v != NULL) {
				char *msg = String_Create("You picked up the %s.", v->name);
				add_message(msg, 0x009900ff);
				String_Destroy(msg);
			}

			playerTookTurn = true;

		} else {
			// Too much to carry
			char *msg = String_Create("You are carrying too much already.");
			add_message(msg, 0x990000ff);
			String_Destroy(msg);
		}
	}
}

void item_lifetime_update() {
	ListElement *e = list_head(carriedItems);
	while (e != NULL) {
		GameObject *go = (GameObject *)list_data(e);
		Equipment *eq = game_object_get_component(go, COMP_EQUIPMENT);
		eq->lifetime -= 1;

		// Check to see if the item is aged beyond use
		if (eq->lifetime <= 0) {
			// Unequip the item if equipped
			bool wasEquipped = false;
			if (eq->isEquipped) {
				wasEquipped = true;
				item_toggle_equip(go);
			}

			// Remove from carried items
			list_remove_element_with_data(carriedItems, go);
			
			// Display a message to the player
			Visibility *v = (Visibility *)game_object_get_component(go, COMP_VISIBILITY);
			char *msg;
			if (wasEquipped) {
				msg = String_Create("The %s crumbles in your hands.", v->name);
			} else {
				msg = String_Create("The %s you are carrying crumbles to dust.", v->name);
			}
			add_message(msg, 0x990000ff);
			String_Destroy(msg);
			
			game_object_destroy(go);
		}

		e = list_next(e);
	}
}

void item_toggle_equip(GameObject *item) {
	if (item == NULL) { return; }

	Equipment *eq = (Equipment *)game_object_get_component(item, COMP_EQUIPMENT);
	if (eq != NULL) {
		eq->isEquipped = !eq->isEquipped;
		Combat *playerCombat = (Combat *)game_object_get_component(player, COMP_COMBAT);
		Combat *c = (Combat *)game_object_get_component(item, COMP_COMBAT);
		if (eq->isEquipped) {
			// Apply the effects of equipping that item
			playerCombat->toHitModifier += c->toHitModifier;
			playerCombat->attackModifier += c->attackModifier;
			playerCombat->defenseModifier += c->defenseModifier;

			// Loop through all other carried items and unequip any other item that might have already
			// been equipped in that slot
			ListElement *le = list_head(carriedItems);
			while (le != NULL) {
				if (le->data != item) {
					Equipment *e = (Equipment *)game_object_get_component(le->data, COMP_EQUIPMENT);
					if (strcmp(e->slot, eq->slot) == 0 && e->isEquipped) {
						e->isEquipped = false;
						Combat *cc = (Combat *)game_object_get_component(le->data, COMP_COMBAT);
						// Apply the effects of unequipping that item
						playerCombat->toHitModifier -= cc->toHitModifier;
						playerCombat->attackModifier -= cc->attackModifier;
						playerCombat->defenseModifier -= cc->defenseModifier;
					}
				}
				le = list_next(le);
			}
		} else {
			// Apply the effects of unequipping that item
			playerCombat->toHitModifier -= c->toHitModifier;
			playerCombat->attackModifier -= c->attackModifier;
			playerCombat->defenseModifier -= c->defenseModifier;
		}
	}
}

void item_drop(GameObject *item) {
	if (item == NULL) { return; }

	// Determine position of dropped item
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

	// Check to see if the the item can be dropped
	List *objects = game_objects_at_position(playerPos->x, playerPos->y);
	bool canBeDropped = true;
	ListElement *le = list_head(objects);
	while (le != NULL) {
		Equipment *eq = (Equipment *)game_object_get_component(le->data, COMP_EQUIPMENT);
		if (eq != NULL) {
			canBeDropped = false;
			break;
		}
		le = list_next(le);
	}

	if (canBeDropped) {
		// Drop the item by assigning it a position
		Position pos = {.objectId = item->id, .x = playerPos->x, .y = playerPos->y, .layer = LAYER_MID};
		game_object_update_component(item, COMP_POSITION, &pos);

		// Unequip the item if equipped
		Equipment *eq = (Equipment *)game_object_get_component(item, COMP_EQUIPMENT);
		if (eq->isEquipped) {
			item_toggle_equip(item);
		}

		// Remove from carried items
		list_remove_element_with_data(carriedItems, item);

		// Display a message to the player
		Visibility *v = (Visibility *)game_object_get_component(item, COMP_VISIBILITY);
		char *msg = String_Create("You dropped the %s.", v->name);
		add_message(msg, 0x990000ff);
		String_Destroy(msg);

	} else {
		char *msg = String_Create("Can't drop here.");
		add_message(msg, 0x990000ff);
		String_Destroy(msg);
	}

}


/* Environment Management routines */

void environment_update(Position *playerPos) {
	// Check to see if there are any items at player's current position
	List *objects = game_objects_at_position(playerPos->x, playerPos->y);
	GameObject *itemObj = NULL;
	ListElement *e = list_head(objects);
	while (e != NULL) {
		GameObject *go = (GameObject *)list_data(e);
		Equipment *eqComp = (Equipment *)game_object_get_component(go, COMP_EQUIPMENT);
		if (eqComp != NULL) {
			itemObj = go;
		}
		Treasure *trComp = (Treasure *)game_object_get_component(go, COMP_TREASURE);
		if (trComp != NULL) {
			itemObj = go;
		}
		Visibility *v = (Visibility *)game_object_get_component(go, COMP_VISIBILITY);
		if ((v != NULL) && (String_Equals(v->name, "Stairs"))) {
			char *msg;
			if (currentLevelNumber < 20) {
				msg = String_Create("There are stairs down here. [D]escend?", v->name);
			} else {
				msg = String_Create("There is a glowing portal here. [E]nter?", v->name);
			}
			add_message(msg, 0xffd700ff);
			String_Destroy(msg);
		}
		e = list_next(e);
	}
	if (itemObj != NULL) {
		Visibility *v = (Visibility *)game_object_get_component(itemObj, COMP_VISIBILITY);
		if (v != NULL) {
			char *msg = String_Create("There is a %s here. [G]et it?", v->name);
			add_message(msg, 0x009900ff);
			String_Destroy(msg);
		}
	}
}


/* Animation Management Routines */

void animation_update() {
	// Look at all animations in the list and do any necessary clean up or
	// keyframe work.
	ListElement *e = list_head(animationComps);
	while (e != NULL) {
		Animation *anim = (Animation *)list_data(e);
		if (anim->finished) {
			// Animation is done - clean it up
			ListElement *eToRemove = e;
			e = list_prev(eToRemove);
			GameObject go = gameObjects[anim->objectId];
			game_object_update_component(&go, COMP_ANIMATION, NULL);
			list_remove(animationComps, eToRemove);
		}

		anim->ticksUntilKeyframe -= 1;
		if (anim->ticksUntilKeyframe <= 0) {
			// Time for a keyframe
			anim->ticksUntilKeyframe = anim->keyFrameInterval;
			anim->keyframeAnimation(anim->objectId);
		}

		e = list_next(e);
	}	
}


/* High-level Game Routines */

void
game_new()
{
	// -- Start a brand new game --
	world_state_init();

	// Create our player
	player = game_object_create();
	Visibility vis = {.objectId=player->id, .glyph='@', .fgColor=0x00FF00FF, .bgColor=0x00000000, .hasBeenSeen=true, .name="Player"};
	game_object_update_component(player, COMP_VISIBILITY, &vis);
	Physical phys = {player->id, true, true};
	game_object_update_component(player, COMP_PHYSICAL, &phys);
	Health hlth = {.objectId = player->id, .currentHP = 20, .maxHP = 20, .recoveryRate = 1};
	game_object_update_component(player, COMP_HEALTH, &hlth);
	Combat com = {.objectId = player->id, .toHit=80, .toHitModifier=0, .attack = 5, .defense = 2, .attackModifier = 0, .defenseModifier = 0};
	game_object_update_component(player, COMP_COMBAT, &com);

	playerName = name_create();

	// Create a level and place our player in it
	currentLevelNumber = 1;
	currentLevel = level_init(currentLevelNumber, player);
	Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);

	fov_calculate(playerPos->x, playerPos->y, fovMap);

	generate_target_map(playerPos->x, playerPos->y);
}

void
game_update() 
{
	// Have things move themselves around the dungeon if the player moved
	if (playerTookTurn) {
		Position *playerPos = (Position *)game_object_get_component(player, COMP_POSITION);
		generate_target_map(playerPos->x, playerPos->y);
		movement_update();		
		item_lifetime_update();
		environment_update(playerPos);

		health_removal_update();
	}

	// Recalculate the FOV if warranted
	if (recalculateFOV) {
		Position *pos = (Position *)game_object_get_component(player, COMP_POSITION);
		fov_calculate(pos->x, pos->y, fovMap);
		recalculateFOV = false;
	}

	// Check for animation updates
	animation_update();
}

void
game_over() {
	// Do endgame processing -- 

	// Load the existing HoF data if necessary
	if (hofConfig == NULL) {
		hofConfig = config_file_parse("./config/hof.cfg");
	}

	// Determine if current game makes the HoF
	ListElement *e = list_head(hofConfig->entities);
	bool hofUpdated = false;

	while (e != NULL) {
		ConfigEntity *entity = (ConfigEntity *)e->data;
		char *gems = config_entity_value(entity, "gems");
		i32 gemCount = atoi(gems);

		// If the current game's score is higher than this entry's score, it made the list right here
		if (gemsFoundTotal >= gemCount) {
			// Create a new config entity and load it up with this game's data
			ConfigEntity *newEntity = (ConfigEntity *)calloc(1, sizeof(ConfigEntity));
			newEntity->name = String_Create("%s", "RECORD");
			config_entity_set_value(newEntity, "name", playerName);
			config_entity_set_value(newEntity, "gems", String_Create("%d", gemsFoundTotal));
			config_entity_set_value(newEntity, "level", String_Create("%d", currentLevelNumber));

			time_t rawTime;
			time(&rawTime);
			struct tm *today = localtime(&rawTime);
			config_entity_set_value(newEntity, "date", String_Create("%d/%d/%d", today->tm_mon + 1, today->tm_mday, today->tm_year + 1900));

			// Insert an element containing new entity before the element 
			// with the entity we're comparing to
			ListElement *insertAfterThis = list_prev(e);
			list_insert_after(hofConfig->entities, insertAfterThis, newEntity);
			hofUpdated = true;
			break;
		}

		e = list_next(e);
	}

	// If the current game doesn't rank higher than other games in the HoF, but the
	// HoF contains fewer than 10 entries, add this game to the end.
	if ((!hofUpdated) && (list_size(hofConfig->entities) < 10)) {
		// Create a new config entity and load it up with this game's data
		ConfigEntity *newEntity = (ConfigEntity *)calloc(1, sizeof(ConfigEntity));
		newEntity->name = String_Create("%s", "RECORD");
		config_entity_set_value(newEntity, "name", playerName);
		config_entity_set_value(newEntity, "gems", String_Create("%d", gemsFoundTotal));
		config_entity_set_value(newEntity, "level", String_Create("%d", currentLevelNumber));

		time_t rawTime;
		time(&rawTime);
		struct tm *today = localtime(&rawTime);
		config_entity_set_value(newEntity, "date", String_Create("%d/%d/%d", today->tm_mon + 1, today->tm_mday, today->tm_year + 1900));

		// Insert an element containing new entity before the element 
		// with the entity we're comparing to
		list_insert_after(hofConfig->entities, list_tail(hofConfig->entities), newEntity);
		hofUpdated = true;
	}

	// Check if the HoF contains more than 10 entries. If so, cull the list.
	if (list_size(hofConfig->entities) > 10) {
		ListElement *le = list_item_at(hofConfig->entities, 10);
		if (le != NULL) {
			list_remove(hofConfig->entities, le);
		}
	}

	// If we updated the HoF, write the new data to file
	if (hofUpdated) {
		// Persist HoF to config file
		config_file_write("./config/hof.cfg", hofConfig);
	}
}

void game_quit() {
	gameIsRunning = false;
}




// Animations

void animateGem(u32 gameObjectId) {
	// Gem color will cycle up to "maximum white" and then 
	// back to original purple, giving a shine effect.
	// Gem original color = 0x753aabff
	u32 oRed = 0x75;
	u32 oGreen = 0x3a;
	u32 oBlue = 0xab;

	// Get our game object
	GameObject go = gameObjects[gameObjectId];

	// Get the animation component
	Animation *anim = (Animation *)game_object_get_component(&go, COMP_ANIMATION);

	// Get the visual component
	Visibility *vis = (Visibility *)game_object_get_component(&go, COMP_VISIBILITY);
	
	u32 color = vis->fgColor;
	u32 r = RED(color);
	u32 g = GREEN(color);
	u32 b = BLUE(color);
	u32 a = ALPHA(color);

	if (anim->value1 == 0) {
		// Moving towards white
		r += 10;
		g += 10;
		b += 10;
		if (r >= 255 || g >= 255 || b >= 255) {
			// We've hit "maximum white", so reverse course
			anim->value1 = 1;
			return;
		}

	} else {
		// Moving towards purple
		r -= 10;
		g -= 10;
		b -= 10;
		if (r < oRed || g < oGreen || b < oBlue) {
			// We've hit "original purple", so reverse course
			anim->value1 = 0;
			return;
		}
	}

	vis->fgColor = COLOR_FROM_RGBA(r, g, b, a);
}
