#include <stdio.h>
#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "draw_utils.h"
#include "persist.h"
#include "settings.h"
#include "main_menu.h"
#include "sound_manager.h"

static void st_Logic( float );
static void st_Draw( float );

#define NUM_SETTINGS 3
#define ITEM_H       32.0f
#define ITEM_SPACING  6.0f
#define VOL_STEP      5

enum { SET_GFX, SET_MUSIC, SET_SFX };

static int cursor;
static int back_hovered;
static int prev_mouse_down;

static const char* setting_labels[] = { "Graphics", "Music Volume", "SFX Volume" };

static aSoundEffect_t sfx_move;
static aSoundEffect_t sfx_click;

/* --- helpers --- */

static const char* st_GetValueStr( int row, char* buf, int buflen )
{
  switch ( row )
  {
    case SET_GFX:
      return settings.gfx_mode == GFX_IMAGE ? "Image" : "ASCII";
    case SET_MUSIC:
      snprintf( buf, buflen, "%d%%", settings.music_vol );
      return buf;
    case SET_SFX:
      snprintf( buf, buflen, "%d%%", settings.sfx_vol );
      return buf;
  }
  return "";
}

static void st_Adjust( int row, int dir )
{
  switch ( row )
  {
    case SET_GFX:
      settings.gfx_mode = !settings.gfx_mode;
      break;
    case SET_MUSIC:
      settings.music_vol += dir * VOL_STEP;
      if ( settings.music_vol < 0 )   settings.music_vol = 0;
      if ( settings.music_vol > 100 ) settings.music_vol = 100;
      SoundManagerSetMusicVolume( settings.music_vol );
      break;
    case SET_SFX:
      settings.sfx_vol += dir * VOL_STEP;
      if ( settings.sfx_vol < 0 )   settings.sfx_vol = 0;
      if ( settings.sfx_vol > 100 ) settings.sfx_vol = 100;
      SoundManagerSetSfxVolume( settings.sfx_vol );
      break;
  }
}

static void st_Save( void )
{
  char buf[64];
  snprintf( buf, sizeof( buf ), "%d\n%d\n%d",
            settings.gfx_mode, settings.music_vol, settings.sfx_vol );
  PersistSave( "settings", buf );
}

static void st_Leave( void )
{
  st_Save();
  a_WidgetCacheFree();
  MainMenuInit();
}

/* --- init --- */

void SettingsInit( void )
{
  app.delegate.logic = st_Logic;
  app.delegate.draw  = st_Draw;

  app.options.scale_factor = 1;

  cursor          = 0;
  back_hovered    = 0;
  prev_mouse_down = 0;

  a_AudioLoadSound( "resources/soundeffects/menu_move.wav", &sfx_move );
  a_AudioLoadSound( "resources/soundeffects/menu_click.wav", &sfx_click );

  a_WidgetsInit( "resources/widgets/settings.auf" );
}

/* --- logic --- */

static void st_Logic( float dt )
{
  a_DoInput();

  /* ESC — save and leave */
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    a_AudioPlaySound( &sfx_click, NULL );
    st_Leave();
    return;
  }

  /* Hot reload */
  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/settings.auf" );
  }

  /* Up / Down — move cursor */
  if ( app.keyboard[A_W] == 1 || app.keyboard[A_UP] == 1 )
  {
    app.keyboard[A_W] = 0;
    app.keyboard[A_UP] = 0;
    cursor = ( cursor - 1 + NUM_SETTINGS ) % NUM_SETTINGS;
    a_AudioPlaySound( &sfx_move, NULL );
  }

  if ( app.keyboard[A_S] == 1 || app.keyboard[A_DOWN] == 1 )
  {
    app.keyboard[A_S] = 0;
    app.keyboard[A_DOWN] = 0;
    cursor = ( cursor + 1 ) % NUM_SETTINGS;
    a_AudioPlaySound( &sfx_move, NULL );
  }

  /* Left / Right — adjust value */
  if ( app.keyboard[A_A] == 1 || app.keyboard[A_LEFT] == 1 )
  {
    app.keyboard[A_A] = 0;
    app.keyboard[A_LEFT] = 0;
    st_Adjust( cursor, -1 );
    a_AudioPlaySound( &sfx_click, NULL );
  }

  if ( app.keyboard[A_D] == 1 || app.keyboard[A_RIGHT] == 1 )
  {
    app.keyboard[A_D] = 0;
    app.keyboard[A_RIGHT] = 0;
    st_Adjust( cursor, 1 );
    a_AudioPlaySound( &sfx_click, NULL );
  }

  /* Enter / Space — toggle (gfx row) */
  if ( app.keyboard[SDL_SCANCODE_RETURN] == 1 ||
       app.keyboard[SDL_SCANCODE_SPACE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_RETURN] = 0;
    app.keyboard[SDL_SCANCODE_SPACE] = 0;
    if ( cursor == SET_GFX )
    {
      st_Adjust( cursor, 1 );
      a_AudioPlaySound( &sfx_click, NULL );
    }
  }

  /* ---- Mouse ---- */
  int mx = app.mouse.x;
  int my = app.mouse.y;
  int mouse_down = app.mouse.pressed && app.mouse.button == SDL_BUTTON_LEFT;
  int clicked = mouse_down && !prev_mouse_down;
  prev_mouse_down = mouse_down;

  /* Back button */
  {
    aContainerWidget_t* bc = a_GetContainerFromWidget( "settings_back" );
    aRectf_t br = bc->rect;
    int hit = PointInRect( mx, my, br.x, br.y, br.w, br.h );

    if ( hit && !back_hovered )
      a_AudioPlaySound( &sfx_move, NULL );
    back_hovered = hit;

    if ( hit && clicked )
    {
      a_AudioPlaySound( &sfx_click, NULL );
      st_Leave();
      return;
    }
  }

  /* Setting rows */
  {
    aContainerWidget_t* pc = a_GetContainerFromWidget( "settings_panel" );
    aRectf_t r = pc->rect;
    float by = r.y + 16;

    for ( int i = 0; i < NUM_SETTINGS; i++ )
    {
      float bx  = r.x + 4;
      float byi = by + i * ( ITEM_H + ITEM_SPACING );

      int hit = PointInRect( mx, my, bx, byi, r.w - 8, ITEM_H );
      if ( hit )
      {
        if ( cursor != i )
        {
          cursor = i;
          a_AudioPlaySound( &sfx_move, NULL );
        }

        if ( clicked )
        {
          /* Left half = decrease, right half = increase */
          float mid = bx + ( r.w - 8 ) / 2.0f;
          int dir = ( mx < mid ) ? -1 : 1;
          st_Adjust( i, dir );
          a_AudioPlaySound( &sfx_click, NULL );
        }
      }
    }
  }
}

