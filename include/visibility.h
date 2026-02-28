#ifndef __VISIBILITY_H__
#define __VISIBILITY_H__

#include "world.h"

void  VisibilityInit( World_t* w );
void  VisibilityUpdate( int player_row, int player_col );
float VisibilityGet( int r, int c );
int   los_clear( int x0, int y0, int x1, int y1 );

#endif
