#include "PbGlobal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////
// void out( const char* pString, ... )
///////////////////////////////////////////////////////////////////////////////

void out( const char* pString, ... )
{
	char buffer1[1024];
	va_list v;

	va_start(v,pString);
	vsprintf(buffer1,pString,v);
	va_end(v);

	gp_Info->printf( buffer1 );
}

///////////////////////////////////////////////////////////////////////////////
// const char* PbGlobal_GetAsBits32
///////////////////////////////////////////////////////////////////////////////

const char* PbGlobal_GetAsBits32( unsigned int value )
{
  int i = 0;
	static char name[128];
  
  int string_pos = 0;
  unsigned int shift_reg = 1<<31;

  name[0] = 0;

  for( i = 0; i < 32; i++ )
  {
    if( (shift_reg & value) == shift_reg )
      name[string_pos] = '1';
    else
      name[string_pos] = '0';
  
    if( !((i+1) % 8) )
    {
      string_pos++;
      name[string_pos] = ' ';
    }
    
    string_pos++;
    shift_reg >>= 1;
  }

  name[string_pos+1] = 0;
  return (const char*)&name;
}

///////////////////////////////////////////////////////////////////////////////
// const char* PbGlobal_GetAsBits32
///////////////////////////////////////////////////////////////////////////////

int PbGlobal_Log( int Value )
{
  int r = 0;
  
  Value--;

  while( Value > 0 )
  {
    Value = Value >> 1;
    r++;
  }
  
  return r;
}

