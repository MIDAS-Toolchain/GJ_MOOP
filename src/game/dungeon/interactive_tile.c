#include <string.h>
#include <Archimedes.h>

#include "interactive_tile.h"
#include "player.h"
#include "enemies.h"

extern Player_t player;

static ITile_t itiles[MAX_ITILES];
static int     num_itiles = 0;
static aSoundEffect_t sfx_web_hit;

static const struct {
  int         tile_id;
  const char* glyph;
  aColor_t    color;
  const char* description;
  int         solid;
} itile_types[] = {
  [ITILE_RAT_HOLE] = {
    10, "O", { 0x09, 0x0a, 0x14, 255 },
    "A dark hole gnawed through the wall. Something is scratching inside.", 1
  },
  [ITILE_HIDDEN_WALL] = {
    1, "#", { 0x81, 0x97, 0x96, 255 },
    "You see a stone wall.", 1
  },
  [ITILE_SPIDER_WEB] = {
    9, "~", { 0x9a, 0x8c, 0x7a, 255 },
    "Thick spider silk stretches across the floor. Stepping in it would slow you down.", 0
  },
  [ITILE_OLD_CRATE] = {
    18, "=", { 0xa0, 0x78, 0x46, 255 },
    "An old wooden crate. Looks like it's been here a while.", 1
  },
  [ITILE_URN] = {
    19, "U", { 0x8a, 0x5c, 0x3e, 255 },
    "A clay urn. The cult placed these with care.", 1
  },
  [ITILE_VOID_PORTAL] = {
    20, "V", { 0x80, 0x20, 0xa0, 255 },
    "A shimmering tear in the air. Something writhes inside.", 1
  },
};

void ITileInit( void )
{
  memset( itiles, 0, sizeof( itiles ) );
  num_itiles = 0;
  a_AudioLoadSound( "resources/soundeffects/web_hit.wav", &sfx_web_hit );
}

void ITilePlace( World_t* world, int x, int y, int type )
{
  if ( num_itiles >= MAX_ITILES ) return;

  int idx = y * world->width + x;

  world->midground[idx].tile     = itile_types[type].tile_id;
  world->midground[idx].glyph    = (char*)itile_types[type].glyph;
  world->midground[idx].glyph_fg = itile_types[type].color;
  world->midground[idx].solid    = itile_types[type].solid;

  itiles[num_itiles].row         = x;
  itiles[num_itiles].col         = y;
  itiles[num_itiles].type        = type;
  itiles[num_itiles].active      = 1;
  itiles[num_itiles].horror_type = -1;
  num_itiles++;
}

ITile_t* ITileAt( int row, int col )
{
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].row == row && itiles[i].col == col )
      return &itiles[i];
  }
  return NULL;
}

void ITileBreak( World_t* world, int row, int col )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t ) return;

  int idx = t->col * world->width + t->row;

  world->midground[idx].tile     = 0;
  world->midground[idx].glyph    = ".";
  world->midground[idx].glyph_fg = (aColor_t){ 0x39, 0x4a, 0x50, 255 };
  world->midground[idx].solid    = 0;

  t->active = 0;
}

const char* ITileDescription( int type )
{
  return itile_types[type].description;
}

void ITileReveal( World_t* world, int row, int col )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_HIDDEN_WALL || t->revealed ) return;

  t->revealed = 1;

  int idx = t->col * world->width + t->row;
  world->midground[idx].glyph    = "#";
  world->midground[idx].glyph_fg = (aColor_t){ 0xc0, 0x94, 0x73, 255 };
  world->background[idx].solid   = 0;
}

int ITileIsRevealedHiddenWall( int row, int col )
{
  ITile_t* t = ITileAt( row, col );
  return ( t && t->type == ITILE_HIDDEN_WALL && t->revealed );
}

int ITileIsHiddenWall( int row, int col )
{
  ITile_t* t = ITileAt( row, col );
  return ( t && t->type == ITILE_HIDDEN_WALL );
}

void ITileOpenHiddenWall( World_t* world, int row, int col )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_HIDDEN_WALL ) return;

  int idx = t->col * world->width + t->row;

  world->midground[idx].tile     = TILE_EMPTY;
  world->midground[idx].glyph    = "";
  world->midground[idx].glyph_fg = (aColor_t){ 0, 0, 0, 0 };
  world->midground[idx].solid    = 0;

  world->background[idx].tile     = 0;
  world->background[idx].glyph    = ".";
  world->background[idx].glyph_fg = (aColor_t){ 0x39, 0x4a, 0x50, 255 };
  world->background[idx].solid    = 0;

  t->active = 0;
}

int ITileWebCheck( World_t* world, int row, int col, int* out_gold )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_SPIDER_WEB || t->cooldown > 0 ) return 0;

  player.root_turns = WEB_ROOT_TURNS;
  a_AudioPlaySound( &sfx_web_hit, NULL );

  *out_gold = 0;
  if ( t->gold > 0 )
  {
    *out_gold = t->gold;
    t->gold = 0;
  }

  /* Destroy the web after all effects are applied */
  ITileBreak( world, row, col );
  return 1;
}

