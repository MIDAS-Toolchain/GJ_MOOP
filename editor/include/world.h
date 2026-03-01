#ifndef __WORLD_H__
#define __WORLD_H__

#include "ed_structs.h"

#define TILE_EMPTY ((uint32_t)-1)

World_t* WorldCreate( int width, int height, int tile_w, int tile_h );
void WorldDraw( int x_off, int y_off,
                World_t* world, aTileset_t* tile_set,
                uint8_t draw_ascii );

#endif

