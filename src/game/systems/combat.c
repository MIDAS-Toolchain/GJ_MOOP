#include <Archimedes.h>

#include <stdio.h>

#include "combat.h"
#include "combat_vfx.h"
#include "enemies.h"
#include "player.h"
#include "console.h"
#include "items.h"
#include "maps.h"
#include "tween.h"
#include "dialogue.h"
#include "movement.h"
#include "poison_pool.h"

extern Player_t player;

static Console_t* console = NULL;
static aSoundEffect_t sfx_hit;

/* Enemy list for buff effects (cleave, reach) */
static Enemy_t* combat_enemies     = NULL;
static int*     combat_enemy_count = NULL;

/* Ground items for enemy drops */
static GroundItem_t* combat_ground_items = NULL;
static int*          combat_ground_count = NULL;

/* Return total effect_value for all equipped items with the given effect name */
static int equipped_effect( const char* name )
{
  int total = 0;
  for ( int i = 0; i < EQUIP_SLOTS; i++ )
  {
    if ( player.equipment[i] < 0 ) continue;
    EquipmentInfo_t* eq = &g_equipment[player.equipment[i]];
    if ( strcmp( eq->effect, name ) == 0 )
      total += eq->effect_value;
  }
  return total;
}

/* Screen shake + red flash on hit */
static TweenManager_t hit_tweens;
static float hit_shake_x = 0;
static float hit_shake_y = 0;
static float hit_flash_alpha = 0;

static void shake_back_x( void* data )
{
  (void)data;
  TweenFloat( &hit_tweens, &hit_shake_x, 0.0f, 0.08f, TWEEN_EASE_OUT_CUBIC );
}

static void shake_back_y( void* data )
{
  (void)data;
  TweenFloat( &hit_tweens, &hit_shake_y, 0.0f, 0.08f, TWEEN_EASE_OUT_CUBIC );
}

static void trigger_hit_effect( void )
{
  StopTweensForTarget( &hit_tweens, &hit_shake_x );
  StopTweensForTarget( &hit_tweens, &hit_shake_y );

  float sx = ( ( rand() % 2 ) ? 2.0f : -2.0f );
  float sy = ( ( rand() % 2 ) ? 1.5f : -1.5f );
  hit_shake_x = 0;
  hit_shake_y = 0;
  TweenFloatWithCallback( &hit_tweens, &hit_shake_x, sx, 0.04f,
                           TWEEN_EASE_OUT_QUAD, shake_back_x, NULL );
  TweenFloatWithCallback( &hit_tweens, &hit_shake_y, sy, 0.04f,
                           TWEEN_EASE_OUT_QUAD, shake_back_y, NULL );

  /* Red flash — only start if not already active */
  if ( hit_flash_alpha < 1.0f )
  {
    hit_flash_alpha = 60.0f;
    TweenFloat( &hit_tweens, &hit_flash_alpha, 0.0f, 0.35f, TWEEN_EASE_OUT_QUAD );
  }
}

void CombatSetEnemies( Enemy_t* list, int* count )
{
  combat_enemies     = list;
  combat_enemy_count = count;
}

void CombatSetGroundItems( GroundItem_t* list, int* count )
{
  combat_ground_items = list;
  combat_ground_count = count;
}

void CombatInit( Console_t* con )
{
  console = con;
  a_AudioLoadSound( "resources/soundeffects/hit_impact.wav", &sfx_hit );
  InitTweenManager( &hit_tweens );
  hit_shake_x = 0;
  hit_shake_y = 0;
  hit_flash_alpha = 0;
}

void CombatUpdate( float dt )
{
  UpdateTweens( &hit_tweens, dt );
}

float CombatFlashAlpha( void ) { return hit_flash_alpha; }
float CombatShakeOX( void )    { return hit_shake_x; }
float CombatShakeOY( void )    { return hit_shake_y; }

