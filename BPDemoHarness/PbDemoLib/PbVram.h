/*
 * PbVram.h - Vram handling functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBVRAM_H_
#define _PBVRAM_H_

#include <tamtypes.h>
#include "PbMisc.h"

#define PB_BLOCKSIZE 8192
#define PB_BLOCKSIZEW 64
#define PB_BLOCKSIZEH 32

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

u32 PbVramAlloc( int Size );
u32 GetVramPointer();
void SetVramPointer(u32 pval);

#endif // _PBVRAM_H_

