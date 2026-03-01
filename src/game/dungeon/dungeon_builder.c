#include <Archimedes.h>

#include "dungeon.h"
#include "doors.h"
#include "objects.h"
#include "room_enumerator.h"

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

static const char* dungeon[DUNGEON_H] = {
  /*         1111111111222222222233333333 */
  /* 01234567890123456789012345678901234 56 */
  "#####################################",  /*  0 */
  "#####################################",  /*  1 */
  "##########################.#[[[[[####",  /*  2  corridor + merc upper      */
  "##########################.R[[[[[####",  /*  3  R door into merc upper     */
  "######################111#.#[[[[[####",  /*  4  jonathon + merc upper      */
  "######.########~~~~~##111#.###R######",  /*  5  jake's + jonathon + bridge */
  "######.########~~~~~##111#.###R######",  /*  6  jake's + jonathon + bridge */
  "#@@@@#........G~~~~~###....W]]]]]####",  /*  7  mat's + jake's + merc lwr  */
  "#@@@@#.##########G#####.####]]]]]####",  /*  8  mat's + G door + merc lwr  */
  "#@@@@#.####22222#.#####W####]]]]]####",  /*  9  mat's + north + merc lwr   */
  "##R###.####22222B.###33333###########",  /* 10  R door + north + gallery   */
  "#4444#.######.###...G33333###########",  /* 11  merc room + gallery       */
  "#4444W.#######.####.#33333B(((((#####",  /* 12  merc room + B to annex     */
  "#4444#########.###..####.##(((((#####",  /* 13  merc room + rogue annex    */
  "#W############B####....B.G.(((((#####",  /* 14  G door -> rogue annex      */
  "#.#########0000000######W##G###W#####",  /* 15  central room + annex corr  */
  "#.#5555####0000000####66666.###.#####",  /* 16  west + central + G to east */
  "#.#5555####0000000####66666##))))####",  /* 17  west + central + merc annex */
  "#.R5555...R0000000W...66666R)))))####",  /* 18  doors + passages + R annex  */
  "#.#5555####0000000####66666#)))))####",  /* 19  west + central + merc annex */
  "#.#5555####0000000####66666###W######",  /* 20  west + central + east     */
  "#.#########0000000####B#######..#####",  /* 21  central + B door + E corr   */
  "#.##########W#G#####$$$$$######.#####",  /* 22  south doors + chris + corr  */
  "#............#.#####$$$$$#####..#####",  /* 23  junction + chris + turn     */
  "#.#######.####.#####$$$$$B.....######",  /* 24  corridor + chris + E corr   */
  "#.###77777####.#####$$$$$####..######",  /* 25  SW room + chris + E corr    */
  "#.###77777#8888888###########.#######",  /* 26  SW room + south room      */
  "#.###77777#8888888#9999999###B#######",  /* 27  SW room + south room      */
  "#.#########8888888#9999999###.#######",  /* 28  south room                */
  "#.############G####9999999W.....#####",  /* 29  SE room                   */
  "#.############.####9999999#####.#####",  /* 30  SE room                   */
  "#.!!!!!#######.####9999999#####.#####",  /* 31  Room ! (no door) + passage     */
  "#.!!!!!............9999999......#####",  /* 32  Room ! + open passage + east   */
  "####.#######R#########B########G#####",  /* 33  White exit + R/B/G spread      */
  "####.#######.#########.########.#####",  /* 34  four corridors south           */
  "####....####.#########....#####.#####",  /* 35  White/Blue turn right          */
  "#######.##...############.###...#####",  /* 36  White/Red/Blue/Green south     */
  "#######.##.##############.###.#######",  /* 37  all corridors south            */
  "##......##.#########......###.#######",  /* 38  White/Blue turn left           */
  "##.#######......####.########......##",  /* 39  Red/Green turn right           */
  "##.############.####.#############.##",  /* 40  all corridors south            */
  "##....#########.####....##########.##",  /* 41  White/Blue turn right          */
  "#####.#######...#######.#######....##",  /* 42  Red/Green turn left            */
  "#####W#######.#########.#######.#####",  /* 43  W door + floor to chambers     */
  "###*****###%%%%%#####^^^^^####&&&&&##",  /* 44  chambers row 1                 */
  "###*****###%%%%%#####^^^^^####&&&&&##",  /* 45  chambers row 2                 */
  "###*****###%%%%%#####^^^^^####&&&&&##",  /* 46  chambers row 3                 */
  "#####################################",  /* 47  wall                            */
  "#####################################",  /* 48  wall                            */
  "#####################################",  /* 49  wall                            */
};

void DungeonBuild( World_t* world )
{
  RoomEnumeratorInit( DUNGEON_W, DUNGEON_H );

  /* Parse the character map into tiles */
  for ( int y = 0; y < DUNGEON_H; y++ )
  {
    for ( int x = 0; x < DUNGEON_W; x++ )
    {
      int  idx = y * DUNGEON_W + x;
      char c   = dungeon[y][x];

      if ( c == '#' )
      {
        world->background[idx].tile     = 1;
        world->background[idx].glyph    = "#";
        world->background[idx].glyph_fg = (aColor_t){ 0x81, 0x97, 0x96, 255 };
        world->background[idx].solid    = 1;
      }
      else if ( c == 'B' || c == 'G' || c == 'R' || c == 'W' )
      {
        int type = ( c == 'B' ) ? DOOR_BLUE  :
                   ( c == 'G' ) ? DOOR_GREEN :
                   ( c == 'R' ) ? DOOR_RED   : DOOR_WHITE;
        DoorPlace( world, x, y, type );
      }
      else
      {
        int room_id = char_to_room_id( c );
        if ( room_id != ROOM_NONE )
          RoomSetTile( x, y, room_id );
        /* both room chars and '.' keep the WorldCreate default (floor, tile 0) */
      }
    }
  }

  RoomLoadData( "resources/data/rooms_floor_01.duf" );

  /* Objects — placed by coordinate, not by map char */
  ObjectPlace( world, 22, 4, OBJ_EASEL );
}

void DungeonPlayerStart( float* wx, float* wy )
{
  *wx = 14 * 16 + 8.0f;
  *wy = 18 * 16 + 8.0f;
}
