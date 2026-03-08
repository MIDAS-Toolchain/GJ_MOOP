/*
 * world_editor/we_Utils.c:
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
#include "tile.h"
#include "utils.h"
#include "world.h"
#include "world_editor.h"
#include "spawn_data.h"
#include "item_catalog.h"

void we_DrawColorPalette( int originx, int originy, int fg_index, int bg_index )
{
  int cx = 0, cy = 0;
  for ( int i = 0; i < MAX_COLOR_PALETTE; i++ )
  {

    if ( cx + GLYPH_WIDTH > ( 6 * GLYPH_WIDTH ) )
    {
      cx = 0;
      cy += GLYPH_HEIGHT;
      if ( cy > ( 6 * GLYPH_HEIGHT ) )
      {
        //        printf( "color grid is too large!\n" );
        //        return;
      }

    }
    aRectf_t color_palette_rect = (aRectf_t){ .x = ( cx + originx ), .y = ( cy + originy ),
    .w = GLYPH_WIDTH, .h = GLYPH_HEIGHT };
    a_DrawFilledRect( color_palette_rect, master_colors[APOLLO_PALETE][i] );

    if ( i == fg_index )
    {
      aRectf_t select_rect = {
        .x = color_palette_rect.x-1,
        .y = color_palette_rect.y-1,
        .w = color_palette_rect.w+1,
        .h = color_palette_rect.h+1
      };
      a_DrawRect( select_rect, magenta );
    }

    if ( i == bg_index )
    {
      aRectf_t select_rect = {
        .x = color_palette_rect.x-1,
        .y = color_palette_rect.y-1,
        .w = color_palette_rect.w+1,
        .h = color_palette_rect.h+1
      };
      a_DrawRect( select_rect, yellow );
    }

    cx += GLYPH_WIDTH;
  }
}

void we_DrawGlyphPalette( int originx, int originy, int glyph_index )
{
  int gx = 0, gy = 0;
  int width = 16, height = 20;
  for ( int i = 0; i < 254; i++ )
  {
    if ( gx + GLYPH_WIDTH > ( width * GLYPH_WIDTH ) )
    {
      gx = 0;
      gy += GLYPH_HEIGHT;
    }
    
    aRectf_t glyph_palette_rect = (aRectf_t){
      .x = ( gx + originx ),
      .y = ( gy + originy ),
      .w = app.glyphs[app.font_type][i].w,
      .h = app.glyphs[app.font_type][i].h
    };

    a_DrawGlyph_special( i, glyph_palette_rect,
                        white, black, FONT_CODE_PAGE_437 );
    
    if ( i == glyph_index )
    {
      a_DrawRect( glyph_palette_rect, magenta );
    }

    gx += GLYPH_WIDTH;
  }
}

/* Palette tiles: floor, wall, 4 doors (EW shown, auto-orient on place) */
static const int g_palette_tiles[] = {
  TILE_LVL1_FLOOR, TILE_LVL1_WALL,
  TILE_BLUE_DOOR_EW, TILE_GREEN_DOOR_EW, TILE_RED_DOOR_EW, TILE_WHITE_DOOR_EW
};
static const int g_palette_count = 6;

void we_DrawTilePalette( int originx, int originy, int tile_index, int tileset )
{
  float scale = 3.0f;
  int tw = g_tile_sets[tileset]->tile_w;
  int th = g_tile_sets[tileset]->tile_h;
  int sw = (int)( tw * scale );
  int sh = (int)( th * scale );
  int cols = 3;
  int pad = 2;

  for ( int s = 0; s < g_palette_count; s++ )
  {
    int ti = g_palette_tiles[s];
    int c = s % cols;
    int r = s / cols;
    int x = originx + c * ( sw + pad );
    int y = originy + r * ( sh + pad );

    aImage_t* img = g_tile_sets[tileset]->img_array[ti].img;
    a_BlitRect( img, NULL, &(aRectf_t){ x, y, img->rect.w, img->rect.h }, scale );

    if ( ti == tile_index )
    {
      aRectf_t rect = { .x = x, .y = y, .w = sw, .h = sh };
      a_DrawRect( rect, magenta );
    }
  }
}

