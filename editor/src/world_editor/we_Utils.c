/*
 * world_editor/we_Utils.c:
 *
 * Copyright (c) 2026 Jacob Kellum <jkellum819@gmail.com>
 *                    Mathew Storm <smattymat@gmail.com>
 ************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <Archimedes.h>
#include <Daedalus.h>

#include "ed_defines.h"
#include "ed_structs.h"

#include "editor.h"
#include "world.h"
#include "world_editor.h"

void we_DrawColorPalette( int originx, int originy, int fg_index, int bg_index )
{
  int cx = 0, cy = 0;
  for ( int i = 0; i < MAX_COLOR_PALETTE; i++ )
  {

    if ( cx + GLYPH_WIDTH > ( 6 * GLYPH_WIDTH ) )
    {
      cx = 0;
      cy += GLYPH_HEIGHT;
      if ( cy > ( 6 * GLYPH_HEIGHT ) )
      {
        //        printf( "color grid is too large!\n" );
        //        return;
      }

    }
    aRectf_t color_palette_rect = (aRectf_t){ .x = ( cx + originx ), .y = ( cy + originy ),
    .w = GLYPH_WIDTH, .h = GLYPH_HEIGHT };
    a_DrawFilledRect( color_palette_rect, master_colors[APOLLO_PALETE][i] );

    if ( i == fg_index )
    {
      a_DrawRect( color_palette_rect, magenta );
    }

    if ( i == bg_index )
    {
      a_DrawRect( color_palette_rect, yellow );
    }

    cx += GLYPH_WIDTH;
  }
}

void we_DrawGlyphPalette( int originx, int originy, int glyph_index )
{
  int gx = 0, gy = 0;
  int width = 16, height = 20;
  for ( int i = 0; i < game_glyphs->count; i++ )
  {
    if ( gx + GLYPH_WIDTH > ( width * GLYPH_WIDTH ) )
    {
      gx = 0;
      gy += GLYPH_HEIGHT;
      if ( gy > ( height * GLYPH_HEIGHT ) )
      {
        //        printf( "color grid is too large!\n" );
        //        return;
      }

    }

    aRectf_t glyph_palette_rect = (aRectf_t){ .x = ( gx + originx ), .y = ( gy + originy ),
    .w = game_glyphs->rects[i].w, .h = game_glyphs->rects[i].h };
    
    a_DrawFilledRect( glyph_palette_rect, black );
   
    a_BlitTextureRect( game_glyphs->texture, game_glyphs->rects[i],
                       gx + originx, gy + originy, 1, white );

    if ( i == glyph_index )
    {
      a_DrawRect( glyph_palette_rect, magenta );
    }

    gx += GLYPH_WIDTH;
  }
}

