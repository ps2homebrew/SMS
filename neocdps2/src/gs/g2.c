//---------------------------------------------------------------------------
// File:	g2.c
// Author:	Tony Saveski, t_saveski@yahoo.com
//---------------------------------------------------------------------------
#include "g2.h"
#include "ps2.h"
#include "dma.h"
#include "gs.h"
#include "gif.h"
#include "clut.h"

// int_mode
#define NON_INTERLACED	0
#define INTERLACED	1

// ntsc_pal
#define NTSC		2
#define PAL		3

// field_mode
#define FRAME		1
#define FIELD		2

#define MAX_TRANSFER	4096  //16k byte 

int disp_w = 440;
int disp_h = 290;

//---------------------------------------------------------------------------
typedef struct
{
	uint16 ntsc_pal;
	uint16 width;
	uint16 height;
	uint16 psm;
	uint16 bpp;
	uint16 magh;
} vmode_t __attribute__((aligned(16)));

vmode_t vmodes[] = {
	 {PAL, 256, 320, 0, 32, 10}		// PAL_256_256_32
	,{PAL, 320, 256, 0, 32, 8}		// PAL_320_256_32
	,{PAL, 384, 256, 0, 32, 7}		// PAL_384_256_32
	,{PAL, 512, 320, 0, 32, 6}		// PAL_512_256_32
	,{PAL, 640, 256, 0, 32, 4}		// PAL_640_256_32
	,{PAL, 320, 256, 2, 16, 8}		// PAL_320_256_16

	,{NTSC, 256, 224, 0, 32, 10}	// PAL_256_512_32
	,{NTSC, 320, 224, 0, 32, 8}		// PAL_320_256_32
	,{NTSC, 384, 224, 0, 32, 7}		// PAL_384_256_32
	,{NTSC, 512, 320, 0, 32, 6}		// PAL_512_256_32
	,{NTSC, 640, 224, 0, 32, 4}		// PAL_640_256_32
};

static vmode_t *cur_mode;

//---------------------------------------------------------------------------
static uint16	g2_max_x=0;		// current resolution max coordinates
static uint16	g2_max_y=0;

static uint8	g2_col_r=0;		// current draw color
static uint8	g2_col_g=0;
static uint8	g2_col_b=0;

static uint8	g2_fill_r=0;	// current fill color
static uint8	g2_fill_g=0;
static uint8	g2_fill_b=0;

static uint16	g2_view_x0=0;	// current viewport coordinates
static uint16	g2_view_x1=1;
static uint16	g2_view_y0=0;
static uint16	g2_view_y1=1;

static uint32	g2_frame_addr[2] = {0,0};	// address of both frame buffers in GS memory
static uint32	g2_texbuf_addr   = 0;		// address of texture buffer in GS memory

static uint32	g2_fontbuf_addr  = 0;		// address of font buffer
static uint32	g2_fontbuf_w     = 128;		// font buffer width
static uint32	g2_fontbuf_h     = 64;		// font buffer height
static uint16	*g2_font_tc      = 0;		// pointer to texture coordinate buffer
static uint16	g2_font_spacing  = 1;		// pixels between each drawn character
static uint16	g2_font_mag      = 1;		// magnification factor

static uint8	g2_visible_frame;			// Identifies the frame buffer to display
static uint8	g2_active_frame;			// Identifies the frame buffer to direct drawing to

static uint32	gs_mem_current;	// points to current GS memory allocation point
static uint16	gs_origin_x;	// used for mapping Primitive to Window coordinate systems
static uint16	gs_origin_y;

uint32 imageBuff[16384] __attribute__ ((section (".sdata"))) __attribute__((aligned(64))); //MAX_TRANSFER*4     // temporary image buffer - 32it pixels are assembled here from 16bit pixels 

//---------------------------------------------------------------------------
DECLARE_GS_PACKET(gs_dma_buf,50);

