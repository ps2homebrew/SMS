/*
 * main.c - Emoon part 2 for pbdemo
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
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
#include "PbPrim.h"
#include "PbMath.h"
#include "PbVec.h"
#include "PbVu0m.h"

#include "Particles/PbParticle.h"

///////////////////////////////////////////////////////////////////////////////
// Reference to our vu1 program
///////////////////////////////////////////////////////////////////////////////

extern u32 g_Vu1CubeRender __attribute__((section(".vudata")));
extern u32 g_Vu1CubeRender_End __attribute__((section(".vudata")));
extern u32 Cube;

///////////////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////////////

const demo_init_t *g_pInfo;


/*  DEFINES AND ENUMERATIONS
 */
 
#define NUM_POINTS 110

/*  DATA
 */
PbMatrix view_screen_matrix;
PbMatrix camera_matrix;
PbFvec points[NUM_POINTS];
PbFvec direction[NUM_POINTS];
PbTexture* texture;
static u32 texture_test_32[16*16] __attribute__((aligned(16)));
static u8 binary_texture_start[] = {0,0,0,0,0,0,0,0,0,0,1,1,0,1,2,0,2,4,1,2,5,1,3,6,1,4,7,1,3,6,1,4,6,1,2,5,1,2,3,0,1,2,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,0,2,3,1,3,6,1,4,9,2,6,12,2,7,15,2,9,17,2,9,16,2,8,15,2,6,12,1,4,8,1,3,6,1,1,2,0,1,1,0,0,0,0,1,2,0,2,4,1,4,7,2,6,12,2,10,18,3,13,25,5,16,31,6,18,34,6,18,32,5,17,30,4,13,24,3,10,18,2,6,10,1,4,6,1,2,3,0,1,1,0,2,3,1,3,7,2,7,13,3,12,21,5,17,32,7,23,42,9,28,51,10,31,56,11,31,56,10,29,51,9,24,43,6,18,33,4,12,21,2,7,12,1,4,6,0,1,2,1,2,6,1,6,10,2,12,22,6,19,35,9,27,50,13,35,64,17,43,74,21,48,80,22,49,81,20,46,76,16,39,67,11,29,51,7,20,35,4,12,21,2,6,11,0,2,4,1,4,8,2,9,17,4,17,31,8,27,50,14,38,67,22,50,83,30,60,95,37,68,104,38,69,106,35,65,100,27,56,89,18,43,72,12,29,51,6,18,31,3,9,16,1,2,4,1,6,11,3,12,23,6,21,40,12,34,61,21,48,81,33,63,99,46,78,115,56,89,124,58,91,126,53,86,122,41,73,108,29,56,90,17,39,65,9,24,42,4,12,23,1,2,5,2,7,13,4,14,27,7,26,48,15,39,69,26,56,91,43,75,112,61,94,128,73,105,137,76,108,139,70,102,135,55,87,123,37,67,102,21,46,76,12,29,49,6,15,26,1,3,6,2,7,15,4,15,29,9,28,50,17,43,73,31,61,96,50,82,119,70,102,135,83,113,142,86,115,143,80,110,141,64,95,130,42,73,108,24,51,81,13,31,54,6,17,29,1,2,5,2,7,14,4,15,28,8,27,50,17,42,72,31,61,96,51,82,118,71,102,135,84,113,142,87,116,143,81,111,141,64,96,130,43,73,108,23,50,80,12,30,51,6,16,28,1,2,5,1,6,12,3,13,25,7,24,43,14,37,66,26,54,89,43,75,111,62,94,128,75,106,138,78,108,139,72,102,135,56,87,122,36,65,100,21,45,73,11,28,47,5,14,24,0,2,4,1,5,9,2,10,20,6,20,37,11,32,56,19,45,76,32,61,95,46,77,112,56,88,123,59,91,125,53,84,119,40,70,105,27,54,84,16,36,60,9,21,37,4,11,20,0,1,2,1,3,6,2,7,14,4,14,26,7,23,43,12,34,60,21,46,78,29,58,91,35,65,99,37,67,102,33,62,95,26,51,83,17,39,65,10,25,44,6,15,26,3,8,13,0,1,1,1,2,4,1,4,9,2,9,16,4,15,28,7,23,42,12,31,55,16,39,66,20,43,73,20,43,72,18,41,69,14,34,58,10,25,43,6,17,29,4,9,17,2,5,8,0,0,0,0,1,1,1,2,4,1,5,9,2,9,17,4,13,25,6,18,34,8,23,42,9,26,45,10,26,46,9,24,42,7,20,34,5,14,25,3,9,15,2,6,9,1,2,4,0,0,0,0,0,0,0,1,2,1,2,5,1,4,9,2,7,12,2,9,17,4,12,21,4,13,24,5,14,25,4,12,21,4,10,18,2,7,12,2,5,9,1,2,4,1,1,2};

