/*
	ps2.h
	
	by Nicolas Plourde <nicolasplourde@hotmail.com>
	
	Copyright (c) Nicolas Plourde - july 2004
*/

#ifndef PS2_H
#define PS2_H

// MIPS CPU Registsers
#define zero	$0		// Always 0
#define at		$1		// Assembler temporary
#define v0		$2		// Function return
#define v1		$3		// 
#define a0		$4		// Function arguments
#define a1		$5
#define a2		$6
#define a3		$7
#define t0		$8		// Temporaries. No need
#define t1		$9		// to preserve in your
#define t2		$10		// functions.
#define t3		$11
#define t4		$12
#define t5		$13
#define t6		$14
#define t7		$15
#define s0		$16		// Saved Temporaries.
#define s1		$17		// Make sure to restore
#define s2		$18		// to original value
#define s3		$19		// if your function
#define s4		$20		// changes their value.
#define s5		$21
#define s6		$22
#define s7		$23
#define t8		$24		// More Temporaries.
#define t9		$25
#define k0		$26		// Reserved for Kernel
#define k1		$27
#define gp		$28		// Global Pointer
#define sp		$29		// Stack Pointer
#define fp		$30		// Frame Pointer
#define ra		$31		// Function Return Address

// Playstation2 GS Privileged Registers
#define pmode		0x12000000	// Setup CRT Controller
#define smode2		0x12000020	// CRTC Video Settings: PAL/NTCS, Interlace, etc.
#define dispfb1		0x12000070	// Setup the CRTC's Read Circuit 1 data source settings
#define display1	0x12000080	// RC1 display output settings
#define dispfb2		0x12000090	// Setup the CRTC's Read Circuit 2 data source settings
#define display2	0x120000a0	// RC2 display output settings
#define extbuf		0x120000b0	// ...
#define extdata		0x120000c0	// ...
#define extwrite	0x120000d0	// ...
#define bgcolor		0x120000e0	// Set CRTC background color
#define csr			0x12001000	// System status and reset
#define imr			0x12001010	// Interrupt Mask Register
#define busdir		0x12001040	// ...
#define siglblid	0x12001080	// ...

// Playstation2 GS General Purpose Registers
#define prim		0x00	// Select and configure current drawing primitive
#define rgbaq		0x01	// Setup current vertex color
#define st			0x02	// ...
#define uv			0x03	// ...
#define xyzf2		0x04	// Set vertex coordinate
#define xyz2		0x05	// Set vertex coordinate and 'kick' drawing
#define tex0_1		0x06	// ...
#define tex0_2		0x07	// ...
#define clamp_1		0x08	// ...
#define clamp_2		0x09	// ...
#define fog			0x0a	// ...
#define xyzf3		0x0c	// ...
#define xyz3		0x0d	// ...
#define tex1_1		0x14	// ...
#define tex1_2		0x15	// ...
#define tex2_1		0x16	// ...
#define tex2_2		0x17	// ...
#define xyoffset_1	0x18	// Mapping from Primitive to Window coordinate system (Context 1)
#define xyoffset_2	0x19	// Mapping from Primitive to Window coordinate system (Context 2)
#define prmodecont	0x1a	// ...
#define prmode		0x1b	// ...
#define texclut		0x1c	// ...
#define scanmsk		0x22	// ...
#define miptbp1_1	0x34	// ...
#define miptbp1_2	0x35	// ...
#define miptbp2_1	0x36	// ...
#define miptbp2_2	0x37	// ...
#define texa		0x3b	// ...
#define fogcol		0x3d	// ...
#define texflush	0x3f	// ...
#define scissor_1	0x40	// Setup clipping rectangle (Context 1)
#define scissor_2	0x41	// Setup clipping rectangle (Context 2)
#define alpha_1		0x42	// ...
#define alpha_2		0x43	// ...
#define dimx		0x44	// ...
#define dthe		0x45	// ...
#define colclamp	0x46	// ...
#define test_1		0x47	// ...
#define test_2		0x48	// ...
#define pabe		0x49	// ...
#define fba_1		0x4a	// ...
#define fba_2		0x4b	// ...
#define frame_1		0x4c	// Frame buffer settings (Context 1)
#define frame_2		0x4d	// Frame buffer settings (Context 2)
#define zbuf_1		0x4e	// ...
#define zbuf_2		0x4f	// ...
#define bitbltbuf	0x50	// ...
#define trxpos		0x51	// ...
#define trxreg		0x52	// ...
#define trxdir		0x53	// ...
#define hwreg		0x54	// ...
#define signal		0x60	// ...
#define finish		0x61	// ...
#define label		0x62	// ...

