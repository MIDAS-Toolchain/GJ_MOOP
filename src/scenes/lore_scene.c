#include <stdio.h>
#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "draw_utils.h"
#include "lore.h"
#include "lore_scene.h"
#include "main_menu.h"

static void ls_Logic( float );
static void ls_Draw( float );

#define ITEM_H       32.0f
#define ITEM_SPACING  6.0f
#define MAX_CATS      16

/* Panel focus: 0 = categories, 1 = entries */
static int focus_panel;
static int cat_cursor;
static int entry_cursor;

static char   categories[MAX_CATS][32];
static int    num_categories;

static int back_hovered;

static aSoundEffect_t sfx_move;
static aSoundEffect_t sfx_click;

/* Helpers */
static int entries_in_category( const char* cat )
{
  int n = 0;
  for ( int i = 0; i < LoreGetCount(); i++ )
  {
    LoreEntry_t* le = LoreGetEntry( i );
    if ( strcmp( le->category, cat ) == 0 ) n++;
  }
  return n;
}

static LoreEntry_t* get_entry_in_cat( const char* cat, int index )
{
  int n = 0;
  for ( int i = 0; i < LoreGetCount(); i++ )
  {
    LoreEntry_t* le = LoreGetEntry( i );
    if ( strcmp( le->category, cat ) == 0 )
    {
      if ( n == index ) return le;
      n++;
    }
  }
  return NULL;
}

void LoreSceneInit( void )
{
  app.delegate.logic = ls_Logic;
  app.delegate.draw  = ls_Draw;

  app.options.scale_factor = 1;

  focus_panel  = 0;
  cat_cursor   = 0;
  entry_cursor = 0;
  back_hovered = 0;

  num_categories = LoreGetCategories( categories, MAX_CATS );

  a_AudioLoadSound( "resources/soundeffects/menu_move.wav", &sfx_move );
  a_AudioLoadSound( "resources/soundeffects/menu_click.wav", &sfx_click );

  a_WidgetsInit( "resources/widgets/lore.auf" );
}

static void ls_Leave( void )
{
  a_WidgetCacheFree();
  MainMenuInit();
}

