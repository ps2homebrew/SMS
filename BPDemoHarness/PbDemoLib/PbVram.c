/*
 * PbVram.c - Vram handling functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbVram.h"

static u32 g_CurrentPointer;

///////////////////////////////////////////////////////////////////////////////
// u32 PbVramAllocPage
///////////////////////////////////////////////////////////////////////////////

u32 PbVramAlloc( int Size )
{
  u32 new_p;

  new_p = g_CurrentPointer;
  g_CurrentPointer += Size;

  if( g_CurrentPointer & 0x1FFF )
  {
    g_CurrentPointer = (g_CurrentPointer & 0xFFFFE000) + 0x2000;
  }

  return new_p;
}

