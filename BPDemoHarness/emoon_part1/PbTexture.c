#include <tamtypes.h>
#include <kernel.h>
#include "shapes.h"
#include "PbTexture.h"
#include "PbGs.h"
#include "PbDma.h"
#include "PbGlobal.h"
#include "PbSpr.h"
#include "ps2gs.h"
#include "gs.h"
#include "PbGs.h"
#include "gif.h"
#include "dma.h"
#include "math.h"
#include "vram_malloc.h"

#define MAX_TEXTURES 40

DECLARE_EXTERN_GS_PACKET(dma_buf);
extern u16 gs_texture_wh(u16 n);

static PbTexture ga_Textures[MAX_TEXTURES];
static int       g_Texture = 0;

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Init
// Envmap, 
///////////////////////////////////////////////////////////////////////////////

void PbTexture_SetActive( PbTexture* pTexture,int Context )
{
  BEGIN_GS_PACKET(dma_buf);

  GIF_TAG_AD(dma_buf, 1, 1, 0, 0, 0);

//  GIF_DATA_AD(dma_buf, PS2_GS_TEXFLUSH, 0x42);
  
  GIF_DATA_AD(dma_buf, PS2_GS_TEX0_1,
	      PS2_GS_SETREG_TEX0_1(
				   pTexture->Vram / 64,	// base pointer
				   pTexture->x  / 64,	// width
				   0,					// 32bit RGBA
				   gs_texture_wh(pTexture->x),	// width
				   gs_texture_wh(pTexture->y),	// height
				   1,					// RGBA
				   PS2_GS_TEX_TFX_DECAL,		       
				   0,0,0,0,0));

  PbDma_Wait02();
  SEND_GS_PACKET(dma_buf);
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Send
// Send a texture over to gs-ram, builds temporary list in SPR
///////////////////////////////////////////////////////////////////////////////

u32 PbTexture_Send( PbTexture* pTexture, u32 Dest )
{
  load_image( pTexture->pMem, pTexture->Vram, pTexture->x, 0, 0, 0, 
              pTexture->x, pTexture->y );

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// void PbTexture_SetRenderTarget
///////////////////////////////////////////////////////////////////////////////

void PbTexture_SetRenderTarget( PbTexture* pTexture )
{
  u64* p_data = NULL;
  u64* p_store = NULL;

  p_data = p_store = PbSpr_Alloc( 3*16, TRUE );

  *p_data++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 2 );
  *p_data++ = GS_AD;

  *p_data++ = GS_SETREG_FRAME_1( pTexture->Vram / 2048, pTexture->x / 64, 0, 0 );
  *p_data++ = GS_REG_FRAME_1;

  *p_data++ = PS2_GS_SETREG_SCISSOR_1(0, pTexture->x - 2, 0, pTexture->y - 2 );
  *p_data++ = PS2_GS_SCISSOR_1;

  PbDma_Wait02();
  PbDma_Send02( p_store, 3, TRUE );
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Copy
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

void PbTexture_Copy( PbTexture* pDest, PbTexture* pSource )
{
  u64* p_data = NULL;
  u64* p_store = NULL;

  PbTexture_SetRenderTarget( pDest );

  rect r;

  r.col = 0;
  r.v[0].col = 0;
  r.v[0].lit = 255;
  r.v[0].u   = 1;
  r.v[0].v   = 1;
  r.v[0].x   = 0;
  r.v[0].y   = 0;
  r.v[0].z   = 0;

  r.col = 0;
  r.v[1].col = 0;
  r.v[1].lit = 255;
  r.v[1].u   = pSource->x-1;
  r.v[1].v   = pSource->y-1;
  r.v[1].x   = pDest->x;
  r.v[1].y   = pDest->y;
  
  fill_rect_tex( r, pSource->Vram, pSource->x, pSource->x );
}

static float count = 0.002f;

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Copy
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

void PbTexture_SimpleBlur( PbTexture* pDest, PbTexture* pSource, 
                           float u1, float v1, float u2, float v2,int a1, int a2 )
{
  u64* p_data = NULL;
  u64* p_store = NULL;
  
  u1 += 0.5f;
  v1 += 0.5f;
  u2 += 0.5f;
  v2 += 0.5f;

  u1 *= 16.0f;
  v1 *= 16.0f;
  u2 *= 16.0f;
  v2 *= 16.0f;

//  v1 *= 4.0f;
//  v2 *= 4.0f;

  PbTexture_SetRenderTarget( pDest );

  p_data = p_store = PbSpr_Alloc( 11*16, TRUE );

  *p_data++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 10 );
  *p_data++ = GS_AD;
  
  *p_data++ = 0x42;
  *p_data++ = GS_REG_TEXFLUSH;

  *p_data++ = GS_SET_TEX0( pSource->Vram/64, pSource->x/64, 0, 8, 8, 1, 
                           PS2_GS_TEX_TFX_MODULATE, 0, 0, 0, 0, 0 );
  *p_data++ = GS_REG_TEX0_1;

  *p_data++ = PS2_GS_SETREG_TEX1_1(0, 0, 1, 0, 0, 0, 0);
  *p_data++ = PS2_GS_TEX1_1;

  //*p_data++ = PS2_GS_SETREG_CLAMP( 2, 2, 0, 256, 0, 256 );
  //*p_data++ = PS2_GS_CLAMP_1;

  *p_data++ = PS2_GS_SETREG_PRIM( PS2_GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0);
  *p_data++ = PS2_GS_PRIM;

  // Alpha

  *p_data++ = 0x0000000029+(((u64)a2)<<32);
  *p_data++ = PS2_GS_ALPHA_1;

  // Color

  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = a1;
  *((unsigned char*)p_data)++ = 0;
  *((float*)p_data)++ = 256.0f;
  *p_data++ = PS2_GS_RGBAQ;

  *p_data++ = PS2_GS_SETREG_UV( (int)u1, (int)v1 );
  *p_data++ = PS2_GS_UV;
  
  *p_data++ = PS2_GS_SETREG_XYZ2( ( 0 + 1024 ) << 4, ( 0 + 1024 ) << 4, 0 );
  *p_data++ = PS2_GS_XYZ2;

  *p_data++ = PS2_GS_SETREG_UV( (int)u2, (int)v2 );
  *p_data++ = PS2_GS_UV;

  *p_data++ = PS2_GS_SETREG_XYZ2( ( 256 + 1024 ) << 4, ( 256 + 1024 ) << 4, 0 );
  *p_data++ = PS2_GS_XYZ2;

  PbDma_Wait02();
  PbDma_Send02( p_store, 11, TRUE );
}


///////////////////////////////////////////////////////////////////////////////
// PbTexture_RadialBlur
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

void PbTexture_RadialBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                           float v, int n, int mix1, int mix2 )
{
  float scale=v;
  int t;

  // Based on aura for laura code.
  
  for( t = 0; t < n; t++ )
  {
    float x1,y1,x2,y2;
      
    x1=scale*(-cx);
    y1=scale*(-cy);
    x2=scale*(pSource->x-cx);
    y2=scale*(pSource->y-cy);
    PbTexture_SimpleBlur( pDest, pSource, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    scale=PbSqrt(scale)*PbSqrt(PbSqrt(scale));
      
    x1=scale*(-cx);
    y1=scale*(-cy);
    x2=scale*(pSource->x-cx);
    y2=scale*(pSource->y-cy);
    PbTexture_SimpleBlur( pSource, pDest, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    scale=PbSqrt(scale)*PbSqrt(PbSqrt(scale));
  }
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_RadialBlur
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

void PbTexture_Blend( PbTexture* pDest, PbTexture* pSource, int a1, int a2 )
{
  u64* p_data = NULL;
  u64* p_store = NULL;

  PbTexture_SetRenderTarget( pDest );

  p_data = p_store = PbSpr_Alloc( 11*16, TRUE );

  *p_data++ = GS_GIF_TAG( 1, 0, 0, 0, 1, 10 );
  *p_data++ = GS_AD;
  
  *p_data++ = 0x42;
  *p_data++ = GS_REG_TEXFLUSH;

  *p_data++ = GS_SET_TEX0( pSource->Vram/64, pSource->x/64, 0, 8, 8, 0, 
                           PS2_GS_TEX_TFX_HIGHLIGHT2, 0, 0, 0, 0, 0 );
  *p_data++ = GS_REG_TEX0_1;

  *p_data++ = PS2_GS_SETREG_TEX1_1(0, 0, 1, 0, 0, 0, 0);
  *p_data++ = PS2_GS_TEX1_1;

  *p_data++ = PS2_GS_SETREG_PRIM( PS2_GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0);
  *p_data++ = PS2_GS_PRIM;

  // Alpha

  *p_data++ = 0x0000000029+(((u64)a2)<<31);
  *p_data++ = PS2_GS_ALPHA_1;

  // Color

  *((unsigned char*)p_data)++ = a1/2;
  *((unsigned char*)p_data)++ = a1/2;
  *((unsigned char*)p_data)++ = a1/2;
  *((unsigned char*)p_data)++ = 0;
  *((float*)p_data)++ = 1.0;
  *p_data++ = PS2_GS_RGBAQ;

  *p_data++ = PS2_GS_SETREG_UV( (int)0, (int)0 );
  *p_data++ = PS2_GS_UV;
  
  *p_data++ = PS2_GS_SETREG_XYZ2( ( 0 + 1024 ) << 4, ( 0 + 1024 ) << 4, 0 );
  *p_data++ = PS2_GS_XYZ2;

  *p_data++ = PS2_GS_SETREG_UV( (int)pSource->x<<4, (int)pSource->y<<4 );
  *p_data++ = PS2_GS_UV;

  *p_data++ = PS2_GS_SETREG_XYZ2( ( 256 + 1024 ) << 4, ( 256 + 1024 ) << 4, 0 );
  *p_data++ = PS2_GS_XYZ2;

  PbDma_Wait02();
  PbDma_Send02( p_store, 11, TRUE );
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_RadialBlur
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

void PbTexture_GaussianBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                             float v, int n, int mix1, int mix2 )
{
  float scale=v;
  int t;

  static int printed = FALSE;

  // source og dest skal have ens image data
  // passende værdi for v er 0.5
  // mix1=mix2=64 giver blur uden forøgelse af styrke
  
  float offset_x = 0.0f;
  float offset_y = 0.0f;

  for( t = 0; t < 16; t++ )
  {
    float x1,y1,x2,y2;
      
    x1 = (-cx) + offset_x;
    y1 = (-cy) + offset_y;
    x2 = (pSource->x-cx) + offset_x;
    y2 = (pSource->y-cy) + offset_y;
    PbTexture_SimpleBlur( pDest, pSource, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );
/*
    if( printed == FALSE )
    {
      gp_Info->printf( "cx+x1: %f\n", cx+x1 );
      gp_Info->printf( "cy+y1: %f\n", cy+y1 );
      gp_Info->printf( "cx+x2: %f\n", cx+x2 );
      gp_Info->printf( "cy+y2: %f\n", cy+y2 );
    }
*/      
    x1 = (-cx) + offset_x;
    y1 = (-cy) - offset_y;
    x2 = (pSource->x-cx) + offset_x;
    y2 = (pSource->y-cy) - offset_y;
    PbTexture_SimpleBlur( pSource, pDest, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    x1 = (-cx) - offset_x;
    y1 = (-cy) - offset_y;
    x2 = (pSource->x-cx) - offset_x;
    y2 = (pSource->y-cy) - offset_y;
    PbTexture_SimpleBlur( pDest, pSource, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    x1 = (-cx) - offset_x;
    y1 = (-cy) + offset_y;
    x2 = (pSource->x-cx) - offset_x;
    y2 = (pSource->y-cy) + offset_y;
    PbTexture_SimpleBlur( pSource, pDest, cx+x1,cy+y1,cx+x2,cy+y2,mix1,mix2 );

    //offset_x += 1.4f;

    offset_x += 0.50f;
    offset_y += 0.50f;
  }

  printed = TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// PbTexture_Alloc
// Allocates a texture from the availible slots (will autowrap)
///////////////////////////////////////////////////////////////////////////////

PbTexture* PbTexture_Alloc( void* pMem, u16 x, u16 y, u16 psm )
{
  PbTexture* pTexture;

  if( g_Texture > MAX_TEXTURES )
    g_Texture = 0;

  g_Texture++;

  pTexture = &ga_Textures[g_Texture-1];

  // Init texture

  pTexture->Vram = vram_malloc( x*y );  // 32bits 
  pTexture->pMem = pMem;
  pTexture->x    = x;
  pTexture->y    = y;
  pTexture->psm  = psm;

  return pTexture;
}

