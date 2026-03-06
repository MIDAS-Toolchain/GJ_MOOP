#include <Archimedes.h>
#include "defines.h"

#define DEAD_ZONE 3

static int g_input_mode = INPUT_MOUSE;
static int g_last_mx = -1;
static int g_last_my = -1;
static int g_initialized = 0;

void InputModeUpdate( void )
{
  int mx = app.mouse.x;
  int my = app.mouse.y;

  if ( !g_initialized )
  {
    g_last_mx = mx;
    g_last_my = my;
    g_initialized = 1;
    return;
  }

  /* Keyboard check first — any key pressed this frame switches to keyboard */
  for ( int i = 0; i < SDL_NUM_SCANCODES; i++ )
  {
    if ( app.keyboard[i] == 1 )
    {
      g_input_mode = INPUT_KEYBOARD;
      g_last_mx = mx;
      g_last_my = my;
      return;
    }
  }

  /* Mouse movement with dead zone switches to mouse */
  int dx = mx - g_last_mx;
  int dy = my - g_last_my;
  if ( dx * dx + dy * dy > DEAD_ZONE * DEAD_ZONE )
  {
    g_input_mode = INPUT_MOUSE;
    g_last_mx = mx;
    g_last_my = my;
  }
}

int InputModeGet( void )
{
  return g_input_mode;
}