void CombatHandleEnemyDeath( Enemy_t* e )
{
  EnemyType_t* t = &g_enemy_types[e->type_idx];

  e->alive = 0;
  ConsolePushF( console, (aColor_t){ 0x75, 0xa7, 0x43, 255 },
                "You defeated the %s!", t->name );

  char kill_flag[MAX_NAME_LENGTH + 8];
  snprintf( kill_flag, sizeof( kill_flag ), "%s_kills", t->key );
  FlagIncr( kill_flag );

  /* Gold drop */
  if ( t->gold_drop > 0 )
  {
    player.gold += t->gold_drop;
    ConsolePushF( console, (aColor_t){ 0xda, 0xaf, 0x20, 255 },
                  "Picked up %d gold.", t->gold_drop );
  }

  /* Drop item on death — check maps first, then consumables */
  if ( t->drop_item[0] != '\0' && combat_ground_items && combat_ground_count )
  {
    int mi = MapByKey( t->drop_item );
    if ( mi >= 0 )
    {
      GroundItemSpawnMap( combat_ground_items, combat_ground_count,
                          mi, e->row, e->col, 16, 16 );
      ConsolePushF( console, g_maps[mi].color,
                    "The %s dropped %s!", t->name, g_maps[mi].name );
    }
    else
    {
      int ci = ConsumableByKey( t->drop_item );
      if ( ci >= 0 )
      {
        GroundItemSpawn( combat_ground_items, combat_ground_count,
                         ci, e->row, e->col, 16, 16 );
        ConsolePushF( console, g_consumables[ci].color,
                      "The %s dropped %s!", t->name, g_consumables[ci].name );
      }
    }
  }

  /* On-death hazard: spawn poison pool */
  if ( t->on_death[0] != '\0'
       && strcmp( t->on_death, "poison_pool" ) == 0 )
  {
    PoisonPoolSpawn( e->row, e->col, t->pool_duration, t->pool_damage );
    ConsolePushF( console, (aColor_t){ 50, 220, 50, 255 },
                  "The %s dissolves into a poison pool!", t->name );
  }
}

/* Helper: deal damage to an enemy, handle death + kill tracking.
   Returns 1 if the enemy died. */
static int deal_damage( Enemy_t* e, int dmg, aColor_t vfx_color )
{
  e->hp -= dmg;
  e->turns_since_hit = 0;

  CombatVFXSpawnNumber( e->world_x, e->world_y, dmg, vfx_color );

  if ( e->hp <= 0 )
  {
    CombatHandleEnemyDeath( e );
    return 1;
  }
  return 0;
}