void we_TilePaletteMouseCheck( int originx, int originy, int* out_index, int tileset )
{
  float scale = 3.0f;
  int tw = g_tile_sets[tileset]->tile_w;
  int th = g_tile_sets[tileset]->tile_h;
  int sw = (int)( tw * scale );
  int sh = (int)( th * scale );
  int cols = 3;
  int pad = 2;

  for ( int s = 0; s < g_palette_count; s++ )
  {
    int c = s % cols;
    int r = s / cols;
    int x = originx + c * ( sw + pad );
    int y = originy + r * ( sh + pad );

    if ( app.mouse.x >= x && app.mouse.x < x + sw
      && app.mouse.y >= y && app.mouse.y < y + sh )
    {
      *out_index = g_palette_tiles[s];
      return;
    }
  }
}

void we_MapMouseCheck( dVec2_t* pos, aRectf_t menu_rect )
{
  if ( g_map == NULL || pos == NULL ) return;
  int grid_x = 0, grid_y =0;
  e_GetCellAtMouseInViewport( g_map->width, g_map->height,
                              g_map->tile_w, g_map->tile_h,
                              menu_rect, 
                              g_map->originx, g_map->originy,
                              &grid_x, &grid_y );
  pos->x = grid_x;
  pos->y = grid_y;
}

void we_MassChange( World_t* world,
                    dVec2_t* selected_pos, dVec2_t* highlighted_pos,
                    uint16_t tile_index, uint8_t change_room, uint16_t room_id )
{

  int grid_w, grid_h;
  dVec2_t start_pos;

  GetSelectGridSize( selected_pos, highlighted_pos,
                     &grid_w, &grid_h, &start_pos );

  for ( int y = (int)start_pos.y; y < (int)start_pos.y + grid_h; y++ )
  {
    for ( int x = (int)start_pos.x; x < (int)start_pos.x + grid_w; x++ )
    {
      if( x >= 0 && x <= world->width && y >= 0 && y <= world->height )
      {
        int index = y * world->width + x;

        if ( !change_room )
        {
          if ( tile_index == TILE_LVL1_WALL || tile_index == TILE_LVL1_FLOOR )
          {
            e_UpdateTile( index, tile_index, TILE_EMPTY, TILE_EMPTY );
          }

          else if( tile_index < TILE_LVL1_FLOOR )
          {
            e_UpdateTile( index, TILE_LVL1_FLOOR, tile_index, TILE_EMPTY );
          }

          else
          {
            e_UpdateTile( index, tile_index, TILE_EMPTY, TILE_EMPTY );
          }
        }

        else
        {
          world->room_ids[index] = room_id;
        }
      }
    }
  }

}

/* -----------------------------------------------------------------------
 * Consumable Spawn Sidebar
 * ----------------------------------------------------------------------- */

#define SP_ROW_H   18
#define SP_ROW_PAD  2
#define SP_ITEM_ROW_H 20

static const char* sp_type_labels[SPAWN_TYPE_COUNT] = {
  "[1]Rnd T1", "[2]Rnd T2", "[3]Consum", "[4]Equip",
  NULL,         "[5]ClsRare", NULL,       "[6]Pool"
};

static aColor_t sp_type_colors[SPAWN_TYPE_COUNT] = {
  { 0x38, 0xb7, 0x64, 255 },  /* T1: green */
  { 0xf7, 0xe2, 0x6b, 255 },  /* T2: yellow */
  { 0x41, 0xa6, 0xf6, 255 },  /* CONSUMABLE: cyan */
  { 0xb5, 0x5a, 0x9e, 255 },  /* EQUIPMENT: magenta */
  { 0x73, 0xef, 0xf7, 255 },  /* MAP: light cyan */
  { 0xe4, 0x3b, 0x44, 255 },  /* CLASS_RARE: red */
  { 0xe4, 0x3b, 0x44, 255 },  /* CLASS_EQUIPMENT: red */
  { 0xf7, 0x76, 0x22, 255 },  /* POOL: orange */
};

