#include <stdio.h>
#include <string.h>
#include <Archimedes.h>
#include <Daedalus.h>

#include "maps.h"

MapInfo_t g_maps[MAX_MAPS];
int       g_num_maps = 0;

static aColor_t ParseDUFColor( dDUFValue_t* color_node )
{
  aColor_t c = { 255, 255, 255, 255 };
  if ( color_node == NULL || color_node->type != D_DUF_ARRAY ) return c;

  dDUFValue_t* ch = color_node->child;
  if ( ch ) { c.r = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.g = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.b = (uint8_t)ch->value_int; ch = ch->next; }
  if ( ch ) { c.a = (uint8_t)ch->value_int; }
  return c;
}

void MapsLoadAll( void )
{
  g_num_maps = 0;
  memset( g_maps, 0, sizeof( g_maps ) );

  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( "resources/data/items_maps.duf", &root );

  if ( err != NULL )
  {
    printf( "DUF parse error at %d:%d - %s\n",
            err->line, err->column, d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return;
  }

  for ( dDUFValue_t* entry = root->child; entry != NULL && g_num_maps < MAX_MAPS; entry = entry->next )
  {
    MapInfo_t* m = &g_maps[g_num_maps];

    if ( entry->key )
      strncpy( m->key, entry->key, MAX_NAME_LENGTH - 1 );

    dDUFValue_t* name     = d_DUFGetObjectItem( entry, "name" );
    dDUFValue_t* glyph    = d_DUFGetObjectItem( entry, "glyph" );
    dDUFValue_t* color    = d_DUFGetObjectItem( entry, "color" );
    dDUFValue_t* effect   = d_DUFGetObjectItem( entry, "effect" );
    dDUFValue_t* desc     = d_DUFGetObjectItem( entry, "description" );
    dDUFValue_t* img_path = d_DUFGetObjectItem( entry, "image_path" );
    dDUFValue_t* tx       = d_DUFGetObjectItem( entry, "target_x" );
    dDUFValue_t* ty       = d_DUFGetObjectItem( entry, "target_y" );

    if ( name )   strncpy( m->name, name->value_string, MAX_NAME_LENGTH - 1 );
    if ( glyph )  strncpy( m->glyph, glyph->value_string, 7 );
    if ( effect ) strncpy( m->effect, effect->value_string, MAX_NAME_LENGTH - 1 );
    if ( desc )   strncpy( m->description, desc->value_string, 255 );
    if ( tx )     m->target_x = (int)tx->value_int;
    if ( ty )     m->target_y = (int)ty->value_int;
    m->color = ParseDUFColor( color );

    if ( img_path && strlen( img_path->value_string ) > 0 )
      m->image = a_ImageLoad( img_path->value_string );

    g_num_maps++;
  }

  d_DUFFree( root );
}

int MapByKey( const char* key )
{
  for ( int i = 0; i < g_num_maps; i++ )
    if ( strcmp( g_maps[i].key, key ) == 0 ) return i;
  return -1;
}
