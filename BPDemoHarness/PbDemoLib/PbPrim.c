/*
 * PbPrim.c - Primitive functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <tamtypes.h>
#include "PbPrim.h"
#include "PbGs.h"
#include "PbSpr.h"
#include "PbScreen.h"
#include "PbDma.h"

///////////////////////////////////////////////////////////////////////////////
// void PbPrimSprite
///////////////////////////////////////////////////////////////////////////////

void PbPrimSprite( int x1, int y1, int x2, int y2, int z, int color )
{
  u64* p_store;
  u64* p_data;

  x1 += 2048 << 4;
  y1 += 2048 << 4;
  x2 += 2048 << 4;
  y2 += 2048 << 4;

  p_store = p_data = PbSprAlloc( 5*16 );
  
  /////////////////////////////////////////////////////////////////////////////
  // Setup for drawing

  *p_data++ = GIF_TAG( 4, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0) ;
  *p_data++ = GS_PRIM;

  *p_data++ = color;
  *p_data++ = GS_RGBAQ;

  *p_data++ = GS_SETREG_XYZ2( x1, y1, z );
  *p_data++ = GS_XYZ2;

  *p_data++ = GS_SETREG_XYZ2( x2, y2, z );
  *p_data++ = GS_XYZ2;
  
  PbDmaSend02Spr( p_store, 5 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbPrimSprite
///////////////////////////////////////////////////////////////////////////////

void PbPrimSpriteNoZtest( int x1, int y1, int x2, int y2, int z, int color )
{
  u64* p_store;
  u64* p_data;

  x1 += 2048 << 4;
  y1 += 2048 << 4;
  x2 += 2048 << 4;
  y2 += 2048 << 4;

  p_store = p_data = PbSprAlloc( 7*16 );
  
  /////////////////////////////////////////////////////////////////////////////
  // Setup for drawing

  *p_data++ = GIF_TAG( 6, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0) ;
  *p_data++ = GS_PRIM;

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
  *p_data++ = GS_TEST_1,     

  *p_data++ = color;
  *p_data++ = GS_RGBAQ;

  *p_data++ = GS_SETREG_XYZ2( x1, y1, z );
  *p_data++ = GS_XYZ2;

  *p_data++ = GS_SETREG_XYZ2( x2, y2, z );
  *p_data++ = GS_XYZ2;

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 2 );     
  *p_data++ = GS_TEST_1,     
  
  PbDmaSend02Spr( p_store, 7 );
}

