/*
 * PbSpr.h - ScratchpadRam functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBSPR_H_
#define _PBSPR_H_

///////////////////////////////////////////////////////////////////////////////
// Defines
// Notice that the spr start adress can be changed (to for example + 8*1024)
// if the app wants to control the first 8kb by itself, else the lib will
// use the whole SPR memory itself
///////////////////////////////////////////////////////////////////////////////

#define SPR_START 0x70000000

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void* PbSprAlloc( int Size );

#endif // _PBSPR_H_

