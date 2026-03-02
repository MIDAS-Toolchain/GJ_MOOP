#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__

#include "game_viewport.h"

void GameCameraInit( GameCamera_t* cam );
void GameCameraFollow( void );
int  GameCameraIntro( float dt );

#endif
