#include <Archimedes.h>

#include "defines.h"
#include "player.h"
#include "draw_utils.h"
#include "console.h"
#include "game_events.h"
#include "inventory_ui.h"
#include "transitions.h"
#include "sound_manager.h"
#include "world.h"
#include "game_viewport.h"
#include "game_scene.h"
#include "main_menu.h"
#include "enemies.h"
#include "combat.h"
#include "combat_vfx.h"
#include "movement.h"
#include "tile_actions.h"
#include "look_mode.h"
#include "hud.h"
#include "visibility.h"

static void gs_Logic( float );
static void gs_Draw( float );

/* Viewport zoom limits (half-height in world units) */
#define ZOOM_MIN_H       20.0f
#define ZOOM_MAX_H      100.0f

/* Panel colors — palette */
#define PANEL_BG  (aColor_t){ 0x09, 0x0a, 0x14, 200 }
#define PANEL_FG  (aColor_t){ 0xc7, 0xcf, 0xcc, 255 }
#define GOLD      (aColor_t){ 0xde, 0x9e, 0x41, 255 }

static aTileset_t*  tileset = NULL;
static World_t*     world   = NULL;
static GameCamera_t camera;

static int prev_mx = -1;
static int prev_my = -1;
static int mouse_moved = 0;

static aSoundEffect_t sfx_move;
static aSoundEffect_t sfx_click;

static Console_t console;

static int hover_row = -1, hover_col = -1;

/* Enemies */
static Enemy_t  enemies[MAX_ENEMIES];
static int      num_enemies = 0;
static int      was_moving = 0;    /* tracks previous frame's moving state */
static float    enemy_turn_delay = 0;  /* brief pause before enemies act */