//---------------------------------------------------------------------------
int g2_init(g2_video_mode mode)
{
vmode_t *v;

	v = &(vmodes[mode]);
	cur_mode = v;

	g2_max_x = v->width - 1;
	g2_max_y = v->height - 1;

	g2_view_x0 = 0;
	g2_view_y0 = 0;
	g2_view_x1 = g2_max_x;
	g2_view_y1 = g2_max_y;

	gs_origin_x = 1024;
	gs_origin_y = 1024;

	gs_mem_current = 0;		// nothing allocated yet

	g2_visible_frame = 0;	// display frame 0
	g2_active_frame  = 0;	// draw to frame 0

	// - Initialize the DMA.
	// - Writes a 0 to most of the DMA registers.
	dma_reset();

	// - Sets the RESET bit if the GS CSR register.
	GS_RESET();

	// - Can someone please tell me what the sync.p
	// instruction does. Synchronizes something :-)
	__asm__("sync.p\n\
		 nop");

	// - Sets up the GS IMR register (i guess).
	// - The IMR register is used to mask and unmask certain interrupts,
	//   for example VSync and HSync. We'll use this properly in Tutorial 2.
	// - Does anyone have code to do this without using the 0x71 syscall?
	// - I havn't gotten around to looking at any PS2 bios code yet.
	gs_set_imr();

	// - Use syscall 0x02 to setup some video mode stuff.
	// - Pretty self explanatory I think.
	// - Does anyone have code to do this without using the syscall? It looks
	//   like it should only set the SMODE2 register, but if I remove this syscall
	//   and set the SMODE2 register myself, it donesn't work. What else does
	//   syscall 0x02 do?
	gs_set_crtc(NON_INTERLACED, v->ntsc_pal, FRAME);

	// - I havn't attempted to understand what the Alpha parameters can do. They
	//   have been blindly copied from the 3stars demo (although they don't seem
	//   do have any impact in this simple 2D code.
	GS_SET_PMODE(
		0,		// ReadCircuit1 OFF
		1,		// ReadCircuit2 ON
		1,		// Use ALP register for Alpha Blending
		1,		// Alpha Value of ReadCircuit2 for output selection
		0,		// Blend Alpha with the output of ReadCircuit2
		0xFF	// Alpha Value = 1.0
	);
/*
	// - Non needed if we use gs_set_crt()
	GS_SET_SMODE2(
		0,		// Non-Interlaced mode
		1,		// FRAME mode (read every line)
		0		// VESA DPMS Mode = ON		??? please explain ???
	);
*/
	GS_SET_DISPFB2(
		0,				// Frame Buffer base pointer = 0 (Address/8192)
		v->width/64,	// Buffer Width (Pixels/64)
		v->psm,			// Pixel Storage Format
		0,				// Upper Left X in Buffer = 0
		0				// Upper Left Y in Buffer = 0
	);

	// Why doesn't (0, 0) equal the very top-left of the TV?
	GS_SET_DISPLAY2(
//		656,		// X position in the display area (in VCK units)
		606,		// X position in the display area (in VCK units)
		23,			// Y position in the display area (in Raster units)
		v->magh-1,	// Horizontal Magnification - 1
		0,						// Vertical Magnification = 1x
		v->width*v->magh-1,		// Display area width  - 1 (in VCK units) (Width*HMag-1)
		v->height-1				// Display area height - 1 (in pixels)	  (Height-1)
	);

	GS_SET_BGCOLOR(
		0,	// RED
		0,	// GREEN
		0	// BLUE
	);


	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 6, 1, 0, 0, 0);

	// Use drawing parameters from PRIM register
	GIF_DATA_AD(gs_dma_buf, prmodecont, 1);

	// Setup frame buffers. Point to 0 initially.
	GIF_DATA_AD(gs_dma_buf, frame_1,
		GS_FRAME(
			0,					// FrameBuffer base pointer = 0 (Address/8192)
			v->width/64,		// Frame buffer width (Pixels/64)
			v->psm,				// Pixel Storage Format
			0));

	// Save address and advance GS memory pointer by buffer size (in bytes)
	// Do this for both frame buffers.
	g2_frame_addr[0] = gs_mem_current;
	gs_mem_current += v->width * v->height * (v->bpp/8);

	g2_frame_addr[1] = gs_mem_current;
	gs_mem_current += v->width * v->height * (v->bpp/8);

	// Displacement between Primitive and Window coordinate systems.
	GIF_DATA_AD(gs_dma_buf, xyoffset_1,
		GS_XYOFFSET(
			gs_origin_x<<4,
			gs_origin_y<<4));

	// Clip to frame buffer.
	GIF_DATA_AD(gs_dma_buf, scissor_1,
		GS_SCISSOR(
			0,
			g2_max_x,
			0,
			g2_max_y));

	// Create a single 256x128 font buffer
	g2_fontbuf_addr = gs_mem_current;
	gs_mem_current += g2_fontbuf_w * g2_fontbuf_h * (v->bpp/8);

	// Create a texture buffer as big as the screen.
	// Just save the address advance GS memory pointer by buffer size (in bytes)
	// The TEX registers are set later, when drawing.
	g2_texbuf_addr = gs_mem_current;
	gs_mem_current += v->width * v->height * (v->bpp/8);

	// Setup test_1 register to allow transparent texture regions where A=0
	GIF_DATA_AD(gs_dma_buf, test_1,
		GS_TEST(
			1,						// Alpha Test ON
			ATST_NOTEQUAL, 0x00,	// Reject pixels with A=0
			AFAIL_KEEP,				// Don't update frame or Z buffers
			0, 0, 0, 0));			// No Destination Alpha or Z-Buffer Tests

	// Setup the ALPHA_1 register to correctly blend edges of
	// pre-antialiased fonts using Alpha Blending stage.
	// The blending formula is
	//   PIXEL=(SRC-FRAME)*SRC_ALPHA>>7+FRAME
	GIF_DATA_AD(gs_dma_buf, alpha_1,
		GS_ALPHA(
			0,			// A - source
			1, 			// B - frame buffer
			0,			// C - alpha from source
			1, 			// D - frame buffer
			0));		// FIX - not needed

	SEND_GS_PACKET(gs_dma_buf);

	return(SUCCESS);
}

