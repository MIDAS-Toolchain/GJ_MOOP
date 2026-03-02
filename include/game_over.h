#ifndef __GAME_OVER_H__
#define __GAME_OVER_H__

void GameOverCheck( float dt );  /* call every frame; triggers overlay when hp<=0 */
int  GameOverActive( void );     /* 1 if overlay is up and blocking input */
int  GameOverLogic( float dt );  /* 1 = consuming input, 2 = exit to main menu */
void GameOverDraw( void );
void GameOverReset( void );      /* call on scene init to clear state */

#endif
