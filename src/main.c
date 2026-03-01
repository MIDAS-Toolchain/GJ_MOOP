#include <stdio.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <Archimedes.h>
#include <Daedalus.h>
#include "defines.h"
#include "sound_manager.h"
#include "persist.h"
#include "lore.h"
#include "main_menu.h"

Player_t player;
GameSettings_t settings = { .gfx_mode = GFX_IMAGE, .music_vol = 100, .sfx_vol = 100 };

void aMainloop( void )
{
  float dt = a_GetDeltaTime();
  a_TimerStart( app.time.FPS_cap_timer );
  a_GetFPS();
  a_PrepareScene();
  
  app.delegate.logic( dt );
  app.delegate.draw( dt );
  
  a_PresentScene();
  app.time.frames++;
  
  if ( app.options.frame_cap )
  {
    int frame_tick = a_TimerGetTicks( app.time.FPS_cap_timer );
    if ( frame_tick < LOGIC_RATE )
    {
      SDL_Delay( LOGIC_RATE - frame_tick );
    }
  }
}

int main( void )
{
  a_Init( SCREEN_WIDTH, SCREEN_HEIGHT, "Archimedes" );

  dLogConfig_t log_cfg = {
    .default_level    = D_LOG_LEVEL_DEBUG,
    .colorize_output  = true,
    .include_timestamp = true
  };
  dLogger_t* logger = d_CreateLogger( log_cfg );
  d_SetGlobalLogger( logger );

  SoundManagerInit();
  PersistInit();

  /* Load persisted settings */
  {
    char* s = PersistLoad( "settings" );
    if ( s )
    {
      sscanf( s, "%d\n%d\n%d",
              &settings.gfx_mode, &settings.music_vol, &settings.sfx_vol );
      free( s );
    }
    SoundManagerSetMusicVolume( settings.music_vol );
    SoundManagerSetSfxVolume( settings.sfx_vol );
  }

  LoreLoadDefinitions();
  LoreLoadSave();
  MainMenuInit();

  #ifdef __EMSCRIPTEN__
    emscripten_set_main_loop( aMainloop, 0, 1 );
  #endif

  #ifndef __EMSCRIPTEN__
    while( app.running ) {
      aMainloop();
    }
  #endif
  
  a_Quit();

  return 0;
}

