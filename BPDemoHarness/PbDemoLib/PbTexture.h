/*
 * PbTexture.h - Texture functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBTEXTURE_H_
#define _PBTEXTURE_H_

#include <tamtypes.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

#define PB_MAX_TEXTURES    40
#define PB_IMAGE_MAX_COUNT 0x7FFF

///////////////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////////////

typedef struct st_PbTexture
{
  u32   x;
  u32   y;
  u32   psm;
  u32   format;
  
  void* pMem;      // Pointer to texture in memory
  u32*  pClut;     // Pointer to color table
  u32   Vram;      // Offset in Vram
  u32   VramClut;  // Offset for Clut

} PbTexture;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void        PbTextureUpload( PbTexture* pTexture );
PbTexture*  PbTextureCreate32( u32* pData, int Width, int Height );
PbTexture*  PbTextureCreate8( char* pData,u32* pPal, int Width, int Height );
PbTexture*  PbTextureAlloc( int Widht, int Height, int Psm );
u64         PbTextureGetTex0( PbTexture* pTexture );
void        PbTextureCopy( PbTexture* pDest, PbTexture* pSource,int Bilinear );

void 		PbTextureSetRenderTarget( PbTexture* pTexture );

///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload32( PbTexture* pTexture );
void PbTextureUpload8( PbTexture* pTexture );
void PbTextureUpload4( PbTexture* pTexture );

void PbTextureSetupPal( PbTexture* pTexture ); 

#endif // _PBTEXTURE_H_