/*  FUNCTIONS
 */
 
float frandom()
{
  return (float)PbRand() / (float)PB_RAND_MAX;
}

void build_matrices()
{
  PbMatrixViewScreen( &view_screen_matrix, 512.0f,1.0f,(float)SCR_H/(float)SCR_W, PbScreenGetOffsetX()+SCR_W/2,PbScreenGetOffsetY()+SCR_H/2, 1.0f, 6777215.0f,64.0f, 5536.0f );
  PbMatrixIdentity( &camera_matrix );
  PbMatrixTranslate( &camera_matrix, 0, 0, 400 ); 
}

void generate_points()
{
  int i;
  
  g_pInfo->printf( "bla\n" );

  for(i=0; i<NUM_POINTS; ++i)
  {
    points[i].x = frandom()*400-200;
    points[i].y = frandom()*400-200;
    points[i].z = frandom()*400-200;
    points[i].z += 800;
    points[i].w = 1.0f;
    
    direction[i].x = direction[i].y = direction[i].z = direction[i].w = 0.0f;
  } 
}


void load_texture()
{ 
  int cnt;
  
  for(cnt=0; cnt<16*16; ++cnt)
  {
    u32 r = binary_texture_start[cnt*3+0]>>3;
    u32 g = binary_texture_start[cnt*3+1]>>3;
    u32 b = binary_texture_start[cnt*3+2]>>3;
    texture_test_32[cnt] = r | (g<<8) | (b<<16) | (0x80<<24UL);
  }
  FlushCache(0);
  texture = PbTextureCreate32(texture_test_32, 16, 16); 
  PbTextureUpload(texture);
  PbDmaWait02();
}

void setup()
{
  PbScreenSetup( SCR_W, SCR_H, SCR_PSM );
  build_matrices();
  generate_points();
  load_texture();
}

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void EmPart2Setup();
void EmPart2RenderCube( PbMatrix* pCamera,PbMatrix* pScreenView,int dest, 
                        float xs, float ys, float zs );
void EmPart2SetAlpha( u64 A, u64 B, u64 C, u64 D, u64 FIX );


///////////////////////////////////////////////////////////////////////////////
// u32 start_demo
///////////////////////////////////////////////////////////////////////////////

u32 start_demo(const demo_init_t *pInfo)
{
  float last_t;
  float angle      = 0.0f;
  float angle_hold = 0.0f;
  PbMatrix rotate_matrix, rotate_matrix2, temprot_matrix, temprot_matrix2;

  g_pInfo = pInfo;

  setup();
  EmPart2Setup();


  //PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &camera_matrix );
  //EmPart2RenderCube( &temprot_matrix, &view_screen_matrix, 100, 0.5f, 0.5f, 0.5f );     

  //PbVu1Dump();
  
  last_t = pInfo->curr_time;


  EmPart2SetAlpha( GS_ALPHA_A_CS, GS_ALPHA_B_ZERO, 
                   GS_ALPHA_C_FIX, GS_ALPHA_D_CD, 0x80 );

//  EmPart2SetAlpha( 0, 1, 0, 1, 0x80 );


  while(pInfo->frame_count > 0)
  {
    float t= pInfo->curr_time;
    //float dt = t-last_t;
    int i = 0;
    last_t= t;
    angle = t*1.5f;

    PbScreenClear(0);

    angle_hold = angle;
/*
    PbMatrixRotateY( &rotate_matrix, t*0.5f );
    PbMatrixTranslate( &rotate_matrix, 0.0f, 0.0f, 400.0f );
    PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &camera_matrix );
    PbMatrixMultiply( &final_matrix, &temprot_matrix, &view_screen_matrix );
    PbParticleSetup(&final_matrix, texture, 12.0f *16.0f, 1.0f, 0x40404040);
		PbParticleDraw(points, NUM_POINTS);
*/
    for( i = 0; i < NUM_POINTS; i += 2 )
    {
      PbMatrixRotateY( &rotate_matrix, angle_hold );
      PbMatrixRotateX( &rotate_matrix2, angle_hold );
      PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &rotate_matrix2 );
      PbMatrixTranslate( &temprot_matrix, points[i+0].x, points[i+0].y, points[i+0].z );
      PbMatrixMultiply( &temprot_matrix2, &temprot_matrix, &camera_matrix );
      EmPart2RenderCube( &temprot_matrix2, &view_screen_matrix, 50, 0.5f, 0.5f, 0.5f );     

      PbMatrixRotateY( &rotate_matrix, angle_hold );
      PbMatrixRotateX( &rotate_matrix2, angle_hold );
      PbMatrixMultiply( &temprot_matrix, &rotate_matrix, &rotate_matrix2 );
      PbMatrixMultiply( &temprot_matrix2, &temprot_matrix, &camera_matrix );
      PbMatrixTranslate( &temprot_matrix2, points[i+1].x, points[i+1].y, points[i+1].z );
      EmPart2RenderCube( &temprot_matrix2, &view_screen_matrix, 150, 0.5f, 0.5f, 0.5f );     

      angle_hold += 0.05f;
    }          
        
    PbScreenSyncFlip();
  }
  
  return pInfo->screen_mode;
}

