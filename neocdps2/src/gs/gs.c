/*
  _____     ___ ____
   ____|   |    ____|      PS2 OpenSource Project
  |     ___|   |____       (C) 2001 Nick Van Veen (nickvv@xtra.co.nz)
  ------------------------------------------------------------------------
  gs.c
  		Functions related to the Graphics Synthesiser.

*/

#include <tamtypes.h>
#include <kernel.h>
#include <string.h>
#include "hw.h"
#include "gs.h"

// draw_env0/1:	Structures used to set the current drawing enviroment. The only difference
//				between each is the value of the fb base pointer (stored in frame_1[0])
struct {
	u64 dmatag[2];
	u64 giftag[2];
	u64 frame_1[2]; // fbp, fbw, psm
	u64 xyoffset_1[2]; // x/y = 0
	u64 zbuf_1[2]; // zbp, zbpsm
	u64 prmodecont[2]; // use PRIM register
	u64 dthe[2]; // dither off
	u64 colclamp[2]; // colour clamp ON
	u64 test1_1[2]; // make all pixels pass ztest, so the following prim can clear the screen
	u64 scissor_1[2]; // scissor w/h = screen w/h
	u64 prim[2]; // triangle stip (used to clear the screen).
	u64 rgbaq[2]; // black
	u64 xyz3_1[2]; // vertex co-ordinates for triangle strip
	u64 xyz3_2[2];
	u64 xyz2_1[2];
	u64 xyz2_2[2];
	u64 test1_2[2]; // depth testing on, pixels with Z greater than Z buffer value pass.
}draw_env0 __attribute__((aligned(16)));

struct {
	u64 dmatag[2];
	u64 giftag[2];
	u64 frame_1[2];
	u64 xyoffset_1[2];
	u64 zbuf_1[2];
	u64 prmodecont[2];
	u64 dthe[2];
	u64 colclamp[2];
	u64 test1_1[2];
	u64 scissor_1[2];
	u64 prim[2];
	u64 rgbaq[2];
	u64 xyz3_1[2];
	u64 xyz3_2[2];
	u64 xyz2_1[2];
	u64 xyz2_2[2];
	u64 test1_2[2];
}draw_env1 __attribute__((aligned(16)));

u64 disp_env[2]; 	// Holds display enviroment information which is copied into the
					// GS register DISPFB1 when GS_SetCrtFrameBuffer is called.

// dx/dy = x/y offset of display on tv screen. width/height = width/height of display.
void GS_SetDispMode(int dx, int dy, int width, int height)
{
	 __asm__(" di ");

	GS_PMODE = 0xFF61;
	//GS_SMODE2 = 0x01; /* Looks like this gets set by sceSetGSCrt */
	GS_DISPFB1 = (width>>6)<<9;
	GS_DISPLAY1 =  (((u64)((height)-1)<<44) |
					((u64)0x9FF<<32) |
					((((2560+width-1)/width)-1)<<23) |
					(dy<<12) |
					(dx*(2560/width)));
	GS_BGCOLOUR = 0;

	 __asm__(" ei ");
}

// Change the framebuffer used to generate the display (buffer = 0 or 1)
void GS_SetCrtFB(int buffer)
{

	GS_DISPFB1 = disp_env[buffer];

}

// Change the framebuffer where primatives are drawn (buffer = 0 or 1)
void GS_SetDrawFB(int buffer)
{

	Dma02Wait();

	if(buffer == 0) SendDma02((void *)&draw_env0);
	else SendDma02((void *)&draw_env1);

}