//---------------------------------------------------------------------------
void g2_end(void)
{
	GS_RESET();
}

//---------------------------------------------------------------------------
void g2_put_pixel(int16 x, int16 y)
{
	x += gs_origin_x;
	y += gs_origin_y;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_POINT, 0, 0, 0, 0, 0, 0, 0, 0));

	GIF_DATA_AD(gs_dma_buf, rgbaq,
		GS_RGBAQ(g2_col_r, g2_col_g, g2_col_b, 0x80, 0));

	// The XYZ coordinates are actually floating point numbers between
	// 0 and 4096 represented as unsigned integers where the lowest order
	// four bits are the fractional point. That's why all coordinates are
	// shifted left 4 bits.
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x<<4, y<<4, 0));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_line(int16 x0, int16 y0, int16 x1, int16 y1)
{
	x0 += gs_origin_x;
	y0 += gs_origin_y;
	x1 += gs_origin_x;
	y1 += gs_origin_y;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_LINE, 0, 0, 0, 0, 0, 0, 0, 0));

	GIF_DATA_AD(gs_dma_buf, rgbaq,
		GS_RGBAQ(g2_col_r, g2_col_g, g2_col_b, 0x80, 0));

	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x1<<4, y1<<4, 0));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_rect(int16 x0, int16 y0, int16 x1, int16 y1)
{
	x0 += gs_origin_x;
	y0 += gs_origin_y;
	x1 += gs_origin_x;
	y1 += gs_origin_y;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 7, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_LINE_STRIP, 0, 0, 0, 0, 0, 0, 0, 0));

	GIF_DATA_AD(gs_dma_buf, rgbaq,
		GS_RGBAQ(g2_col_r, g2_col_g, g2_col_b, 0x80, 0));

	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x1<<4, y0<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x1<<4, y1<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y1<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_fill_rect(int16 x0, int16 y0, int16 x1, int16 y1)
{
	x0 += gs_origin_x;
	y0 += gs_origin_y;
	x1 += gs_origin_x;
	y1 += gs_origin_y;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0));

	GIF_DATA_AD(gs_dma_buf, rgbaq,
		GS_RGBAQ(g2_fill_r, g2_fill_g, g2_fill_b, 0x80, 0));

	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));

	// It looks like the default operation for the SPRITE primitive is to
	// not draw the right and bottom 'lines' of the rectangle refined by
	// the parameters. Add +1 to change this.
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2((x1+1)<<4, (y1+1)<<4, 0));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_put_image(int16 x, int16 y, int16 w, int16 h, int16 dw, int16 dh, const uint32 *data)
{
	x += gs_origin_x;
	y += gs_origin_y;

	// - Call this to copy the texture data from EE memory to GS memory.
	// - The g2_texbuf_addr variable holds the byte address of the
	//   'texture buffer' and is setup in g2_init() to be just after
	//   the frame buffer(s). When only the standard resolutions are
	//   used this buffer is guaranteed to be correctly aligned on 256
	//   bytes.
	gs_load_texture(0, 0, w, h, (uint32)data, g2_texbuf_addr, g2_max_x+1);

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 6, 1, 0, 0, 0);

	// Setup the Texture Buffer register. Note the width and height! They
	// must both be a power of 2.
	GIF_DATA_AD(gs_dma_buf, tex0_1,
		GS_TEX0(
			g2_texbuf_addr/256,	// base pointer
			(g2_max_x+1)/64,	// width
			0,					// 32bit RGBA
			gs_texture_wh(w),	// width
			gs_texture_wh(h),	// height
			1,					// RGBA
			TEX_DECAL,			// just overwrite existing pixels
			0,0,0,0,0));

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_SPRITE,
			0,			// flat shading
			1, 			// texture mapping ON
			0, 1, 0,	// no fog, use alpha, no antialiasing
			1, 			// use UV register for coordinates.
			0,
			0));

	// Texture and vertex coordinates are specified consistently, with
	// the last four bits being the decimal part (always X.0 here).

	// Top/Left, texture coordinates (0, 0).
	GIF_DATA_AD(gs_dma_buf, uv,    GS_UV(0, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2(x<<4, y<<4, 0));

	// Bottom/Right, texture coordinates (w, h).
	GIF_DATA_AD(gs_dma_buf, uv,    GS_UV(w<<4, h<<4));
	GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2((x+dw)<<4, (y+dh)<<4, 0));

	// Finally send the command buffer to the GIF.
	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_put_image16(uint16 x, uint16 y, uint16 w, uint16 h, uint16* data)
{
	uint32 i;		// DMA buffer loop counter
	uint32 frac;		// flag for whether to run a fractional buffer or not
	uint32 current;		// number of pixels to transfer in current DMA
	uint32 qtotal;		// total number of qwords of data to transfer
	
	//x += gs_origin_x;
	//y += gs_origin_y;
	
	//uint32 imageBuff[16384]; //MAX_TRANSFER*4     // temporary image buffer - 32it pixels are assembled here from 16bit pixels 
	uint32    imageOffs = 0;
	uint32    buffOffs;
	uint32    imageMax = w * h;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 4, 1, 0, 0, 0);
	GIF_DATA_AD(gs_dma_buf, bitbltbuf,
 		GS_BITBLTBUF(0, 0, 0,
			0,				// frame buffer address
			(g2_max_x+1)>>6, // -> /64		// frame buffer width
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

	qtotal = w*h>>2;	//w*h/4;	// total number of quadwords to transfer.
	current = qtotal % MAX_TRANSFER;	// work out if a partial buffer transfer is needed.
	frac=1;					// assume yes.
	if(!current)				// if there is no need for partial buffer
	{
		current = MAX_TRANSFER;		// start with a full buffer
		frac=0;				// and don't do extra partial buffer first
	}
	for(i=0; i<(qtotal/MAX_TRANSFER)+frac; i++)
	{
		//fill temporary buffer data
                for (buffOffs = 0; buffOffs < (current<<2) &&  imageOffs < imageMax; buffOffs++, imageOffs++) // *4
                {
			// global CLUT
			imageBuff[buffOffs] = CLUT[(data[imageOffs])];
         	}

		BEGIN_GS_PACKET(gs_dma_buf);
		GIF_TAG_IMG(gs_dma_buf, current);
		SEND_GS_PACKET(gs_dma_buf);

		SET_QWC(GIF_QWC, current);
		SET_MADR(GIF_MADR, imageBuff, 0);
		SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);
		DMA_WAIT(GIF_CHCR);

		//data += current*4;
		current = MAX_TRANSFER;		// after the first one, all are full buffers
	}
}


