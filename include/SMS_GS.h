/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_GS_H
# define __SMS_GS_H

typedef enum GSFieldMode {

 GSFieldMode_Field = 0x00,
 GSFieldMode_Frame = 0x01

} GSFieldMode;

typedef enum GSInterlaceMode {

 GSInterlaceMode_Off = 0x00,
 GSInterlaceMode_On  = 0x01

} GSInterlaceMode;

typedef enum GSVideoMode {

 GSVideoMode_NTSC         = 0x02,
 GSVideoMode_PAL          = 0x03,
 GSVideoMode_DTV_720x480P = 0x50,
 GSVideoMode_Default      = 0xFF

} GSVideoMode;

typedef enum GSPixelFormat {

 GSPixelFormat_PSMCT32  = 0x00,
 GSPixelFormat_PSMCT24  = 0x01,
 GSPixelFormat_PSMCT16  = 0x02,
 GSPixelFormat_PSMCT16S = 0x0A,
 GSPixelFormat_PSMT8    = 0x13,
 GSPixelFormat_PSMT4    = 0x14,
 GSPixelFormat_PSMT8H   = 0x1B,
 GSPixelFormat_PSMT4HL  = 0x24,
 GSPixelFormat_PSMT4HH  = 0x2C,
 GSPixelFormat_PSMZ32   = 0x30,
 GSPixelFormat_PSMZ24   = 0x31,
 GSPixelFormat_PSMZ16   = 0x32,
 GSPixelFormat_PSMZ16S  = 0x3A

} GSPixelFormat;

typedef enum GSZTest {

 GSZTest_Off = 0,
 GSZTest_On  = 1

} GSZTest;

typedef enum GSDoubleBuffer {

 GSDoubleBuffer_Off = 0,
 GSDoubleBuffer_On  = 1

} GSDoubleBuffer;

typedef enum GSClearFlag {

 GSClearFlag_Off = 0,
 GSClearFlag_On  = 1

} GSClearFlag;

typedef enum GSPaintMethod {

 GSPaintMethod_Continue   = 0x01,
 GSPaintMethod_Init       = 0x02,
 GSPaintMethod_InitClear  = 0x03

} GSPaintMethod;

typedef enum GSFlushMethod {

 GSFlushMethod_KeepLists,
 GSFlushMethod_DeleteLists

} GSFlushMethod;

typedef enum GSCodePage {

 GSCodePage_WinLatin2   = 0,
 GSCodePage_WinCyrillic = 1,
 GSCodePage_WinLatin1   = 2,
 GSCodePage_WinGreek    = 3

} GSCodePage;

# define GIFTAG_FLG_PACKED  0
# define GIFTAG_FLG_REGLIST 1
# define GIFTAG_FLG_IMAGE   2

# define GIFTAG_REGS_PRIM     0x0
# define GIFTAG_REGS_RGBAQ    0x1
# define GIFTAG_REGS_ST       0x2
# define GIFTAG_REGS_UV       0x3
# define GIFTAG_REGS_XYZF2    0x4
# define GIFTAG_REGS_XYZ2     0x5
# define GIFTAG_REGS_TEX0_1   0x6
# define GIFTAG_REGS_TEX0_2   0x7
# define GIFTAG_REGS_CLAMP_1  0x8
# define GIFTAG_REGS_CLAMP_2  0x9
# define GIFTAG_REGS_FOG      0xA
# define GIFTAG_REGS_RESERVED 0xA
# define GIFTAG_REGS_XYZF3    0xC
# define GIFTAG_REGS_XYZ3     0xD
# define GIFTAG_REGS_AD       0xE
# define GIFTAG_REGS_NOP      0xF

# define GIF_TAG( NLOOP, EOP, PRE, PRIM, FLG, NREG ) \
 (  ( unsigned long )( NLOOP ) <<  0  ) |            \
 (  ( unsigned long )( EOP   ) << 15  ) |            \
 (  ( unsigned long )( PRE   ) << 46  ) |            \
 (  ( unsigned long )( PRIM  ) << 47  ) |            \
 (  ( unsigned long )( FLG   ) << 58  ) |            \
 (  ( unsigned long )( NREG  ) << 60  )

