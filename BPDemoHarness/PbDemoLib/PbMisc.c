/*
 * PbMisc.c - Contains misc functions and defines for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbMisc.h"

///////////////////////////////////////////////////////////////////////////////
// const char* PbMiscGetAsBits
///////////////////////////////////////////////////////////////////////////////

void PbMiscGetAsBits( char* pDest, unsigned int value )
{
  int i = 0;
  int string_pos = 0;
  unsigned int shift_reg = 1<<31;  

  for( i = 0; i < 36; i++ )
    pDest[i] = 0;

  for( i = 0; i < 32; i++ )
  {
    if( ( value & shift_reg ) == shift_reg )
      pDest[string_pos] = '1';
    else
      pDest[string_pos] = '0';

    if( !((i+1) % 8) )
    {
      string_pos++;
      pDest[string_pos] = ' ';
    }
    
    string_pos++;
    shift_reg >>= 1;
  }

  pDest[string_pos+1] = 0;
}

///////////////////////////////////////////////////////////////////////////////
// const char* PbMiscGetAsBits
///////////////////////////////////////////////////////////////////////////////

void PbMiscPrintReg( const char* pName, u64 value, u64 reg )
{
  char temp[38];
  char temp2[38];

  PbMiscGetAsBits( temp, value );
  PbMiscGetAsBits( temp2, value >> 32 );

//  printf( "%s VALUE: %s| %s\n",pName, temp, temp2 );

  PbMiscGetAsBits( temp, reg );
  PbMiscGetAsBits( temp2, reg >> 32 );

//  printf( "%s REGTR: %s| %s\n",pName, temp, temp2 );
}

