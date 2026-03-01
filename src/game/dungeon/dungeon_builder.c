#include <Archimedes.h>

#include "dungeon.h"

static const char* dungeon[DUNGEON_H] = {
  /*         1111111111222222222 */
  /* 0123456789012345678901234567 8 */
  "#############################",  /*  0 */
  "#############################",  /*  1 */
  "#############################",  /*  2 */
  "#############################",  /*  3  jonathon room top wall     */
  "######################...####",  /*  4  jonathon room interior     */
  "######################...####",  /*  5  jonathon + easel           */
  "######################...####",  /*  6  jonathon room interior     */
  "#######################.#####",  /*  7  bottom wall, opening x=23  */
  "#######################.#####",  /*  8  corridor (x=23)            */
  "###########........####+#####",  /*  9  north room + white door    */
  "###########........##.....###",  /* 10  north room + gallery       */
  "##############.###..+.....###",  /* 11  corridor + blue door       */
  "##############.######.....###",  /* 12  corridor + gallery         */
  "##############.#########.####",  /* 13  corridor + gallery bottom  */
  "##############+#########.####",  /* 14  central north door + corr  */
  "###########.......######+####",  /* 15  central room + white door  */
  "##.....####.......####.....##",  /* 16  west + central + east      */
  "##.....####.......####.....##",  /* 17                             */
  "##........+.......+........##",  /* 18  west door + east door      */
  "##.....####.......####.....##",  /* 19                             */
  "##.....####.......####.....##",  /* 20                             */
  "###########.......###########",  /* 21  central room interior      */
  "##############+##############",  /* 22  central room, south door   */
  "##############.##############",  /* 23  corridor                   */
  "##############.##############",  /* 24                             */
  "##############.##############",  /* 25  opening                    */
  "###########.......###########",  /* 26  south room interior        */
  "###########.......###########",  /* 27                             */
  "###########.......###########",  /* 28                             */
  "#############################",  /* 29  south room bottom wall     */
  "#############################",  /* 30                             */
};

void DungeonBuild( World_t* world )
{
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
      else if ( c == '+' )
      {
        /* Floor underneath the door */
        world->background[idx].tile     = 0;
        world->background[idx].glyph    = ".";
        world->background[idx].glyph_fg = (aColor_t){ 0x39, 0x4a, 0x50, 255 };
        /* Door on midground (solid until opened) */
        world->midground[idx].tile     = 2;
        world->midground[idx].glyph    = "+";
        world->midground[idx].glyph_fg = (aColor_t){ 0xc0, 0x94, 0x73, 255 };
        world->midground[idx].solid    = 1;
      }
      /* '.' tiles keep the WorldCreate default (floor, tile 0) */
    }
  }

  /* Give each door its own color + tile */
  {
    struct { int x, y; uint32_t tile; aColor_t color; } doors[] = {
      { 14, 14, 2, { 0x4f, 0x8f, 0xba, 255 } },  /* north: blue            */
      { 14, 22, 3, { 0x75, 0xa7, 0x43, 255 } },  /* south: green           */
      { 10, 18, 4, { 0xa5, 0x30, 0x30, 255 } },  /* west:  red             */
      { 18, 18, 5, { 0xc7, 0xcf, 0xcc, 255 } },  /* east:  white           */
      { 20, 11, 2, { 0x4f, 0x8f, 0xba, 255 } },  /* gallery: blue          */
      { 24, 15, 5, { 0xc7, 0xcf, 0xcc, 255 } },  /* east-to-gallery: white */
      { 23,  9, 5, { 0xc7, 0xcf, 0xcc, 255 } },  /* gallery-to-studio: white */
    };
    int num_doors = sizeof( doors ) / sizeof( doors[0] );
    for ( int i = 0; i < num_doors; i++ )
    {
      int idx = doors[i].y * DUNGEON_W + doors[i].x;
      world->midground[idx].tile     = doors[i].tile;
      world->midground[idx].glyph_fg = doors[i].color;
    }
  }

  /* Easel: solid floor tile at (22, 4) — Jonathon's painting.
     Rendered as background glyph "E" (ASCII) or image overlay (GFX).
     No midground tile so tile_has_door won't trigger. */
  {
    int idx = 4 * DUNGEON_W + 22;
    world->background[idx].solid    = 1;
    world->background[idx].glyph    = "E";
    world->background[idx].glyph_fg = (aColor_t){ 0xc0, 0x94, 0x73, 255 };
  }
}

void DungeonPlayerStart( float* wx, float* wy )
{
  *wx = 14 * 16 + 8.0f;
  *wy = 18 * 16 + 8.0f;
}
