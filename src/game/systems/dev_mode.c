#include <Archimedes.h>
#include <string.h>

#include "dev_mode.h"
#include "dialogue.h"
#include "player.h"
#include "items.h"
#include "movement.h"
#include "visibility.h"
#include "context_menu.h"
#include "draw_utils.h"

extern Player_t player;

static int         dev_active = 0;
static int         dev_noclip = 0;
static Console_t*  dev_console = NULL;
static NPC_t*      dev_npcs = NULL;
static int*        dev_num_npcs = NULL;

#define DEV_COLOR (aColor_t){ 0x39, 0xff, 0x14, 255 }

/* ---- Teleport NPC selector ---- */
static int         tp_open   = 0;
static int         tp_cursor = 0;
static const char* tp_labels[MAX_NPCS];
static int         tp_npc_map[MAX_NPCS]; /* label index → dev_npcs[] index */
static int         tp_count  = 0;

/* ---- Gear-up selector ---- */
static int         gear_open   = 0;
static int         gear_cursor = 0;
#define GEAR_FLOORS 2
static const char* gear_labels[GEAR_FLOORS] = { "Floor 01 Gear", "Floor 02 Gear" };

/* Returns which row the mouse hovers over (-1 if none) */
static int dev_menu_mouse_row( const char** labels, int count )
{
  float max_tw = 0;
  for ( int i = 0; i < count; i++ )
  {
    float tw, th;
    a_CalcTextDimensions( labels[i], a_default_text_style.type, &tw, &th );
    tw *= CTX_MENU_TEXT_S;
    if ( tw > max_tw ) max_tw = tw;
  }
  float menu_w = max_tw + 24;
  if ( menu_w < CTX_MENU_W ) menu_w = CTX_MENU_W;

  float row_step = CTX_MENU_ROW_H + CTX_MENU_PAD;
  float menu_h   = count * row_step - CTX_MENU_PAD;
  float mx = ( SCREEN_WIDTH  - menu_w ) / 2.0f;
  float my = ( SCREEN_HEIGHT - menu_h ) / 2.0f;

  for ( int i = 0; i < count; i++ )
  {
    float ry = my + i * row_step;
    if ( PointInRect( app.mouse.x, app.mouse.y,
                      mx, ry, menu_w, CTX_MENU_ROW_H ) )
      return i;
  }
  return -1;
}

void DevModeInit( Console_t* console )
{
  dev_console = console;
  dev_active  = 0;
}

void DevModeSetNPCs( NPC_t* list, int* count )
{
  dev_npcs     = list;
  dev_num_npcs = count;
}

int DevModeActive( void )
{
  return dev_active;
}

int DevModeNoclip( void )
{
  return dev_noclip;
}