//---------------------------------------------------------------------------
void g2_put_image_tc(int16 x, int16 y, int16 w, int16 h, uint32 *data,
				int16 iw, int16 ih, int16 u0, int16 v0, int16 u1, int16 v1)
{
	x += gs_origin_x;
	y += gs_origin_y;

	// - Call this to copy the texture data from EE memory to GS memory.
	// - The g2_texbuf_addr variable holds the byte address of the
	//   'texture buffer' and is setup in g2_init() to be just after
	//   the frame buffer(s). When only the standard resolutions are
	//   used this buffer is guaranteed to be correctly aligned on 256
	//   bytes.
	gs_load_texture(0, 0, iw, ih, (uint32)data, g2_texbuf_addr, g2_max_x+1);

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 6, 1, 0, 0, 0);

	// Setup the Texture Buffer register. Note the width and height! They
	// must both be a power of 2.
	GIF_DATA_AD(gs_dma_buf, tex0_1,
		GS_TEX0(
			g2_texbuf_addr/256,	// base pointer
			(g2_max_x+1)/64,	// width
			0,					// 32bit RGBA
			gs_texture_wh(iw),	// width
			gs_texture_wh(ih),	// height
			1,					// RGBA
			TEX_DECAL,			// just overwrite existing pixels
			0,0,0,0,0));

	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_SPRITE,
			0,			// flat shading
			1, 			// texture mapping ON
			0, 0, 0,	// no fog, alpha, or antialiasing
			1, 			// use UV register for coordinates.
			0,
			0));

	// Texture and vertex coordinates are specified consistently, with
	// the last four bits being the decimal part (always X.0 here).

	// Top/Left, texture coordinates (0, 0).
	GIF_DATA_AD(gs_dma_buf, uv,    GS_UV(u0<<4, v0<<4));
	GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2(x<<4, y<<4, 0));

	// Bottom/Right, texture coordinates (w, h).
	GIF_DATA_AD(gs_dma_buf, uv,    GS_UV(u1<<4, v1<<4));
	GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2((x+w)<<4, (y+h)<<4, 0));

	// Finally send the command buffer to the GIF.
	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_set_font(uint32 *data, uint16 w, uint16 h, uint16 *tc)
{
	g2_font_tc = tc;
	gs_load_texture(0, 0, w, h, (uint32)data, g2_fontbuf_addr, g2_fontbuf_w);
}

