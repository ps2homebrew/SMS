/*
 * PbParticle.c - EE-only (so far) Particle functions for Pb demo library
 *
 * Copyright (c) 2004   jar <dsl123588@vip.cybercity.dk>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com> 
 */

#include <tamtypes.h>
#include <kernel.h>

#include "PbPrim.h"
#include "PbGs.h"
#include "PbDma.h"
#include "PbVec.h"
#include "PbMatrix.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

#define MIN(a,b) ((a)<(b) ? (a) : (b))

#define CACHE_SIZE 2048
#define HALF_CACHE_SIZE_IN_UNITS (CACHE_SIZE*5*2 + 2)


///////////////////////////////////////////////////////////////////////////////
// Private data
///////////////////////////////////////////////////////////////////////////////

static float      PbParticleMaxSize = 28.0f *16.0f;
static float      PbParticleDistanceFalloff = 3;
static PbMatrix   PbLocalToScreen;
static PbTexture* PbParticleTexture;
static u64*       PbParticleCachePtr;
static u64	      PbParticleCache[HALF_CACHE_SIZE_IN_UNITS*2] __attribute__((aligned(16)));
static int		  PbParticleCacheFlag = 0;

static PbIvec     PbParticleScreenPos[CACHE_SIZE];
static PbIvec     PbParticleColor = { 0x80, 0x80, 0x80, 0x80 };
static PbIvec     PbParticleUv0   = { 0, 0, 0, 0 };
static PbIvec     PbParticleUv1   = { 0, 0, 0, 0 };


///////////////////////////////////////////////////////////////////////////////
// Private functions
///////////////////////////////////////////////////////////////////////////////

static inline u64* PbParticleCacheGetBase()
{
	return &PbParticleCache[PbParticleCacheFlag*HALF_CACHE_SIZE_IN_UNITS];
}

static void PbParticleCacheAlloc()
{   
  PbParticleCacheFlag ^= 1;
  PbParticleCachePtr = PbParticleCacheGetBase();
  *PbParticleCachePtr++ = GIF_TAG(0, 0, 1, GS_SETREG_PRIM(GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0), 0, 5); 
  *PbParticleCachePtr++ = GS_RGBAQ | (GS_UV<<4) | (GS_XYZ2<<8) | (GS_UV<<12) | (GS_XYZ2<<16);	
}

static int PbParticleCacheGetCount()
{
  return ((u32)PbParticleCachePtr - (u32)PbParticleCacheGetBase()) / 16;
}

static void PbParticleCacheFlush()
{
  int length= PbParticleCacheGetCount();
  if(length > 1)
  {
  	PbParticleCacheGetBase()[0] |= (length - 1) / 5; // divide by NREGS
  	FlushCache(0);
  	PbDmaWait02();
    PbDmaSend02( PbParticleCacheGetBase(), length );    
  }
}

static inline float PbParticleGetSize(float z)
{
  z *= 1.0f / 5536.0f; // 5536.0f is zfar (should be parameter?)
  return PbParticleMaxSize - MIN(PbParticleMaxSize, z * PbParticleDistanceFalloff);
}

static void PbParticleDrawBatch(PbFvec* pPoints, int iNumPoints)
{
  int i;
  if(iNumPoints > CACHE_SIZE)
  {
    printf("iNumPoints > CACHE_SIZE in PbParticleDrawBatch.\n");
    return;
  }
  PbParticleCacheAlloc();

  // transform and project particles

 __asm__ ( 
  "move		       $8, %0                       \n"
  "move		       $9, %1                       \n"
  "move		       $10, %2                      \n"
  "tranform_loop:                               \n"
  "lqc2            vf05,0x00($9)                \n" 
  "vmulax.xyzw     ACC,vf01,vf05                \n" 
  "vmadday.xyzw    ACC,vf02,vf05                \n" 
  "vmaddaz.xyzw    ACC,vf03,vf05                \n" 
  "vmaddw.xyzw     vf06,vf04,vf05               \n" 
  "vdiv            Q,vf0w,vf06w                 \n" 
  "vwaitq                                       \n" 
  "vmulq.xyz       vf06,vf06,Q                  \n" 
  "vftoi4.xyzw     vf07,vf06                    \n" 
  "sqc2            vf07,0x00($8)                \n" 
  "addiu 		   $8, 16                       \n"
  "addiu 		   $9, 16                       \n"
  ".set	push                                    \n"
  ".set	noreorder                               \n"  
  "addiu		   $10, -1                      \n"
  "bnez		       $10, tranform_loop           \n"
  "nop                                          \n"
  "nop                                          \n"
  ".set	pop                                     \n"
  : : "r"(&PbParticleScreenPos[0]), "r"(&pPoints[0]), "r"(iNumPoints) : "memory"    );  	

  // add them to the giflist, PbParticleSetup adds texture coordinates and color

  for(i=0; i<iNumPoints; ++i)
  {
    PbIvec* t = &PbParticleScreenPos[i];    
    int size = PbParticleGetSize(t->z/16.0f);

    //*((PbIvec*)PbParticleCachePtr)++ = PbParticleColor;
    //*((PbIvec*)PbParticleCachePtr)++ = PbParticleUv0;
    PbParticleCachePtr+=4;
    
    t->x -= size;
    t->y -= size;
    *((PbIvec*)PbParticleCachePtr)++ = *t; 
    
    //*((PbIvec*)PbParticleCachePtr)++ = PbParticleUv1; 
    PbParticleCachePtr +=2;
    
    t->x += size<<1;
    t->y += size<<1;
    *((PbIvec*)PbParticleCachePtr)++ = *t;    
  }	
  PbParticleCacheFlush();
}

