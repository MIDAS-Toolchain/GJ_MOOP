#ifndef SPAWN_DATA_H
#define SPAWN_DATA_H

typedef enum
{
  SPAWN_RANDOM_T1,
  SPAWN_RANDOM_T2,
  SPAWN_CONSUMABLE,
  SPAWN_EQUIPMENT,
  SPAWN_MAP,
  SPAWN_CLASS_RARE,
  SPAWN_CLASS_EQUIPMENT,
  SPAWN_POOL,
  SPAWN_TYPE_COUNT
} SpawnType_t;

typedef struct
{
  int          row, col;
  SpawnType_t  type;
  char         key[64];           /* consumable/equipment/map key */
  char         class_keys[3][64]; /* [0]=mercenary [1]=rogue [2]=mage */
  int          pool_id;           /* index into SpawnList_t.pools */
} SpawnPoint_t;

typedef struct
{
  char  pick_type[32];          /* "equipment", "consumable", "class_equipment" */
  char  pick_key[64];           /* e.g. "golden_ring" (single-item picks) */
  char  pick_class_keys[3][64]; /* [0]=mercenary [1]=rogue [2]=mage */
  char  fill_type[32];          /* e.g. "random_t2" */
} SpawnPool_t;

typedef struct
{
  SpawnPoint_t*  points;
  int            count;
  int            capacity;
  SpawnPool_t*   pools;
  int            num_pools;
  int            pool_capacity;
} SpawnList_t;

void SpawnListInit( SpawnList_t* list );
void SpawnListDestroy( SpawnList_t* list );
void SpawnListAdd( SpawnList_t* list, SpawnPoint_t point );
void SpawnListRemoveAt( SpawnList_t* list, int index );
int  SpawnListFindAt( SpawnList_t* list, int row, int col );
int  SpawnListAddPool( SpawnList_t* list, SpawnPool_t pool );

/* DUF read/write */
int  SpawnDUFLoad( const char* path, SpawnList_t* list );
int  SpawnDUFSave( const char* path, SpawnList_t* list );
void SpawnDUFFilename( const char* map_path, char* out, int out_size );

#endif
