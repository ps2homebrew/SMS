#include "PbGs.h"

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

