/*
 * PbVif.h - VIF functions/defines for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbVif.h"
#include "PbDma.h"
#include "PbMisc.h"
#include "PbSpr.h"

///////////////////////////////////////////////////////////////////////////////
// void PbVifUploadPrg
///////////////////////////////////////////////////////////////////////////////

void PbVif1UploadPrg( int Dest, void* pStart, void* pEnd )
{
  int   count = 0;
  void* p_chain = NULL;
  void* p_store = NULL;

  p_chain = p_store = PbSprAlloc( 20*16 );

  //////////////////////////////////////////////////////////////////////////////
  // get the size of the code as we can only send 256 instructions in
  // each MPGtag

  count = PbVifGetProgSize( pStart, pEnd ); 

  while( count > 0 )
  {
    u32 current_count = count > 256 ? 256 : count;
    
    *((u64*) p_chain)++ = DMA_REF_TAG( (u32)pStart, current_count/2 );
    *((u32*) p_chain)++ = VIF_CODE( VIF_NOP,0,0 );
    *((u32*) p_chain)++ = VIF_CODE( VIF_MPG,current_count&0xff,Dest );

    pStart += current_count*2;
    count -= current_count; 
    Dest += current_count;
  }

  *((u64*) p_chain)++ = DMA_END_TAG( 0 );
  *((u32*) p_chain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*) p_chain)++ = VIF_CODE( VIF_NOP,0,0 );

  // Send it to vif1

  PbDmaSend01ChainSpr( p_store );
}

///////////////////////////////////////////////////////////////////////////////
// PbVifGetProgSize
///////////////////////////////////////////////////////////////////////////////

int PbVifGetProgSize( u32* pStart, u32* pEnd )
{
  int size = ( pEnd-pStart )/2;

	// if size is odd we have make it even in order for the transfer to work
	// (quadwords, because of that its VERY important to have an extra nop nop
	// at the end of each vuprogram

	if( size&1 )
		size++;

	return size;
}

