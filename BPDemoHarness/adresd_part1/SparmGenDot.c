/*
 * SpharmGenDot.c - breakpoint demo part 1 adresd.
 * Spherical Harmonics Generator, setup and kick to vu1
 *
 * Copyright (c) 2004   adresd <adresd_ps2dev@yahoo.com>
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

typedef struct {
  float x,y,z,w;
} fvec __attribute__((aligned(16)));;

typedef struct {
  unsigned int x,y,z,w;
} ivec __attribute__((aligned(16)));;

typedef struct {
  fvec iDU;
  fvec iDV;
  fvec iSCALE;
  fvec iMFX;
  fvec iMF02;
  fvec iMF46;
  ivec iRESOLI;
  ivec iRESOLJ;
  fvec iCOLOR;
  ivec iGIFTAGJ;
} PbPart2_Datat __attribute__((aligned(16)));;

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

#define PI 3.142f
#define TWOPI (PI * 2.0f)
void PbPart2_setparam(PbPart2_Datat *data,
	int resoli,int resolj,float scale,
	float *mf)
{
  float resolif = (float) resoli;
  float resoljf = (float) resolj;
//iDU             = 12	;du in each member (TWOPI / (float)resolutioni)
  set_fvec(&data->iDU,(TWOPI/resolif));
//iDV             = 13	;dv in each member (PI / (float)resolutionj)
//  set_fvec(&data->iDV,(PI/resoljf));
  set_fvec(&data->iDV,(PI/resoljf));
//iSCALE          = 14	;scale in all members
  set_fvec(&data->iSCALE,scale);
  data->iSCALE.x *= 2.0f;
  data->iSCALE.z *= 2.0f;
//iMFX            = 15	;(mf_X) should have x=mf[1],y=mf[3],z=mf[5],w=mf[7]
  data->iMFX.x = mf[1];
  data->iMFX.y = mf[3];
  data->iMFX.z = mf[5];
  data->iMFX.w = mf[7];
//iMF02           = 16	;(mf_02) x=mf[0],z=mf[2]
  data->iMF02.x = mf[0];
  data->iMF02.z = mf[2];
//iMF46           = 17	;(mf_46) x=mf[4],z=mf[6]
  data->iMF46.x = mf[4];
  data->iMF46.z = mf[6];
//iRESOLI         = 18	;resolution-i (int)
  set_ivec(&data->iRESOLI,resoli);
//iRESOLJ         = 19    ;resolution-j (int)
  set_ivec(&data->iRESOLJ,resolj);
//iGIFTAGJ        = 20	;gif tag (for j verts)

}

static PbPart2_Datat pPart2Param;

/////////////////////////////////////////////////////////////////////////////////////////
// void* PbPart1_DrawObject
/////////////////////////////////////////////////////////////////////////////////////////
void* PbPart2_DrawObject( PbMatrix* pLocalToScreen,int resolj, void *pChain)
{
  s32  num_coords   = 0;
  u32  num_sections = 0;
  u16  i            = 0;
  int  set_gif = 1;
  u64* p_store = pChain = PbSprAlloc( 10*16 );

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
  // Add parameters  to the list
  *((u64*)pChain)++ = DMA_REF_TAG( (u32)&pPart2Param, 9 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,9,12 );

  ////////////////////////////////////////////////////////////////////////////////////
  // Add GifTag to list

  *((u64*)pChain)++ = DMA_CNT_TAG( 1 );
  *((u32*)pChain)++ = VIF_CODE( VIF_STCYL,0,0x0404 );
  *((u32*)pChain)++ = VIF_CODE( VIF_UNPACK_V4_32,1,21 );

//    GIF_TAG(NLOOP,EOP,PRE,PRIM,FLG,NREG)
  *((u64*)pChain)++ = GIF_TAG( resolj-1, 1, 1, GS_SETREG_PRIM( GS_PRIM_PRIM_POINT,
                               0, 0, 0, 0, 0, 0, 0, 0 ), 0, 2 );
  *((u64*)pChain)++ = 0x51; // registers to set

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
  PbDmaSend01ChainSpr( p_store);

  return NULL;
}
static float mf[] = {8.0f,2.0f,3.0f,2.0f,3.0f,2.0f,3.0f,2.0f};
//                                       4         6
void *SparmGenDot_Render(PbMatrix *pCameraToScreen, PbMatrix *pWorldToCamera,void *pChain, int time)
{
  PbMatrix pWorldToScreen;
  PbMatrix pLocalToScreen;
  PbMatrix pLocalToWorld;
  static float a = 0.0f;
  static float angle = 0.0f;
  static float scale = 100.0f;

  int resoli = 240; 
  int resolj = 112;

  a += 0.2f;
  mf[0] += 0.01f;
  mf[2] += 0.002f;

  mf[4] += 0.0001f;
  mf[6] += 0.0001f;

  scale -=  (scale *0.005f);
  if (scale < 0.01f) scale = 100.0f;
  PbPart2_setparam(&pPart2Param,resoli,resolj,70.0f,mf);

  PbMatrixMultiply(&pWorldToScreen,pWorldToCamera,pCameraToScreen);

  PbMatrixRotateY( &pLocalToWorld, angle);
  angle += 0.01f;

  PbMatrixMultiply(&pLocalToScreen,&pLocalToWorld,&pWorldToScreen);
  PbPart2_DrawObject( &pLocalToScreen, resolj,pChain);

  return NULL;
}

SparmGenDot_SetIntens(float value)
{
  set_fvec(&pPart2Param.iCOLOR,value);
}
