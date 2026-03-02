#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#include "ed_defines.h"
#include "ed_structs.h"

void e_GetOrigin( World_t* map, int* originx, int* originy );
void e_GetCellAtMouseInViewport( const int width,   const int height,
                                 const int originx, const int originy,
                                 int* grid_x, int* grid_y );
void e_GetCellAtMouse( const int width,      const int height,
                       const int originx,    const int originy,
                       const int cell_width, const int cell_height,
                       int* grid_x, int* grid_y, const int centered );

Tileset_t* e_TilesetCreate( const char* filename,
                            const int tile_w, const int tile_h );

void e_TilesetMouseCheck( const int originx, const int originy, int* index,
                      int* grid_x, int* grid_y, const int centered );

void e_GlyphMouseCheck( const int originx, const int originy, int* index,
                        int* grid_x, int* grid_y, const int centered );

void e_ColorMouseCheck( const int originx, const int originy, int* index,
                        int* grid_x, int* grid_y, const int centered );

void e_LoadColorPalette( aColor_t palette[MAX_COLOR_GROUPS][MAX_COLOR_PALETTE],
                         const char * filename );

#endif

