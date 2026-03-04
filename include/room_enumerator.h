#ifndef __ROOM_ENUMERATOR_H__
#define __ROOM_ENUMERATOR_H__

#define ROOM_NONE     -1
#define MAX_ROOMS      42

enum {
  ROOM_CENTRAL   = 0,   /* '0' */
  ROOM_JONATHON  = 1,   /* '1' */
  ROOM_NORTH     = 2,   /* '2' */
  ROOM_GALLERY   = 3,   /* '3' */
  ROOM_MERC      = 4,   /* '4' */
  ROOM_WEST      = 5,   /* '5' */
  ROOM_EAST      = 6,   /* '6' */
  ROOM_SW        = 7,   /* '7' */
  ROOM_SOUTH     = 8,   /* '8' */
  ROOM_SE        = 9,   /* '9' */
  ROOM_A_LEFT    = 10,  /* '!' */
  ROOM_MINER_MAT  = 11, /* '@' */
  ROOM_MINER_JAKE = 12, /* '~' */
  ROOM_MINER_CHRIS  = 13, /* '$' */
  ROOM_RED_CHAMBER  = 14, /* '%' */
  ROOM_BLUE_CHAMBER = 15, /* '^' */
  ROOM_GREEN_CHAMBER= 16, /* '&' */
  ROOM_WHITE_CHAMBER= 17, /* '*' */
  ROOM_ROGUE_ANNEX  = 18, /* '(' */
  ROOM_MERC_ANNEX   = 19, /* ')' */
  ROOM_MERC_UPPER   = 20, /* '[' */
  ROOM_MERC_LOWER   = 21, /* ']' */
  ROOM_22           = 22, /* '{' */
  ROOM_EXIT_CHAMBER = 23, /* '}' */
  ROOM_RAT_HOLE     = 24, /* '-' */
  ROOM_25           = 25, /* '_' */
  ROOM_26           = 26, /* '+' */
  ROOM_27           = 27, /* '=' */
  ROOM_28           = 28, /* '\\' */
  ROOM_29           = 29, /* '|' */
  ROOM_30           = 30, /* ';' */
  ROOM_31           = 31, /* ':' */
  ROOM_32           = 32, /* 'q' */
  ROOM_33           = 33, /* 'w' */
  ROOM_34           = 34, /* 'e' */
  ROOM_35           = 35, /* 'r' */
  ROOM_36           = 36, /* 't' */
  ROOM_37           = 37, /* 'y' */
  ROOM_38           = 38, /* 'u' */
  ROOM_39           = 39, /* 'i' */
  ROOM_40           = 40, /* 'o' */
  ROOM_41           = 41, /* 'p' */
};

void        RoomEnumeratorInit( int width, int height );
void        RoomSetTile( int x, int y, int room_id );
int         RoomAt( int x, int y );
void        RoomLoadData( const char* path );
const char* RoomName( int room_id );

#endif
