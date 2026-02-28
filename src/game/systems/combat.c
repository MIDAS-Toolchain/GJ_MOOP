#include <Archimedes.h>

#include "combat.h"
#include "combat_vfx.h"
#include "enemies.h"
#include "player.h"
#include "console.h"
#include "items.h"
#include "tween.h"

extern Player_t player;

static Console_t* console = NULL;
static aSoundEffect_t sfx_hit;

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

int CombatAttack( Enemy_t* e )
{
  EnemyType_t* t = &g_enemy_types[e->type_idx];
  int pdmg = PlayerStat( "damage" );
  if ( pdmg < 1 ) pdmg = 1;
  e->hp -= pdmg;
  e->turns_since_hit = 0;

  CombatVFXSpawnNumber( e->world_x, e->world_y, pdmg,
                        (aColor_t){ 0xeb, 0xed, 0xe9, 255 } );
  a_AudioPlaySound( &sfx_hit, NULL );
  ConsolePushF( console, (aColor_t){ 0xe8, 0xc1, 0x70, 255 },
                "You hit %s for %d damage.", t->name, pdmg );

  if ( e->hp <= 0 )
  {
    e->alive = 0;
    ConsolePushF( console, (aColor_t){ 0x75, 0xa7, 0x43, 255 },
                  "You defeated the %s!", t->name );
    return 1;
  }

  return 0;
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
    e->hp -= thorns;
    e->turns_since_hit = 0;
    CombatVFXSpawnNumber( e->world_x, e->world_y, thorns,
                          (aColor_t){ 0xde, 0x9e, 0x41, 255 } );
    ConsolePushF( console, (aColor_t){ 0xde, 0x9e, 0x41, 255 },
                  "Thorns deals %d damage to %s.", thorns, t->name );
    if ( e->hp <= 0 )
    {
      e->alive = 0;
      ConsolePushF( console, (aColor_t){ 0x75, 0xa7, 0x43, 255 },
                    "You defeated the %s!", t->name );
    }
  }
}
