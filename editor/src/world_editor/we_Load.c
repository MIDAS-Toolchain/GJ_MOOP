/*
 * world_editor/we_Load.c:
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
#include "world.h"
#include "world_editor.h"
#include "spawn_data.h"

extern SpawnList_t g_edit_spawns;
extern int         g_spawns_loaded;

static void wel_LoadLogic( float dt );
static void wel_LoadDraw( float dt );
static void load_file_action( void );

static int pending_load = 0;

void we_Load( void )
{
  app.delegate.logic = wel_LoadLogic;
  app.delegate.draw  = wel_LoadDraw;

  pending_load = 0;

  a_ContainerClearComponents( "load_menu" );

  for ( size_t i = 0; i < g_map_filenames->count; i++ )
  {
    char* name = (char*)d_ArrayGet( g_map_filenames, i );
    char btn_name[64];
    snprintf( btn_name, sizeof(btn_name), "load_%zu", i );
    a_ContainerAddButton( "load_menu", btn_name, name, load_file_action );
  }

  aWidget_t* mode = a_GetWidget( "mode_bar" );
  aWidget_t* toggle = a_GetWidget( "toggle_bar" );
  if ( mode )   mode->hidden = 1;
  if ( toggle ) toggle->hidden = 1;

  app.active_widget = a_GetWidget( "load_menu" );
  app.active_widget->hidden = 0;
}

static void load_file_action( void )
{
  pending_load = 1;
}

static void wel_LoadLogic( float dt )
{
  a_DoInput();

  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    e_WorldEditorInit();
    return;
  }

  a_DoWidget();

  if ( pending_load )
  {
    pending_load = 0;
    aContainerWidget_t* bc = a_GetContainerFromWidget( "load_menu" );
    if ( bc && bc->focus_index >= 0 && bc->focus_index < bc->num_components )
    {
      char* name = (char*)d_ArrayGet( g_map_filenames, bc->focus_index );
      if ( name )
        wel_LoadYes( name );
    }
  }
}

static void wel_LoadDraw( float dt )
{
  a_DrawWidgets();
}

void wel_LoadYes( char* name )
{
  STRNCPY(g_current_filename, name, MAX_FILENAME_LENGTH);

  if ( g_current_filename != NULL )
  {
    if ( g_map != NULL )
    {
      WorldDestroy( g_map );
    }

    g_map = convert_mats_worlds( g_current_filename );

    /* Auto-load companion consumable spawns DUF if it exists */
    if ( g_spawns_loaded )
      SpawnListDestroy( &g_edit_spawns );

    SpawnListInit( &g_edit_spawns );
    char duf_path[256];
    SpawnDUFFilename( g_current_filename, duf_path, sizeof(duf_path) );
    if ( SpawnDUFLoad( duf_path, &g_edit_spawns ) )
      printf( "EDITOR: loaded %d spawns from %s\n", g_edit_spawns.count, duf_path );
    g_spawns_loaded = 1;
  }

  e_WorldEditorInit();
}

void wel_LoadNo( void )
{
  e_WorldEditorInit();
}
