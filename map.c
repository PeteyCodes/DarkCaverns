/*
* map.c
*
* Functions and Types for map generation.
*/

#define MAP_WIDTH	80
#define MAP_HEIGHT	40


typedef struct {
	i32 x, y;
} Point;

typedef struct {
	Point start;
	Point mid;
	Point end;
	i32 roomFrom;
	i32 roomTo;
	bool hasWaypoint;
} Segment;


/* Function Declarations */
void map_carve_hallway_horz(Point from, Point to, bool (*mapCells)[MAP_HEIGHT]);
void map_carve_hallway_vert(Point from, Point to, bool (*mapCells)[MAP_HEIGHT]);
bool map_carve_room(u32 x, u32 y, u32 w, u32 h, bool (*mapCells)[MAP_HEIGHT]);
void map_carve_segments(List *hallways, bool (*mapCells)[MAP_HEIGHT]);
void map_get_segments(List *segments, Point from, Point to, UIRect *rooms, u32 roomCount);
Point rect_random_point(UIRect rect);
i32 room_containing_point(Point pt, UIRect *rooms, i32 roomCount);


/* Map Management */

void map_generate(bool (*mapCells)[MAP_HEIGHT]) {
	// Mark all the map cells as "filled"
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			mapCells[x][y] = true;
		}
	}

	// Carve out non-overlapping rooms that are randomly placed, and of 
	// random size. 
	bool roomsDone = false;
	UIRect rooms[100];
	u32 cellsUsed = 0;
	u32 roomCount = 0;
	while (!roomsDone) {
		// Generate a random width/height for a room
		u32 w = (rand() % 17) + 5;
		u32 h = (rand() % 17) + 5;
		u32 x = rand() % (MAP_WIDTH - w - 1);
		u32 y = rand() % (MAP_HEIGHT - h - 1);
		if (x == 0) x = 1;
		if (y == 0) y = 1;

		bool success = map_carve_room(x, y, w, h, mapCells);
		if (success) {
			UIRect r = {x, y, w, h};
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
	List *hallways = list_new(free);

	for (u32 r = 1; r < roomCount; r++) {
		// Join two rooms via random points in those rooms
		Point fromPt = rect_random_point(rooms[r-1]);
		Point toPt = rect_random_point(rooms[r]);

		List *segments = list_new(free);

		// Break the proposed hallway into segments joining rooms
		map_get_segments(segments, fromPt, toPt, rooms, roomCount);

		// Walk the segment list and skip adding any segments
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
				Segment *segCopy = calloc(1, sizeof(Segment));
				memcpy(segCopy, uSeg, sizeof(Segment));
				list_insert_after(hallways, NULL, segCopy);
			}
		}

		// Clean up
		list_destroy(segments);
	}

	// Carve out unique hallways
	map_carve_segments(hallways, mapCells);

	// Clean up
	list_destroy(hallways);
}

void map_carve_hallway_horz(Point from, Point to, bool (*mapCells)[MAP_HEIGHT]) {
	u32 first, last;
	if (from.x < to.x) {
		first = from.x;
		last = to.x;
	} else {
		first = to.x;
		last = from.x;
	}

	for (u32 x = first; x <= last; x++) {
		mapCells[x][from.y] = false;
	}
}

void map_carve_hallway_vert(Point from, Point to, bool (*mapCells)[MAP_HEIGHT]) {
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

bool map_carve_room(u32 x, u32 y, u32 w, u32 h, bool (*mapCells)[MAP_HEIGHT]) {
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

void map_carve_segments(List *hallways, bool (*mapCells)[MAP_HEIGHT]) {

	for (ListElement *e = list_head(hallways); e != NULL; e = e->next) {
		Segment *seg = (Segment *)e->data;

		if (seg->hasWaypoint) {
			// This segment turns midway, so draw both parts of the segment
			Point p1 = seg->start;
			Point p2 = seg->mid;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2, mapCells);			
			} else {
				map_carve_hallway_horz(p1, p2, mapCells);
			}

			p1 = seg->mid;
			p2 = seg->end;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2, mapCells);			
			} else {
				map_carve_hallway_horz(p1, p2, mapCells);
			}

		} else {
			Point p1 = seg->start;
			Point p2 = seg->end;

			if (p1.x == p2.x) {
				map_carve_hallway_vert(p1, p2, mapCells);			
			} else {
				map_carve_hallway_horz(p1, p2, mapCells);
			}
		}
	}

}

