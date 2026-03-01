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
#include "quest_tracker.h"
#include "objects.h"
#include "maps.h"
#include "shop.h"
#include "shop_ui.h"
#include "poison_pool.h"
#include "placed_traps.h"
#include "spell_vfx.h"
#include "target_mode.h"
#include "pause_menu.h"

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
static int hud_pause_clicked = 0;

/* Enemies */
static Enemy_t  enemies[MAX_ENEMIES];
static int      num_enemies = 0;
static struct {
  int   was_moving;    /* previous frame's moving state */
  float enemy_delay;   /* brief pause before enemies act */
} turn;

/* NPCs */
static NPC_t    npcs[MAX_NPCS];
static int      num_npcs = 0;

/* Ground items */
static GroundItem_t ground_items[MAX_GROUND_ITEMS];
static int          num_ground_items = 0;

/* Frame-level player tile cache — set once in update_systems() */
static int frame_pr, frame_pc;

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
  ConsoleInit( &console );
  ObjectsInit( &console );
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
  TargetModeInit( world, &console, &camera, &sfx_move, &sfx_click );
  InventoryUIInit( &sfx_move, &sfx_click );

  VisibilityInit( world );

  GameEventsInit( &console );
  ConsolePush( &console, "Welcome, adventurer.", white );

  /* Enemies & combat */
  EnemiesLoadTypes();
  EnemiesInit( enemies, &num_enemies );
  CombatInit( &console );
  CombatSetEnemies( enemies, &num_enemies );
  CombatSetGroundItems( ground_items, &num_ground_items );
  CombatVFXInit();
  EnemyProjectileInit();
  EnemiesSetWorld( world );
  turn.was_moving = 0;
  turn.enemy_delay = 0;

  /* NPCs & dialogue */
  FlagsInit();
  DialogueLoadAll();
  EnemiesSetNPCs( npcs, &num_npcs );
  NPCsInit( npcs, &num_npcs );
  DialogueUIInit( &sfx_move, &sfx_click );

  /* Ground items */
  GroundItemsInit( ground_items, &num_ground_items );

  /* Shop */
  ShopLoadPool( "resources/data/shops/floor_01_shop.duf" );
  ShopUIInit( &sfx_move, &sfx_click, &console );

  /* Poison pools */
  PoisonPoolInit( &console );

  /* Placed traps */
  PlacedTrapsInit( &console );

  /* Spell VFX */
  SpellVFXInit( world );

  /* Spawn all dungeon entities (items, NPCs, enemies) */
  DungeonSpawn( npcs, &num_npcs, enemies, &num_enemies,
                ground_items, &num_ground_items, world );

  TileActionsSetNPCs( npcs, &num_npcs );
  TileActionsSetGroundItems( ground_items, &num_ground_items );

  DungeonHandlerInit( world );

  SoundManagerPlayGame();
  TransitionIntroStart();
}

/* ===== gs_Logic helper functions ===== */

static void frame_begin( float dt )
{
  a_DoInput();
  SoundManagerUpdate( dt );
  mouse_moved = ( app.mouse.x != prev_mx || app.mouse.y != prev_my );
  prev_mx = app.mouse.x;
  prev_my = app.mouse.y;
}

static void frame_end( void )
{
  camera.x = player.world_x + PlayerShakeOX();
  camera.y = player.world_y + PlayerShakeOY();
  a_DoWidget();
}

static int handle_intro( float dt )
{
  if ( !TransitionIntroActive() ) return 0;

  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    TransitionIntroSkip();
  }
  else
  {
    TransitionIntroUpdate( dt );
  }

  camera.half_h = TransitionGetViewportH();
  camera.x = player.world_x;
  camera.y = player.world_y;

  {
    int vr, vc;
    PlayerGetTile( &vr, &vc );
    VisibilityUpdate( vr, vc );
  }

  a_DoWidget();
  return 1;
}

static int handle_overlays( void )
{
  if ( DialogueUILogic() ) return 1;
  if ( ShopUILogic() )     return 1;
  if ( TileActionsLogic( mouse_moved, enemies, num_enemies ) ) return 1;
  return 0;
}