int we_DrawSpawnTypePalette( int ox, int oy, int selected_type )
{
  aTextStyle_t ts = {
    .type = FONT_CODE_PAGE_437,
    .fg = white,
    .bg = { 0, 0, 0, 0 },
    .align = TEXT_ALIGN_LEFT,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
  };

  int row = 0;
  for ( int i = 0; i < SPAWN_TYPE_COUNT; i++ )
  {
    if ( sp_type_labels[i] == NULL ) continue;

    int ry = oy + row * ( SP_ROW_H + SP_ROW_PAD );

    /* Colored swatch */
    aRectf_t swatch = { ox, ry + 3, 12, 12 };
    a_DrawFilledRect( swatch, sp_type_colors[i] );

    /* Label */
    a_DrawText( sp_type_labels[i], ox + 16, ry + 2, ts );

    /* Selection highlight */
    if ( i == selected_type )
    {
      aRectf_t sel = { ox - 1, ry, 145, SP_ROW_H };
      a_DrawRect( sel, magenta );
    }
    row++;
  }

  return oy + row * ( SP_ROW_H + SP_ROW_PAD );
}

int we_SpawnTypeMouseCheck( int ox, int oy, int* out_type )
{
  if ( app.mouse.button != 1 || app.mouse.state != 1 ) return 0;

  int row = 0;
  for ( int i = 0; i < SPAWN_TYPE_COUNT; i++ )
  {
    if ( sp_type_labels[i] == NULL ) continue;

    int ry = oy + row * ( SP_ROW_H + SP_ROW_PAD );
    if ( app.mouse.x >= ox && app.mouse.x <= ox + 145 &&
         app.mouse.y >= ry && app.mouse.y <= ry + SP_ROW_H )
    {
      *out_type = i;
      return 1;
    }
    row++;
  }
  return 0;
}

void we_DrawItemList( int ox, int oy, int max_h,
                      int* indices, int count, int scroll,
                      const char* selected_key )
{
  aTextStyle_t ts = {
    .type = FONT_CODE_PAGE_437,
    .fg = { 199, 207, 204, 255 },
    .bg = { 0, 0, 0, 0 },
    .align = TEXT_ALIGN_LEFT,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
  };

  int visible = max_h / SP_ITEM_ROW_H;
  if ( visible > count - scroll ) visible = count - scroll;
  if ( visible < 0 ) visible = 0;

  for ( int v = 0; v < visible; v++ )
  {
    int idx = indices[scroll + v];
    int ry = oy + v * SP_ITEM_ROW_H;

    /* Item image (scaled to 16x16) */
    if ( g_ed_items[idx].image != NULL )
    {
      aImage_t* img = g_ed_items[idx].image;
      float s = 16.0f / img->rect.w;
      a_BlitRect( img, NULL, &(aRectf_t){ ox, ry + 2, img->rect.w, img->rect.h }, s );
    }
    else if ( g_ed_items[idx].glyph[0] != '\0' )
    {
      a_DrawGlyph( g_ed_items[idx].glyph, ox, ry + 2, 16, 16,
                   g_ed_items[idx].color, (aColor_t){ 0, 0, 0, 0 },
                   FONT_CODE_PAGE_437 );
    }

    /* Name (truncated) */
    char name_buf[16];
    snprintf( name_buf, sizeof(name_buf), "%.14s", g_ed_items[idx].name );
    a_DrawText( name_buf, ox + 20, ry + 4, ts );

    /* Highlight selected */
    if ( selected_key && strcmp( g_ed_items[idx].key, selected_key ) == 0 )
    {
      aRectf_t sel = { ox - 1, ry, 145, SP_ITEM_ROW_H };
      a_DrawRect( sel, magenta );
    }
  }

  /* Scroll indicator */
  if ( count > max_h / SP_ITEM_ROW_H )
  {
    aTextStyle_t hint_ts = ts;
    hint_ts.fg = (aColor_t){ 120, 120, 120, 255 };
    char scroll_txt[32];
    snprintf( scroll_txt, sizeof(scroll_txt), "%d/%d", scroll + 1, count );
    a_DrawText( scroll_txt, ox + 80, oy + max_h - 12, hint_ts );
  }
}

int we_ItemListMouseCheck( int ox, int oy, int max_h,
                           int* indices, int count,
                           int* scroll, char* out_key )
{
  int visible = max_h / SP_ITEM_ROW_H;
  int max_scroll = count - visible;
  if ( max_scroll < 0 ) max_scroll = 0;

  /* Mouse wheel scrolling */
  if ( app.mouse.x >= ox && app.mouse.x <= ox + 145 &&
       app.mouse.y >= oy && app.mouse.y <= oy + max_h )
  {
    if ( app.mouse.wheel != 0 )
    {
      *scroll -= app.mouse.wheel;
      if ( *scroll < 0 ) *scroll = 0;
      if ( *scroll > max_scroll ) *scroll = max_scroll;
      app.mouse.wheel = 0;
    }
  }

  /* Click detection */
  if ( app.mouse.button != 1 || app.mouse.state != 1 ) return 0;

  for ( int v = 0; v < visible && ( *scroll + v ) < count; v++ )
  {
    int ry = oy + v * SP_ITEM_ROW_H;
    if ( app.mouse.x >= ox && app.mouse.x <= ox + 145 &&
         app.mouse.y >= ry && app.mouse.y <= ry + SP_ITEM_ROW_H )
    {
      int idx = indices[*scroll + v];
      strncpy( out_key, g_ed_items[idx].key, 63 );
      out_key[63] = '\0';
      return 1;
    }
  }
  return 0;
}

