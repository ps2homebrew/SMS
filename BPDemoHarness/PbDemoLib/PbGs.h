/*
 * PbGs.h - GS functions/defines for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBGS_H_
#define _PBGS_H_

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

// crtmode : mode
#define GS_VESA 0
#define GS_DTV  1
#define GS_NTSC 2
#define GS_PAL  3

//  crtmode : res (NTSC, PAL)
#define GS_NOINTERLACE  0
#define GS_INTERLACE  1
#define GS_FIELD    0x00000
#define GS_FRAME    0x10000

//  crtmode : res (VESA)
#define GS_640x480    0
#define GS_800x600    1
#define GS_1024x768   2
#define GS_1280x1024  3
#define GS_60Hz   0x0100
#define GS_75Hz   0x0200

//  crtmode : res (DTV)
#define GS_480P 0
#define GS_1080I  1
#define GS_720P 2

//  GS pixel format
#define GS_PSMCT32    0
#define GS_PSMCT24    1
#define GS_PSMCT16    2
#define GS_PSMCT16S   10
#define GS_PSMT8      19
#define GS_PSMT4      20
#define GS_PSMT8H     27
#define GS_PSMT4HL    36
#define GS_PSMT4HH    44
#define GS_PSMZ32     48
#define GS_PSMZ24     49
#define GS_PSMZ16     50
#define GS_PSMZ16S    58

// GS registers
#define GS_PRIM       0x00
#define GS_RGBAQ      0x01
#define GS_ST         0x02
#define GS_UV         0x03
#define GS_XYZF2      0x04
#define GS_XYZ2       0x05
#define GS_TEX0_1     0x06
#define GS_TEX0_2     0x07
#define GS_CLAMP_1    0x08
#define GS_CLAMP_2    0x09
#define GS_FOG        0x0a
#define GS_XYZF3      0x0c
#define GS_XYZ3       0x0d
#define GS_TEX1_1     0x14
#define GS_TEX1_2     0x15
#define GS_TEX2_1     0x16
#define GS_TEX2_2     0x17
#define GS_XYOFFSET_1 0x18
#define GS_XYOFFSET_2 0x19
#define GS_PRMODECONT 0x1a
#define GS_PRMODE     0x1b
#define GS_TEXCLUT    0x1c
#define GS_SCANMSK    0x22
#define GS_MIPTBP1_1  0x34
#define GS_MIPTBP1_2  0x35
#define GS_MIPTBP2_1  0x36
#define GS_MIPTBP2_2  0x37
#define GS_TEXA       0x3b
#define GS_FOGCOL     0x3d
#define GS_TEXFLUSH   0x3f
#define GS_SCISSOR_1  0x40
#define GS_SCISSOR_2  0x41
#define GS_ALPHA_1    0x42
#define GS_ALPHA_2    0x43
#define GS_DIMX       0x44
#define GS_DTHE       0x45
#define GS_COLCLAMP   0x46
#define GS_TEST_1     0x47
#define GS_TEST_2     0x48
#define GS_PABE       0x49
#define GS_FBA_1      0x4a
#define GS_FBA_2      0x4b
#define GS_FRAME_1    0x4c
#define GS_FRAME_2    0x4d
#define GS_ZBUF_1     0x4e
#define GS_ZBUF_2     0x4f
#define GS_BITBLTBUF  0x50
#define GS_TRXPOS     0x51
#define GS_TRXREG     0x52
#define GS_TRXDIR     0x53
#define GS_HWREG      0x54
#define GS_SIGNAL     0x60
#define GS_FINISH     0x61
#define GS_LABEL      0x62
#define GS_NOP        0x7f

///////////////////////////////////////////////////////////////////////////////
// Setting utils
///////////////////////////////////////////////////////////////////////////////

#define GS_SETREG_ALPHA_1 GS_SET_ALPHA
#define GS_SETREG_ALPHA_2 GS_SET_ALPHA
#define GS_SETREG_ALPHA(a, b, c, d, fix) \
  ((u64)(a)       | ((u64)(b) << 2)     | ((u64)(c) << 4) | \
  ((u64)(d) << 6) | ((u64)(fix) << 32))

#define GS_SETREG_BITBLTBUF(sbp, sbw, spsm, dbp, dbw, dpsm) \
  ((u64)(sbp)         | ((u64)(sbw) << 16) | \
  ((u64)(spsm) << 24) | ((u64)(dbp) << 32) | \
  ((u64)(dbw) << 48)  | ((u64)(dpsm) << 56))

#define GS_SETREG_CLAMP_1 GS_SET_CLAMP
#define GS_SETREG_CLAMP_2 GS_SET_CLAMP
#define GS_SETREG_CLAMP(wms, wmt, minu, maxu, minv, maxv) \
  ((u64)(wms)         | ((u64)(wmt) << 2) | \
  ((u64)(minu) << 4)  | ((u64)(maxu) << 14) | \
  ((u64)(minv) << 24) | ((u64)(maxv) << 34))

#define GS_SETREG_COLCLAMP(clamp) ((u64)(clamp))

#define GS_SETREG_DIMX(dm00, dm01, dm02, dm03, dm10, dm11, dm12, dm13, dm20, dm21, dm22, dm23, dm30, dm31, dm32, dm33) \
  ((u64)(dm00)        | ((u64)(dm01) << 4)  | \
  ((u64)(dm02) << 8)  | ((u64)(dm03) << 12) | \
  ((u64)(dm10) << 16) | ((u64)(dm11) << 20) | \
  ((u64)(dm12) << 24) | ((u64)(dm13) << 28) | \
  ((u64)(dm20) << 32) | ((u64)(dm21) << 36) | \
  ((u64)(dm22) << 40) | ((u64)(dm23) << 44) | \
  ((u64)(dm30) << 48) | ((u64)(dm31) << 52) | \
  ((u64)(dm32) << 56) | ((u64)(dm33) << 60))

#define GS_SETREG_DTHE(dthe) ((u64)(dthe))

#define GS_SETREG_FBA_1 GS_SETREG_FBA
#define GS_SETREG_FBA_2 GS_SETREG_FBA
#define GS_SETREG_FBA(fba) ((u64)(fba))

#define GS_SETREG_FOG(f) ((u64)(f) << 56)

#define GS_SETREG_FOGCOL(fcr, fcg, fcb) \
  ((u64)(fcr) | ((u64)(fcg) << 8) | ((u64)(fcb) << 16))

#define GS_SETREG_FRAME_1 GS_SETREG_FRAME
#define GS_SETREG_FRAME_2 GS_SETREG_FRAME
#define GS_SETREG_FRAME(fbp, fbw, psm, fbmask) \
  ((u64)(fbp)        | ((u64)(fbw) << 16) | \
  ((u64)(psm) << 24) | ((u64)(fbmask) << 32))

#define GS_SETREG_LABEL(id, idmsk) \
  ((u64)(id) | ((u64)(idmsk) << 32))

#define GS_SETREG_MIPTBP1_1 GS_SETREG_MIPTBP1
#define GS_SETREG_MIPTBP1_2 GS_SETREG_MIPTBP1
#define GS_SETREG_MIPTBP1(tbp1, tbw1, tbp2, tbw2, tbp3, tbw3) \
  ((u64)(tbp1)        | ((u64)(tbw1) << 14) | \
  ((u64)(tbp2) << 20) | ((u64)(tbw2) << 34) | \
  ((u64)(tbp3) << 40) | ((u64)(tbw3) << 54))

#define GS_SETREG_MIPTBP2_1 GS_SETREG_MIPTBP2
#define GS_SETREG_MIPTBP2_2 GS_SETREG_MIPTBP2
#define GS_SETREG_MIPTBP2(tbp4, tbw4, tbp5, tbw5, tbp6, tbw6) \
  ((u64)(tbp4)        | ((u64)(tbw4) << 14) | \
  ((u64)(tbp5) << 20) | ((u64)(tbw5) << 34) | \
  ((u64)(tbp6) << 40) | ((u64)(tbw6) << 54))

#define GS_SETREG_PABE(pabe) ((u64)(pabe))

#define GS_SETREG_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix) \
  ((u64)(prim)      | ((u64)(iip) << 3)  | ((u64)(tme) << 4) | \
  ((u64)(fge) << 5) | ((u64)(abe) << 6)  | ((u64)(aa1) << 7) | \
  ((u64)(fst) << 8) | ((u64)(ctxt) << 9) | ((u64)(fix) << 10))

#define GS_SETREG_PRMODE(iip, tme, fge, abe, aa1, fst, ctxt, fix) \
  (((u64)(iip) << 3) | ((u64)(tme) << 4)  | \
   ((u64)(fge) << 5) | ((u64)(abe) << 6)  | ((u64)(aa1) << 7) | \
   ((u64)(fst) << 8) | ((u64)(ctxt) << 9) | ((u64)(fix) << 10))

#define GS_SETREG_PRMODECONT(ac) ((u64)(ac))

#define GS_SETREG_RGBAQ(r, g, b, a, q) \
  ((u64)(r)        | ((u64)(g) << 8) | ((u64)(b) << 16) | \
  ((u64)(a) << 24) | ((u64)(q) << 32))

#define GS_SETREG_SCANMSK(msk) ((u64)(msk))

#define GS_SETREG_SCISSOR_1 GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR_2 GS_SETREG_SCISSOR
#define GS_SETREG_SCISSOR(scax0, scax1, scay0, scay1) \
  ((u64)(scax0)        | ((u64)(scax1) << 16) | \
  ((u64)(scay0) << 32) | ((u64)(scay1) << 48))

#define GS_SETREG_SIGNAL(id, idmsk) \
  ((u64)(id) | ((u64)(idmsk) << 32))

#define GS_SETREG_ST(s, t) ((u64)(s) |  ((u64)(t) << 32))

#define GS_SETREG_TEST_1 GS_SETREG_TEST
#define GS_SETREG_TEST_2 GS_SETREG_TEST
#define GS_SETREG_TEST(ate, atst, aref, afail, date, datm, zte, ztst) \
  ((u64)(ate)         | ((u64)(atst) << 1) | \
  ((u64)(aref) << 4)  | ((u64)(afail) << 12) | \
  ((u64)(date) << 14) | ((u64)(datm) << 15) | \
  ((u64)(zte) << 16)  | ((u64)(ztst) << 17))

#define GS_SETREG_TEX0_1  GS_SETREG_TEX0
#define GS_SETREG_TEX0_2  GS_SETREG_TEX0
#define GS_SETREG_TEX0(tbp, tbw, psm, tw, th, tcc, tfx,cbp, cpsm, csm, csa, cld) \
  ((u64)(tbp)         | ((u64)(tbw) << 14) | \
  ((u64)(psm) << 20)  | ((u64)(tw) << 26) | \
  ((u64)(th) << 30)   | ((u64)(tcc) << 34) | \
  ((u64)(tfx) << 35)  | ((u64)(cbp) << 37) | \
  ((u64)(cpsm) << 51) | ((u64)(csm) << 55) | \
  ((u64)(csa) << 56)  | ((u64)(cld) << 61))

#define GS_SETREG_TEX1_1  GS_SETREG_TEX1
#define GS_SETREG_TEX1_2  GS_SETREG_TEX1
#define GS_SETREG_TEX1(lcm, mxl, mmag, mmin, mtba, l, k) \
  ((u64)(lcm)        | ((u64)(mxl) << 2)  | \
  ((u64)(mmag) << 5) | ((u64)(mmin) << 6) | \
  ((u64)(mtba) << 9) | ((u64)(l) << 19) | \
  ((u64)(k) << 32))

#define GS_SETREG_TEX2_1  GS_SETREG_TEX2
#define GS_SETREG_TEX2_2  GS_SETREG_TEX2
#define GS_SETREG_TEX2(psm, cbp, cpsm, csm, csa, cld) \
  (((u64)(psm) << 20) | ((u64)(cbp) << 37) | \
  ((u64)(cpsm) << 51) | ((u64)(csm) << 55) | \
  ((u64)(csa) << 56)  | ((u64)(cld) << 61))

#define GS_SETREG_TEXA(ta0, aem, ta1) \
  ((u64)(ta0) | ((u64)(aem) << 15) | ((u64)(ta1) << 32))

#define GS_SETREG_TEXCLUT(cbw, cou, cov) \
  ((u64)(cbw) | ((u64)(cou) << 6) | ((u64)(cov) << 12))

#define GS_SETREG_TRXDIR(xdr) ((u64)(xdr))

#define GS_SETREG_TRXPOS(ssax, ssay, dsax, dsay, dir) \
  ((u64)(ssax)        | ((u64)(ssay) << 16) | \
  ((u64)(dsax) << 32) | ((u64)(dsay) << 48) | \
  ((u64)(dir) << 59))

#define GS_SETREG_TRXREG(rrw, rrh) \
  ((u64)(rrw) | ((u64)(rrh) << 32))

#define GS_SETREG_UV(u, v) ((u64)(u) | ((u64)(v) << 16))

#define GS_SETREG_XYOFFSET_1  GS_SETREG_XYOFFSET
#define GS_SETREG_XYOFFSET_2  GS_SETREG_XYOFFSET
#define GS_SETREG_XYOFFSET(ofx, ofy) ((u64)(ofx) | ((u64)(ofy) << 32))

#define GS_SETREG_XYZ3 GS_SETREG_XYZ
#define GS_SETREG_XYZ2 GS_SETREG_XYZ
#define GS_SETREG_XYZ(x, y, z) \
  ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32))

#define GS_SETREG_XYZF3 GS_SETREG_XYZF
#define GS_SETREG_XYZF2 GS_SETREG_XYZF
#define GS_SETREG_XYZF(x, y, z, f) \
  ((u64)(x) | ((u64)(y) << 16) | ((u64)(z) << 32) | \
  ((u64)(f) << 56))

#define GS_SETREG_ZBUF_1  GS_SETREG_ZBUF
#define GS_SETREG_ZBUF_2  GS_SETREG_ZBUF
#define GS_SETREG_ZBUF(zbp, psm, zmsk) \
  ((u64)(zbp) | ((u64)(psm) << 24) | \
  ((u64)(zmsk) << 32))

#define GIF_AD		0x0e
#define GIF_NOP		0x0f

#define GIF_TAG(NLOOP,EOP,PRE,PRIM,FLG,NREG) \
		((u64)(NLOOP)<< 0)		| \
		((u64)(EOP)	<< 15)		| \
		((u64)(PRE)	<< 46)		| \
		((u64)(PRIM)	<< 47)	| \
		((u64)(FLG)	<< 58)		| \
		((u64)(NREG)	<< 60);
	
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

///////////////////////////////////////////////////////////////////////////////
// Special registers
///////////////////////////////////////////////////////////////////////////////

#define GSSREG_PMODE  0x00
#define GSSREG_SMODE1 0x01
#define GSSREG_SMODE2 0x02
#define GSSREG_SRFSH  0x03
#define GSSREG_SYNCH1 0x04
#define GSSREG_SYNCH2 0x05
#define GSSREG_SYNCV  0x06
#define GSSREG_DISPFB1  0x07
#define GSSREG_DISPLAY1 0x08
#define GSSREG_DISPFB2  0x09
#define GSSREG_DISPLAY2 0x0a
#define GSSREG_EXTBUF 0x0b
#define GSSREG_EXTDATA  0x0c
#define GSSREG_EXTWRITE 0x0d
#define GSSREG_BGCOLOR  0x0e
#define GSSREG_CSR    0x40
#define GSSREG_IMR    0x41
#define GSSREG_BUSDIR 0x44
#define GSSREG_SIGLBLID 0x48
#define GSSREG_SYSCNT 0x4f

/* GS register bit assign/define */
#define GS_CLEAR_GSREG(p) *(u64 *)(p) = 0
/* ALPHA */