void GameSceneInit( void )
{
  app.delegate.logic = gs_Logic;
  app.delegate.draw  = gs_Draw;

  app.options.scale_factor = 1;

  a_WidgetsInit( "resources/widgets/game_scene.auf" );
  app.active_widget = a_GetWidget( "inv_panel" );

  /* ---- Build dungeon from character map ----
     '#' = wall   '.' = floor   '+' = door
     Edit this to reshape the dungeon. */
  #define MAP_W 29
  #define MAP_H 25
  static const char* dungeon[MAP_H] = {
    /*         1111111111222222222 */
    /* 0123456789012345678901234567 8 */
    "#############################",  /*  0 */
    "#############################",  /*  1  north room top wall      */
    "###########.......###########",  /*  2  north room interior      */
    "###########.......###########",  /*  3                           */
    "###########.......###########",  /*  4                           */
    "##############.##############",  /*  5  opening + corridor       */
    "##############.##############",  /*  6                           */
    "##############.##############",  /*  7                           */
    "##############+##############",  /*  8  central room, north door */
    "###########.......###########",  /*  9  central room interior    */
    "##.....####.......####.....##",  /* 10  west room + east room    */
    "##.....####.......####.....##",  /* 11                           */
    "##........+.......+........##",  /* 12  west door + east door    */
    "##.....####.......####.....##",  /* 13                           */
    "##.....####.......####.....##",  /* 14                           */
    "###########.......###########",  /* 15  central room interior    */
    "##############+##############",  /* 16  central room, south door */
    "##############.##############",  /* 17  corridor                 */
    "##############.##############",  /* 18                           */
    "##############.##############",  /* 19  opening                  */
    "###########.......###########",  /* 20  south room interior      */
    "###########.......###########",  /* 21                           */
    "###########.......###########",  /* 22                           */
    "#############################",  /* 23  south room bottom wall   */
    "#############################",  /* 24                           */
  };

  tileset = a_TilesetCreate( "resources/assets/tiles/level01tilemap.png", 16, 16 );
  world   = WorldCreate( MAP_W, MAP_H, 16, 16 );

  /* Parse the character map into tiles */
  for ( int y = 0; y < MAP_H; y++ )
  {
    for ( int x = 0; x < MAP_W; x++ )
    {
      int  idx = y * MAP_W + x;
      char c   = dungeon[y][x];

      if ( c == '#' )
      {
        world->background[idx].tile     = 1;
        world->background[idx].glyph    = "#";
        world->background[idx].glyph_fg = (aColor_t){ 0x81, 0x97, 0x96, 255 };
        world->background[idx].solid    = 1;
      }
      else if ( c == '+' )
      {
        /* Floor underneath the door */
        world->background[idx].tile     = 0;
        world->background[idx].glyph    = ".";
        world->background[idx].glyph_fg = (aColor_t){ 0x39, 0x4a, 0x50, 255 };
        /* Door on midground (solid until opened) */
        world->midground[idx].tile     = 2;
        world->midground[idx].glyph    = "+";
        world->midground[idx].glyph_fg = (aColor_t){ 0xc0, 0x94, 0x73, 255 };
        world->midground[idx].solid    = 1;
      }
      /* '.' tiles keep the WorldCreate default (floor, tile 0) */
    }
  }

  /* Give each central-room door its own color + tile */
  {
    struct { int x, y; uint32_t tile; aColor_t color; } doors[] = {
      { 14,  8, 2, { 0x4f, 0x8f, 0xba, 255 } },  /* north: blue  */
      { 14, 16, 3, { 0x75, 0xa7, 0x43, 255 } },  /* south: green */
      { 10, 12, 4, { 0xa5, 0x30, 0x30, 255 } },  /* west:  red   */
      { 18, 12, 5, { 0xc7, 0xcf, 0xcc, 255 } },  /* east:  white */
    };
    for ( int i = 0; i < 4; i++ )
    {
      int idx = doors[i].y * MAP_W + doors[i].x;
      world->midground[idx].tile     = doors[i].tile;
      world->midground[idx].glyph_fg = doors[i].color;
    }
  }

  /* Player starts in center of central room (tile 14,12) */
  player.world_x = 14 * 16 + 8.0f;
  player.world_y = 12 * 16 + 8.0f;

  /* Initialize game camera centered on player */
  camera = (GameCamera_t){ player.world_x, player.world_y, 64.0f };
  app.g_viewport = (aRectf_t){ 0, 0, 0, 0 };

  hover_row = hover_col = -1;

  a_AudioLoadSound( "resources/soundeffects/menu_move.wav", &sfx_move );
  a_AudioLoadSound( "resources/soundeffects/menu_click.wav", &sfx_click );

  /* Init subsystems */
  MovementInit( world );
  TileActionsInit( world, &camera, &console, &sfx_move, &sfx_click );
  LookModeInit( world, &console, &sfx_move, &sfx_click );
  InventoryUIInit( &sfx_move, &sfx_click );

  VisibilityInit( world );

  ConsoleInit( &console );
  GameEventsInit( &console );
  ConsolePush( &console, "Welcome, adventurer.", white );

  /* Enemies & combat */
  EnemiesLoadTypes();
  EnemiesInit( enemies, &num_enemies );
  CombatInit( &console );
  CombatVFXInit();
  EnemiesSetWorld( world );
  EnemySpawn( enemies, &num_enemies, EnemyTypeByKey( "rat" ),
              13, 10, world->tile_w, world->tile_h );  /* central */
  EnemySpawn( enemies, &num_enemies, EnemyTypeByKey( "rat" ),
              14, 3, world->tile_w, world->tile_h );   /* north */
  EnemySpawn( enemies, &num_enemies, EnemyTypeByKey( "rat" ),
              14, 21, world->tile_w, world->tile_h );  /* south */
  EnemySpawn( enemies, &num_enemies, EnemyTypeByKey( "rat" ),
              4, 12, world->tile_w, world->tile_h );   /* west */
  EnemySpawn( enemies, &num_enemies, EnemyTypeByKey( "rat" ),
              24, 12, world->tile_w, world->tile_h );  /* east */
  was_moving = 0;

  SoundManagerPlayGame();
  TransitionIntroStart();
}

