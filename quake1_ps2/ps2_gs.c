/*
	vid_ps2_lib.c
	
	by Nicolas Plourde <nicolasplourde@hotmail.com>
	
	Copyright (c) Nicolas Plourde - july 2004
*/

#include "ps2_gs.h"
 
/*
	void ps2_flush_cache(int) - Flushes data cache
*/
void ps2_flush_cache(int command)
{
	asm (
			"li	$3,0x64"	"\n\t"
			"syscall"		"\n\t"
			:
			:"r"(command)
		);
			
	return;
}

/*
	void dma_reset() - Reset dma
*/
void dma_reset()
{
	asm (
			"li		$2,0x1000A000"	"\n\t"
			"nop"					"\n\t"
			"sw		$0,0x80($2)"	"\n\t"
			"sw		$0,0($2)"		"\n\t"
			"sw		$0,0x30($2)"	"\n\t"
			"sw		$0,0x10($2)"	"\n\t"
			"sw		$0,0x50($2)"	"\n\t"
			"sw		$0,0x40($2)"	"\n\t"
			"li		$2,0xFF1F"		"\n\t"
			"sw		$2,0x1000E010"	"\n\t"
			"lw		$2,0x1000E010"	"\n\t"
			"li		$3,0xFF1F"		"\n\t"
			"and	$2,$3"			"\n\t"
			"sw		$2,0x1000E010"	"\n\t"
			"sw		$0,0x1000E000"	"\n\t"
			"sw		$0,0x1000E020"	"\n\t"
			"sw		$0,0x1000E030"	"\n\t"
			"sw		$0,0x1000E050"	"\n\t"
			"sw		$0,0x1000E040"	"\n\t"
			"li		$3,1"			"\n\t"
			"lw		$2,0x1000E000"	"\n\t"
			"ori	$3,$2,1"		"\n\t"
			"nop"					"\n\t"
			"sw		$3,0x1000E000"	"\n\t"
			"nop"					"\n\t"
			:
			:
		);
			
	return;
}

/*
	void gs_set_imr() -
*/
void gs_set_imr()
{
	asm (
			"li		$4,0x0000FF00"  "\n\t"
			"ld		$2,0x12001000"  "\n\t"
			"dsrl   $2,16"			"\n\t"
			"andi   $2,0xFF"		"\n\t"
			"li		$3,0x71"		"\n\t"
			"nop"					"\n\t"
			"syscall"				"\n\t"
			:
			:
		);
}

/*
	void gs_set_crtc(uint8 int_mode, uint8 ntsc_pal, uint8 field_mode) -
*/
void gs_set_crtc(unsigned char int_mode, unsigned char ntsc_pal_vesa, unsigned char field_mode)
{
	asm (
			"li		$3,0x02"	"\n\t"
			"syscall"			"\n\t"
			:
			:"r" (int_mode),"r" (ntsc_pal_vesa),"r" (field_mode)
		);
}

/*
	void gs_init(GS_MODE mode) -
	
	GS_MODE VESA_640_480_32_75 = {0x1C,1280,480,1,32,1,356,18}; //VESA 640x480x32 75hertz
*/
// int_mode
#define NON_INTERLACED	0
#define INTERLACED		1

// field_mode
#define FRAME			1
#define FIELD			2

DECLARE_GS_PACKET(gs_dma_buf,50);

static uint16	max_x;
static uint16	max_y;

