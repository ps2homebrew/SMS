#ifndef _PBTEXTURE_H_
#define _PBTEXTURE_H_

#include <tamtypes.h>

#define IMAGE_MAX_COUNT 0x7FF0

typedef struct st_PbTexture
{
  u16 psm;
  u16 x;
  u16 y;
  u16 format;
  
  void* pMem;      // Pointer to texture in memory
  u32   Vram;      // Offset in Vram

} PbTexture;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

u32  PbTexture_Send( PbTexture* pTexture, u32 Dest );
void PbTexture_SetActive( PbTexture* pTexture,int Context );
void PbTexture_Init();
void PbTexture_SetRenderTarget( PbTexture* pTexture );

void PbTexture_Copy( PbTexture* pDest, PbTexture* pSource );
void PbTexture_SimpleBlur( PbTexture* pDest, PbTexture* pSource, 
                           float u1, float v1, float u2, float v2,int a1, int a2 );
void PbTexture_RadialBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                           float v, int n, int mix1, int mix2 );
void PbTexture_GaussianBlur( PbTexture* pSource,PbTexture* pDest, float cx, float cy, 
                             float v, int n, int mix1, int mix2 );
void PbTexture_Blend( PbTexture* pDest, PbTexture* pSource, int a1, int a2 );

PbTexture* PbTexture_Alloc( void* pMem, u16 x, u16 y, u16 psm );

#endif //_PBTEXTURE_H_
