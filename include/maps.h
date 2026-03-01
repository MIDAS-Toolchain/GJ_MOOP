#ifndef __MAPS_H__
#define __MAPS_H__

#include <Archimedes.h>

#define MAX_MAPS 8

typedef struct
{
  char key[MAX_NAME_LENGTH];
  char name[MAX_NAME_LENGTH];
  char glyph[8];
  aColor_t color;
  char effect[MAX_NAME_LENGTH];
  int target_x;
  int target_y;
  char description[256];
  aImage_t* image;
} MapInfo_t;

extern MapInfo_t g_maps[MAX_MAPS];
extern int       g_num_maps;

void MapsLoadAll( void );
int  MapByKey( const char* key );

#endif