//---------------------------------------------------------------------------
void g2_out_text(int16 x, int16 y, char *str)
{
char	c;				// current character
uint16	*tc;			// current texture coordinates [4]
uint16	x0, y0, x1, y1;	// rectangle for current character
uint16	w, h;			// width and height of above rectangle

	x += gs_origin_x;
	y += gs_origin_y;

	c = *str;
	while(c)
	{
		// Read the texture coordinates for current character
		tc = &g2_font_tc[c*4];
		x0 = *tc++;
		y0 = *tc++;
		x1 = *tc++;
		y1 = *tc++;
		w  = x1-x0+1;
		h  = y1-y0+1;

		// Draw a sprite with current character mapped onto it
		BEGIN_GS_PACKET(gs_dma_buf);
		GIF_TAG_AD(gs_dma_buf, 6, 1, 0, 0, 0);

		GIF_DATA_AD(gs_dma_buf, tex0_1,
			GS_TEX0(
				g2_fontbuf_addr/256,		// base pointer
				(g2_fontbuf_w)/64,			// width
				0,							// 32bit RGBA
				gs_texture_wh(g2_fontbuf_w),// width
				gs_texture_wh(g2_fontbuf_w),// height
				1,							// RGBA
				TEX_DECAL,					// just overwrite existing pixels
				0,0,0,0,0));
/*
		GIF_DATA_AD(gs_dma_buf, tex1_1,
			GS_TEX1(
				0, 0,
				FILTER_LINEAR,
				FILTER_LINEAR,
				0, 0, 0));

		GIF_DATA_AD(gs_dma_buf, clamp_1, 0x05);
*/
		GIF_DATA_AD(gs_dma_buf, prim,
			GS_PRIM(PRIM_SPRITE,
				0,			// flat shading
				1, 			// texture mapping ON
				0, 1, 0,	// no fog or antialiasing, but use alpha
				1, 			// use UV register for coordinates.
				0,
				0));

		GIF_DATA_AD(gs_dma_buf, uv,    GS_UV(x0<<4, y0<<4));
		GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2(x<<4, y<<4, 0));
		GIF_DATA_AD(gs_dma_buf, uv,    GS_UV((x1+1)<<4, (y1+1)<<4));
		GIF_DATA_AD(gs_dma_buf, xyz2,  GS_XYZ2(
				(x+w*g2_font_mag)<<4, (y+h*g2_font_mag)<<4, 0));

		SEND_GS_PACKET(gs_dma_buf);

		// Advance drawing position
		x += (w + g2_font_spacing) * g2_font_mag;

		// Get next character
		str++;
		c = *str;
	}
}