static void ls_Logic( float dt )
{
  a_DoInput();

  /* ESC — back to main menu */
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    ls_Leave();
    return;
  }

  /* Hot reload */
  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/lore.auf" );
  }

  /* Tab / left-right — switch panels */
  if ( app.keyboard[SDL_SCANCODE_TAB] == 1 ||
       app.keyboard[A_LEFT] == 1 || app.keyboard[A_RIGHT] == 1 )
  {
    app.keyboard[SDL_SCANCODE_TAB] = 0;
    app.keyboard[A_LEFT] = 0;
    app.keyboard[A_RIGHT] = 0;
    focus_panel = !focus_panel;
    a_AudioPlaySound( &sfx_click, NULL );

    /* Clamp entry cursor when switching to entries panel */
    if ( focus_panel == 1 && num_categories > 0 )
    {
      int count = entries_in_category( categories[cat_cursor] );
      if ( entry_cursor >= count ) entry_cursor = count > 0 ? count - 1 : 0;
    }
  }

  /* Up / Down */
  if ( app.keyboard[A_W] == 1 || app.keyboard[A_UP] == 1 )
  {
    app.keyboard[A_W] = 0;
    app.keyboard[A_UP] = 0;

    if ( focus_panel == 0 )
    {
      if ( num_categories > 0 )
        cat_cursor = ( cat_cursor - 1 + num_categories ) % num_categories;
      entry_cursor = 0;
    }
    else
    {
      int count = num_categories > 0 ?
                  entries_in_category( categories[cat_cursor] ) : 0;
      if ( count > 0 )
        entry_cursor = ( entry_cursor - 1 + count ) % count;
    }
    a_AudioPlaySound( &sfx_move, NULL );
  }

  if ( app.keyboard[A_S] == 1 || app.keyboard[A_DOWN] == 1 )
  {
    app.keyboard[A_S] = 0;
    app.keyboard[A_DOWN] = 0;

    if ( focus_panel == 0 )
    {
      if ( num_categories > 0 )
        cat_cursor = ( cat_cursor + 1 ) % num_categories;
      entry_cursor = 0;
    }
    else
    {
      int count = num_categories > 0 ?
                  entries_in_category( categories[cat_cursor] ) : 0;
      if ( count > 0 )
        entry_cursor = ( entry_cursor + 1 ) % count;
    }
    a_AudioPlaySound( &sfx_move, NULL );
  }

  /* Enter — switch to entries panel if on categories */
  if ( app.keyboard[SDL_SCANCODE_RETURN] == 1 ||
       app.keyboard[SDL_SCANCODE_SPACE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_RETURN] = 0;
    app.keyboard[SDL_SCANCODE_SPACE] = 0;

    if ( focus_panel == 0 )
    {
      focus_panel = 1;
      entry_cursor = 0;
      a_AudioPlaySound( &sfx_click, NULL );
    }
  }

  /* ---- Mouse ---- */

  int mx = app.mouse.x;
  int my = app.mouse.y;
  int clicked = app.mouse.pressed && app.mouse.button == SDL_BUTTON_LEFT;

  /* Back button */
  {
    aContainerWidget_t* bc = a_GetContainerFromWidget( "lore_back" );
    aRectf_t br = bc->rect;
    int hit = PointInRect( mx, my, br.x, br.y, br.w, br.h );

    if ( hit && !back_hovered )
      a_AudioPlaySound( &sfx_move, NULL );
    back_hovered = hit;

    if ( hit && clicked )
    {
      a_AudioPlaySound( &sfx_click, NULL );
      ls_Leave();
      return;
    }
  }

  /* Categories panel — mouse hover + click */
  if ( num_categories > 0 )
  {
    aContainerWidget_t* cc = a_GetContainerFromWidget( "lore_categories" );
    aRectf_t r = cc->rect;
    float by = r.y + 32;

    for ( int i = 0; i < num_categories; i++ )
    {
      float bx = r.x + 4;
      float byi = by + i * ( ITEM_H + ITEM_SPACING );

      int hit = PointInRect( mx, my, bx, byi, r.w - 8, ITEM_H );
      if ( hit )
      {
        if ( focus_panel != 0 || cat_cursor != i )
        {
          focus_panel = 0;
          if ( cat_cursor != i )
          {
            cat_cursor = i;
            entry_cursor = 0;
          }
          a_AudioPlaySound( &sfx_move, NULL );
        }

        if ( clicked )
        {
          focus_panel = 1;
          entry_cursor = 0;
          a_AudioPlaySound( &sfx_click, NULL );
        }
      }
    }
  }

  /* Entries panel — mouse hover + click */
  if ( num_categories > 0 )
  {
    aContainerWidget_t* ec = a_GetContainerFromWidget( "lore_entries" );
    aRectf_t r = ec->rect;
    const char* cat = categories[cat_cursor];
    int count = entries_in_category( cat );
    float by = r.y + 8;

    for ( int i = 0; i < count; i++ )
    {
      float bx = r.x + 4;
      float byi = by + i * ( ITEM_H + ITEM_SPACING );

      int hit = PointInRect( mx, my, bx, byi, r.w - 8, ITEM_H );
      if ( hit )
      {
        if ( focus_panel != 1 || entry_cursor != i )
        {
          focus_panel = 1;
          entry_cursor = i;
          a_AudioPlaySound( &sfx_move, NULL );
        }
      }
    }
  }
}

