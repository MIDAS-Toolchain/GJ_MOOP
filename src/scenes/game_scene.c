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
#include "npc.h"
#include "dialogue.h"
#include "dialogue_ui.h"
#include "ground_items.h"
#include "items.h"
#include "dungeon.h"

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

/* NPCs */
static NPC_t    npcs[MAX_NPCS];
static int      num_npcs = 0;

/* Ground items */
static GroundItem_t ground_items[MAX_GROUND_ITEMS];
static int          num_ground_items = 0;
static int          consumable_used = 0;

void GameSceneUseConsumable( void ) { consumable_used = 1; }

void GameSceneInit( void )
{
  app.delegate.logic = gs_Logic;
  app.delegate.draw  = gs_Draw;

  app.options.scale_factor = 1;

  a_WidgetsInit( "resources/widgets/game_scene.auf" );
  app.active_widget = a_GetWidget( "inv_panel" );

  /* ---- Build dungeon ---- */
  tileset = a_TilesetCreate( "resources/assets/tiles/level01tilemap.png", 16, 16 );
  world   = WorldCreate( DUNGEON_W, DUNGEON_H, 16, 16 );
  DungeonBuild( world );
  DungeonPlayerStart( &player.world_x, &player.world_y );

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
  CombatSetEnemies( enemies, &num_enemies );
  CombatVFXInit();
  EnemyProjectileInit();
  EnemiesSetWorld( world );
  was_moving = 0;
  consumable_used = 0;

  /* NPCs & dialogue */
  FlagsInit();
  DialogueLoadAll();
  EnemiesSetNPCs( npcs, &num_npcs );
  NPCsInit( npcs, &num_npcs );
  DialogueUIInit( &sfx_move, &sfx_click );

  /* Ground items */
  GroundItemsInit( ground_items, &num_ground_items );

  /* Spawn all dungeon entities (items, NPCs, enemies) */
  DungeonSpawn( npcs, &num_npcs, enemies, &num_enemies,
                ground_items, &num_ground_items, world );

  TileActionsSetNPCs( npcs, &num_npcs );
  TileActionsSetGroundItems( ground_items, &num_ground_items );

  DungeonHandlerInit( world );

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

    /* Compute visibility so darkness fade works during intro */
    {
      int vr, vc;
      PlayerGetTile( &vr, &vc );
      VisibilityUpdate( vr, vc );
    }

    a_DoWidget();
    return;
  }

  /* ---- Dialogue UI (highest priority after intro) ---- */
  if ( DialogueUILogic() )
    goto gs_logic_end;

  /* ---- Tile action menu ---- */
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
  EnemyProjectileUpdate( dt );
  CombatUpdate( dt );
  CombatVFXUpdate( dt );

  /* Update visibility around the player */
  {
    int vr, vc;
    PlayerGetTile( &vr, &vc );
    VisibilityUpdate( vr, vc );
  }

  /* Pickup ground items when player finishes moving */
  if ( was_moving && !PlayerIsMoving() )
  {
    int pr, pc;
    PlayerGetTile( &pr, &pc );
    GroundItem_t* gi = GroundItemAt( ground_items, num_ground_items, pr, pc );
    if ( gi )
    {
      ConsumableInfo_t* ci = &g_consumables[gi->consumable_idx];
      int slot = InventoryAdd( INV_CONSUMABLE, gi->consumable_idx );
      if ( slot >= 0 )
      {
        gi->alive = 0;
        a_AudioPlaySound( &sfx_click, NULL );
        ConsolePushF( &console, ci->color, "Picked up %s.", ci->name );
      }
      else
      {
        ConsolePushF( &console, (aColor_t){ 0xcf, 0x57, 0x3c, 255 },
                      "Inventory full! Can't pick up %s.", ci->name );
      }
    }
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

  /* Using a consumable costs a turn */
  if ( consumable_used && !PlayerIsMoving() && !EnemiesTurning()
       && enemy_turn_delay <= 0 )
  {
    consumable_used = 0;
    enemy_turn_delay = 0.2f;
    player.turns_since_hit++;
    for ( int i = 0; i < num_enemies; i++ )
      if ( enemies[i].alive ) enemies[i].turns_since_hit++;
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
      NPC_t* bump_npc = NPCAt( npcs, num_npcs, tr, tc );
      if ( bump )
      {
        PlayerLunge( dr, dc );
        CombatAttack( bump );
      }
      else if ( bump_npc )
      {
        PlayerSetFacing( bump_npc->world_x < player.world_x );
        if ( EnemiesInCombat( enemies, num_enemies ) )
        {
          NPCType_t* nt = &g_npc_types[bump_npc->type_idx];
          CombatVFXSpawnText( bump_npc->world_x, bump_npc->world_y,
                              nt->combat_bark, nt->color );
          ConsolePushF( &console, nt->color,
                        "%s yells \"%s\"", nt->name, nt->combat_bark );
        }
        else
        {
          DialogueStart( bump_npc->type_idx );
        }
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
             && !EnemyAt( enemies, num_enemies, hover_row, hover_col )
             && !NPCAt( npcs, num_npcs, hover_row, hover_col ) )
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
  HUDDrawTopBar( EnemiesInCombat( enemies, num_enemies ) );

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

  /* Console panel (dialogue drawn later, on top of viewport) */
  if ( !DialogueActive() )
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

    /* Draw dungeon props (easel etc.) before darkness */
    DungeonDrawProps( vp_rect, &draw_cam, world, settings.gfx_mode );

    /* Draw ground items BEFORE enemies/darkness so they get dimmed */
    GroundItemsDrawAll( vp_rect, &draw_cam, ground_items, num_ground_items,
                        world, settings.gfx_mode );

    /* Draw enemies BEFORE darkness so they get dimmed too */
    EnemiesDrawAll( vp_rect, &draw_cam, enemies, num_enemies,
                    world, settings.gfx_mode );

    /* Draw NPCs before darkness */
    NPCsDrawAll( vp_rect, &draw_cam, npcs, num_npcs,
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

    /* Skeleton telegraph + projectiles */
    EnemiesDrawTelegraph( vp_rect, &draw_cam, enemies, num_enemies, world );
    EnemyProjectileDraw( vp_rect, &draw_cam );

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

  /* Dialogue UI — drawn on top of viewport */
  if ( DialogueActive() )
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    aRectf_t cr = cp->rect;
    cr.y += TransitionGetConsoleOY();
    DialogueUIDraw( cr );
  }

  /* Tile action menu (drawn outside clip rect, over everything) */
  TileActionsDraw( enemies, num_enemies );

  if ( TransitionShowLabels() )
    a_DrawWidgets();

  InventoryUISetIntroOffset( TransitionGetRightOX(), TransitionGetUIAlpha() );
  InventoryUIDraw();
}
