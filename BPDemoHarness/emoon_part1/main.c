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
 	PbMatrix ViewScreenMatrix;
	PbMatrix CameraMatrix;
  float    angles[4] __attribute__((aligned(16)));
  int      down = 1;

  gp_Info = pInfo;

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

  ///////////////////////////////////////////////////////////////////////////////////////
  // Debugdump
/*
 	PbPart1_DrawObject( &ViewScreenMatrix, &CameraMatrix, (float*)&angles,  NULL );
	PbVu1_Wait();
	PbVu1_DumpMem();
*/
	///////////////////////////////////////////////////////////////////////////////////////
	// Enter loop

  angles[0] = angles[1] = angles[2] = angles[3] = 0.0f;
	angles[0] = 0.05f;

  angles[3] = 0.101; // this is the lagrate

  while( pInfo->time_count > 0 )
  {
  	angles[2] = pInfo->curr_time;
/*
    if( down == 1 )
    {
      angles[3] -= 0.04f;
    	angles[0] -= 0.02f;

      if( angles[3] < -2.0f )
        down = 0;
    }
    else
    {
      angles[3] += 0.04f;
    	angles[0] += 0.02f;

      if( angles[3] > 2.0f )
      {
        angles[3] = 2.0;
        down = 1;
      }
    }
*/
    angles[0] += 0.01f;

	
		set_zbufcmp( 1 );
		PbGfx_ClearScreen();

		set_zbufcmp( 2 );

  	PbPart1_DrawObject( &ViewScreenMatrix, &CameraMatrix, (float*)&angles,  NULL );
				
    PbGfx_Update();

  }
  
  return pInfo->screen_mode;
}

