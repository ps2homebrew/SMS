// ******************************************************************************
//
// Hardware.c
//
// PS2 Screen setup, also has screen+palette uploads
// Load
//
// ******************************************************************************


#include <tamtypes.h>
#include <string.h>
#include <kernel.h>
#include <sifrpc.h>
//#include <stdio.h>
//#include <stdlib.h>
#include "hw.h"
#include "hardware.h"
//#include "pad.h"


#define SCREEN_CLUT_8     		(0x3000)		// VRAM location of screen Palette
#define	SCREEN_TEXTURE			(0x3010)		// VRAM location of screen texture

extern int show_logo;
	//
	// Emulated screen size
	//
	int		g_nScreen_X	= 366;					// These can go UP TO 512x512
	int		g_nScreen_Y	= 256;
	int		g_nFiltered = 0;					// set to 1 to bi-linear filter the screen

	int		g_nDisplayWidth = 366;
	int		g_nDisplayHeight= 256;

	int		g_nClearScreen = 0;					// set to 1 to clear screen after FLIP
	U32		g_nClearColour = 0x00000000;		// clear screen colour

//	int		g_nScaleH = 4;
//	int		g_nScaleV = 0;


	u64	DMABuffer[1*1024] __attribute__((__section__(".bss"))) __attribute__((aligned(16)));				// a general 512K DMA buffer
	u64	ScreenBuffer[512*512+(16*30)] __attribute__((__section__(".bss"))) __attribute__((aligned(16)));	// The 512x512 texture map for the screen.
//	u64	ImageBuffer[512*512] __attribute__((__section__(".bss"))) __attribute__((aligned(16)));	// The 512x512 texture map for the screen.
	PU8	pScreen = (PU8) &ScreenBuffer[12];



	U8 PS2Palette[] __attribute__((aligned(16))) __attribute__((__section__(".text")))={
			0x00,0x00,0x00,0x80,		// black
			0x00,0x00,0x80,0x80,			// blue
			0x80,0x00,0x00,0x80,			// red
			0x80,0x00,0x80,0x80,			// magenta
			0x00,0x80,0x00,0x80,			// green
			0x00,0x80,0x80,0x80,			// cyan
			0x80,0x80,0x00,0x80,			// yellow
			0x80,0x80,0x80,0x80,			// white

			0x00,0x00,0x00,0x80,		// black
			0x00,0x00,0xff,0x80,			// blue
			0xff,0x00,0x00,0x80,			// red
			0xff,0x00,0xff,0x80,			// magenta
			0x00,0xff,0x00,0x80,			// green
			0x00,0xff,0xff,0x80,			// cyan
			0xff,0xff,0x00,0x80,			// yellow
			0xff,0xff,0xff,0x80,			// white

			0x37,0x55,0x78,0x80,	0x40,0x00,0x00,0x80,	0x60,0x00,0x00,0x80,	0x7F,0x3D,0x00,0x80,
			0x7F,0x7A,0x00,0x80,	0x40,0x40,0x40,0x80,	0x60,0x60,0x60,0x80,	0x7F,0x7F,0x7F,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,
			0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80,	0x00,0x00,0x00,0x80
		};
	PU32	pPalette32 = (PU32) &PS2Palette[0];

        // JH - palette mapper - naps palette to correct place in the CLUT
        u8 palmap[256] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
            0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
            0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
            0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
            0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
            0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
            0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
            0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
            0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
            0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
            0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
            0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
            0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
            0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
            0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
            0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
            0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
            0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
            0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
            0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
            0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
            0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
            0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
            0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
            0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
            0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
            0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
            0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
            0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
            0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
        };


//#include "systemfont.c"

