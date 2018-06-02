#ifndef _ECS_H_
#define _ECS_H_

#include "list.h"
#include "map.h"
#include "typedefs.h"


typedef enum {
	COMP_POSITION = 0,
	COMP_VISIBILITY,
	COMP_PHYSICAL,
	COMP_HEALTH,
	COMP_MOVEMENT,
	COMP_COMBAT,
	COMP_EQUIPMENT,
	COMP_TREASURE,
	COMP_ANIMATION, 

	/* Define other components above here */
	COMPONENT_COUNT
} GameComponentType;

// Unused entities are given this ID
#define UNUSED	-1

// Maximum number of entities
#define MAX_GO 	10000

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
	char *name;
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

typedef struct {
	i32 objectId;
	i32 currentHP;
	i32 maxHP;
	i32 recoveryRate;		// HP recovered per tick.
	i32 ticksUntilRemoval;	// Countdown to removal from world state
} Health;

typedef struct {
	i32 objectId;
	i32 toHit;				// chance to hit
	i32 toHitModifier;
	i32 attack;				// attack = damage inflicted per hit
	i32 attackModifier;		// based on weapons/items
	i32 defense;			// defense = damage absorbed before HP is affected
	i32 defenseModifier;	// based on armor/items
} Combat;

typedef struct {
	i32 objectId;
	i32 quantity;
	i32 weight;
	i32 lifetime;			// turns until equipment degrades beyond use.
	char *slot;
	bool isEquipped;
} Equipment;

typedef struct {
	i32 objectId;
	i32 value;
} Treasure;

typedef struct {
	i32 objectId;
	i32 keyFrameInterval;
	i32 ticksUntilKeyframe;
	bool finished;
	void (*keyframeAnimation)(u32);
	u32 value1;
} Animation;


// Public ECS Runtime data -- 
extern GameObject *player;

extern GameObject gameObjects[MAX_GO];
extern List *positionComps;
extern List *visibilityComps;
extern List *physicalComps;
extern List *movementComps;
extern List *healthComps;
extern List *combatComps;
extern List *equipmentComps;
extern List *treasureComps;
extern List *animationComps;

extern List *goPositions[MAP_WIDTH][MAP_HEIGHT];


// Public Interface -- 

GameObject *game_object_create();

void game_object_update_component(GameObject *obj, 
							  GameComponentType comp,
							  void *compData);

void game_object_destroy(GameObject *obj);

void *game_object_get_component(GameObject *obj, 
								GameComponentType comp);

List *game_objects_at_position(u32 x, u32 y);

#endif // _ECS_H_
