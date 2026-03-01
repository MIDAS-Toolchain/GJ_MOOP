#ifndef __DEFINES_H__
#define __DEFINES_H__

#include <Archimedes.h>
#include "player.h"

#define GFX_IMAGE  0
#define GFX_ASCII  1

#define WORLD_WIDTH   512.0f
#define WORLD_HEIGHT  512.0f

typedef struct
{
  int gfx_mode;    /* GFX_IMAGE or GFX_ASCII */
  int music_vol;   /* 0-100 */
  int sfx_vol;     /* 0-100 */
} GameSettings_t;

extern Player_t player;
extern GameSettings_t settings;

#endif