typedef union GIFTag {

 struct {

  unsigned int NLOOP: 15 __attribute__(  ( packed )  );
  unsigned int EOP  :  1 __attribute__(  ( packed )  );
  unsigned int m_Pad: 30 __attribute__(  ( packed )  );
  unsigned int PRE  :  1 __attribute__(  ( packed )  );
  unsigned int PRIM : 11 __attribute__(  ( packed )  );
  unsigned int FLG  :  2 __attribute__(  ( packed )  );
  unsigned int NREG :  4 __attribute__(  ( packed )  );
  unsigned int R0   :  4 __attribute__(  ( packed )  );
  unsigned int R1   :  4 __attribute__(  ( packed )  );
  unsigned int R2   :  4 __attribute__(  ( packed )  );
  unsigned int R3   :  4 __attribute__(  ( packed )  );
  unsigned int R4   :  4 __attribute__(  ( packed )  );
  unsigned int R5   :  4 __attribute__(  ( packed )  );
  unsigned int R6   :  4 __attribute__(  ( packed )  );
  unsigned int R7   :  4 __attribute__(  ( packed )  );
  unsigned int R8   :  4 __attribute__(  ( packed )  );
  unsigned int R9   :  4 __attribute__(  ( packed )  );
  unsigned int RA   :  4 __attribute__(  ( packed )  );
  unsigned int RB   :  4 __attribute__(  ( packed )  );
  unsigned int RC   :  4 __attribute__(  ( packed )  );
  unsigned int RD   :  4 __attribute__(  ( packed )  );
  unsigned int RE   :  4 __attribute__(  ( packed )  );
  unsigned int RF   :  4 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 struct {

  unsigned long m_Lo __attribute__(  ( packed )  );
  unsigned long m_Hi __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

} GIFTag __attribute__(   (  aligned( 16 )  )   );

# define GS_ALPHA_1    0x42
# define GS_ALPHA_2    0x43
# define GS_BITBLTBUF  0x50
# define GS_COLCLAMP   0x46
# define GS_DTHE       0x45
# define GS_FBA_1      0x4A
# define GS_FBA_2      0x4B
# define GS_FINISH     0x61
# define GS_FRAME_1    0x4C
# define GS_FRAME_2    0x4D
# define GS_PABE       0x49
# define GS_PRIM       0x00
# define GS_PRMODECONT 0x1A
# define GS_RGBAQ      0x01L
# define GS_SCISSOR_1  0x40
# define GS_SCISSOR_2  0x41
# define GS_TEST_1     0x47
# define GS_TEST_2     0x48
# define GS_TEX0_1     0x06
# define GS_TEX0_2     0x07
# define GS_TEX1_1     0x14
# define GS_TEX1_2     0x15
# define GS_TEXA       0x3B
# define GS_TEXFLUSH   0x3F
# define GS_TRXDIR     0x53
# define GS_TRXPOS     0x51
# define GS_TRXREG     0x52
# define GS_UV         0x03
# define GS_XYOFFSET_1 0x18
# define GS_XYOFFSET_2 0x19
# define GS_XYZ2       0x05L
# define GS_ZBUF_1     0x4E
# define GS_ZBUF_2     0x4F

# define GS_SET_ALPHA_1 GS_SET_ALPHA
# define GS_SET_ALPHA_2 GS_SET_ALPHA
# define GS_SET_ALPHA( a, b, c, d, f )                                        \
 (   (  ( unsigned long )( a ) <<  0  ) | (  ( unsigned long )( b ) << 2  ) | \
     (  ( unsigned long )( c ) <<  4  ) | (  ( unsigned long )( d ) << 6  ) | \
     (  ( unsigned long )( f ) << 32  )                                       \
 )

# define GS_SET_BITBLTBUF( sbp, sbw, spsm, dbp, dbw, dpsm )                          \
 (   (  ( unsigned long )( sbp  ) <<  0  ) | (  ( unsigned long )( sbw  ) << 16  ) | \
     (  ( unsigned long )( spsm ) << 24  ) | (  ( unsigned long )( dbp  ) << 32  ) | \
     (  ( unsigned long )( dbw  ) << 48  ) | (  ( unsigned long )( dpsm ) << 56  )   \
 )

# define GS_SET_FBA_1 GS_SET_FBA
# define GS_SET_FBA_2 GS_SET_FBA
# define GS_SET_FBA( fba ) (  ( unsigned long )( fba )  )

# define GS_SET_FINISH( v ) (  ( unsigned long )( v )  )

# define GS_SET_FRAME_1 GS_SET_FRAME
# define GS_SET_FRAME_2 GS_SET_FRAME
# define GS_SET_FRAME( fbp, fbw, psm, fbmask )                                        \
 (   (  ( unsigned long )( fbp ) <<  0  ) | (  ( unsigned long )( fbw    ) << 16  ) | \
     (  ( unsigned long )( psm ) << 24  ) | (  ( unsigned long )( fbmask ) << 32  )   \
 )

# define GS_SET_PABE( pabe ) (  ( unsigned long )( pabe )  )

# define GS_SET_PRIM( prim, iip, tme, fge, abe, aa1, fst, ctxt, fix )                                                     \
 (   (  ( unsigned long )( prim ) << 0  ) | (  ( unsigned long )( iip  ) << 3  ) | (  ( unsigned long )( tme ) <<  4  ) | \
     (  ( unsigned long )( fge  ) << 5  ) | (  ( unsigned long )( abe  ) << 6  ) | (  ( unsigned long )( aa1 ) <<  7  ) | \
     (  ( unsigned long )( fst  ) << 8  ) | (  ( unsigned long )( ctxt ) << 9  ) | (  ( unsigned long )( fix ) << 10  )   \
 )

# define GS_SET_RGBAQ( r, g, b, a, q )                                         \
 (   (  ( unsigned long )( r ) <<  0  ) | (  ( unsigned long )( g ) <<  8  ) | \
     (  ( unsigned long )( b ) << 16  ) | (  ( unsigned long )( a ) << 24  ) | \
     (  ( unsigned long )( q ) << 32  )                                        \
 )

# define GS_SET_SCISSOR_1 GS_SET_SCISSOR
# define GS_SET_SCISSOR_2 GS_SET_SCISSOR
# define GS_SET_SCISSOR( scax0, scax1, scay0, scay1 ) \
 (   (  ( unsigned long )( scax0 ) <<  0  ) |         \
     (  ( unsigned long )( scax1 ) << 16  ) |         \
     (  ( unsigned long )( scay0 ) << 32  ) |         \
     (  ( unsigned long )( scay1 ) << 48  )           \
 )

# define GS_SET_TEST_1 GS_SET_TEST
# define GS_SET_TEST_2 GS_SET_TEST
# define GS_SET_TEST( ate, atst, aref, afail, date, datm, zte, ztst ) \
 (   (  ( unsigned long )( ate   ) <<  0  ) |                         \
     (  ( unsigned long )( atst  ) <<  1  ) |                         \
     (  ( unsigned long )( aref  ) <<  4  ) |                         \
     (  ( unsigned long )( afail ) << 12  ) |                         \
     (  ( unsigned long )( date  ) << 14  ) |                         \
     (  ( unsigned long )( datm  ) << 15  ) |                         \
     (  ( unsigned long )( zte   ) << 16  ) |                         \
     (  ( unsigned long )( ztst  ) << 17  )                           \
 )

# define GS_SET_TEX0_1 GS_SET_TEX0
# define GS_SET_TEX0_2 GS_SET_TEX0
# define GS_SET_TEX0(                                                               \
          tbp, tbw, psm, tw, th, tcc, tfx,                                          \
          cbp, cpsm, csm, csa, cld                                                  \
         )                                                                          \
 (   (  ( unsigned long )( tbp  ) <<  0  ) | (  ( unsigned long )( tbw ) << 14  ) | \
     (  ( unsigned long )( psm  ) << 20  ) | (  ( unsigned long )( tw  ) << 26  ) | \
     (  ( unsigned long )( th   ) << 30  ) | (  ( unsigned long )( tcc ) << 34  ) | \
     (  ( unsigned long )( tfx  ) << 35  ) | (  ( unsigned long )( cbp ) << 37  ) | \
     (  ( unsigned long )( cpsm ) << 51  ) | (  ( unsigned long )( csm ) << 55  ) | \
     (  ( unsigned long )( csa  ) << 56  ) | (  ( unsigned long )( cld ) << 61  )   \
 )

# define GS_SET_TEX1_1 GS_SET_TEX1
# define GS_SET_TEX1_2 GS_SET_TEX1
# define GS_SET_TEX1( lcm, mxl, mmag, mmin, mtba, l, k ) \
 (   (  ( unsigned long )( lcm  ) <<  0  ) |             \
     (  ( unsigned long )( mxl  ) <<  2  ) |             \
     (  ( unsigned long )( mmag ) <<  5  ) |             \
     (  ( unsigned long )( mmin ) <<  6  ) |             \
     (  ( unsigned long )( mtba ) <<  9  ) |             \
     (  ( unsigned long )( l    ) << 19  ) |             \
     (  ( unsigned long )( k    ) << 32  )               \
 )

# define GS_SET_TEXFLUSH( v ) (  ( unsigned long )( v )  )

# define GS_SET_TEXA( ta0, aem, ta1 )       \
 (   (  ( unsigned long )( ta0 ) <<  0  ) | \
     (  ( unsigned long )( aem ) << 15  ) | \
     (  ( unsigned long )( ta1 ) << 32  )   \
 )

# define GS_SET_TRXDIR( xdr ) (  ( unsigned long )( xdr )  )

# define GS_SET_TRXPOS( ssax, ssay, dsax, dsay, dir )                              \
 (   (  ( unsigned long )( ssax ) <<  0  ) | (  ( unsigned long )( ssay ) << 16) | \
     (  ( unsigned long )( dsax ) << 32  ) | (  ( unsigned long )( dsay ) << 48) | \
     (  ( unsigned long )( dir  ) << 59  )                                         \
 )

# define GS_SET_TRXREG( rrw, rrh ) \
 (   ( unsigned long )( rrw ) | (  ( unsigned long )( rrh ) << 32  )   )

# define GS_SET_UV( u, v ) (   ( unsigned long )( u ) | (  ( unsigned long )( v ) << 16  )   )

# define GS_SET_XYOFFSET_1 GS_SET_XYOFFSET
# define GS_SET_XYOFFSET_2 GS_SET_XYOFFSET
# define GS_SET_XYOFFSET( ofx, ofy )        \
 (   (  ( unsigned long )( ofx ) <<  0  ) | \
     (  ( unsigned long )( ofy ) << 32 )    \
 )

# define GS_SET_XYZ3 GS_SET_XYZ
# define GS_SET_XYZ2 GS_SET_XYZ
# define GS_SET_XYZ( x, y, z )            \
 (   (  ( unsigned long )( x ) <<  0  ) | \
     (  ( unsigned long )( y ) << 16  ) | \
     (  ( unsigned long )( z ) << 32  )   \
 )

# define GS_SET_ZBUF_1 GS_SET_ZBUF
# define GS_SET_ZBUF_2 GS_SET_ZBUF
# define GS_SET_ZBUF( zbp, psm, zmsk )       \
 (   (  ( unsigned long )( zbp  ) <<  0  ) | \
     (  ( unsigned long )( psm  ) << 24  ) | \
     (  ( unsigned long )( zmsk ) << 32  )   \
 )

# define GS_PMODE_EN_OFF   0
# define GS_PMODE_EN_ON    1

# define GS_PMODE_MMOD_RC1 0
# define GS_PMODE_MMOD_ALP 1

# define GS_PMODE_AMOD_RC1 0
# define GS_PMODE_AMOD_RC2 1

# define GS_PMODE_SLBG_RC2 0
# define GS_PMODE_SLBG_BG  1

typedef union GSRegPMODE {

 struct {

  unsigned long EN1    :  1 __attribute__(  ( packed )  );
  unsigned long EN2    :  1 __attribute__(  ( packed )  );
  unsigned long CRTMD  :  3 __attribute__(  ( packed )  );
  unsigned long MMOD   :  1 __attribute__(  ( packed )  );
  unsigned long AMOD   :  1 __attribute__(  ( packed )  );
  unsigned long SLBG   :  1 __attribute__(  ( packed )  );
  unsigned long ALP    :  8 __attribute__(  ( packed )  );
  unsigned long m_Undef: 17 __attribute__(  ( packed )  );
  unsigned long m_Pad  : 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegPMODE;

# define GS_SMODE2_INT_NOINTERLACE 0
# define GS_SMODE2_INT_INTERLACE   1

# define GS_SMODE2_FFMD_FIELD      0
# define GS_SMODE2_FFMD_FRAME      1

# define GS_SMODE2_DPMS_ON         0
# define GS_SMODE2_DPMS_STANDBY    1
# define GS_SMODE2_DPMS_SUSPEND    2
# define GS_SMODE2_DPMS_OFF        3

typedef union GSRegSMODE2 {

 struct {

  unsigned long INT  :  1 __attribute__(  ( packed )  );
  unsigned long FFMD :  1 __attribute__(  ( packed )  );
  unsigned long DPMS :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad: 60 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegSMODE2;

typedef union GSRegDISFB {

 struct {

  unsigned long FBP   :  9 __attribute__(  ( packed )  );
  unsigned long FBW   :  6 __attribute__(  ( packed )  );
  unsigned long PSM   :  5 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 12 __attribute__(  ( packed )  );
  unsigned long DBX   : 11 __attribute__(  ( packed )  );
  unsigned long DBY   : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad1: 10 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegDISFB;

typedef union GSRegDISPLAY {

 struct {

  unsigned long DX    : 12 __attribute__(  ( packed )  );
  unsigned long DY    : 11 __attribute__(  ( packed )  );
  unsigned long MAGH  :  4 __attribute__(  ( packed )  );
  unsigned long MAGV  :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  3 __attribute__(  ( packed )  );
  unsigned long DW    : 12 __attribute__(  ( packed )  );
  unsigned long DH    : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  9 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegDISPLAY;

typedef union GSRegEXTDATA {

 struct {

  unsigned long SX    : 12 __attribute__(  ( packed )  );
  unsigned long SY    : 11 __attribute__(  ( packed )  );
  unsigned long SMPH  :  4 __attribute__(  ( packed )  );
  unsigned long SMPV  :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  3 __attribute__(  ( packed )  );
  unsigned long WW    : 12 __attribute__(  ( packed )  );
  unsigned long WH    : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  9 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegEXTDATA __attribute__(  ( packed )  );

typedef union GSRegFRAME {

 struct {

  unsigned long FBP   :  9 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  7 __attribute__(  ( packed )  );
  unsigned long FBW   :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  2 __attribute__(  ( packed )  );
  unsigned long PSM   :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad2:  2 __attribute__(  ( packed )  );
  unsigned long FBMSK : 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegFRAME;

typedef union GSRegZBUF {

 struct {

  unsigned long ZBP   :  9 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 15 __attribute__(  ( packed )  );
  unsigned long PSM   :  4 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  4 __attribute__(  ( packed )  );
  unsigned long ZMSK  :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad2: 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegZBUF;

typedef union GSRegXYOFFSET {

 struct {

  unsigned long OFX   : 16 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 16 __attribute__(  ( packed )  );
  unsigned long OFY   : 16 __attribute__(  ( packed )  );
  unsigned long m_Pad1: 16 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegXYOFFSET;

typedef union GSRegSCISSOR {

 struct {

  unsigned long SCAX0 : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  5 __attribute__(  ( packed )  );
  unsigned long SCAX1 : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  5 __attribute__(  ( packed )  );
  unsigned long SCAY0 : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad2:  5 __attribute__(  ( packed )  );
  unsigned long SCAY1 : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad3:  5 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegSCISSOR;

# define GS_PRMODECONT_PRMODE 0
# define GS_PRMODECONT_PRIM   1

typedef union GSRegPRMODECONT {

 struct {

  unsigned long AC   :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegPRMODECONT;

# define GS_PABE_OFF 0
# define GS_PABE_ON  1

typedef struct GSRegPABE {

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegPABE;

# define GS_COLCLAMP_MASK  0
# define GS_COLCLAMP_CLAMP 1

typedef union GSRegCOLCLAMP {

 struct {

  unsigned long CLAMP:  1 __attribute__(  ( packed )  );
  unsigned long m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value  __attribute__(  ( packed )  );

} GSRegCOLCLAMP;

# define GS_DTHE_OFF 0
# define GS_DTHE_ON  1

typedef union GSRegDTHE {

 struct {

  unsigned long DTHE :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegDTHE;

# define GS_TEST_ATE_OFF        0
# define GS_TEST_ATE_ON         1

# define GS_TEST_ATST_NEVER     0
# define GS_TEST_ATST_ALWAYS    1
# define GS_TEST_ATST_LESS      2
# define GS_TEST_ATST_LEQUAL    3
# define GS_TEST_ATST_EQUAL     4
# define GS_TEST_ATST_GEQUAL    5
# define GS_TEST_ATST_GREATER   6
# define GS_TEST_ATST_NOTEQUAL  7

# define GS_TEST_AFAIL_KEEP     0
# define GS_TEST_AFAIL_FB_ONLY  1
# define GS_TEST_AFAIL_ZB_ONLY  2
# define GS_TEST_AFAIL_RGB_ONLY 3

# define GS_TEST_DATE_OFF       0
# define GS_TEST_DATE_ON        1

# define GS_TEST_DATM_0PASS     0
# define GS_TEST_DATM_1PASS     1

# define GS_TEST_ZTE_OFF        0
# define GS_TEST_ZTE_ON         1
# define GS_TEST_ZTST_NEVER     0
# define GS_TEST_ZTST_ALWAYS    1
# define GS_TEST_ZTST_GEQUAL    2
# define GS_TEST_ZTST_GREATER   3

typedef union GSRegTEST {

 struct {

  unsigned long ATE  :  1 __attribute__(  ( packed )  );
  unsigned long ATST :  3 __attribute__(  ( packed )  );
  unsigned long AREF :  8 __attribute__(  ( packed )  );
  unsigned long AFAIL:  2 __attribute__(  ( packed )  );
  unsigned long DATE :  1 __attribute__(  ( packed )  );
  unsigned long DATM :  1 __attribute__(  ( packed )  );
  unsigned long ZTE  :  1 __attribute__(  ( packed )  );
  unsigned long ZTST :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad: 45 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTEST;

# define GS_ALPHA_A_CS      0
# define GS_ALPHA_A_CD      1
# define GS_ALPHA_A_ZERO    2
# define GS_ALPHA_B_CS      0
# define GS_ALPHA_B_CD      1
# define GS_ALPHA_B_ZERO    2
# define GS_ALPHA_C_AS      0
# define GS_ALPHA_C_AD      1
# define GS_ALPHA_C_FIX     2
# define GS_ALPHA_D_CS      0
# define GS_ALPHA_D_CD      1
# define GS_ALPHA_D_ZERO    2

typedef union GSRegALPHA {

 struct {

  unsigned long A     :  2 __attribute__(  ( packed )  );
  unsigned long B     :  2 __attribute__(  ( packed )  );
  unsigned long C     :  2 __attribute__(  ( packed )  );
  unsigned long D     :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 24 __attribute__(  ( packed )  );
  unsigned long FIX   :  8 __attribute__(  ( packed )  );
  unsigned long m_Pad1: 24 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegALPHA;

# define GS_PRIM_PRIM_POINT     0
# define GS_PRIM_PRIM_LINE      1
# define GS_PRIM_PRIM_LINESTRIP 2
# define GS_PRIM_PRIM_TRIANGLE  3
# define GS_PRIM_PRIM_TRISTRIP  4
# define GS_PRIM_PRIM_TRIFAN    5
# define GS_PRIM_PRIM_SPRITE    6

# define GS_PRIM_IIP_FLAT       0
# define GS_PRIM_IIP_GOURAUD    1

# define GS_PRIM_TME_OFF        0
# define GS_PRIM_TME_ON         1

# define GS_PRIM_FGE_OFF        0
# define GS_PRIM_FGE_ON         1

# define GS_PRIM_ABE_OFF        0
# define GS_PRIM_ABE_ON         1

# define GS_PRIM_AA1_OFF        0
# define GS_PRIM_AA1_ON         1

# define GS_PRIM_FST_STQ        0
# define GS_PRIM_FST_UV         1

# define GS_PRIM_CTXT_1         0
# define GS_PRIM_CTXT_2         1

# define GS_PRIM_FIX_UNFIXED    0
# define GS_PRIM_FIX_FIXED      1

typedef union GSRegPRIM {

 struct {

  unsigned long PRIM :  3 __attribute__(  ( packed )  );
  unsigned long IIP  :  1 __attribute__(  ( packed )  );
  unsigned long TME  :  1 __attribute__(  ( packed )  );
  unsigned long FGE  :  1 __attribute__(  ( packed )  );
  unsigned long ABE  :  1 __attribute__(  ( packed )  );
  unsigned long AA1  :  1 __attribute__(  ( packed )  );
  unsigned long FST  :  1 __attribute__(  ( packed )  );
  unsigned long CTXT :  1 __attribute__(  ( packed )  );
  unsigned long FIX  :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad: 53 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegPRIM;

typedef union GSRegBITBLTBUF {

 struct {

  unsigned long SBP   : 14 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  2 __attribute__(  ( packed )  );
  unsigned long SBW   :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  2 __attribute__(  ( packed )  );
  unsigned long SPSM  :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad2:  2 __attribute__(  ( packed )  );
  unsigned long DBP   : 14 __attribute__(  ( packed )  );
  unsigned long m_Pad3:  2 __attribute__(  ( packed )  );
  unsigned long DBW   :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad4:  2 __attribute__(  ( packed )  );
  unsigned long DPSM  :  6 __attribute__(  ( packed )  );
  unsigned long m_Pad5:  2 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegBITBLTBUF;

# define GS_TRXPOS_DIR_LR_UD 0
# define GS_TRXPOS_DIR_LR_DU 1
# define GS_TRXPOS_DIR_RL_UD 2
# define GS_TRXPOS_DIR_RL_DU 3

typedef union GSRegTRXPOS {

 struct {

  unsigned long SSAX  : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  5 __attribute__(  ( packed )  );
  unsigned long SSAY  : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  5 __attribute__(  ( packed )  );
  unsigned long DSAX  : 11 __attribute__(  ( packed )  );
  unsigned long m_Pad2:  5 __attribute__(  ( packed )  );
  unsigned long DSAY  : 11 __attribute__(  ( packed )  );
  unsigned long DIR   :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad3:  3 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTRXPOS;

typedef union GSRegTRXREG {

 struct {

  unsigned long RRW   : 12 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 20 __attribute__(  ( packed )  );
  unsigned long RRH   : 12 __attribute__(  ( packed )  );
  unsigned long m_Pad1: 20 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTRXREG;

# define GS_TRXDIR_HOST_TO_LOCAL  0
# define GS_TRXDIR_LOCAL_TO_HOST  1
# define GS_TRXDIR_LOCAL_TO_LOCAL 2

typedef union GSRegTRXDIR {

 struct {

  unsigned long XDR  :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad: 62 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTRXDIR;

typedef struct GSRegFINISH {

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegFINISH;

# define GS_ZBUF_ZMSK_NOMASK 0
# define GS_ZBUF_ZMSK_MASK   1

typedef union GSRefZBUF {

 struct {

  unsigned long ZBP   :  9 __attribute__(  ( packed )  );
  unsigned long m_Pad0: 15 __attribute__(  ( packed )  );
  unsigned long PSM   :  4 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  4 __attribute__(  ( packed )  );
  unsigned long ZMSK  :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad2: 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRefZBUF;

# define GS_TEX_TCC_RGB              0
# define GS_TEX_TCC_RGBA             1
# define GS_TEX_TFX_MODULATE         0
# define GS_TEX_TFX_DECAL            1
# define GS_TEX_TFX_HIGHLIGHT        2
# define GS_TEX_TFX_HIGHLIGHT2       3
# define GS_TEX_CSM_CSM1             0
# define GS_TEX_CSM_CSM2             1
# define GS_TEX_CLD_NOUPDATE         0
# define GS_TEX_CLD_LOAD             1
# define GS_TEX_CLD_LOAD_COPY0       2
# define GS_TEX_CLD_LOAD_COPY1       3
# define GS_TEX_CLD_TEST0_LOAD_COPY0 4
# define GS_TEX_CLD_TEST1_LOAD_COPY1 5

typedef union GSRegTEX0 {

 struct {

  unsigned long TBP0: 14 __attribute__(  ( packed )  );
  unsigned long TBW :  6 __attribute__(  ( packed )  );
  unsigned long PSM :  6 __attribute__(  ( packed )  );
  unsigned long TW  :  4 __attribute__(  ( packed )  );
  unsigned long TH  :  4 __attribute__(  ( packed )  );
  unsigned long TCC :  1 __attribute__(  ( packed )  );
  unsigned long TFX :  2 __attribute__(  ( packed )  );
  unsigned long CBP : 14 __attribute__(  ( packed )  );
  unsigned long CPSM:  4 __attribute__(  ( packed )  );
  unsigned long CSM :  1 __attribute__(  ( packed )  );
  unsigned long CSA :  5 __attribute__(  ( packed )  );
  unsigned long CLD :  3 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTEX0;

# define GS_TEX1_LCM_CALC                    0
# define GS_TEX1_LCM_K                       1
# define GS_TEX1_MMAG_NEAREST                0
# define GS_TEX1_MMAG_LINEAR                 1
# define GS_TEX1_MMIN_NEAREST                0
# define GS_TEX1_MMIN_LINEAR                 1
# define GS_TEX1_MMIN_NEAREST_MIPMAP_NEAREST 2
# define GS_TEX1_MMIN_NEAREST_MIPMAP_LINEAR  3
# define GS_TEX1_MMIN_LINEAR_MIPMAP_NEAREST  4
# define GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR   5
# define GS_TEX1_MTBA_NOAUTO                 0
# define GS_TEX1_MTBA_AUTO                   1

typedef union GSRegTEX1 {

 struct {

  unsigned long LCM   :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  1 __attribute__(  ( packed )  );
  unsigned long MXL   :  3 __attribute__(  ( packed )  );
  unsigned long MMAG  :  1 __attribute__(  ( packed )  );
  unsigned long MMIN  :  3 __attribute__(  ( packed )  );
  unsigned long MTBA  :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  9 __attribute__(  ( packed )  );
  unsigned long L     :  2 __attribute__(  ( packed )  );
  unsigned long m_Pad2: 11 __attribute__(  ( packed )  );
  unsigned long K     : 12 __attribute__(  ( packed )  );
  unsigned long m_Pad3: 20 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTEX1 __attribute__(  ( packed )  );

# define GS_CSR_SIGNAL 0x0000000000000001L
# define GS_CSR_FINISH 0x0000000000000002L
# define GS_CSR_HSINT  0x0000000000000004L
# define GS_CSR_VSINT  0x0000000000000008L
# define GS_CSR_EDWINT 0x0000000000000010L
# define GS_CSR_FLUSH  0x0000000000000100L
# define GS_CSR_RESET  0x0000000000000200L
# define GS_CSR_NFIELD 0x0000000000001000L
# define GS_CSR_FIELD  0x0000000000002000L

typedef union GSRegCSR {

 struct {

  unsigned long SIGNAL:  1 __attribute__(  ( packed )  );
  unsigned long FINISH:  1 __attribute__(  ( packed )  );
  unsigned long HSINT :  1 __attribute__(  ( packed )  );
  unsigned long VSINT :  1 __attribute__(  ( packed )  );
  unsigned long EDWINT:  1 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  1 __attribute__(  ( packed )  );
  unsigned long m_Pad1:  1 __attribute__(  ( packed )  );
  unsigned long m_Pad2:  1 __attribute__(  ( packed )  );
  unsigned long FLUSH :  1 __attribute__(  ( packed )  );
  unsigned long RESET :  1 __attribute__(  ( packed )  );
  unsigned long m_Pad3:  1 __attribute__(  ( packed )  );
  unsigned long m_Pad4:  1 __attribute__(  ( packed )  );
  unsigned long NFIELD:  1 __attribute__(  ( packed )  );
  unsigned long FIELD :  1 __attribute__(  ( packed )  );
  unsigned long FIFO  :  2 __attribute__(  ( packed )  );
  unsigned long REV   :  8 __attribute__(  ( packed )  );
  unsigned long ID    :  8 __attribute__(  ( packed )  );
  unsigned long m_Pad5: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegCSR __attribute__(  (  aligned( 16 )  )  );

typedef union GSRegRGBAQ {

 struct {

  unsigned long R:  8 __attribute__(  ( packed )  );
  unsigned long G:  8 __attribute__(  ( packed )  );
  unsigned long B:  8 __attribute__(  ( packed )  );
  unsigned long A:  8 __attribute__(  ( packed )  );
  unsigned long Q: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegRGBAQ;

typedef union GSRegXYZ {

 struct {

  unsigned long X: 16 __attribute__(  ( packed )  );
  unsigned long Y: 16 __attribute__(  ( packed )  );
  unsigned long Z: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegXYZ;

typedef struct GSRegTEXFLUSH {

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegTEXFLUSH;

typedef union GSRegUV {

 union {

  unsigned long U     : 14 __attribute__(  ( packed )  );
  unsigned long m_Pad0:  2 __attribute__(  ( packed )  );
  unsigned long V     : 14 __attribute__(  ( packed )  );
  unsigned long m_Pad1: 34 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 unsigned long m_Value __attribute__(  ( packed )  );

} GSRegUV;

typedef struct GSDC {

 GSRegPMODE   m_PMODE;
 GSRegSMODE2  m_SMODE2;
 GSRegDISFB   m_DISPFB;
 GSRegDISPLAY m_DISPLAY;

} GSDC __attribute__(   (  aligned( 16 )  )   );

typedef struct GSGC {

 GIFTag          m_Tag;
 GSRegFRAME      m_FRAMEVal;
 unsigned long   m_FRAMETag;
 GSRegZBUF       m_ZBUFVal;
 unsigned long   m_ZBUFTag;
 GSRegXYOFFSET   m_XYOFFSETVal;
 unsigned long   m_XYOFFSETTag;
 GSRegSCISSOR    m_SCISSORVal;
 unsigned long   m_SCISSORTag;
 GSRegPRMODECONT m_PRMODECONTVal;
 unsigned long   m_PRMODECONTTag;
 GSRegCOLCLAMP   m_COLCLAMPVal;
 unsigned long   m_COLCLAMPTag;
 GSRegDTHE       m_DTHEVal;
 unsigned long   m_DTHETag;
 GSRegTEST       m_TESTVal;
 unsigned long   m_TESTTag;
 GSRegALPHA      m_ALPHAVal;
 unsigned long   m_ALPHATag;
 GSRegPABE       m_PABEVal;
 unsigned long   m_PABETag;
 GSRegTEX1       m_TEX1Val;
 unsigned long   m_TEX1Tag;

} GSGC __attribute__(   (  aligned( 16 )  )   );

typedef struct GSClearPacket {

 unsigned long m_VIFCodes[ 2 ];
 GIFTag        m_Tag;
 GSRegTEST     m_TESTSVal;
 unsigned long m_TESTSTag;
 GSRegPRIM     m_PRIMVal;
 unsigned long m_PRIMTag;
 GSRegRGBAQ    m_RGBAQVal;
 unsigned long m_RGBAQTag;
 GSRegXYZ      m_XYZ20Val;
 unsigned long m_XYZ20Tag;
 GSRegXYZ      m_XYZ21Val;
 unsigned long m_XYZ21Tag;
 GSRegTEST     m_TESTRVal;
 unsigned long m_TESTRTag;

} GSClearPacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSBitBltPacket {

 unsigned long  m_VIFCodes[ 2 ];
 GIFTag         m_Tag;
 GSRegBITBLTBUF m_BITBLTBUFVal;
 unsigned long  m_BITBLTBUFTag;
 GSRegTRXREG    m_TRXREGVal;
 unsigned long  m_TRXREGTag;
 GSRegTRXPOS    m_TRXPOSVal;
 unsigned long  m_TRXPOSTag;
 GSRegTRXDIR    m_TRXDIRVal;
 unsigned long  m_TRXDIRTag;
 GSRegFINISH    m_FINISHVal;
 unsigned long  m_FINISHTag;

} GSBitBltPacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSRoundRectPacket {

 unsigned long m_VIFCodes[ 2 ];
 GIFTag        m_Tag0;
 GSRegPRIM     m_PRIM;
 GSRegRGBAQ    m_RGBAQ;
 GIFTag        m_Tag1;
 GSRegXYZ      m_XYZ[ 28 ];

} GSRoundRectPacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSTexSpritePacket {

 unsigned long m_VIFCodes[ 2 ];
 GIFTag        m_Tag0;
 GSRegTEXFLUSH m_TEXFLUSHVal;
 unsigned long m_TEXFLUSHTag;
 GIFTag        m_Tag1;
 GSRegTEX0     m_TEX0Val;
 GSRegPRIM     m_PRIMVal;
 GIFTag        m_Tag2;
 GSRegUV       m_UV0Val;
 GSRegXYZ      m_XYZ0Val;
 GSRegUV       m_UV1Val;
 GSRegXYZ      m_XYZ1Val;

} GSTexSpritePacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSLoadImage {

 unsigned long  m_DMATag1[ 2 ];
 GIFTag         m_Tag1;
 GSRegTEXFLUSH  m_TexFlushReg;
 unsigned long  m_TexFlushTag;
 GSRegBITBLTBUF m_BitBltBufReg;
 unsigned long  m_BitBltBufTag;
 GSRegTRXPOS    m_TrxPosReg;
 unsigned long  m_TrxPosTag;
 GSRegTRXREG    m_TrxRegReg;
 unsigned long  m_TrxRegTag;
 GSRegTRXDIR    m_TrxDirReg;
 unsigned long  m_TrxDirTag;
 GIFTag         m_Tag2;
 unsigned long  m_DMATag2[ 2 ];

} GSLoadImage __attribute__(   (  aligned( 16 )  )   );

typedef struct GSParams {

 unsigned short m_fInterlace;
 unsigned short m_GSCRTMode;
 unsigned short m_GSFrameField;
 unsigned short m_GSRevision;
 int            m_fGSIntHandler;
 int            m_GSIntHandlerID;
 float          m_AspectRatio[ 4 ];
 float          m_PARNTSC;
 float          m_PARPAL;
 float          m_PARVESA;

} GSParams __attribute__(   (  aligned( 16 )  )   );

typedef struct GSCharIndent {

 char m_Left [ 32 ] __attribute__(  ( packed )  );
 char m_Right[ 32 ] __attribute__(  ( packed )  );

} GSCharIndent __attribute__(   (  aligned( 8 )  )   );

typedef struct GSContext {

 unsigned int   m_Width;
 unsigned int   m_Height;
 unsigned int   m_PWidth;
 unsigned int   m_PHeight;
          int   m_OffsetX;
          int   m_OffsetY;
 unsigned int   m_VRAMPtr;
 unsigned int   m_VRAMPtr2;
          int   m_nAlloc[ 2 ];
 unsigned long* m_pDisplayList[ 2 ];
 unsigned long* m_pLastTag[ 2 ];
 unsigned long  m_BkColor;
          int   m_PutIndex[ 2 ];
 GSDC           m_DispCtx;
 GSGC           m_DrawCtx[ 2 ];
 GSClearPacket  m_ClearPkt;
 unsigned int   m_CLUT[ 4 ];
 GSCodePage     m_CodePage;
 unsigned int   m_VRAMFontPtr;
          int   m_TextColor;
 unsigned int   m_VRAMTexPtr;
 unsigned int   m_TBW;
 unsigned int   m_TW;
 unsigned int   m_TH;

} GSContext;

extern GSContext    g_GSCtx;
extern GSCharIndent g_GSCharIndent[ 224 ];

# define GS_CSR      (  ( volatile unsigned long* )0x12001000  )
# define GS_PMODE    (  ( volatile unsigned long* )0x12000000  )
# define GS_DISPFB1  (  ( volatile unsigned long* )0x12000070  )
# define GS_DISPLAY1 (  ( volatile unsigned long* )0x12000080  )
# define GS_DISPFB2  (  ( volatile unsigned long* )0x12000090  )
# define GS_DISPLAY2 (  ( volatile unsigned long* )0x120000A0  )

# define GS_TXT_PACKET_SIZE( n ) (  ( n << 2 ) + 6  )
# define GS_RRT_PACKET_SIZE()    ( 34               )
# define GS_BBT_PACKET_SIZE()    ( 12               )
# define GS_TSP_PACKET_SIZE()    ( 14               )
# define GS_VGR_PACKET_SIZE()    ( 12               )

static int inline GS_PowerOf2 ( int aVal ) {
 int i;
 for ( i = 0; ( 1 << i ) < aVal; ++i );
 return i;
}  /* end GS_PowerOf2 */
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

GSParams*     GS_Params          ( void                                                                        );
void          GS_Reset           ( GSInterlaceMode, GSVideoMode, GSFieldMode                                   );
void          GS_InitDC          ( GSDC*, GSPixelFormat, int, int, int, int                                    );
void          GS_SetDC           ( GSDC*, int                                                                  );
unsigned int  GS_InitGC          ( int, GSGC*, GSPixelFormat, int, int, GSZTest                                );
void          GS_SetGC           ( GSGC*                                                                       );
unsigned long GS_XYZ             ( int, int, int                                                               );
unsigned long GS_XYZF            ( int, int, int, int                                                          );
void          GS_InitClear       ( GSClearPacket*, int, int, int, int, unsigned long, GSZTest                  );
void          GS_Clear           ( GSClearPacket*                                                              );
void          GS_InitLoadImage   ( GSLoadImage*, unsigned int, unsigned int, GSPixelFormat, int, int, int, int );
void          GS_LoadImage       ( GSLoadImage*, void*                                                         );
void          GS_RRV             ( unsigned long*, int, int, int, int, int                                     );
void          GS_RenderRoundRect ( GSRoundRectPacket*, int, int, int, int, int, long                           );
void          GS_VSync           ( void                                                                        );

void           GSContext_Init            ( GSVideoMode, GSZTest, GSDoubleBuffer                                      );
unsigned long* GSContext_NewPacket       ( int, int, GSPaintMethod                                                   );
void           GSContext_Flush           ( int, GSFlushMethod                                                        );
unsigned long* GSContext_NewList         ( unsigned int                                                              );
void           GSContext_DeleteList      ( unsigned long*                                                            );
void           GSContext_CallList        ( int, unsigned long*                                                       );
void           GSContext_SetTextColor    ( unsigned int, unsigned long                                               );
void           GSContext_InitBitBlt      ( GSBitBltPacket*, unsigned int, int, int, int, int, unsigned int, int, int );
void           GSContext_BitBlt          ( GSBitBltPacket*                                                           );
void           GSContext_RenderTexSprite ( GSTexSpritePacket*, int, int, int, int, int, int, int, int                );
void           GSContext_RenderVGRect    ( unsigned long*, int, int, int, int, unsigned long, unsigned long          );

void GSFont_Init     ( void                                                    );
int  GSFont_Width    ( unsigned char*, int                                     );
int  GSFont_WidthEx  ( unsigned char*, int, int                                );
void GSFont_Render   ( unsigned char*, int, int, int, unsigned long*           );
void GSFont_RenderEx ( unsigned char*, int, int, int, unsigned long*, int, int );
void GSFont_Set      ( unsigned int, void*                                     );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_GS_H */
