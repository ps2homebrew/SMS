/*
 * PbTexture.c - Texture functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbTexture.h"
#include "PbDma.h"
#include "PbSpr.h"
#include "PbGs.h"
#include "PbVram.h"

///////////////////////////////////////////////////////////////////////////////
// Variables, NOT to be accessed from outside this file (use access functions)
///////////////////////////////////////////////////////////////////////////////

static u32 g_TextureCount = 0;
static u32 g_TexturePtr   = 0;
static PbTexture ga_Textures[PB_MAX_TEXTURES];

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

PbTexture*  PbTextureAlloc( int Width, int Height, int Psm )
{
  PbTexture* pTexture;

  if( g_TextureCount > PB_MAX_TEXTURES )
    g_TextureCount = 0;

  g_TextureCount++;

  pTexture = &ga_Textures[g_TextureCount-1];

  switch( Psm )
  {
    case GS_PSMCT32 : pTexture->Vram = PbVramAlloc( Width*Height ); break;
    case GS_PSMT8   : pTexture->Vram = PbVramAlloc( (Width*Height)/4 ); break;
    case GS_PSMT4   : pTexture->Vram = PbVramAlloc( (Width*Height)/8 ); break;
  }

  pTexture->pMem  = NULL;
  pTexture->pClut = NULL;
  pTexture->x     = Width;
  pTexture->y     = Height;
  pTexture->psm   = Psm;

  // if not texture ptr is set we set it to the first alloced texture ptr

  if( g_TexturePtr == 0 )
    g_TexturePtr = pTexture->Vram;  

  return pTexture;
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload( PbTexture* pTexture )
{
  u64* p_data       = NULL;
  u64* p_store      = NULL;
  char* p_tmap_data = NULL;       
  int n_quads       = 0; 
  int curr_quads    = 0;

  /////////////////////////////////////////////////////////////////////////////
  // Setup registers

  p_data = p_store = PbSprAlloc( 6*16 );

  *p_data++ = DMA_CNT_TAG( 5 );
  *p_data++ = 0;

  *p_data++ = GIF_TAG( 4, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD;

  /////////////////////////////////////////////////////////////////////////////
  // Setup the blitting

  *p_data++ = (u64)( ((pTexture->psm & 0x3f) << 24) | (((pTexture->x>>6) & 0x3f)<<16) | 
                      ((pTexture->Vram/64) & 0x3fff) ) << 32;
  *p_data++ = GS_BITBLTBUF;

  *p_data++ = (unsigned long)( ((0)<<16) | (0) ) << 32;  // x,y offsets 0 for now
  *p_data++ = GS_TRXPOS;

  *p_data++= ((unsigned long)(pTexture->y) << 32) | pTexture->x;
  *p_data++ = GS_TRXREG;

  *p_data++ = 0;
  *p_data++ = GS_TRXDIR;

  /////////////////////////////////////////////////////////////////////////////
  // 

  PbDmaSend02ChainSpr( p_store );


  /////////////////////////////////////////////////////////////////////////////
  // Calculate number of quads

  n_quads = pTexture->x * pTexture->y;  
  n_quads = (n_quads >> 2) + ((n_quads & 0x03 ) != 0 ? 1 : 0);

  p_tmap_data = pTexture->pMem;

  /////////////////////////////////////////////////////////////////////////////
  // send the data

  while( n_quads )
  {
    if( n_quads > PB_IMAGE_MAX_COUNT ) 
       curr_quads = PB_IMAGE_MAX_COUNT;
    else 
      curr_quads = n_quads;

    *p_data++ = DMA_CNT_TAG( 1 );
    *p_data++ = 0; // pad

    *p_data++ = 0x0800000000000000 | curr_quads; // TODO: use proper macro
    *p_data++ = 0; // pad

    *p_data++ = DMA_REF_TAG( (u32)p_tmap_data, curr_quads );
    *p_data++ = 0; // pad

    n_quads     -= curr_quads;
    p_tmap_data += (curr_quads << 4);
  }

  /////////////////////////////////////////////////////////////////////////////
  // end the list

  *p_data++ = DMA_END_TAG( 0 );
  *p_data++ = 0;

  /////////////////////////////////////////////////////////////////////////////
  // And send it

  PbDmaSend02ChainSpr( p_store );
}