#define GS_ALPHA_A_CS 0
#define GS_ALPHA_A_CD 1
#define GS_ALPHA_A_ZERO 2
#define GS_ALPHA_B_CS 0
#define GS_ALPHA_B_CD 1
#define GS_ALPHA_B_ZERO 2
#define GS_ALPHA_C_AS 0
#define GS_ALPHA_C_AD 1
#define GS_ALPHA_C_FIX  2
#define GS_ALPHA_D_CS 0
#define GS_ALPHA_D_CD 1
#define GS_ALPHA_D_ZERO 2

/* BITBLTBUF */
/** use ioctl PS2IOC_{LOAD,SAVE}IMAGE for HOST<->LOCAL xfer **/

#define GS_CLAMP_REPEAT   0
#define GS_CLAMP_CLAMP    1
#define GS_CLAMP_REGION_CLAMP 2
#define GS_CLAMP_REGION_REPEAT  3

#define GS_COLCLAMP_MASK    0
#define GS_COLCLAMP_CLAMP   1

/* DTHE */

#define GS_DTHE_OFF   0
#define GS_DTHE_ON    1

#define GS_PRIM_PRIM_POINT    0
#define GS_PRIM_PRIM_LINE   1
#define GS_PRIM_PRIM_LINESTRIP  2
#define GS_PRIM_PRIM_TRIANGLE 3
#define GS_PRIM_PRIM_TRISTRIP 4
#define GS_PRIM_PRIM_TRIFAN   5
#define GS_PRIM_PRIM_SPRITE   6
#define GS_PRIM_IIP_FLAT    0
#define GS_PRIM_IIP_GOURAUD   1
#define GS_PRIM_TME_OFF   0
#define GS_PRIM_TME_ON    1
#define GS_PRIM_FGE_OFF   0
#define GS_PRIM_FGE_ON    1
#define GS_PRIM_ABE_OFF   0
#define GS_PRIM_ABE_ON    1
#define GS_PRIM_AA1_OFF   0
#define GS_PRIM_AA1_ON    1
#define GS_PRIM_FST_STQ   0
#define GS_PRIM_FST_UV    1
#define GS_PRIM_CTXT_CONTEXT1 0
#define GS_PRIM_CTXT_CONTEXT2 1
#define GS_PRIM_FIX_NOFIXDDA  0
#define GS_PRIM_FIX_FIXDDA    1

