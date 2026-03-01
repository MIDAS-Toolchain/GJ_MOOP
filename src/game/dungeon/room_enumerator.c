#include <Daedalus.h>
#include <string.h>
#include <stdio.h>

#include "room_enumerator.h"
#include "dungeon.h"

static int  room_map[DUNGEON_W * DUNGEON_H];
static char room_names[MAX_ROOMS][64];
static int  map_width;

static int char_to_room_id( char c )
{
  if ( c >= '0' && c <= '9' ) return c - '0';
  switch ( c )
  {
    case '!': return 10;  case '@': return 11;  case '~': return 12;
    case '$': return 13;  case '%': return 14;  case '^': return 15;
    case '&': return 16;  case '*': return 17;  case '(': return 18;
    case ')': return 19;  case '[': return 20;  case ']': return 21;
    case '{': return 22;  case '}': return 23;  case '-': return 24;
    case '_': return 25;  case '+': return 26;  case '=': return 27;
    default:  return ROOM_NONE;
  }
}

void RoomEnumeratorInit( int width, int height )
{
  map_width = width;
  int total = width * height;
  for ( int i = 0; i < total; i++ )
    room_map[i] = ROOM_NONE;

  for ( int i = 0; i < MAX_ROOMS; i++ )
    room_names[i][0] = '\0';
}

void RoomSetTile( int x, int y, int room_id )
{
  room_map[y * map_width + x] = room_id;
}

int RoomAt( int x, int y )
{
  return room_map[y * map_width + x];
}

void RoomLoadData( const char* path )
{
  dDUFValue_t* root = NULL;
  dDUFError_t* err = d_DUFParseFile( path, &root );

  if ( err != NULL )
  {
    printf( "ROOMS: parse error in %s at %d:%d - %s\n",
            path, err->line, err->column, d_StringPeek( err->message ) );
    d_DUFErrorFree( err );
    return;
  }

  for ( dDUFValue_t* entry = root->child; entry != NULL; entry = entry->next )
  {
    if ( !entry->key ) continue;

    dDUFValue_t* rid_node = d_DUFGetObjectItem( entry, "room_id" );
    dDUFValue_t* name     = d_DUFGetObjectItem( entry, "name" );

    if ( !rid_node || !rid_node->value_string ) continue;

    int rid = char_to_room_id( rid_node->value_string[0] );
    if ( rid == ROOM_NONE || rid >= MAX_ROOMS ) continue;

    if ( name && name->value_string )
      strncpy( room_names[rid], name->value_string, 63 );
  }

  d_DUFFree( root );
}

const char* RoomName( int room_id )
{
  if ( room_id < 0 || room_id >= MAX_ROOMS ) return "Unknown";
  if ( room_names[room_id][0] == '\0' )      return "Unknown";
  return room_names[room_id];
}