static void gs_Logic( float dt )
{
  a_DoInput();
  SoundManagerUpdate( dt );

  /* Track whether the mouse actually moved this frame */
  mouse_moved = ( app.mouse.x != prev_mx || app.mouse.y != prev_my );
  prev_mx = app.mouse.x;
  prev_my = app.mouse.y;

  /* Intro cinematic — blocks all input until done */
  if ( TransitionIntroActive() )
  {
    if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
    {
      app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
      TransitionIntroSkip();
    }
    else
    {
      TransitionIntroUpdate( dt );
    }

    /* Override camera during intro */
    camera.half_h = TransitionGetViewportH();
    camera.x = player.world_x;
    camera.y = player.world_y;

    a_DoWidget();
    return;
  }

  /* ---- Tile action menu (highest priority after intro) ---- */
  if ( TileActionsLogic( mouse_moved, enemies, num_enemies ) )
    goto gs_logic_end;

  /* ---- ESC: close look mode, then inventory menus, then main menu ---- */
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    if ( LookModeActive() )
      LookModeExit();
    else if ( !InventoryUICloseMenus() )
    {
      a_WidgetCacheFree();
      MainMenuInit();
      return;
    }
  }

  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/game_scene.auf" );
  }

  InventoryUILogic( mouse_moved );

  /* When inventory is focused, consume WASD/arrows */
  if ( InventoryUIFocused() )
  {
    app.keyboard[SDL_SCANCODE_W] = 0;
    app.keyboard[SDL_SCANCODE_S] = 0;
    app.keyboard[SDL_SCANCODE_A] = 0;
    app.keyboard[SDL_SCANCODE_D] = 0;
    app.keyboard[SDL_SCANCODE_UP] = 0;
    app.keyboard[SDL_SCANCODE_DOWN] = 0;
    app.keyboard[SDL_SCANCODE_LEFT] = 0;
    app.keyboard[SDL_SCANCODE_RIGHT] = 0;
  }

  /* ---- Movement update (always runs, even in look mode) ---- */
  MovementUpdate( dt );
  EnemiesUpdate( dt );
  CombatUpdate( dt );
  CombatVFXUpdate( dt );

  /* Update visibility around the player */
  {
    int vr, vc;
    PlayerGetTile( &vr, &vc );
    VisibilityUpdate( vr, vc );
  }

  /* Enemy turn: brief pause after player finishes, then enemies act */
  if ( was_moving && !PlayerIsMoving() && !EnemiesTurning() )
  {
    enemy_turn_delay = 0.2f;
    player.turns_since_hit++;
    for ( int i = 0; i < num_enemies; i++ )
      if ( enemies[i].alive ) enemies[i].turns_since_hit++;
  }
  was_moving = PlayerIsMoving();

  if ( enemy_turn_delay > 0 )
  {
    enemy_turn_delay -= dt;
    if ( enemy_turn_delay <= 0 )
    {
      enemy_turn_delay = 0;
      int pr, pc;
      PlayerGetTile( &pr, &pc );
      EnemiesStartTurn( enemies, num_enemies, pr, pc, TileWalkable );
    }
  }

  /* ---- Look mode ---- */
  if ( LookModeLogic( mouse_moved ) )
  {
    hover_row = -1;
    hover_col = -1;
    goto gs_logic_end;
  }

  /* ---- L key: enter look mode ---- */
  if ( app.keyboard[SDL_SCANCODE_L] == 1 )
  {
    app.keyboard[SDL_SCANCODE_L] = 0;
    int pr, pc;
    PlayerGetTile( &pr, &pc );
    LookModeEnter( pr, pc );
  }

  /* ---- Arrow key / WASD movement (when not moving, not inventory focused) ---- */
  if ( !PlayerIsMoving() && !EnemiesTurning() && !InventoryUIFocused()
       && enemy_turn_delay <= 0 )
  {
    int dr = 0, dc = 0;
    if ( app.keyboard[SDL_SCANCODE_UP] == 1 || app.keyboard[SDL_SCANCODE_W] == 1 )
    { app.keyboard[SDL_SCANCODE_UP] = 0; app.keyboard[SDL_SCANCODE_W] = 0; dc = -1; }
    else if ( app.keyboard[SDL_SCANCODE_DOWN] == 1 || app.keyboard[SDL_SCANCODE_S] == 1 )
    { app.keyboard[SDL_SCANCODE_DOWN] = 0; app.keyboard[SDL_SCANCODE_S] = 0; dc =  1; }
    else if ( app.keyboard[SDL_SCANCODE_LEFT] == 1 || app.keyboard[SDL_SCANCODE_A] == 1 )
    { app.keyboard[SDL_SCANCODE_LEFT] = 0; app.keyboard[SDL_SCANCODE_A] = 0; dr = -1; }
    else if ( app.keyboard[SDL_SCANCODE_RIGHT] == 1 || app.keyboard[SDL_SCANCODE_D] == 1 )
    { app.keyboard[SDL_SCANCODE_RIGHT] = 0; app.keyboard[SDL_SCANCODE_D] = 0; dr =  1; }

    if ( dr != 0 || dc != 0 )
    {
      int pr, pc;
      PlayerGetTile( &pr, &pc );
      int tr = pr + dr;
      int tc = pc + dc;

      /* Bump-to-attack: enemy on target tile */
      Enemy_t* bump = EnemyAt( enemies, num_enemies, tr, tc );
      if ( bump )
      {
        PlayerLunge( dr, dc );
        CombatAttack( bump );
      }
      else if ( TileWalkable( tr, tc ) )
        PlayerStartMove( tr, tc );
      else if ( TileHasDoor( tr, tc ) )
      {
        if ( TileActionsTryOpen( tr, tc ) )
          PlayerStartMove( tr, tc );
        else
          PlayerShake( dr, dc );
      }
      else
        PlayerWallBump( dr, dc );
    }
  }

  /* ---- Mouse hover over viewport tiles ---- */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    int mx = app.mouse.x;
    int my = app.mouse.y;
    hover_row = -1;
    hover_col = -1;

    if ( PointInRect( mx, my, vp->rect.x, vp->rect.y, vp->rect.w, vp->rect.h ) )
    {
      float wx, wy;
      GV_ScreenToWorld( vp->rect, &camera, mx, my, &wx, &wy );
      int r = (int)( wx / world->tile_w );
      int c = (int)( wy / world->tile_h );
      if ( r >= 0 && r < world->width && c >= 0 && c < world->height
           && VisibilityGet( r, c ) > 0.01f )
      {
        hover_row = r;
        hover_col = c;
      }

      /* Mouse is over a tile — clear inventory focus/highlights */
      if ( mouse_moved && hover_row >= 0 )
        InventoryUIUnfocus();

      /* Click on tile (blocked while moving / enemies turning / delay) */
      if ( app.mouse.pressed && hover_row >= 0
           && !PlayerIsMoving() && !EnemiesTurning()
           && enemy_turn_delay <= 0 )
      {
        /* Rapid-move: click adjacent walkable tile within window */
        if ( RapidMoveActive()
             && TileAdjacent( hover_row, hover_col )
             && TileWalkable( hover_row, hover_col )
             && !EnemyAt( enemies, num_enemies, hover_row, hover_col ) )
        {
          app.mouse.pressed = 0;
          PlayerStartMove( hover_row, hover_col );
        }
        /* Normal click — open action menu */
        else
        {
          app.mouse.pressed = 0;
          int pr, pc;
          PlayerGetTile( &pr, &pc );
          int on_self = ( hover_row == pr && hover_col == pc );
          TileActionsOpen( hover_row, hover_col, on_self );
        }
      }
    }
  }

  /* ---- Mouse wheel: console scroll or viewport zoom ---- */
  if ( app.mouse.wheel != 0 )
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    if ( PointInRect( app.mouse.x, app.mouse.y,
                      cp->rect.x, cp->rect.y, cp->rect.w, cp->rect.h ) )
    {
      ConsoleScroll( &console, app.mouse.wheel );
      app.mouse.wheel = 0;
    }
  }
  if ( app.mouse.wheel != 0 )
  {
    GV_Zoom( &camera, app.mouse.wheel, ZOOM_MIN_H, ZOOM_MAX_H );
    app.mouse.wheel = 0;
  }

