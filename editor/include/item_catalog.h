#ifndef __ITEM_CATALOG_H__
#define __ITEM_CATALOG_H__

#include <Archimedes.h>

#define ED_MAX_ITEMS 128

typedef struct
{
  char key[64];
  char name[64];
  char type[64];       /* "food", "ammo", "scroll", "potion", etc. */
  char glyph[8];
  aColor_t color;
  aImage_t* image;
} EdItemInfo_t;

extern EdItemInfo_t g_ed_items[];
extern int          g_ed_num_items;

#define ED_MAX_TIER 16
extern int g_ed_t1_indices[];
extern int g_ed_t1_count;
extern int g_ed_t2_indices[];
extern int g_ed_t2_count;
extern int g_ed_rare_indices[];
extern int g_ed_rare_count;

void EdItemCatalogLoad( void );
int  EdItemByKey( const char* key );

#endif
