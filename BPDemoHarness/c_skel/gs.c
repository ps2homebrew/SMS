/* Basic GS handling functions */
#include <kernel.h>
#include <tamtypes.h>
#include "../harness.h"
#include "ps2gs.h"
#include "dma.h"
#include "gs.h"
#include "gif.h"
#include "prim.h"
#include "vram_malloc.h"

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

DECLARE_GS_PACKET(dma_buf, 4096); /*  Declare a 50 element dma buffer */

static u32 fb_pointers[2]; /* Pointers to our two frame buffers */
static u32 zbuf_pointer;   /* Poniter to z buffer */
static u32 tex_pointer;    /* Pointer to text vram */
static int curr_fb; /* The current fb number */
static int vis_fb;  /* The current visible fb number */
int offs_x, offs_y; /* Offset of framebuffer to display */
static int max_x, max_y;

int init_gs()

{
  u32 curr_size;

  reset_malloc();
  offs_x = 1024;
  offs_y = 1024;
  curr_fb = 0;
  vis_fb = 0;
  max_x = SCR_W - 1; 
  max_y = SCR_H - 1; 
  curr_size = SCR_W * SCR_H; /* Assuming PSM0 */
  fb_pointers[0] = vram_malloc(curr_size);
  fb_pointers[1] = vram_malloc(curr_size);
  zbuf_pointer = vram_malloc(curr_size);
  tex_pointer = vram_malloc(curr_size);

  /* Statically allocate vram. No checking done */

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
		 SCR_W / 64,	// Buffer Width (Address/64)
		 SCR_PSM,			// Pixel Storage Format
		 0,				// Upper Left X in Buffer = 0
		 0				// Upper Left Y in Buffer = 0
		 );
  
  
  GS_SET_BGCOLOR(
		 0,	// RED
		 0,	// GREEN
		 0	// BLUE
		 );
  
  BEGIN_GS_PACKET(dma_buf);
  
  GIF_TAG_AD(dma_buf, 6, 1, 0, 0, 0);
  
  GIF_DATA_AD(dma_buf, PS2_GS_PRMODECONT, 1);
  GIF_DATA_AD(dma_buf, PS2_GS_FRAME_1, PS2_GS_SETREG_FRAME_1(0, SCR_W / 64, SCR_PSM, 0));
  GIF_DATA_AD(dma_buf, PS2_GS_XYOFFSET_1, PS2_GS_SETREG_XYOFFSET_1(offs_x << 4, offs_y << 4));
  GIF_DATA_AD(dma_buf, PS2_GS_SCISSOR_1,PS2_GS_SETREG_SCISSOR_1(0, SCR_W-1, 0, SCR_H - 1));      
  GIF_DATA_AD(dma_buf, PS2_GS_ZBUF_1, PS2_GS_SETREG_ZBUF_1(zbuf_pointer / 2048, 0, 0));
  GIF_DATA_AD(dma_buf, PS2_GS_TEST_1, PS2_GS_SETREG_TEST(1, 7, 0xFF, 0, 0, 0, 0, 0));
  
  SEND_GS_PACKET(dma_buf);
  
  return 1; /* Return success */
}

#define MAX_TRANSFER	16384

void load_image(void *img, int dest, int fbw, int psm, int x, int y, int w, int h)

{
  u32 i;			// DMA buffer loop counter
  u32 frac;		// flag for whether to run a fractional buffer or not
  u32 current;		// number of pixels to transfer in current DMA
  u32 qtotal;		// total number of qwords of data to transfer
  u32 data_adr;

  data_adr = (u32) img;  
  BEGIN_GS_PACKET(dma_buf);
  GIF_TAG_AD(dma_buf, 4, 1, 0, 0, 0);
  GIF_DATA_AD(dma_buf, PS2_GS_BITBLTBUF,
	      PS2_GS_SETREG_BITBLTBUF(0, 0, 0,
				      dest/64,		// frame buffer address
				      fbw/64,			// frame buffer width
				      0));
  GIF_DATA_AD(dma_buf, PS2_GS_TRXPOS,
	      PS2_GS_SETREG_TRXPOS(
				   0,
				   0,
				   x,
				   y,
				   0));	// left to right/top to bottom
  GIF_DATA_AD(dma_buf, PS2_GS_TRXREG, PS2_GS_SETREG_TRXREG(w, h));
  GIF_DATA_AD(dma_buf, PS2_GS_TRXDIR, PS2_GS_SETREG_TRXDIR(0));
  SEND_GS_PACKET(dma_buf);

  qtotal = w*h/4; 

  current = qtotal % MAX_TRANSFER;// work out if a partial buffer transfer is needed.
  frac=1;							// assume yes.
  if(!current)					// if there is no need for partial buffer
    {
      current = MAX_TRANSFER;		// start with a full buffer
      frac=0;						// and don't do extra partial buffer first
    }
  for(i=0; i<(qtotal/MAX_TRANSFER)+frac; i++)
    {
      BEGIN_GS_PACKET(dma_buf);
      GIF_TAG_IMG(dma_buf, current);
      SEND_GS_PACKET(dma_buf);
      
      SET_QWC(GIF_QWC, current);
      SET_MADR(GIF_MADR, data_adr, 0);
      SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);
      DMA_WAIT(GIF_CHCR);
      
      data_adr += current*16;
      current = MAX_TRANSFER;		// after the first one, all are full buffers
    }
}

void clr_scr(u32 col) /* Clear the screen using col */

{
  fill_rect(0, 0, max_x, max_y, 0, col);
}

void set_visible_fb(u8 fb)

{
  GS_SET_DISPFB2(fb_pointers[fb & 1] / 2048, SCR_W / 64, SCR_PSM, 0, 0);
}

void set_active_fb(u8 fb)

{
  BEGIN_GS_PACKET(dma_buf);
  GIF_TAG_AD(dma_buf, 1, 1, 0, 0, 0);
  
  GIF_DATA_AD(dma_buf, PS2_GS_FRAME_1, PS2_GS_SETREG_FRAME_1(fb_pointers[fb&1]/2048, SCR_W / 64, SCR_PSM, 0));
  
  SEND_GS_PACKET(dma_buf);
}

void wait_vsync(void)

{
  *CSR = *CSR & 8;
  while(!(*CSR & 8));
}

int get_max_x(void)

{
  return max_x;
}

int get_max_y(void)

{
  return max_y;
}

u32 get_tex_pointer(void)

{
  return tex_pointer;
}

u32 get_fb_pointer(int no)

{
  return fb_pointers[no & 1];
}

void set_zbufcmp(int cmpmode)

{
  BEGIN_GS_PACKET(dma_buf);
  GIF_TAG_AD(dma_buf, 1, 1, 0, 0, 0); 

  GIF_DATA_AD(dma_buf, PS2_GS_TEST_1, PS2_GS_SETREG_TEST(1, 7, 0xFF, 0, 0, 0, 1, cmpmode)); /* Set up z buffer test */

  SEND_GS_PACKET(dma_buf);
}

void set_bg_colour(u32 r, u32 g, u32 b)

{
  GS_SET_BGCOLOR(r,g,b);
}

int is_pal(void)

{
  if(*((char *)0x1FC80000 - 0xAE) == 'E')
    return 1;

  return 0;
}
