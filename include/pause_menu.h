#ifndef __PAUSE_MENU_H__
#define __PAUSE_MENU_H__

void PauseMenuOpen( void );
void PauseMenuClose( void );
int  PauseMenuActive( void );
int  PauseMenuLogic( void );   /* 1 = consuming input, 2 = exit to main menu */
void PauseMenuDraw( void );

#endif
