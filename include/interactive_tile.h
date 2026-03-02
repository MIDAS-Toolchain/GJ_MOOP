#ifndef __INTERACTIVE_TILE_H__
#define __INTERACTIVE_TILE_H__

#include "world.h"

#define MAX_ITILES     16
#define ITILE_RAT_HOLE  0

typedef struct {
  int row, col, type, active;
} ITile_t;

void         ITileInit( void );
void         ITilePlace( World_t* world, int x, int y, int type );
ITile_t*     ITileAt( int row, int col );
void         ITileBreak( World_t* world, int row, int col );
const char*  ITileDescription( int type );

#endif
