#ifndef __TILE_H__
#define __TILE_H__

enum
{
  LVL1_TILESET = 0,
  LVL2_TILESET,
  LVL3_TILESET,
  LVL4_TILESET,
};

enum
{
  TILE_LVL1_FLOOR    = 0,
  TILE_LVL1_WALL     = 1,
  TILE_BLUE_DOOR_EW  = 2,   /* walls E+W = game DOOR_BLUE */
  TILE_GREEN_DOOR_EW = 3,   /* walls E+W = game DOOR_GREEN */
  TILE_RED_DOOR_EW   = 4,   /* walls E+W = game DOOR_RED */
  TILE_WHITE_DOOR_EW = 5,   /* walls E+W = game DOOR_WHITE */
  TILE_BLUE_DOOR_NS  = 11,  /* walls N+S = game DOOR_BLUE_V */
  TILE_GREEN_DOOR_NS = 12,  /* walls N+S = game DOOR_GREEN_V */
  TILE_RED_DOOR_NS   = 13,  /* walls N+S = game DOOR_RED_V */
  TILE_WHITE_DOOR_NS = 14,  /* walls N+S = game DOOR_WHITE_V */
};

enum
{
  TILE_GLYPH_WALL       = '#',
  TILE_GLYPH_FLOOR      = '.',
  TILE_GLYPH_RED_DOOR   = 'R',
  TILE_GLYPH_GREEN_DOOR = 'G',
  TILE_GLYPH_BLUE_DOOR  = 'B',
  TILE_GLYPH_WHITE_DOOR = 'W'
};


#endif

