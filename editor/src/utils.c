
#include <stdio.h>
#include <stdlib.h>

#include <Archimedes.h>
#include <Daedalus.h>

#include "ed_defines.h"
#include "ed_structs.h"

void e_GetOrigin( World_t* map, int* originx, int* originy )
{
  *originx = app.g_viewport.x - ( (float)( map->width  * map->tile_w ) / 2 );
  *originy = app.g_viewport.y - ( (float)( map->height * map->tile_h ) / 2 );
}

void e_GetCellAtMouseInViewport( int width,   int height,
                                 int originx, int originy,
                                 int* grid_x, int* grid_y )
{
  aPoint2f_t scale = a_ViewportCalculateScale();
  float view_x = app.g_viewport.x - app.g_viewport.w;
  float view_y = app.g_viewport.y - app.g_viewport.h;

  float world_mouse_x = ( app.mouse.x / scale.x ) + view_x;
  float world_mouse_y = ( app.mouse.y / scale.y ) + view_y;

  float relative_x = world_mouse_x - originx;
  float relative_y = world_mouse_y - originy;

  int cell_x = (int)( relative_x / 16 );
  int cell_y = (int)( relative_y / 16 );
  
  int extreme_w = ( EDITOR_WORLD_WIDTH  / width );
  int extreme_h = ( EDITOR_WORLD_HEIGHT / height );

  if ( cell_x >= 0 && cell_x < extreme_w &&
       cell_y >= 0 && cell_y < extreme_h )
  {
    *grid_x = cell_x;
    *grid_y = cell_y;
  }
}

void e_GetCellAtMouse( int width,      int height,
                       int originx,    int originy,
                       int cell_width, int cell_height,
                       int* grid_x,    int* grid_y, int centered )
{
  int edge_x = 0;
  int edge_y = 0;

  if ( centered == 1 )
  {
    edge_x = originx - ( ( width  * cell_width  ) / 2 );
    edge_y = originy - ( ( height * cell_height ) / 2 );
  }
  
  else
  {
    edge_x = originx;
    edge_y = originy;
  }

  if ( app.mouse.x > edge_x && app.mouse.x <= ( edge_x + ( width  * cell_width ) ) &&
       app.mouse.y > edge_y && app.mouse.y <= ( edge_y + ( height * cell_height ) ) )
  {
    int mousex = ( ( app.mouse.x - edge_x ) / cell_width  );
    int mousey = ( ( app.mouse.y - edge_y ) / cell_height );
    
    *grid_x = mousex;
    *grid_y = mousey;
  }
}

void e_ImgMouseCheck( int originx, int originy, int* index, int* grid_x,
                      int* grid_y, int centered )
{

  int img_grid_w = 16;
  int img_grid_h = 16;

  e_GetCellAtMouse( img_grid_w, img_grid_h,
                    originx, originy,
                    TILE_WIDTH, TILE_HEIGHT,
                    grid_x, grid_y, centered );

  *index = *grid_y * img_grid_w + *grid_x;

}

void e_GlyphMouseCheck( int originx, int originy, int* index, int* grid_x,
                        int* grid_y, int centered )
{
  int glyph_grid_w = 16;
  int glyph_grid_h = 16;

  e_GetCellAtMouse( glyph_grid_w, glyph_grid_h,
                    originx, originy,
                    GLYPH_WIDTH, GLYPH_HEIGHT,
                    grid_x, grid_y, centered );

  *index = *grid_y * glyph_grid_w + *grid_x;
}

void e_ColorMouseCheck( int originx, int originy, int* index, int* grid_x,
                        int* grid_y, int centered )
{
  int color_grid_w = 6;
  int color_grid_h = 8;

  e_GetCellAtMouse( color_grid_w, color_grid_h,
                    originx, originy,
                    GLYPH_WIDTH, GLYPH_HEIGHT,
                    grid_x, grid_y, centered );

  *index = *grid_y * color_grid_w + *grid_x;
}

void e_LoadColorPalette( aColor_t palette[MAX_COLOR_GROUPS][MAX_COLOR_PALETTE],
                       const char * filename )
{
  FILE* file;
  char line[8];
  unsigned int hex_value;
  uint8_t r, g, b;
  int i = 0;

  file = fopen( filename, "rb" );
  if ( file == NULL )
  {
    printf( "Failed to open file %s\n", filename );
    return;
  } 

  while( fgets( line, sizeof( line ), file ) != NULL )
  {
    hex_value = ( unsigned int )strtol( line, NULL, 16 );
    r = hex_value >> 16;
    g = hex_value >> 8;
    b = hex_value;
    
    if ( i >= 0 && i < MAX_COLOR_PALETTE )
    {
      palette[APOLLO_PALETE][i].r = r;
      palette[APOLLO_PALETE][i].g = g;
      palette[APOLLO_PALETE][i].b = b;
      palette[APOLLO_PALETE][i].a = 255;

    }

    i++;

  }

  fclose( file );
}

GlyphArray_t* e_InitGlyphs( const char* filename, int glyph_width,
                            int glyph_height )
{
  SDL_Surface* surface, *glyph_surf;
  SDL_Rect dest, rect;
  
  GlyphArray_t* new_glyphs = ( GlyphArray_t* )malloc( sizeof( GlyphArray_t ) );
  if ( new_glyphs == NULL )
  {
    printf( "Failed to allocate memory for new_glyphs %s\n", filename );
    return NULL;
  }
  new_glyphs->texture = NULL;
  new_glyphs->count = 0;
  
  glyph_surf = a_Image( filename );
  if( glyph_surf == NULL )
  {
    printf( "Failed to open font surface %s, %s", filename, SDL_GetError() );
    return NULL;
  }

  surface = SDL_CreateRGBSurface( 0, FONT_TEXTURE_SIZE, FONT_TEXTURE_SIZE, 32,
                                  0, 0, 0, 0xff );

  SDL_SetColorKey( surface, SDL_TRUE, SDL_MapRGBA( surface->format, 0, 0, 0,
                                                   0 ) );

  dest.x = dest.y = 0;
  rect.x = rect.y = 0;
  rect.w = dest.w = glyph_width;
  rect.h = dest.h = glyph_height;

  while ( rect.x < glyph_surf->w )
  {
    if ( dest.x + dest.w >= GAME_GLYPH_TEXTURE_SIZE )
    {
      dest.x = 0;
      dest.y += dest.h + 1;
      if ( dest.y + dest.h >= GAME_GLYPH_TEXTURE_SIZE )
      {
        SDL_LogMessage( SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_CRITICAL,
                        "Out of glyph space in %dx%d font atlas texture map.",
                        FONT_TEXTURE_SIZE, FONT_TEXTURE_SIZE );
        return NULL;
      }
    }

    SDL_BlitSurface( glyph_surf, &rect, surface, &dest );
    
    new_glyphs->rects[new_glyphs->count++] = dest;

    dest.x += dest.w;
    rect.x += rect.w;
  }

  new_glyphs->texture = a_ToTexture( surface, 1 );
  
  return new_glyphs;
}
