
#include <stdio.h>
#include <stdlib.h>

#include <Archimedes.h>
#include <Daedalus.h>

#include "ed_defines.h"
#include "ed_structs.h"

#include "world_editor.h"

void e_GetOrigin( World_t* map, int* originx, int* originy )
{
  if ( !map ) return;
  *originx = app.g_viewport.x - ( (float)( map->width  * map->tile_w ) / 2 );
  *originy = app.g_viewport.y - ( (float)( map->height * map->tile_h ) / 2 );
}

void e_GetCellAtMouseInViewport( const int width,   const int height,
                                 const int originx, const int originy,
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

void e_GetCellAtMouse( const int width,      const int height,
                       const int originx,    const int originy,
                       const int cell_width, const int cell_height,
                       int* grid_x,    int* grid_y, const int centered )
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

void e_TilesetMouseCheck( const int originx, const int originy, int* index,
                      int* grid_x, int* grid_y, const int centered )
{

  int row = tile_sets[current_tileset]->row;
  int col = tile_sets[current_tileset]->col;
  int tile_w = tile_sets[current_tileset]->tile_w;
  int tile_h = tile_sets[current_tileset]->tile_h;

  e_GetCellAtMouse( col, row,
                    originx, originy,
                    tile_w, tile_h,
                    grid_x, grid_y, centered );

  *index = *grid_y * col + *grid_x;
}

void e_GlyphMouseCheck( const int originx, const int originy, int* index,
                        int* grid_x, int* grid_y, const int centered )
{
  int glyph_grid_w = 16;
  int glyph_grid_h = 16;

  e_GetCellAtMouse( glyph_grid_w, glyph_grid_h,
                    originx, originy,
                    GLYPH_WIDTH, GLYPH_HEIGHT,
                    grid_x, grid_y, centered );

  *index = *grid_y * glyph_grid_w + *grid_x;
}

void e_ColorMouseCheck( const int originx, const int originy, int* index,
                        int* grid_x, int* grid_y, const int centered )
{
  int color_grid_w = 6;
  int color_grid_h = 8;

  e_GetCellAtMouse( color_grid_w, color_grid_h,
                    originx, originy,
                    GLYPH_WIDTH, GLYPH_HEIGHT,
                    grid_x, grid_y, centered );

  *index = *grid_y * color_grid_w + *grid_x;
}

Tileset_t* e_TilesetCreate( const char* filename,
                             const int tile_w, const int tile_h )
{
  aSpriteSheet_t* temp_sheet = a_SpriteSheetCreate( filename, tile_w, tile_h );
  
  Tileset_t* new_set = malloc( sizeof( Tileset_t ) );
  if ( new_set == NULL ) return NULL;

  new_set->img_array = malloc( sizeof( ImageArray_t ) * temp_sheet->img_count );
  if ( new_set->img_array == NULL ) return NULL;
  
  new_set->glyph = malloc( sizeof( uint16_t ) * temp_sheet->img_count );
  if ( new_set->glyph == NULL ) 
  {
    free( new_set->glyph );
    free( new_set );
    return NULL;
  }

  new_set->tile_count = temp_sheet->img_count;
  new_set->row = temp_sheet->h_count;
  new_set->col = temp_sheet->v_count;
  new_set->tile_w = tile_w;
  new_set->tile_h = tile_h;

  for ( int i = 0; i < temp_sheet->img_count; i++ )
  {
    int row = i % temp_sheet->v_count;
    int col = i / temp_sheet->v_count;
    new_set->img_array[i].img = a_ImageFromSpriteSheet( temp_sheet, row, col );
    new_set->glyph[i] = 0;
  }

  return new_set;
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

World_t* convert_mats_worlds( const char* filename )
{
  int file_size = 0;
  int newline_count = 0;
  char* file_string;
  char** lines;
  
  int world_width  = 0;
  int world_height = 0;

  World_t* new_world = malloc( sizeof( World_t ) );
  if ( new_world == NULL ) return NULL;

  file_string = a_ReadFile( filename, &file_size );
  
  newline_count = a_CountNewLines( file_string, file_size );

  lines = a_ParseLinesInFile( file_string, file_size, newline_count );
   
  char* string = lines[0];
  if ( string != NULL && string[0] == '/' && string[1] == '/' )
  {
    
  }


  return new_world;
}