#define GS_TEST_ATE_OFF   0
#define GS_TEST_ATE_ON    1
#define GS_TEST_ATST_NEVER    0
#define GS_TEST_ATST_ALWAYS   1
#define GS_TEST_ATST_LESS   2
#define GS_TEST_ATST_LEQUAL   3
#define GS_TEST_ATST_EQUAL    4
#define GS_TEST_ATST_GEQUAL   5
#define GS_TEST_ATST_GREATER  6
#define GS_TEST_ATST_NOTEQUAL 7
#define GS_TEST_AFAIL_KEEP    0
#define GS_TEST_AFAIL_FB_ONLY 1
#define GS_TEST_AFAIL_ZB_ONLY 2
#define GS_TEST_AFAIL_RGB_ONLY  3
#define GS_TEST_DATE_OFF    0
#define GS_TEST_DATE_ON   1
#define GS_TEST_DATM_PASS0    0
#define GS_TEST_DATM_PASS1    1
#define GS_TEST_ZTE_OFF   0
#define GS_TEST_ZTE_ON    1
#define GS_TEST_ZTST_NEVER    0
#define GS_TEST_ZTST_ALWAYS   1
#define GS_TEST_ZTST_GEQUAL   2
#define GS_TEST_ZTST_GREATER  3
#define GS_ZNEVER   GS_TEST_ZTST_NEVER
#define GS_ZALWAYS    GS_TEST_ZTST_ALWAYS
#define GS_ZGEQUAL    GS_TEST_ZTST_GEQUAL
#define GS_ZGREATER   GS_TEST_ZTST_GREATER