/* Resolve food buff effects after primary hit */
static void resolve_buff( Enemy_t* primary )
{
  if ( !player.buff.active ) return;

  int pr, pc;
  PlayerGetTile( &pr, &pc );

  aColor_t buff_color = { 0xde, 0x9e, 0x41, 255 };

  /* Cleave: hit all alive enemies adjacent to player (Manhattan dist 1),
     excluding the primary target */
  if ( strcmp( player.buff.effect, "cleave" ) == 0 && combat_enemies && combat_enemy_count )
  {
    for ( int i = 0; i < *combat_enemy_count; i++ )
    {
      Enemy_t* ce = &combat_enemies[i];
      if ( !ce->alive || ce == primary ) continue;
      int dr = abs( ce->row - pr );
      int dc = abs( ce->col - pc );
      if ( dr + dc == 1 )
      {
        EnemyType_t* ct = &g_enemy_types[ce->type_idx];
        int cdmg = PlayerStat( "damage" ) + player.buff.bonus_damage;
        if ( cdmg < 1 ) cdmg = 1;
        ConsolePushF( console, buff_color,
                      "Cleave hits %s for %d damage!", ct->name, cdmg );
        deal_damage( ce, cdmg, buff_color );
      }
    }
  }
  /* Reach: hit enemy behind the primary target (same direction) */
  else if ( strcmp( player.buff.effect, "reach" ) == 0 && combat_enemies && combat_enemy_count )
  {
    int dir_r = primary->row - pr;
    int dir_c = primary->col - pc;
    int behind_r = primary->row + dir_r;
    int behind_c = primary->col + dir_c;

    for ( int i = 0; i < *combat_enemy_count; i++ )
    {
      Enemy_t* ce = &combat_enemies[i];
      if ( !ce->alive || ce == primary ) continue;
      if ( ce->row == behind_r && ce->col == behind_c )
      {
        EnemyType_t* ct = &g_enemy_types[ce->type_idx];
        int cdmg = PlayerStat( "damage" ) + player.buff.bonus_damage;
        if ( cdmg < 1 ) cdmg = 1;
        ConsolePushF( console, buff_color,
                      "Reach hits %s for %d damage!", ct->name, cdmg );
        deal_damage( ce, cdmg, buff_color );
        break;
      }
    }
  }
  /* Lifesteal: heal player */
  else if ( strcmp( player.buff.effect, "lifesteal" ) == 0 )
  {
    int heal = player.buff.heal;
    if ( heal > 0 )
    {
      player.hp += heal;
      if ( player.hp > player.max_hp ) player.hp = player.max_hp;
      CombatVFXSpawnNumber( player.world_x, player.world_y, heal,
                            (aColor_t){ 0x75, 0xa7, 0x43, 255 } );
      ConsolePushF( console, (aColor_t){ 0x75, 0xa7, 0x43, 255 },
                    "Lifesteal heals %d HP.", heal );
    }
  }
  /* "none" or unknown — bonus damage was already applied, nothing extra */

  memset( &player.buff, 0, sizeof( ConsumableBuff_t ) );
}

int CombatAttack( Enemy_t* e )
{
  EnemyType_t* t = &g_enemy_types[e->type_idx];
  int pdmg = PlayerStat( "damage" );

  /* Add food buff bonus damage */
  if ( player.buff.active )
    pdmg += player.buff.bonus_damage;

  if ( pdmg < 1 ) pdmg = 1;

  a_AudioPlaySound( &sfx_hit, NULL );
  ConsolePushF( console, (aColor_t){ 0xe8, 0xc1, 0x70, 255 },
                "You hit %s for %d damage.", t->name, pdmg );

  int killed = deal_damage( e, pdmg, (aColor_t){ 0xeb, 0xed, 0xe9, 255 } );

  /* Resolve buff effects after primary hit */
  if ( player.buff.active )
    resolve_buff( e );

  return killed;
}

void CombatEnemyHit( Enemy_t* e )
{
  EnemyType_t* t = &g_enemy_types[e->type_idx];
  int edmg = t->damage - PlayerStat( "defense" );
  if ( edmg < 1 ) edmg = 1;
  player.hp -= edmg;
  player.turns_since_hit = 0;

  CombatVFXSpawnNumber( player.world_x, player.world_y, edmg,
                        (aColor_t){ 0xcf, 0x57, 0x3c, 255 } );
  trigger_hit_effect();

  ConsolePushF( console, (aColor_t){ 0xcf, 0x57, 0x3c, 255 },
                "%s hits you for %d damage.", t->name, edmg );

  if ( player.hp <= 0 )
  {
    player.hp = 0;
    ConsolePush( console, "You have fallen...",
                 (aColor_t){ 0xa5, 0x30, 0x30, 255 } );
  }

  /* Passive: thorns — reflect damage back */
  int thorns = equipped_effect( "thorns" );
  if ( thorns > 0 && e->alive )
  {
    ConsolePushF( console, (aColor_t){ 0xde, 0x9e, 0x41, 255 },
                  "Thorns deals %d damage to %s.", thorns, t->name );
    deal_damage( e, thorns, (aColor_t){ 0xde, 0x9e, 0x41, 255 } );
  }
}