/* --- draw --- */

static void st_Draw( float dt )
{
  aColor_t bg_norm  = { 0x10, 0x14, 0x1f, 255 };
  aColor_t bg_hover = { 0x20, 0x2e, 0x37, 255 };
  aColor_t fg_norm  = { 0x81, 0x97, 0x96, 255 };
  aColor_t fg_hover = { 0xc7, 0xcf, 0xcc, 255 };
  aColor_t gold     = { 0xde, 0x9e, 0x41, 255 };
  aColor_t dim      = { 0x81, 0x97, 0x96, 120 };

  /* Title */
  {
    aContainerWidget_t* tc = a_GetContainerFromWidget( "settings_title" );
    aRectf_t tr = tc->rect;

    aTextStyle_t ts = a_default_text_style;
    ts.fg    = gold;
    ts.bg    = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 2.0f;
    ts.align = TEXT_ALIGN_CENTER;
    a_DrawText( "Settings", (int)( tr.x + tr.w / 2.0f ),
                (int)( tr.y + tr.h / 2.0f ), ts );
  }

  /* Panel */
  {
    aContainerWidget_t* pc = a_GetContainerFromWidget( "settings_panel" );
    aRectf_t r = pc->rect;

    a_DrawFilledRect( r, (aColor_t){ 0x08, 0x0a, 0x10, 200 } );

    float by = r.y + 16;
    for ( int i = 0; i < NUM_SETTINGS; i++ )
    {
      int sel = ( cursor == i );
      float bx  = r.x + 4;
      float byi = by + i * ( ITEM_H + ITEM_SPACING );
      float bw  = r.w - 8;

      /* Row background */
      aRectf_t row_r = { bx, byi, bw, ITEM_H };
      a_DrawFilledRect( row_r, sel ? bg_hover : bg_norm );

      aTextStyle_t ts = a_default_text_style;
      ts.bg    = (aColor_t){ 0, 0, 0, 0 };
      ts.scale = 1.2f;

      /* Label — left aligned */
      ts.fg    = sel ? fg_hover : fg_norm;
      ts.align = TEXT_ALIGN_LEFT;
      a_DrawText( setting_labels[i],
                  (int)( bx + 8 ),
                  (int)( byi + ITEM_H / 2.0f ), ts );

      /* Value — right aligned with arrows when selected */
      char vbuf[16];
      const char* val = st_GetValueStr( i, vbuf, sizeof( vbuf ) );

      if ( sel )
      {
        char display[32];
        snprintf( display, sizeof( display ), "< %s >", val );
        ts.fg    = gold;
        ts.align = TEXT_ALIGN_RIGHT;
        a_DrawText( display,
                    (int)( bx + bw - 8 ),
                    (int)( byi + ITEM_H / 2.0f ), ts );
      }
      else
      {
        ts.fg    = fg_norm;
        ts.align = TEXT_ALIGN_RIGHT;
        a_DrawText( val,
                    (int)( bx + bw - 8 ),
                    (int)( byi + ITEM_H / 2.0f ), ts );
      }
    }

    /* Panel border */
    a_DrawRect( r, gold );
  }

  /* Back button */
  {
    aContainerWidget_t* bc = a_GetContainerFromWidget( "settings_back" );
    aRectf_t br = bc->rect;
    DrawButton( br.x, br.y, br.w, br.h, "Back [ESC]", 1.4f, back_hovered,
                bg_norm, bg_hover, fg_norm, fg_hover );
  }

  /* Hint */
  {
    aContainerWidget_t* hc = a_GetContainerFromWidget( "settings_hint" );
    aRectf_t hr = hc->rect;
    aTextStyle_t ts = a_default_text_style;
    ts.fg    = dim;
    ts.bg    = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 1.0f;
    ts.align = TEXT_ALIGN_CENTER;
    a_DrawText( "[LEFT/RIGHT] Adjust  [ESC] Back",
                (int)( hr.x + hr.w / 2.0f ),
                (int)( hr.y + hr.h / 2.0f ), ts );
  }
}
