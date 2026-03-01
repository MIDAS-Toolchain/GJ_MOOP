#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>

#include "dungeon.h"
#include "player.h"
#include "items.h"
#include "movement.h"

extern Player_t player;

void DungeonSpawn( NPC_t* npcs, int* num_npcs,
                   Enemy_t* enemies, int* num_enemies,
                   GroundItem_t* items, int* num_items,
                   World_t* world )
{
  /* Randomized central room layout:
     4 near-corner positions -> shuffle -> 3 get consumables, 1 gets NPC */
  {
    int corners[4][2] = { {12,16}, {16,16}, {12,20}, {16,20} };

    /* Fisher-Yates shuffle */
    for ( int i = 3; i > 0; i-- )
    {
      int j = rand() % ( i + 1 );
      int tr = corners[i][0], tc = corners[i][1];
      corners[i][0] = corners[j][0]; corners[i][1] = corners[j][1];
      corners[j][0] = tr;            corners[j][1] = tc;
    }

    /* Determine class index for filtered consumables */
    int class_idx = 0;
    for ( int i = 0; i < 3; i++ )
    {
      if ( strcmp( player.name, g_classes[i].name ) == 0 )
      { class_idx = i; break; }
    }

    /* Get class-appropriate consumables */
    FilteredItem_t filtered[16];
    int num_filtered = ItemsBuildFiltered( class_idx, filtered, 16 );

    /* Spawn up to 3 consumables at first 3 shuffled corners */
    int spawned = 0;
    for ( int i = 0; i < num_filtered && spawned < 3; i++ )
    {
      if ( filtered[i].type == FILTERED_CONSUMABLE )
      {
        GroundItemSpawn( items, num_items,
                         filtered[i].index,
                         corners[spawned][0], corners[spawned][1],
                         world->tile_w, world->tile_h );
        spawned++;
      }
    }

    /* 4th corner gets Graf */
    NPCSpawn( npcs, num_npcs, NPCTypeByKey( "graf" ),
              corners[3][0], corners[3][1],
              world->tile_w, world->tile_h );
  }

  /* Spawn Jonathon in his studio */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "jonathon" ),
            23, 4, world->tile_w, world->tile_h );

  /* Spawn enemies at random walkable floor tiles in central room,
     avoiding player (14,18), NPC, and ground item positions */
  {
    /* Collect available floor tiles in central room (x=11..17, y=15..21) */
    int avail[64][2];
    int num_avail = 0;
    for ( int x = 11; x <= 17 && num_avail < 64; x++ )
    {
      for ( int y = 15; y <= 21 && num_avail < 64; y++ )
      {
        if ( x == 14 && y == 18 ) continue; /* player start */
        if ( !TileWalkable( x, y ) ) continue;
        if ( NPCAt( npcs, *num_npcs, x, y ) ) continue;
        if ( GroundItemAt( items, *num_items, x, y ) ) continue;
        avail[num_avail][0] = x;
        avail[num_avail][1] = y;
        num_avail++;
      }
    }

    /* Pick 2 random positions for rat + skeleton */
    if ( num_avail >= 2 )
    {
      int i1 = rand() % num_avail;
      EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
                  avail[i1][0], avail[i1][1],
                  world->tile_w, world->tile_h );

      /* Remove used position */
      avail[i1][0] = avail[num_avail - 1][0];
      avail[i1][1] = avail[num_avail - 1][1];
      num_avail--;

      int i2 = rand() % num_avail;
      EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "skeleton" ),
                  avail[i2][0], avail[i2][1],
                  world->tile_w, world->tile_h );
    }
  }

  /* Skeleton in the gallery (below Jonathon's studio) */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "skeleton" ),
              23, 11, world->tile_w, world->tile_h );

  /* Rats in side rooms */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
              14, 9, world->tile_w, world->tile_h );   /* north */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
              14, 27, world->tile_w, world->tile_h );  /* south */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
              4, 18, world->tile_w, world->tile_h );   /* west */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "rat" ),
              24, 18, world->tile_w, world->tile_h );  /* east */
}
