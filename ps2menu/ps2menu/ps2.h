// **************************************************************************
// **************************************************************************
//
// PS2 hardware specific stuff, macros, registers, structs etc.
// By Bigboy
//
// **************************************************************************
// **************************************************************************

#ifndef _PS2_H_
#define _PS2_H_

#ifdef __cplusplus
extern "C" {
#endif


//#define	MASTER				// set this when building a CD version (final)
//#define	USESOUND				// If you use this, you can't "reset" via NAPLINK, must be a PS2 reset


#ifdef	MASTER
#define	FILESYS		"cdrom0:"
#define	FILESYS_E	";1"
#else
#define	FILESYS		"host:"
#define	FILESYS_E
#endif



//
// Bigboy's "types" These are much nicer, shorter and more "cross-platform"
// (commented out ones are already defined)
//
typedef	u64					*PU64;
typedef	u64					*pu64;
typedef	u64					U64;
//typedef	unsigned int		u32;
typedef	unsigned int		U32;
typedef	unsigned int		*pu32;
typedef	unsigned int		*PU32;
//typedef	unsigned short		u16;
typedef	unsigned short		U16;
typedef	unsigned short		*pu16;
typedef	unsigned short		*PU16;
//typedef	unsigned char		u8;
typedef	unsigned char		U8;
typedef	unsigned char		*pu8;
typedef	unsigned char		*PU8;

// signed
typedef	s64					*PS64;
typedef	s64					*ps64;
//typedef	signed int			s32;
typedef	signed int			S32;
typedef	signed int			*ps32;
typedef	signed int			*PS32;
//typedef	signed short		s16;
typedef	signed short		S16;
typedef	signed short		*ps16;
typedef	signed short		*PS16;
//typedef	signed char			s8;
typedef	signed char			S8;
typedef	signed char			*ps8;
typedef	signed char			*PS8;


// register defines for the GNU assembler

#define zero    $0
#define at      $1
#define v0      $2
#define v1      $3
#define a0      $4
#define a1      $5
#define a2      $6
#define a3      $7
#define t0      $8
#define t1      $9
#define t2      $10
#define t3      $11
#define t4      $12
#define t5      $13
#define t6      $14
#define t7      $15
#define s0      $16
#define s1      $17
#define s2      $18
#define s3      $19
#define s4      $20
#define s5      $21
#define s6      $22
#define s7      $23
#define t8      $24
#define t9      $25
#define k0      $26
#define k1      $27
#define gp      $28
#define sp      $29
#define fp      $30
#define ra      $31


//
// Currently known System defines taken from various samples. 
// Please note: Some are multiply defined and are not "clear cut". This should improve with time.
//

#define TIMER0	        				((volatile unsigned int*)0x10000000)
#define TIMER1	        				((volatile unsigned int*)0x10000800)
#define TIMER2	        				((volatile unsigned int*)0x10001000)			/* Kernal timer */
#define TIMER3	        				((volatile unsigned int*)0x10001800)			/* Kernal timer */

#define TIMER_COUNT_OFFSET        		((volatile unsigned int*)0x00000000)
#define TIMER_MODE_OFFSET     			((volatile unsigned int*)0x00000010)			/* Offsets from timer base	*/
#define TIMER_COMP_OFFSET         		((volatile unsigned int*)0x00000020)
#define TIMER_HOLD_OFFSET         		((volatile unsigned int*)0x00000030)

#define T0_COUNT        				((volatile unsigned int*)0x10000000)
#define T0_MODE         				((volatile unsigned int*)0x10000010)
#define T0_COMP         				((volatile unsigned int*)0x10000020)
#define T0_HOLD         				((volatile unsigned int*)0x10000030)

#define T1_COUNT        				((volatile unsigned int*)0x10000800)
#define T1_MODE         				((volatile unsigned int*)0x10000810)
#define T1_COMP         				((volatile unsigned int*)0x10000820)
#define T1_HOLD         				((volatile unsigned int*)0x10000830)


#define GS_PMODE        				((volatile unsigned long*)0x12000000)
#define GS_SMODE2       				((volatile unsigned long*)0x12000020)
#define GS_DISPFB1      				((volatile unsigned long*)0x12000070)
#define GS_DISPLAY1     				((volatile unsigned long*)0x12000080)
#define GS_DISPFB2      				((volatile unsigned long*)0x12000090)
#define GS_DISPLAY2     				((volatile unsigned long*)0x120000a0)
#define GS_EXTBUF       				((volatile unsigned long*)0x120000b0)
#define GS_EXTDATA      				((volatile unsigned long*)0x120000c0)
#define GS_EXTWRITE     				((volatile unsigned long*)0x120000d0)
#define GS_BGCOLOR      				((volatile unsigned long*)0x120000e0)
#define GS_CSR          				((volatile unsigned long*)0x12001000)
#define GS_IMR          				((volatile unsigned long*)0x12001010)
#define GS_BUSDIR       				((volatile unsigned long*)0x12001040)
#define GS_SIGID        				((volatile unsigned long*)0x12001080)
#define GS_LABELID      				((volatile unsigned long*)0x12001090)


//
// PRIM register stuff
//
#define	GS_POINT			(0)				// GS primitives
#define	GS_LINE				(1)
#define	GS_LINESTRIP		(2)
#define	GS_TRIANGLE			(3)
#define	GS_TRISTRIP			(4)
#define	GS_TRIFAN			(5)
#define	GS_SPRITE			(6)
#define	GS_PRIM_SPRITE		(6)
#define	GS_UNKNOWN			(7)

#define	PRIM_GSHADE			(8)				// Gouraud shading ON
#define	GS_GSHADE			(8)				// Gouraud shading ON
#define	PRIM_TEXTURE		(16)			// texture mapping ON
#define	GS_PRIM_TME			(16)
#define	PRIM_ALPHA			(64)			// alpha ON
#define	PRIM_UV				(0x100)			// UV's are used (not atual coords) - UV register used
#define	GS_PRIM_FST			(0x100)			//
//
// GIF packet stuff
//
#define	GIF_PACKED				(0)
#define	GIF_REGLIST				(1)
#define	GIF_IMAGE				(2)
#define	GIF_PRE					(1)
#define	GIF_PRIM				(1)
#define	GIF_EOP					(1)

#define	GS_PACKED				(0<<58)
#define	GS_REGLIST				(1<<58)
#define	GS_IMAGE				(2<<58)
#define	GS_PRE					(1<<46)
#define	GS_PRIM					(1<<46)
#define	GS_EOP					(1<<15)


#define	GS_NEAREST				(0)
#define	GS_POINT				(0)
#define	GS_BILINEAR				(1)
#define	GS_LINEAR				(1)

//
// GS registers
//
#define	GS_FRAME_1				(0x4C)				// frame buffer details
#define	GS_FRAME_2				(0x4D)
#define	GS_ZBUF_1				(0x4E)				// zbuffer details
#define	GS_ZBUF_2				(0x4F)
#define	GS_XYOFFSET_1			(0x18)
#define	GS_XYOFFSET_2			(0x19)
#define	GS_CLIP_1				(0x40)				// 2D scissor style clipping
#define	GS_CLIP_2				(0x41)
#define	GS_PRMODECONT			(0x1a)
#define	GS_COLCLAMP				(0x46)
#define	GS_DITHER				(0x45)
#define	GS_TEXTUREFLUSH			(0x3f)				// Flush texture cache
#define	GS_TEXFLUSH				(0x3f)				// ( this used as well... which is correct?)
#define	GS_TEX1_1				(0x14)
#define	GS_TEX1_2				(0x15)
#define	GS_ALPHA_1				(0x42)
#define	GS_ALPHA_2				(0x43)
#define	GS_FBA_1				(0x4a)
#define	GS_FBA_2				(0x4b)
#define	GS_TEST_1				(0x47)				// Alpha Testing
#define	GS_TEST_2				(0x48)
#define	GS_FOGCOLOUR			(0x3d)
#define	GS_BITBLTBUF			(0x50)				// These are to do with transfers from memory->vram
#define	GS_TRXPOS				(0x51)				//
#define	GS_TRXREG				(0x52)				//
#define	GS_TRXDIR				(0x53)				//
#define	GS_CLAMP_1				(0x08)				// texture clamping
#define	GS_CLAMP_2				(0x08)				//
#define	GS_TEX0_1				(0x06)
#define	GS_TEX0_2				(0x07)


//
// Screen RGBA formats
//
#define	GS_ARGB32				(0)
#define	GS_Z32					(0)

#define	GS_PSMCT32				(0)
#define	GS_PSMCT24				(1)
#define	GS_PSMCT16				(2)
#define	GS_PSMCT16S				(10)
#define	GS_PSMT8				(19)
#define	GS_PSMT4				(20)
#define	GS_PSMT8H				(27)
#define	GS_PSMT4HL				(36)
#define	GS_PSMT4HH				(44)
#define	GS_PSMZ32				(48)
#define	GS_PSMZ24				(49)
#define	GS_PSMZ16				(50)
#define	GS_PSMZ16S				(58)



#define	DMA_ID_REFE				(0<<28)
#define	DMA_ID_CNT				(1<<28)
#define	DMA_ID_NEXT				(2<<28)
#define	DMA_ID_REF				(3<<28)
#define	DMA_ID_REFS				(4<<28)
#define	DMA_ID_CALL				(5<<28)
#define	DMA_ID_RET				(6<<28)
#define	DMA_ID_END				(7<<28)

#define	GS_HOSTLOCAL			(0)
#define	GS_LOCALHOST			(1)
#define	GS_LOCALLOCAL			(2)

	// 1 = local->host
	// 2 = local->local


//
// Usually used in GIF tags etc. Allows better/easier access to tags
//
typedef union {
	u128				ul128;
	u64					ul64[2];
	unsigned int		u32[4];
	unsigned short		u16[8];
	unsigned char		u8[16];
} U128;




#define GS_SET_RGBAQ(r, g, b, a, q) ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24) | ((u64)(q) << 32))

#define GS_SET_XYZ(x, y, z) ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))

	//	loop, end, pre, prim, flg, nreg
#define GIF_SET_TAG(loop, end, pre, prim, flg, nreg) ((long)(loop) | ((long)(end)<<15) | ((long)(pre) << 46) | ((long)(prim)<<47) | ((long)(flg)<<58)  | ((long)(nreg)<<60)	)


// Seems to be wrong...
//#define GS_SET_DISPLAY(dx, dy, magh, magv, ddw, ddh ) 
//	((long)(dx) | ((long)(dy)<<12) | ((long)(magh) << 24) |  
//	((long)(magv)<<28) | ((long)(ddw)<<32)  | ((long)(ddh)<<44) )

// Fudge... but looks like it works... ?
#define GS_SET_DISPLAY(dx, dy, magh, magv, ddw, ddh ) ((long)(dx) | ((long)(dy)<<12) | ((long)(magh) << 23) | ((long)(magv)<<27) | ((long)(ddw)<<32)  | ((long)(ddh)<<44) )


#define GS_SET_FRAME_1(base, width, format, mask ) ((long)(base)) | ((long)(width) << 16) | ((long)(format)<<24) | ((long)(mask)<<32)


#define	GS_SET_ZBUF_1(base, format,  mask) 	((long)(base)) | ((long)(format)<<24) | ((long)(mask)<<32)

#define	GS_SET_XYOFFSET_1(offx, offy )			((long)(offx)) | ((long)(offy)<<32)

#define GS_SET_CLIP_1(x1, y1, x2, y2 ) ((long)(x1)) | ((long)(y1) << 32) | ((long)(x2)<<16) | ((long)(y2)<<48)


#define	GS_SET_PRMODECONT( extras )			((long)(extras ))

#define	GS_SET_COLCLAMP( clamp )			((long)(clamp ))
#define	GS_SET_DTHE( dither )				((long)(dither))


#define GS_SET_ALPHA(alpha1, alpha2, alpha3, alpha4, alpha5 ) ((long)(alpha1) | ((long)(alpha1)<<2) | ((long)(alpha3) << 4) | ((long)(alpha4)<<6) | ((long)(alpha5) << 32) )

#define	GS_SET_FBA( alpha )					((long)(alpha))


#define GS_SET_TEST( onoff, mode, check, failcheck, destonoff, destmode, ztest, ztestmode ) ((long)(onoff) | ((long)(mode)<<1) | ((long)(check) << 4) | ((long)(failcheck)<<12) | ((long)(destonoff)<<14)  | ((long)(destmode)<<15) | ((long)(ztest)<<16) | ((long)(ztestmode)<<16))



#define GS_SET_BITBLTBUF( source, swidth, sformat, dest, dwidth, dformat) ((long)(source) | ((long)(swidth)<<16) | ((long)(sformat) << 24) | ((long)(dest)<<32) | ((long)(dwidth) << 48) | ((long)(dformat) << 56) )


#define GS_SET_TRXPOS( srcx, srcy, dstx, dsty, dir ) ((long)(srcx) | ((long)(srcy)<<16) | ((long)(dstx) << 32) | ((long)(dsty)<<48) | ((long)(dir) << 59) )

#define GS_SET_TRXREG( w, h ) 		( (long)(w) | ((long)(h)<<32) )

#define GS_SET_TRXDIR( d ) 		( (long)(d) )

#define GS_SET_CLAMP( wh, wv, minu, maxu, minv, maxv) ((long)(wh) | ((long)(wv)<<2) | ((long)(minu) << 4) | ((long)(maxu)<<14) | ((long)(minv) << 24) | ((long)(maxv) << 34) )

#define GS_SET_TEX0( tbase, tbuffw, tformat, tw, th, tc, tf, clut, clutformat, clutstore, clutoff, clutbl) ((long)(tbase) 			| ((long)(tbuffw)<<14) 	| ((long)(tformat) << 20) | ((long)(tw)<<26) | ((long)(th) << 30) 	| ((long)(tc) << 34)  | ((long)(tf)<<35) 		| ((long)(clut) << 37) 	| ((long)(clutformat) << 51) | ((long)(clutstore)<<55) | ((long)(clutoff)<<56) 	| ((long)(clutbl) << 61) )
/* JH - easy to read
#define GS_SET_TEX0( tbase, tbuffw, tformat, tw, th, tc, tf, clut, clutformat, clutstore, clutoff, clutbl)
    ((long)(tbase) 	|
    ((long)(tbuffw)<<14) |
    ((long)(tformat) << 20) |
    ((long)(tw)<<26) |
    ((long)(th) << 30) 	|
    ((long)(tc) << 34)  |
    ((long)(tf)<<35) |
    ((long)(clut) << 37) |
    ((long)(clutformat) << 51) |
    ((long)(clutstore)<<55) |
    ((long)(clutoff)<<56) |
    ((long)(clutbl) << 61) )
*/
#define GS_SET_TEX1(mipcalc, maxmip, expfilter, shrinkfilt, mipbase, mipl, mipk ) ((long)(mipcalc) | ((long)(maxmip)<<2) | ((long)(expfilter) << 5) | ((long)(shrinkfilt)<<8) | ((long)(mipbase)<<9)  | ((long)(mipl)<<15) | ((long)(mipk)<<32) )


typedef struct SGifTag {
	unsigned long NLOOP:15;
	unsigned long EOP:1;
	unsigned long pad1:16;
	unsigned long pad2:14;
	unsigned long PRE:1;
	unsigned long PRIM:11;
	unsigned long FLG:2;
	unsigned long NREG:4;
	unsigned long REGS0:4;
	unsigned long REGS1:4;
	unsigned long REGS2:4;
	unsigned long REGS3:4;
	unsigned long REGS4:4;
	unsigned long REGS5:4;
	unsigned long REGS6:4;
	unsigned long REGS7:4;
	unsigned long REGS8:4;
	unsigned long REGS9:4;
	unsigned long REGS10:4;
	unsigned long REGS11:4;
	unsigned long REGS12:4;
	unsigned long REGS13:4;
	unsigned long REGS14:4;
	unsigned long REGS15:4;
} SGifTag, *PSGifTag __attribute__((aligned(16)));

typedef struct SDmaTag {
	unsigned short	qwc;
	unsigned int	pad1:10;
	unsigned int	pce:2;
	unsigned int	id:3;
	unsigned int	irq:1;
	unsigned int	*next;
	unsigned long	pad2;
} SDmaTag, *PSDmaTag __attribute__ ((aligned(16)));

typedef struct STriStrip{
	u64 prim;
	u64 rgbaq1;
	u64 xyz2_1;
	u64 rgbaq2;
	u64 xyz2_2;
	u64 rgbaq3;
	u64 xyz2_3;
	u64 rgbaq4;
	u64 xyz2_4;
	u64 padd;
}STriStrip, *PSTriStrip; 		// Triangle STRIP in register mode

typedef	struct SQuadPoly{
	SGifTag tag;
	STriStrip	Prim;
} SQuadPoly,*PSQuadPoly;			// __attribute__((aligned(16)));




#define	SCREEN_TOP				(1792)
#define	SCREEN_LEFT				(1792)
#define	SCRN_W					(512)
#define	SCRN_H					(512)
#define OFFX 					(((4096-SCRN_W)/2)<<4)
#define OFFY 					(((4096-SCRN_H)/2)<<4)


#define UNCACHED_MEM 			(0x20000000)
#define DMA_MEM 				(0x0FFFFFFF)
#define SCRATCHPAD 				(0x70000000)

#define	BLACK					(0)
#define	WHITE					(0xffffff)
#define	RED						(0x0000ff)
#define	BLUE					(0xff0000)
#define	GREEN					(0x00ff00)


//
// Timing BAR stuff, switches off in MASTER mode
//
#ifdef	MASTER
#define	TBAR(a)
#define	TBAR_RGB(r,g,b)
#else
#define	TBAR(a)					*GS_BGCOLOR = a
#define	TBAR_RGB(r,g,b)			*GS_BGCOLOR = (r|(g<<8)|(b<<16))
#endif



//
// No defines for these is PS2LIB yet!
// 
//extern	int printf(char *format, ...);
//extern	int sprintf(const char* pDest, char *format, ...);
extern	int	fio_open( u8 *fname, int mode);
extern	int	fio_close( int fd);
extern	int	fio_read( int fd, void *buff, int buff_size);
extern	int	fio_lseek( int fd, u32 pos, int mode);


#ifdef __cplusplus
}
#endif

#endif	//_PS2_H_
