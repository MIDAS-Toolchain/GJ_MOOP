#include <Archimedes.h>

#include "defines.h"
#include "dungeon.h"
#include "visibility.h"

#define EASEL_COL 22
#define EASEL_ROW  4

static aImage_t* easel_image = NULL;
static float easel_wx = 0;
static float easel_wy = 0;

void DungeonHandlerInit( World_t* world )
{
  easel_image = a_ImageLoad( "resources/assets/objects/jonathon-easel.png" );
  easel_wx = EASEL_COL * world->tile_w + world->tile_w / 2.0f;
  easel_wy = EASEL_ROW * world->tile_h + world->tile_h / 2.0f;
}

void DungeonDrawProps( aRectf_t vp_rect, GameCamera_t* cam,
                       World_t* world, int gfx_mode )
{
  if ( VisibilityGet( EASEL_COL, EASEL_ROW ) < 0.01f ) return;

  if ( easel_image && gfx_mode == GFX_IMAGE )
  {
    GV_DrawSprite( vp_rect, cam, easel_image,
                   easel_wx, easel_wy,
                   (float)world->tile_w, (float)world->tile_h );
  }
  /* ASCII mode: midground glyph "E" is already drawn by GV_DrawWorld */
}
