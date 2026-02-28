#include <stdlib.h>

#include "enemies.h"
#include "visibility.h"

#define RAT_SIGHT_RANGE 6

void EnemyRatTick( Enemy_t* e, int player_row, int player_col,
                   int (*walkable)(int,int),
                   Enemy_t* all, int count )
{
  if ( !e->alive ) return;

  int dr = player_row - e->row;
  int dc = player_col - e->col;
  int dist = abs( dr ) + abs( dc );

  /* LOS gate — too far or can't see the player: idle */
  if ( dist > RAT_SIGHT_RANGE ) return;
  if ( !los_clear( e->row, e->col, player_row, player_col ) ) return;

  /* Already adjacent — stay put, combat handled elsewhere */
  if ( dist <= 1 ) return;

  /* 4-direction pathfinding: pick the walkable, unoccupied neighbor
     closest to the player (Manhattan distance).  Only move if it
     actually gets us closer — prevents oscillation. */
  static const int dx[] = { 1, -1, 0, 0 };
  static const int dy[] = { 0, 0, 1, -1 };

  int best_dist = dist;  /* must improve on current distance */
  int best_r = e->row;
  int best_c = e->col;

  for ( int i = 0; i < 4; i++ )
  {
    int nr = e->row + dx[i];
    int nc = e->col + dy[i];

    if ( !walkable( nr, nc ) )          continue;
    if ( EnemyAt( all, count, nr, nc ) ) continue;

    int nd = abs( player_row - nr ) + abs( player_col - nc );
    if ( nd < best_dist )
    {
      best_dist = nd;
      best_r = nr;
      best_c = nc;
    }
  }

  if ( best_r != e->row || best_c != e->col )
  {
    e->row = best_r;
    e->col = best_c;
  }
}
