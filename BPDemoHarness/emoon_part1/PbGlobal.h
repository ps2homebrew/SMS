#ifndef _PBGLOBAL_H_
#define _PBGLOBAL_H_

/////////////////////////////////////////////////////////////////////
// misc stuff from harness
/////////////////////////////////////////////////////////////////////

#include <tamtypes.h> 	
#include "../harness.h" // dependent on tamtypes
extern const demo_init_t* gp_Info; // nice

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/////////////////////////////////////////////////////////////////////
// output stuff, made with intention of being fast to type 
/////////////////////////////////////////////////////////////////////

void out( const char* pString, ... );
int  PbGlobal_Log( int Value );
const char* PbGlobal_GetAsBits32( unsigned int value );

#define line out( "LINE: %d\n", __LINE__ );

#endif //_PBGLOBAL_H_

