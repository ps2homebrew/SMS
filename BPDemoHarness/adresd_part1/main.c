/*
 * main.c - breakpoint demo part 1 adresd.
 *
 * Copyright (c) 2004   adresd <adresd_ps2dev@yahoo.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com>
 */

#include <tamtypes.h>
#include <kernel.h>
#include "../harness.h"
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

#include "SparmGenDot.h"

///////////////////////////////////////////////////////////////////////////////
// Reference to our vu1 program
///////////////////////////////////////////////////////////////////////////////

extern u32 g_Vu1_SparmGenDots __attribute__((section(".vudata")));
extern u32 g_Vu1_SparmGenDots_End __attribute__((section(".vudata")));


const demo_init_t *gp_Info;

void set_zbufcmp(int cmpmode)
{
  u64* p_store;
  u64* p_data;

  p_store = p_data = PbSprAlloc( 7*16 );
  
  /////////////////////////////////////////////////////////////////////////////
  // Setup for drawing

  *p_data++ = GIF_TAG( 1, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, cmpmode );     
  *p_data++ = GS_TEST_1,     

  PbDmaSend02Spr( p_store, 2);
}

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

u32 start_demo( const demo_init_t* pInfo )
{
  PbMatrix ViewScreenMatrix;
  PbMatrix CameraMatrix;
  int      offset_x;
  int      offset_y;
  int      down = 1;

  gp_Info = pInfo;

  /////////////////////////////////////////////////////////////////////////////
  // Setup screen and get virtual screen offsets

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  offset_x = PbScreenGetOffsetX();
  offset_y = PbScreenGetOffsetY();

  /////////////////////////////////////////////////////////////////////////////
  // Upload our program

  PbVif1UploadPrg( 0, &g_Vu1_SparmGenDots, &g_Vu1_SparmGenDots_End );

  /////////////////////////////////////////////////////////////////////////////
  // Create our view to screen matrix

  PbMatrixViewScreen( &ViewScreenMatrix, 512.0f,1.0f,1.0f, 
                      offset_x+SCR_W/2,offset_y+SCR_H/2,
                      1.0f, 6777215.0f,64.0f, 5536.0f );

  /////////////////////////////////////////////////////////////////////////////
  // Setup the matrix for our camera

  PbMatrixIdentity( &CameraMatrix );
  PbMatrixTranslate( &CameraMatrix, 0, 0, 700  );

  // Enter loop
  while( pInfo->time_count > 0 )
  { 
    set_zbufcmp( 1 );
    PbScreenClear( 50<<16|20<<8|20 );

    set_zbufcmp( 2 );
#define SPLIT_TIME1 201
#define SPLIT_TIME2 401
#define SPLIT_TIME3 601
#define SPLIT_TIME4 1000

//    if ( ((int)(pInfo->time_count) > SPLIT_TIME3) &&
//         ((int)(pInfo->time_count) <= SPLIT_TIME4) )
//    {
      // This is the nice dot/tri/tristrip version
      SparmGenDot_Render(&ViewScreenMatrix, &CameraMatrix,NULL, (int)(pInfo->time_count)-SPLIT_TIME3);
      PbVu1Wait();
//    }

    ///////////////////////////////////////////////////////////////////////////
    // Sync and flipscreen
    PbScreenSyncFlip();   
  }
  return pInfo->screen_mode;
}


