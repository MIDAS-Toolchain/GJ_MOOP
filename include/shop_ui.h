#ifndef __SHOP_UI_H__
#define __SHOP_UI_H__

#include <Archimedes.h>
#include "console.h"

void ShopUIInit( aSoundEffect_t* move, aSoundEffect_t* click, Console_t* con );
void ShopUIOpen( int shop_item_index );
int  ShopUILogic( void );
void ShopUIDraw( aRectf_t panel_rect );
int  ShopUIActive( void );

#endif