//********************************************************
//
// Name:       ClearScreen
// Function:   This clears the screen to a colour, and also
//             wipes the ZBuffer.(based on Sjeep sample)
// Author:     Bigboy
//
// In:         ARGB = colour to clear to
// Out:        None
//
//********************************************************
void	ClearScreen( U32 ARGB )
{

	PSQuadPoly		pBuff = (PSQuadPoly) &DMABuffer[0];

	pBuff->tag.NLOOP = 1; 					// set nloop to the number of sprite primatives to be sent
	pBuff->tag.EOP = 1; 					// set end of packet, since we're only sending one structure.
	pBuff->tag.PRE = 0;						// pre and prim field of giftag are disregared in reglist mode
	pBuff->tag.PRIM = 0;					// TRISTRIP mode
	pBuff->tag.FLG = 1; 					// reglist mode
	pBuff->tag.NREG = 9; 					// 4 registers to be set
	pBuff->tag.REGS0 = 0x00; 				// prim register
	pBuff->tag.REGS1 = 0x01; 				// rgbaq register
	pBuff->tag.REGS2 = 0x05; 				// xyz2 register
	pBuff->tag.REGS3 = 0x01; 				// rgbaq register
	pBuff->tag.REGS4 = 0x05;
	pBuff->tag.REGS5 = 0x01; 				// rgbaq register
	pBuff->tag.REGS6 = 0x05;
	pBuff->tag.REGS7 = 0x01; 				// rgbaq register
	pBuff->tag.REGS8 = 0x05;

	pBuff->Prim.prim = GS_TRISTRIP;

	pBuff->Prim.rgbaq1 = ARGB | ((u64)0x3f800000<<32);

	pBuff->Prim.xyz2_1 = GS_SET_XYZ( SCREEN_LEFT<<4, 		SCREEN_TOP<<4, 0);
	pBuff->Prim.xyz2_2 = GS_SET_XYZ( (SCREEN_LEFT+512)<<4,	SCREEN_TOP<<4,	0);
	pBuff->Prim.xyz2_3 = GS_SET_XYZ( SCREEN_LEFT<<4,		(SCREEN_TOP+512)<<4, 0);
	pBuff->Prim.xyz2_4 = GS_SET_XYZ((SCREEN_LEFT+512)<<4,	(SCREEN_TOP+512)<<4, 0);


//	k_FlushCache(0); 										// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)pBuff,sizeof(SQuadPoly)/16,0x101); 	// send Quad to GIF
	Dma02Wait(); // wait for DMA transfer to finish

}

//********************************************************
//
// Name:       SetupScreen
// Function:   Setup the Screen and drawing areas etc.
// Author:     Bigboy
//
// In:         nDisplayBuffer = Which buffer to display (0 or 1)
// Out:        None
//
//********************************************************
void	SetupScreen( int nDisplayBuffer )
{	
	U128	 		Buffer[15];
	U128	 		*pBuff = &Buffer[0];
	
	// Set display environment (512x512 interlace, non-interlaced screen )
	if(show_logo) *GS_PMODE = 0x8067;
	else *GS_PMODE = 0xff65;
	//*GS_PMODE = 0x8005;				// RC 1 & 2 both on
	asm __volatile__ ("sync.l");
	*GS_SMODE2 = 1;
	asm __volatile__ ("sync.l");

	if( nDisplayBuffer == 0 ){
		*GS_DISPFB1 = 0x1000;
		asm __volatile__ ("sync.l");
		*GS_DISPFB2 = 0x1100;
		asm __volatile__ ("sync.l");
	}
	else{
		*GS_DISPFB1 = 0x1080;
		asm __volatile__ ("sync.l");
		*GS_DISPFB2 = 0x1100;
		asm __volatile__ ("sync.l");
	}

	//
	// No IDEA what this one could be used for... (you CAN get a 160x128 screen....but why?!?!?!)
	//
	if( (g_nDisplayWidth == 160) && (g_nDisplayHeight==172) )
	{
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 15, 2, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 15, 2, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
	}
	//
	// 256x256, nice mode for say a SNES emulator that has a 256x256 screen
	//
	else if( ( (g_nDisplayWidth == 256) && (g_nDisplayHeight==256) ) ||
	 	( (g_nDisplayWidth == 256) && (g_nDisplayHeight==240) ) )
	{
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 9, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 9, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
	}
	//
	// 320x256, nice mode for say a Spectrum emulator that has a 256x200 + border area
	//
	else if(  ( (g_nDisplayWidth == 320) && (g_nDisplayHeight==256) ) ||
	 	( (g_nDisplayWidth == 320) && (g_nDisplayHeight==240) ) )
	{
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 7, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 7, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
	}
	//
	// 366x256, nice mode for say a C64 emulator (one that has a 320x200 screen+border area)
	//
	else if( ( (g_nDisplayWidth == 366) && (g_nDisplayHeight==256) ) ||
	 	( (g_nDisplayWidth == 366) && (g_nDisplayHeight==240) ) )
	{
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 6, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");					 
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 6, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
	}
	//
	// 427x256....?
	//
	else if( (g_nDisplayWidth == 427) && (g_nDisplayHeight==256) )
	{
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 6, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");					 
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 6, 1, 0x9ff, 0x1ff );
		asm __volatile__ ("sync.l");
	}
	//
	// 640x256....? Our texture is only 512x512 in size... can't display this......yet!
	//
