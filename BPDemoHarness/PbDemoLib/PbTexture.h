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
  u16 x;
  u16 y;
  char psm;
  char format;
  
  void* pMem;      // Pointer to texture in memory
  void* pClut;     // Pointer to color table
  u32   Vram;      // Offset in Vram

} PbTexture;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////


void        PbTextureUpload( PbTexture* pTexture );
PbTexture*  PbTextureCreate32( u32* pData, int Width, int Height );
PbTexture*  PbTextureAlloc( int Widht, int Height, int Psm );

u64         PbTextureGetTex0( PbTexture* pTexture );

///////////////////////////////////////////////////////////////////////////////
// Internal functions
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload32( PbTexture* pTexture );
void PbTextureUpload8( PbTexture* pTexture );
void PbTextureUpload4( PbTexture* pTexture );

#endif // _PBTEXTURE_H_


