/* Basic GS handling functions */
#include <tamtypes.h>
#include <kernel.h>
#include "harness.h"

#define CSR ((volatile u64 *)(0x12001000))
#define GS_RESET() *CSR = ((u64)(1)	<< 9)
#define PMODE		((volatile u64 *)(0x12000000))
#define GS_SET_PMODE(EN1,EN2,MMOD,AMOD,SLBG,ALP) \
	*PMODE = \
	((u64)(EN1) 	<< 0) 	| \
	((u64)(EN2) 	<< 1) 	| \
	((u64)(001)	<< 2) 	| \
	((u64)(MMOD)	<< 5) 	| \
	((u64)(AMOD) << 6) 	| \
	((u64)(SLBG) << 7) 	| \
	((u64)(ALP) 	<< 8)
//---------------------------------------------------------------------------
// DISPFP2 Register
//---------------------------------------------------------------------------
#define DISPFB2		((volatile u64 *)(0x12000090))
#define GS_SET_DISPFB2(FBP,FBW,PSM,DBX,DBY) \
	*DISPFB2 = \
	((u64)(FBP)	<< 0)	| \
	((u64)(FBW)	<< 9)	| \
	((u64)(PSM)	<< 15)	| \
	((u64)(DBX)	<< 32)	| \
	((u64)(DBY)	<< 43)

//---------------------------------------------------------------------------
// DISPLAY2 Register
//---------------------------------------------------------------------------
#define DISPLAY2	((volatile u64 *)(0x120000a0))
#define GS_SET_DISPLAY2(DX,DY,MAGH,MAGV,DW,DH) \
	*DISPLAY2 = \
	((u64)(DX)	<< 0)	| \
	((u64)(DY)	<< 12)	| \
	((u64)(MAGH)	<< 23)	| \
	((u64)(MAGV)	<< 27)	| \
	((u64)(DW)	<< 32)	| \
	((u64)(DH)	<< 44)

//---------------------------------------------------------------------------
// DISPFP1 Register
//---------------------------------------------------------------------------
#define DISPFB1	((volatile u64 *)(0x12000070))
#define GS_SET_DISPFB1(FBP,FBW,PSM,DBX,DBY) \
	*DISPFB1 = \
	((u64)(FBP)	<< 0)	| \
	((u64)(FBW)	<< 9)	| \
	((u64)(PSM)	<< 15)	| \
	((u64)(DBX)	<< 32)	| \
	((u64)(DBY)	<< 43)

//---------------------------------------------------------------------------
// DISPLAY1 Register
//---------------------------------------------------------------------------
#define DISPLAY1	((volatile u64 *)(0x12000080))
#define GS_SET_DISPLAY1(DX,DY,MAGH,MAGV,DW,DH) \
	*DISPLAY1 = \
	((u64)(DX)	<< 0)	| \
	((u64)(DY)	<< 12)	| \
	((u64)(MAGH)	<< 23)	| \
	((u64)(MAGV)	<< 27)	| \
	((u64)(DW)	<< 32)	| \
	((u64)(DH)	<< 44)

//---------------------------------------------------------------------------
// BGCOLOR Register
//---------------------------------------------------------------------------
#define BGCOLOR		((volatile u64 *)(0x120000e0))
#define GS_SET_BGCOLOR(R,G,B) \
	*BGCOLOR = \
	((u64)(R)	<< 0)		| \
	((u64)(G)	<< 8)		| \
	((u64)(B)	<< 16)

int init_gs(int scr_mode)

{
  /* Statically allocate vram. No checking done */

  GS_RESET();
  
  __asm__(" sync.p\n" \
	  " nop\n" \
	);

  GsPutIMR(0x0000F700);
  
  SetGsCrt(SCR_INT, scr_mode, SCR_FIELD);
  
  GS_SET_PMODE(
	       0,		// ReadCircuit1 OFF 
	       1,		// ReadCircuit2 ON
	       0,		// Use ALP register for Alpha Blending
	       1,		// Alpha Value of ReadCircuit2 for output selection
	       0,		// Blend Alpha with the output of ReadCircuit2
	       0xFF	// Alpha Value = 1.0
	       );
  
  
  GS_SET_DISPFB2(
		 0,				// Frame Buffer base pointer = 0 (Address/2048)
		 SCR_W/64,	// Buffer Width (Address/64)
		 SCR_PSM,			// Pixel Storage Format
		 0,				// Upper Left X in Buffer = 0
		 0				// Upper Left Y in Buffer = 0
		 );
  
  
  GS_SET_DISPLAY2(
		  656,		// X position in the display area (in VCK units)
		  30,			// Y position in the display area (in Raster units)
		  SCR_MAGW-1,	// Horizontal Magnification - 1
		  0,			// Vertical Magnification = 1x
		  SCR_W*SCR_MAGW-1,	// Display area width  - 1 (in VCK units) (Width*HMag-1)
		  SCR_H-1		// Display area height - 1 (in pixels)	  (Height-1)
		  );
  
  GS_SET_BGCOLOR(
		 0,	// RED
		 0,	// GREEN
		 0	// BLUE
		 );
  
  return 1; /* Return success */
}


void wait_vsync(void)

{
  *CSR = *CSR & 8;
  while(!(*CSR & 8));
}

int is_pal(void)

{
  if(*((char *)0x1FC80000 - 0xAE) == 'E')
    return 1;

  return 0;
}