//	else if( (g_nDisplayWidth == 640) && (g_nDisplayHeight==256) )
//	{
//		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 3, 1, 0x9ff, 0x1ff );
//		asm __volatile__ ("sync.l");					 
//		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 3, 1, 0x9ff, 0x1ff );
//		asm __volatile__ ("sync.l");
//	}
	else
	{
		//
		// Default screen size is 512x512
		//
		*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, 4, 0, 0x9ff, 0x1ff );		//0x001ff9ff020482a4
		asm __volatile__ ("sync.l");
		*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, 4, 0, 0x9ff, 0x1ff );		//0x001ff9ff020482a4
		asm __volatile__ ("sync.l");
	}

//	*(pu64)GS_DISPLAY1= GS_SET_DISPLAY( 0x2a4, 0x48, g_nScaleH, g_nScaleV, 0x9ff, 0x1ff );		//0x001ff9ff020482a4
//	asm __volatile__ ("sync.l");
//	*(pu64)GS_DISPLAY2= GS_SET_DISPLAY( 0x2a4, 0x48, g_nScaleH, g_nScaleV, 0x9ff, 0x1ff );		//0x001ff9ff020482a4
//	asm __volatile__ ("sync.l");


//	printf("DISPLAY1=%x\n",GS_SET_DISPLAY( 0x2a4, 0x48, 2, 0, 0x9ff, 0x1ff ) );
		
	// set GIF tag 
	pBuff[0].ul64[0] = GIF_SET_TAG( 12, 1, NULL, NULL, GIF_PACKED, 1);
	pBuff[0].ul64[1] = 0xe;

	// frame buffer settting
	if( nDisplayBuffer == 0 ){
		pBuff[1].ul64[0] = GS_SET_FRAME_1(0x80, SCRN_W/64, GS_ARGB32, 0);
	}else{
		pBuff[1].ul64[0] = GS_SET_FRAME_1(0, SCRN_W/64, GS_ARGB32, 0);
	}
	pBuff[1].ul64[1] = GS_FRAME_1;

	// Z buffer settting
	pBuff[2].ul64[0] = GS_SET_ZBUF_1(0x100, GS_Z32, 0);
	pBuff[2].ul64[1] = GS_ZBUF_1;

	// offset value ( PRIM coord -> WIN coord )
	pBuff[3].ul64[0] = GS_SET_XYOFFSET_1( OFFX, OFFY );			// this gives us a CLIP area, but remember to add on OFFX and OFFY
	pBuff[3].ul64[1] = GS_XYOFFSET_1;

	// scissor settings ( WIN coordinates x0,x1,y0,y1 )
	pBuff[4].ul64[0] = GS_SET_CLIP_1(0,0, (SCRN_W-1),(SCRN_H-1) );
	pBuff[4].ul64[1] = GS_CLIP_1;

	// enable PRIM register
	pBuff[5].ul64[0] = GS_SET_PRMODECONT(1);
	pBuff[5].ul64[1] = GS_PRMODECONT;


	// set color clamping
	pBuff[6].ul64[0] = GS_SET_COLCLAMP(1);
	pBuff[6].ul64[1] = GS_COLCLAMP;

	// set the dither
	pBuff[7].ul64[0] = GS_SET_DTHE(0);
	pBuff[7].ul64[1] = GS_DITHER;

	// flush the texture cache
	pBuff[8].ul64[0] = 0;
	pBuff[8].ul64[1] = GS_TEXTUREFLUSH;

	// set filtering
	pBuff[9].ul64[0] = GS_SET_TEX1(NULL, NULL, GS_BILINEAR, GS_BILINEAR, NULL, NULL, NULL);
	pBuff[9].ul64[1] = GS_TEX1_1;

	// alpha blend settings
	pBuff[10].ul64[0] = GS_SET_ALPHA(0, 1, 0, 1, 0);			// ????
	pBuff[10].ul64[1] = GS_ALPHA_1;

	// alpha correction value off
	pBuff[11].ul64[0] = GS_SET_FBA(0);
	pBuff[11].ul64[1] = GS_FBA_1;

	// pixel test control (none, disable Z buffer )
	pBuff[12].ul64[0] = GS_SET_TEST(0,0,0,0,0,0,0,0);
	pBuff[12].ul64[1] = GS_TEST_1;

	// DMA send (Normal Mode) to GS via path 3		

