/*
 * PbVram.c - Vram handling functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbVram.h"

static u32 g_CurrentPointer = (-PB_BLOCKSIZE)&(0+PB_BLOCKSIZE-1);

///////////////////////////////////////////////////////////////////////////////
// u32 PbVramAllocPage
///////////////////////////////////////////////////////////////////////////////

u32 PbVramAlloc( int Size )
{
	Size=(-PB_BLOCKSIZE)&(Size+PB_BLOCKSIZE-1);
	g_CurrentPointer+=Size;

#ifdef _DEBUG_
  printf( "Alloced vram at: 0x%x\n", g_CurrentPointer-Size );
#endif

	return g_CurrentPointer-Size;
}

///////////////////////////////////////////////////////////////////////////////
// u32 GetVramPointer
///////////////////////////////////////////////////////////////////////////////

u32 GetVramPointer()
{
    return g_CurrentPointer;
}

///////////////////////////////////////////////////////////////////////////////
// u32 SetVramPointer
///////////////////////////////////////////////////////////////////////////////

void SetVramPointer(u32 pval)
{
    g_CurrentPointer = pval;
}