/* Returns 1 if the scene exited (caller should return without frame_end) */
static int handle_esc_key( void )
{
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] != 1 ) return 0;

  app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
  if ( TargetModeActive() )
    TargetModeExit();
  else if ( LookModeActive() )
    LookModeExit();
  else if ( !InventoryUICloseMenus() )
  {
    PauseMenuOpen();
  }
  return 0;
}

static void handle_inventory( void )
{
  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/game_scene.auf" );
  }

  InventoryUILogic( mouse_moved );

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
}

static void update_systems( float dt )
{
  MovementUpdate( dt );
  EnemiesUpdate( dt );
  EnemyProjectileUpdate( dt );
  CombatUpdate( dt );
  CombatVFXUpdate( dt );
  SpellVFXUpdate( dt );

  PlayerGetTile( &frame_pr, &frame_pc );
  VisibilityUpdate( frame_pr, frame_pc );
}

/* Player's move just resolved — pickup items, tick poison, advance turn */
static void handle_turn_end( float dt )
{
  if ( turn.was_moving && !PlayerIsMoving() )
  {
    /* Ground pickup / shop prompt */
    ShopItem_t* si = ShopItemAt( frame_pr, frame_pc );
    if ( si )
    {
      ShopUIOpen( (int)( si - g_shop_items ) );
    }
    else
    {
      GroundItem_t* gi = GroundItemAt( ground_items, num_ground_items,
                                       frame_pr, frame_pc );
      if ( gi )
      {
        int inv_type = ( gi->item_type == GROUND_MAP ) ? INV_MAP : INV_CONSUMABLE;
        const char* iname;
        aColor_t    icolor;
        if ( gi->item_type == GROUND_MAP )
        {
          iname  = g_maps[gi->item_idx].name;
          icolor = g_maps[gi->item_idx].color;
        }
        else
        {
          iname  = g_consumables[gi->item_idx].name;
          icolor = g_consumables[gi->item_idx].color;
        }

        int slot = InventoryAdd( inv_type, gi->item_idx );
        if ( slot >= 0 )
        {
          gi->alive = 0;
          a_AudioPlaySound( &sfx_click, NULL );
          ConsolePushF( &console, icolor, "Picked up %s.", iname );
        }
        else
        {
          ConsolePushF( &console, (aColor_t){ 0xcf, 0x57, 0x3c, 255 },
                        "Inventory full! Can't pick up %s.", iname );
        }
      }
    }

    /* Turn tick (skip when shop prompt just opened — browsing is free) */
    if ( !ShopUIActive() && !EnemiesTurning() )
    {
      PoisonPoolTick( frame_pr, frame_pc );
      GameEventsNewTurn();
      player.turns_since_hit++;
      for ( int i = 0; i < num_enemies; i++ )
        if ( enemies[i].alive ) enemies[i].turns_since_hit++;

      if ( EnemiesInCombat( enemies, num_enemies ) )
      {
        turn.enemy_delay = 0.2f;
      }
      else
      {
        EnemiesStartTurn( enemies, num_enemies, frame_pr, frame_pc, TileWalkable );
      }

      NPCsIdleTick( npcs, num_npcs );
    }
  }
  turn.was_moving = PlayerIsMoving();

  /* Enemy turn delay countdown */
  if ( turn.enemy_delay > 0 )
  {
    turn.enemy_delay -= dt;
    if ( turn.enemy_delay <= 0 && !EnemyProjectileInFlight()
         && !SpellVFXActive() )
    {
      turn.enemy_delay = 0;
      EnemiesStartTurn( enemies, num_enemies, frame_pr, frame_pc, TileWalkable );
    }
  }
}

