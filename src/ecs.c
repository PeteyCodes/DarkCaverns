/*
    Entity-Component System implementation for Dark Caverns
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ecs.h"

// Entity & Component data

GameObject *player = NULL;
GameObject gameObjects[MAX_GO];
List *positionComps;
List *visibilityComps;
List *physicalComps;
List *movementComps;
List *healthComps;
List *combatComps;
List *equipmentComps;
List *treasureComps;
List *animationComps;
List *goPositions[MAP_WIDTH][MAP_HEIGHT];


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
			if (compData != NULL) {
				Position *pos = obj->components[COMP_POSITION];
				bool addedNew = false;
				if (pos == NULL) {
					pos = (Position *)calloc(1, sizeof(Position));
					addedNew = true;
				} else {
					// Remove game obj from the position helper DS
					List *ls = goPositions[pos->x][pos->y];
					list_remove_element_with_data(ls, obj);
				}
				Position *posData = (Position *)compData;
				pos->objectId = obj->id;
				pos->x = posData->x;
				pos->y = posData->y;
				pos->layer = posData->layer;

				// Only insert into world component list if we just allocated a new component
				if (addedNew) {
					list_insert_after(positionComps, NULL, pos);					
				}
	
				obj->components[comp] = pos;

				// Update our helper DS 
				List *gos = goPositions[posData->x][posData->y];
				if (gos == NULL) {
					gos = list_new(free);
					goPositions[posData->x][posData->y] = gos;
				}
				list_insert_after(gos, NULL, obj);

			} else {
				// Clear component 
				Position *pos = obj->components[COMP_POSITION];
				if (pos != NULL) {
					list_remove_element_with_data(positionComps, pos);	
				}
				obj->components[comp] = NULL;

				// Remove game obj from the position helper DS
				List *ls = goPositions[pos->x][pos->y];
				list_remove_element_with_data(ls, obj);
			}

			break;
		}

		case COMP_VISIBILITY: {
			if (compData != NULL) {
				Visibility *vis = obj->components[COMP_VISIBILITY];
				bool addedNew = false;
				if (vis == NULL)  {
					vis = (Visibility *)calloc(1, sizeof(Visibility));
					addedNew = true;
				}
				Visibility *visData = (Visibility *)compData;
				vis->objectId = obj->id;
				vis->glyph = visData->glyph;
				vis->fgColor = visData->fgColor;
				vis->bgColor = visData->bgColor;
				vis->hasBeenSeen = visData->hasBeenSeen;
				vis->visibleOutsideFOV = visData->visibleOutsideFOV;
				if (visData->name != NULL) {
					vis->name = calloc(strlen(visData->name) + 1, sizeof(char));
					strcpy(vis->name, visData->name);
				}

				if (addedNew) {
					list_insert_after(visibilityComps, NULL, vis);					
				}
	
				obj->components[comp] = vis;

			} else {
				// Clear component 
				Visibility *vis = obj->components[COMP_VISIBILITY];
				if (vis != NULL) {
					list_remove_element_with_data(visibilityComps, vis);				
				}
				obj->components[comp] = NULL;
			}

			break;
		}

		case COMP_PHYSICAL: {
			if (compData != NULL) {
				Physical *phys = obj->components[COMP_PHYSICAL];
				bool addedNew = false;
				if (phys == NULL) {
					phys = (Physical *)calloc(1, sizeof(Physical));
					addedNew = true;
				}
				Physical *physData = (Physical *)compData;
				phys->objectId = obj->id;
				phys->blocksSight = physData->blocksSight;
				phys->blocksMovement = physData->blocksMovement;

				if (addedNew) {
					list_insert_after(physicalComps, NULL, phys);					
				}
				obj->components[comp] = phys;

			} else {
				// Clear component 
				Physical *phys = obj->components[COMP_PHYSICAL];
				if (phys != NULL) {
					list_remove_element_with_data(physicalComps, phys);				
				}
				obj->components[comp] = NULL;
			}

			break;
		}

		case COMP_MOVEMENT: {
			if (compData != NULL) {
				Movement *mv = obj->components[COMP_MOVEMENT];
				bool addedNew = false;
				if (mv == NULL) {
					mv = (Movement *)calloc(1, sizeof(Movement));
					addedNew = true;
				}
				Movement *mvData = (Movement *)compData;
				mv->objectId = obj->id;
				mv->speed = mvData->speed;
				mv->frequency = mvData->frequency;
				mv->ticksUntilNextMove = mvData->ticksUntilNextMove;
				mv->chasingPlayer = mvData->chasingPlayer;
				mv->turnsSincePlayerSeen = mvData->turnsSincePlayerSeen;

				if (addedNew) {
					list_insert_after(movementComps, NULL, mv);				
				}
				obj->components[comp] = mv;

			} else {
				// Clear component 
				Movement *mv = obj->components[COMP_MOVEMENT];
				if (mv != NULL) {
					list_remove_element_with_data(movementComps, mv);				
				}
				obj->components[comp] = NULL;				
			}

			break;
		}

		case COMP_HEALTH: {
			if (compData != NULL) {
				Health *hlth = obj->components[COMP_HEALTH];
				bool addedNew = false;
				if (hlth == NULL) {
					hlth = (Health *)calloc(1, sizeof(Health));
					addedNew = true;
				}
				Health *hlthData = (Health *)compData;
				hlth->objectId = obj->id;
				hlth->currentHP = hlthData->currentHP;
				hlth->maxHP = hlthData->maxHP;
				hlth->recoveryRate = hlthData->recoveryRate;
				hlth->ticksUntilRemoval = hlthData->ticksUntilRemoval;

				if (addedNew) {
					list_insert_after(healthComps, NULL, hlth);				
				}
				obj->components[comp] = hlth;

			} else {
				// Clear component 
				Health *h = obj->components[COMP_HEALTH];
				if (h != NULL) {
					list_remove_element_with_data(healthComps, h);				
				}
				obj->components[comp] = NULL;				
			}

			break;
		}

		case COMP_COMBAT: {
			if (compData != NULL) {
				Combat *com = obj->components[COMP_COMBAT];
				bool addedNew = false;
				if (com == NULL) {
					com = (Combat *)calloc(1, sizeof(Combat));
					addedNew = true;
				}
				Combat *combatData = (Combat *)compData;
				com->objectId = obj->id;
				com->toHit = combatData->toHit;
				com->toHitModifier = combatData->toHitModifier;
				com->attack = combatData->attack;
				com->defense = combatData->defense;
				com->attackModifier = combatData->attackModifier;
				com->defenseModifier = combatData->defenseModifier;

				if (addedNew) {
					list_insert_after(combatComps, NULL, com);				
				}
				obj->components[comp] = com;
				
			} else {
				// Clear component 
				Combat *c = obj->components[COMP_COMBAT];
				if (c != NULL) {
					list_remove_element_with_data(combatComps, c);				
				}
				obj->components[comp] = NULL;				
			}

			break;
		}

		case COMP_EQUIPMENT: {
			if (compData != NULL) {
				Equipment *equip = obj->components[COMP_EQUIPMENT];
				bool addedNew = false;
				if (equip == NULL) {
					equip = (Equipment *)calloc(1, sizeof(Equipment));
					addedNew = true;
				}

				Equipment *equipData = (Equipment *)compData;
				equip->objectId = obj->id;
				equip->quantity = equipData->quantity;
				equip->weight = equipData->weight;
				equip->lifetime = equipData->lifetime;
				if (equipData->slot != NULL) {
					equip->slot = calloc(strlen(equipData->slot) + 1, sizeof(char));
					strcpy(equip->slot, equipData->slot);
				}
				equip->isEquipped = equipData->isEquipped;

				if (addedNew) {
					list_insert_after(equipmentComps, NULL, equip);				
				}
				obj->components[comp] = equip;
				
			} else {
				// Clear component 
				Equipment *e = obj->components[COMP_EQUIPMENT];
				if (e != NULL) {
					list_remove_element_with_data(equipmentComps, e);				
				}
				obj->components[comp] = NULL;				
			}

			break;
		}

		case COMP_TREASURE: {
			if (compData != NULL) {
				Treasure *treas = obj->components[COMP_TREASURE];
				bool addedNew = false;
				if (treas == NULL) {
					treas = (Treasure *)calloc(1, sizeof(Treasure));
					addedNew = true;
				}

				Treasure *treasData = (Treasure *)compData;
				treas->objectId = obj->id;
				treas->value = treasData->value;

				if (addedNew) {
					list_insert_after(treasureComps, NULL, treas);				
				}
				obj->components[comp] = treas;
				
			} else {
				// Clear component 
				Treasure *t = obj->components[COMP_TREASURE];
				if (t != NULL) {
					list_remove_element_with_data(treasureComps, t);				
				}
				obj->components[comp] = NULL;				
			}

			break;
		}

		case COMP_ANIMATION: {
			if (compData != NULL) {
				Animation *anim = obj->components[COMP_ANIMATION];
				bool addedNew = false;
				if (anim == NULL) {
					anim = (Animation *)calloc(1, sizeof(Animation));
					addedNew = true;
				}

				Animation *animData = (Animation *)compData;
				anim->objectId = obj->id;
				anim->keyFrameInterval = animData->keyFrameInterval;
				anim->ticksUntilKeyframe = animData->ticksUntilKeyframe;
				anim->finished = animData->finished;
				anim->keyframeAnimation = animData->keyframeAnimation;
				anim->value1 = animData->value1;

				if (addedNew) {
					list_insert_after(animationComps, NULL, anim);				
				}
				obj->components[comp] = anim;
				
			} else {
				// Clear component 
				Animation *a = obj->components[COMP_ANIMATION];
				if (a != NULL) {
					list_remove_element_with_data(animationComps, a);				
				}
				obj->components[comp] = NULL;				
			}

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

	elementToRemove = list_search(healthComps, obj->components[COMP_HEALTH]);
	if (elementToRemove != NULL ) { list_remove(healthComps, elementToRemove); }

	elementToRemove = list_search(combatComps, obj->components[COMP_COMBAT]);
	if (elementToRemove != NULL ) { list_remove(combatComps, elementToRemove); }

	elementToRemove = list_search(equipmentComps, obj->components[COMP_EQUIPMENT]);
	if (elementToRemove != NULL ) { list_remove(equipmentComps, elementToRemove); }

	elementToRemove = list_search(treasureComps, obj->components[COMP_TREASURE]);
	if (elementToRemove != NULL ) { list_remove(treasureComps, elementToRemove); }

	elementToRemove = list_search(animationComps, obj->components[COMP_ANIMATION]);
	if (elementToRemove != NULL ) { list_remove(animationComps, elementToRemove); }

	// TODO: Clean up other components used by this object

	obj->id = UNUSED;
	for (i32 i = 0; i < COMPONENT_COUNT; i++) {
		obj->components[i] = NULL;
	}
}


void *game_object_get_component(GameObject *obj, 
								GameComponentType comp) {
	return obj->components[comp];
}

List *game_objects_at_position(u32 x, u32 y) {
	return goPositions[x][y];
}