static void ls_Draw( float dt )
{
  aColor_t bg_norm  = { 0x10, 0x14, 0x1f, 255 };
  aColor_t bg_hover = { 0x20, 0x2e, 0x37, 255 };
  aColor_t fg_norm  = { 0x81, 0x97, 0x96, 255 };
  aColor_t fg_hover = { 0xc7, 0xcf, 0xcc, 255 };
  aColor_t gold     = { 0xde, 0x9e, 0x41, 255 };
  aColor_t dim      = { 0x81, 0x97, 0x96, 120 };

  /* Title */
  {
    aContainerWidget_t* tc = a_GetContainerFromWidget( "lore_title" );
    aRectf_t tr = tc->rect;

    char title[64];
    snprintf( title, sizeof( title ), "Lore (%d/%d)",
              LoreCountDiscovered(), LoreGetCount() );

    aTextStyle_t ts = a_default_text_style;
    ts.fg    = gold;
    ts.bg    = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 2.0f;
    ts.align = TEXT_ALIGN_CENTER;
    a_DrawText( title, (int)( tr.x + tr.w / 2.0f ),
                (int)( tr.y + tr.h / 2.0f ), ts );
  }

  /* Categories panel */
  {
    aContainerWidget_t* cc = a_GetContainerFromWidget( "lore_categories" );
    aRectf_t r = cc->rect;

    /* Panel bg */
    a_DrawFilledRect( r, (aColor_t){ 0x08, 0x0a, 0x10, 200 } );

    /* Panel label */
    aTextStyle_t lbl = a_default_text_style;
    lbl.fg    = dim;
    lbl.bg    = (aColor_t){ 0, 0, 0, 0 };
    lbl.scale = 1.0f;
    lbl.align = TEXT_ALIGN_LEFT;
    a_DrawText( "Categories", (int)( r.x + 8 ), (int)( r.y + 8 ), lbl );

    float by = r.y + 32;
    for ( int i = 0; i < num_categories; i++ )
    {
      int sel = ( focus_panel == 0 && cat_cursor == i );
      int active = ( cat_cursor == i );

      /* Category item with discovered count */
      char buf[64];
      int disc = LoreCountInCategory( categories[i] );
      int total = entries_in_category( categories[i] );
      snprintf( buf, sizeof( buf ), "%s (%d/%d)", categories[i], disc, total );

      DrawButton( r.x + 4, by, r.w - 8, ITEM_H, buf, 1.2f, sel,
                  active ? bg_hover : bg_norm, bg_hover, fg_norm, fg_hover );
      by += ITEM_H + ITEM_SPACING;
    }

    /* Focus indicator */
    if ( focus_panel == 0 )
      a_DrawRect( r, gold );
  }

  /* Entries panel */
  {
    aContainerWidget_t* ec = a_GetContainerFromWidget( "lore_entries" );
    aRectf_t r = ec->rect;

    /* Panel bg */
    a_DrawFilledRect( r, (aColor_t){ 0x08, 0x0a, 0x10, 200 } );

    if ( num_categories == 0 )
    {
      aTextStyle_t ts = a_default_text_style;
      ts.fg    = dim;
      ts.bg    = (aColor_t){ 0, 0, 0, 0 };
      ts.scale = 1.2f;
      ts.align = TEXT_ALIGN_CENTER;
      a_DrawText( "No lore discovered yet.",
                  (int)( r.x + r.w / 2.0f ),
                  (int)( r.y + r.h / 2.0f ), ts );
    }
    else
    {
      const char* cat = categories[cat_cursor];
      int count = entries_in_category( cat );

      /* Entry list — top half */
      float by = r.y + 8;
      for ( int i = 0; i < count; i++ )
      {
        LoreEntry_t* le = get_entry_in_cat( cat, i );
        if ( !le ) continue;

        int sel = ( focus_panel == 1 && entry_cursor == i );
        const char* display = le->discovered ? le->title : "???";

        DrawButton( r.x + 4, by, r.w - 8, ITEM_H, display, 1.2f, sel,
                    bg_norm, bg_hover, fg_norm, fg_hover );
        by += ITEM_H + ITEM_SPACING;
      }

      /* Description — below entry list */
      if ( count > 0 )
      {
        LoreEntry_t* sel_entry = get_entry_in_cat( cat, entry_cursor );
        if ( sel_entry && sel_entry->discovered )
        {
          float desc_y = by + 16;

          /* Separator line */
          aRectf_t sep = { r.x + 16, desc_y, r.w - 32, 1 };
          a_DrawFilledRect( sep, dim );

          float desc_w = r.w - 32;
          aTextStyle_t ts = a_default_text_style;
          ts.fg    = fg_hover;
          ts.bg    = (aColor_t){ 0, 0, 0, 0 };
          ts.scale = 1.0f;
          ts.align = TEXT_ALIGN_LEFT;
          ts.wrap_width = (int)desc_w;
          a_DrawText( sel_entry->description,
                      (int)( r.x + 16 ), (int)( desc_y + 12 ), ts );
        }
      }
    }

    /* Focus indicator */
    if ( focus_panel == 1 )
      a_DrawRect( r, gold );
  }

  /* Back button */
  {
    aContainerWidget_t* bc = a_GetContainerFromWidget( "lore_back" );
    aRectf_t br = bc->rect;
    DrawButton( br.x, br.y, br.w, br.h, "Back [ESC]", 1.4f, back_hovered,
                bg_norm, bg_hover, fg_norm, fg_hover );
  }

  /* Hint */
  {
    aContainerWidget_t* hc = a_GetContainerFromWidget( "lore_hint" );
    aRectf_t hr = hc->rect;
    aTextStyle_t ts = a_default_text_style;
    ts.fg    = dim;
    ts.bg    = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 1.0f;
    ts.align = TEXT_ALIGN_CENTER;
    a_DrawText( "[TAB] Switch Panel",
                (int)( hr.x + hr.w / 2.0f ),
                (int)( hr.y + hr.h / 2.0f ), ts );
  }
}