gs_logic_end:
  /* Always center camera on player + shake offset */
  camera.x = player.world_x + PlayerShakeOX();
  camera.y = player.world_y + PlayerShakeOY();

  a_DoWidget();
}

static void gs_Draw( float dt )
{
  (void)dt;

  /* Top bar */
  HUDDrawTopBar();

  /* Game viewport — shrink 1px on right so it doesn't overlap right panels */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    aRectf_t vr = { vp->rect.x, vp->rect.y, vp->rect.w - 1, vp->rect.h };
    float va = TransitionGetViewportAlpha();
    a_DrawFilledRect( vr, (aColor_t){ 0x09, 0x0a, 0x14, (int)( 255 * va ) } );
    a_DrawRect( vr, (aColor_t){ 0x39, 0x4a, 0x50, (int)( 255 * va ) } );
  }

  /* Inventory panel background */
  {
    aContainerWidget_t* ip = a_GetContainerFromWidget( "inv_panel" );
    aRectf_t ir = ip->rect;
    ir.x += TransitionGetRightOX();
    a_DrawFilledRect( ir, PANEL_BG );
  }

  /* Equipment panel background */
  {
    aContainerWidget_t* kp = a_GetContainerFromWidget( "key_panel" );
    aRectf_t kr = kp->rect;
    kr.x += TransitionGetRightOX();
    a_DrawFilledRect( kr, PANEL_BG );
  }

  /* Console panel */
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    aRectf_t cr = cp->rect;
    cr.y += TransitionGetConsoleOY();
    aColor_t con_bg = PANEL_BG;
    con_bg.a = (int)( con_bg.a * TransitionGetUIAlpha() );
    aColor_t con_fg = PANEL_FG;
    con_fg.a = (int)( con_fg.a * TransitionGetUIAlpha() );
    a_DrawFilledRect( cr, con_bg );
    a_DrawRect( cr, con_fg );
    ConsoleDraw( &console, cr );
  }

  /* World + Player — clipped to game_viewport panel */
  {
    aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
    aRectf_t clip = { vp->rect.x + 1, vp->rect.y + 1, vp->rect.w - 3, vp->rect.h - 2 };
    a_SetClipRect( clip );

    aRectf_t vp_rect = vp->rect;

    /* Apply combat hit shake to camera */
    GameCamera_t draw_cam = camera;
    draw_cam.x += CombatShakeOX();
    draw_cam.y += CombatShakeOY();

    if ( world && tileset )
      GV_DrawWorld( vp_rect, &draw_cam, world, tileset, settings.gfx_mode == GFX_ASCII );

    /* Draw enemies BEFORE darkness so they get dimmed too */
    EnemiesDrawAll( vp_rect, &draw_cam, enemies, num_enemies,
                    world, settings.gfx_mode );

    /* Darkness overlay — covers world + enemies, player drawn on top.
       fade param: 0 = all black (intro start), 1 = normal visibility. */
    GV_DrawDarkness( vp_rect, &draw_cam, world,
                     TransitionGetViewportAlpha() );

    /* Hover highlight */
    if ( hover_row >= 0 && hover_col >= 0 )
      GV_DrawTileOutline( vp_rect, &draw_cam, hover_row, hover_col,
                          world->tile_w, world->tile_h,
                          (aColor_t){ 0xeb, 0xed, 0xe9, 80 } );

    /* Look mode cursor */
    LookModeDraw( vp_rect, &draw_cam );

    /* Tile action menu target highlight */
    if ( TileActionsIsOpen() )
      GV_DrawTileOutline( vp_rect, &draw_cam,
                          TileActionsGetRow(), TileActionsGetCol(),
                          world->tile_w, world->tile_h, GOLD );

    /* Player sprite (drawn after darkness — always visible) */
    PlayerDraw( vp_rect, &draw_cam, settings.gfx_mode );

    /* Health bars — hide after 2 turns without taking damage */
    for ( int i = 0; i < num_enemies; i++ )
    {
      if ( !enemies[i].alive ) continue;
      if ( enemies[i].turns_since_hit >= 2 ) continue;
      int ex = (int)( enemies[i].world_x / world->tile_w );
      int ey = (int)( enemies[i].world_y / world->tile_h );
      if ( VisibilityGet( ex, ey ) < 0.01f ) continue;
      EnemyType_t* et = &g_enemy_types[enemies[i].type_idx];
      CombatVFXDrawHealthBar( vp_rect, &draw_cam,
                              enemies[i].world_x, enemies[i].world_y,
                              enemies[i].hp, et->hp );
    }
    if ( player.turns_since_hit < 2 )
      CombatVFXDrawHealthBar( vp_rect, &draw_cam,
                              player.world_x, player.world_y,
                              player.hp, player.max_hp );

    /* Floating damage numbers */
    CombatVFXDraw( vp_rect, &draw_cam );

    /* Red flash overlay on hit (fires once, won't restart while active) */
    float flash = CombatFlashAlpha();
    if ( flash > 0.5f )
      a_DrawFilledRect( clip, (aColor_t){ 0xa5, 0x30, 0x30, (int)flash } );

    a_DisableClipRect();
  }

  /* Tile action menu (drawn outside clip rect, over everything) */
  TileActionsDraw( enemies, num_enemies );

  if ( TransitionShowLabels() )
    a_DrawWidgets();

  InventoryUISetIntroOffset( TransitionGetRightOX(), TransitionGetUIAlpha() );
  InventoryUIDraw();
}
