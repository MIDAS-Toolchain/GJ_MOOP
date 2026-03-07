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

static void wel_LoadLogic( float dt );
static void wel_LoadDraw( float dt );

#define BTN_H         42.0f
#define BTN_SPACING   14.0f

static size_t cursor = 0;
static size_t num_buttons = 0;

void we_Load( void )
{
  app.delegate.logic = wel_LoadLogic;
  app.delegate.draw  = wel_LoadDraw;
  
  num_buttons = g_map_filenames->count;
  app.active_widget = a_GetWidget( "load_menu" );

  app.active_widget->hidden = 0;
}

static void wel_LoadLogic( float dt )
{
  a_DoInput();

  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    e_WorldEditorInit();
  }
  
  aContainerWidget_t* bc = a_GetContainerFromWidget( "load_menu" );
  aRectf_t r = bc->rect;
  float btn_w = 500;
  float total_h = num_buttons * BTN_H + ( num_buttons - 1 ) * BTN_SPACING;
  float by = r.y + 100 + ( r.h - total_h ) / 2.0f;
  
  for ( size_t i = 0; i < num_buttons; i++ )
  {
    float bx = r.x - 200;
    float byi = by + i * ( BTN_H + BTN_SPACING );
    aRectf_t rect = { bx, byi, btn_w, BTN_H };
    int hit = WithinRange( app.mouse.x, app.mouse.y, rect );

    if ( hit )
    {
      cursor = i;
    }

    if ( hit && app.mouse.pressed && app.mouse.button == SDL_BUTTON_LEFT )
    {
      char* name = (char*)d_ArrayGet(g_map_filenames, i);
      wel_LoadYes(name);
      return;
    }
  }
  
  a_DoWidget();
}

static void wel_LoadDraw( float dt )
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

  a_DrawText( "Load?", 635, 270, fps_style );
  
  aContainerWidget_t* bc = a_GetContainerFromWidget( "load_menu" );
  aRectf_t r = bc->rect;
  float btn_w = 500;
  float total_h = num_buttons * BTN_H + ( num_buttons - 1 ) * BTN_SPACING;
  float by = r.y + 100 + ( r.h - total_h ) / 2.0f;

  aColor_t bg_norm  = { 0x10, 0x14, 0x1f, 255 };
  aColor_t bg_hover = { 0x20, 0x2e, 0x37, 255 };
  aColor_t fg_norm  = { 0x81, 0x97, 0x96, 255 };
  aColor_t fg_hover = { 0xc7, 0xcf, 0xcc, 255 };
  
  for ( size_t i = 0; i < g_map_filenames->count; i++ )
  {
    float bx = r.x - 200;
    float byi = by + i * ( BTN_H + BTN_SPACING );
    int sel = ( cursor == i );
    
    char* name = (char*)d_ArrayGet( g_map_filenames, i );

    DrawButton( bx, byi, btn_w, BTN_H, name, 1.5f, sel,
               bg_norm, bg_hover, fg_norm, fg_hover );
  }

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
  }
  
  e_WorldEditorInit();
}

void wel_LoadNo( void )
{
  e_WorldEditorInit();
}

