#include "PbDma.h"
#include <tamtypes.h>

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01Chain
// NOTE: This function doesnt check if the DMA is ready, outside functions
//       need to do that.
//////////////////////////////////////////////////////////////////////////////

void PbDma_Send01Chain( void* pList, int SPR )
{
  // We need to set the topbit if we are sending from SPR
	
	if( SPR == 1 )
		*D1_TADR = (u32)pList | 0x80000000;
  else
		*D1_TADR = (u32)pList;
		
	*D1_QWC  = 0;               // no size as we send with chains.
 	*D1_CHCR = DMA_CHCR( 1,     // Start DMA
                       0,     // No Interrupts
                       1,     // Transfer DMAtag
                       0,     // Adress stack pointer
                       1,     // ChainMode
                       1 );   // Direction
}

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01Chain
// NOTE: Should be inline, but i dont trust gcc's inline stuff
//////////////////////////////////////////////////////////////////////////////

void PbDma_Wait01()
{
	while(*D1_CHCR&0x100);
}

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01Chain
// NOTE: This function doesnt check if the DMA is ready, outside functions
//       need to do that.
//////////////////////////////////////////////////////////////////////////////

void PbDma_Send02Chain( void* pList, int SPR )
{
  // We need to set the topbit if we are sending from SPR
	
	if( SPR == 1 )
		*D2_TADR = (u32)pList | 0x80000000;
  else
		*D2_TADR = (u32)pList;
		
	*D2_QWC  = 0;               // no size as we send with chains.
 	*D2_CHCR = DMA_CHCR( 1,     // Start DMA
                       0,     // No Interrupts
                       0,     // Transfer DMAtag
                       0,     // Adress stack pointer
                       1,     // ChainMode
                       1 );   // Direction
}

///////////////////////////////////////////////////////////////////////////////
// void PbDma_Send01Chain
// NOTE: Should be inline, but i dont trust gcc's inline stuff
//////////////////////////////////////////////////////////////////////////////

void PbDma_Wait02()
{
	while(*D2_CHCR&0x100);
}

