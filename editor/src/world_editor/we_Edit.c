/*
 * world_editor/we_Edit.c:
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
#include "utils.h"
#include "world_editor.h"

static dVec2_t selected_pos;
static dVec2_t highlighted_pos;
static int glyph_index = 0;
static int fg_index = 0;
static int bg_index = 0;
static uint8_t selected_glyph_x = 0, selected_glyph_y = 0;
static uint8_t selected_fg_x = 0, selected_fg_y = 0;
static uint8_t selected_bg_x = 0, selected_bg_y = 0;
static int editor_mode = 0;
static Tile_t* clipboard = NULL;

static int originx = 0;
static int originy = 0;

static void we_EditLogic( float dt );
static void we_EditDraw( float dt );

char* wem_strings[WEM_MAX+1] =
{
  "WEM_NONE",
  "WEM_BRUSH",
  "WEM_COPY",
  "WEM_PASTE",
  "WEM_MASS_CHANGE",
  "WEM_SELECT",
  "WEM_MAX"
};

void we_Edit( void )
{
  app.delegate.logic = we_EditLogic;
  app.delegate.draw  = we_EditDraw;

  e_GetOrigin( map, &originx, &originy );

  app.active_widget = a_GetWidget( "generation_menu" );
  aContainerWidget_t* container = a_GetContainerFromWidget( "generation_menu" );
  app.active_widget->hidden = 1;

  for ( int i = 0; i < container->num_components; i++ )
  {
    aWidget_t* current = &container->components[i];
    current->hidden = 1;

  }
}

static void we_EditLogic( float dt )
{
  a_DoInput();
  
  if ( map != NULL )
  {
    int grid_x = 0, grid_y = 0;
    e_GetCellAtMouseInViewport( map->width, map->height,
                     originx, originy, &grid_x, &grid_y );
    if ( editor_mode == WEM_SELECT )

    if ( app.mouse.button == 1 )
    {
      app.mouse.button = 0;
      int index = grid_y * map->width + grid_x;
      map->background[index].tile = 1;
    }
    
    if ( app.mouse.button == 3 )
    {
      app.mouse.button = 0;
      int index = grid_y * map->width + grid_x;
      map->background[index].tile = 0;
    }
  }
  
  if ( app.keyboard[A_S] == 1 )
  {
    app.keyboard[A_S] = 0;

    editor_mode = WEM_SELECT;
  }
  
  if ( app.keyboard[A_X] == 1 )
  {
    app.keyboard[A_X] = 0;

    editor_mode = WEM_MASS_CHANGE;
  }

  if ( app.keyboard[A_C] == 1 )
  {
    app.keyboard[A_C] = 0;

    editor_mode = WEM_COPY;
  }
  
  if ( app.keyboard[A_V] == 1 )
  {
    app.keyboard[A_V] = 0;

    editor_mode = WEM_PASTE;
  }
  
  if ( app.keyboard[A_B] == 1 )
  {
    app.keyboard[A_B] = 0;

    editor_mode = WEM_BRUSH;
  }
  
  a_DoWidget();
}

static void we_EditDraw( float dt )
{

  a_DrawWidgets();
}

