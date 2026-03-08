#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Daedalus.h>

#include "spawn_data.h"

static const char* type_strings[SPAWN_TYPE_COUNT] =
{
  "random_t1", "random_t2", "consumable", "equipment", "map",
  "class_rare", "class_equipment", "pool"
};

static SpawnType_t TypeFromString( const char* s )
{
  for ( int i = 0; i < SPAWN_TYPE_COUNT; i++ )
    if ( strcmp( s, type_strings[i] ) == 0 ) return (SpawnType_t)i;
  return SPAWN_RANDOM_T1;
}

static void SafeCopy( char* dst, const char* src, int n )
{
  if ( src ) strncpy( dst, src, n - 1 );
  dst[n - 1] = '\0';
}

/* ====== Load ====== */

int SpawnDUFLoad( const char* path, SpawnList_t* list )
{
  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( path, &root );

  if ( err != NULL )
  {
    printf( "SPAWN_DUF: parse error in %s at %d:%d - %s\n",
            path, err->line, err->column, d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return 0;
  }

  for ( dDUFValue_t* entry = root->child; entry != NULL; entry = entry->next )
  {
    if ( !entry->key ) continue;

    /* Pool definitions: @pool_N { ... } */
    if ( strncmp( entry->key, "pool_", 5 ) == 0 )
    {
      SpawnPool_t pool = {0};
      dDUFValue_t* v;

      if ( ( v = d_DUFGetObjectItem( entry, "pick_type" ) ) )
        SafeCopy( pool.pick_type, v->value_string, 32 );
      if ( ( v = d_DUFGetObjectItem( entry, "pick_key" ) ) )
        SafeCopy( pool.pick_key, v->value_string, 64 );
      if ( ( v = d_DUFGetObjectItem( entry, "mercenary" ) ) )
        SafeCopy( pool.pick_class_keys[0], v->value_string, 64 );
      if ( ( v = d_DUFGetObjectItem( entry, "rogue" ) ) )
        SafeCopy( pool.pick_class_keys[1], v->value_string, 64 );
      if ( ( v = d_DUFGetObjectItem( entry, "mage" ) ) )
        SafeCopy( pool.pick_class_keys[2], v->value_string, 64 );
      if ( ( v = d_DUFGetObjectItem( entry, "fill_type" ) ) )
        SafeCopy( pool.fill_type, v->value_string, 32 );

      SpawnListAddPool( list, pool );
      continue;
    }

    /* Spawn points: @spawn_N { ... } */
    if ( strncmp( entry->key, "spawn_", 6 ) != 0 ) continue;

    SpawnPoint_t pt = {0};
    dDUFValue_t* v;

    if ( ( v = d_DUFGetObjectItem( entry, "row" ) ) )
      pt.row = (int)v->value_int;
    if ( ( v = d_DUFGetObjectItem( entry, "col" ) ) )
      pt.col = (int)v->value_int;
    if ( ( v = d_DUFGetObjectItem( entry, "type" ) ) )
      pt.type = TypeFromString( v->value_string );

    if ( ( v = d_DUFGetObjectItem( entry, "key" ) ) )
      SafeCopy( pt.key, v->value_string, 64 );

    /* Class keys */
    if ( ( v = d_DUFGetObjectItem( entry, "mercenary" ) ) )
      SafeCopy( pt.class_keys[0], v->value_string, 64 );
    if ( ( v = d_DUFGetObjectItem( entry, "rogue" ) ) )
      SafeCopy( pt.class_keys[1], v->value_string, 64 );
    if ( ( v = d_DUFGetObjectItem( entry, "mage" ) ) )
      SafeCopy( pt.class_keys[2], v->value_string, 64 );

    /* Pool reference */
    if ( ( v = d_DUFGetObjectItem( entry, "pool" ) ) )
    {
      /* "pool_0" -> 0 */
      const char* pname = v->value_string;
      if ( pname && strncmp( pname, "pool_", 5 ) == 0 )
        pt.pool_id = atoi( pname + 5 );
    }

    SpawnListAdd( list, pt );
  }

  d_DUFFree( root );
  return 1;
}

/* ====== Save ====== */

int SpawnDUFSave( const char* path, SpawnList_t* list )
{
  FILE* fp = fopen( path, "w" );
  if ( !fp )
  {
    printf( "SPAWN_DUF: failed to open %s for writing\n", path );
    return 0;
  }

  /* Write pools first */
  for ( int i = 0; i < list->num_pools; i++ )
  {
    SpawnPool_t* p = &list->pools[i];
    fprintf( fp, "@pool_%d {\n", i );
    fprintf( fp, "    rule: \"pick_one\"\n" );
    fprintf( fp, "    pick_type: \"%s\"\n", p->pick_type );
    if ( strcmp( p->pick_type, "class_equipment" ) == 0 )
    {
      fprintf( fp, "    mercenary: \"%s\"\n", p->pick_class_keys[0] );
      fprintf( fp, "    rogue: \"%s\"\n", p->pick_class_keys[1] );
      fprintf( fp, "    mage: \"%s\"\n", p->pick_class_keys[2] );
    }
    else
    {
      fprintf( fp, "    pick_key: \"%s\"\n", p->pick_key );
    }
    fprintf( fp, "    fill_type: \"%s\"\n", p->fill_type );
    fprintf( fp, "}\n" );
  }

  /* Write spawn points */
  for ( int i = 0; i < list->count; i++ )
  {
    SpawnPoint_t* pt = &list->points[i];
    fprintf( fp, "@spawn_%d {\n", i );
    fprintf( fp, "    row: %d\n", pt->row );
    fprintf( fp, "    col: %d\n", pt->col );
    fprintf( fp, "    type: \"%s\"\n", type_strings[pt->type] );

    if ( pt->type == SPAWN_CONSUMABLE || pt->type == SPAWN_EQUIPMENT
         || pt->type == SPAWN_MAP )
    {
      fprintf( fp, "    key: \"%s\"\n", pt->key );
    }

    if ( pt->type == SPAWN_CLASS_RARE || pt->type == SPAWN_CLASS_EQUIPMENT )
    {
      fprintf( fp, "    mercenary: \"%s\"\n", pt->class_keys[0] );
      fprintf( fp, "    rogue: \"%s\"\n", pt->class_keys[1] );
      fprintf( fp, "    mage: \"%s\"\n", pt->class_keys[2] );
    }

    if ( pt->type == SPAWN_POOL )
    {
      fprintf( fp, "    pool: \"pool_%d\"\n", pt->pool_id );
    }

    fprintf( fp, "}\n" );
  }

  fclose( fp );
  return 1;
}

/* ====== Filename helper ====== */

void SpawnDUFFilename( const char* map_path, char* out, int out_size )
{
  /* "resources/data/floors/floor_01/floor_01.map" ->
     "resources/data/floors/floor_01/floor_01_consumable_spawns.duf" */
  strncpy( out, map_path, out_size - 1 );
  out[out_size - 1] = '\0';

  char* dot = strrchr( out, '.' );
  if ( dot )
  {
    int remain = out_size - (int)( dot - out );
    snprintf( dot, remain, "_consumable_spawns.duf" );
  }
}
