/*
 * PbScreen.h - Screen handling functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <tamtypes.h>
#include "PbScreen.h"
#include "PbGs.h"
#include "PbVram.h"
#include "PbDma.h"
#include "PbPrim.h"
#include "PbSpr.h"

///////////////////////////////////////////////////////////////////////////////
// Variables, NOT to be accessed from outside this file (use access functions)
///////////////////////////////////////////////////////////////////////////////

static u32 g_ScreenBuffer[2];
static u32 g_Zbuffer;
static int g_OffsetX = 2048;
static int g_OffsetY = 2048;
static int g_Width;
static int g_Height;
static int g_Psm;
static int g_ActiveBuffer = 1;

///////////////////////////////////////////////////////////////////////////////
// void PbScreenSetup()
///////////////////////////////////////////////////////////////////////////////

void PbScreenSetup( int Width, int Height, int PSM )
{
  u64* p_data;
  u64* p_store;

  g_Width   = Width;
  g_Height  = Height;
  g_Psm     = PSM;
  g_OffsetX = 2048;
  g_OffsetY = 2048;

  GS_SET_PMODE(
	       0,		// ReadCircuit1 OFF 
	       1,		// ReadCircuit2 ON
	       0,		// Use ALP register for Alpha Blending
	       1,		// Alpha Value of ReadCircuit2 for output selection
	       0,		// Blend Alpha with the output of ReadCircuit2
	       0xFF	// Alpha Value = 1.0
	       );
  
  GS_SET_DISPFB2(
		 0,				    // Frame Buffer base pointer = 0 (Address/2048)
		 Width / 64,	// Buffer Width (Address/64)
		 PSM,			    // Pixel Storage Format
		 0,				    // Upper Left X in Buffer = 0
		 0				    // Upper Left Y in Buffer = 0
		 );
  
  GS_SET_BGCOLOR(
		 0,	// RED
		 0,	// GREEN
		 0	// BLUE
		 );

  ////////////////////////////////////////////////////////////////////////////
  // Alloc data for the screens and zbuffer

  g_ScreenBuffer[0] = PbVramAlloc( 256*640*4 );  
  g_ScreenBuffer[1] = PbVramAlloc( 256*640*4 );  
  g_Zbuffer         = PbVramAlloc( 256*640*4 );  
  p_data = p_store  = PbSprAlloc( (8+5)*16 );

  ////////////////////////////////////////////////////////////////////////////
  // Setup diffrent default values for the registers

  *p_data++ = GIF_TAG( 7+5, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  *p_data++ = 1;  
  *p_data++ = GS_PRMODECONT,    

  // Setup for context 1

  *p_data++ = GS_SETREG_FRAME_1( 0, Width / 64, PSM, 0 );  
  *p_data++ = GS_FRAME_1,    

  *p_data++ = GS_SETREG_XYOFFSET_1( g_OffsetX << 4, g_OffsetY << 4 );
  *p_data++ = GS_XYOFFSET_1, 

  *p_data++ = GS_SETREG_SCISSOR_1( 0, Width-1, 0, Height-1 ); 
  *p_data++ = GS_SCISSOR_1,  

  *p_data++ = GS_SETREG_ZBUF_1( g_Zbuffer / 8192, 0, 0 );   
  *p_data++ = GS_ZBUF_1,     

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
  *p_data++ = GS_TEST_1,     

  *p_data++ = GS_SETREG_COLCLAMP( 255 );                    
  *p_data++ = GS_COLCLAMP,   

  // Setup for context 2

  *p_data++ = GS_SETREG_FRAME_1( 0, Width / 64, PSM, 0 );  
  *p_data++ = GS_FRAME_2,    

  *p_data++ = GS_SETREG_XYOFFSET_1( g_OffsetX << 4, g_OffsetY << 4 );
  *p_data++ = GS_XYOFFSET_2, 

  *p_data++ = GS_SETREG_SCISSOR_1( 0, Width-1, 0, Height-1 ); 
  *p_data++ = GS_SCISSOR_2,  

  *p_data++ = GS_SETREG_ZBUF_1( g_Zbuffer / 8192, 0, 0 );   
  *p_data++ = GS_ZBUF_2,     

  *p_data++ = GS_SETREG_TEST( 1, 7, 0xFF, 0, 0, 0, 1, 1 );     
  *p_data++ = GS_TEST_2,     
  
  ////////////////////////////////////////////////////////////////////////////
  // Send the data to gif
  
  PbDmaSend02Spr( p_store, 8+5 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenSyncFlip()
///////////////////////////////////////////////////////////////////////////////

void PbScreenSyncFlip()
{
  PbScreenVsync();

  // set visible buffer

  GS_SET_DISPFB2( g_ScreenBuffer[g_ActiveBuffer & 1] / 8192, 
                  g_Width / 64, g_Psm, 0, 0 );

  g_ActiveBuffer ^= 1;

  // set active buffer 

  PbScreenSetActive( g_ActiveBuffer );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenSetActive
///////////////////////////////////////////////////////////////////////////////

void PbScreenSetActive( int Buffer )
{
  u64* p_data;
  u64* p_store;

  p_data = p_store = PbSprAlloc( (3+2)*16 );

  *p_data++ = GIF_TAG( 2+2, 1, 0, 0, 0, 1 );
  *p_data++ = GIF_AD;

  // context 1

  *p_data++ = GS_SETREG_SCISSOR_1( 0, g_Width-1, 0, g_Height - 1 ); 
  *p_data++ = GS_SCISSOR_1;

  *p_data++ = GS_SETREG_FRAME_1( g_ScreenBuffer[Buffer & 1] / 8192, 
                                 g_Width / 64, g_Psm, 0 );  
  *p_data++ = GS_FRAME_1; 

  // context 2

  *p_data++ = GS_SETREG_SCISSOR_1( 0, g_Width-1, 0, g_Height - 1 ); 
  *p_data++ = GS_SCISSOR_2;

  *p_data++ = GS_SETREG_FRAME_1( g_ScreenBuffer[Buffer & 1] / 8192, 
                                 g_Width / 64, g_Psm, 0 );  
  *p_data++ = GS_FRAME_2; 
  
  ////////////////////////////////////////////////////////////////////////////
  // Send the data to gif
  
  PbDmaSend02Spr( p_store, 3+2 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenSetCurrentActive
///////////////////////////////////////////////////////////////////////////////

void PbScreenSetCurrentActive()
{
	 PbScreenSetActive( g_ActiveBuffer );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenVsync()
///////////////////////////////////////////////////////////////////////////////

void PbScreenVsync()
{
  *CSR = *CSR & 8;
  while(!(*CSR & 8));
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenGetOffsetX()
///////////////////////////////////////////////////////////////////////////////

void PbScreenClear( int Color )
{
  PbPrimSpriteNoZtest( 0, 0, g_Width<<4, g_Height<<4, 0, Color );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenDisplayAt
///////////////////////////////////////////////////////////////////////////////

void PbScreenDisplayAt( u32 Pos, int Width )
{
  GS_SET_DISPFB2( Pos / 8192, Width / 64, 0, 0, 0 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenGetOffsetX()
///////////////////////////////////////////////////////////////////////////////

int PbScreenGetOffsetX()
{
  return g_OffsetX;
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenGetOffsetY()
///////////////////////////////////////////////////////////////////////////////

int PbScreenGetOffsetY()
{
  return g_OffsetY;
}

///////////////////////////////////////////////////////////////////////////////
// void PbScreenGetOffsetY()
///////////////////////////////////////////////////////////////////////////////

int PbScreenGetActive()
{
  return g_ActiveBuffer;
}

