#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>
#include <Daedalus.h>

#include "defines.h"
#include "shop.h"
#include "items.h"
#include "player.h"
#include "visibility.h"

ShopItem_t  g_shop_items[MAX_SHOP_ITEMS];
int         g_num_shop_items = 0;

static ShopPoolEntry_t pool[MAX_SHOP_POOL];
static int             pool_count = 0;
static int             spawn_min  = 4;
static int             spawn_max  = 6;

/* Rug tile positions — 3x2 grid in room 9 */
static const int rug_tiles[6][2] = {
  {21, 29}, {22, 29}, {23, 29},
  {21, 30}, {22, 30}, {23, 30}
};
#define NUM_RUG_TILES 6

#define RUG_COLOR (aColor_t){ 140, 60, 60, 255 }

void ShopLoadPool( const char* path )
{
  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( path, &root );

  if ( err != NULL )
  {
    printf( "DUF parse error at %d:%d - %s\n",
            err->line, err->column, d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return;
  }

  pool_count = 0;

  for ( dDUFValue_t* entry = root->child; entry != NULL; entry = entry->next )
  {
    if ( entry->key == NULL ) continue;

    /* Metadata block */
    if ( strcmp( entry->key, "shop" ) == 0 )
    {
      dDUFValue_t* v;
      if ( ( v = d_DUFGetObjectItem( entry, "spawn_min" ) ) )
        spawn_min = (int)v->value_int;
      if ( ( v = d_DUFGetObjectItem( entry, "spawn_max" ) ) )
        spawn_max = (int)v->value_int;
      continue;
    }

    if ( pool_count >= MAX_SHOP_POOL ) break;

    ShopPoolEntry_t* p = &pool[pool_count];
    memset( p, 0, sizeof( ShopPoolEntry_t ) );

    dDUFValue_t* ikey  = d_DUFGetObjectItem( entry, "item_key" );
    dDUFValue_t* type  = d_DUFGetObjectItem( entry, "type" );
    dDUFValue_t* cost  = d_DUFGetObjectItem( entry, "cost" );
    dDUFValue_t* weight = d_DUFGetObjectItem( entry, "weight" );
    dDUFValue_t* cls   = d_DUFGetObjectItem( entry, "class" );

    if ( ikey )   strncpy( p->item_key, ikey->value_string, MAX_NAME_LENGTH - 1 );
    if ( type )   strncpy( p->type, type->value_string, MAX_NAME_LENGTH - 1 );
    if ( cls )    strncpy( p->class_name, cls->value_string, MAX_NAME_LENGTH - 1 );
    if ( cost )   p->cost = (int)cost->value_int;
    if ( weight ) p->weight = (int)weight->value_int;
    if ( p->weight < 1 ) p->weight = 1;

    pool_count++;
  }

  d_DUFFree( root );
}

void ShopSpawn( World_t* world )
{
  g_num_shop_items = 0;
  if ( pool_count == 0 ) return;

  int count = spawn_min + rand() % ( spawn_max - spawn_min + 1 );
  if ( count > MAX_SHOP_ITEMS ) count = MAX_SHOP_ITEMS;
  if ( count > NUM_RUG_TILES )  count = NUM_RUG_TILES;

  /* Shuffle rug tile positions (Fisher-Yates) */
  int tiles[NUM_RUG_TILES][2];
  for ( int i = 0; i < NUM_RUG_TILES; i++ )
  {
    tiles[i][0] = rug_tiles[i][0];
    tiles[i][1] = rug_tiles[i][1];
  }
  for ( int i = NUM_RUG_TILES - 1; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tr = tiles[i][0], tc = tiles[i][1];
    tiles[i][0] = tiles[j][0]; tiles[i][1] = tiles[j][1];
    tiles[j][0] = tr;          tiles[j][1] = tc;
  }

  /* Build class-filtered pool indices + total weight */
  int filtered[MAX_SHOP_POOL];
  int num_filtered = 0;
  int total_weight = 0;
  for ( int i = 0; i < pool_count; i++ )
  {
    if ( pool[i].class_name[0] == '\0'
         || strcmp( pool[i].class_name, player.name ) == 0 )
    {
      filtered[num_filtered++] = i;
      total_weight += pool[i].weight;
    }
  }
  if ( total_weight == 0 ) return;

  /* Weighted random selection from filtered pool — duplicates allowed */
  for ( int s = 0; s < count; s++ )
  {
    int roll = rand() % total_weight;
    int accum = 0;
    int pick = filtered[0];
    for ( int f = 0; f < num_filtered; f++ )
    {
      accum += pool[filtered[f]].weight;
      if ( accum > roll ) { pick = filtered[f]; break; }
    }

    ShopPoolEntry_t* pe = &pool[pick];
    ShopItem_t* si = &g_shop_items[g_num_shop_items];
    memset( si, 0, sizeof( ShopItem_t ) );

    if ( strcmp( pe->type, "consumable" ) == 0 )
    {
      si->item_type  = INV_CONSUMABLE;
      si->item_index = ConsumableByKey( pe->item_key );
      if ( si->item_index < 0 ) continue;
    }
    else if ( strcmp( pe->type, "equipment" ) == 0 )
    {
      si->item_type  = INV_EQUIPMENT;
      si->item_index = EquipmentByKey( pe->item_key );
      if ( si->item_index < 0 ) continue;
    }
    else continue;

    si->cost    = pe->cost;
    si->row     = tiles[s][0];
    si->col     = tiles[s][1];
    si->world_x = si->row * world->tile_w + world->tile_w / 2.0f;
    si->world_y = si->col * world->tile_h + world->tile_h / 2.0f;
    si->alive   = 1;
    g_num_shop_items++;
  }
}

ShopItem_t* ShopItemAt( int row, int col )
{
  for ( int i = 0; i < g_num_shop_items; i++ )
  {
    if ( g_shop_items[i].alive
         && g_shop_items[i].row == row
         && g_shop_items[i].col == col )
      return &g_shop_items[i];
  }
  return NULL;
}

int ShopIsRugTile( int row, int col )
{
  for ( int i = 0; i < NUM_RUG_TILES; i++ )
  {
    if ( rug_tiles[i][0] == row && rug_tiles[i][1] == col )
      return 1;
  }
  return 0;
}

void ShopDrawRug( aRectf_t vp_rect, GameCamera_t* cam,
                  World_t* world, int gfx_mode )
{
  (void)gfx_mode;

  for ( int i = 0; i < NUM_RUG_TILES; i++ )
  {
    int r = rug_tiles[i][0];
    int c = rug_tiles[i][1];
    if ( VisibilityGet( r, c ) < 0.01f ) continue;

    float wx = r * world->tile_w + world->tile_w / 2.0f;
    float wy = c * world->tile_h + world->tile_h / 2.0f;
    GV_DrawFilledRect( vp_rect, cam, wx, wy,
                       (float)world->tile_w, (float)world->tile_h,
                       RUG_COLOR );
  }
}

void ShopDrawItems( aRectf_t vp_rect, GameCamera_t* cam,
                    World_t* world, int gfx_mode )
{
  for ( int i = 0; i < g_num_shop_items; i++ )
  {
    ShopItem_t* si = &g_shop_items[i];
    if ( !si->alive ) continue;
    if ( VisibilityGet( si->row, si->col ) < 0.01f ) continue;

    /* Item glyph/image */
    const char* glyph = NULL;
    aColor_t color = { 255, 255, 255, 255 };
    aImage_t* img = NULL;

    if ( si->item_type == INV_CONSUMABLE )
    {
      ConsumableInfo_t* ci = &g_consumables[si->item_index];
      glyph = ci->glyph;
      color = ci->color;
      img   = ci->image;
    }
    else
    {
      EquipmentInfo_t* ei = &g_equipment[si->item_index];
      glyph = ei->glyph;
      color = ei->color;
      img   = NULL; /* equipment has no image field */
    }

    if ( img && gfx_mode == GFX_IMAGE )
    {
      GV_DrawSprite( vp_rect, cam, img,
                     si->world_x, si->world_y,
                     (float)world->tile_w, (float)world->tile_h );
    }
    else if ( glyph && glyph[0] )
    {
      float sx, sy;
      GV_WorldToScreen( vp_rect, cam,
                        si->world_x - world->tile_w / 2.0f,
                        si->world_y - world->tile_h / 2.0f,
                        &sx, &sy );
      float half_w = cam->half_h * ( vp_rect.w / vp_rect.h );
      int dw = (int)( world->tile_w * ( vp_rect.w / ( half_w * 2.0f ) ) );
      int dh = (int)( world->tile_h * ( vp_rect.h / ( cam->half_h * 2.0f ) ) );

      a_DrawGlyph( glyph, (int)sx, (int)sy, dw, dh,
                   color, (aColor_t){ 0, 0, 0, 0 }, FONT_CODE_PAGE_437 );
    }

    /* Price tag — top center of tile, black bg */
    {
      float sx, sy;
      GV_WorldToScreen( vp_rect, cam,
                        si->world_x - world->tile_w / 2.0f,
                        si->world_y - world->tile_h / 2.0f,
                        &sx, &sy );
      float half_w = cam->half_h * ( vp_rect.w / vp_rect.h );
      int tw = (int)( world->tile_w * ( vp_rect.w / ( half_w * 2.0f ) ) );

      char buf[16];
      snprintf( buf, sizeof( buf ), "%dg", si->cost );
      aTextStyle_t ts = a_default_text_style;
      ts.fg    = (aColor_t){ 0xde, 0x9e, 0x41, 255 };
      ts.bg    = (aColor_t){ 0, 0, 0, 200 };
      ts.scale = 0.7f;
      ts.align = TEXT_ALIGN_CENTER;
      a_DrawText( buf, (int)sx + tw / 2, (int)sy, ts );
    }
  }
}
