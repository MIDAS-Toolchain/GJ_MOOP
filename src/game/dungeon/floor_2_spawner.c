#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>

#include "dungeon.h"
#include "dungeon_spawner.h"
#include "interactive_tile.h"
#include "player.h"
#include "items.h"
#include "movement.h"
#include "shop.h"

void SpawnFloor2( NPC_t* npcs, int* num_npcs,
                  Enemy_t* enemies, int* num_enemies,
                  GroundItem_t* items, int* num_items,
                  World_t* world )
{
  int tw = world->tile_w, th = world->tile_h;
  (void)0;

  /* Room 0 (entry room, x=47..53, y=4..8):
     4 corner-not-corner positions, shuffled among:
     Laura (fighting NPC), Glorbnax (NPC), rat, goblin grunt */
  {
    int corners[4][2] = { {48,5}, {52,5}, {48,7}, {52,7} };

    /* Fisher-Yates shuffle */
    for ( int i = 3; i > 0; i-- )
    {
      int j = rand() % ( i + 1 );
      int tr = corners[i][0], tc = corners[i][1];
      corners[i][0] = corners[j][0]; corners[i][1] = corners[j][1];
      corners[j][0] = tr;            corners[j][1] = tc;
    }

    NPCSpawn( npcs, num_npcs, NPCTypeByKey( "laura" ),
              corners[0][0], corners[0][1], tw, th );
    NPCSpawn( npcs, num_npcs, NPCTypeByKey( "glorbnax_the_serene" ),
              corners[1][0], corners[1][1], tw, th );
    EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
                corners[2][0], corners[2][1], tw, th );
    EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "goblin_grunt" ),
                corners[3][0], corners[3][1], tw, th );
  }

  /* Cave spider in the %%%% room, just before Burble */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "spider" ),
              28, 6, tw, th );

  /* Burble - gate guard, beside the W door into the peace tribe town */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "burble_the_observant" ),
            20, 6, tw, th );

  /* Grishnak - peace goblin in town square, just south of the shop */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "grishnak_the_unyielding" ),
            13, 5, tw, th );

  /* Nettle - tea shop / reluctant shopkeeper in ) room */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "nettle_the_brewer" ),
            10, 3, tw, th );
  ShopSpawn( world );

  /* Thistlewick - meditating in the ( room */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "thistlewick_the_still" ),
            4, 6, tw, th );

  /* Clink - goblin banker at the end of the * room */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "clink_the_counter" ),
            17, 10, tw, th );

  /* Red slimes - scattered across the bottom half + the --- room */
  int rs = EnemyTypeByKey( "red_slime" );

  /* --- room (top, cols 20-24) */
  EnemySpawn( enemies, num_enemies, rs, 22, 1, tw, th );

  /* Bottom rooms */
  EnemySpawn( enemies, num_enemies, rs, 24, 25, tw, th );  /* room 6 */
  EnemySpawn( enemies, num_enemies, rs, 54, 25, tw, th );  /* room ~ */
  EnemySpawn( enemies, num_enemies, rs, 76, 19, tw, th );  /* room 9 */

  /* Bottom corridors */
  EnemySpawn( enemies, num_enemies, rs, 28, 15, tw, th );  /* left vertical */
  EnemySpawn( enemies, num_enemies, rs, 42, 21, tw, th );  /* center vertical */
  EnemySpawn( enemies, num_enemies, rs, 58, 17, tw, th );  /* right vertical */

  /* Spiders - in barren bottom rooms */
  int sp = EnemyTypeByKey( "spider" );
  EnemySpawn( enemies, num_enemies, sp, 24, 20, tw, th );  /* room 5 */
  EnemySpawn( enemies, num_enemies, sp, 36, 20, tw, th );  /* room 7 */
  EnemySpawn( enemies, num_enemies, sp, 62, 20, tw, th );  /* room 8 */
  EnemySpawn( enemies, num_enemies, sp, 44, 26, tw, th );  /* room @ */
  EnemySpawn( enemies, num_enemies, sp, 74, 26, tw, th );  /* room ! */

  /* Spider webs - near spider rooms */
  ITilePlace( world, 25, 20, ITILE_SPIDER_WEB );  /* room 5 */
  ITilePlace( world, 23, 20, ITILE_SPIDER_WEB );  /* room 5 */
  ITilePlace( world, 37, 20, ITILE_SPIDER_WEB );  /* room 7 */
  ITilePlace( world, 36, 19, ITILE_SPIDER_WEB );  /* room 7 */
  ITilePlace( world, 63, 20, ITILE_SPIDER_WEB );  /* room 8 */
  ITilePlace( world, 61, 20, ITILE_SPIDER_WEB );  /* room 8 */
  ITilePlace( world, 45, 26, ITILE_SPIDER_WEB );  /* room @ */
  ITilePlace( world, 44, 25, ITILE_SPIDER_WEB );  /* room @ */
  ITilePlace( world, 75, 26, ITILE_SPIDER_WEB );  /* room ! */
  ITilePlace( world, 74, 24, ITILE_SPIDER_WEB );  /* room ! */
  ITilePlace( world, 29, 6, ITILE_SPIDER_WEB );   /* before Burble */
  ITileWebScatterGold();

  /* === War Goblin Territory (top-right) === */
  {
    int gr = EnemyTypeByKey( "goblin_grunt" );
    int sh = EnemyTypeByKey( "goblin_shaman" );
    int sl = EnemyTypeByKey( "goblin_slinger" );
    int kr = EnemyTypeByKey( "kragthar" );

    /* \ room (Goblin Barracks) - grunt + slinger */
    EnemySpawn( enemies, num_enemies, gr, 63, 2, tw, th );
    EnemySpawn( enemies, num_enemies, sl, 64, 3, tw, th );

    /* ; room (Goblin Den) - shaman + grunt + slinger */
    EnemySpawn( enemies, num_enemies, sh, 80, 2, tw, th );
    EnemySpawn( enemies, num_enemies, gr, 79, 3, tw, th );
    EnemySpawn( enemies, num_enemies, sl, 81, 1, tw, th );

    /* : room (Goblin Warren) - slinger + grunt */
    EnemySpawn( enemies, num_enemies, sl, 91, 3, tw, th );
    EnemySpawn( enemies, num_enemies, gr, 90, 2, tw, th );

    /* Goblin door - blocks the path east of the Warren */
    NPCSpawn( npcs, num_npcs, NPCTypeByKey( "goblin_door" ),
              86, 2, tw, th );

    /* e room (Goblin Jail) - jailer + shaman */
    EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "goblin_jailer" ),
                51, 17, tw, th );
    EnemySpawn( enemies, num_enemies, sh, 50, 19, tw, th );

    /* Corridor leading to boss */
    EnemySpawn( enemies, num_enemies, gr, 82, 7, tw, th );

    /* q room (Goblin Boss) - Kragthar + shaman + grunt + slinger */
    EnemySpawn( enemies, num_enemies, kr, 90, 11, tw, th );
    EnemySpawn( enemies, num_enemies, sh, 91, 10, tw, th );
    EnemySpawn( enemies, num_enemies, gr, 89, 12, tw, th );
    EnemySpawn( enemies, num_enemies, sl, 92, 12, tw, th );

  }

  /* Middle-band rooms (south): 1 goblin grunt + 1 random consumable each */
  {
    int gr = EnemyTypeByKey( "goblin_grunt" );
    EnemySpawn( enemies, num_enemies, gr, 22, 10, tw, th );  /* room 2 */
    EnemySpawn( enemies, num_enemies, gr, 34, 10, tw, th );  /* room 1 */
    EnemySpawn( enemies, num_enemies, gr, 64, 10, tw, th );  /* room 3 */
    EnemySpawn( enemies, num_enemies, gr, 76, 10, tw, th );  /* room 4 */

  }

  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "goblin_grunt" ),
              50, 29, tw, th );
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "goblin_slinger" ),
              50, 30, tw, th );

  /* Barricade - south passage, 2 tiles below Laura's relocate spot */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "barricade" ),
            59, 40, tw, th );

  /* Lost Horror - spawned dynamically when barricade is cleared
     (see gs_Logic in game_scene.c) */

  /* Stairway to floor 03 - deep south past the barricade */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "stairway" ),
            53, 47, tw, th );

  /* Consumable/item spawns from DUF */
  DungeonSpawnFromDUF( "resources/data/floors/floor_02/floor_02_consumable_spawns.duf",
                       items, num_items, world );
}
