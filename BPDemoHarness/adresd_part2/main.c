/*
 * main.c - breakpoint demo part 2 adresd.
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

#include "SparmEnv.h"

///////////////////////////////////////////////////////////////////////////////
// Reference to our vu1 program
///////////////////////////////////////////////////////////////////////////////
extern u32 g_Vu1_SparmGenEnv __attribute__((section(".vudata")));
extern u32 g_Vu1_SparmGenEnv_End __attribute__((section(".vudata")));

extern char binary_funkyenv1_start[];
extern char binary_funkyenv2_start[];

static u32 texture_test_32[256*256] __attribute__((aligned(16)));

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

void tex0_setup( PbTexture* pTex, u32 context) 
{ 
  u64* p_store; 
  u64* p_data; 
  int  size = 2;

  u32 g_PbPrimContext=0;
  u64 color = 0x40404040;

  p_store = p_data = PbSprAlloc( 2*16 ); 
   
  *p_data++ = GIF_TAG( size-1, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD; 

  *p_data++ = PbTextureGetTex0( pTex ); 
  *p_data++ = GS_TEX0_1+context; 

  *p_data++ = GS_SETREG_TEX1_1(0,0,1,1,0,0,0);
  *p_data++ = GS_TEX1_1+context; 

  PbDmaSend02Spr( p_store, size ); 
}

void PbTextureSetRenderTarget2( PbTexture* pTexture,u32 context )
{
	u64* p_data = NULL;
	u64* p_store = NULL;
	
	p_data = p_store = PbSprAlloc( 3*16 );
	
	*p_data++ = GIF_TAG( 2, 1, 0, 0, 0, 1 );
	*p_data++ = GIF_AD;
	
	*p_data++ = GS_SETREG_SCISSOR_1( 0, pTexture->x - 1, 0, pTexture->y - 1 ); 
	*p_data++ = GS_SCISSOR_1+context;
	
	*p_data++ = GS_SETREG_FRAME_1( pTexture->Vram / 8192, pTexture->x / 64, pTexture->psm, 0 );  
	*p_data++ = GS_FRAME_1+context; 
	
	
	PbDmaSend02Spr(p_store, 3);
	PbDmaWait02();
}

///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

PbTexture* p_texture_env = NULL;
PbTexture* p_texture_env_met = NULL;

u32 start_demo( const demo_init_t* pInfo )
{
  PbMatrix ViewScreenMatrix;
  PbMatrix ViewScreenMatrixTex;
  PbMatrix CameraMatrix;
  PbTexture* p_texture1 = NULL;
  PbTexture* p_texture2 = NULL;
  PbTexture* p_texture3 = NULL;
  PbTexture* p_textmp = NULL;
  int      offset_x;
  int      offset_y;
  int      down = 1;

#define MIN_RECTSIZE 0
#define MAX_RECTSIZE 200
#define RECT_FALLOFF 5
  u16 *fft_data;
  u32 syncs_left = pInfo->no_syncs;
  volatile u32 *syncs = pInfo->sync_points;
  u32 beat_impulse = MIN_RECTSIZE;

  gp_Info = pInfo;

  PbMiscSetOutput(pInfo->printf);

  /////////////////////////////////////////////////////////////////////////////
  // Setup screen and get virtual screen offsets

  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );

  offset_x = PbScreenGetOffsetX();
  offset_y = PbScreenGetOffsetY();

  /////////////////////////////////////////////////////////////////////////////
  // Upload our program

  PbVif1UploadPrg( 0, &g_Vu1_SparmGenEnv, &g_Vu1_SparmGenEnv_End );

  /////////////////////////////////////////////////////////////////////////////
  // Create our view to screen matrix
  PbMatrixViewScreen( &ViewScreenMatrix, 512.0f,1.0f,1.0f, 
                      offset_x+SCR_W/2,offset_y+SCR_H/2,
                      1.0f, 6777215.0f,64.0f, 5536.0f );

  /////////////////////////////////////////////////////////////////////////////
  // Create our view to screen matrix for texture rendering
  PbMatrixViewScreen( &ViewScreenMatrixTex, 512.0f,1.0f,1.0f, // 0.5,0.8// 1.3f for last
                      offset_x+512/2,offset_y+256/2,
                      1.0f, 6777215.0f,64.0f, 5536.0f );
  /////////////////////////////////////////////////////////////////////////////
  // Setup the matrix for our camera

  PbMatrixIdentity( &CameraMatrix );
  PbMatrixTranslate( &CameraMatrix, 0, 0, 700  );

  //////////////////////////////////////////////////////////////////////////////
  // Setup the two texture buffers
  p_texture1 = PbTextureCreate32( texture_test_32, 512, 256 ); 
  p_texture2 = PbTextureCreate32( texture_test_32, 512, 256 ); 
  p_texture3 = PbTextureCreate32( texture_test_32, 512, 256 ); 
  p_texture_env = PbTextureCreate32( binary_funkyenv1_start, 256, 256 ); 
  p_texture_env_met = PbTextureCreate32( binary_funkyenv2_start, 256, 256 ); 

  //////////////////////////////////////////////////////////////////////////////
  // Upload our textures to vram
  PbTextureUpload( p_texture_env );
  PbTextureUpload( p_texture_env_met );
 
  water_init();
  tex0_setup(p_texture_env,1);

  PbDmaWait02();
  set_zbufcmp( 1 );
  PbScreenClear( 0<<16|0<<8|0 );
  PbScreenSyncFlip();   

  // Enter loop
  while( pInfo->time_count > 0 )
  { 
    PbScreenSetCurrentActive();
    set_zbufcmp( 1 );
    PbScreenClear( 30<<16|10<<8|10 );

    GS_SET_BGCOLOR(0,0,0);

    // Draw to render target    
    PbTextureSetRenderTarget2(p_texture1,1);

    PbPrimSetContext(1 ); // This clears out our current render texture
    PbPrimSpriteNoZtest( 0, 0, SCR_W<<4, SCR_H<<4, 0, 0x000000 );
    PbPrimSetContext(0 );

    set_zbufcmp( 2 );

    // This is the smooth normals version, not as complex, but smoother
    PbPart6_DoObjects(&ViewScreenMatrixTex, &CameraMatrix,NULL, (int)(pInfo->time_count),beat_impulse);

    PbPart6_Do2D(p_texture2,p_texture3);

    // This is the FFT and sync stuff
    fft_data = pInfo->get_fft();
    if(beat_impulse > MIN_RECTSIZE)
    {
      beat_impulse -= RECT_FALLOFF;
      if(beat_impulse < MIN_RECTSIZE) beat_impulse = MIN_RECTSIZE;
    }
    if(syncs_left > 0)
    {
      if(*syncs < pInfo->get_pos())
      {
        beat_impulse = MAX_RECTSIZE;
        syncs++;
        syncs_left--;
      } 
    }
    PbMatrixIdentity( &CameraMatrix );
    PbMatrixTranslate( &CameraMatrix, (MAX_RECTSIZE - beat_impulse)/6, (MAX_RECTSIZE - beat_impulse)/6, 800 - (beat_impulse/2) );


    PbVu1Wait();
    PbDmaWait02();

    // swap texture pointers over
    p_textmp = p_texture1;
    p_texture1 = p_texture2;
    p_texture2 = p_textmp;

    ///////////////////////////////////////////////////////////////////////////
    // Sync and flipscreen
    PbScreenSyncFlip();   
  }
  return pInfo->screen_mode;
}


