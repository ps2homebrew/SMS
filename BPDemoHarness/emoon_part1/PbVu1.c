#include "PbVu1.h"
#include "PbDma.h"
#include "PbVif.h"
#include "PbFile.h"
#include "PbGlobal.h"
#include "../harness.h"

extern const demo_init_t* gp_Info; // YUCK EXTERN!

///////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////

extern u32 g_PbVu1_FlatSimple __attribute__((section(".vudata")));
extern u32 g_PbVu1_FlatSimple_End __attribute__((section(".vudata")));

///////////////////////////////////////////////////////////////////////////////
// PBVu1_Start( int adress )
///////////////////////////////////////////////////////////////////////////////

void PbVu1_Start( int adress )
{

}

///////////////////////////////////////////////////////////////////////////////
// PBVu1_Start( int adress )
///////////////////////////////////////////////////////////////////////////////

void PbVu1_AddToChain( int ProgId,void* pChain,int Adress )
{
  u32* p_start = NULL;
  u32* p_end   = NULL;

  /////////////////////////////////////////////////////////////////////////////
  // finc the correct program

  switch( ProgId )
  {
    case VU1M_TS_SIMPLE : 
    {
      p_start = &g_PbVu1_FlatSimple;
      p_end   = &g_PbVu1_FlatSimple_End;
      break;
    }
  }

  if( p_start == NULL )
    return;

  //////////////////////////////////////////////////////////////////////////////
  // get the size of the code as we can only send 256 instructions in
  // each MPGtag

  int count = PbVu1_GetSize( p_start, p_end ); 

  gp_Info->printf( "Code Size: %d\n", count );
  gp_Info->printf( "tag: 0x%x\n", &g_PbVu1_FlatSimple );

  while( count > 0 )
  {
    u32 current_count = count > 256 ? 256 : count;
    
    *((u64*) pChain)++ = DMA_REF_TAG( (u32)p_start, current_count/2 );
    *((u32*) pChain)++ = VIF_CODE( VIF_NOP,0,0 );
    *((u32*) pChain)++ = VIF_CODE( VIF_MPG,current_count&0xff,0 );

		p_start += count*2;
    count -= 256; 
    Adress += 256;
  }


  *((u64*) pChain)++ = DMA_END_TAG( 0 );
  *((u32*) pChain)++ = VIF_CODE( VIF_NOP,0,0 );
  *((u32*) pChain)++ = VIF_CODE( VIF_NOP,0,0 );
}

///////////////////////////////////////////////////////////////////////////////
// PbVu1_Wait
// NOTE: Waits for the vu1 to be done, should ONLY be used when really needed
//       for example when doing debug outputs
///////////////////////////////////////////////////////////////////////////////

void PbVu1_Wait()
{
	asm __volatile__(
		"nop\n"
		"nop\n"
"0:\n"
		"bc2t 0b\n"
		"nop\n"
		"nop\n" );
}

///////////////////////////////////////////////////////////////////////////////
// PBVu1_Start( int adress )
///////////////////////////////////////////////////////////////////////////////

int PbVu1_GetSize( u32* pStart, u32* pEnd  )
{
  int size = ( pEnd-pStart )/2;

	// if size is odd we have make it even in order for the transfer to work
	// (quadwords, because of that its VERY important to have an extra nop nop
	// at the end of each vuprogram

	if( size&1 )
		size++;

  out( "Vu1 progsize: %d\n", size );

	return size;
}

///////////////////////////////////////////////////////////////////////////////
// PBVu1_Start( int adress )
///////////////////////////////////////////////////////////////////////////////

void PbVu1_DumpMem()
{
  PbFile_Write( "host:VU1MICROMEM",((void*)0x11008000),1024*16 );
  PbFile_Write( "host:VU1MEM", ((void*)0x1100c000),1024*16 );
}

///////////////////////////////////////////////////////////////////////////////
// PBVu1_Start( int adress )
///////////////////////////////////////////////////////////////////////////////

void PbVu1_ShowStats()
{
	out( "VIF1_STAT:  %s\n", PbGlobal_GetAsBits32( *VIF1_STAT ) );
	out( "VIF1_FBRST: %s\n", PbGlobal_GetAsBits32( *VIF1_FBRST ) );
	out( "VIF1_ERR:   %s\n", PbGlobal_GetAsBits32( *VIF1_ERR ) );
	out( "VIF1_MARK:  %s\n", PbGlobal_GetAsBits32( *VIF1_MARK ) );
}

