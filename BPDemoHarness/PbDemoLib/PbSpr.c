/*
 * PbSpr.c - ScratchpadRam functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbSpr.h"

static void* gp_Spr = (void*)SPR_START;

///////////////////////////////////////////////////////////////////////////////
// void* PbSpr_Alloc
// This function is used so we can easy get SPR memory that isnt in
// use, if one for example was DMAong something from the SPR and
// one tried to changed the data while dma is obtaining it funky
// stuff may happen
///////////////////////////////////////////////////////////////////////////////

void* PbSprAlloc( int Size )
{
  void* p_spr = gp_Spr;

  if( gp_Spr+Size >= ( ((void*)0x70000000)+16*1024 ) )
    gp_Spr = (void*)SPR_START;

  p_spr = gp_Spr;
  gp_Spr += Size;

  return p_spr;
}