//---------------------------------------------------------------------------
void g2_set_font_color(uint8 red, uint8 green, uint8 blue)
{
uint64 x0;
uint64 y0;
uint64 x1;
uint64 y1;

	x0 = gs_origin_x-10;
	y0 = gs_origin_y-10;
	x1 = g2_fontbuf_w + gs_origin_x;
	y1 = g2_fontbuf_h + gs_origin_y;

	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 6, 1, 0, 0, 0);

	// Restore the frame buffer to the current active frame buffer.
	GIF_DATA_AD(gs_dma_buf, frame_1,
		GS_FRAME(
			g2_fontbuf_addr/8192,
			cur_mode->width/64,
			cur_mode->psm,
			0));

	// Draw the colored rectangle over the entire Font Bitmap.
	GIF_DATA_AD(gs_dma_buf, prim,
		GS_PRIM(PRIM_LINE, 0, 0, 0, 0, 0, 0, 0, 0));
//		GS_PRIM(PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0));

	GIF_DATA_AD(gs_dma_buf, rgbaq,
		GS_RGBAQ(red, green, blue, 0x80, 0));

	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x0<<4, y0<<4, 0));
	GIF_DATA_AD(gs_dma_buf, xyz2, GS_XYZ2(x1<<4, y1<<4, 0));

	// Restore the frame buffer to the current active frame buffer.
	GIF_DATA_AD(gs_dma_buf, frame_1,
		GS_FRAME(
			g2_frame_addr[g2_active_frame]/8192,
			cur_mode->width/64,
			cur_mode->psm,
			0));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_set_font_spacing(uint16 s)
{
	g2_font_spacing = s;
}

//---------------------------------------------------------------------------
uint16 g2_get_font_spacing(void)
{
	return(g2_font_spacing);
}

//---------------------------------------------------------------------------
void g2_set_font_mag(uint16 m)
{
	g2_font_mag = m;
}

