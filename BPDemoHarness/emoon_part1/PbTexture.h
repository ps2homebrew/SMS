#ifndef _PBTEXTURE_H_
#define _PBTEXTURE_H_

#include <tamtypes.h>

#define IMAGE_MAX_COUNT 0x7FF0

typedef struct st_PbTexture
{
  u32 psm;
  u32 x;
  u32 y;
  u32 format;

} PbTexture;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

u32  PbTexture_Send( PbTexture* pTexture, u32 Dest );
void PbTexture_SetActive( u32 Texture,int Context );
void PbTexture_Init();

#endif //_PBTEXTURE_H_
