/*
 * PbMisc.h - Contains misc functions and defines for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBMISC_H_
#define _PBMISC_H_

#include <tamtypes.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#define _DEBUG_

void PbMiscSetOutput( int (*pFunction)(const char *,...) );

///////////////////////////////////////////////////////////////////////////////
// Functions (mostly debug stuff)
///////////////////////////////////////////////////////////////////////////////

void PbMiscGetAsBits( char* pDest, unsigned int value );
void PbMiscPrintReg( const char* pName, u64 value, u64 reg );

extern int (*out)(const char *,...);

#define PB_LINE out( "LINE: %d\n", __LINE__ );

#endif // _PBMISC_H_