int DevModeInput( void )
{
  /* F1 toggles dev mode */
  if ( app.keyboard[SDL_SCANCODE_F1] == 1 )
  {
    app.keyboard[SDL_SCANCODE_F1] = 0;
    dev_active = !dev_active;
    if ( dev_active )
    {
      ConsolePush( dev_console, "DEV MODE ON", DEV_COLOR );
      ConsolePush( dev_console, "T - teleport to NPC", DEV_COLOR );
      ConsolePush( dev_console, "G - gear up (select floor)", DEV_COLOR );
      ConsolePush( dev_console, "H - heal to full", DEV_COLOR );
      ConsolePush( dev_console, "L - show coordinates", DEV_COLOR );
      ConsolePush( dev_console, "N - toggle noclip", DEV_COLOR );
    }
    else
    {
      ConsolePush( dev_console, "DEV MODE OFF", DEV_COLOR );
    }
    return 1;
  }

  if ( !dev_active ) return 0;

  /* Teleport menu active - handle navigation */
  if ( tp_open )
  {
    if ( app.keyboard[SDL_SCANCODE_W] == 1 || app.keyboard[SDL_SCANCODE_UP] == 1 )
    {
      app.keyboard[SDL_SCANCODE_W] = 0;
      app.keyboard[SDL_SCANCODE_UP] = 0;
      tp_cursor = ( tp_cursor - 1 + tp_count ) % tp_count;
    }
    if ( app.keyboard[SDL_SCANCODE_S] == 1 || app.keyboard[SDL_SCANCODE_DOWN] == 1 )
    {
      app.keyboard[SDL_SCANCODE_S] = 0;
      app.keyboard[SDL_SCANCODE_DOWN] = 0;
      tp_cursor = ( tp_cursor + 1 ) % tp_count;
    }

    /* Mouse hover + click */
    int tp_hover = dev_menu_mouse_row( tp_labels, tp_count );
    if ( tp_hover >= 0 ) tp_cursor = tp_hover;

    int tp_confirm = ( app.keyboard[SDL_SCANCODE_RETURN] == 1
                       || app.keyboard[SDL_SCANCODE_SPACE] == 1
                       || ( tp_hover >= 0 && app.mouse.pressed
                            && app.mouse.button == SDL_BUTTON_LEFT ) );

    /* Select */
    if ( tp_confirm )
    {
      app.keyboard[SDL_SCANCODE_RETURN] = 0;
      app.keyboard[SDL_SCANCODE_SPACE]  = 0;
      app.mouse.pressed = 0;

      NPC_t* n = &dev_npcs[tp_npc_map[tp_cursor]];
      NPCType_t* nt = &g_npc_types[n->type_idx];

      /* Try adjacent tiles: below, above, right, left */
      static const int dr[] = { 0, 0, 1, -1 };
      static const int dc[] = { 1, -1, 0, 0 };
      int dest_r = n->row, dest_c = n->col;

      for ( int d = 0; d < 4; d++ )
      {
        int tr = n->row + dr[d];
        int tc = n->col + dc[d];
        if ( TileWalkable( tr, tc ) )
        {
          dest_r = tr;
          dest_c = tc;
          break;
        }
      }

      PlayerSetWorldPos( dest_r * 16 + 8.0f, dest_c * 16 + 8.0f );

      int pr, pc;
      PlayerGetTile( &pr, &pc );
      VisibilityUpdate( pr, pc );

      ConsolePushF( dev_console, DEV_COLOR, "TP: %s",
                    d_StringPeek( nt->name ) );
      tp_open = 0;
    }

    /* Cancel */
    if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
    {
      app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
      tp_open = 0;
    }

    /* Consume all movement keys while menu is open */
    app.keyboard[SDL_SCANCODE_W] = 0;
    app.keyboard[SDL_SCANCODE_A] = 0;
    app.keyboard[SDL_SCANCODE_S] = 0;
    app.keyboard[SDL_SCANCODE_D] = 0;
    app.keyboard[SDL_SCANCODE_UP] = 0;
    app.keyboard[SDL_SCANCODE_DOWN] = 0;
    app.keyboard[SDL_SCANCODE_LEFT] = 0;
    app.keyboard[SDL_SCANCODE_RIGHT] = 0;
    return 1;
  }

  /* Gear menu active - handle navigation */
  if ( gear_open )
  {
    if ( app.keyboard[SDL_SCANCODE_W] == 1 || app.keyboard[SDL_SCANCODE_UP] == 1 )
    {
      app.keyboard[SDL_SCANCODE_W] = 0;
      app.keyboard[SDL_SCANCODE_UP] = 0;
      gear_cursor = ( gear_cursor - 1 + GEAR_FLOORS ) % GEAR_FLOORS;
    }
    if ( app.keyboard[SDL_SCANCODE_S] == 1 || app.keyboard[SDL_SCANCODE_DOWN] == 1 )
    {
      app.keyboard[SDL_SCANCODE_S] = 0;
      app.keyboard[SDL_SCANCODE_DOWN] = 0;
      gear_cursor = ( gear_cursor + 1 ) % GEAR_FLOORS;
    }

    /* Mouse hover + click */
    int gear_hover = dev_menu_mouse_row( gear_labels, GEAR_FLOORS );
    if ( gear_hover >= 0 ) gear_cursor = gear_hover;

    int gear_confirm = ( app.keyboard[SDL_SCANCODE_RETURN] == 1
                         || app.keyboard[SDL_SCANCODE_SPACE] == 1
                         || ( gear_hover >= 0 && app.mouse.pressed
                              && app.mouse.button == SDL_BUTTON_LEFT ) );

    /* Select */
    if ( gear_confirm )
    {
      app.keyboard[SDL_SCANCODE_RETURN] = 0;
      app.keyboard[SDL_SCANCODE_SPACE]  = 0;
      app.mouse.pressed = 0;

      const char* cls = PlayerClassKey();
      const char* weapon_key  = NULL;
      const char* armor_key   = NULL;
      const char* trinket_key = NULL;

      if ( gear_cursor == 0 ) /* Floor 01 */
      {
        if ( strcmp( cls, "mercenary" ) == 0 )
        { weapon_key = "heavy_mace"; armor_key = "chainmail"; trinket_key = "blood_medal"; }
        else if ( strcmp( cls, "rogue" ) == 0 )
        { weapon_key = "serrated_blade"; armor_key = "shadow_vest"; trinket_key = "viper_fang"; }
        else if ( strcmp( cls, "mage" ) == 0 )
        { weapon_key = "arcane_rod"; armor_key = "warded_robes"; trinket_key = "surge_stone"; }
      }
      else /* Floor 02 */
      {
        if ( strcmp( cls, "mercenary" ) == 0 )
        { weapon_key = "war_hammer"; armor_key = "iron_platemail"; trinket_key = "crimson_ring"; }
        else if ( strcmp( cls, "rogue" ) == 0 )
        { weapon_key = "blacksteel_knife"; armor_key = "ghost_leather"; trinket_key = "phantom_charm"; }
        else if ( strcmp( cls, "mage" ) == 0 )
        { weapon_key = "conduit_staff"; armor_key = "runecloth_vestments"; trinket_key = "prism_pendant"; }
      }

      if ( weapon_key )
      {
        int wi = EquipmentByKey( weapon_key );
        if ( wi >= 0 ) PlayerEquip( EQUIP_WEAPON, wi );
        int ai = EquipmentByKey( armor_key );
        if ( ai >= 0 ) PlayerEquip( EQUIP_ARMOR, ai );
        int ti = EquipmentByKey( trinket_key );
        if ( ti >= 0 )
        {
          if ( gear_cursor == 0 )
            PlayerEquip( EQUIP_TRINKET2, ti ); /* floor 01: 2nd trinket slot */
          else
            PlayerEquip( EQUIP_TRINKET1, ti ); /* floor 02: replace starter */
        }
        PlayerRecalcStats();
        ConsolePushF( dev_console, DEV_COLOR, "GEAR UP: %s equipped",
                      gear_labels[gear_cursor] );
      }

      gear_open = 0;
    }

    /* Cancel */
    if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
    {
      app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
      gear_open = 0;
    }

    app.keyboard[SDL_SCANCODE_W] = 0;
    app.keyboard[SDL_SCANCODE_A] = 0;
    app.keyboard[SDL_SCANCODE_S] = 0;
    app.keyboard[SDL_SCANCODE_D] = 0;
    app.keyboard[SDL_SCANCODE_UP] = 0;
    app.keyboard[SDL_SCANCODE_DOWN] = 0;
    app.keyboard[SDL_SCANCODE_LEFT] = 0;
    app.keyboard[SDL_SCANCODE_RIGHT] = 0;
    return 1;
  }

  /* T - open teleport NPC selector */
  if ( app.keyboard[SDL_SCANCODE_T] == 1 )
  {
    app.keyboard[SDL_SCANCODE_T] = 0;

    tp_count  = 0;
    tp_cursor = 0;
    for ( int i = 0; i < *dev_num_npcs; i++ )
    {
      if ( !dev_npcs[i].alive ) continue;
      NPCType_t* nt = &g_npc_types[dev_npcs[i].type_idx];
      tp_labels[tp_count]  = d_StringPeek( nt->name );
      tp_npc_map[tp_count] = i;
      tp_count++;
    }

    if ( tp_count > 0 ) tp_open = 1;
    return 1;
  }

  /* G - open gear-up selector */
  if ( app.keyboard[SDL_SCANCODE_G] == 1 )
  {
    app.keyboard[SDL_SCANCODE_G] = 0;
    gear_cursor = 0;
    gear_open   = 1;
    return 1;
  }

  /* H - heal to full */
  if ( app.keyboard[SDL_SCANCODE_H] == 1 )
  {
    app.keyboard[SDL_SCANCODE_H] = 0;
    PlayerHeal( player.max_hp );
    ConsolePush( dev_console, "HEALED TO FULL", DEV_COLOR );
    return 1;
  }

  /* L - show current coordinates */
  if ( app.keyboard[SDL_SCANCODE_L] == 1 )
  {
    app.keyboard[SDL_SCANCODE_L] = 0;
    int pr, pc;
    PlayerGetTile( &pr, &pc );
    ConsolePushF( dev_console, DEV_COLOR, "POS: (%d, %d)", pr, pc );
    return 1;
  }

  /* N - toggle noclip */
  if ( app.keyboard[SDL_SCANCODE_N] == 1 )
  {
    app.keyboard[SDL_SCANCODE_N] = 0;
    dev_noclip = !dev_noclip;
    ConsolePushF( dev_console, DEV_COLOR, "NOCLIP %s", dev_noclip ? "ON" : "OFF" );
    return 1;
  }

  return 0;
}