void map_get_segments(List *segments, Point from, Point to, UIRect *rooms, u32 roomCount) {
	// Walk between our two points and find all the spans between rooms
	bool usingWaypoint = false;
	Point wayPoint = to;
	if (from.x != to.x && from.y != to.y) {
		// Need to use a two-part segment to get between points
		// Determine a waypoint where we'll turn
		usingWaypoint = true;
		if (rand() % 2 == 0) {
			// Move horizontal, then vertical
			wayPoint.x = to.x;
			wayPoint.y = from.y;
		} else {
			// Move vertical, then horizontal
			wayPoint.x = from.x;
			wayPoint.y = to.y;
		}
	}

	Point curr = from;
	bool isHorz = false;
	i8 step = 1;
	if (from.y == wayPoint.y) { 
		isHorz = true;
		if (from.x > wayPoint.x) { step = -1; }
	} else {
		if (from.y > wayPoint.y) { step = -1; }
	}

	i8 currRoom = room_containing_point(curr, rooms, roomCount);
	Point lastPoint = from;
	bool done = false;
	Segment *turnSegment = NULL;
	while (!done) {
		i32 rm = room_containing_point(curr, rooms, roomCount);
		if (usingWaypoint && curr.x == wayPoint.x && curr.y == wayPoint.y) {
			// Check to see if we're in a room
			if (rm != -1) {
				if (rm != currRoom) {
					// We have a new segment between currRoom and rm
					Segment *s = calloc(1, sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasWaypoint = false;
					list_insert_after(segments, NULL, s);

					currRoom = rm;
				} else {
					// We haven't left our starting room yet, so change our lastPoint
					// to be our waypoint, so we're just drawing a single part segment.
					lastPoint = wayPoint;
				}

			} else {
				// We hit our midpoint and we're outside a room - record a partial segment
				turnSegment = calloc(1, sizeof(Segment));
				turnSegment->start = lastPoint;
				turnSegment->mid = curr;
				turnSegment->hasWaypoint = true;
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
			// We hit our endpoint - check if we're in another room or still in the same room
			if (rm != currRoom) {
				if (turnSegment != NULL) {
					// We already have a partial segment, so complete it
					turnSegment->end = curr;
					turnSegment->roomTo = rm;
					list_insert_after(segments, NULL, turnSegment);
					turnSegment = NULL;

				} else {
					// We have a new segment between currRoom and rm
					Segment *s = calloc(1, sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasWaypoint = false;
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
					Segment *s = calloc(1, sizeof(Segment));
					s->start = lastPoint;
					s->end = curr;
					s->roomFrom = currRoom;
					s->roomTo = rm;
					s->hasWaypoint = false;
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

Point rect_random_point(UIRect rect) {
	u32 px = (rand() % (rect.w - 1)) + rect.x;
	u32 py = (rand() % (rect.h - 1)) + rect.y;
	Point ret = {px, py};
	return ret;
}

i32 room_containing_point(Point pt, UIRect *rooms, i32 roomCount) {
	for (i32 i = 0; i < roomCount; i++) {
		if ((rooms[i].x <= pt.x) && ((rooms[i].x + rooms[i].w) > pt.x) &&
			(rooms[i].y <= pt.y) && ((rooms[i].y + rooms[i].h) > pt.y)) {
			return i;
		}
	}
	return -1;
}