// Set up the draw_env0/1 structures. Arguments:
// width/height = width/height of the display
// fbp1/2 = vram memory address of framebuffer 1/2
// psm = framebuffer pixel storage format
// zbp = vram memory address of z buffer
// zbpsm = zbuffer pixel storage format
void GS_SetEnv(int width, int height, int fbp1, int fbp2, int psm, int zbp, int zbpsm)
{

	memset(&draw_env0,0,sizeof(draw_env0));
	memset(&draw_env1,0,sizeof(draw_env1));

	draw_env0.dmatag[0] =    DMA_SET_TAG(16,0,0x07,0,0);
	draw_env0.giftag[0] =    GIF_SET_TAG(15, 1, 0, 0, 0, 1);
	draw_env0.giftag[1] =    0x000000000000000e;

	draw_env0.frame_1[0] = GS_SET_FRAME((fbp1 / 0x2000), (width / 64), psm, 0);
	draw_env0.frame_1[1] = 0x4C;

	draw_env0.xyoffset_1[0] = GS_SET_XYOFFSET(0,0);
	draw_env0.xyoffset_1[1] = 0x18;

	draw_env0.zbuf_1[0] =     GS_SET_ZBUF((zbp / 0x2000), zbpsm, 0);
	draw_env0.zbuf_1[1] =     0x4E;

	draw_env0.prmodecont[0] = 0x0000000000000001;
	draw_env0.prmodecont[1] = 0x1A;

	draw_env0.dthe[0]       = 0x0000000000000000;
	draw_env0.dthe[1]       = 0x45;

	draw_env0.colclamp[0]   = 0x0000000000000001;
	draw_env0.colclamp[1]   = 0x46;

	draw_env0.test1_1[0]    = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 0x01);
	draw_env0.test1_1[1]    = 0x47;

	draw_env0.scissor_1[0]  = GS_SET_SCISSOR(0, width, 0, height);
	draw_env0.scissor_1[1]  = 0x40;

	draw_env0.prim[0]       = GS_SET_PRIM(0x04, 0, 0, 0, 0, 0, 0, 0, 0);
	draw_env0.prim[1]       = 0x00;
	draw_env0.rgbaq[0]      = GS_SET_RGBAQ(0,0,0,0,0x3f800000);
	draw_env0.rgbaq[1]      = 0x01;
	draw_env0.xyz3_1[0]     = GS_SET_XYZ(0, 0, 0);
	draw_env0.xyz3_1[1]     = 0x0d;
	draw_env0.xyz3_2[0]     = GS_SET_XYZ(width<<4, 0, 0);
	draw_env0.xyz3_2[1]     = 0x0d;
	draw_env0.xyz2_1[0]     = GS_SET_XYZ(0, height<<4, 0);
	draw_env0.xyz2_1[1]     = 0x05;
	draw_env0.xyz2_2[0]     = GS_SET_XYZ(width<<4, height<<4, 0);
	draw_env0.xyz2_2[1]     = 0x05;

	draw_env0.test1_2[0]    = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 0x03);
	draw_env0.test1_2[1]    = 0x47;

	memcpy(&draw_env1,&draw_env0,sizeof(draw_env0));
	draw_env1.frame_1[0] = GS_SET_FRAME((fbp2 / 0x2000), (width / 64), psm, 0);
	draw_env1.frame_1[1] = 0x4C;

	disp_env[0] = GS_SET_DISPFB((fbp1 / 0x2000), (width / 64), psm, 0, 0);
	disp_env[1] = GS_SET_DISPFB((fbp2 / 0x2000), (width / 64), psm, 0, 0);
}

// mode: 2 = NTSC, 3 = PAL
// interlace: 0 = non-interlace, 1 = interlace
void GS_InitGraph(int mode, int interlace)
{
__asm("
	addiu $29, -8
    sd $4, 0($29)
	addiu $29, -8
 	sd $5, 0($29)

    lui $3, 0x1200
    daddiu $2, $0, 0x0200
    sd $2, 0x1000($3)       # reset GS
    sync.p
    sd $0, 0x1000($3)

    # putIMR thru bios (GsPutIMR)
    ori $4, $0, 0xff00
    addiu $3, $0, 0x0071
    syscall
    nop

    # sceSetGSCrt
 	ld $4, 0($29)           # Interlace/Non-interlace mode
    ld $5, 8($29)           # NTSC/PAL mode
    ori $6, $0, 0           # field mode
    addiu $3, $0, 2
    syscall
    nop

    addiu $29, 16

    jr $31
    nop
");
}