void gs_init(GS_MODE mode)
{
	max_x=mode.width-1;		// current resolution max coordinates
	max_y=mode.height-1;

	dma_reset();
	GS_RESET();

	// - Can someone please tell me what the sync.p 
	// instruction does. Synchronizes something :-)
	__asm__(
				"sync.p" "\n\t"
				"nop"    "\n\t"
				:
				:
			);

	gs_set_imr();
	gs_set_crtc(NON_INTERLACED, mode.ntsc_pal, FRAME);

	GS_SET_PMODE(
		0,		// ReadCircuit1 OFF 
		1,		// ReadCircuit2 ON
		1,		// Use ALP register for Alpha Blending
		1,		// Alpha Value of ReadCircuit2 for output selection
		0,		// Blend Alpha with the output of ReadCircuit2
		0xFF	// Alpha Value = 1.0
	);

	GS_SET_DISPFB2(
		0,				// Frame Buffer base pointer = 0 (Address/2048)
		mode.width/64,	// Buffer Width (Address/64)
		mode.psm,			// Pixel Storage Format
		0,				// Upper Left X in Buffer = 0
		0				// Upper Left Y in Buffer = 0
	);

	//vDISPLAY(479,1279,0,1,18,356) 75 hertz
	GS_SET_DISPLAY2(
		mode.startx,		// X position in the display area (in VCK units)
		mode.starty,			// Y position in the display area (in Raster units)
		mode.magh-1,	// Horizontal Magnification - 1
		0,			// Vertical Magnification = 1x
		mode.width*mode.magh-1, // Display area width  - 1 (in VCK units) (Width*HMag-1)
		mode.height-1 // Display area height - 1 (in pixels)	  (Height-1)
	);

	GS_SET_BGCOLOR(
		0,	// RED
		0,	// GREEN
		0	// BLUE
	);


	BEGIN_GS_PACKET(gs_dma_buf);

	GIF_TAG_AD(gs_dma_buf, 3, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, frame_1,
		GS_FRAME(
			0,					// FrameBuffer base pointer = 0 (Address/2048)
			mode.width/64,		// Frame buffer width (Pixels/64)
			mode.psm,				// Pixel Storage Format
			0));

	// No displacement between Primitive and Window coordinate systems.
	GIF_DATA_AD(gs_dma_buf, xyoffset_1,
		GS_XYOFFSET(
			0x0,
			0x0));

	// Clip to frame buffer.
	GIF_DATA_AD(gs_dma_buf, scissor_1,
		GS_SCISSOR(
			0,
			max_x,
			0,
			max_y));

	SEND_GS_PACKET(gs_dma_buf);
}

/*
	int gs_detect_mode(GS_MODE mode) -
*/
int gs_detect_mode()
{
	asm (
			"lui	$8,0x1fc8"		"\n\t"
			"lb		$8,-0xae($8)"   "\n\t"
			"li		$9,'E'"			"\n\t"
			"beql   $8,$9,pal_mode" "\n\t"

			"li		$2,2"			"\n\t"
			"jr		$31"			"\n\t"
			"pal_mode:"				"\n\t"
			"li		$2,3"			"\n\t"
			"jr		$31"			"\n\t"
		);

	return 0;
}

#define MAX_TRANSFER	16384
void put_image(uint16 x, uint16 y, uint16 w, uint16 h, uint32 *data)
{
	uint32 i;			// DMA buffer loop counter
	uint32 frac;		// flag for whether to run a fractional buffer or not
	uint32 current;		// number of pixels to transfer in current DMA
	uint32 qtotal;		// total number of qwords of data to transfer

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);
	GIF_DATA_AD(gs_dma_buf, bitbltbuf,
		GS_BITBLTBUF(0, 0, 0,
			0,						// frame buffer address
			(max_x+1)/64,		// frame buffer width
			0));
	GIF_DATA_AD(gs_dma_buf, trxpos,
		GS_TRXPOS(
			0,
			0,
			x,
			y,
			0));	// left to right/top to bottom
	GIF_DATA_AD(gs_dma_buf, trxreg, GS_TRXREG(w, h));
	GIF_DATA_AD(gs_dma_buf, trxdir, GS_TRXDIR(XDIR_EE_GS));
	SEND_GS_PACKET(gs_dma_buf);

	qtotal = w*h*4;					// total number of quadwords to transfer.
	current = qtotal % MAX_TRANSFER;// work out if a partial buffer transfer is needed.
	frac=1;							// assume yes.
	if(!current)					// if there is no need for partial buffer
	{
		current = MAX_TRANSFER;		// start with a full buffer
		frac=0;						// and don't do extra partial buffer first
	}
	for(i=0; i<(qtotal/MAX_TRANSFER)+frac; i++)
	{
		BEGIN_GS_PACKET(gs_dma_buf);
		GIF_TAG_IMG(gs_dma_buf, current);
		SEND_GS_PACKET(gs_dma_buf);

		SET_QWC(GIF_QWC, current);
		SET_MADR(GIF_MADR, data, 0);
		SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);
		DMA_WAIT(GIF_CHCR);

		data += current*4;
		current = MAX_TRANSFER;		// after the first one, all are full buffers
	}
}

void fill_rect(uint16 x0, uint16 y0, uint16 x1, uint16 y1)
{
	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);
	GIF_DATA_AD(gs_dma_buf, prim, GS_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 1, 0, 0, 0));
	GIF_DATA_AD(gs_dma_buf, rgbaq,	GS_RGBAQ(100, 100, 100, 100, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2((x1+1)<<4, (y1+1)<<4, 0));
	SEND_GS_PACKET(gs_dma_buf);
}
