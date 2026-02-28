#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>

#include "enemies.h"
#include "combat.h"
#include "world.h"
#include "tween.h"

static World_t* world = NULL;
static TweenManager_t tweens;

/* Turn state machine */
#define TURN_IDLE     0
#define TURN_MOVING   1   /* one enemy moving at a time */
#define TURN_ATTACK   2   /* one enemy lunging at a time */

static int turn_state = TURN_IDLE;

/* Saved from EnemiesStartTurn for use across phases */
static Enemy_t* turn_list  = NULL;
static int      turn_count = 0;
static int      turn_pr, turn_pc;
static int      was_adjacent[MAX_ENEMIES]; /* adjacent before moving */
static int      did_move[MAX_ENEMIES];     /* moved this turn */
static int      move_idx;                  /* current enemy being processed */
static int (*turn_walkable)(int,int);

void EnemiesSetWorld( World_t* w )
{
  world = w;
  InitTweenManager( &tweens );
  turn_state = TURN_IDLE;
}

/* --- Lunge callback data --- */

typedef struct
{
  float* target;
  float  home;
} LungeBack_t;

static LungeBack_t lunge_data[MAX_ENEMIES * 2];
static int lunge_count = 0;

static void lunge_back_cb( void* data )
{
  LungeBack_t* lb = (LungeBack_t*)data;
  TweenFloat( &tweens, lb->target, lb->home, 0.06f, TWEEN_EASE_OUT_CUBIC );
}

/* --- Sequential movement: advance to next enemy --- */

static void start_next_move( void );
static void start_next_attack( void );

static void tick_and_move( int i )
{
  EnemyType_t* t = &g_enemy_types[turn_list[i].type_idx];

  int old_row = turn_list[i].row;
  int old_col = turn_list[i].col;

  if ( strcmp( t->ai, "basic" ) == 0 )
    EnemyRatTick( &turn_list[i], turn_pr, turn_pc,
                  turn_walkable, turn_list, turn_count );

  if ( turn_list[i].row != old_row || turn_list[i].col != old_col )
  {
    if ( turn_list[i].row < old_row ) turn_list[i].facing_left = 0;
    else if ( turn_list[i].row > old_row ) turn_list[i].facing_left = 1;

    float tx = turn_list[i].row * world->tile_w + world->tile_w / 2.0f;
    float ty = turn_list[i].col * world->tile_h + world->tile_h / 2.0f;
    TweenFloat( &tweens, &turn_list[i].world_x, tx, 0.15f, TWEEN_EASE_OUT_CUBIC );
    TweenFloat( &tweens, &turn_list[i].world_y, ty, 0.15f, TWEEN_EASE_OUT_CUBIC );
    did_move[i] = 1;
  }
}

static void start_next_move( void )
{
  /* Find next alive enemy to move */
  while ( move_idx < turn_count )
  {
    if ( turn_list[move_idx].alive )
    {
      tick_and_move( move_idx );
      if ( did_move[move_idx] )
      {
        /* Tween started — wait for it to finish */
        turn_state = TURN_MOVING;
        move_idx++;
        return;
      }
    }
    move_idx++;
  }

  /* All enemies processed — begin attack phase */
  move_idx = 0;
  start_next_attack();
}

/* --- Sequential attacks: one lunge at a time --- */

static void do_attack( int i )
{
  int dr = turn_pr - turn_list[i].row;
  int dc = turn_pc - turn_list[i].col;

  float lunge_dist = 3.0f;
  lunge_count = 0;

  LungeBack_t* lbx = &lunge_data[lunge_count++];
  lbx->target = &turn_list[i].world_x;
  lbx->home   = turn_list[i].world_x;
  TweenFloatWithCallback( &tweens, &turn_list[i].world_x,
                          turn_list[i].world_x + dr * lunge_dist,
                          0.06f, TWEEN_EASE_OUT_QUAD,
                          lunge_back_cb, lbx );

  LungeBack_t* lby = &lunge_data[lunge_count++];
  lby->target = &turn_list[i].world_y;
  lby->home   = turn_list[i].world_y;
  TweenFloatWithCallback( &tweens, &turn_list[i].world_y,
                          turn_list[i].world_y + dc * lunge_dist,
                          0.06f, TWEEN_EASE_OUT_QUAD,
                          lunge_back_cb, lby );

  CombatEnemyHit( &turn_list[i] );
}

static void start_next_attack( void )
{
  while ( move_idx < turn_count )
  {
    if ( turn_list[move_idx].alive && was_adjacent[move_idx] )
    {
      int dr = abs( turn_pr - turn_list[move_idx].row );
      int dc = abs( turn_pc - turn_list[move_idx].col );
      if ( dr + dc == 1 )
      {
        do_attack( move_idx );
        turn_state = TURN_ATTACK;
        move_idx++;
        return;
      }
    }
    move_idx++;
  }

  /* No more attackers */
  turn_state = TURN_IDLE;
}

/* --- Public API --- */

void EnemiesStartTurn( Enemy_t* list, int count,
                       int player_row, int player_col,
                       int (*walkable)(int,int) )
{
  turn_list    = list;
  turn_count   = count;
  turn_pr      = player_row;
  turn_pc      = player_col;
  turn_walkable = walkable;

  /* Record which enemies are already adjacent before they move */
  for ( int i = 0; i < count; i++ )
  {
    int dr = abs( player_row - list[i].row );
    int dc = abs( player_col - list[i].col );
    was_adjacent[i] = list[i].alive && ( dr + dc == 1 );
    did_move[i] = 0;
  }

  /* Start sequential movement from enemy 0 */
  move_idx = 0;
  start_next_move();
}

void EnemiesUpdate( float dt )
{
  if ( turn_state == TURN_IDLE ) return;

  UpdateTweens( &tweens, dt );

  if ( GetActiveTweenCount( &tweens ) == 0 )
  {
    if ( turn_state == TURN_MOVING )
      start_next_move();
    else if ( turn_state == TURN_ATTACK )
      start_next_attack();
  }
}

int EnemiesTurning( void )
{
  return turn_state != TURN_IDLE;
}
