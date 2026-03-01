#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#include "ed_defines.h"
#include "ed_structs.h"

void e_GetOrigin( World_t* map, int* originx, int* originy );
void e_GetCellAtMouseInViewport( int width, int height,
                                 int originx, int originy,
                                 int* grid_x, int* grid_y );
void e_GetCellAtMouse( int width, int height,
                       int originx, int originy,
                       int cell_width, int cell_height,
                       int* grid_x, int* grid_y, int centered );

#endif

