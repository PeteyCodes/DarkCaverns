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


typedef struct {
	u32 x, y;
} Point;

typedef struct {
	Point start;
	Point mid;
	Point end;
	i32 roomFrom;
	i32 roomTo;
	bool hasMidPt;
} Segment;


/* Function Declarations */
void map_carve_hallway_horz(Point from, Point to);
void map_carve_hallway_vert(Point from, Point to);
bool map_carve_room(u32 x, u32 y, u32 w, u32 h);
void map_carve_segments(List *hallways);
void map_get_segments(List *segments, Point from, Point to, PT_Rect *rooms, u32 roomCount);
Point rect_random_point(PT_Rect rect);
i32 room_containing_point(Point pt, PT_Rect *rooms, u32 roomCount);



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
		u32 x = rand() % (MAP_WIDTH - w - 1);
		u32 y = rand() % (MAP_HEIGHT - h - 1);
		if (x == 0) x = 1;
		if (y == 0) y = 1;

		bool success = map_carve_room(x, y, w, h);
		if (success) {
			PT_Rect r = {x, y, w, h};
			rooms[roomCount] = r;
			roomCount += 1;
			cellsUsed += (w * h);
		}

		// Exit condition - more that desired % of cells in use
		if (((float)cellsUsed / (float)(MAP_HEIGHT * MAP_WIDTH)) > 0.45) {
			roomsDone = true;
		}
	}

	// Join all rooms with corridors, so that all rooms are reachable
	List *hallways = list_init(free);

	for (u32 r = 1; r < roomCount; r++) {
		// Join two rooms via random points in those rooms
		Point fromPt = rect_random_point(rooms[r-1]);
		Point toPt = rect_random_point(rooms[r]);

		List *segments = list_init(free);

		// Break the proposed hallway into segments joining rooms
		map_get_segments(segments, fromPt, toPt, rooms, roomCount);

		// Walk the segment list and eliminate any segments
		// that join rooms that are already joined 
		for (ListElement *e = list_head(segments); e != NULL; e = e->next) { 
			i32 rm1 = ((Segment *)(e->data))->roomFrom;
			i32 rm2 = ((Segment *)(e->data))->roomTo;

			Segment *uSeg = NULL;
			if (hallways->size == 0) {
				uSeg = (Segment *)e->data;
				
			} else {
				bool unique = true;
				for (ListElement *h = list_head(hallways); h != NULL; h = h->next) {
					Segment *seg = (Segment *)(h->data);
					if (((seg->roomFrom == rm1) && (seg->roomTo == rm2)) || 
						((seg->roomTo == rm1) && (seg->roomFrom == rm2))) {
						unique = false;
						break;
					}
				}
				if (unique) {
					uSeg = (Segment *)e->data;
				}
			}

			if (uSeg != NULL) {
				Segment *segCopy = malloc(sizeof(Segment));
				memcpy(segCopy, uSeg, sizeof(Segment));
				list_insert_after(hallways, NULL, segCopy);
			}
		}

		// Clean up
		list_destroy(segments);
	}

	// Carve out unique hallways
	map_carve_segments(hallways);

	// Clean up
	list_destroy(hallways);
}

void map_carve_hallway_horz(Point from, Point to) {
	u32 first, last;
	if (from.x < to.x) {
		first = from.x;
		last = to.x;
	} else {
		first = to.x;
		last = from.x;
	}

	for (u32 x = first; x <= last; x ++) {
		mapCells[x][from.y] = false;
	}
}

void map_carve_hallway_vert(Point from, Point to) {
	u32 first, last;
	if (from.y < to.y) {
		first = from.y;
		last = to.y;
	} else {
		first = to.y;
		last = from.y;
	}

	for (u32 y = first; y <= last; y++) {
		mapCells[from.x][y] = false;
	}
}

bool map_carve_room(u32 x, u32 y, u32 w, u32 h) {
	// Determine if all the cells within the given rectangle are filled
	for (u8 i = x-1; i < x + (w + 1); i++) {
		for (u8 j = y-1; j < y + (h + 1); j++) {
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

void map_carve_segments(List *hallways) {
	ListElement *e = list_head(hallways);
	while (e != NULL) {
		Segment *seg = ((Segment *)(e->data));
		if (seg->hasMidPt) {
			// This segment turns midway, so draw both parts of the segment
			Point p1 = seg->start;
			Point p2 = seg->mid;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2);			
			} else {
				map_carve_hallway_horz(p1, p2);
			}

			p1 = seg->mid;
			p2 = seg->end;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2);			
			} else {
				map_carve_hallway_horz(p1, p2);
			}

		} else {
			Point p1 = seg->start;
			Point p2 = seg->end;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2);			
			} else {
				map_carve_hallway_horz(p1, p2);
			}
		}

		e = e->next;
	}
}

