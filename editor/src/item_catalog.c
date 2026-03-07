/*
 * item_catalog.c: Lightweight item data loader for the editor.
 * Parses consumable and equipment DUFs to extract key/name/type/glyph/color/image.
 * No game dependencies.
 *
 * Copyright (c) 2026 Jacob Kellum <jkellum819@gmail.com>
 *                    Mathew Storm <smattymat@gmail.com>
 ************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Archimedes.h>
#include <Daedalus.h>

#include "item_catalog.h"

EdItemInfo_t g_ed_items[ED_MAX_ITEMS];
int          g_ed_num_items = 0;

int g_ed_t1_indices[ED_MAX_TIER];
int g_ed_t1_count = 0;
int g_ed_t2_indices[ED_MAX_TIER];
int g_ed_t2_count = 0;
int g_ed_rare_indices[ED_MAX_TIER];
int g_ed_rare_count = 0;

static aColor_t ParseColor( dDUFValue_t* node )
{
  aColor_t c = { 255, 255, 255, 255 };
  if ( node == NULL || node->type != D_DUF_ARRAY ) return c;

  dDUFValue_t* ch = node->child;
  if ( ch ) { c.r = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.g = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.b = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.a = (uint8_t)ch->value_int; }
  return c;
}

static void LoadDUF( const char* path )
{
  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( path, &root );

  if ( err != NULL )
  {
    printf( "EdItemCatalog: parse error in %s at %d:%d - %s\n",
            path, err->line, err->column, d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return;
  }

  for ( dDUFValue_t* entry = root->child;
        entry != NULL && g_ed_num_items < ED_MAX_ITEMS;
        entry = entry->next )
  {
    EdItemInfo_t* item = &g_ed_items[g_ed_num_items];
    memset( item, 0, sizeof( EdItemInfo_t ) );

    if ( entry->key )
      strncpy( item->key, entry->key, 63 );

    dDUFValue_t* name     = d_DUFGetObjectItem( entry, "name" );
    dDUFValue_t* type     = d_DUFGetObjectItem( entry, "type" );
    dDUFValue_t* kind     = d_DUFGetObjectItem( entry, "kind" );
    dDUFValue_t* glyph    = d_DUFGetObjectItem( entry, "glyph" );
    dDUFValue_t* color    = d_DUFGetObjectItem( entry, "color" );
    dDUFValue_t* img_path = d_DUFGetObjectItem( entry, "image_path" );

    if ( name )  strncpy( item->name,  name->value_string, 63 );
    if ( type )  strncpy( item->type,  type->value_string, 63 );
    if ( kind )  strncpy( item->type,  kind->value_string, 63 );
    if ( glyph ) strncpy( item->glyph, glyph->value_string, 7 );
    item->color = ParseColor( color );

    if ( img_path && strlen( img_path->value_string ) > 0 )
    {
      char full_path[256];
      snprintf( full_path, sizeof(full_path), "../%s", img_path->value_string );
      if ( access( full_path, F_OK ) == 0 )
        item->image = a_ImageLoad( full_path );
      else
        printf( "EdItemCatalog: missing image '%s' for '%s'\n",
                full_path, item->key );
    }

    g_ed_num_items++;
  }

  d_DUFFree( root );
}

static void BuildTierArrays( void )
{
  /* Class consumable types: food (merc), gadget (rogue), scroll (mage).
     First 3 of each type = T1, next 3 = T2. */
  static const char* class_types[] = { "food", "gadget", "scroll" };

  g_ed_t1_count = 0;
  g_ed_t2_count = 0;
  g_ed_rare_count = 0;

  for ( int t = 0; t < 3; t++ )
  {
    int seen = 0;
    for ( int i = 0; i < g_ed_num_items; i++ )
    {
      if ( strcmp( g_ed_items[i].type, class_types[t] ) != 0 ) continue;

      if ( seen < 3 )
      {
        if ( g_ed_t1_count < ED_MAX_TIER )
          g_ed_t1_indices[g_ed_t1_count++] = i;
      }
      else if ( seen < 6 )
      {
        if ( g_ed_t2_count < ED_MAX_TIER )
          g_ed_t2_indices[g_ed_t2_count++] = i;
      }
      else if ( seen == 6 )
      {
        if ( g_ed_rare_count < ED_MAX_TIER )
          g_ed_rare_indices[g_ed_rare_count++] = i;
      }
      seen++;
    }
  }
}

void EdItemCatalogLoad( void )
{
  g_ed_num_items = 0;
  memset( g_ed_items, 0, sizeof( g_ed_items ) );

  LoadDUF( "../resources/data/consumables.duf" );
  LoadDUF( "../resources/data/equipment_starters.duf" );
  LoadDUF( "../resources/data/equipment_drops.duf" );
  LoadDUF( "../resources/data/items_maps.duf" );

  BuildTierArrays();

  printf( "EdItemCatalog: loaded %d items (%d T1, %d T2, %d Rare)\n",
          g_ed_num_items, g_ed_t1_count, g_ed_t2_count, g_ed_rare_count );
}

int EdItemByKey( const char* key )
{
  if ( key == NULL || key[0] == '\0' ) return -1;
  for ( int i = 0; i < g_ed_num_items; i++ )
  {
    if ( strcmp( g_ed_items[i].key, key ) == 0 )
      return i;
  }
  return -1;
}
