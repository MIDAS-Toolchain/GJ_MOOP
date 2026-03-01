#ifndef __DIALOGUE_H__
#define __DIALOGUE_H__

#include <Archimedes.h>

#define MAX_NPC_TYPES      16
#define MAX_DIALOGUE_NODES 512
#define MAX_NODE_OPTIONS    8
#define MAX_CONDITIONS      4
#define MAX_FLAGS          128

/* A single dialogue entry — speech node OR player option */
typedef struct
{
  char key[MAX_NAME_LENGTH];

  /* Speech node fields (NPC talks) */
  char text[512];
  char option_keys[MAX_NODE_OPTIONS][MAX_NAME_LENGTH];
  int  num_options;

  /* Option fields (player chooses) */
  char label[256];
  char goto_key[MAX_NAME_LENGTH];
  aColor_t label_color;       /* manual color override (alpha 0 = not set) */

  /* Start node */
  int  is_start;
  int  priority;

  /* Conditions (multiple require_flag / require_not_flag supported) */
  char require_class[MAX_NAME_LENGTH];
  char require_flag[MAX_CONDITIONS][MAX_NAME_LENGTH];
  int  num_require_flag;
  char require_flag_min[MAX_NAME_LENGTH];
  char require_not_flag[MAX_CONDITIONS][MAX_NAME_LENGTH];
  int  num_require_not_flag;
  char require_item[MAX_NAME_LENGTH];
  char require_lore[MAX_CONDITIONS][MAX_NAME_LENGTH];
  int  num_require_lore;

  /* Conditions (gold) */
  int  require_gold_min;

  /* Actions */
  char set_flag[MAX_NAME_LENGTH];
  char incr_flag[MAX_NAME_LENGTH];
  char clear_flag[MAX_NAME_LENGTH];
  char give_item[MAX_NAME_LENGTH];
  char take_item[MAX_NAME_LENGTH];
  char set_lore[MAX_NAME_LENGTH];
  int  give_gold;
} DialogueEntry_t;

/* NPC type — loaded from one DUF file */
typedef struct
{
  char     key[MAX_NAME_LENGTH];       /* filename stem: "guard" */
  char     name[MAX_NAME_LENGTH];
  char     glyph[8];
  aColor_t color;
  char     description[256];
  char     combat_bark[128];
  char     idle_bark[128];
  int      idle_cooldown;           /* turns between idle barks (0 = disabled) */
  aImage_t* image;
  DialogueEntry_t entries[MAX_DIALOGUE_NODES];
  int      num_entries;
} NPCType_t;

extern NPCType_t g_npc_types[MAX_NPC_TYPES];
extern int        g_num_npc_types;

/* Flag system — global key-value table on the player */
int  FlagGet( const char* name );
void FlagSet( const char* name, int value );
void FlagIncr( const char* name );
void FlagClear( const char* name );
void FlagsInit( void );

/* Loading */
void DialogueLoadAll( void );
int  NPCTypeByKey( const char* key );

/* Runtime */
void DialogueStart( int npc_type_idx );
void DialogueSelectOption( int index );
void DialogueEnd( void );
int  DialogueActive( void );

/* Current dialogue state (for UI) */
const char* DialogueGetNPCName( void );
aColor_t    DialogueGetNPCColor( void );
const char* DialogueGetText( void );
int         DialogueGetOptionCount( void );
const char* DialogueGetOptionLabel( int index );
aColor_t    DialogueGetOptionColor( int index );
aImage_t*   DialogueGetNPCImage( void );
const char* DialogueGetNPCGlyph( void );

#endif
