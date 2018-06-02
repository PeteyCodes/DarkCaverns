#ifndef _GAME_H_
#define _GAME_H_


#include "config.h"
#include "ecs.h"
#include "map.h"
#include "typedefs.h"


#define LAYER_UNSET		0
#define LAYER_GROUND	1
#define LAYER_MID		2
#define LAYER_AIR		3
#define LAYER_TOP		4


#define MONSTER_TYPE_COUNT	100
#define ITEM_TYPE_COUNT		100
#define MAX_DUNGEON_LEVEL	20
#define GEMS_PER_LEVEL		5


/* Level Support */

typedef struct {
	i32 level;
	bool (*mapWalls)[MAP_HEIGHT];
} DungeonLevel;


/* Message Log */
typedef struct {
	char *msg;
	u32 fgColor;
} Message;


/* Hall of Fame */
typedef struct {
	char *name;
	i32 level;
	i32 gems;
	char *date;
} HOFRecord;


// Public Game State --
extern bool gameIsRunning;
extern char* playerName;

extern List *carriedItems;
extern i32 maxWeightAllowed;

extern i32 gemsFoundThisLevel;
extern i32 gemsFoundTotal;

extern bool currentlyInGame;
extern	bool recalculateFOV;
extern	bool playerTookTurn;

extern i32 currentLevelNumber;
extern DungeonLevel *currentLevel;
extern u32 fovMap[MAP_WIDTH][MAP_HEIGHT];
extern i32 (*targetMap)[MAP_HEIGHT];
extern Config *monsterConfig;
extern i32 monsterProbability[MONSTER_TYPE_COUNT][MAX_DUNGEON_LEVEL];		// TODO: dynamically size this based on actual count of monsters in config file
extern Config *itemConfig;
extern i32 itemProbability[ITEM_TYPE_COUNT][MAX_DUNGEON_LEVEL];		// TODO: dynamically size this based on actual count of monsters in config file
extern Config *levelConfig;
extern i32 maxMonsters[MAX_DUNGEON_LEVEL];
extern i32 maxItems[MAX_DUNGEON_LEVEL];
extern List *messageLog;
extern Config *hofConfig;



// Public Interface --

void game_new();
void game_update();
void game_over();
void game_quit();

// Movement --
bool can_move(Position pos);
void level_descend();

// Combat --
void combat_attack(GameObject *attacker, GameObject *defender);

// Health --
void health_recover_player_only();

// Item Management --
i32 item_get_weight_carried();
void item_get();
void item_toggle_equip(GameObject *item);
void item_drop(GameObject *item);


#endif // _GAME_H_
