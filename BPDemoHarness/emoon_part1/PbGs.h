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

// GS PSM Settings
#define GS_PSMCT32		0x00
#define GS_PSMCT24		0x01
#define GS_PSMCT16		0x02
#define GS_PSMCT16S		0x0A
#define GS_PSGPU24		0x12

#define GS_PSMT8		0x13
#define GS_PSMT4		0x14
#define GS_PSMT8H		0x1B
#define GS_PSMT4HL		0x24
#define GS_PSMT4HH		0x2C

#define GS_PSMZ32		0x00
#define GS_PSMZ24		0x01
#define GS_PSMZ16		0x02
#define GS_PSMZ16S		0x0A

#define GS_CSM1			0
#define GS_CSM2			1

#define GS_CONTEXT_1 0
#define GS_CONTEXT_2 1

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

#define GS_AD  0xe
#define GS_NOP 0xf

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

#define GS_SET_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld) \
	((u64)(tbp)         | ((u64)(tbw) << 14) | ((u64)(psm) << 20)  | ((u64)(tw) << 26) | \
	((u64)(th) << 30)   | ((u64)(tcc) << 34) | ((u64)(tfx) << 35)  | ((u64)(cbp) << 37) | \
	((u64)(cpsm) << 51) | ((u64)(csm) << 55) | ((u64)(csa) << 56)  | ((u64)(cld) << 61))

#define GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
	((u64)(prim)      | ((u64)(iip) << 3)  | ((u64)(tme) << 4) | \
	((u64)(fge) << 5) | ((u64)(abe) << 6)  | ((u64)(aa1) << 7) | \
	((u64)(fst) << 8) | ((u64)(ctxt) << 9) | ((u64)(fix) << 10))

#define GS_SET_TEX1_1	GS_SET_TEX1
#define GS_SET_TEX1_2	GS_SET_TEX1
#define GS_SET_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
	((u64)(lcm)        | ((u64)(mxl) << 2)  | \
	((u64)(mmag) << 5) | ((u64)(mmin) << 6) | \
	((u64)(mtba) << 9) | ((u64)(l) << 19) | \
	((u64)(k) << 32))

#define GS_SET_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
	((u64)(wms)         | ((u64)(wmt) << 2) | ((u64)(minu) << 4)  | ((u64)(maxu) << 14) | \
	((u64)(minv) << 24) | ((u64)(maxv) << 34))

#define GS_SETREG_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
	((u64)(ate)         | ((u64)(atst) << 1) | \
	((u64)(aref) << 4)  | ((u64)(afail) << 12) | \
	((u64)(date) << 14) | ((u64)(datm) << 15) | \
	((u64)(zte) << 16)  | ((u64)(ztst) << 17))

#define GS_SETREG_FRAME_1	GS_SETREG_FRAME
#define GS_SETREG_FRAME_2	GS_SETREG_FRAME
#define GS_SETREG_FRAME(fbp, fbw, psm, fbmask) \
	((u64)(fbp)        | ((u64)(fbw) << 16) | \
	((u64)(psm) << 24) | ((u64)(fbmask) << 32))

#define GS_SETREG_SCISSOR_1	GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR_2	GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR(scax0, scax1, scay0, scay1) \
	((u64)(scax0)        | ((u64)(scax1) << 16) | \
	((u64)(scay0) << 32) | ((u64)(scay1) << 48))


#define GS_GIF_TAG( nreg, flag, prim, pre, eop, nloop )( ((u64)nreg << 60) | ((u64)flag << 58) | \
                                                         ((u64)prim << 47) | ((u64)pre  << 46) | \
                                                         ((u64)eop  << 15) | nloop )



///////////////////////////////////////////////////////////////////////////////
// Debug function(s)
///////////////////////////////////////////////////////////////////////////////

void PbGs_ShowStats();
void PbGs_SetZbufferTest( int Mode, int Context );
//void PbGs_SetRenderTarget( u32 Target );


#endif //_PBGS_H_

