#ifndef __GAME_EVENTS_H__
#define __GAME_EVENTS_H__

#include "console.h"
#include "items.h"
#include "enemies.h"

typedef enum
{
  EVT_LOOK_EQUIPMENT,
  EVT_LOOK_CONSUMABLE,
  EVT_LOOK_MAP,
  EVT_EQUIP,
  EVT_UNEQUIP,
  EVT_SWAP_EQUIP,
  EVT_USE_CONSUMABLE,
} GameEventType_t;

void GameEventsInit( Console_t* c );
void GameEventsNewTurn( void );
int  GameEventsConsumableUsed( void );
void GameEvent( GameEventType_t type, int index );
void GameEventSwap( int new_idx, int old_idx );

/* Try to use a consumable. Returns 1 on success, 0 on failure (wrong type). */
int  GameEventUseConsumable( int consumable_index );

/* Try to use a map. Returns 0 always (maps are never consumed). */
int  GameEventUseMap( int map_index );

/* Resolve a targeted consumable effect. Returns 1 on success. */
int  GameEventResolveTarget( int consumable_idx, int inv_slot,
                             int target_row, int target_col,
                             Enemy_t* enemies, int num_enemies );

#endif
