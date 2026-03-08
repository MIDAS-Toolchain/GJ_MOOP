#include <stdlib.h>
#include <string.h>

#include "enemies.h"
#include "visibility.h"
#include "combat.h"
#include "combat_vfx.h"
#include "spell_vfx.h"
#include "console.h"
#include "game_events.h"

#define STONE_HEAL_AMOUNT  2
#define STONE_HEAL_RANGE   8   /* Manhattan distance */

/* ---- Stone Healer AI ---- */
/* Immobile. Each turn, heals the most-damaged ally in range. */

void EnemyStoneHealerTick( Enemy_t* e, int player_row, int player_col,
                           int (*walkable)(int,int),
                           Enemy_t* all, int count )
{
  (void)player_row; (void)player_col; (void)walkable;
  if ( !e->alive ) return;
  if ( e->stun_turns > 0 ) return;

  Enemy_t* best = NULL;
  int best_missing = 0;

  for ( int i = 0; i < count; i++ )
  {
    if ( &all[i] == e || !all[i].alive ) continue;
    /* Don't heal other static/stone objects */
    const char* ai = g_enemy_types[all[i].type_idx].ai;
    if ( strcmp( ai, "static" ) == 0 ) continue;
    if ( strcmp( ai, "stone_healer" ) == 0 ) continue;
    if ( strcmp( ai, "stone_ranged" ) == 0 ) continue;

    int dr = abs( all[i].row - e->row );
    int dc = abs( all[i].col - e->col );
    if ( dr + dc > STONE_HEAL_RANGE ) continue;

    int max_hp  = g_enemy_types[all[i].type_idx].hp;
    int missing = max_hp - all[i].hp;
    if ( missing > 0 && missing > best_missing )
    {
      best = &all[i];
      best_missing = missing;
    }
  }

  if ( !best ) return;

  int max_hp = g_enemy_types[best->type_idx].hp;
  int heal = STONE_HEAL_AMOUNT;
  if ( best->hp + heal > max_hp )
    heal = max_hp - best->hp;

  best->hp += heal;
  SpellVFXHeal( e->world_x, e->world_y, best->world_x, best->world_y );
  CombatVFXSpawnNumber( best->world_x, best->world_y, heal,
                        (aColor_t){ 0xc8, 0x50, 0x50, 255 } );
  CombatVFXSpawnText( e->world_x, e->world_y,
                      "Hums!", (aColor_t){ 0xc8, 0x50, 0x50, 255 } );

  EnemyType_t* bt = &g_enemy_types[best->type_idx];
  ConsolePushF( GameEventsGetConsole(), (aColor_t){ 0xc8, 0x50, 0x50, 255 },
                "The Humming Stone heals %s for %d HP.", bt->name, heal );
}

/* ---- Stone Ranged AI ---- */
/* Immobile. Fires a ranged shot at the player every other turn. */
/* ai_state: 0 = acquire, 1 = telegraph, 2 = cooldown */

#define SST_ACQUIRE   0
#define SST_TELEGRAPH 1
#define SST_COOLDOWN  2

/* Check if player is on a clear cardinal line within range. */
static int stone_has_shot( Enemy_t* e, int pr, int pc, int range,
                           int (*walkable)(int,int),
                           int* out_dr, int* out_dc )
{
  if ( e->row == pr && e->col != pc )
  {
    int dc = ( pc > e->col ) ? 1 : -1;
    int dist = abs( pc - e->col );
    if ( dist > range ) return 0;
    int cc = e->col;
    for ( int s = 0; s < dist; s++ )
    {
      cc += dc;
      if ( !walkable( e->row, cc ) ) return 0;
      if ( EnemyBlockedByNPC( e->row, cc ) ) return 0;
    }
    *out_dr = 0;
    *out_dc = dc;
    return 1;
  }
  else if ( e->col == pc && e->row != pr )
  {
    int dr = ( pr > e->row ) ? 1 : -1;
    int dist = abs( pr - e->row );
    if ( dist > range ) return 0;
    int cr = e->row;
    for ( int s = 0; s < dist; s++ )
    {
      cr += dr;
      if ( !walkable( cr, e->col ) ) return 0;
      if ( EnemyBlockedByNPC( cr, e->col ) ) return 0;
    }
    *out_dr = dr;
    *out_dc = 0;
    return 1;
  }
  return 0;
}

void EnemyStoneRangedTick( Enemy_t* e, int player_row, int player_col,
                           int (*walkable)(int,int),
                           Enemy_t* all, int count )
{
  (void)all; (void)count;
  if ( !e->alive ) return;
  if ( e->stun_turns > 0 ) return;

  EnemyType_t* t = &g_enemy_types[e->type_idx];
  int dr = abs( player_row - e->row );
  int dc = abs( player_col - e->col );
  int dist = dr + dc;

  int can_see = ( dist <= t->sight_range
                  && los_clear( e->row, e->col, player_row, player_col ) );

  switch ( e->ai_state )
  {
    case SST_ACQUIRE:
    {
      int shot_dr, shot_dc;
      if ( can_see
           && stone_has_shot( e, player_row, player_col, t->range,
                              walkable, &shot_dr, &shot_dc ) )
      {
        e->ai_dir_row = shot_dr;
        e->ai_dir_col = shot_dc;
        e->ai_state = SST_TELEGRAPH;
      }
      /* If no shot, just wait (stone can't move) */
      break;
    }

    case SST_TELEGRAPH:
    {
      /* Fire along locked direction */
      int cr = e->row;
      int cc = e->col;

      for ( int step = 0; step < t->range; step++ )
      {
        cr += e->ai_dir_row;
        cc += e->ai_dir_col;
        if ( !walkable( cr, cc ) ) break;
        if ( EnemyBlockedByNPC( cr, cc ) ) break;

        if ( cr == player_row && cc == player_col )
        {
          CombatEnemyHit( e );
          break;
        }
      }

      /* Spawn projectile VFX */
      {
        float tw = EnemyTileW(), th = EnemyTileH();
        EnemyProjectileSpawn( e->world_x, e->world_y,
                              cr * tw + tw / 2.0f, cc * th + th / 2.0f,
                              e->ai_dir_row, e->ai_dir_col );
      }

      e->ai_state = SST_COOLDOWN;
      break;
    }

    case SST_COOLDOWN:
    {
      /* One turn cooldown, then back to acquire */
      e->ai_state = SST_ACQUIRE;
      break;
    }

    default:
      e->ai_state = SST_ACQUIRE;
      break;
  }
}
