/*
 * main.c - Sample of a simple vu1 drawing program.
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <tamtypes.h>
#include <kernel.h>
#include "../../harness.h"
#include "PbScreen.h"
#include "PbTexture.h"
#include "PbDma.h"
#include "PbPrim.h"

static u32 texture_test_32[256*256] __attribute__((aligned(16)));

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

u32 start_demo( const demo_init_t* pInfo )
{
  PbTexture* p_texture = NULL;
  int x = 0;
  int y = 0;

  /////////////////////////////////////////////////////////////////////////////
  // Setup screen

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  /////////////////////////////////////////////////////////////////////////////
  // Create a nice looking texture

  for( y = 0; y < 256; y++ )
    for( x = 0; x < 256; x++ )
      texture_test_32[(y*256)+x] = ((x^y)<<16)|((x^y)<<8)|(x^y);

  FlushCache(0);

  p_texture = PbTextureCreate32( texture_test_32, 256, 256 ); 

  //////////////////////////////////////////////////////////////////////////////
  // Upload our texture to vram

  PbTextureUpload( p_texture );
  PbDmaWait02();

  /////////////////////////////////////////////////////////////////////////////
  // Loop of the demo (just clears the screen, vsync and flip buffer

  while( pInfo->time_count > 0 )
  {
    PbScreenClear( 30<<16|20<<8|40 );

    ///////////////////////////////////////////////////////////////////////////
    // Draws a sprite with texture

    PbPrimSpriteTexture( p_texture, 
                          10<<4,  10<<4,   0<<4,   0<<4, 
                         400<<4, 100<<4, 255<<4, 255<<4, 0, 80<<16|70<<8|60 );
 
    ///////////////////////////////////////////////////////////////////////////
    // Sync and flipscreen


    PbScreenSyncFlip();
  }
  
  return pInfo->screen_mode;
}
