/*
 * world_editor.c:
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
#include "entity_editor.h"
#include "items_editor.h"
#include "utils.h"
#include "world.h"
#include "world_editor.h"

static void e_WorldEditorLogic( float );
static void e_WorldEditorDraw( float );

World_t* map = NULL;

static aPoint2f_t selected_pos;
static aPoint2f_t highlighted_pos;
char* pos_text;

static int originx = 0;
static int originy = 0;

static int toggle_ascii = 0;
aTileset_t* tile_set = NULL;

void e_WorldEditorInit( void )
{
  app.delegate.logic = e_WorldEditorLogic;
  app.delegate.draw  = e_WorldEditorDraw;
  
  tile_set = a_TilesetCreate( "resources/assets/level01tilemap.png", 16, 16 );
 
  //originx = EDITOR_WORLD_WIDTH /2;
  //originy = EDITOR_WORLD_HEIGHT/2;
  if ( map != NULL )
  {
    originx = app.g_viewport.x - ( (float)( map->width  * map->tile_w ) / 2 );
    originy = app.g_viewport.y - ( (float)( map->height * map->tile_h ) / 2 );
  }
 
  float ratio = SCREEN_WIDTH/SCREEN_HEIGHT;
  float view_h = 50.0f;
  float view_w = view_h * ratio;
  app.g_viewport = (aRectf_t){ 512.0f, 512.0f, view_h, view_w };

  pos_text = malloc( sizeof(char) * 50 );

  selected_pos = ( aPoint2f_t ){ .x = 0, .y = 0 };

  snprintf( pos_text, 50, "%f,%f\n", selected_pos.x, selected_pos.y );

  a_WidgetsInit( "resources/widgets/editor/world.auf" );
  app.active_widget = a_GetWidget( "tab_bar" );

  aContainerWidget_t* tab_container = a_GetContainerFromWidget( "tab_bar" );
  for ( int i = 0; i < tab_container->num_components; i++ )
  {
    aWidget_t* current = &tab_container->components[i];

    if ( strcmp( current->name, "world" ) == 0 )
    {
      current->action = e_WorldEditorInit;
    }

    if ( strcmp( current->name, "item" ) == 0 )
    {
      current->action = e_ItemEditorInit;
    }

    if ( strcmp( current->name, "entity" ) == 0 )
    {
      current->action = e_EntityEditorInit;
    }
  }

  aContainerWidget_t* world_menu_container = a_GetContainerFromWidget("world_menu_bar");
  for ( int i = 0; i < world_menu_container->num_components; i++ )
  {
    aWidget_t* current = &world_menu_container->components[i];

    if ( strcmp( current->name, "creation" ) == 0 )
    {
      current->action = we_Creation;
    }

    if ( strcmp( current->name, "edit" ) == 0 )
    {
      current->action = we_Edit;
    }

    if ( strcmp( current->name, "save" ) == 0 )
    {
      current->action = we_Save;
    }

    if ( strcmp( current->name, "load" ) == 0 )
    {
      current->action = we_Load;
    }
  }

}

static void e_WorldEditorLogic( float dt )
{
  a_DoInput();
  
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    EditorInit();
  }
  
  a_ViewportInput( &app.g_viewport, EDITOR_WORLD_WIDTH, EDITOR_WORLD_HEIGHT );
  
  a_DoWidget();
}

static void e_WorldEditorDraw( float dt )
{
  if ( map != NULL )
  {
    WorldDraw( originx, originy, map, tile_set, toggle_ascii );
  }

  a_DrawWidgets();
}