///////////////////////////////////////////////////////////////////////////////
// EmPart2Setup
// Upload vu program and our cube (only uploaded once as it will not change)
///////////////////////////////////////////////////////////////////////////////

void EmPart2Setup()
{
  u64* p_data;
  u64* p_store;
  u32* p_cube = &Cube;

  //p_cube += (4*3)*10;

  PbVif1UploadPrg( 0, &g_Vu1CubeRender, &g_Vu1CubeRender_End );

  p_data = p_store = PbSprAlloc( 2*16 );
  
  *p_data++         = DMA_REF_TAG( (u32)p_cube, 3*12 );
  *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,3*12,14 );

  *((u64*)p_data)++ = DMA_END_TAG( 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_NOP, 0, 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_NOP, 0, 0 );

  PbDmaSend01ChainSpr( p_store );
}

///////////////////////////////////////////////////////////////////////////////
// EmPart2Setup
// Upload vu program and our cube (only uploaded once as it will not change)
///////////////////////////////////////////////////////////////////////////////

void EmPart2RenderCube( PbMatrix* pCamera,PbMatrix* pScreenView,int dest, 
                        float xs, float ys, float zs )
{
  float scale[4] __attribute__((aligned(16)));
  u64* p_data;
  u64* p_store;

  scale[0] = xs;  
  scale[1] = ys;  
  scale[2] = zs;  
  scale[3] = 1;  

  p_data = p_store = PbSprAlloc( 10*16 );

  FlushCache(0);

  /////////////////////////////////////////////////////////////////////////////
  // Add CameraToScreen & WorldToCamera

  *((u64*)p_data)++ = DMA_REF_TAG( (u32)pScreenView, 4 );
  *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

  *((u64*)p_data)++ = DMA_REF_TAG( (u32)pCamera, 4 );
  *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,4,4 );
  
  /////////////////////////////////////////////////////////////////////////////
  // Parameters

  *((u64*)p_data)++ = DMA_REF_TAG( (u32)&scale, 1 );
  *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,1,8 );

  /////////////////////////////////////////////////////////////////////////////
  // Add Giftag

  *((u64*)p_data)++ = DMA_CNT_TAG( 1 );
  *((u32*)p_data)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)p_data)++ = VIF_CODE( VIF_UNPACK_V4_32,1,13 );

  *((u64*)p_data)++ = GIF_TAG( 12, 1, 1, GS_SETREG_PRIM( GS_PRIM_PRIM_TRIANGLE,
                                                         0, 0, 0, 1, 0, 0, 0, 0 ), 0, 4 );
  *((u64*)p_data)++ = 0x5551; // registers to set

  /////////////////////////////////////////////////////////////////////////////
  // End and call

  *((u64*)p_data)++ = DMA_END_TAG( 0 );
  *((u32*)p_data)++ = VIF_CODE( VIF_ITOP, 0, dest );
  *((u32*)p_data)++ = VIF_CODE( VIF_MSCAL, 0, 0 );

  /////////////////////////////////////////////////////////////////////////////
  // Send data to vif

  PbDmaSend01ChainSpr( p_store );
}

///////////////////////////////////////////////////////////////////////////////
// void PbPrimSetAlpha
///////////////////////////////////////////////////////////////////////////////

void EmPart2SetAlpha( u64 A, u64 B, u64 C, u64 D, u64 FIX )
{
  u64* p_data;
  u64* p_store;

  p_data = p_store = PbSprAlloc( 2*16 );

  *p_data++ = GIF_TAG( 1, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  *p_data++ = GS_SETREG_ALPHA( A, B, C, D, FIX );
  *p_data++ = GS_ALPHA_1;

  PbDmaSend02Spr( p_store, 2 );
}