/* Shared helper to draw a centered selection menu */
static void draw_dev_menu( const char** labels, int count, int cursor )
{
  float max_tw = 0;
  for ( int i = 0; i < count; i++ )
  {
    float tw, th;
    a_CalcTextDimensions( labels[i], a_default_text_style.type, &tw, &th );
    tw *= CTX_MENU_TEXT_S;
    if ( tw > max_tw ) max_tw = tw;
  }

  float menu_w = max_tw + 24;
  if ( menu_w < CTX_MENU_W ) menu_w = CTX_MENU_W;

  float menu_h = count * ( CTX_MENU_ROW_H + CTX_MENU_PAD ) - CTX_MENU_PAD;
  float mx = ( SCREEN_WIDTH  - menu_w ) / 2.0f;
  float my = ( SCREEN_HEIGHT - menu_h ) / 2.0f;

  aColor_t bg     = { 0x09, 0x0a, 0x14, 255 };
  aColor_t border = { 0x39, 0x4a, 0x50, 255 };
  aColor_t sel_bg = { 0x15, 0x1d, 0x28, 255 };
  aColor_t sel_fg = { 0xde, 0x9e, 0x41, 255 };
  aColor_t nor_fg = { 0xc7, 0xcf, 0xcc, 255 };

  a_DrawFilledRect( (aRectf_t){ mx, my, menu_w, menu_h }, bg );
  a_DrawRect( (aRectf_t){ mx, my, menu_w, menu_h }, border );

  aTextStyle_t ts = a_default_text_style;
  ts.bg    = (aColor_t){ 0, 0, 0, 0 };
  ts.scale = CTX_MENU_TEXT_S;
  ts.align = TEXT_ALIGN_LEFT;

  for ( int i = 0; i < count; i++ )
  {
    float ry = my + i * ( CTX_MENU_ROW_H + CTX_MENU_PAD );
    aRectf_t row = { mx + 2, ry, menu_w - 4, CTX_MENU_ROW_H };

    if ( i == cursor )
    {
      a_DrawFilledRect( row, sel_bg );
      a_DrawRect( row, sel_fg );
      ts.fg = sel_fg;
    }
    else
    {
      ts.fg = nor_fg;
    }

    a_DrawText( labels[i], (int)( row.x + 8 ), (int)( ry + 5 ), ts );
  }
}

void DevModeDraw( void )
{
  if ( tp_open && tp_count > 0 )
    draw_dev_menu( tp_labels, tp_count, tp_cursor );

  if ( gear_open )
    draw_dev_menu( gear_labels, GEAR_FLOORS, gear_cursor );
}
