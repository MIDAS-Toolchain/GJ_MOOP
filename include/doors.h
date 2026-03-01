#ifndef __DOORS_H__
#define __DOORS_H__

#include "world.h"
#include "console.h"

#define DOOR_BLUE    2   /* Mage */
#define DOOR_GREEN   3   /* Rogue */
#define DOOR_RED     4   /* Mercenary */
#define DOOR_WHITE   5   /* Anyone */

void  DoorsInit( Console_t* con );
void  DoorPlace( World_t* w, int x, int y, int type );
int   DoorIsDoor( uint32_t tile );
int   DoorCanOpen( uint32_t tile );
int   DoorTryOpen( World_t* w, int x, int y );
void  DoorDescribe( World_t* w, int x, int y );

#endif
