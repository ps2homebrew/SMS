#include "PbGs.h"
#include "PbGlobal.h"
#include "PbSpr.h"
#include "PbDma.h"

///////////////////////////////////////////////////////////////////////////////
// void PbGs_SetZbufferTest
///////////////////////////////////////////////////////////////////////////////

void PbGs_SetZbufferTest( int Mode, int Context )
{
  u64* p_data = PbSpr_Alloc( 2*16, TRUE );

  p_data[0] = GS_GIF_TAG( 1, 0, 0, 0, 1, 1 );
  p_data[1] = GS_AD;
  
  p_data[2] = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, Mode );
  p_data[3] = GS_REG_TEST_1+Context;

  PbDma_Wait02();
  PbDma_Send02( p_data, 2, 1 ); 
}

///////////////////////////////////////////////////////////////////////////////
// PbGs_ShowStats()
///////////////////////////////////////////////////////////////////////////////

void PbGs_ShowStats()
{
	out( "GIF_CTRL:  %s\n", PbGlobal_GetAsBits32( *GIF_CTRL ) );
	out( "GIF_MODE:  %s\n", PbGlobal_GetAsBits32( *GIF_MODE ) );
	out( "GIF_STAT:  %s\n", PbGlobal_GetAsBits32( *GIF_STAT ) );
	out( "GIF_TAG0:  %s\n", PbGlobal_GetAsBits32( *GIF_TAG0 ) );
	out( "GIF_TAG1:  %s\n", PbGlobal_GetAsBits32( *GIF_TAG1 ) );
	out( "GIF_TAG2:  %s\n", PbGlobal_GetAsBits32( *GIF_TAG2 ) );
	out( "GIF_TAG3:  %s\n", PbGlobal_GetAsBits32( *GIF_TAG3 ) );
	out( "GIF_CNT:   %s\n", PbGlobal_GetAsBits32( *GIF_CNT ) );
	out( "GIF_P3CNT: %s\n", PbGlobal_GetAsBits32( *GIF_P3CNT ) );
}

