#include "PbGfx.h"
#include <tamtypes.h>
#include <stdio.h>
#include "gs.h"
#include "prim.h"
#include "../harness.h"
#include "PbSpr.h"
#include "PbGlobal.h"
#include "PbGs.h"
#include "ps2gs.h"
#include "PbDma.h"

int fb = 0;
u32 color = 0;

///////////////////////////////////////////////////////////////////////////////
// PBGfx_Setup
///////////////////////////////////////////////////////////////////////////////

void PbGfx_Setup()
{
	init_gs();
	set_bg_colour(0x00, 0x00, 0x00);
	set_active_fb(fb);
	clr_scr(color++);

	set_visible_fb(fb);
	fb ^= 1;
	set_active_fb(fb);
}

///////////////////////////////////////////////////////////////////////////////
// PBGfx_Update
///////////////////////////////////////////////////////////////////////////////

void PbGfx_SetActiveScreen()
{
	set_active_fb(fb);
}

///////////////////////////////////////////////////////////////////////////////
// PBGfx_Update
///////////////////////////////////////////////////////////////////////////////

void PbGfx_Update()
{
	wait_vsync();
	set_visible_fb(fb);
	fb ^= 1;
	set_active_fb(fb);
}

///////////////////////////////////////////////////////////////////////////////
// PBGfx_Update
///////////////////////////////////////////////////////////////////////////////

void PbGfx_ClearScreen()
{
	clr_scr( 0/*(127<<16)|127*/ ); 
}

///////////////////////////////////////////////////////////////////////////////
// PBGfx_Update
// Draws a flat alpha blended sprite on top of the other effects, notice
// that this uses context 2 to not interfer with on going vu1 drawing.
// (that is using context 1)
//
///////////////////////////////////////////////////////////////////////////////

void PbGfx_Flash( u32 color )
{
  u64* p_data;

  p_data = PbSpr_Alloc( 7*16, TRUE );

  p_data[0] = GS_GIF_TAG( 1, 0, 0, 0, 1, 6 );
  p_data[1] = GS_AD;

  p_data[2] = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );
  p_data[3] = GS_REG_TEST_2;

  p_data[2] = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );
  p_data[3] = GS_REG_TEST_2;

  p_data[4] = PS2_GS_SETREG_ALPHA(0, 1, 0, 1, 0x80);
  p_data[5] = GS_REG_ALPHA_2;

  p_data[6] = GS_SET_PRIM( GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 1, 0 );
  p_data[7] = GS_REG_PRIM;
  
  p_data[8] = (color<<24)|(255<<16)|(255<<8)|255;
  p_data[9] = GS_REG_RGBAQ;

  p_data[10] = ((1024L + 0) << 4 ) | ( ((1024L + 0) << 4) << 16 );
  p_data[11] = GS_REG_XYZ2;

  p_data[12] = (u64)(( 1024 + SCR_W+1 ) << 4) | ((u64)(( 1024 + SCR_H+1 ) << 4) << 16);
  p_data[13] = GS_REG_XYZ2;

  PbDma_Wait02();  
  PbDma_Send02( p_data, 7, TRUE );
}

