#ifndef DUNGEON_SPAWNER_H
#define DUNGEON_SPAWNER_H

#include "dungeon.h"
#include "enemies.h"
#include "ground_items.h"

/* Per-floor spawners (called from DungeonSpawn) */
void SpawnFloor1( NPC_t* npcs, int* num_npcs,
                  Enemy_t* enemies, int* num_enemies,
                  GroundItem_t* items, int* num_items,
                  World_t* world );

void SpawnFloor2( NPC_t* npcs, int* num_npcs,
                  Enemy_t* enemies, int* num_enemies,
                  GroundItem_t* items, int* num_items,
                  World_t* world );

void SpawnFloor3( NPC_t* npcs, int* num_npcs,
                  Enemy_t* enemies, int* num_enemies,
                  GroundItem_t* items, int* num_items,
                  World_t* world );

/* Per-frame deferred spawns (flag-triggered) */
void DungeonDeferredSpawns( NPC_t* npcs, int num_npcs,
                            Enemy_t* enemies, int* num_enemies,
                            World_t* world );

/* Shared spawn helpers */
int  SpawnerGetClassIdx( void );
void SpawnRandomConsumable( GroundItem_t* items, int* num_items,
                            int row, int col, int tw, int th );
void SpawnRandomT2Consumable( GroundItem_t* items, int* num_items,
                              int row, int col, int tw, int th );
void SpawnRandomEnemy( Enemy_t* enemies, int* num_enemies,
                       int x, int y, int tw, int th );
void SpawnClassElite( Enemy_t* enemies, int* num_enemies,
                      int x, int y, int tw, int th );
void SpawnT2Consumable( GroundItem_t* items, int* num_items,
                        int x, int y, int tw, int th );

#endif
