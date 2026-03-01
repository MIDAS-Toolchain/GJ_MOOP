#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "player.h"
#include "items.h"
#include "console.h"
#include "game_events.h"
#include "game_scene.h"

extern Player_t player;

static Console_t* con;

void GameEventsInit( Console_t* c )
{
  con = c;
}

static void evt_LookEquipment( int index )
{
  EquipmentInfo_t* e = &g_equipment[index];

  ConsolePushF( con, e->color, "%s looked at %s", player.name, e->name );
  if ( e->damage > 0 )
    ConsolePushF( con, (aColor_t){ 0xeb, 0xed, 0xe9, 255 }, "  DMG: +%d", e->damage );
  if ( e->defense > 0 )
    ConsolePushF( con, (aColor_t){ 0xeb, 0xed, 0xe9, 255 }, "  DEF: +%d", e->defense );
  if ( strcmp( e->effect, "none" ) != 0 )
    ConsolePushF( con, (aColor_t){ 0xde, 0x9e, 0x41, 255 }, "  %s (%d)", e->effect, e->effect_value );
  ConsolePushF( con, (aColor_t){ 0x81, 0x97, 0x96, 255 }, "  %s", e->description );
}

static void evt_LookConsumable( int index )
{
  ConsumableInfo_t* c = &g_consumables[index];

  ConsolePushF( con, c->color, "%s looked at %s", player.name, c->name );
  if ( c->bonus_damage > 0 )
    ConsolePushF( con, (aColor_t){ 0xeb, 0xed, 0xe9, 255 }, "  DMG: +%d", c->bonus_damage );
  if ( strcmp( c->effect, "none" ) != 0 && strlen( c->effect ) > 0 )
    ConsolePushF( con, (aColor_t){ 0xde, 0x9e, 0x41, 255 }, "  %s", c->effect );
  ConsolePushF( con, (aColor_t){ 0x81, 0x97, 0x96, 255 }, "  %s", c->description );
}

static void evt_Equip( int index )
{
  EquipmentInfo_t* e = &g_equipment[index];
  ConsolePushF( con, e->color, "%s equipped %s", player.name, e->name );
  PlayerRecalcStats();
}

static void evt_Unequip( int index )
{
  EquipmentInfo_t* e = &g_equipment[index];
  ConsolePushF( con, (aColor_t){ 0x81, 0x97, 0x96, 255 }, "%s unequipped %s",
                player.name, e->name );
  PlayerRecalcStats();
}

static void evt_SwapEquip( int new_idx, int old_idx )
{
  EquipmentInfo_t* new_e = &g_equipment[new_idx];
  EquipmentInfo_t* old_e = &g_equipment[old_idx];
  ConsolePushF( con, new_e->color, "%s swapped %s for %s",
                player.name, old_e->name, new_e->name );
  PlayerRecalcStats();
}

static void evt_UseConsumable( int index )
{
  ConsumableInfo_t* c = &g_consumables[index];
  ConsolePushF( con, c->color, "%s used %s", player.name, c->name );
}

int GameEventUseConsumable( int consumable_index )
{
  if ( consumable_index < 0 || consumable_index >= g_num_consumables )
    return 0;

  ConsumableInfo_t* c = &g_consumables[consumable_index];

  if ( strcmp( c->type, "food" ) == 0 )
  {
    player.buff.active = 1;
    player.buff.bonus_damage = c->bonus_damage;
    strncpy( player.buff.effect, c->effect, MAX_NAME_LENGTH - 1 );
    player.buff.heal = c->heal;

    if ( strcmp( c->effect, "none" ) != 0 && strlen( c->effect ) > 0 )
      ConsolePushF( con, c->color, "%s eats %s. Next attack: %s (+%d dmg).",
                    player.name, c->name, c->effect, c->bonus_damage );
    else
      ConsolePushF( con, c->color, "%s eats %s. Next attack: +%d dmg.",
                    player.name, c->name, c->bonus_damage );

    GameSceneUseConsumable();
    return 1;
  }

  ConsolePushF( con, (aColor_t){ 0xcf, 0x57, 0x3c, 255 },
                "%s can't use %s yet.", player.name, c->name );
  return 0;
}

void GameEvent( GameEventType_t type, int index )
{
  switch ( type )
  {
    case EVT_LOOK_EQUIPMENT:  evt_LookEquipment( index );  break;
    case EVT_LOOK_CONSUMABLE: evt_LookConsumable( index );  break;
    case EVT_EQUIP:           evt_Equip( index );           break;
    case EVT_UNEQUIP:         evt_Unequip( index );         break;
    case EVT_USE_CONSUMABLE:  evt_UseConsumable( index );   break;
    default: break;
  }
}

void GameEventSwap( int new_idx, int old_idx )
{
  evt_SwapEquip( new_idx, old_idx );
}
