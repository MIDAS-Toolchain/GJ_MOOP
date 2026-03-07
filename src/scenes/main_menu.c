#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Archimedes.h>

#include "defines.h"
#include "draw_utils.h"
#include "game_viewport.h"
#include "class_select.h"
#include "lore_scene.h"
#include "settings.h"
#include "sound_manager.h"

static void mm_Logic( float );
static void mm_Draw( float );

#define NUM_BUTTONS   4
#define BTN_H         42.0f
#define BTN_SPACING   14.0f

enum { BTN_PLAY, BTN_LORE, BTN_SETTINGS, BTN_QUIT };

static const char* btn_labels[NUM_BUTTONS] = { "Play", "Lore", "Settings", "Quit" };

static int cursor = 0;
static int hovered[NUM_BUTTONS] = { 0 };

static aSoundEffect_t sfx_hover;
static aSoundEffect_t sfx_click;

/* ---- background dungeon ---- */
#define MM_MAP_W  100
#define MM_MAP_H  75
#define MM_TILE   16

static uint8_t       mm_solid[MM_MAP_W * MM_MAP_H];
static float         mm_vis[MM_MAP_W * MM_MAP_H];
static aTileset_t*   mm_tileset  = NULL;
static int           mm_loaded   = 0;
static GameCamera_t  mm_cam;
static float         mm_bounds_x0, mm_bounds_y0;  /* content top-left in world px */
static float         mm_bounds_x1, mm_bounds_y1;  /* content bottom-right in world px */

#define MM_VIS_RADIUS 6

/* ---- wandering sprites ---- */
#define SPRITE_COUNT  12

static const char* sprite_paths[] = {
  "resources/assets/enemies/skeleton.png",
  "resources/assets/enemies/rat.png",
  "resources/assets/enemies/acid_slime.png",
  "resources/assets/enemies/cultist.png",
  "resources/assets/enemies/lost_horror.png",
  "resources/assets/enemies/goblin_grunt.png",
  "resources/assets/enemies/goblin_shaman.png",
  "resources/assets/enemies/goblin_slinger.png",
  "resources/assets/enemies/undead_miner.png",
  "resources/assets/enemies/phantom_thief.png",
  "resources/assets/enemies/void_slime.png",
  "resources/assets/enemies/horror.png",
  "resources/assets/enemies/elder_horror.png",
  "resources/assets/npcs/jonathon.png",
  "resources/assets/npcs/laura.png",
  "resources/assets/npcs/graf.png",
  "resources/assets/npcs/greta.png",
  "resources/assets/npcs/muri.png",
  "resources/assets/npcs/drem.png",
  "resources/assets/enemies/baby_horror.png",
  "resources/assets/enemies/red_slime.png",
  "resources/assets/enemies/cave_spider.png",
  "resources/assets/enemies/fallen_knight.png",
  "resources/assets/enemies/rat_king.png",
  "resources/assets/enemies/goblin_jailer.png",
  NULL
};

typedef struct {
  aImage_t* img;
  float x, y;          /* world position (pixels) */
  int   dir;           /* 0=up 1=right 2=down 3=left */
  float move_timer;    /* time until next step */
  int   flipped;
  int   is_enemy;      /* enemies face right in sprite, need default flip */
} MMSprite_t;

static aImage_t*  sprite_images[64];
static int        sprite_is_enemy[64];
static int        sprite_image_count = 0;
static MMSprite_t sprites[SPRITE_COUNT];

static float randf( float lo, float hi )
{
  return lo + ( (float)rand() / (float)RAND_MAX ) * ( hi - lo );
}

/* Bresenham LOS through mm_solid (same approach as visibility.c) */
static int mm_los( int x0, int y0, int x1, int y1 )
{
  int ddx = abs( x1 - x0 );
  int ddy = abs( y1 - y0 );
  int ssx = ( x0 < x1 ) ? 1 : -1;
  int ssy = ( y0 < y1 ) ? 1 : -1;
  int err = ddx - ddy;
  int cx = x0, cy = y0;

  while ( cx != x1 || cy != y1 )
  {
    int e2 = 2 * err;
    if ( e2 > -ddy ) { err -= ddy; cx += ssx; }
    if ( e2 <  ddx ) { err += ddx; cy += ssy; }
    if ( cx == x1 && cy == y1 ) break;
    if ( cx < 0 || cx >= MM_MAP_W || cy < 0 || cy >= MM_MAP_H ) return 0;
    if ( mm_solid[cy * MM_MAP_W + cx] ) return 0;
  }
  return 1;
}

