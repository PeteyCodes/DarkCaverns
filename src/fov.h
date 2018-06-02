#ifndef _FOV_H_
#define _FOV_H_


#include "map.h"
#include "typedefs.h"


#define FOV_DISTANCE	10

typedef struct {
	u32 x, y;
} FovCell;

typedef struct {
	float startSlope;
	float endSlope;
} Shadow;


void fov_calculate(u32 heroX, u32 heroY, u32 fovMap[][MAP_HEIGHT]);



#endif // _FOV_H_