//	k_FlushCache(0); 										// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)pBuff,13,0x101); 					// send Quad to GIF
	Dma02Wait(); 											// wait for DMA transfer to finish

	if( g_nClearScreen==1 ){
		ClearScreen( g_nClearColour );
	}
}



//********************************************************
//
// Name:       RenderQuad
// Function:   DMA and Display a single QUAD. This is
//             VERY slow, since we wait for it to finish 
//             drawing... but who cares...
// Author:     Bigboy
//
// In:         none
// Out:        None
//
// Notes:	   Test GS functions.
//
//********************************************************
void	RenderQuad2( void )
{

	U128			*pBuff = (U128*) &DMABuffer[0];


	//	loop, end, pre, prim, flg, nreg
	pBuff[0].ul64[0] = GIF_SET_TAG(1, 1, 1, GS_TRISTRIP|GS_GSHADE, 0, 8 );
	pBuff[0].ul64[1] = 0x51515151;

	pBuff[1].u32[0] = 0xff;								// colours
	pBuff[1].u32[1] = 0xff;					
	pBuff[1].u32[2] = 0xff;					
	pBuff[1].u32[3] = 0x3f800000;					

	pBuff[2].u32[0] = (SCREEN_LEFT) << 4;				// XY1
	pBuff[2].u32[1] = (SCREEN_TOP) << 4;	
	pBuff[2].u32[2] = 5;					
	pBuff[2].u32[3] = 0;						
		  	
	pBuff[3].u32[0] = 0xff;								// colours
	pBuff[3].u32[1] = 0x0;					
	pBuff[3].u32[2] = 0x0;					
	pBuff[3].u32[3] = 0x3f800000;

	pBuff[4].u32[0] = (SCREEN_LEFT+256) << 4;
	pBuff[4].u32[1] = (SCREEN_TOP) << 4;
	pBuff[4].u32[2] = 5;
	pBuff[4].u32[3] = 0;

	pBuff[5].u32[0] = 0x0;								// colours
	pBuff[5].u32[1] = 0xff;					
	pBuff[5].u32[2] = 0x0;					
	pBuff[5].u32[3] = 0x3f800000;

	pBuff[6].u32[0] = (SCREEN_LEFT) << 4;
	pBuff[6].u32[1] = (SCREEN_TOP+256) << 4;
	pBuff[6].u32[2] = 5;
	pBuff[6].u32[3] = 0;						

	pBuff[7].u32[0] = 0x0;								// colours
	pBuff[7].u32[1] = 0x0;					
	pBuff[7].u32[2] = 0xff;					
	pBuff[7].u32[3] = 0x0;					
							  
	pBuff[8].u32[0] = (SCREEN_LEFT+256) << 4;
	pBuff[8].u32[1] = (SCREEN_TOP+256) << 4;
	pBuff[8].u32[2] = 1 << 4;
	pBuff[8].u32[3] = 0;						


//	k_FlushCache(0); 									// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)&pBuff[0], 9, 0x101); 			// send Quad to GIF
	Dma02Wait(); 										// wait for DMA transfer to finish

}



