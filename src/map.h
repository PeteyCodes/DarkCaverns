#ifndef _MAP_H_
#define _MAP_H_

#include "typedefs.h"


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


void map_generate(bool (*mapCells)[MAP_HEIGHT]);


#endif // _MAP_H_
