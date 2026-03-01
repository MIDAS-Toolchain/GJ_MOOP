#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "ground_items.h"
#include "items.h"
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
  g->consumable_idx = consumable_idx;
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

    ConsumableInfo_t* ci = &g_consumables[list[i].consumable_idx];

    if ( ci->image && gfx_mode == GFX_IMAGE )
    {
      GV_DrawSprite( vp_rect, cam, ci->image,
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

      a_DrawGlyph( ci->glyph, (int)sx, (int)sy, dw, dh,
                   ci->color, (aColor_t){ 0, 0, 0, 0 }, FONT_CODE_PAGE_437 );
    }
  }
}
