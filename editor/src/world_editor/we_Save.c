/*
 * world_editor/we_Save.c:
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

static void wes_SaveLogic( float dt );
static void wes_SaveDraw( float dt );

static int no = 0;
static int saved = 0;
static float saved_timer = 0.0f;

void we_Save( void )
{
  app.delegate.logic = wes_SaveLogic;
  app.delegate.draw  = wes_SaveDraw;
  
  no = 0;
  saved = 0;
  saved_timer = 0.0f;

  app.active_widget = a_GetWidget( "save_menu" );
  aContainerWidget_t* container =
    a_GetContainerFromWidget( "save_menu" );

  app.active_widget->hidden = 0;

  for ( int i = 0; i < container->num_components; i++ )
  {
    aWidget_t* current = &container->components[i];
    current->hidden = 0;

    if ( strcmp( current->name, "save_yes" ) == 0 )
    {
      current->action = wes_SaveYes;
    }
    
    if ( strcmp( current->name, "save_no" ) == 0 )
    {
      current->action = wes_SaveNo;
    }
  }
}

static void wes_SaveLogic( float dt )
{
  a_DoInput();

  if ( saved )
  {
    saved_timer += dt;
    if ( saved_timer >= 1.5f
         || app.keyboard[SDL_SCANCODE_ESCAPE] == 1
         || app.keyboard[SDL_SCANCODE_RETURN] == 1
         || app.mouse.button == SDL_BUTTON_LEFT )
    {
      app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
      app.keyboard[SDL_SCANCODE_RETURN] = 0;
      e_WorldEditorDestroy();
      we_Load();
      return;
    }
    return;
  }

  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    we_Edit();
    return;
  }

  a_DoWidget();
}

static void wes_SaveDraw( float dt )
{
  aTextStyle_t fps_style = {
    .type = FONT_CODE_PAGE_437,
    .fg = white,
    .bg = black,
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
  };
  
  if ( saved )
  {
    aTextStyle_t green_style = fps_style;
    green_style.fg = (aColor_t){ 80, 220, 80, 255 };
    a_DrawText( "Save Successful!", 635, 270, green_style );
  }
  else if ( !no )
  {
    a_DrawText( "Save?", 635, 270, fps_style );
    a_DrawWidgets();
  }
  else
  {
    a_DrawText( "Are you sure?", 635, 270, fps_style );
    a_DrawWidgets();
  }
}

void wes_SaveYes( void )
{
  if( !no )
  {
    if ( g_map != NULL && g_current_filename != NULL )
    {
      e_SaveWorld( g_map, g_current_filename );
    }

    if ( g_spawns_loaded && g_current_filename != NULL )
    {
      char duf_path[256];
      SpawnDUFFilename( g_current_filename, duf_path, sizeof(duf_path) );
      if ( SpawnDUFSave( duf_path, &g_edit_spawns ) )
        printf( "EDITOR: saved %d spawns to %s\n", g_edit_spawns.count, duf_path );
    }

    saved = 1;
    saved_timer = 0.0f;

    aWidget_t* save_w = a_GetWidget( "save_menu" );
    if ( save_w ) save_w->hidden = 1;
  }
  else
  {
    e_WorldEditorDestroy();
    EditorInit();
  }
}

void wes_SaveNo( void )
{
  if ( !no )
  {
    no = !no;
  }
}