//---------------------------------------------------------------------------
uint16 g2_get_font_mag(void)
{
	return(g2_font_mag);
}

//---------------------------------------------------------------------------
void g2_set_visible_frame(uint8 frame)
{
	GS_SET_DISPFB2(
		g2_frame_addr[frame]/8192,	// Frame Buffer base pointer = Address/8192
		cur_mode->width/64,			// Buffer Width (Pixels/64)
		cur_mode->psm,				// Pixel Storage Format
		0,							// Upper Left X in Buffer = 0
		0							// Upper Left Y in Buffer = 0
	);

	g2_visible_frame = frame;
}

//---------------------------------------------------------------------------
void g2_set_active_frame(uint8 frame)
{
	BEGIN_GS_PACKET(gs_dma_buf);
	GIF_TAG_AD(gs_dma_buf, 1, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, frame_1,
		GS_FRAME(
			g2_frame_addr[frame]/8192,	// FrameBuffer base pointer = Address/8192
			cur_mode->width/64,			// Frame buffer width (Pixels/64)
			cur_mode->psm,				// Pixel Storage Format
			0));

	SEND_GS_PACKET(gs_dma_buf);

	g2_active_frame = frame;
}

//---------------------------------------------------------------------------
uint8 g2_get_visible_frame(void)
{
	return(g2_visible_frame);
}

//---------------------------------------------------------------------------
uint8 g2_get_active_frame(void)
{
	return(g2_active_frame);
}

//---------------------------------------------------------------------------
void g2_wait_vsync(void)
{
	*CSR = *CSR & 8;
	while(!(*CSR & 8)) ;
}

//---------------------------------------------------------------------------
void g2_wait_hsync(void)
{
	*CSR = *CSR & 4;
	while(!(*CSR & 4));
}

//---------------------------------------------------------------------------
void g2_set_viewport(uint16 x0, uint16 y0, uint16 x1, uint16 y1)
{
	g2_view_x0 = x0;
	g2_view_x1 = x1;
	g2_view_y0 = y0;
	g2_view_y1 = y1;

	BEGIN_GS_PACKET(gs_dma_buf);

	GIF_TAG_AD(gs_dma_buf, 1, 1, 0, 0, 0);

	GIF_DATA_AD(gs_dma_buf, scissor_1,
		GS_SCISSOR(x0, x1, y0, y1));

	SEND_GS_PACKET(gs_dma_buf);
}

//---------------------------------------------------------------------------
void g2_get_viewport(uint16 *x0, uint16 *y0, uint16 *x1, uint16 *y1)
{
	*x0 = g2_view_x0;
	*x1 = g2_view_x1;
	*y0 = g2_view_y0;
	*y1 = g2_view_y1;
}

//---------------------------------------------------------------------------
uint16 g2_get_max_x(void)
{
	return(g2_max_x);
}

//---------------------------------------------------------------------------
uint16 g2_get_max_y(void)
{
	return(g2_max_y);
}

//---------------------------------------------------------------------------
void g2_get_color(uint8 *r, uint8 *g, uint8 *b)
{
	*r = g2_col_r;
	*g = g2_col_g;
	*b = g2_col_b;
}

//---------------------------------------------------------------------------
void g2_get_fill_color(uint8 *r, uint8 *g, uint8 *b)
{
	*r = g2_fill_r;
	*g = g2_fill_g;
	*b = g2_fill_b;
}

//---------------------------------------------------------------------------
void g2_set_color(uint8 r, uint8 g, uint8 b)
{
	g2_col_r = r;
	g2_col_g = g;
	g2_col_b = b;
}

//---------------------------------------------------------------------------
void g2_set_fill_color(uint8 r, uint8 g, uint8 b)
{
	g2_fill_r = r;
	g2_fill_g = g;
	g2_fill_b = b;
}

void g2_flip_buffers(void) {
  g2_wait_vsync();
  g2_set_visible_frame(1 - g2_get_visible_frame());
  g2_set_active_frame(1 - g2_get_active_frame());
}

