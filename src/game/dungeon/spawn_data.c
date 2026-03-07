#include <stdlib.h>
#include <string.h>

#include "spawn_data.h"

#define SPAWN_INIT_CAP  32
#define POOL_INIT_CAP   4

void SpawnListInit( SpawnList_t* list )
{
  list->points   = malloc( sizeof(SpawnPoint_t) * SPAWN_INIT_CAP );
  list->count    = 0;
  list->capacity = SPAWN_INIT_CAP;

  list->pools         = malloc( sizeof(SpawnPool_t) * POOL_INIT_CAP );
  list->num_pools     = 0;
  list->pool_capacity = POOL_INIT_CAP;
}

void SpawnListDestroy( SpawnList_t* list )
{
  free( list->points );
  free( list->pools );
  list->points   = NULL;
  list->pools    = NULL;
  list->count    = 0;
  list->capacity = 0;
  list->num_pools     = 0;
  list->pool_capacity = 0;
}

void SpawnListAdd( SpawnList_t* list, SpawnPoint_t point )
{
  if ( list->count >= list->capacity )
  {
    list->capacity *= 2;
    list->points = realloc( list->points,
                            sizeof(SpawnPoint_t) * list->capacity );
  }
  list->points[list->count++] = point;
}

void SpawnListRemoveAt( SpawnList_t* list, int index )
{
  if ( index < 0 || index >= list->count ) return;
  list->points[index] = list->points[--list->count];
}

int SpawnListFindAt( SpawnList_t* list, int row, int col )
{
  for ( int i = 0; i < list->count; i++ )
  {
    if ( list->points[i].row == row && list->points[i].col == col )
      return i;
  }
  return -1;
}

int SpawnListAddPool( SpawnList_t* list, SpawnPool_t pool )
{
  if ( list->num_pools >= list->pool_capacity )
  {
    list->pool_capacity *= 2;
    list->pools = realloc( list->pools,
                           sizeof(SpawnPool_t) * list->pool_capacity );
  }
  list->pools[list->num_pools] = pool;
  return list->num_pools++;
}
