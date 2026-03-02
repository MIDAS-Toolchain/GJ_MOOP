#ifndef __DOORS_H__
#define __DOORS_H__

#include "world.h"
#include "console.h"

/* Horizontal doors (walls left/right, passage up/down) */
#define DOOR_BLUE    2   /* Mage */
#define DOOR_GREEN   3   /* Rogue */
#define DOOR_RED     4   /* Mercenary */
#define DOOR_WHITE   5   /* Anyone */

/* Vertical doors (walls above/below, passage left/right) */
#define DOOR_BLUE_V   11
#define DOOR_GREEN_V  12
#define DOOR_RED_V    13
#define DOOR_WHITE_V  14

void  DoorsInit( Console_t* con );
void  DoorPlace( World_t* w, int x, int y, int type, int vertical );
int   DoorIsDoor( uint32_t tile );
int   DoorCanOpen( uint32_t tile );
int   DoorTryOpen( World_t* w, int x, int y );
void  DoorDescribe( World_t* w, int x, int y );

#endif
