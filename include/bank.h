#ifndef __BANK_H__
#define __BANK_H__

#include "console.h"

void BankInit( Console_t* con );
void BankDeposit( int amt );
void BankWithdraw( int amt );
void BankCheck( void );
void BankGreeting( void );

#endif
