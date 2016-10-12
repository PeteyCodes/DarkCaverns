/*
* fov.c
*/

#define FOV_DISTANCE	10

typedef struct {
	u32 x, y;
} FovCell;

typedef struct {
	float startSlope;
	float endSlope;
} Shadow;


void add_shadow(Shadow s);
bool cell_blocks_sight(u32 x, u32 y);
bool cell_in_shadow(float cellSlope);
float fov_distance_between(u32 x1, u32 y1, u32 x2, u32 y2);
float line_slope_between(float x1, float y1, float x2, float y2);
FovCell map_cell_for_local_cell(u8 sector, FovCell heroMapCell, FovCell cellToTranslate);

global_variable u32 fovMap[MAP_WIDTH][MAP_HEIGHT];

internal Shadow knownShadows[10];
internal u8 shadowCount = 0;


void fov_calculate(u32 heroX, u32 heroY, u32 fovMap[][MAP_HEIGHT]) {

	// Reset FOV to default state (hidden)
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			fovMap[x][y] = 0;
		}
	}

	// Mark hero cell visible
	fovMap[heroX][heroY] = 1;

	// Loop through all 8 sectors around the player
	for (u8 sector = 1; sector <= 8; sector++) {
		bool prev_blocking = false;
		shadowCount = 0;
		float shadowStart = 0.0;
		float shadowEnd = 0.0;
		// For each distance from 1 to FOV range
		for (int cellY = 1; cellY < FOV_DISTANCE; cellY++) {
			prev_blocking = false;
			// For each cell in the span
			for (int cellX = 0; cellX <= cellY; cellX++) {
				// Translate cellX, cellY to map coordinates
				FovCell heroCell = {heroX, heroY};
				FovCell cellToTranslate = {cellX, cellY};
				FovCell mapCell = map_cell_for_local_cell(sector, heroCell, cellToTranslate);

				// Is cell within map?
				if ((mapCell.x >= 0) && (mapCell.y >= 0) && (mapCell.x < MAP_WIDTH) && (mapCell.y < MAP_HEIGHT)) {
					// Is cell within view distance?
					if (fov_distance_between(0, 0, cellX, cellY) <= FOV_DISTANCE) {
						// Is cell within known shadow?
						float cellSlope = line_slope_between(0, 0, cellX, cellY);
						if (!cell_in_shadow(cellSlope)) {
							// No - Mark as visible
							fovMap[mapCell.x][mapCell.y] = 1;
							// Is cell blocking?
							if (cell_blocks_sight(mapCell.x, mapCell.y)) {
								// Was the last cell blocking?
								if (prev_blocking == false) {
									// No - calc start of a new shadow
									shadowStart = line_slope_between(0, 0, cellX, cellY);
									prev_blocking = true;
								}
							} else {
								// Was the last cell blocking?
								if (prev_blocking) {
									// Calc end slope of shadow.
									shadowEnd = line_slope_between(0, 0, cellX+0.5, cellY);
									// Add to shadow list 
									Shadow s = {shadowStart, shadowEnd};
									add_shadow(s);
								}
							}
						}
					}
				}
			}
			// Do we have an open shadow
			if (prev_blocking) {
				// If so, calc end and add shadow to list before moving to next span
				shadowEnd = line_slope_between(0, 0, cellY+0.5, cellY);
				// Add to shadow list 
				Shadow s = {shadowStart, shadowEnd};
				add_shadow(s);
			}
		}
	}

}

void add_shadow(Shadow s) {
	knownShadows[shadowCount] = s;
	shadowCount += 1;
}

bool cell_blocks_sight(u32 x, u32 y) {
	GameObject *go = game_object_at_position(x, y);
	if (go != NULL) {
		Physical *phys = (Physical *)game_object_get_component(go, COMP_PHYSICAL);
		if (phys->blocksSight) {
			return true;
		}
	}
	return false;
}

bool cell_in_shadow(float cellSlope) {
	for (u8 i = 0; i < shadowCount; i++) {
		Shadow s = knownShadows[i];
		if (s.startSlope <= cellSlope && s.endSlope >= cellSlope) {
			return true;
		}
	}
	return false;
}

float fov_distance_between(u32 x1, u32 y1, u32 x2, u32 y2) {
	return sqrt(pow(x2-x1, 2) + pow(y2-y1, 2));
}

float line_slope_between(float x1, float y1, float x2, float y2) {
	// We're actually calculating the inverse slope here, to yield a value
	// between 0 and 1.
	if (x2 - x1 <= 0) { 
		return 0;
	} else {
		return (x2 - x1) / (y2 - y1);	
	}
}

// Return the map cell corresponding to the given local sector coordinates
FovCell map_cell_for_local_cell(u8 sector, FovCell heroMapCell, FovCell cellToTranslate) {
	switch (sector) {
		case 1: {
			FovCell mapCell = {heroMapCell.x + cellToTranslate.x, heroMapCell.y - cellToTranslate.y};
			return mapCell;
		}
		break;
		case 2: {
			FovCell mapCell = {heroMapCell.x + cellToTranslate.y, heroMapCell.y - cellToTranslate.x};
			return mapCell;
		}
		break;
		case 3: {
			FovCell mapCell = {heroMapCell.x + cellToTranslate.y, heroMapCell.y + cellToTranslate.x};
			return mapCell;
		}
		break;
		case 4: {
			FovCell mapCell = {heroMapCell.x + cellToTranslate.x, heroMapCell.y + cellToTranslate.y};
			return mapCell;
		}
		break;
		case 5: {
			FovCell mapCell = {heroMapCell.x - cellToTranslate.x, heroMapCell.y + cellToTranslate.y};
			return mapCell;
		}
		break;
		case 6: {
			FovCell mapCell = {heroMapCell.x - cellToTranslate.y, heroMapCell.y + cellToTranslate.x};
			return mapCell;
		}
		break;
		case 7: {
			FovCell mapCell = {heroMapCell.x - cellToTranslate.y, heroMapCell.y - cellToTranslate.x};
			return mapCell;
		}
		break;
		case 8: {
			FovCell mapCell = {heroMapCell.x - cellToTranslate.x, heroMapCell.y - cellToTranslate.y};
			return mapCell;
		}
		break;
		default:
			assert(false);
			FovCell c = {0,0};
			return c;
	}
}
