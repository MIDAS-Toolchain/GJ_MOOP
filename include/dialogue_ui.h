#ifndef __DIALOGUE_UI_H__
#define __DIALOGUE_UI_H__

#include <Archimedes.h>

void DialogueUIInit( aSoundEffect_t* move, aSoundEffect_t* click );
void DialogueUISetRect( aRectf_t rect );
int  DialogueUILogic( void );
void DialogueUIDraw( aRectf_t panel_rect );

#endif
