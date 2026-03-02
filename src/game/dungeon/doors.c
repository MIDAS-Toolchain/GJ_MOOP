#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "doors.h"
#include "player.h"

extern Player_t player;

static Console_t* console;

static aSoundEffect_t sfx_door_white;
static aSoundEffect_t sfx_door_red;
static aSoundEffect_t sfx_door_green;
static aSoundEffect_t sfx_door_blue;
static aSoundEffect_t sfx_door_fail;

/* Map any door tile (horizontal or vertical) to its base type */
static int door_base( uint32_t tile )
{
  if ( tile >= DOOR_BLUE_V && tile <= DOOR_WHITE_V )
    return (int)tile - ( DOOR_BLUE_V - DOOR_BLUE );
  return (int)tile;
}

static aColor_t door_color( int type )
{
  switch ( door_base( type ) )
  {
    case DOOR_BLUE:  return (aColor_t){ 0x4f, 0x8f, 0xba, 255 };
    case DOOR_GREEN: return (aColor_t){ 0x75, 0xa7, 0x43, 255 };
    case DOOR_RED:   return (aColor_t){ 0xa5, 0x30, 0x30, 255 };
    case DOOR_WHITE: return (aColor_t){ 0xc7, 0xcf, 0xcc, 255 };
    default:         return (aColor_t){ 0xc0, 0x94, 0x73, 255 };
  }
}

void DoorsInit( Console_t* con )
{
  console = con;
  a_AudioLoadSound( "resources/soundeffects/door_white_open.wav", &sfx_door_white );
  a_AudioLoadSound( "resources/soundeffects/door_red_open.ogg",   &sfx_door_red );
  a_AudioLoadSound( "resources/soundeffects/door_green_open.ogg", &sfx_door_green );
  a_AudioLoadSound( "resources/soundeffects/door_blue_open.ogg",  &sfx_door_blue );
  a_AudioLoadSound( "resources/soundeffects/door_fail.ogg",       &sfx_door_fail );
}

void DoorPlace( World_t* w, int x, int y, int type, int vertical )
{
  int idx = y * w->width + x;
  int tile = vertical ? type + ( DOOR_BLUE_V - DOOR_BLUE ) : type;

  /* Floor underneath */
  w->background[idx].tile     = 0;
  w->background[idx].glyph    = ".";
  w->background[idx].glyph_fg = (aColor_t){ 0x39, 0x4a, 0x50, 255 };

  /* Door on midground */
  w->midground[idx].tile     = (uint32_t)tile;
  w->midground[idx].glyph    = "+";
  w->midground[idx].glyph_fg = door_color( type );
  w->midground[idx].solid    = 1;
}

int DoorIsDoor( uint32_t tile )
{
  return ( tile >= DOOR_BLUE && tile <= DOOR_WHITE ) ||
         ( tile >= DOOR_BLUE_V && tile <= DOOR_WHITE_V );
}

int DoorCanOpen( uint32_t tile )
{
  const char* cls = player.name;
  switch ( door_base( tile ) )
  {
    case DOOR_WHITE: return 1;
    case DOOR_RED:   return ( strcmp( cls, "Mercenary" ) == 0 );
    case DOOR_GREEN: return ( strcmp( cls, "Rogue" ) == 0 );
    case DOOR_BLUE:  return ( strcmp( cls, "Mage" ) == 0 );
    default:         return 0;
  }
}

int DoorTryOpen( World_t* w, int x, int y )
{
  int idx = y * w->width + x;
  Tile_t* door = &w->midground[idx];

  if ( DoorCanOpen( door->tile ) )
  {
    switch ( door_base( door->tile ) )
    {
      case DOOR_WHITE: a_AudioPlaySound( &sfx_door_white, NULL ); break;
      case DOOR_RED:   a_AudioPlaySound( &sfx_door_red,   NULL ); break;
      case DOOR_GREEN: a_AudioPlaySound( &sfx_door_green, NULL ); break;
      case DOOR_BLUE:  a_AudioPlaySound( &sfx_door_blue,  NULL ); break;
    }
    switch ( door_base( door->tile ) )
    {
      case DOOR_RED:   ConsolePushF( console, door->glyph_fg, "You break down the door." ); break;
      case DOOR_GREEN: ConsolePushF( console, door->glyph_fg, "You pick the lock." );       break;
      case DOOR_BLUE:  ConsolePushF( console, door->glyph_fg, "You dispel the barrier." );  break;
      default:         ConsolePushF( console, door->glyph_fg, "You open the door." );       break;
    }
    door->tile  = TILE_EMPTY;
    door->solid = 0;
    door->glyph = "";
    return 1;
  }

  a_AudioPlaySound( &sfx_door_fail, NULL );
  ConsolePush( console, "The door is locked.", door->glyph_fg );
  return 0;
}

void DoorDescribe( World_t* w, int x, int y )
{
  int idx = y * w->width + x;
  Tile_t* door = &w->midground[idx];

  switch ( door_base( door->tile ) )
  {
    case DOOR_WHITE:
      ConsolePush( console, "An unlocked door.", door->glyph_fg );
      ConsolePush( console, "  Anybody can open this.", door->glyph_fg );
      break;
    case DOOR_RED:
      ConsolePush( console, "A locked door.", door->glyph_fg );
      if ( DoorCanOpen( door->tile ) )
        ConsolePush( console, "  You can bust it down.", door->glyph_fg );
      else
        ConsolePush( console, "  Someone stronger could bust it down.", door->glyph_fg );
      break;
    case DOOR_GREEN:
      ConsolePush( console, "A locked door.", door->glyph_fg );
      if ( DoorCanOpen( door->tile ) )
        ConsolePush( console, "  You can pick this lock.", door->glyph_fg );
      else
        ConsolePush( console, "  Someone more agile could pick this lock.", door->glyph_fg );
      break;
    case DOOR_BLUE:
      ConsolePush( console, "A locked door.", door->glyph_fg );
      if ( DoorCanOpen( door->tile ) )
        ConsolePush( console, "  You can dispel this barrier.", door->glyph_fg );
      else
        ConsolePush( console, "  Someone smarter could dispel this barrier.", door->glyph_fg );
      break;
    default:
      ConsolePush( console, "A door.", door->glyph_fg );
      break;
  }
}
