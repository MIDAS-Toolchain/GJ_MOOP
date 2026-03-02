#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "ground_items.h"
#include "items.h"
#include "maps.h"
#include "game_viewport.h"
#include "world.h"
#include "visibility.h"

void GroundItemsInit( GroundItem_t* list, int* count )
{
  memset( list, 0, sizeof( GroundItem_t ) * MAX_GROUND_ITEMS );
  *count = 0;
}

GroundItem_t* GroundItemSpawn( GroundItem_t* list, int* count,
                               int consumable_idx, int row, int col,
                               int tile_w, int tile_h )
{
  if ( *count >= MAX_GROUND_ITEMS || consumable_idx < 0
       || consumable_idx >= g_num_consumables )
    return NULL;

  GroundItem_t* g = &list[*count];
  g->item_type = GROUND_CONSUMABLE;
  g->item_idx  = consumable_idx;
  g->row     = row;
  g->col     = col;
  g->world_x = row * tile_w + tile_w / 2.0f;
  g->world_y = col * tile_h + tile_h / 2.0f;
  g->alive   = 1;
  ( *count )++;
  return g;
}

GroundItem_t* GroundItemSpawnMap( GroundItem_t* list, int* count,
                                  int map_idx, int row, int col,
                                  int tile_w, int tile_h )
{
  if ( *count >= MAX_GROUND_ITEMS || map_idx < 0
       || map_idx >= g_num_maps )
    return NULL;

  GroundItem_t* g = &list[*count];
  g->item_type = GROUND_MAP;
  g->item_idx  = map_idx;
  g->row     = row;
  g->col     = col;
  g->world_x = row * tile_w + tile_w / 2.0f;
  g->world_y = col * tile_h + tile_h / 2.0f;
  g->alive   = 1;
  ( *count )++;
  return g;
}

GroundItem_t* GroundItemSpawnEquipment( GroundItem_t* list, int* count,
                                        int equip_idx, int row, int col,
                                        int tile_w, int tile_h )
{
  if ( *count >= MAX_GROUND_ITEMS || equip_idx < 0
       || equip_idx >= g_num_equipment )
    return NULL;

  GroundItem_t* g = &list[*count];
  g->item_type = GROUND_EQUIPMENT;
  g->item_idx  = equip_idx;
  g->row     = row;
  g->col     = col;
  g->world_x = row * tile_w + tile_w / 2.0f;
  g->world_y = col * tile_h + tile_h / 2.0f;
  g->alive   = 1;
  ( *count )++;
  return g;
}

GroundItem_t* GroundItemAt( GroundItem_t* list, int count, int row, int col )
{
  for ( int i = 0; i < count; i++ )
  {
    if ( list[i].alive && list[i].row == row && list[i].col == col )
      return &list[i];
  }
  return NULL;
}

void GroundItemsDrawAll( aRectf_t vp_rect, GameCamera_t* cam,
                          GroundItem_t* list, int count,
                          World_t* world, int gfx_mode )
{
  for ( int i = 0; i < count; i++ )
  {
    if ( !list[i].alive ) continue;

    /* Skip items outside visible tiles */
    if ( VisibilityGet( list[i].row, list[i].col ) < 0.01f ) continue;

    /* Get glyph/color/image based on item type */
    const char* glyph = NULL;
    aColor_t color = { 255, 255, 255, 255 };
    aImage_t* image = NULL;

    if ( list[i].item_type == GROUND_MAP )
    {
      MapInfo_t* mi = &g_maps[list[i].item_idx];
      glyph = mi->glyph;
      color = mi->color;
      image = mi->image;
    }
    else if ( list[i].item_type == GROUND_EQUIPMENT )
    {
      EquipmentInfo_t* ei = &g_equipment[list[i].item_idx];
      glyph = ei->glyph;
      color = ei->color;
    }
    else
    {
      ConsumableInfo_t* ci = &g_consumables[list[i].item_idx];
      glyph = ci->glyph;
      color = ci->color;
      image = ci->image;
    }

    if ( image && gfx_mode == GFX_IMAGE )
    {
      GV_DrawSprite( vp_rect, cam, image,
                     list[i].world_x, list[i].world_y,
                     (float)world->tile_w, (float)world->tile_h );
    }
    else
    {
      float sx, sy;
      GV_WorldToScreen( vp_rect, cam,
                        list[i].world_x - world->tile_w / 2.0f,
                        list[i].world_y - world->tile_h / 2.0f,
                        &sx, &sy );
      float half_w = cam->half_h * ( vp_rect.w / vp_rect.h );
      int dw = (int)( world->tile_w * ( vp_rect.w / ( half_w * 2.0f ) ) );
      int dh = (int)( world->tile_h * ( vp_rect.h / ( cam->half_h * 2.0f ) ) );

      a_DrawGlyph( glyph, (int)sx, (int)sy, dw, dh,
                   color, (aColor_t){ 0, 0, 0, 0 }, FONT_CODE_PAGE_437 );
    }
  }
}
