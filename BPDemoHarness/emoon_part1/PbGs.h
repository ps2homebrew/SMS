#ifndef _PBGS_H_
#define _PBGS_H_

#define GS_REG_PRIM       0x00
#define GS_REG_RGBAQ      0x01
#define GS_REG_UV			    0x03
#define GS_REG_XYZ2			  0x05
#define GS_REG_TEX0_1		  0x06
#define GS_REG_TEX0_2		  0x07
#define GS_REG_CLAMP_1		0x08
#define GS_REG_TEX1_1     0x14
#define GS_REG_XYOFFSET_1	0x18
#define GS_REG_XYOFFSET_2	0x19
#define GS_REG_PRMODECONT	0x1A
#define GS_REG_TEXCLUT		0x1C
#define GS_REG_TEXA			  0x3B
#define GS_REG_TEXFLUSH		0x3F
#define GS_REG_SCISSOR_1	0x40
#define GS_REG_SCISSOR_2	0x41
#define GS_REG_ALPHA_1		0x42
#define GS_REG_ALPHA_2		0x43
#define GS_REG_DTHE			  0x45
#define GS_REG_COLCLAMP		0x46
#define GS_REG_TEST_1		  0x47
#define GS_REG_TEST_2		  0x48
#define GS_REG_PABE			  0x49
#define GS_REG_FRAME_1		0x4C
#define GS_REG_FRAME_2		0x4D
#define GS_REG_ZBUF_1		  0x4E
#define GS_REG_ZBUF_2		  0x4F
#define GS_REG_BITBLTBUF	0x50
#define GS_REG_TRXPOS		  0x51
#define GS_REG_TRXREG		  0x52
#define GS_REG_TRXDIR		  0x53

// GS PRIM TYPES

#define GS_PRIM_POINT			      0x0
#define GS_PRIM_LINE			      0x1
#define GS_PRIM_LINESTRIP		    0x2
#define GS_PRIM_TRIANGLE		    0x3
#define GS_PRIM_TRIANGLE_STRIP	0x4
#define GS_PRIM_TRIANGLE_FAN	  0x5
#define GS_PRIM_SPRITE			    0x6

#define GS_ALPHA_SOURCE  0x00
#define GS_ALPHA_FRAME   0x01
#define GS_ALPHA_FIXED   0x02

#define GIF_CTRL        ((volatile unsigned int *)(0x10003000))
#define GIF_MODE        ((volatile unsigned int *)(0x10003010))
#define GIF_STAT        ((volatile unsigned int *)(0x10003020))
#define GIF_TAG0        ((volatile unsigned int *)(0x10003040))
#define GIF_TAG1        ((volatile unsigned int *)(0x10003050))
#define GIF_TAG2        ((volatile unsigned int *)(0x10003060))
#define GIF_TAG3        ((volatile unsigned int *)(0x10003070))
#define GIF_CNT         ((volatile unsigned int *)(0x10003080))
#define GIF_P3CNT       ((volatile unsigned int *)(0x10003090))
#define GIF_P3TAG       ((volatile unsigned int *)(0x100030a0))


#define GS_GIF_TAG( nreg, flag, prim, pre, eop, nloop )( ((unsigned long)nreg << 60) | ((unsigned long)flag << 58) | \
                                                         ((unsigned long)prim << 47) | ((unsigned long)pre  << 46) | \
                                                         ((unsigned long)eop  << 15) | nloop )

///////////////////////////////////////////////////////////////////////////////
// Debug function(s)
///////////////////////////////////////////////////////////////////////////////

void PbGs_ShowStats();


#endif //_PBGS_H_

