/*
 * PbDma.c - Dma functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbDma.h"
#include "PbMisc.h"
#include "PbSpr.h"
#include <tamtypes.h>

static void* gp_StorePtr;

///////////////////////////////////////////////////////////////////////////////
// void PbDmaBeginSpr
// Starts a spr chain to fill with data. it allocs 100*16 bytes which should
// be enough for building small temporary lists.
//////////////////////////////////////////////////////////////////////////////

void* PbDmaBeginSpr()
{
  gp_StorePtr = PbSprAlloc( 100*16 );
  return gp_StorePtr;
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaEndSpr
// Ends the SPR chain, send back the end of the list ptr
//////////////////////////////////////////////////////////////////////////////

int PbDmaEndSpr( void* pData )
{
  int q_size = (u32)pData - (u32)gp_StorePtr;
  return q_size >> 4;
}

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01ChainSpr
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend01ChainSpr( void* pList )
{
  PbDmaWait01();

  *D1_TADR = (u32)pList | 0x80000000;
  *D1_QWC  = 0;               // no size as we send with chains.
  *D1_CHCR = DMA_CHCR( 1,     // Direction
                       1,     // ChainMode
                       0,     // Adress stack pointer
                       1,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01ChainSpr
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend01Chain( void* pList )
{
  PbDmaWait01();

  *D1_TADR = (u32)pList;
  *D1_QWC  = 0;               // no size as we send with chains.
  *D1_CHCR = DMA_CHCR( 1,     // Direction
                       1,     // ChainMode
                       0,     // Adress stack pointer
                       1,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend01Chain
// NOTE: Should be inline, but i dont trust gcc's inline stuff
//////////////////////////////////////////////////////////////////////////////

void PbDmaWait01()
{
  while( *D1_CHCR&0x100 )
  {
    asm __volatile__( "nop;nop;nop;nop;nop;nop;nop;nop;" );
  }
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend02ChainSpr
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend02ChainSpr( void* pList )
{
  PbDmaWait02();
  
  *D2_TADR = (u32)pList | 0x80000000;
  *D2_QWC  = 0;               // no size as we send with chains.
  *D2_CHCR = DMA_CHCR( 1,     // Direction
                       1,     // ChainMode
                       0,     // Adress stack pointer
                       0,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend02ChainSpr
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend02Chain( void* pList )
{
  PbDmaWait02();
  
  *D2_TADR = (u32)pList;
  *D2_QWC  = 0;               // no size as we send with chains.
  *D2_CHCR = DMA_CHCR( 1,     // Direction
                       1,     // ChainMode
                       0,     // Adress stack pointer
                       0,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend02Spr
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend02Spr( void* pList,int Size )
{
  PbDmaWait02();

  *D2_MADR = (u32)pList | 0x80000000;
  *D2_QWC = Size;             // Write size
  *D2_CHCR = DMA_CHCR( 1,     // Direction
                       0,     // ChainMode
                       0,     // Adress stack pointer
                       0,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend02
//////////////////////////////////////////////////////////////////////////////

void PbDmaSend02( void* pList,int Size )
{
  PbDmaWait02();

  *D2_MADR = (u32)pList;
  *D2_QWC = Size;             // Write size
  *D2_CHCR = DMA_CHCR( 1,     // Direction
                       0,     // ChainMode
                       0,     // Adress stack pointer
                       0,     // Transfer DMAtag
                       0,     // No Interrupts
                       1 );   // Start DMA
}

///////////////////////////////////////////////////////////////////////////////
// void PbDmaSend01Chain
// NOTE: Should be inline, but i dont trust gcc's inline stuff
//////////////////////////////////////////////////////////////////////////////

void PbDmaWait02()
{
  while(*D2_CHCR&0x100)
  {
    asm __volatile__( "nop;nop;nop;nop;nop;nop;nop;nop;" );
  }
}


