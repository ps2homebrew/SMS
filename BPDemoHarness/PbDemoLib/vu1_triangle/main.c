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
#include "PbDma.h"
#include "PbGs.h"
#include "PbScreen.h"
#include "PbMisc.h"
#include "PbTexture.h"
#include "PbSpr.h"
#include "PbMatrix.h"
#include "PbVif.h"
#include "PbVu1.h"

///////////////////////////////////////////////////////////////////////////////
// Reference to our vu1 program
///////////////////////////////////////////////////////////////////////////////

extern u32 g_Vu1_Triangle __attribute__((section(".vudata")));
extern u32 g_Vu1_Triangle_End __attribute__((section(".vudata")));

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

u32 start_demo(const demo_init_t *t)
{
  PbMatrix viewscreen_matrix;
  PbMatrix camera_matrix;
  PbMatrix rotate_matrix;
  PbMatrix final_matrix;
  PbMatrix temprot_matrix;
  u64*     p_store; 
  u64*     p_data; 
  int      offset_x;
  int      offset_y;
  float    angle = 0.0f;

  /////////////////////////////////////////////////////////////////////////////
  // Setup screen and get virtual screen offsets

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  offset_x = PbScreenGetOffsetX();
  offset_y = PbScreenGetOffsetY();

  /////////////////////////////////////////////////////////////////////////////
  // Upload our program

  PbVif1UploadPrg( 0, &g_Vu1_Triangle, &g_Vu1_Triangle_End );

  /////////////////////////////////////////////////////////////////////////////
  // Create our view to screen matrix

  PbMatrixViewScreen( &viewscreen_matrix, 512.0f,1.0f,1.0f, 
                      offset_x+SCR_W/2,offset_y+SCR_H/2,
                      1.0f, 6777215.0f,64.0f, 5536.0f );

  /////////////////////////////////////////////////////////////////////////////
  // Setup the matrix for our camera

  PbMatrixIdentity( &camera_matrix );
  PbMatrixTranslate( &camera_matrix, 0, 0, 800 );

  /////////////////////////////////////////////////////////////////////////////
  // Loop of the demo

  while( 1 )
  {
    angle += 0.03f;

    PbScreenClear( 30<<16|20<<8|40 );

    ///////////////////////////////////////////////////////////////////////////
    // Rotate & build final matrix to be used in the vu1 program.

    PbMatrixRotateZ( &rotate_matrix, angle );
    PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &camera_matrix );
    PbMatrixMultiply( &final_matrix, &temprot_matrix, &viewscreen_matrix );
    FlushCache(0);  

    p_data = p_store = PbSprAlloc( 10*16 );

    ///////////////////////////////////////////////////////////////////////////
    // Build the data we need to send to the vu1

    *((u64*)p_data)++ = DMA_REF_TAG( (u32)&final_matrix, 4 );
    *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
    *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

    ///////////////////////////////////////////////////////////////////////////
    // Add giftag and color of the triangle

    *((u64*)p_data)++ = DMA_CNT_TAG( 5 );
    *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
    *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,5,4 );

    *((u64*)p_data)++ = GIF_TAG( 1, 1, 1, GS_SETREG_PRIM( GS_PRIM_PRIM_TRIANGLE,
                                          0, 0, 0, 0, 0, 0, 0, 0 ), 0, 4 );
    *((u64*)p_data)++ = 0x5551;

    *((u32*)p_data)++  = 127;
    *((u32*)p_data)++  = 127;
    *((u32*)p_data)++  = 127;
    *((u32*)p_data)++  = 0;

    ///////////////////////////////////////////////////////////////////////////
    // add vertex coords of the triangle

    *((float*)p_data)++ = -100.0f;
    *((float*)p_data)++ =  100.0f;
    *((float*)p_data)++ =    0.0f;
    *((float*)p_data)++ =    1.0f;

    *((float*)p_data)++ = -100.0f;
    *((float*)p_data)++ = -100.0f;
    *((float*)p_data)++ =    0.0f;
    *((float*)p_data)++ =    1.0f;

    *((float*)p_data)++ =   50.0f;
    *((float*)p_data)++ =   50.0f;
    *((float*)p_data)++ =  100.0f;
    *((float*)p_data)++ =    1.0f;

    ///////////////////////////////////////////////////////////////////////////
    // End the list and start the program

    *((u64*)p_data)++ = DMA_END_TAG( 0 );
    *((u32*)p_data)++ = VIF_CODE( VIF_NOP,0,0 );
    *((u32*)p_data)++ = VIF_CODE( VIF_MSCAL,0,0 );

    PbDmaSend01ChainSpr( p_store );

    ///////////////////////////////////////////////////////////////////////////
    // Sync and flipscreen

    PbScreenSyncFlip();
  }
  
  return t->screen_mode;
}
