/*
 * SparmEnv.c - breakpoint demo part 2 adresd.
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
#include "PbMath.h"
#include "PbVu1.h"

#include "SparmEnv.h"

#include "water.h"


static u64 test[1024*400];

typedef struct {
  float x,y,z,w;
} fvec __attribute__((aligned(16)));;

typedef struct {
  unsigned int x,y,z,w;
} ivec __attribute__((aligned(16)));;

typedef struct {
  fvec iDU;
  fvec iDV;
  fvec iPHIMOD;
  fvec iTHETAMOD;
  ivec iRESOLI;
  ivec iRESOLJ;
  fvec iSCALE;
  fvec MF0;
  fvec MF2;
  fvec MF4;
  fvec MF6;
  fvec MF13;
  fvec MF57;
} PbPart6_Datat __attribute__((aligned(16)));;

static inline void set_fvec(fvec *vec,float val)
{
  vec->x = val;
  vec->y = val;
  vec->z = val;
  vec->w = val;
}
static inline void set_ivec(ivec *vec,unsigned int val)
{
  vec->x = val;
  vec->y = val;
  vec->z = val;
  vec->w = val;
}

static u64 texlog(u64 a)
{
  u64 r=0;
	
  a--;
  while(a>0)
  {
    a=a>>1;
    r++;
  }
	
  return r;
}

#define PI 3.142f
#define TWOPI (PI * 2.0f)
void PbPart6_setparam(PbPart6_Datat *data,
	int resoli,int resolj,float scale,
	float *mf)
{
  float resolif = (float) resoli;
  float resoljf = (float) resolj;
  set_fvec(&data->iDU,(PI/resolif));
  set_fvec(&data->iDV,(TWOPI/resoljf));
  data->iPHIMOD.x = data->iPHIMOD.y = 0.0f;
  data->iPHIMOD.z = data->iPHIMOD.w = (PI/resolif)/30.0f;
  data->iTHETAMOD.x = data->iTHETAMOD.y = 0.0f;
  data->iTHETAMOD.z = data->iTHETAMOD.w = (TWOPI/resoljf)/30.0f;
//iRESOLI         = 16	;resolution-i (int)
  set_ivec(&data->iRESOLI,resoli);
//iRESOLJ         = 17    ;resolution-j (int)
  set_ivec(&data->iRESOLJ,resolj);
//iSCALE          = 18	;scale in all members
  set_fvec(&data->iSCALE,scale);
;  data->iSCALE.x *= 2.0f;
;  data->iSCALE.z *= 2.0f;
//MF0             = 19	;mf[0]
  set_fvec(&data->MF0,mf[0]);
//MF2             = 20	;mf[2]
  set_fvec(&data->MF2,mf[2]);
//MF4             = 21	;mf[4]
  set_fvec(&data->MF4,mf[4]);
//MF6             = 22	;mf[6]
  set_fvec(&data->MF6,mf[6]);
//MF13            = 23	;mf[1],mf[3],mf[1],mf[3]
  data->MF13.x = mf[1];
  data->MF13.y = mf[3];
  data->MF13.z = mf[1];
  data->MF13.w = mf[3];
//MF57            = 24	;mf[5],mf[7],mf[5],mf[7]
  data->MF57.x = mf[5];
  data->MF57.y = mf[7];
  data->MF57.z = mf[5];
  data->MF57.w = mf[7];
}

static PbPart6_Datat pPart6Param;


void* PbPart6_DrawObjectSmoothNormals( PbMatrix* pLocalToScreen,PbMatrix* pNormalMatrix,int resolj, void *pChain,int prim_type)
{
  s32  num_coords   = 0;
  u32  num_sections = 0;
  u16  i            = 0;
  void* p_store_chain = pChain = (void*)&test;

  /////////////////////////////////////////////////////////////////////////////////////
  // Setup double buffering

  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  *((u32*)pChain)++ = VIF_CODE( VIF_STMOD,0,0 );  // normalmode
  *((u32*)pChain)++ = VIF_CODE( VIF_BASE,0,9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_OFFSET,0,512-9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add LocalToScreen  to the list
  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pLocalToScreen, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,0 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add Normal Matrix to the list
  *((u64*)pChain)++ = DMA_REF_TAG( (u32)pNormalMatrix, 4 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,4,4 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add parameters  to the list
  *((u64*)pChain)++ = DMA_REF_TAG( (u32)&pPart6Param, 15 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,15,12 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add GifTag to list

  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,25 );
  // we will be doing 2tri's for each data point (xyz,rgba,st), so 2*resolj
  *((u64*)pChain)++ = GIF_TAG( ((resolj*2)), 1,1, prim_type , 0, 3 );
  *((u64*)pChain)++ = 0x512; // registers to set - XYZF2,RGBAQ,ST

 ////////////////////////////////////////////////////////////////////////////////////
  // Add Call the program
  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_MSCAL,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  *((u64*)pChain)++ = DMA_END_TAG( 0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*)pChain)++ = VIF_CODE( VIF_NOP,0,0 );

  FlushCache(0);

  PbDmaWait01(); // lets make sure the dma is ready.
  PbDmaSend01Chain( p_store_chain );

  return NULL;
}

#define SET_MF(mfx,a,b,c,d,e,f,g,h) {mfx[0]=a;mfx[1]=b;mfx[2]=c;mfx[3]=d;mfx[4]=e;mfx[5]=f;mfx[6]=g;mfx[7]=h;}

static float mf[] = {1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f};
static float mf_add[] = {0.001f,0.0f,0.002f,0.0f,0.0f,0.0f,0.0f,0.0f};

static int render_method = 11; //11

extern PbTexture* p_texture_env;
extern PbTexture* p_texture_env_met;

void *PbPart6_DoObjects(PbMatrix *pCameraToScreen, PbMatrix *pWorldToCamera,void *pChain,
int time,u32 beat_impulse)
{
  PbMatrix pWorldToScreen;
  PbMatrix pLocalToScreen;
  PbMatrix pLocalToWorld;
  static float a = 0.005f;
  static float b = 0.0f;
  static float angle = 0.0f;
  static float scale = 100.0f;
  static int prim_type = GS_PRIM_PRIM_LINESTRIP;
  static int stepup = 0;
  static int resoli = 2; 
  static int resolj = 2;
  static float resolif = 1.0f; static float resoljf = 1.0f;
  // This is a 0 to 100 value, turn it into a %
  float beat_mult = 1.5f / ((201.0f-(float)beat_impulse) / 15.0f);
  beat_mult += 4.0f;

  if (render_method == 11)
  {
  if (beat_impulse == 200)
   ripple_click( 128,128);
  }

//initial value
if (time == 181) stepup = 0;
if (time == 180)
{ // big massive flower - metallic texture
  SET_MF(mf,40.0f,1.0f,40.0f,1.0f,1.0f,1.0f,1.0f,1.0f)
  SET_MF(mf_add,0.01f,0.0f,0.01f,0.0f,0.0f,0.0f,0.0f,0.0f)
  scale = 90.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_LINESTRIP;
  stepup = 1;
  render_method = 0;
  tex0_setup( p_texture_env_met,1);
}
if (time == 121) stepup = 0;
if (time == 120)
{ // growing flower
  SET_MF(mf,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f,1.0f)
  SET_MF(mf_add,0.002f,0.0f,0.002f,0.0f,0.0f,0.0f,0.0f,0.0f)
  scale = 100.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_LINESTRIP;
  stepup = 1;
  render_method = 11;
  tex0_setup( p_texture_env,1 );
}

if (time == 80) stepup = 0;
if (time == 81)
{ // This is the nice round globby object
  SET_MF(mf    ,2.0f,2.0f,0.0f,2.0f,2.0f,2.0f,0.0f,2.0f)
  SET_MF(mf_add,0.03f,0.0f,0.0f,0.0f,0.03f,0.0f,0.0f,0.0f)
  scale = 40.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_TRISTRIP ;
  stepup = 1;
  render_method = 12;

  tex0_setup( p_texture_env,1);
}

// above here isnt used, but was test objects, left in in case
// turn out to be useful
// This is where the demo proper starts, 26 seconds to go

if (time == 26) stepup = 0;
if (time == 25)
{ // nice blend and grow
//  SET_MF(mf,4.0f,2.0f,3.0f,2.0f,3.0f,2.0f,3.0f,2.0f)
  SET_MF(mf,    4.0f,2.0f,1.0f,2.0f,3.0f,2.0f,3.0f,2.0f)
  SET_MF(mf_add,0.0f,0.0f,0.004f,0.0f,0.0f,0.0f,0.0f,0.0f)
  scale = 80.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_TRISTRIP;
  stepup = 1;
  render_method = 0;
  tex0_setup( p_texture_env,1);
}

if (time == 17) stepup = 0;
if (time == 16)
{ // this is the nice growing tall organic shape
  SET_MF(mf,1.0f,6.0f,1.0f,3.0f,1.0f,3.0f,1.0f,6.0f)
  SET_MF(mf_add,0.0015f,0.0f,0.0015f,0.0f,0.0f,0.0f,0.0f,0.0f)
  scale = 100.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_TRISTRIP ;
//  prim_type = GS_PRIM_PRIM_TRISTRIP | GS_PRIM_GOURAUD | PS2_GS_PRIM_TME_ON |PS2_GS_PRIM_ABE_ON;
  stepup = 1;
  render_method = 10;
  tex0_setup( p_texture_env_met,1);
}

if (time == 9) stepup = 0;
if (time == 8)
{ // squareish obj
  SET_MF(mf,6.0f,3.0f,6.0f,3.0f,2.0f,3.0f,2.0f,3.0f)
  SET_MF(mf_add,0.005f,0.0f,0.005f,0.0f,0.0f,0.0f,0.0f,0.0f)
  scale = 75.0f;
  resoli = resolj = 1;
  prim_type = GS_PRIM_PRIM_LINESTRIP;
  stepup = 1;
  render_method = 11;
  tex0_setup( p_texture_env ,1);
}

if (time == 1) 
  stepup = 0;

  // This updates our values, for anim
  {
    int c;
    for (c=0;c<8;c++)
      mf[c] += (mf_add[c] * beat_mult);
  }

  if (stepup)
  { // This does growing, so appears smoothly
    if (resolif < 160.0f) resolif += 2.0f;   // for vu3
    else resolif = 160.0f;
    if (resoljf < 98.0f)  resoljf += 1.0f;
    else resoljf = 98.0f;
  }
  else 
  { // This does shrinking, so disappears
    if (resolif > 3.0f) resolif += -3.0f;
    else resolif = 1.0f;
    if (resoljf > 2.0f) resoljf += -2.0f;
    else resoljf = 1.0f;
  }
  resoli = (int) resolif;
  resolj = (int) resoljf;

  PbPart6_setparam(&pPart6Param,resoli,resolj,scale,mf);

  PbMatrixMultiply(&pWorldToScreen,pWorldToCamera,pCameraToScreen);

  angle += 0.01f;
  PbMatrixRotateY( &pLocalToWorld, angle);

  PbMatrixMultiply(&pLocalToScreen,&pLocalToWorld,&pWorldToScreen);

  prim_type  |= (1<<4) | (1<<9);
  PbMatrixRotateY( &pLocalToWorld, angle);
  PbPart6_DrawObjectSmoothNormals(&pLocalToScreen, &pLocalToWorld, resolj,pChain,prim_type);

  return NULL;
}

#define ALPHA_SRC 0
#define ALPHA_DST 1
#define ALPHA_ZERO 2
#define ALPHA_FIX 2
#define ALPHA(A,B,C,D,FIX) ( (((u64)(A))&3) | ((((u64)(B))&3)<<2) | ((((u64)(C))&3)<<4) | ((((u64)(D))&3)<<6) | ((((u64)(FIX)))<<32UL) )//(A - B)*C >> 7 + D 

void blit_rendertarget(PbTexture* tex, int xoff, int yoff, int xzoom, int yzoom, int intensity, u32 color)
{		
	  u64* p_store;
	  u64* p_data;
	
	  int x1 = xzoom   + ((0 + 2048) << 4);
	  int y1 = yzoom   + ((0 + 2048) << 4);
	  int x2 = -xzoom  + ((SCR_W + 2048) << 4);
	  int y2 = -yzoom  + ((SCR_H + 2048) << 4);
	  int u1 = (0) << 4;
	  int v1 = (0) << 4;
	  int u2 = (tex->x - 1) << 4;
	  int v2 = (tex->y - 1) << 4;
	  
	  x1 += xoff;
	  y1 += yoff;
	  x2 += xoff;
	  y2 += yoff;
	
	  p_store = p_data = PbSprAlloc( 10*16 );
	
	  *p_data++ = GIF_TAG( 9, 1, 0, 0, 0, 1 );
	  *p_data++ = GIF_AD;
	
	  *p_data++ = PbTextureGetTex0( tex );
	  *p_data++ = GS_TEX0_1;
	
	  *p_data++ = 0x20; // bilinear filtering
	  *p_data++ = GS_TEX1_1;
	  
	  *p_data++ = ALPHA(ALPHA_SRC,ALPHA_ZERO,ALPHA_FIX,ALPHA_DST,intensity);
	  *p_data++ = GS_ALPHA_1;
	
// abe	  
        *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0) ;
// next is aa1, abe by default
//	  *p_data++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 1, 1, 0, 0) ;
	  *p_data++ = GS_PRIM;

	  *p_data++ = color;
	  *p_data++ = GS_RGBAQ;
	
	  *p_data++ = GS_SETREG_XYZ2( x2, y2, 0 );
	  *p_data++ = GS_XYZ2;
	
	  *p_data++ = GS_SETREG_UV( u1, v1 );
	  *p_data++ = GS_UV;
	
	  *p_data++ = GS_SETREG_XYZ2( x1, y1, 0 );
	  *p_data++ = GS_XYZ2;
	
	  *p_data++ = GS_SETREG_UV( u2, v2 );
	  *p_data++ = GS_UV;
	  
	  PbDmaWait02();
	  PbDmaSend02Spr( p_store, 10 );
}




void *PbPart6_Do2D(PbTexture* p_texture,PbTexture* p_texturetemp)
{
  u32 sizex = p_texture->x << 4;
  u32 sizey = p_texture->y << 4;

  switch(render_method)
  {
    case 0: // one fullscreen sprite
      PbPrimSpriteTexture( p_texture, 0, 0,0,0, 
                          SCR_W<<4, SCR_H<<4, sizex,sizey , 0, 0x80808080 );
     break;
    case 1: // 4 quarter sized ones (512)
      // x=0, y=0
      PbPrimSpriteTexture( p_texture, 0, 0,0,0, 
                          SCR_W<<3, SCR_H<<3, sizex,sizey , 0, 0x80800000 );

      // x=1, y=1
      PbPrimSpriteTexture( p_texture, SCR_W<<3, SCR_H<<3,0,0, 
                          SCR_W<<4, SCR_H<<4,sizex,sizey , 0, 0x80000080 );

      // x=0, y=1
      PbPrimSpriteTexture( p_texture, SCR_W<<3, 0,0,0, 
                          SCR_W<<4, SCR_H<<3, sizex,sizey , 0, 0x80808080 );

      // x=1, y=0
      PbPrimSpriteTexture( p_texture, 0, SCR_H<<3,0,0, 
                          SCR_W<<3, SCR_H<<4, sizex,sizey , 0, 0x80800080 );
     break;
    case 2: // 4 quarter sized ones (256)
      // x=0, y=0
      PbPrimSpriteTexture( p_texture, 0, 0,sizex,sizey, 
                          SCR_W<<3, SCR_H<<3, 0,0 , 0, 0x80800000 );

      // x=1, y=1
      PbPrimSpriteTexture( p_texture, SCR_W<<3, SCR_H<<3,sizex,sizey, 
                          SCR_W<<4, SCR_H<<4, 0,0 , 0, 0x80000080 );

      // x=0, y=1
      PbPrimSpriteTexture( p_texture, SCR_W<<3, 0,sizex,sizey, 
                          SCR_W<<4, SCR_H<<3, 0,0 , 0, 0x80808080 );

      // x=1, y=0
      PbPrimSpriteTexture( p_texture, 0, SCR_H<<3,sizex,sizey, 
                          SCR_W<<3, SCR_H<<4, 0,0 , 0, 0x80800080 );
     break;
    case 3: // water effect
      PbPrimSpriteTexture( p_texture, 0, 0,sizex,sizey, 
                          SCR_W<<2, SCR_H<<2, 0,0 , 0, 0x80808080 );
     break;
    case 4:
     break;
    case 10: // This does the double draw, blended one (200-180)
     set_zbufcmp( 1 );
     blit_rendertarget(p_texture, 0, 0, 0, 0, 128 | 64,0x40206060);
     blit_rendertarget(p_texture, 0, 0, -440, -180, 128 | 10,0x40400000);
     break;
    case 11: // WATER
      water_tick(p_texture,p_texture_env);
      break;
    case 12: // seperated colors, scaled slightly.. for the spikey obj
     set_zbufcmp( 1 );
     blit_rendertarget(p_texture, 0, 0, 0   , 0   , 128  | 1,0x40400000);
     blit_rendertarget(p_texture, 0, 0, -220, -90,  128  | 1,0x40000020);
     blit_rendertarget(p_texture, 0, 0, -440, -180, 128  | 1,0x40000040);
     break;
    default:
     render_method = 0;
     break;
  }
}
