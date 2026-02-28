#include <stdio.h>
#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "player.h"
#include "items.h"
#include "draw_utils.h"
#include "console.h"
#include "game_events.h"
#include "inventory_ui.h"
#include "transitions.h"
#include "sound_manager.h"
#include "game_scene.h"
#include "main_menu.h"

static void gs_Logic( float );
static void gs_Draw( float );

/* Top bar layout */
#define TB_PAD_X        12.0f
#define TB_PAD_Y         8.0f
#define TB_NAME_SCALE    1.6f
#define TB_STAT_SCALE    1.2f
#define TB_STAT_GAP     12.0f   /* gap between each stat token */
#define TB_SECTION_GAP  32.0f   /* gap between name and stats */
#define TB_ESC_SCALE     1.0f

/* Viewport zoom limits (world units for height) */
#define ZOOM_MIN_H       50.0f
#define ZOOM_MAX_H       300.0f

/* Panel colors */
#define PANEL_BG  (aColor_t){ 0, 0, 0, 200 }
#define PANEL_FG  (aColor_t){ 255, 255, 255, 255 }

static int prev_mx = -1;
static int prev_my = -1;
static int mouse_moved = 0;

static aSoundEffect_t sfx_move;
static aSoundEffect_t sfx_click;

static Console_t console;

void GameSceneInit( void )
{
  app.delegate.logic = gs_Logic;
  app.delegate.draw  = gs_Draw;

  app.options.scale_factor = 1;

  a_WidgetsInit( "resources/widgets/game_scene.auf" );
  app.active_widget = a_GetWidget( "inv_panel" );

  /* Center viewport on player — w,h must match panel aspect ratio */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    float aspect = vp->rect.w / vp->rect.h;
    float vh = 200.0f;
    float vw = vh * aspect;
    app.g_viewport = (aRectf_t){
      player.world_x, player.world_y,
      vw, vh
    };
  }

  a_AudioLoadSound( "resources/soundeffects/menu_move.wav", &sfx_move );
  a_AudioLoadSound( "resources/soundeffects/menu_click.wav", &sfx_click );

  InventoryUIInit( &sfx_move, &sfx_click );

  ConsoleInit( &console );
  GameEventsInit( &console );
  ConsolePush( &console, "Welcome, adventurer.", white );

  SoundManagerPlayGame();
  TransitionIntroStart();
}

static void gs_Logic( float dt )
{
  a_DoInput();
  SoundManagerUpdate( dt );

  /* Track whether the mouse actually moved this frame */
  mouse_moved = ( app.mouse.x != prev_mx || app.mouse.y != prev_my );
  prev_mx = app.mouse.x;
  prev_my = app.mouse.y;

  /* Intro cinematic — blocks all input until done */
  if ( TransitionIntroActive() )
  {
    if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
    {
      app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
      TransitionIntroSkip();
    }
    else
    {
      TransitionIntroUpdate( dt );
    }

    /* Override viewport during intro */
    {
      aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
      float aspect = vp->rect.w / vp->rect.h;
      app.g_viewport.h = TransitionGetViewportH();
      app.g_viewport.w = app.g_viewport.h * aspect;
    }
    app.g_viewport.x = player.world_x;
    app.g_viewport.y = player.world_y;

    a_DoWidget();
    return;
  }

  /* Close action menu on Escape, or go to main menu if no menu open */
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    if ( !InventoryUICloseMenus() )
    {
      a_WidgetCacheFree();
      MainMenuInit();
      return;
    }
  }

  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/game_scene.auf" );
  }

  InventoryUILogic( mouse_moved );

  /* When inventory is focused, consume WASD/arrows so viewport only gets zoom */
  if ( InventoryUIFocused() )
  {
    app.keyboard[SDL_SCANCODE_W] = 0;
    app.keyboard[SDL_SCANCODE_S] = 0;
    app.keyboard[SDL_SCANCODE_A] = 0;
    app.keyboard[SDL_SCANCODE_D] = 0;
    app.keyboard[SDL_SCANCODE_UP] = 0;
    app.keyboard[SDL_SCANCODE_DOWN] = 0;
    app.keyboard[SDL_SCANCODE_LEFT] = 0;
    app.keyboard[SDL_SCANCODE_RIGHT] = 0;
  }

  /* Mouse wheel over console — scroll instead of zooming viewport */
  if ( app.mouse.wheel != 0 )
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    if ( PointInRect( app.mouse.x, app.mouse.y,
                      cp->rect.x, cp->rect.y, cp->rect.w, cp->rect.h ) )
    {
      ConsoleScroll( &console, app.mouse.wheel );
      app.mouse.wheel = 0;
    }
  }

  a_ViewportInput( &app.g_viewport, WORLD_WIDTH, WORLD_HEIGHT );

  /* Clamp zoom and fix aspect ratio */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    float aspect = vp->rect.w / vp->rect.h;
    if ( app.g_viewport.h < ZOOM_MIN_H ) app.g_viewport.h = ZOOM_MIN_H;
    if ( app.g_viewport.h > ZOOM_MAX_H ) app.g_viewport.h = ZOOM_MAX_H;
    app.g_viewport.w = app.g_viewport.h * aspect;
  }

  /* Always re-center viewport on player */
  app.g_viewport.x = player.world_x;
  app.g_viewport.y = player.world_y;

  a_DoWidget();
}

