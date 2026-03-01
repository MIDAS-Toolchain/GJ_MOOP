#ifndef __NPC_H__
#define __NPC_H__

#include <Archimedes.h>
#include "dialogue.h"
#include "world.h"
#include "game_viewport.h"

#define MAX_NPCS 16

/* NPC instance — spawned in world */
typedef struct
{
  int   type_idx;          /* index into g_npc_types[] */
  int   row, col;
  float world_x, world_y;
  int   alive;
} NPC_t;

void   NPCsInit( NPC_t* list, int* count );
NPC_t* NPCSpawn( NPC_t* list, int* count,
                  int type_idx, int row, int col,
                  int tile_w, int tile_h );
NPC_t* NPCAt( NPC_t* list, int count, int row, int col );

void NPCsDrawAll( aRectf_t vp_rect, GameCamera_t* cam,
                   NPC_t* list, int count,
                   World_t* world, int gfx_mode );

#endif