//********************************************************
//
// Name:       RenderQuad
// Function:   DMA and Display a single QUAD. This is
//             VERY slow, since we wait for it to finish 
//             drawing... but who cares...
// Author:     Bigboy
//
// In:         none
// Out:        None
//
// Notes:	   Test GS functions.
//
//********************************************************
void	RenderQuad( int xx )
{

	U128			*pBuff = (U128*) &DMABuffer[0];


	//	loop, end, pre, prim, flg, nreg
	pBuff[0].ul64[0] = GIF_SET_TAG(1, 1, 1, GS_TRISTRIP|GS_GSHADE, 0, 8 );
	pBuff[0].ul64[1] = 0x51515151;

	pBuff[1].u32[0] = 0xff;								// colours
	pBuff[1].u32[1] = 0xff;					
	pBuff[1].u32[2] = 0xff;					
	pBuff[1].u32[3] = 0x3f800000;					

	pBuff[2].u32[0] = (SCREEN_LEFT+xx) << 4;				// XY1
	pBuff[2].u32[1] = (SCREEN_TOP) << 4;	
	pBuff[2].u32[2] = 5;					
	pBuff[2].u32[3] = 0;						
		  	
	pBuff[3].u32[0] = 0xff;								// colours
	pBuff[3].u32[1] = 0x0;					
	pBuff[3].u32[2] = 0x0;					
	pBuff[3].u32[3] = 0x3f800000;

	pBuff[4].u32[0] = (SCREEN_LEFT+256+xx) << 4;
	pBuff[4].u32[1] = (SCREEN_TOP) << 4;
	pBuff[4].u32[2] = 5;
	pBuff[4].u32[3] = 0;

	pBuff[5].u32[0] = 0x0;								// colours
	pBuff[5].u32[1] = 0xff;					
	pBuff[5].u32[2] = 0x0;					
	pBuff[5].u32[3] = 0x3f800000;

	pBuff[6].u32[0] = (SCREEN_LEFT+xx) << 4;
	pBuff[6].u32[1] = (SCREEN_TOP+256) << 4;
	pBuff[6].u32[2] = 5;
	pBuff[6].u32[3] = 0;						

	pBuff[7].u32[0] = 0x0;								// colours
	pBuff[7].u32[1] = 0x0;					
	pBuff[7].u32[2] = 0xff;					
	pBuff[7].u32[3] = 0x0;					
							  
	pBuff[8].u32[0] = (SCREEN_LEFT+256+xx) << 4;
	pBuff[8].u32[1] = (SCREEN_TOP+256) << 4;
	pBuff[8].u32[2] = 1 << 4;
	pBuff[8].u32[3] = 0;						


//	k_FlushCache(0); 									// Flush the cache to get data ready
    FlushCache(0);
    dma02_send((void *)&pBuff[0], 9, 0x101); 			// send Quad to GIF
	Dma02Wait(); 										// wait for DMA transfer to finish

}






//********************************************************
//
// Name:       DrawScreen
// Function:   Draw a texture (our screen) to the display
// Author:     Bigboy
//
//********************************************************
#define	OFFSETX			(0)
#define	OFFSETY			(0)
void	DrawScreen( void )
{
	U128			*pBuff = (U128*) &DMABuffer[0];

	pBuff[1].ul64[0] = GIF_SET_TAG(5, 0, 0, 0, 0, 1);
	pBuff[1].ul64[1] = 0xe;

	pBuff[2].ul64[0] = 0;
	pBuff[2].ul64[1] = GS_TEXFLUSH;

	pBuff[3].ul64[0] = GS_SET_TEX0( SCREEN_TEXTURE,  8, GS_PSMT8, 9, 9, 1, 1, SCREEN_CLUT_8, GS_PSMCT32, 0, 0, 1);
    pBuff[3].ul64[1]=  GS_TEX0_1;

	// filter display?
	if( g_nFiltered!=0){
		pBuff[4].ul64[0] = GS_SET_TEX1(0, 0, GS_LINEAR, GS_LINEAR, 0, 0, 0);
	}
	else{
		pBuff[4].ul64[0] = GS_SET_TEX1(0, 0, GS_NEAREST, GS_NEAREST, 0, 0, 0);
	}
	pBuff[4].ul64[1] = GS_TEX1_1;

	pBuff[5].ul64[0] = GS_SET_CLAMP(1, 1, 0, 0, 0, 0);
	pBuff[5].ul64[1] = GS_CLAMP_1;
	
    pBuff[6].ul64[0] = 0;											
    pBuff[6].ul64[1] = GS_TEST_1;								// Disable Z buffer

	pBuff[7].ul64[0] = GIF_SET_TAG(1, 1, 1, GS_SPRITE | GS_PRIM_TME | GS_PRIM_FST, 0, 5);
	pBuff[7].ul64[1] = 0x53531;

	pBuff[8].u32[0] = 0xff;										// colours
	pBuff[8].u32[1] = 0xff;					
	pBuff[8].u32[2] = 0xff;					
	pBuff[8].u32[3] = 0x0;					

	pBuff[9].ul128 = (u128)0;
	pBuff[9].u32[0] = 0;
	pBuff[9].u32[1] = 0;

	pBuff[10].u32[0] = (SCREEN_LEFT+OFFSETX) << 4;				// XY1
	pBuff[10].u32[1] = (SCREEN_TOP+OFFSETY) << 4;	
	pBuff[10].u32[2] = 1 << 4;					
	pBuff[10].u32[3] = 0;						
		  
	pBuff[11].ul128 = (u128)0;
	pBuff[11].u32[0] = g_nScreen_X<<4;
	pBuff[11].u32[1] = g_nScreen_Y<<4; 
		  	
	pBuff[12].u32[0] = (SCREEN_LEFT+g_nDisplayWidth+OFFSETX) << 4;
	pBuff[12].u32[1] = (SCREEN_TOP+g_nDisplayHeight+OFFSETY) << 4;
	pBuff[12].u32[2] = 1 << 4;
	pBuff[12].u32[3] = 0;						


//	k_FlushCache(0); 											// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)&pBuff[1], 12, 0x101); 					// send Quad to GIF
	Dma02Wait(); 												// wait for DMA transfer to finish
}