void we_DrawPoolList( int ox, int oy, SpawnList_t* spawns, int selected_pool )
{
  aTextStyle_t ts = {
    .type = FONT_CODE_PAGE_437,
    .fg = { 199, 207, 204, 255 },
    .bg = { 0, 0, 0, 0 },
    .align = TEXT_ALIGN_LEFT,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
  };

  if ( spawns == NULL || spawns->num_pools == 0 )
  {
    ts.fg = (aColor_t){ 120, 120, 120, 255 };
    a_DrawText( "No pools", ox, oy + 4, ts );
    return;
  }

  for ( int i = 0; i < spawns->num_pools; i++ )
  {
    int row_h = SP_ITEM_ROW_H + 10;
    int ry = oy + i * row_h;
    SpawnPool_t* pool = &spawns->pools[i];

    /* Orange swatch */
    a_DrawFilledRect( (aRectf_t){ ox, ry + 3, 12, 12 },
                      sp_type_colors[SPAWN_POOL] );

    /* Pool label */
    char label[32];
    snprintf( label, sizeof(label), "Pool %d", i );
    a_DrawText( label, ox + 16, ry + 2, ts );

    /* Pick info on second line */
    aTextStyle_t sub_ts = ts;
    sub_ts.fg = (aColor_t){ 150, 150, 150, 255 };
    char pick_buf[32];
    if ( strcmp( pool->pick_type, "class_equipment" ) == 0 )
      snprintf( pick_buf, sizeof(pick_buf), " class_equip" );
    else
      snprintf( pick_buf, sizeof(pick_buf), " %.14s", pool->pick_key );
    a_DrawText( pick_buf, ox + 16, ry + 12, sub_ts );

    if ( i == selected_pool )
    {
      aRectf_t sel = { ox - 1, ry, 145, row_h };
      a_DrawRect( sel, magenta );
    }
  }
}

int we_PoolListMouseCheck( int ox, int oy, SpawnList_t* spawns, int* out_pool )
{
  if ( spawns == NULL || spawns->num_pools == 0 ) return 0;
  if ( app.mouse.button != 1 || app.mouse.state != 1 ) return 0;

  for ( int i = 0; i < spawns->num_pools; i++ )
  {
    int ry = oy + i * ( SP_ITEM_ROW_H + 10 );
    if ( app.mouse.x >= ox && app.mouse.x <= ox + 145 &&
         app.mouse.y >= ry && app.mouse.y <= ry + SP_ITEM_ROW_H + 10 )
    {
      *out_pool = i;
      return 1;
    }
  }
  return 0;
}

void we_BuildFilteredItems( int spawn_type, int* out_indices, int* out_count )
{
  *out_count = 0;

  for ( int i = 0; i < g_ed_num_items; i++ )
  {
    int match = 0;
    const char* t = g_ed_items[i].type;

    switch ( spawn_type )
    {
      case SPAWN_CONSUMABLE:
        match = ( strcmp( t, "food" ) == 0 || strcmp( t, "gadget" ) == 0 ||
                  strcmp( t, "scroll" ) == 0 || strcmp( t, "potion" ) == 0 ||
                  strcmp( t, "pickup" ) == 0 );
        break;
      case SPAWN_EQUIPMENT:
        match = ( strcmp( t, "weapon" ) == 0 || strcmp( t, "armor" ) == 0 ||
                  strcmp( t, "trinket" ) == 0 );
        break;
      case SPAWN_MAP:
        match = ( strcmp( t, "map" ) == 0 );
        break;
      default:
        break;
    }

    if ( match && *out_count < ED_MAX_ITEMS )
      out_indices[(*out_count)++] = i;
  }
}

