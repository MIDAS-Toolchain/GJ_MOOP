#ifndef __SHOP_H__
#define __SHOP_H__

#include <Archimedes.h>
#include "world.h"
#include "game_viewport.h"

#define MAX_SHOP_ITEMS    8
#define MAX_SHOP_POOL    16

typedef struct
{
  char item_key[MAX_NAME_LENGTH];
  char type[MAX_NAME_LENGTH];   /* "consumable" or "equipment" */
  char class_name[MAX_NAME_LENGTH];
  int  cost;
  int  weight;
} ShopPoolEntry_t;

typedef struct
{
  int   item_type;         /* INV_CONSUMABLE or INV_EQUIPMENT */
  int   item_index;        /* index into g_consumables[] or g_equipment[] */
  int   cost;
  int   row, col;
  float world_x, world_y;
  int   alive;
} ShopItem_t;

void        ShopLoadPool( const char* path );
void        ShopSpawn( World_t* world );
ShopItem_t* ShopItemAt( int row, int col );
int         ShopIsRugTile( int row, int col );

void ShopDrawRug( aRectf_t vp_rect, GameCamera_t* cam,
                  World_t* world, int gfx_mode );
void ShopDrawItems( aRectf_t vp_rect, GameCamera_t* cam,
                    World_t* world, int gfx_mode );

extern ShopItem_t  g_shop_items[MAX_SHOP_ITEMS];
extern int         g_num_shop_items;

#endif