void ITileTick( void )
{
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].cooldown > 0 )
      itiles[i].cooldown--;
  }
}

void ITileWebScatterGold( void )
{
  /* Collect indices of all spider web itiles */
  int webs[MAX_ITILES], nw = 0;
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].type == ITILE_SPIDER_WEB )
      webs[nw++] = i;
  }
  if ( nw < 4 ) return;

  /* Shuffle web indices (Fisher-Yates) */
  for ( int i = nw - 1; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tmp = webs[i]; webs[i] = webs[j]; webs[j] = tmp;
  }

  /* First 3 get 1 gold, 4th gets 2 gold */
  itiles[webs[0]].gold = 1;
  itiles[webs[1]].gold = 1;
  itiles[webs[2]].gold = 1;
  itiles[webs[3]].gold = 2;
}

int ITileCrateCheck( World_t* world, int row, int col, int* out_gold )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_OLD_CRATE ) return 0;

  a_AudioPlaySound( &sfx_web_hit, NULL );
  *out_gold = t->gold;
  t->gold = 0;
  ITileBreak( world, row, col );
  return 1;
}

void ITileCrateScatterGold( void )
{
  int crates[MAX_ITILES], nc = 0;
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].type == ITILE_OLD_CRATE )
      crates[nc++] = i;
  }
  if ( nc < 4 ) return;

  /* Shuffle (Fisher-Yates) */
  for ( int i = nc - 1; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tmp = crates[i]; crates[i] = crates[j]; crates[j] = tmp;
  }

  /* 3 get 1 gold, 1 gets 2 gold, rest empty */
  itiles[crates[0]].gold = 1;
  itiles[crates[1]].gold = 1;
  itiles[crates[2]].gold = 1;
  itiles[crates[3]].gold = 2;
}

int ITileUrnCheck( World_t* world, int row, int col, int* out_gold )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_URN ) return 0;

  a_AudioPlaySound( &sfx_web_hit, NULL );
  *out_gold = t->gold;
  t->gold = 0;
  ITileBreak( world, row, col );
  return 1;
}

void ITileUrnScatterGold( void )
{
  int urns[MAX_ITILES], nu = 0;
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].type == ITILE_URN )
      urns[nu++] = i;
  }
  if ( nu < 4 ) return;

  /* Shuffle (Fisher-Yates) */
  for ( int i = nu - 1; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tmp = urns[i]; urns[i] = urns[j]; urns[j] = tmp;
  }

  /* 3 get 1 gold, 1 gets 2 gold, rest empty */
  itiles[urns[0]].gold = 1;
  itiles[urns[1]].gold = 1;
  itiles[urns[2]].gold = 1;
  itiles[urns[3]].gold = 2;
}

int ITileVoidPortalCheck( World_t* world, int row, int col,
                           int* out_gold, int* out_horror )
{
  ITile_t* t = ITileAt( row, col );
  if ( !t || t->type != ITILE_VOID_PORTAL ) return 0;

  a_AudioPlaySound( &sfx_web_hit, NULL );
  *out_gold   = t->gold;
  *out_horror = t->horror_type;
  t->gold = 0;
  t->horror_type = -1;
  ITileBreak( world, row, col );
  return 1;
}

void ITileVoidPortalScatter( void )
{
  int portals[MAX_ITILES], np = 0;
  for ( int i = 0; i < num_itiles; i++ )
  {
    if ( itiles[i].active && itiles[i].type == ITILE_VOID_PORTAL )
      portals[np++] = i;
  }
  if ( np < 8 ) return;

  /* Shuffle (Fisher-Yates) */
  for ( int i = np - 1; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tmp = portals[i]; portals[i] = portals[j]; portals[j] = tmp;
  }

  /* Gold: 3 give 1 gold, 1 gives 2 gold */
  itiles[portals[0]].gold = 1;
  itiles[portals[1]].gold = 1;
  itiles[portals[2]].gold = 1;
  itiles[portals[3]].gold = 2;

  /* Horror spawns: 4 baby, 2 normal, 1 lost, 1 elder */
  int baby  = EnemyTypeByKey( "baby_horror" );
  int norm  = EnemyTypeByKey( "horror" );
  int lost  = EnemyTypeByKey( "lost_horror" );
  int elder = EnemyTypeByKey( "elder_horror" );

  int types[8] = { baby, baby, baby, baby, norm, norm, lost, elder };

  /* Shuffle the horror assignments */
  for ( int i = 7; i > 0; i-- )
  {
    int j = rand() % ( i + 1 );
    int tmp = types[i]; types[i] = types[j]; types[j] = tmp;
  }

  for ( int i = 0; i < 8; i++ )
    itiles[portals[i]].horror_type = types[i];
}