static void gs_DrawTopBar( void )
{
  aContainerWidget_t* tb = a_GetContainerFromWidget( "top_bar" );
  aRectf_t r = tb->rect;
  r.y += TransitionGetTopBarOY();
  float tb_alpha = TransitionGetUIAlpha();

  /* Background + border */
  aColor_t tb_bg = PANEL_BG;
  tb_bg.a = (int)( tb_bg.a * tb_alpha );
  aColor_t tb_fg = PANEL_FG;
  tb_fg.a = (int)( tb_fg.a * tb_alpha );
  a_DrawFilledRect( r, tb_bg );
  a_DrawRect( r, tb_fg );

  aTextStyle_t ts = a_default_text_style;
  ts.bg = (aColor_t){ 0, 0, 0, 0 };

  float tx = r.x + TB_PAD_X;
  float ty = r.y + TB_PAD_Y;

  /* Player name — left side, larger */
  ts.fg = white;
  ts.scale = TB_NAME_SCALE;
  ts.align = TEXT_ALIGN_LEFT;
  a_DrawText( player.name, (int)tx, (int)ty, ts );

  /* Measure name width to place stats right after it */
  float name_w = strlen( player.name ) * 8.0f * TB_NAME_SCALE;
  float sx = tx + name_w + TB_SECTION_GAP;

  /* Stats — flexed right of name, smaller */
  ts.scale = TB_STAT_SCALE;
  ts.align = TEXT_ALIGN_LEFT;
  char buf[32];

  /* HP — color based on percentage */
  float hp_pct = ( player.max_hp > 0 ) ? (float)player.hp / player.max_hp : 0;
  if ( hp_pct > 0.5f )       ts.fg = white;
  else if ( hp_pct > 0.25f ) ts.fg = yellow;
  else                        ts.fg = (aColor_t){ 255, 60, 60, 255 };

  snprintf( buf, sizeof( buf ), "HP: %d/%d", player.hp, player.max_hp );
  a_DrawText( buf, (int)sx, (int)( ty + 4 ), ts );
  sx += strlen( buf ) * 8.0f * TB_STAT_SCALE + TB_STAT_GAP;

  /* DMG */
  snprintf( buf, sizeof( buf ), "DMG: %d", player.base_damage );
  ts.fg = white;
  a_DrawText( buf, (int)sx, (int)( ty + 4 ), ts );
  sx += strlen( buf ) * 8.0f * TB_STAT_SCALE + TB_STAT_GAP;

  /* DEF */
  snprintf( buf, sizeof( buf ), "DEF: %d", player.defense );
  ts.fg = white;
  a_DrawText( buf, (int)sx, (int)( ty + 4 ), ts );

  /* Settings[ESC] — far right */
  ts.scale = TB_ESC_SCALE;
  ts.fg = (aColor_t){ 160, 160, 160, 255 };
  ts.align = TEXT_ALIGN_RIGHT;
  a_DrawText( "Settings[ESC]", (int)( r.x + r.w - TB_PAD_X ), (int)( ty + 6 ), ts );
}

static void gs_Draw( float dt )
{
  /* Draw manual backgrounds for boxed:0 panels */

  /* Top bar */
  gs_DrawTopBar();

  /* Game viewport — shrink 1px on right so it doesn't overlap right panels */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    aRectf_t vr = { vp->rect.x, vp->rect.y, vp->rect.w - 1, vp->rect.h };
    a_DrawFilledRect( vr, (aColor_t){ 0, 0, 0, 255 } );
    a_DrawRect( vr, PANEL_FG );
  }

  /* Inventory panel background */
  {
    aContainerWidget_t* ip = a_GetContainerFromWidget( "inv_panel" );
    aRectf_t ir = ip->rect;
    ir.x += TransitionGetRightOX();
    a_DrawFilledRect( ir, PANEL_BG );
  }

  /* Equipment panel background */
  {
    aContainerWidget_t* kp = a_GetContainerFromWidget( "key_panel" );
    aRectf_t kr = kp->rect;
    kr.x += TransitionGetRightOX();
    a_DrawFilledRect( kr, PANEL_BG );
  }

  /* Console panel */
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    aRectf_t cr = cp->rect;
    cr.y += TransitionGetConsoleOY();
    aColor_t con_bg = PANEL_BG;
    con_bg.a = (int)( con_bg.a * TransitionGetUIAlpha() );
    aColor_t con_fg = PANEL_FG;
    con_fg.a = (int)( con_fg.a * TransitionGetUIAlpha() );
    a_DrawFilledRect( cr, con_bg );
    a_DrawRect( cr, con_fg );
    ConsoleDraw( &console, cr );
  }

  if ( TransitionShowLabels() )
    a_DrawWidgets();

  InventoryUISetIntroOffset( TransitionGetRightOX(), TransitionGetUIAlpha() );
  InventoryUIDraw();

  /* Player character — always centered in game_viewport panel */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    aRectf_t clip = { vp->rect.x + 1, vp->rect.y + 1, vp->rect.w - 3, vp->rect.h - 2 };
    a_SetClipRect( clip );

    float scale = vp->rect.h / app.g_viewport.h;
    float sprite_sz = 16.0f * scale;
    float cx = vp->rect.x + vp->rect.w / 2.0f - sprite_sz / 2.0f;
    float cy = vp->rect.y + vp->rect.h / 2.0f - sprite_sz / 2.0f;

    if ( player.image && settings.gfx_mode == GFX_IMAGE )
      a_BlitRect( player.image, NULL, &(aRectf_t){ cx, cy, sprite_sz, sprite_sz }, 1.0f );
    else
      a_DrawFilledRect( (aRectf_t){ cx, cy, sprite_sz, sprite_sz }, white );

    a_DisableClipRect();
  }
}