// Playstation2 DMA Channel Registers
#define gif_chcr	0x1000a000	// GIF Channel Control Register
#define gif_madr	0x1000a010	// Transfer Address Register
#define gif_qwc		0x1000a020	// Transfer Size Register (in qwords)
#define gif_tadr	0x1000a030	// ...

//---------------------------------------------------------------------------
// CHCR Register - Channel Control Register
//---------------------------------------------------------------------------
#define GIF_CHCR		((volatile uint32 *)(0x1000a000))

#define SET_CHCR(WHICH,DIR,MOD,ASP,TTE,TIE,STR,TAG) \
	*WHICH = \
	((uint32)(DIR)	<< 0)		| \
	((uint32)(MOD)	<< 2)		| \
	((uint32)(ASP)	<< 4)		| \
	((uint32)(TTE)	<< 6)		| \
	((uint32)(TIE)	<< 7)		| \
	((uint32)(STR)	<< 8)		| \
	((uint32)(TAG)	<< 16)

#define DMA_WAIT(WHICH) \
	while((*WHICH) & (1<<8))

//---------------------------------------------------------------------------
// MADR Register - Transfer Address Register
//---------------------------------------------------------------------------
#define GIF_MADR		((volatile uint32 *)(0x1000a010))

#define SET_MADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((uint32)(ADDR)	<< 0)		| \
	((uint32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// TADR Register - Tag Address Register
//---------------------------------------------------------------------------
#define GIF_TADR		((volatile uint32 *)(0x1000a030))

#define SET_TADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((uint32)(ADDR)	<< 0)		| \
	((uint32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// QWC Register - Transfer Data Size Register
//---------------------------------------------------------------------------
#define GIF_QWC		((volatile uint32 *)(0x1000a020))

#define SET_QWC(WHICH,SIZE) \
	*WHICH = (uint32)(SIZE)
	
//---------------------------------------------------------------------------
// CSR Register
//---------------------------------------------------------------------------
#define CSR			((volatile uint64 *)(0x12001000))
#define GS_SET_CSR(SIGNAL,FINISH,HSINT,VSINT,EDWINT,FLUSH,RESET,NFIELD,FIELD,FIFO,REV,ID) \
	*CSR = \
	((uint64)(SIGNAL)	<< 0)		| \
	((uint64)(FINISH)	<< 1)		| \
	((uint64)(HSINT)	<< 2)		| \
	((uint64)(VSINT)	<< 3)		| \
	((uint64)(EDWINT)	<< 4)		| \
	((uint64)(FLUSH)	<< 8)		| \
	((uint64)(RESET)	<< 9)		| \
	((uint64)(NFIELD)	<< 12)		| \
	((uint64)(FIELD)	<< 13)		| \
	((uint64)(FIFO)		<< 14)		| \
	((uint64)(REV)		<< 16)		| \
	((uint64)(ID)		<< 24)

#define GS_RESET() \
	*CSR = ((uint64)(1)	<< 9)

//---------------------------------------------------------------------------
// PMODE Register
//---------------------------------------------------------------------------
#define PMODE		((volatile uint64 *)(0x12000000))
#define GS_SET_PMODE(EN1,EN2,MMOD,AMOD,SLBG,ALP) \
	*PMODE = \
	((uint64)(EN1) 	<< 0) 	| \
	((uint64)(EN2) 	<< 1) 	| \
	((uint64)(001)	<< 2) 	| \
	((uint64)(MMOD)	<< 5) 	| \
	((uint64)(AMOD) << 6) 	| \
	((uint64)(SLBG) << 7) 	| \
	((uint64)(ALP) 	<< 8)

//---------------------------------------------------------------------------
// SMODE2 Register
//---------------------------------------------------------------------------
#define SMODE2		((volatile uint64 *)(0x12000020))
#define GS_SET_SMODE2(INT,FFMD,DPMS) \
	*SMODE2 = \
	((uint64)(INT)	<< 0)	| \
	((uint64)(FFMD)	<< 1)	| \
	((uint64)(DPMS)	<< 2)

//---------------------------------------------------------------------------
// DISPFP1 Register
//---------------------------------------------------------------------------
#define DISPFB1		((volatile uint64 *)(0x12000070))
#define GS_SET_DISPFB1(FBP,FBW,PSM,DBX,DBY) \
	*DISPFB1 = \
	((uint64)(FBP)	<< 0)	| \
	((uint64)(FBW)	<< 9)	| \
	((uint64)(PSM)	<< 15)	| \
	((uint64)(DBX)	<< 32)	| \
	((uint64)(DBY)	<< 43)

//---------------------------------------------------------------------------
// DISPLAY1 Register
//---------------------------------------------------------------------------
#define DISPLAY1	((volatile uint64 *)(0x12000080))
#define GS_SET_DISPLAY1(DX,DY,MAGH,MAGV,DW,DH) \
	*DISPLAY1 = \
	((uint64)(DX)	<< 0)	| \
	((uint64)(DY)	<< 12)	| \
	((uint64)(MAGH)	<< 23)	| \
	((uint64)(MAGV)	<< 27)	| \
	((uint64)(DW)	<< 32)	| \
	((uint64)(DH)	<< 44)

//---------------------------------------------------------------------------
// DISPFP2 Register
//---------------------------------------------------------------------------
#define DISPFB2		((volatile uint64 *)(0x12000090))
#define GS_SET_DISPFB2(FBP,FBW,PSM,DBX,DBY) \
	*DISPFB2 = \
	((uint64)(FBP)	<< 0)	| \
	((uint64)(FBW)	<< 9)	| \
	((uint64)(PSM)	<< 15)	| \
	((uint64)(DBX)	<< 32)	| \
	((uint64)(DBY)	<< 43)

//---------------------------------------------------------------------------
// DISPLAY2 Register
//---------------------------------------------------------------------------
#define DISPLAY2	((volatile uint64 *)(0x120000a0))
#define GS_SET_DISPLAY2(DX,DY,MAGH,MAGV,DW,DH) \
	*DISPLAY2 = \
	((uint64)(DX)	<< 0)	| \
	((uint64)(DY)	<< 12)	| \
	((uint64)(MAGH)	<< 23)	| \
	((uint64)(MAGV)	<< 27)	| \
	((uint64)(DW)	<< 32)	| \
	((uint64)(DH)	<< 44)

//---------------------------------------------------------------------------
// BGCOLOR Register
//---------------------------------------------------------------------------
#define BGCOLOR		((volatile uint64 *)(0x120000e0))
#define GS_SET_BGCOLOR(R,G,B) \
	*BGCOLOR = \
	((uint64)(R)	<< 0)		| \
	((uint64)(G)	<< 8)		| \
	((uint64)(B)	<< 16)

//---------------------------------------------------------------------------
// FRAME_x Register
//---------------------------------------------------------------------------
#define GS_FRAME(FBP,FBW,PSM,FBMSK) \
	(((uint64)(FBP)		<< 0)		| \
	 ((uint64)(FBW)		<< 16)		| \
	 ((uint64)(PSM)		<< 24)		| \
	 ((uint64)(FBMSK)	<< 32))

//---------------------------------------------------------------------------
// PRIM Register
//---------------------------------------------------------------------------
#define PRIM_POINT			0
#define PRIM_LINE			1
#define PRIM_LINE_STRIP		2
#define PRIM_TRI			3
#define PRIM_TRI_STRIP		4
#define PRIM_TRI_FAN		5
#define PRIM_SPRITE			6

#define GS_PRIM(PRI,IIP,TME,FGE,ABE,AA1,FST,CTXT,FIX) \
	(((uint64)(PRI)		<< 0)		| \
	 ((uint64)(IIP)		<< 3)		| \
	 ((uint64)(TME)		<< 4)		| \
	 ((uint64)(FGE)		<< 5)		| \
	 ((uint64)(ABE)		<< 6)		| \
	 ((uint64)(AA1)		<< 7)		| \
	 ((uint64)(FST)		<< 8)		| \
	 ((uint64)(CTXT)	<< 9)		| \
	 ((uint64)(FIX)		<< 10))

//---------------------------------------------------------------------------
// RGBAQ Register
//---------------------------------------------------------------------------
#define GS_RGBAQ(R,G,B,A,Q) \
	(((uint64)(R)		<< 0)		| \
	 ((uint64)(G)		<< 8)		| \
	 ((uint64)(B)		<< 16)		| \
	 ((uint64)(A)		<< 24)		| \
	 ((uint64)(Q)		<< 32))

//---------------------------------------------------------------------------
// SCISSOR_x Register
//---------------------------------------------------------------------------
#define GS_SCISSOR(X0,X1,Y0,Y1) \
	(((uint64)(X0)		<< 0)		| \
	 ((uint64)(X1)		<< 16)		| \
	 ((uint64)(Y0)		<< 32)		| \
	 ((uint64)(Y1)		<< 48))

//---------------------------------------------------------------------------
// XYZ2 Register
//---------------------------------------------------------------------------
#define GS_XYZ2(X,Y,Z)	\
	(((uint64)(X)		<< 0)		| \
	 ((uint64)(Y)		<< 16)		| \
	 ((uint64)(Z)		<< 32))

//---------------------------------------------------------------------------
// XYOFFSET_x Register
//---------------------------------------------------------------------------
#define GS_XYOFFSET(OFX,OFY)	\
	(((uint64)(OFX)		<< 0)		| \
	 ((uint64)(OFY)		<< 32))

//---------------------------------------------------------------------------
// ZBUF_x Register
//---------------------------------------------------------------------------
#define GS_ZBUF(ZBP,PSM,ZMSK)	\
	(((uint64)(ZBP)		<< 0)		| \
	 ((uint64)(PSM)		<< 24)		| \
	 ((uint64)(ZMSK)	<< 32))

//---------------------------------------------------------------------------
#define GIF_AD		0x0e
#define GIF_NOP		0x0f

//---------------------------------------------------------------------------
// GS_PACKET macros
//---------------------------------------------------------------------------

#define DECLARE_GS_PACKET(NAME,ITEMS) \
	uint64 __attribute__((aligned(64))) NAME[ITEMS*2+2]; \
	int NAME##_cur; \
	int NAME##_dma_size

#define BEGIN_GS_PACKET(NAME) \
	NAME##_cur = 0

#define GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,NREG,REGS) \
	NAME##_dma_size = NLOOP+1; \
	NAME[NAME##_cur++] = \
		((uint64)(NLOOP)<< 0)		| \
		((uint64)(EOP)	<< 15)		| \
		((uint64)(PRE)	<< 46)		| \
		((uint64)(PRIM)	<< 47)		| \
		((uint64)(FLG)	<< 58)		| \
		((uint64)(NREG)	<< 60);		\
	NAME[NAME##_cur++] = (uint64)REGS

#define GIF_TAG_AD(NAME,NLOOP,EOP,PRE,PRIM,FLG) \
	GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,1,GIF_AD)

#define GIF_TAG_IMG(NAME,QSIZE) \
	GIF_TAG(NAME,(QSIZE),1,0,0,2,0,0); \
	NAME##_dma_size = 1 \
	
#define GIF_DATA_AD(NAME,REG,DAT) \
	NAME[NAME##_cur++] = (uint64)DAT; \
	NAME[NAME##_cur++] = (uint64)REG

#define SEND_GS_PACKET(NAME) \
	ps2_flush_cache(0);							\
	SET_QWC(GIF_QWC, NAME##_dma_size);			\
	SET_MADR(GIF_MADR, &(NAME), 0);				\
	SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);	\
	DMA_WAIT(GIF_CHCR)
	
//---------------------------------------------------------------------------
// BITBLTBUF Register - Setup Image Transfer Between EE and GS
//   SBP  - Source buffer address (Address/64)
//   SBW  - Source buffer width (Pixels/64)
//   SPSM - Source pixel format (0 = 32bit RGBA)
//   DBP  - Destination buffer address (Address/64)
//   DBW  - Destination buffer width (Pixels/64)
//   DPSM - Destination pixel format (0 = 32bit RGBA)
//
// - When transferring from EE to GS, only the Detination fields
//   need to be set. (Only Source fields for GS->EE, and all for GS->GS).
//---------------------------------------------------------------------------
#define GS_BITBLTBUF(SBP,SBW,SPSM,DBP,DBW,DPSM) \
	(((uint64)(SBP)		<< 0)		| \
	 ((uint64)(SBW)		<< 16)		| \
	 ((uint64)(SPSM)	<< 24)		| \
	 ((uint64)(DBP)		<< 32)		| \
	 ((uint64)(DBW)		<< 48)		| \
	 ((uint64)(DPSM)	<< 56))
	 
//---------------------------------------------------------------------------
// TRXDIR Register - Set Image Transfer Directon, and Start Transfer
//   XDIR - (0=EE->GS, 1=GS->EE, 2=GS-GS, 3=Transmission is deactivated)
//---------------------------------------------------------------------------
#define XDIR_EE_GS			0
#define XDIR_GS_EE			1
#define XDIR_GS_GS			2
#define XDIR_DEACTIVATE		3

#define GS_TRXDIR(XDIR)	\
	((uint64)(XDIR))

//---------------------------------------------------------------------------
// TRXPOS Register - Setup Image Transfer Coordinates
//   SSAX - Source Upper Left X
//   SSAY - Source Upper Left Y
//   DSAX - Destionation Upper Left X
//   DSAY - Destionation Upper Left Y
//   DIR  - Pixel Transmission Order (00 = top left -> bottom right)
//
// - When transferring from EE to GS, only the Detination fields
//   need to be set. (Only Source fields for GS->EE, and all for GS->GS).
//---------------------------------------------------------------------------
#define GS_TRXPOS(SSAX,SSAY,DSAX,DSAY,DIR)	\
	(((uint64)(SSAX)	<< 0)		| \
	 ((uint64)(SSAY)	<< 16)		| \
	 ((uint64)(DSAX)	<< 32)		| \
	 ((uint64)(DSAY)	<< 48)		| \
	 ((uint64)(DIR)		<< 59))

//---------------------------------------------------------------------------
// TRXREG Register - Setup Image Transfer Size
//   RRW - Image Width
//   RRH - Image Height
//---------------------------------------------------------------------------
#define GS_TRXREG(RRW,RRH)	\
	(((uint64)(RRW)	<< 0)		| \
	 ((uint64)(RRH)	<< 32))


////////////////////////////////////////////////////////////////////////

typedef char	int8;
typedef short	int16;
typedef int		int32;
typedef long	int64;

typedef unsigned char	uint8;
typedef unsigned short	uint16;
typedef unsigned int	uint32;
typedef unsigned long	uint64;

typedef struct int128
{
	int64 lo, hi;
} int128 __attribute__((aligned(16)));

typedef struct uint128
{
	uint64 lo, hi;
} uint128 __attribute__((aligned(16)));

typedef struct
{
	uint16 ntsc_pal;
	uint16 width;
	uint16 height;
	uint16 psm;
	uint16 bpp;
	uint16 magh;
	uint16 startx;
	uint16 starty;
} GS_MODE __attribute__((aligned(16)));

#ifdef __cplusplus
extern "C" {
#endif

extern void ps2_flush_cache(int command);
extern void dma_reset();
extern void gs_set_imr();
extern void gs_set_crtc(unsigned char int_mode, unsigned char ntsc_pal_vesa, unsigned char field_mode);
extern void gs_init(GS_MODE mode);
extern int gs_detect_mode();
extern void put_image(uint16 x, uint16 y, uint16 w, uint16 h, uint32 *data);
extern void fill_rect(uint16 x0, uint16 y0, uint16 x1, uint16 y1);

#ifdef __cplusplus
}
#endif

#endif // PS2_H
