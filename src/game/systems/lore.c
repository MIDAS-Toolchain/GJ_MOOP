#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Daedalus.h>

#include "lore.h"
#include "persist.h"

static LoreEntry_t g_lore[MAX_LORE_ENTRIES];
static int         g_num_lore = 0;

/* ---- Load definitions from lore.duf ---- */

void LoreLoadDefinitions( void )
{
  g_num_lore = 0;
  memset( g_lore, 0, sizeof( g_lore ) );

  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( LORE_DATA_PATH, &root );
  if ( err )
  {
    printf( "LORE: parse error in %s - %s\n", LORE_DATA_PATH,
            d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return;
  }

  for ( dDUFValue_t* entry = root->child; entry; entry = entry->next )
  {
    if ( !entry->key || g_num_lore >= MAX_LORE_ENTRIES ) continue;

    LoreEntry_t* le = &g_lore[g_num_lore];
    strncpy( le->key, entry->key, 63 );

    dDUFValue_t* title = d_DUFGetObjectItem( entry, "title" );
    if ( title && title->value_string )
      strncpy( le->title, title->value_string, 127 );

    dDUFValue_t* cat = d_DUFGetObjectItem( entry, "category" );
    if ( cat && cat->value_string )
      strncpy( le->category, cat->value_string, 31 );

    dDUFValue_t* desc = d_DUFGetObjectItem( entry, "description" );
    if ( desc && desc->value_string )
      strncpy( le->description, desc->value_string, 511 );

    le->discovered = 0;
    g_num_lore++;
  }

  d_DUFFree( root );
  printf( "LORE: loaded %d definitions.\n", g_num_lore );
}

/* ---- Save / Load discovered state ---- */

void LoreLoadSave( void )
{
  char* data = PersistLoad( "lore" );
  if ( !data ) return;

  char* line = strtok( data, "\n" );
  while ( line )
  {
    for ( int i = 0; i < g_num_lore; i++ )
    {
      if ( strcmp( g_lore[i].key, line ) == 0 )
      {
        g_lore[i].discovered = 1;
        break;
      }
    }
    line = strtok( NULL, "\n" );
  }
  free( data );
}

void LoreSave( void )
{
  char buf[4096] = { 0 };
  for ( int i = 0; i < g_num_lore; i++ )
  {
    if ( g_lore[i].discovered )
    {
      strcat( buf, g_lore[i].key );
      strcat( buf, "\n" );
    }
  }
  PersistSave( "lore", buf );
}

/* ---- Runtime ---- */

int LoreUnlock( const char* key )
{
  for ( int i = 0; i < g_num_lore; i++ )
  {
    if ( strcmp( g_lore[i].key, key ) == 0 )
    {
      if ( g_lore[i].discovered ) return 0;
      g_lore[i].discovered = 1;
      LoreSave();
      return 1;
    }
  }
  printf( "LORE: key '%s' not found in definitions!\n", key );
  return 0;
}

int LoreIsDiscovered( const char* key )
{
  for ( int i = 0; i < g_num_lore; i++ )
  {
    if ( strcmp( g_lore[i].key, key ) == 0 )
      return g_lore[i].discovered;
  }
  return 0;
}

int LoreGetCount( void ) { return g_num_lore; }

LoreEntry_t* LoreGetEntry( int index )
{
  if ( index < 0 || index >= g_num_lore ) return NULL;
  return &g_lore[index];
}

int LoreCountDiscovered( void )
{
  int n = 0;
  for ( int i = 0; i < g_num_lore; i++ )
    if ( g_lore[i].discovered ) n++;
  return n;
}

int LoreCountInCategory( const char* category )
{
  int n = 0;
  for ( int i = 0; i < g_num_lore; i++ )
    if ( g_lore[i].discovered && strcmp( g_lore[i].category, category ) == 0 )
      n++;
  return n;
}

int LoreGetCategories( char out[][32], int max )
{
  int count = 0;
  for ( int i = 0; i < g_num_lore && count < max; i++ )
  {
    int dup = 0;
    for ( int j = 0; j < count; j++ )
      if ( strcmp( out[j], g_lore[i].category ) == 0 ) { dup = 1; break; }
    if ( !dup )
      strncpy( out[count++], g_lore[i].category, 31 );
  }
  return count;
}