//********************************************************
//
// Name:		UploadPalette
// Function:   	This function uploads a 256 colour palette
// Author:		Bigboy
//
// In:			Address =	Base address, 16 byte aligned
//				Xsize	= 	size of texture (or 16 for pallete)
//				Ysize	= 	size of texture (or 16 for pallete)
//				VAdd	= 	VRAM address
// Out:			None
//
//********************************************************
void	UploadPalette( void* Address, U32 Xsize, U32 Ysize, U32 VAdd )
{
	U128			*pBuff = (U128*) &DMABuffer[0];
	U32 			dTextureQuadWordCount;
	U32 			dBuffWidth;


//	k_FlushCache(0); 												// Flush the cache to get data ready
    FlushCache(0);
	Dma02Wait(); 													// wait for DMA transfer to finish

	// get quad word count for image
	dBuffWidth = (Xsize+63) >> 6;

	//	loop, end, pre, prim, flg, nreg
	pBuff[0].ul64[0] = GIF_SET_TAG(4,0,NULL,NULL,GIF_PACKED,1);
	pBuff[0].ul64[1] = 0xe;

	// set transmission between buffers	(Palette???)
	dTextureQuadWordCount = (Xsize*Ysize*4)/16;
	pBuff[1].ul64[0] = GS_SET_BITBLTBUF(NULL, NULL, NULL, VAdd, dBuffWidth, GS_PSMCT32 );
	pBuff[1].ul64[1] = GS_BITBLTBUF;

	memcpy( &pBuff[6], Address, dTextureQuadWordCount*16);			// copy texture/Palette into place
	
	// set transmission area between buffers	( source x,y  dest x,y  and direction )
	pBuff[2].ul64[0] = GS_SET_TRXPOS(0, 0, 0, 0, 0);
	pBuff[2].ul64[1] = GS_TRXPOS;

	// set size of transmission area 
	pBuff[3].ul64[0] = GS_SET_TRXREG( Xsize, Ysize );
	pBuff[3].ul64[1] = GS_TRXREG;

	// set transmission direction  ( HOST -> LOCAL Transmission )
	pBuff[4].ul64[0] = GS_SET_TRXDIR( GS_HOSTLOCAL );
	pBuff[4].ul64[1] = GS_TRXDIR;

	// GIF tag for texture
	pBuff[5].ul64[0] = GIF_SET_TAG( dTextureQuadWordCount, 0, NULL,NULL, GIF_IMAGE, NULL);
	pBuff[5].ul64[1] = 0x0;



//	k_FlushCache(0); 												// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)&pBuff[0], 6+dTextureQuadWordCount ,0x101); 				// 
	Dma02Wait(); 													// wait for DMA transfer to finish

}


