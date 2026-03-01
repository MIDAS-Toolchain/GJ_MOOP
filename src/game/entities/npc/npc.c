#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "npc.h"
#include "dialogue.h"
#include "player.h"
#include "draw_utils.h"
#include "game_viewport.h"
#include "world.h"

extern Player_t player;

void NPCsInit( NPC_t* list, int* count )
{
  memset( list, 0, sizeof( NPC_t ) * MAX_NPCS );
  *count = 0;
}

NPC_t* NPCSpawn( NPC_t* list, int* count,
                  int type_idx, int row, int col,
                  int tile_w, int tile_h )
{
  if ( *count >= MAX_NPCS || type_idx < 0 || type_idx >= g_num_npc_types )
    return NULL;

  NPC_t* n = &list[*count];
  n->type_idx = type_idx;
  n->row      = row;
  n->col      = col;
  n->world_x  = row * tile_w + tile_w / 2.0f;
  n->world_y  = col * tile_h + tile_h / 2.0f;
  n->alive    = 1;
  ( *count )++;
  return n;
}

NPC_t* NPCAt( NPC_t* list, int count, int row, int col )
{
  for ( int i = 0; i < count; i++ )
  {
    if ( list[i].alive && list[i].row == row && list[i].col == col )
      return &list[i];
  }
  return NULL;
}

void NPCsDrawAll( aRectf_t vp_rect, GameCamera_t* cam,
                   NPC_t* list, int count,
                   World_t* world, int gfx_mode )
{
  for ( int i = 0; i < count; i++ )
  {
    if ( !list[i].alive ) continue;
    NPCType_t* nt = &g_npc_types[list[i].type_idx];

    /* Face toward the player */
    int face_left = ( player.world_x < list[i].world_x );

    /* Shadow */
    GV_DrawFilledRect( vp_rect, cam,
                       list[i].world_x, list[i].world_y + 8.0f,
                       10.0f, 3.0f,
                       (aColor_t){ 0, 0, 0, 80 } );

    if ( nt->image && gfx_mode == GFX_IMAGE )
    {
      if ( face_left )
        GV_DrawSpriteFlipped( vp_rect, cam, nt->image,
                              list[i].world_x, list[i].world_y,
                              (float)world->tile_w, (float)world->tile_h, 'x' );
      else
        GV_DrawSprite( vp_rect, cam, nt->image,
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

      a_DrawGlyph( nt->glyph, (int)sx, (int)sy, dw, dh,
                   nt->color, (aColor_t){ 0, 0, 0, 0 }, FONT_CODE_PAGE_437 );
    }
  }
}
