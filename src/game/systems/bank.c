#include <stdlib.h>
#include <string.h>
#include <Daedalus.h>

#include "bank.h"
#include "persist.h"
#include "player.h"
#include "console.h"
#include "dialogue.h"

extern Player_t player;

static Console_t* console = NULL;
static int vault_gold = 0;

#define GOLD_COLOR  (aColor_t){ 0xe8, 0xc1, 0x70, 255 }
#define BANK_INCREMENT 10

static void bank_save( void )
{
  char buf[32];
  snprintf( buf, sizeof( buf ), "%d", vault_gold );
  PersistSave( "vault", buf );
}

static void bank_update_flag( void )
{
  if ( vault_gold > 0 ) FlagSet( "has_vault_gold", 1 );
  else                  FlagClear( "has_vault_gold" );
}

void BankInit( Console_t* con )
{
  console = con;
  vault_gold = 0;

  char* s = PersistLoad( "vault" );
  if ( s )
  {
    vault_gold = atoi( s );
    free( s );
  }
  bank_update_flag();
}

void BankDeposit( int amt )
{
  if ( player.gold < amt )
  {
    ConsolePushF( console, GOLD_COLOR, "You need at least %d gold to deposit.", amt );
    return;
  }

  PlayerSpendGold( amt );
  vault_gold += amt;
  bank_save();
  bank_update_flag();

  ConsolePushF( console, GOLD_COLOR, "Deposited %d gold. Vault: %d", amt, vault_gold );
}

void BankWithdraw( int amt )
{
  if ( vault_gold <= 0 )
  {
    ConsolePushF( console, GOLD_COLOR, "The vault is empty." );
    return;
  }

  if ( amt > vault_gold ) amt = vault_gold;
  vault_gold -= amt;
  PlayerAddGold( amt );
  bank_save();
  bank_update_flag();

  ConsolePushF( console, GOLD_COLOR, "Withdrew %d gold. Vault: %d", amt, vault_gold );
}

void BankCheck( void )
{
  dString_t* s = d_StringInit();
  if ( vault_gold <= 0 )
    d_StringSet( s, "Your vault is empty. Nothing to count. This is the worst part of my job." );
  else
    d_StringFormat( s, "You have %d gold in the vault. I counted it four times.", vault_gold );
  DialogueOverrideText( d_StringPeek( s ) );
  d_StringDestroy( s );
}

void BankGreeting( void )
{
  dString_t* s = d_StringInit();
  if ( vault_gold <= 0 )
    d_StringSet( s, "Welcome to the Goblin Vault. Your vault is empty. Would you like to change that?" );
  else
    d_StringFormat( s, "Welcome back. You have %d gold in the vault. I counted it four times while you were gone.", vault_gold );
  DialogueOverrideText( d_StringPeek( s ) );
  d_StringDestroy( s );
}