//********************************************************
//
// Name:		UploadTexture
// Function:   	This function uploads palettised textures.
//				it uploads the screen into a 512x512 texture.
//				If the screen is < 512x512, it uploads it into 
//				the top left corner ofthe 512x512
// Author:		Bigboy
//
// In:			Address =	Base address, 16 byte aligned
//				Xsize	= 	size of texture (or 16 for pallete)
//				Ysize	= 	size of texture (or 16 for pallete)
//				VAdd	= 	VRAM address
// Out:			None
//
//********************************************************
void	UploadScreen( U32 Xsize, U32 Ysize, U32 VAdd )
{
	U128			*pBuff = (U128*) &ScreenBuffer[0];
	U32 			dTextureQuadWordCount;
	U32 			dBuffWidth;


//	k_FlushCache(0); 														// Flush the cache to get data ready
    FlushCache(0);
	Dma02Wait(); 															// wait for DMA transfer to finish

	// get quad word count for image
	dBuffWidth = (512+63) >> 6;												// Dest Texture size is 512x512

	//	loop, end, pre, prim, flg, nreg
	pBuff[0].ul64[0] = GIF_SET_TAG(4,0,NULL,NULL,GIF_PACKED,1);
	pBuff[0].ul64[1] = 0xe;

	// set transmission between buffers	(Palette???)
	//pBuff[1].ul64[0] = GS_SET_BITBLTBUF(NULL, NULL, NULL, VAdd, dBuffWidth, GS_PSMT8);
	pBuff[1].ul64[0] = GS_SET_BITBLTBUF(NULL, NULL, NULL, VAdd, dBuffWidth, GS_PSMT8);
	pBuff[1].ul64[1] = GS_BITBLTBUF;

	// set transmission area between buffers	( source x,y  dest x,y  and direction )
	pBuff[2].ul64[0] = GS_SET_TRXPOS(0, 0, 0, 0, 0);
	pBuff[2].ul64[1] = GS_TRXPOS;

	// set size of transmission area 
	pBuff[3].ul64[0] = GS_SET_TRXREG( Xsize, Ysize );
	pBuff[3].ul64[1] = GS_TRXREG;

	// set transmission direction  ( HOST -> LOCAL Transmission )
	pBuff[4].ul64[0] = GS_SET_TRXDIR( GS_HOSTLOCAL );
	pBuff[4].ul64[1] = GS_TRXDIR;

	// GIF tag for texture
	dTextureQuadWordCount = ((Xsize*Ysize)+15)/16;

	//	loop, end, pre, prim, flg, nreg
	pBuff[5].ul64[0] = GIF_SET_TAG( dTextureQuadWordCount, 0, NULL,NULL, GIF_IMAGE, NULL);
	pBuff[5].ul64[1] = 0x0;



//	k_FlushCache(0); 														// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)&pBuff[0], 6+dTextureQuadWordCount ,0x101); 			// 
	Dma02Wait(); 															// wait for DMA transfer to finish

}


//********************************************************
//
// Name:		Uploadimage24
// Function:   	This function uploads a 24 bit image.
//				If the screen is < 512x512, it uploads it into
//				the top left corner ofthe 512x512
// Author:		james
//
// In:			Address =	Base address, 16 byte aligned
//				Xsize	= 	size of texture (or 16 for pallete)
//				Ysize	= 	size of texture (or 16 for pallete)
//				VAdd	= 	VRAM address
// Out:			None
//
//********************************************************
void	UploadImage24( U32 *src, U32 Xsize, U32 Ysize, U32 VAdd )
{
	U128			pBuff[32 + 320 * 256] __attribute__((aligned(16)));
	U32 			dTextureQuadWordCount;
	U32 			dBuffWidth;
    int i, n;
    U8 *p8;
    U8 *q8;

    // copy src into DMA buffer, convert RGB to ARGB
    n = (Xsize * Ysize);
    p8 = (U8 *)pBuff; p8 += 96;
    q8 = (U8 *)src;
    for(i=0; i< n; i++) {
        *p8++ = *q8++;
        *p8++ = *q8++;
        *p8++ = *q8++;
        *p8++ = 0xFF;
    }

//    k_FlushCache(0); 														// Flush the cache to get data ready
    FlushCache(0);
	Dma02Wait(); 															// wait for DMA transfer to finish

	// get quad word count for image
	dBuffWidth = (512+63) >> 6;												// Dest Texture size is 512x512

	//	loop, end, pre, prim, flg, nreg
	pBuff[0].ul64[0] = GIF_SET_TAG(4,0,NULL,NULL,GIF_PACKED,1);
	pBuff[0].ul64[1] = 0xe;

	// set transmission between buffers	(Palette???)
	pBuff[1].ul64[0] = GS_SET_BITBLTBUF(NULL, NULL, NULL, VAdd, dBuffWidth, GS_PSMCT32);
	pBuff[1].ul64[1] = GS_BITBLTBUF;

	// set transmission area between buffers	( source x,y  dest x,y  and direction )
	pBuff[2].ul64[0] = GS_SET_TRXPOS(0, 0, 0, 0, 0);
	pBuff[2].ul64[1] = GS_TRXPOS;

	// set size of transmission area 
	pBuff[3].ul64[0] = GS_SET_TRXREG( Xsize, Ysize );
	pBuff[3].ul64[1] = GS_TRXREG;

	// set transmission direction  ( HOST -> LOCAL Transmission )
	pBuff[4].ul64[0] = GS_SET_TRXDIR( GS_HOSTLOCAL );
	pBuff[4].ul64[1] = GS_TRXDIR;

 	// GIF tag for texture
	dTextureQuadWordCount = ((Xsize*Ysize)+15)/4;

	//	loop, end, pre, prim, flg, nreg
	pBuff[5].ul64[0] = GIF_SET_TAG( dTextureQuadWordCount, 0, NULL,NULL, GIF_IMAGE, NULL);
	pBuff[5].ul64[1] = 0x0;


//	k_FlushCache(0); 														// Flush the cache to get data ready
    FlushCache(0);
	dma02_send((void *)&pBuff[0], 6+dTextureQuadWordCount ,0x101); 			//
	Dma02Wait(); 															// wait for DMA transfer to finish

}




