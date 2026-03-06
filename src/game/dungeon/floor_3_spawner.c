#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>

#include "dungeon.h"
#include "dungeon_spawner.h"
#include "player.h"
#include "items.h"
#include "movement.h"
#include "shop.h"

void SpawnFloor3( NPC_t* npcs, int* num_npcs,
                  Enemy_t* enemies, int* num_enemies,
                  GroundItem_t* items, int* num_items,
                  World_t* world )
{
  int tw = world->tile_w, th = world->tile_h;

  int cult = NPCTypeByKey( "cultist" );

  /* Greta - in her chamber west of room 0, through the W door */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "greta" ),
            33, 71, tw, th );

  /* Church door - blocks entry until Greta opens the way */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "church_door" ),
            42, 69, tw, th );

  /* Church door 2 - blocks exit until betrayal/defy */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "church_door2" ),
            42, 47, tw, th );

  /* Cultists - church hall and corridors */
  NPCSpawn( npcs, num_npcs, cult, 37, 64, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 42, 65, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 47, 64, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 40, 55, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 44, 55, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 44, 51, tw, th );
  NPCSpawn( npcs, num_npcs, cult, 40, 51, tw, th );

  /* Ex-cultist town (room t) */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "muri" ),
            54, 41, tw, th );
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "drem" ),
            54, 51, tw, th );
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "murl" ),
            55, 49, tw, th );

  /* Found Horror — Bloop (room )) */
  NPCSpawn( npcs, num_npcs, NPCTypeByKey( "found_horror" ),
            15, 55, tw, th );

  /* Lost Horror (room () */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "lost_horror" ),
              15, 52, tw, th );

  /* Horror (room 7) */
  EnemySpawn( enemies, num_enemies, EnemyTypeByKey( "horror" ),
              50, 70, tw, th );

  /* Cultist enemies (room w) */
  int cult_e = EnemyTypeByKey( "cultist" );
  EnemySpawn( enemies, num_enemies, cult_e, 20, 44, tw, th );
  EnemySpawn( enemies, num_enemies, cult_e, 22, 44, tw, th );

  /* Void slimes */
  int vs = EnemyTypeByKey( "void_slime" );
  EnemySpawn( enemies, num_enemies, vs, 34, 47, tw, th );
  EnemySpawn( enemies, num_enemies, vs, 34, 43, tw, th );
  EnemySpawn( enemies, num_enemies, vs, 39, 40, tw, th );
  EnemySpawn( enemies, num_enemies, vs, 50, 44, tw, th );

  /* T2 consumables beside slimes */
  SpawnRandomT2Consumable( items, num_items, 35, 47, tw, th );
  SpawnRandomT2Consumable( items, num_items, 35, 43, tw, th );
  SpawnRandomT2Consumable( items, num_items, 40, 40, tw, th );

  /* Medium health potion */
  {
    int mhp = ConsumableByKey( "medium_health_potion" );
    if ( mhp >= 0 )
      GroundItemSpawn( items, num_items, mhp, 41, 44, tw, th );
  }

  SpawnRandomT2Consumable( items, num_items, 26, 43, tw, th );

  /* T2 consumables */
  SpawnRandomT2Consumable( items, num_items, 55, 66, tw, th );
  SpawnRandomT2Consumable( items, num_items, 53, 72, tw, th );
  SpawnRandomT2Consumable( items, num_items, 67, 70, tw, th );
  SpawnRandomT2Consumable( items, num_items, 42, 60, tw, th );
}
