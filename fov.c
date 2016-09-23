/*
* fov.c
*/

#define FOV_DISTANCE	5

global_variable u32 fovMap[MAP_WIDTH][MAP_HEIGHT];

void fov_calculate(u32 x, u32 y, u32 fovMap[][MAP_HEIGHT]) {

	// Reset FOV to default state (hidden)
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			fovMap[x][y] = 0;
		}
	}

	// Cast visibility out in four directions
	// Determine our visibility rectangle
	u32 x1 = 0;
	if (x >= FOV_DISTANCE) { x1 = x - FOV_DISTANCE; }

	u32 x2 = x + FOV_DISTANCE;
	if (x2 >= MAP_WIDTH) { x2 = MAP_WIDTH - 1; }

	u32 y1 = 0;
	if (y >= FOV_DISTANCE) { y1 = y - FOV_DISTANCE; }
	
	u32 y2 = y + FOV_DISTANCE;
	if (y2 >= MAP_HEIGHT) { y2 = MAP_HEIGHT - 1; }

	// Apply visibility to FOV map
	for (u32 fx = x1; fx <= x2; fx++) {
		for (u32 fy = y1; fy <= y2; fy++) {
			fovMap[fx][fy] = 10;
		}
	}

}