/* TEX0 */

#define GS_TEX_TCC_RGB      0
#define GS_TEX_TCC_RGBA     1
#define GS_TEX_TFX_MODULATE     0
#define GS_TEX_TFX_DECAL      1
#define GS_TEX_TFX_HIGHLIGHT    2
#define GS_TEX_TFX_HIGHLIGHT2   3
#define GS_TEX_CSM_CSM1     0
#define GS_TEX_CSM_CSM2     1
#define GS_TEX_CLD_NOUPDATE     0
#define GS_TEX_CLD_LOAD     1
#define GS_TEX_CLD_LOAD_COPY0   2
#define GS_TEX_CLD_LOAD_COPY1   3
#define GS_TEX_CLD_TEST0_LOAD_COPY0   4
#define GS_TEX_CLD_TEST1_LOAD_COPY1   5

/* TEX1 */

#define GS_TEX1_LCM_CALC        0
#define GS_TEX1_LCM_K       1
#define GS_TEX1_MMAG_NEAREST      0
#define GS_TEX1_MMAG_LINEAR       1
#define GS_TEX1_MMIN_NEAREST      0
#define GS_TEX1_MMIN_LINEAR       1
#define GS_TEX1_MMIN_NEAREST_MIPMAP_NEAREST   2
#define GS_TEX1_MMIN_NEAREST_MIPMAP_LINEAR    3
#define GS_TEX1_MMIN_LINEAR_MIPMAP_NEAREST    4
#define GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR   5
#define GS_TEX1_MTBA_NOAUTO       0
#define GS_TEX1_MTBA_AUTO       1

