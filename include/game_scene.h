#ifndef __GAME_SCENE_H__
#define __GAME_SCENE_H__

void GameSceneInit( void );

/* Signal that a consumable was used — triggers enemy turn cost */
void GameSceneUseConsumable( void );

#endif