//********************************************************
//
// Name:		UploadScreen
// Function:   	This function uploads the palette and screen
//				texture. Use this each frame to update display
// Author:		Bigboy
//
// In:			None
// Out:			None
//
//********************************************************
void	UpdateScreen( void )
{
	//UploadPalette( &Palette[0], 16, 16, SCREEN_CLUT_8 );
	UploadPalette( pPalette32, 16, 16, SCREEN_CLUT_8 );
	UploadScreen( g_nScreen_X, g_nScreen_Y, SCREEN_TEXTURE );
	DrawScreen( );
}


//********************************************************
//
// Name:		SetPaletteEntry
// Function:   	The PS2 256 colour palette is a bit funny,
//				so we use this function to set the colours
// Author:		Bigboy
//
// In:			ARGB = colour to set
//				index= index to set
// Out:			None
//
//********************************************************
void	SetPaletteEntry( U32 ARGB, U32 index )
{
	//int	i;
	//i = index & 0xe7;
	//i |= (index&8)<<1;
	//i |= (index&16)>>1;
    // JH
/*
    int lobits = index & 0x1f;
    int hiofs = index & 0xe0;
    int loofs;

    if(lobits >= 8 && lobits < 16) loofs = lobits + 8;
    else if(lobits >= 16 && lobits < 24) loofs = lobits - 8;
    else loofs = lobits;
	pPalette32[hiofs + loofs] = ARGB;
*/
    pPalette32[palmap[index]] = ARGB;
}

/*
// ********************************************************
//
// Name:		Load
// Function:    Load a file into RAM
//
// Author:		Bigboy
//
// In:			pszName = filename to load
//				pBuffer = Destination buffer to load INTO
// Out:			size of loaded data, or -1 for error
//
// Examples:-	"host:\\homer_simpson.pcm"
//				"cdrom:\\homer_simpson.pcm;1"
//
// ********************************************************
int	Load( char* pszName, PU8 pBuffer )
{
	int	handle;
	char	c[512];

	//sprintf(&c[0],FILESYS"\\%s"FILESYS_E,pszName);
#ifndef	MASTER
	//printf("PATH=%s\n", &c[0]);
#endif

	//handle = fio_open( &c[0], 0 );
	handle = fio_open( pszName, 0 );
	if( handle>=0)
	{
		int size;
		size = fio_lseek( handle, 0, 2 );
		fio_lseek( handle, 0, 0 );
		fio_read( handle, pBuffer, size );
		fio_close( handle );
		return size;
	}

#ifdef	DEBUG
	printf("Can't open file!!!\n" );
#endif
	return -1;
}
*/

/*

// ********************************************************
//
// Name:		LoadSoundModules
// Function:   	Load and init the sound module
// Author:		Sjeep
//
// In:			none
// Out:			none
//
// notes:		Seems to disable NAPLINK reset.
//
// ********************************************************
void LoadSoundModules( void )
{
    int ret;

	jprintf("Loading module: LIBSD");
	ret = sif_load_module("rom0:LIBSD", 0, NULL);
	if (ret < 0) {		  
			jprintf("Failed to load module: LIBSD");
			k_SleepThread();
    }						  

	jprintf("Loading module: SJPCM.IRX");
	ret = sif_load_module(FILESYS"SJPCM.IRX", 0, NULL);
	if (ret < 0) {
			jprintf("Failed to load module: SJPCM.IRX");
			k_SleepThread();
    }

}

*/