static int handle_target_mode( void )
{
  int tr, tc, ci, si;
  if ( TargetModeConfirmed( &tr, &tc, &ci, &si ) )
  {
    if ( GameEventResolveTarget( ci, si, tr, tc, enemies, num_enemies ) )
    {
      turn.enemy_delay = 0.2f;
      player.turns_since_hit++;
      for ( int i = 0; i < num_enemies; i++ )
        if ( enemies[i].alive ) enemies[i].turns_since_hit++;
    }
  }

  if ( TargetModeLogic( enemies, num_enemies ) )
  {
    hover_row = hover_col = -1;
    return 1;
  }
  return 0;
}

static int handle_look_mode( void )
{
  if ( LookModeLogic( mouse_moved ) )
  {
    hover_row = hover_col = -1;
    return 1;
  }

  if ( app.keyboard[SDL_SCANCODE_L] == 1 )
  {
    app.keyboard[SDL_SCANCODE_L] = 0;
    LookModeEnter( frame_pr, frame_pc );
  }
  return 0;
}

static void handle_movement( void )
{
  if ( PlayerIsMoving() || EnemiesTurning() || InventoryUIFocused()
       || turn.enemy_delay > 0 )
    return;

  int dr = 0, dc = 0;
  if ( app.keyboard[SDL_SCANCODE_UP] == 1 || app.keyboard[SDL_SCANCODE_W] == 1 )
  { app.keyboard[SDL_SCANCODE_UP] = 0; app.keyboard[SDL_SCANCODE_W] = 0; dc = -1; }
  else if ( app.keyboard[SDL_SCANCODE_DOWN] == 1 || app.keyboard[SDL_SCANCODE_S] == 1 )
  { app.keyboard[SDL_SCANCODE_DOWN] = 0; app.keyboard[SDL_SCANCODE_S] = 0; dc =  1; }
  else if ( app.keyboard[SDL_SCANCODE_LEFT] == 1 || app.keyboard[SDL_SCANCODE_A] == 1 )
  { app.keyboard[SDL_SCANCODE_LEFT] = 0; app.keyboard[SDL_SCANCODE_A] = 0; dr = -1; }
  else if ( app.keyboard[SDL_SCANCODE_RIGHT] == 1 || app.keyboard[SDL_SCANCODE_D] == 1 )
  { app.keyboard[SDL_SCANCODE_RIGHT] = 0; app.keyboard[SDL_SCANCODE_D] = 0; dr =  1; }

  if ( dr == 0 && dc == 0 ) return;

  int tr = frame_pr + dr;
  int tc = frame_pc + dc;

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

static void handle_mouse( void )
{
  aContainerWidget_t* vp = a_GetContainerFromWidget( "game_viewport" );
  int mx = app.mouse.x;
  int my = app.mouse.y;
  hover_row = -1;
  hover_col = -1;

  if ( !PointInRect( mx, my, vp->rect.x, vp->rect.y, vp->rect.w, vp->rect.h ) )
    return;

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

  if ( app.mouse.pressed && hover_row >= 0
       && !PlayerIsMoving() && !EnemiesTurning()
       && turn.enemy_delay <= 0 )
  {
    if ( RapidMoveActive()
         && TileAdjacent( hover_row, hover_col )
         && TileWalkable( hover_row, hover_col )
         && !EnemyAt( enemies, num_enemies, hover_row, hover_col )
         && !NPCAt( npcs, num_npcs, hover_row, hover_col ) )
    {
      app.mouse.pressed = 0;
      PlayerStartMove( hover_row, hover_col );
    }
    else
    {
      app.mouse.pressed = 0;
      int on_self = ( hover_row == frame_pr && hover_col == frame_pc );
      TileActionsOpen( hover_row, hover_col, on_self );
    }
  }
}

static void handle_zoom( void )
{
  if ( app.mouse.wheel == 0 ) return;

  aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
  if ( PointInRect( app.mouse.x, app.mouse.y,
                    cp->rect.x, cp->rect.y, cp->rect.w, cp->rect.h ) )
  {
    ConsoleScroll( &console, app.mouse.wheel );
    app.mouse.wheel = 0;
  }
  if ( app.mouse.wheel != 0 )
  {
    GV_Zoom( &camera, app.mouse.wheel, ZOOM_MIN_H, ZOOM_MAX_H );
    app.mouse.wheel = 0;
  }
}

/* ===== Main logic loop ===== */

static void gs_Logic( float dt )
{
  frame_begin( dt );

  /* Check HUD pause button click from previous draw frame */
  if ( hud_pause_clicked )
  {
    hud_pause_clicked = 0;
    if ( !PauseMenuActive() ) PauseMenuOpen();
  }

  if ( handle_intro( dt ) )         return;

  if ( PauseMenuActive() )
  {
    int r = PauseMenuLogic();
    if ( r == 2 ) { a_WidgetCacheFree(); MainMenuInit(); return; }
    frame_end();
    return;
  }

  if ( handle_overlays() )          { frame_end(); return; }
  if ( handle_esc_key() )           return;

  handle_inventory();
  update_systems( dt );
  handle_turn_end( dt );
  if ( handle_target_mode() )       { frame_end(); return; }
  if ( handle_look_mode() )         { frame_end(); return; }

  handle_movement();
  handle_mouse();
  handle_zoom();

  frame_end();
}

static void gs_Draw( float dt )
{
  (void)dt;

  /* Top bar */
  if ( HUDDrawTopBar( EnemiesInCombat( enemies, num_enemies ) ) )
    hud_pause_clicked = 1;

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

  /* Console panel (dialogue/shop UI drawn later, on top of viewport) */
  if ( !DialogueActive() && !ShopUIActive() )
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
    draw_cam.x += CombatShakeOX() + SpellVFXShakeOX();
    draw_cam.y += CombatShakeOY() + SpellVFXShakeOY();

    if ( world && tileset )
      GV_DrawWorld( vp_rect, &draw_cam, world, tileset, settings.gfx_mode == GFX_ASCII );

    /* Draw dungeon props (easel etc.) before darkness */
    DungeonDrawProps( vp_rect, &draw_cam, world, settings.gfx_mode );

    /* Draw shop rug + items on rug */
    ShopDrawRug( vp_rect, &draw_cam, world, settings.gfx_mode );
    ShopDrawItems( vp_rect, &draw_cam, world, settings.gfx_mode );

    /* Draw poison pools */
    PoisonPoolDrawAll( vp_rect, &draw_cam, world, settings.gfx_mode );

    /* Draw placed traps */
    PlacedTrapsDrawAll( vp_rect, &draw_cam, world, settings.gfx_mode );

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

    /* Target mode range + cursor */
    TargetModeDraw( vp_rect, &draw_cam, world );

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

    /* Spell VFX (projectiles, zap lines, tile flashes) */
    SpellVFXDraw( vp_rect, &draw_cam );

    /* Red flash overlay on hit (fires once, won't restart while active) */
    float flash = CombatFlashAlpha();
    if ( flash > 0.5f )
      a_DrawFilledRect( clip, (aColor_t){ 0xa5, 0x30, 0x30, (int)flash } );

    /* Spell colored screen flash */
    {
      float sf = SpellVFXFlashAlpha();
      if ( sf > 0.5f )
      {
        aColor_t sfc = SpellVFXFlashColor();
        sfc.a = (int)sf;
        a_DrawFilledRect( clip, sfc );
      }
    }

    /* Quest tracker — top-left of viewport */
    QuestTrackerDraw( vp_rect );

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
  else if ( ShopUIActive() )
  {
    aContainerWidget_t* cp = a_GetContainerFromWidget( "console_panel" );
    aRectf_t cr = cp->rect;
    cr.y += TransitionGetConsoleOY();
    ShopUIDraw( cr );
  }

  /* Tile action menu (drawn outside clip rect, over everything) */
  TileActionsDraw( enemies, num_enemies );

  if ( TransitionShowLabels() )
    a_DrawWidgets();

  InventoryUISetIntroOffset( TransitionGetRightOX(), TransitionGetUIAlpha() );
  InventoryUIDraw();

  /* Pause menu — drawn on top of everything */
  PauseMenuDraw();
}
