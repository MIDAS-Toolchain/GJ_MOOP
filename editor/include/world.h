#ifndef __WORLD_H__
#define __WORLD_H__

#include "ed_structs.h"

#define TILE_EMPTY ((uint32_t)-1)

World_t* WorldCreate( const int width, const int height,
                      const int tile_w, const int tile_h );
void WorldDraw( const int x_off, const int y_off,
                World_t* world, Tileset_t* tile_set,
                const uint8_t draw_ascii );

#endif

