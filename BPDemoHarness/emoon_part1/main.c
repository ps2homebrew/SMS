#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdarg.h>
#include <string.h>
#include <loadfile.h>
#include "../harness.h"
#include "PbGfx.h"
#include "PbDma.h"
#include "PbVif.h"
#include "PbVu1.h"
#include "PbMatrix.h"
#include "PbPart1.h"
#include "PbGs.h"
#include "dma.h"
#include "gif.h"
#include "ps2gs.h"

const demo_init_t* gp_Info;

//static int dma_chain[1024];
extern int offs_x, offs_y; /* Offset of framebuffer to display */
extern u64 dma_buf[4096*2+2]; /*  Declare a 50 element dma buffer */
extern int dma_buf_cur;
extern int dma_buf_dma_size;

void set_zbufcmp(int cmpmode);

/////////////////////////////////////////////////////////////////////////////////////////
// DoStuff
/////////////////////////////////////////////////////////////////////////////////////////

u32 start_demo( const demo_init_t* pInfo )
{
  gp_Info = pInfo;
  
	PbMatrix ViewScreenMatrix;
	PbMatrix CameraMatrix;
	PbMatrix CombindedMatrix;
	PbMatrix FinalMatrix;
	PbMatrix RotateMatrix;
	PbMatrix RotateMatrix2;
  float    angles[4] __attribute__((aligned(16)));

  PbGfx_Setup();
  
	///////////////////////////////////////////////////////////////////////////////////////
	// Send our only vu1 program to vif1/vu1

	PbVu1_AddToChain( 0, (void*)0x70000000, 0 );
	PbDma_Send01Chain( (void*)0x70000000, 1 );
	PbDma_Wait01();

	///////////////////////////////////////////////////////////////////////////////////////
	// Setup camera matrix and so


  PbMatrix_CreateViewScreen( &ViewScreenMatrix, 
														 512.0f,1.0f,1.0f, 1024+SCR_W/2,(1024+SCR_H/2),
														 1.0f, 6777215.0f,64.0f, 5536.0f );

	PbMatrix_MakeIdentity( &CameraMatrix );
	PbMatrix_Translate( &CameraMatrix, 0, 0, 500  );

	PbMatrix_Multiply( &CombindedMatrix, &CameraMatrix, &ViewScreenMatrix );
/*
	set_zbufcmp( 2 );

	float angle = 0.20f;

	PbMatrix_BuildHeading( &RotateMatrix, angle );
	PbMatrix_Multiply( &CombindedMatrix, &RotateMatrix, &CameraMatrix );
  PbMatrix_Multiply( &FinalMatrix, &CombindedMatrix, &ViewScreenMatrix );

	PbMatrix_Print( "FinalMatrix", &FinalMatrix );	

 	PbPart1_DrawObject( &FinalMatrix, NULL );

	PbVu1_Wait();
	PbVu1_DumpMem();

	PbMatrix_Print( "FinalMatrix", &FinalMatrix );	
*/

 	//PbPart1_DrawObject( &ViewScreenMatrix, &CameraMatrix, &angles,  NULL );
	//PbVu1_Wait();
	//PbVu1_DumpMem();

	///////////////////////////////////////////////////////////////////////////////////////
	// Enter loop

  angles[0] = angles[1] = angles[2] = angles[3] = 0.0f;
	angles[0] = 0.05f;

  angles[3] = 0.001; // this is the lagrate

  float angle = 0.0f;

  while( pInfo->time_count > 0 )
  {
  	angles[2] = pInfo->curr_time;
  	angles[0] += 0.01f;

		//PbMatrix_BuildPitch( &RotateMatrix2, angle );
		//PbMatrix_BuildHeading( &RotateMatrix, angle );
		//PbMatrix_Multiply( &RotateMatrix, &RotateMatrix, &RotateMatrix2 );
		//PbMatrix_Multiply( &CombindedMatrix, &RotateMatrix, &CameraMatrix );
 	  //PbMatrix_Multiply( &FinalMatrix, &CombindedMatrix, &ViewScreenMatrix );
	
		set_zbufcmp( 1 );
		PbGfx_ClearScreen();

		set_zbufcmp( 2 );

  	PbPart1_DrawObject( &ViewScreenMatrix, &CameraMatrix, &angles,  NULL );
				
    PbGfx_Update();

  }
  
  return pInfo->screen_mode;
}

/*
		dma_buf[dma_buf_cur++] = PS2_GS_PRIM;
		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_PRIM( PS2_GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0);

		dma_buf[dma_buf_cur++] = PS2_GS_RGBAQ;
		dma_buf[dma_buf_cur++] = color;

		dma_buf[dma_buf_cur++] = PS2_GS_XYZ2;
		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_XYZ2( (0+offs_x)<<4, (0+offs_y)<<4, 0 );

		dma_buf[dma_buf_cur++] = PS2_GS_XYZ2;
		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_XYZ2( (200+offs_x)<<4, (200+offs_y)<<4, 0 );
*/

/*
		dma_buf[dma_buf_cur++] = ((u64)(4)<< 0) | ((u64)(1)	<< 15) | ((u64)(0)	<< 46) | ((u64)(0)	<< 47) | ((u64)(0)	<< 58) | ((u64)(1)	<< 60);
		dma_buf[dma_buf_cur++] = GIF_AD;

		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_PRIM( PS2_GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0);
		dma_buf[dma_buf_cur++] = PS2_GS_PRIM;
  
		dma_buf[dma_buf_cur++] = color;
		dma_buf[dma_buf_cur++] = PS2_GS_RGBAQ;
  
		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_XYZ2( (0+offs_x)<<4, (0+offs_y)<<4, 0 );
		dma_buf[dma_buf_cur++] = PS2_GS_XYZ2;
  
		dma_buf[dma_buf_cur++] = PS2_GS_SETREG_XYZ2( (200+offs_x)<<4, (200+offs_y)<<4, 0 );
		dma_buf[dma_buf_cur++] = PS2_GS_XYZ2;
*/

/*
		BEGIN_GS_PACKET(dma_buf);

		dma_buf_dma_size = 5;

		dma_buf[dma_buf_cur++] = ((u64)4<< 60) | ((u64)0 << 58) | ((u64)3 << 47) | ((u64)1 << 46) | ((u64)1 << 15) | 1;
		dma_buf[dma_buf_cur++] = 0x5551;

		dma_buf[dma_buf_cur++] = 127;
		dma_buf[dma_buf_cur++] = 0;
    
		dma_buf[dma_buf_cur++] = ((1024L + 100) << 4 ) | ( ((1024L + 20) << 4) << 32 );  // X,Y;
		dma_buf[dma_buf_cur++] = 0;
  
		dma_buf[dma_buf_cur++] = ((1024L + 100) << 4 ) | ( ((1024L + 84) << 4) << 32 );
		dma_buf[dma_buf_cur++] = 0;

		dma_buf[dma_buf_cur++] = ((1024L + 160) << 4 ) | ( ((1024L + 84) << 4) << 32 );
		dma_buf[dma_buf_cur++] = 0;

	  SEND_GS_PACKET(dma_buf);


	


		while(*D2_CHCR&0x100);
*/
