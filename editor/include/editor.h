/*
 * editor.h:
 *
 * Copyright (c) 2026 Jacob Kellum <jkellum819@gmail.com>
 *                    Mathew Storm <smattymat@gmail.com>
 ************************************************************************
 */

#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <Archimedes.h>
#include <Daedalus.h>

#include "ed_defines.h"
#include "ed_structs.h"

extern aColor_t master_colors[MAX_COLOR_GROUPS][48];
extern Tileset_t* g_tile_sets[MAX_TILESETS];
extern dArray_t* g_map_filenames;

void EditorInit( void );
void EditorDestroy( void );

#endif