/* TEXA */
#define GS_TEXA_AEM_BLACKTHRU 1

/* TRXDIR */

#define GS_TRXDIR_HOST_TO_LOCAL 0
#define GS_TRXDIR_LOCAL_TO_HOST 1
#define GS_TRXDIR_LOCAL_TO_LOCAL  2

/* TRXPOS */

#define GS_TRXPOS_DIR_LR_UD   0
#define GS_TRXPOS_DIR_LR_DU   1
#define GS_TRXPOS_DIR_RL_UD   2
#define GS_TRXPOS_DIR_RL_DU   3

/* ZBUF */

#define GS_ZBUF_ZMSK_NOMASK 0
#define GS_ZBUF_ZMSK_MASK 1

/* CSR */
/** see ps2gs_{en,jp}.txt **/

#define GS_CSR_FLUSH    1
#define GS_CSR_FIELD_EVEN   0
#define GS_CSR_FIELD_ODD    1
#define GS_CSR_FIFO_HALFFULL  0
#define GS_CSR_FIFO_EMPTY   1
#define GS_CSR_FIFO_ALMOSTFULL  2

/* EXTBUF */

#define GS_EXTBUF_FBIN_OUT1   0
#define GS_EXTBUF_FBIN_OUT2   1
#define GS_EXTBUF_WFFMD_FIELD 0
#define GS_EXTBUF_WFFMD_FRAME 1
#define GS_EXTBUF_EMODA_THURU 0
#define GS_EXTBUF_EMODA_Y   1
#define GS_EXTBUF_EMODA_Y2    2
#define GS_EXTBUF_EMODA_ZERO  3
#define GS_EXTBUF_EMODC_THURU 0
#define GS_EXTBUF_EMODC_MONO  1
#define GS_EXTBUF_EMODC_YCbCr 2
#define GS_EXTBUF_EMODC_ALPHA 3

/* EXTWRITE */

#define GS_EXTWRITE_STOP  0
#define GS_EXTWRITE_START 1

/* PMODE */

#define GS_PMODE_EN_OFF   0
#define GS_PMODE_EN_ON    1
#define GS_PMODE_MMOD_PORT1   0
#define GS_PMODE_MMOD_ALP   1
#define GS_PMODE_AMOD_PORT1   0
#define GS_PMODE_AMOD_PORT2   1
#define GS_PMODE_SLBG_BLEND2  0
#define GS_PMODE_SLBG_BLENDBG 1

/* SMODE2 */

#define GS_SMODE2_INT_NOINTERLACE 0
#define GS_SMODE2_INT_INTERLACE 1
#define GS_SMODE2_FFMD_FIELD  0
#define GS_SMODE2_FFMD_FRAME  1
#define GS_SMODE2_DPMS_ON   0
#define GS_SMODE2_DPMS_STANDBY  1
#define GS_SMODE2_DPMS_SUSPEND  2
#define GS_SMODE2_DPMS_OFF    3

#endif // _PBGS_H_