///////////////////////////////////////////////////////////////////////////////
// Public functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// void PbParticleSetup
// Copies the paramters to the private data variables and fills the cache
// with color and UV coordinates. 
//////////////////////////////////////////////////////////////////////////////

void PbParticleSetup(PbMatrix* pLocalToScreen, PbTexture* pTex, float fParticleMaxSize, float fParticleDistanceFalloff, int iColor)
{
  int i;
  PbIvec* cache;
  
  for(i=0;i<16;++i)
    PbLocalToScreen.m_fMatrix[i] = pLocalToScreen->m_fMatrix[i];
	
  PbParticleTexture = pTex;
  PbParticleMaxSize = fParticleMaxSize;
  PbParticleDistanceFalloff = fParticleDistanceFalloff;
  PbParticleUv1.x = (pTex->x-1) << 4;
  PbParticleUv1.y = (pTex->y-1) << 4;
  
  PbParticleColor.x = iColor & 0xff; // r
  PbParticleColor.y = (iColor>>8) & 0xff; // g
  PbParticleColor.z = (iColor>>16) & 0xff; // b
  PbParticleColor.w = (iColor>>24) & 0xff; // a
  
  cache = (PbIvec*)(&PbParticleCache[2]); // leave room for giftag
  for(i=0;i<CACHE_SIZE;++i)
  {
  	*cache++ = PbParticleColor;
  	*cache++ = PbParticleUv0;
  	cache++; // first coord
  	*cache++ = PbParticleUv1;
  	cache++; // second coord
  }
  cache = (PbIvec*)(&PbParticleCache[HALF_CACHE_SIZE_IN_UNITS+2]); // leave room for giftag
  for(i=0;i<CACHE_SIZE;++i)
  {
  	*cache++ = PbParticleColor;
  	*cache++ = PbParticleUv0;
  	cache++; // first coord
  	*cache++ = PbParticleUv1;
  	cache++; // second coord
  }
}

///////////////////////////////////////////////////////////////////////////////
// void PbParticleDraw
// Draws the iNumPoints particles stored in pPoints.
// PbParticleSetup *MUST* be called before this function.
//////////////////////////////////////////////////////////////////////////////

void PbParticleDraw(PbFvec* pPoints, int iNumPoints)
{
  // Place the matrix in vf01-vf04
  __asm__ (
  "lqc2			vf01,0x00(%0)\n"
  "lqc2			vf02,0x10(%0)\n"
  "lqc2			vf03,0x20(%0)\n"
  "lqc2			vf04,0x30(%0)\n"
  : : "r"(&PbLocalToScreen)	);

  // Prepare the GS
  PbGifListBegin();
  PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 ) ); // Disable z-buffering
  PbGifListAdd( GS_TEX0_1, PbTextureGetTex0( PbParticleTexture ) );	
  PbGifListAdd( GS_TEX1_1, 0x20 ); // Enable bilinear filtering
  PbGifListAdd( GS_PABE, 0 );	
  PbGifListAdd( GS_ALPHA_1, GS_SETREG_ALPHA(0,2,2,1,0x80));	// Additive, without respect to alpha
  PbGifListSend();
  
  while(iNumPoints > 0)
  {
  	int this_batch = MIN(CACHE_SIZE, iNumPoints);
    PbParticleDrawBatch(pPoints,this_batch);
    pPoints += this_batch;
    iNumPoints -= this_batch;
  }
  
  PbGifListBegin();
  PbGifListAdd( GS_TEST_1, GS_SETREG_TEST( 2, 7, 0xFF, 0, 0, 0, 1, 1 ) ); // Re-enable z-buffering
  PbGifListSend();  
}