static void mm_update_vis( void )
{
  memset( mm_vis, 0, sizeof( mm_vis ) );
  if ( sprite_image_count == 0 ) return;

  /* Pass 1: light floor tiles near each sprite via LOS */
  for ( int s = 0; s < SPRITE_COUNT; s++ )
  {
    int pr = (int)( sprites[s].x / MM_TILE );
    int pc = (int)( sprites[s].y / MM_TILE );

    for ( int ty = pc - MM_VIS_RADIUS; ty <= pc + MM_VIS_RADIUS; ty++ )
    {
      for ( int tx = pr - MM_VIS_RADIUS; tx <= pr + MM_VIS_RADIUS; tx++ )
      {
        if ( tx < 0 || tx >= MM_MAP_W || ty < 0 || ty >= MM_MAP_H ) continue;

        int adx = abs( tx - pr );
        int ady = abs( ty - pc );
        int dist = ( adx > ady ) ? adx : ady;
        if ( dist > MM_VIS_RADIUS ) continue;

        int idx = ty * MM_MAP_W + tx;
        if ( mm_solid[idx] ) continue;

        if ( dist <= 1 || mm_los( pr, pc, tx, ty ) )
        {
          float v = 1.0f - ( (float)dist / ( MM_VIS_RADIUS + 1.0f ) );
          if ( v > mm_vis[idx] ) mm_vis[idx] = v;
        }
      }
    }
  }

  /* Pass 2: walls inherit brightness from lit floor neighbors */
  static const int ox[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
  static const int oy[] = { 0, 0, -1, 1, -1, 1, -1, 1 };

  for ( int s = 0; s < SPRITE_COUNT; s++ )
  {
    int pr = (int)( sprites[s].x / MM_TILE );
    int pc = (int)( sprites[s].y / MM_TILE );

    for ( int ty = pc - MM_VIS_RADIUS; ty <= pc + MM_VIS_RADIUS; ty++ )
    {
      for ( int tx = pr - MM_VIS_RADIUS; tx <= pr + MM_VIS_RADIUS; tx++ )
      {
        if ( tx < 0 || tx >= MM_MAP_W || ty < 0 || ty >= MM_MAP_H ) continue;
        int idx = ty * MM_MAP_W + tx;
        if ( mm_vis[idx] > 0 ) continue;
        if ( !mm_solid[idx] ) continue;

        float best = 0;
        for ( int d = 0; d < 8; d++ )
        {
          int nx = tx + ox[d];
          int ny = ty + oy[d];
          if ( nx < 0 || nx >= MM_MAP_W || ny < 0 || ny >= MM_MAP_H ) continue;
          int nidx = ny * MM_MAP_W + nx;
          if ( mm_solid[nidx] ) continue;
          if ( mm_vis[nidx] > best ) best = mm_vis[nidx];
        }
        if ( best > 0 ) mm_vis[idx] = best;
      }
    }
  }
}

static int mm_is_walkable( int tx, int ty )
{
  if ( tx < 0 || tx >= MM_MAP_W || ty < 0 || ty >= MM_MAP_H ) return 0;
  return !mm_solid[ty * MM_MAP_W + tx];
}

static void mm_random_floor( int* ox, int* oy )
{
  for ( int tries = 0; tries < 500; tries++ )
  {
    int tx = rand() % MM_MAP_W;
    int ty = rand() % MM_MAP_H;
    if ( mm_is_walkable( tx, ty ) )
    {
      *ox = tx;
      *oy = ty;
      return;
    }
  }
  *ox = MM_MAP_W / 2;
  *oy = MM_MAP_H / 2;
}

static void sprite_spawn( MMSprite_t* s )
{
  s->img = sprite_images[ rand() % sprite_image_count ];
  int tx, ty;
  mm_random_floor( &tx, &ty );
  s->x = tx * MM_TILE + MM_TILE / 2.0f;
  s->y = ty * MM_TILE + MM_TILE / 2.0f;
  s->dir = rand() % 4;
  s->move_timer = randf( 0.3f, 0.8f );
  s->flipped = 0;
}

static const int dx[] = { 0, 1, 0, -1 };
static const int dy[] = { -1, 0, 1, 0 };

static void sprite_move( MMSprite_t* s, int d, int nx, int ny )
{
  s->x = nx * MM_TILE + MM_TILE / 2.0f;
  s->y = ny * MM_TILE + MM_TILE / 2.0f;
  s->dir = d;
  /* Enemies face left in source art; NPCs face right */
  if ( s->is_enemy )
  {
    if ( d == 1 )      s->flipped = 1;  /* moving right = flip */
    else if ( d == 3 ) s->flipped = 0;  /* moving left = natural */
  }
  else
  {
    if ( d == 1 )      s->flipped = 0;  /* moving right = natural */
    else if ( d == 3 ) s->flipped = 1;  /* moving left = flip */
  }
}

static void sprite_step( MMSprite_t* s )
{
  int tx = (int)( s->x / MM_TILE );
  int ty = (int)( s->y / MM_TILE );

  int fwd = s->dir;
  int back = ( fwd + 2 ) % 4;

  /* Can we keep going forward? */
  int fwd_ok = mm_is_walkable( tx + dx[fwd], ty + dy[fwd] );

  /* Count non-backtrack options (branching corridors) */
  int sides[4];
  int side_count = 0;
  for ( int d = 0; d < 4; d++ )
  {
    if ( d == fwd || d == back ) continue;
    if ( mm_is_walkable( tx + dx[d], ty + dy[d] ) )
      sides[side_count++] = d;
  }

  /* At an intersection: 70% keep forward, 30% take a branch */
  if ( fwd_ok && side_count > 0 )
  {
    if ( rand() % 100 < 70 )
    {
      sprite_move( s, fwd, tx + dx[fwd], ty + dy[fwd] );
      return;
    }
    int d = sides[ rand() % side_count ];
    sprite_move( s, d, tx + dx[d], ty + dy[d] );
    return;
  }

  /* Corridor: keep going forward */
  if ( fwd_ok )
  {
    sprite_move( s, fwd, tx + dx[fwd], ty + dy[fwd] );
    return;
  }

  /* Dead end or corner: try sides first, then backtrack */
  if ( side_count > 0 )
  {
    int d = sides[ rand() % side_count ];
    sprite_move( s, d, tx + dx[d], ty + dy[d] );
    return;
  }

  /* Last resort: turn around */
  if ( mm_is_walkable( tx + dx[back], ty + dy[back] ) )
    sprite_move( s, back, tx + dx[back], ty + dy[back] );
}

static void mm_load_bg( void )
{
  if ( mm_loaded ) return;
  mm_loaded = 1;

  mm_tileset = a_TilesetCreate( "resources/assets/tiles/level01tilemap.png", 16, 16 );

  memset( mm_solid, 1, sizeof( mm_solid ) );

  FILE* fp = fopen( "resources/data/floors/floor_01.map", "r" );
  if ( !fp ) return;

  char buf[256];
  int y = 0;
  while ( y < MM_MAP_H && fgets( buf, sizeof( buf ), fp ) )
  {
    size_t len = strlen( buf );
    while ( len > 0 && ( buf[len-1] == '\n' || buf[len-1] == '\r' ) )
      buf[--len] = '\0';
    if ( len >= 2 && buf[0] == '/' && buf[1] == '/' )
      continue;

    for ( int x = 0; x < MM_MAP_W && x < (int)len; x++ )
    {
      char c = buf[x];
      if ( c == '#' || c == 'S' || c == '?' )
        mm_solid[y * MM_MAP_W + x] = 1;
      else
        mm_solid[y * MM_MAP_W + x] = 0;
    }
    y++;
  }
  fclose( fp );

  /* Find the bounding box of actual walkable content */
  int min_x = MM_MAP_W, max_x = 0, min_y = MM_MAP_H, max_y = 0;
  for ( int cy = 0; cy < MM_MAP_H; cy++ )
    for ( int cx = 0; cx < MM_MAP_W; cx++ )
      if ( !mm_solid[cy * MM_MAP_W + cx] )
      {
        if ( cx < min_x ) min_x = cx;
        if ( cx > max_x ) max_x = cx;
        if ( cy < min_y ) min_y = cy;
        if ( cy > max_y ) max_y = cy;
      }

  mm_bounds_x0 = min_x * MM_TILE;
  mm_bounds_y0 = min_y * MM_TILE;
  mm_bounds_x1 = ( max_x + 1 ) * MM_TILE;
  mm_bounds_y1 = ( max_y + 1 ) * MM_TILE;

  mm_cam.x      = ( mm_bounds_x0 + mm_bounds_x1 ) / 2.0f;
  mm_cam.y      = ( mm_bounds_y0 + mm_bounds_y1 ) / 2.0f;
  mm_cam.half_h = 120.0f;

  sprite_image_count = 0;
  for ( int i = 0; sprite_paths[i] != NULL; i++ )
  {
    aImage_t* img = a_ImageLoad( sprite_paths[i] );
    if ( img )
    {
      sprite_is_enemy[sprite_image_count] = ( strstr( sprite_paths[i], "enemies/" ) != NULL );
      sprite_images[sprite_image_count++] = img;
    }
  }

  if ( sprite_image_count > 0 )
  {
    /* Shuffle a copy of the image indices so each sprite gets a unique look */
    int indices[64];
    for ( int i = 0; i < sprite_image_count; i++ )
      indices[i] = i;
    for ( int i = sprite_image_count - 1; i > 0; i-- )
    {
      int j = rand() % ( i + 1 );
      int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
    }

    for ( int i = 0; i < SPRITE_COUNT; i++ )
    {
      sprite_spawn( &sprites[i] );
      int idx = indices[i % sprite_image_count];
      sprites[i].img = sprite_images[idx];
      sprites[i].is_enemy = sprite_is_enemy[idx];
      sprites[i].flipped = sprite_is_enemy[idx] ? 0 : 1;  /* enemies face left, NPCs face right */
    }
  }
}

static void mm_draw_bg( void )
{
  if ( !mm_tileset ) return;

  aRectf_t vp = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

  float sx, sy, cl, ct;
  {
    float aspect = vp.w / vp.h;
    float cam_w  = mm_cam.half_h * aspect;
    sx = vp.w / ( cam_w * 2.0f );
    sy = vp.h / ( mm_cam.half_h * 2.0f );
    cl = mm_cam.x - cam_w;
    ct = mm_cam.y - mm_cam.half_h;
  }

  for ( int i = 0; i < MM_MAP_W * MM_MAP_H; i++ )
  {
    int x = i % MM_MAP_W;
    int y = i / MM_MAP_W;

    float wx = x * MM_TILE;
    float wy = y * MM_TILE;

    float px = ( wx - cl ) * sx + vp.x;
    float py = ( wy - ct ) * sy + vp.y;
    float pw = MM_TILE * sx;
    float ph = MM_TILE * sy;

    /* Cull off-screen tiles */
    if ( px + pw < 0 || px > SCREEN_WIDTH || py + ph < 0 || py > SCREEN_HEIGHT )
      continue;

    float nx = (int)px;
    float ny = (int)py;
    float nw = (int)( px + pw + 0.5f ) - (int)px;
    float nh = (int)( py + ph + 0.5f ) - (int)py;

    int tile_idx = mm_solid[i] ? 1 : 0;
    aRectf_t dst = { nx, ny, nw, nh };
    a_BlitRect( mm_tileset[tile_idx].img, NULL, &dst, 1.0f );
  }

  /* Per-tile darkness using LOS visibility (like the game does) */
  for ( int i = 0; i < MM_MAP_W * MM_MAP_H; i++ )
  {
    int x = i % MM_MAP_W;
    int y = i / MM_MAP_W;

    float px = ( (float)( x * MM_TILE ) - cl ) * sx + vp.x;
    float py = ( (float)( y * MM_TILE ) - ct ) * sy + vp.y;
    float pw = MM_TILE * sx;
    float ph = MM_TILE * sy;

    if ( px + pw < 0 || px > SCREEN_WIDTH || py + ph < 0 || py > SCREEN_HEIGHT )
      continue;

    int alpha = (int)( ( 1.0f - mm_vis[i] ) * 255 );
    if ( alpha < 1 ) continue;
    if ( alpha > 255 ) alpha = 255;

    float nx = (int)px;
    float ny = (int)py;
    float nw = (int)( px + pw + 0.5f ) - (int)px;
    float nh = (int)( py + ph + 0.5f ) - (int)py;

    a_DrawFilledRect( (aRectf_t){ nx, ny, nw, nh },
                      (aColor_t){ 0, 0, 0, alpha } );
  }

  /* Sprites walking through corridors */
  if ( sprite_image_count == 0 ) return;

  for ( int i = 0; i < SPRITE_COUNT; i++ )
  {
    MMSprite_t* s = &sprites[i];
    float dw = MM_TILE;
    float dh = MM_TILE;

    float dpx = ( s->x - dw / 2.0f - cl ) * sx + vp.x;
    float dpy = ( s->y - dh / 2.0f - ct ) * sy + vp.y;
    float dpw = dw * sx;

    float scale = dpw / s->img->rect.w;
    if ( s->flipped )
      a_BlitRectFlipped( s->img, NULL,
                         &(aRectf_t){ dpx, dpy, s->img->rect.w, s->img->rect.h },
                         scale, 'x' );
    else
      a_BlitRect( s->img, NULL,
                  &(aRectf_t){ dpx, dpy, s->img->rect.w, s->img->rect.h },
                  scale );
  }
}

static void mm_update_sprites( float dt )
{
  if ( sprite_image_count == 0 ) return;

  for ( int i = 0; i < SPRITE_COUNT; i++ )
  {
    MMSprite_t* s = &sprites[i];
    s->move_timer -= dt;
    if ( s->move_timer <= 0 )
    {
      sprite_step( s );
      s->move_timer = randf( 0.3f, 0.8f );
    }
  }

  mm_update_vis();
}

/* Slow camera pan across the dungeon */
static float cam_dir_x = 1.0f;
static float cam_dir_y = 0.6f;
#define CAM_PAN_SPEED 8.0f

static void mm_update_cam( float dt )
{
  float aspect = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
  float cam_hw = mm_cam.half_h * aspect;

  mm_cam.x += cam_dir_x * CAM_PAN_SPEED * dt;
  mm_cam.y += cam_dir_y * CAM_PAN_SPEED * dt;

  if ( mm_cam.x - cam_hw < mm_bounds_x0 )    { mm_cam.x = mm_bounds_x0 + cam_hw;          cam_dir_x =  1.0f; }
  if ( mm_cam.x + cam_hw > mm_bounds_x1 )    { mm_cam.x = mm_bounds_x1 - cam_hw;          cam_dir_x = -1.0f; }
  if ( mm_cam.y - mm_cam.half_h < mm_bounds_y0 ) { mm_cam.y = mm_bounds_y0 + mm_cam.half_h; cam_dir_y =  0.6f; }
  if ( mm_cam.y + mm_cam.half_h > mm_bounds_y1 ) { mm_cam.y = mm_bounds_y1 - mm_cam.half_h; cam_dir_y = -0.6f; }
}

void MainMenuInit( void )
{
  app.delegate.logic = mm_Logic;
  app.delegate.draw  = mm_Draw;

  app.options.scale_factor = 1;

  cursor = 0;
  for ( int i = 0; i < NUM_BUTTONS; i++ )
    hovered[i] = 0;

  a_AudioLoadSound( "resources/soundeffects/menu_move.wav", &sfx_hover );
  a_AudioLoadSound( "resources/soundeffects/menu_click.wav", &sfx_click );

  a_WidgetsInit( "resources/widgets/main_menu.auf" );
  app.active_widget = a_GetWidget( "mm_buttons" );

  mm_load_bg();

  SoundManagerPlayMenu();
}

static void mm_Execute( int index )
{
  a_AudioPlaySound( &sfx_click, NULL );

  switch ( index )
  {
    case BTN_PLAY:
      a_WidgetCacheFree();
      ClassSelectInit();
      break;
    case BTN_LORE:
      a_WidgetCacheFree();
      LoreSceneInit();
      break;
    case BTN_SETTINGS:
      a_WidgetCacheFree();
      SettingsInit();
      break;
    case BTN_QUIT:
      app.running = 0;
      break;
  }
}

static void mm_Logic( float dt )
{
  a_DoInput();

  mm_update_cam( dt );
  mm_update_sprites( dt );

  /* ESC - quit */
  if ( app.keyboard[SDL_SCANCODE_ESCAPE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
    app.running = 0;
    return;
  }

  /* Hot reload AUF */
  if ( app.keyboard[A_R] == 1 )
  {
    app.keyboard[A_R] = 0;
    a_WidgetsInit( "resources/widgets/main_menu.auf" );
  }

  /* Keyboard nav */
  if ( app.keyboard[A_W] == 1 || app.keyboard[A_UP] == 1 )
  {
    app.keyboard[A_W] = 0;
    app.keyboard[A_UP] = 0;
    cursor = ( cursor - 1 + NUM_BUTTONS ) % NUM_BUTTONS;
    a_AudioPlaySound( &sfx_hover, NULL );
  }

  if ( app.keyboard[A_S] == 1 || app.keyboard[A_DOWN] == 1 )
  {
    app.keyboard[A_S] = 0;
    app.keyboard[A_DOWN] = 0;
    cursor = ( cursor + 1 ) % NUM_BUTTONS;
    a_AudioPlaySound( &sfx_hover, NULL );
  }

  if ( app.keyboard[SDL_SCANCODE_RETURN] == 1 || app.keyboard[SDL_SCANCODE_SPACE] == 1 )
  {
    app.keyboard[SDL_SCANCODE_RETURN] = 0;
    app.keyboard[SDL_SCANCODE_SPACE] = 0;
    mm_Execute( cursor );
    return;
  }

  /* Mouse - hit test each button rect */
  aContainerWidget_t* bc = a_GetContainerFromWidget( "mm_buttons" );
  aRectf_t r = bc->rect;
  float btn_w = r.w;
  float total_h = NUM_BUTTONS * BTN_H + ( NUM_BUTTONS - 1 ) * BTN_SPACING;
  float by = r.y + ( r.h - total_h ) / 2.0f;

  for ( int i = 0; i < NUM_BUTTONS; i++ )
  {
    float bx = r.x;
    float byi = by + i * ( BTN_H + BTN_SPACING );

    int hit = PointInRect( app.mouse.x, app.mouse.y, bx, byi, btn_w, BTN_H );

    if ( hit && !hovered[i] )
    {
      cursor = i;
      a_AudioPlaySound( &sfx_hover, NULL );
    }
    hovered[i] = hit;

    if ( hit && app.mouse.pressed && app.mouse.button == SDL_BUTTON_LEFT )
    {
      mm_Execute( i );
      return;
    }
  }
}

static void mm_Draw( float dt )
{
  /* Dungeon background with wandering sprites */
  mm_draw_bg();

  /* Title — "Open Door Dungeon" with colored door icons inline */
  {
    aContainerWidget_t* tc = a_GetContainerFromWidget( "mm_title" );
    aRectf_t tr = tc->rect;

    aTextStyle_t ts = a_default_text_style;
    ts.fg = (aColor_t){ 0xde, 0x9e, 0x41, 255 };
    ts.bg = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 2.5f;
    ts.align = TEXT_ALIGN_LEFT;

    aTextStyle_t shadow = ts;
    shadow.fg = (aColor_t){ 0x10, 0x10, 0x10, 180 };
    int sd = 3;

    float s = ts.scale;
    float th;

    float w_open, w_door, w_dung;
    a_CalcTextDimensions( "Open",    ts.type, &w_open, &th ); w_open *= s;
    a_CalcTextDimensions( "Door",    ts.type, &w_door, &th ); w_door *= s;
    a_CalcTextDimensions( "Dungeon", ts.type, &w_dung, &th ); w_dung *= s;
    th *= s;

    float icon_sz = th;
    float gap = icon_sz * 0.3f;
    float total_w = w_open + w_door + w_dung + icon_sz * 4 + gap * 7;

    float cx = tr.x + tr.w / 2 - total_w / 2;
    int   cy = (int)( tr.y + tr.h / 2 );
    float icon_y = cy + th * 0.15f - 6;

    /* red door + "Open" */
    aRectf_t dst = { cx, icon_y, icon_sz, icon_sz };
    a_BlitRect( mm_tileset[4].img, NULL, &dst, 1.0f );
    cx += icon_sz + gap;
    a_DrawText( "Open", (int)cx + sd, cy + sd, shadow );
    a_DrawText( "Open", (int)cx, cy, ts );
    cx += w_open + gap;

    /* green door + "Door" */
    dst = (aRectf_t){ cx, icon_y, icon_sz, icon_sz };
    a_BlitRect( mm_tileset[3].img, NULL, &dst, 1.0f );
    cx += icon_sz + gap;
    a_DrawText( "Door", (int)cx + sd, cy + sd, shadow );
    a_DrawText( "Door", (int)cx, cy, ts );
    cx += w_door + gap;

    /* blue door + "Dungeon" */
    dst = (aRectf_t){ cx, icon_y, icon_sz, icon_sz };
    a_BlitRect( mm_tileset[2].img, NULL, &dst, 1.0f );
    cx += icon_sz + gap;
    a_DrawText( "Dungeon", (int)cx + sd, cy + sd, shadow );
    a_DrawText( "Dungeon", (int)cx, cy, ts );
    cx += w_dung + gap;

    /* white door at the end */
    dst = (aRectf_t){ cx, icon_y, icon_sz, icon_sz };
    a_BlitRect( mm_tileset[5].img, NULL, &dst, 1.0f );
  }

  /* Buttons */
  {
    aContainerWidget_t* bc = a_GetContainerFromWidget( "mm_buttons" );
    aRectf_t r = bc->rect;
    float btn_w = r.w;
    float total_h = NUM_BUTTONS * BTN_H + ( NUM_BUTTONS - 1 ) * BTN_SPACING;
    float by = r.y + ( r.h - total_h ) / 2.0f;

    aColor_t bg_norm  = { 0x10, 0x14, 0x1f, 255 };
    aColor_t bg_hover = { 0x20, 0x2e, 0x37, 255 };
    aColor_t fg_norm  = { 0x81, 0x97, 0x96, 255 };
    aColor_t fg_hover = { 0xc7, 0xcf, 0xcc, 255 };

    for ( int i = 0; i < NUM_BUTTONS; i++ )
    {
      float bx = r.x;
      float byi = by + i * ( BTN_H + BTN_SPACING );
      int sel = ( cursor == i );

      DrawButton( bx, byi, btn_w, BTN_H, btn_labels[i], 1.5f, sel,
                  bg_norm, bg_hover, fg_norm, fg_hover );
    }
  }

  /* Version hint */
  {
    aContainerWidget_t* vc = a_GetContainerFromWidget( "mm_version" );
    aRectf_t vr = vc->rect;

    aTextStyle_t ts = a_default_text_style;
    ts.fg = (aColor_t){ 0x81, 0x97, 0x96, 120 };
    ts.bg = (aColor_t){ 0, 0, 0, 0 };
    ts.scale = 1.0f;
    ts.align = TEXT_ALIGN_CENTER;
    a_DrawText( "v0.2", (int)( vr.x + vr.w / 2.0f ),
                (int)( vr.y + vr.h / 2.0f ), ts );
  }
}
