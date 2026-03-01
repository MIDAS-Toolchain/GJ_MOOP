#include <string.h>
#include <Archimedes.h>

#include "placed_traps.h"
#include "visibility.h"
#include "defines.h"

static PlacedTrap_t traps[MAX_PLACED_TRAPS];
static int          num_traps = 0;
static Console_t*   console   = NULL;
static aImage_t*    trap_image = NULL;

#define TRAP_COLOR       (aColor_t){ 160, 160, 160, 80 }
#define TRAP_GLYPH_COLOR (aColor_t){ 160, 160, 160, 200 }

void PlacedTrapsInit( Console_t* con )
{
  console = con;
  memset( traps, 0, sizeof( traps ) );
  num_traps = 0;
  if ( !trap_image )
    trap_image = a_ImageLoad( "resources/assets/consumables/bear_trap.png" );
}

void PlacedTrapSpawn( int row, int col, int damage, int stun_turns )
{
  int slot = -1;
  for ( int i = 0; i < num_traps; i++ )
  {
    if ( !traps[i].active ) { slot = i; break; }
  }
  if ( slot < 0 )
  {
    if ( num_traps >= MAX_PLACED_TRAPS ) return;
    slot = num_traps++;
  }

  traps[slot].row         = row;
  traps[slot].col         = col;
  traps[slot].damage      = damage;
  traps[slot].stun_turns  = stun_turns;
  traps[slot].active      = 1;
}

PlacedTrap_t* PlacedTrapAt( int row, int col )
{
  for ( int i = 0; i < num_traps; i++ )
  {
    if ( traps[i].active && traps[i].row == row && traps[i].col == col )
      return &traps[i];
  }
  return NULL;
}

void PlacedTrapRemove( PlacedTrap_t* trap )
{
  trap->active = 0;
}

void PlacedTrapsDrawAll( aRectf_t vp_rect, GameCamera_t* cam,
                         World_t* world, int gfx_mode )
{
  for ( int i = 0; i < num_traps; i++ )
  {
    if ( !traps[i].active ) continue;
    if ( VisibilityGet( traps[i].row, traps[i].col ) < 0.01f ) continue;

    float wx = traps[i].row * world->tile_w + world->tile_w / 2.0f;
    float wy = traps[i].col * world->tile_h + world->tile_h / 2.0f;

    if ( trap_image && gfx_mode == GFX_IMAGE )
    {
      GV_DrawSprite( vp_rect, cam, trap_image,
                     wx, wy,
                     (float)world->tile_w, (float)world->tile_h );
    }
    else
    {
      GV_DrawFilledRect( vp_rect, cam, wx, wy,
                         (float)world->tile_w, (float)world->tile_h,
                         TRAP_COLOR );

      float sx, sy;
      GV_WorldToScreen( vp_rect, cam,
                        wx - world->tile_w / 2.0f,
                        wy - world->tile_h / 2.0f,
                        &sx, &sy );
      float half_w = cam->half_h * ( vp_rect.w / vp_rect.h );
      int dw = (int)( world->tile_w * ( vp_rect.w / ( half_w * 2.0f ) ) );
      int dh = (int)( world->tile_h * ( vp_rect.h / ( cam->half_h * 2.0f ) ) );
      a_DrawGlyph( "^", (int)sx, (int)sy, dw, dh,
                   TRAP_GLYPH_COLOR, (aColor_t){ 0, 0, 0, 0 },
                   FONT_CODE_PAGE_437 );
    }
  }
}
