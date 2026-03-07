#ifndef __WORLD_H__
#define __WORLD_H__

#include <Daedalus.h>

#include "ed_structs.h"

World_t* WorldCreate( const int width, const int height,
                      const int tile_w, const int tile_h );

void WorldDestroy( World_t* world );

void WorldDraw( const int x_off, const int y_off,
                World_t* world, Tileset_t* tile_set,
                const uint8_t draw_room_des,
                const uint8_t draw_ascii );

void DrawSelected( World_t* world, dVec2_t* selected_pos,
                   dVec2_t* highlighted_pos );

#endif