void map_get_segments(List *segments, Point from, Point to, PT_Rect *rooms, u32 roomCount) {
	// Walk between our two points and find all the spans between rooms

	// Determine a mid point, by moving first either horizontally or vertically
	Point mid;
	if (rand() % 2 == 0) {
		// Move horizontal, then vertical
		mid.x = to.x;
		mid.y = from.y;
	} else {
		// Move vertical, then horizontal
		mid.x = from.x;
		mid.y = to.y;
	}

	Point curr = from;
	bool isHorz = false;
	i8 step = 1;
	if (from.y == mid.y) { 
		isHorz = true;
		if (from.x > mid.x) { step = -1; }
	} else {
		if (from.y > mid.y) { step = -1; }
	}

	i8 currRoom = room_containing_point(curr, rooms, roomCount);
	Point lastPoint = from;
	bool done = false;
	Segment *turnSegment = NULL;
	while (!done) {
		i32 rm = room_containing_point(curr, rooms, roomCount);
		if (curr.x == mid.x && curr.y == mid.y) {
			// Check to see if we're in a room
			if (rm != -1) {
				if (rm != currRoom) {
					// We have a new segment between currRoom and rm
					Segment *s = malloc(sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasMidPt = false;
					list_insert_after(segments, NULL, s);
				}

			} else {
				// We hit our midpoint and we're outside a room - record a partial segment
				turnSegment = malloc(sizeof(Segment));
				turnSegment->start = lastPoint;
				turnSegment->mid = curr;
				turnSegment->hasMidPt = true;
				turnSegment->roomFrom = currRoom;
			}

			// Set a new "from" and change our step to reflect our new direction
			from = curr;
			isHorz = false;
			step = 1;
			if (from.y == to.y) { 
				isHorz = true;
				if (from.x > to.x) { step = -1; } 
			} else {
				if (from.y > to.y) { step = -1; }
			}

			// Move to next cell
			if (isHorz) {
				curr.x += step;
			} else {
				curr.y += step;
			}

		} else if (curr.x == to.x && curr.y == to.y) { 
			// We hit our endpoint - check if we're in another room or outside all rooms
			// and if so, add another segment
			if (rm != currRoom) {
				if (turnSegment != NULL) {
					// Complete our partial segment
					turnSegment->end = curr;
					turnSegment->roomTo = rm;
					list_insert_after(segments, NULL, turnSegment);
					turnSegment = NULL;

				} else {
					// We have a new segment between currRoom and rm
					Segment *s = malloc(sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasMidPt = false;
					list_insert_after(segments, NULL, s);
				}
			}
			done = true; 

		} else {
			if (rm != -1 && rm != currRoom) {
				if (turnSegment != NULL) {
					// Complete our partial segment
					turnSegment->end = curr;
					turnSegment->roomTo = rm;
					list_insert_after(segments, NULL, turnSegment);
					turnSegment = NULL;
					
				} else {
					// We have a new segment between currRoom and rm
					Segment *s = malloc(sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasMidPt = false;
					list_insert_after(segments, NULL, s);
				}

				currRoom = rm;
				lastPoint = curr;
			}

			// Move to next cell
			if (isHorz) {
				curr.x += step;
			} else {
				curr.y += step;
			}
		}
	}
}

Point rect_random_point(PT_Rect rect) {
	u32 px = (rand() % rect.w) + rect.x;
	u32 py = (rand() % rect.h) + rect.y;
	Point ret = {px, py};
	return ret;
}

i32 room_containing_point(Point pt, PT_Rect *rooms, u32 roomCount) {
	for (i8 i = 0; i < roomCount; i++) {
		if ((rooms[i].x <= pt.x) && ((rooms[i].x + rooms[i].w) > pt.x) &&
			(rooms[i].y <= pt.y) && ((rooms[i].y + rooms[i].w) > pt.y)) {
			return i;
		}
	}
	return -1;
}

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
