/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006/7 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_GS_H
#define __SMS_GS_H

#include <tamtypes.h>

#define GS_BGCOLOR() (  *( volatile u64*           )0x120000E0  )

#define GIF_MODE() (  *( volatile unsigned int* )0x10003010  )
#define GIF_STAT() (  *( volatile unsigned int* )0x10003020  )

typedef enum GSFieldMode {

 GSFieldMode_Field = 0x00,
 GSFieldMode_Frame = 0x01

} GSFieldMode;

typedef enum GSInterlaceMode {

 GSInterlaceMode_Off = 0x00,
 GSInterlaceMode_On  = 0x01

} GSInterlaceMode;

typedef enum GSVideoMode {

 GSVideoMode_NTSC           = 0x02,
 GSVideoMode_PAL            = 0x03,
 GSVideoMode_DTV_720x480P   = 0x50,
 GSVideoMode_DTV_1920x1080I = 0x51,
 GSVideoMode_DTV_1280x720P  = 0x52,
 GSVideoMode_DTV_640x576P   = 0x53,
 GSVideoMode_VESA_60Hz      = 0x1A,
 GSVideoMode_VESA_75Hz      = 0x1C,
 GSVideoMode_Default        = 0xFF

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
 GSFlushMethod_DeleteLists,
 GSFlushMethod_DeleteListsOnly

} GSFlushMethod;

typedef enum GSCodePage {

 GSCodePage_WinLatin2   = 0,
 GSCodePage_WinCyrillic = 1,
 GSCodePage_WinLatin1   = 2,
 GSCodePage_WinGreek    = 3

} GSCodePage;

#define GIFTAG_FLG_PACKED  0
#define GIFTAG_FLG_REGLIST 1
#define GIFTAG_FLG_IMAGE   2

#define GIFTAG_REGS_PRIM     0x0
#define GIFTAG_REGS_RGBAQ    0x1
#define GIFTAG_REGS_ST       0x2
#define GIFTAG_REGS_UV       0x3
#define GIFTAG_REGS_XYZF2    0x4
#define GIFTAG_REGS_XYZ2     0x5
#define GIFTAG_REGS_TEX0_1   0x6
#define GIFTAG_REGS_TEX0_2   0x7
#define GIFTAG_REGS_CLAMP_1  0x8
#define GIFTAG_REGS_CLAMP_2  0x9
#define GIFTAG_REGS_FOG      0xA
#define GIFTAG_REGS_RESERVED 0xA
#define GIFTAG_REGS_XYZF3    0xC
#define GIFTAG_REGS_XYZ3     0xD
#define GIFTAG_REGS_AD       0xE
#define GIFTAG_REGS_NOP      0xF

#define GIF_TAG( NLOOP, EOP, PRE, PRIM, FLG, NREG ) \
 (  ( u64           )( NLOOP ) <<  0  ) |            \
 (  ( u64           )( EOP   ) << 15  ) |            \
 (  ( u64           )( PRE   ) << 46  ) |            \
 (  ( u64           )( PRIM  ) << 47  ) |            \
 (  ( u64           )( FLG   ) << 58  ) |            \
 (  ( u64           )( NREG  ) << 60  )

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

  u64           m_Lo __attribute__(  ( packed )  );
  u64           m_Hi __attribute__(  ( packed )  );

 } m_HiLo;

} GIFTag __attribute__(   (  aligned( 16 )  )   );

#define GS_ALPHA_1    0x42
#define GS_ALPHA_2    0x43
#define GS_BITBLTBUF  0x50
#define GS_COLCLAMP   0x46
#define GS_DIMX       0x44
#define GS_DTHE       0x45
#define GS_FBA_1      0x4A
#define GS_FBA_2      0x4B
#define GS_FINISH     0x61
#define GS_FRAME_1    0x4C
#define GS_FRAME_2    0x4D
#define GS_PABE       0x49
#define GS_PRIM       0x00
#define GS_PRMODECONT 0x1A
#define GS_RGBAQ      0x01L
#define GS_SCISSOR_1  0x40
#define GS_SCISSOR_2  0x41
#define GS_TEST_1     0x47
#define GS_TEST_2     0x48
#define GS_TEX0_1     0x06
#define GS_TEX0_2     0x07
#define GS_TEX1_1     0x14
#define GS_TEX1_2     0x15
#define GS_TEXA       0x3B
#define GS_TEXFLUSH   0x3F
#define GS_TRXDIR     0x53
#define GS_TRXPOS     0x51
#define GS_TRXREG     0x52
#define GS_UV         0x03
#define GS_XYOFFSET_1 0x18
#define GS_XYOFFSET_2 0x19
#define GS_XYZ2       0x05L
#define GS_ZBUF_1     0x4E
#define GS_ZBUF_2     0x4F
#define GS_NOP        0x0FL

#define GS_SET_ALPHA_1 GS_SET_ALPHA
#define GS_SET_ALPHA_2 GS_SET_ALPHA
#define GS_SET_ALPHA( a, b, c, d, f )                                        \
 (   (  ( u64           )( a ) <<  0  ) | (  ( u64           )( b ) << 2  ) | \
     (  ( u64           )( c ) <<  4  ) | (  ( u64           )( d ) << 6  ) | \
     (  ( u64           )( f ) << 32  )                                       \
 )

#define GS_SET_BITBLTBUF( sbp, sbw, spsm, dbp, dbw, dpsm )                          \
 (   (  ( u64           )( sbp  ) <<  0  ) | (  ( u64           )( sbw  ) << 16  ) | \
     (  ( u64           )( spsm ) << 24  ) | (  ( u64           )( dbp  ) << 32  ) | \
     (  ( u64           )( dbw  ) << 48  ) | (  ( u64           )( dpsm ) << 56  )   \
 )

#define GS_SET_DTHE( v ) ( v )

#define GS_SET_FBA_1 GS_SET_FBA
#define GS_SET_FBA_2 GS_SET_FBA
#define GS_SET_FBA( fba ) (  ( u64           )( fba )  )

#define GS_SET_FINISH( v ) (  ( u64           )( v )  )

#define GS_SET_FRAME_1 GS_SET_FRAME
#define GS_SET_FRAME_2 GS_SET_FRAME
#define GS_SET_FRAME( fbp, fbw, psm, fbmask )                                        \
 (   (  ( u64           )( fbp ) <<  0  ) | (  ( u64           )( fbw    ) << 16  ) | \
     (  ( u64           )( psm ) << 24  ) | (  ( u64           )( fbmask ) << 32  )   \
 )

#define GS_SET_PABE( pabe ) (  ( u64           )( pabe )  )

#define GS_SET_PRIM( prim, iip, tme, fge, abe, aa1, fst, ctxt, fix )                                                     \
 (   (  ( u64           )( prim ) << 0  ) | (  ( u64           )( iip  ) << 3  ) | (  ( u64           )( tme ) <<  4  ) | \
     (  ( u64           )( fge  ) << 5  ) | (  ( u64           )( abe  ) << 6  ) | (  ( u64           )( aa1 ) <<  7  ) | \
     (  ( u64           )( fst  ) << 8  ) | (  ( u64           )( ctxt ) << 9  ) | (  ( u64           )( fix ) << 10  )   \
 )

#define GS_SET_RGBAQ( r, g, b, a, q )                                         \
 (   (  ( u64           )( r ) <<  0  ) | (  ( u64           )( g ) <<  8  ) | \
     (  ( u64           )( b ) << 16  ) | (  ( u64           )( a ) << 24  ) | \
     (  ( u64           )( q ) << 32  )                                        \
 )

#define GS_SET_SCISSOR_1 GS_SET_SCISSOR
#define GS_SET_SCISSOR_2 GS_SET_SCISSOR
#define GS_SET_SCISSOR( scax0, scax1, scay0, scay1 ) \
 (   (  ( u64           )( scax0 ) <<  0  ) |         \
     (  ( u64           )( scax1 ) << 16  ) |         \
     (  ( u64           )( scay0 ) << 32  ) |         \
     (  ( u64           )( scay1 ) << 48  )           \
 )

#define GS_SET_TEST_1 GS_SET_TEST
#define GS_SET_TEST_2 GS_SET_TEST
#define GS_SET_TEST( ate, atst, aref, afail, date, datm, zte, ztst ) \
 (   (  ( u64           )( ate   ) <<  0  ) |                         \
     (  ( u64           )( atst  ) <<  1  ) |                         \
     (  ( u64           )( aref  ) <<  4  ) |                         \
     (  ( u64           )( afail ) << 12  ) |                         \
     (  ( u64           )( date  ) << 14  ) |                         \
     (  ( u64           )( datm  ) << 15  ) |                         \
     (  ( u64           )( zte   ) << 16  ) |                         \
     (  ( u64           )( ztst  ) << 17  )                           \
 )

#define GS_SET_TEX0_1 GS_SET_TEX0
#define GS_SET_TEX0_2 GS_SET_TEX0
#define GS_SET_TEX0(                                                               \
          tbp, tbw, psm, tw, th, tcc, tfx,                                          \
          cbp, cpsm, csm, csa, cld                                                  \
         )                                                                          \
 (   (  ( u64           )( tbp  ) <<  0  ) | (  ( u64           )( tbw ) << 14  ) | \
     (  ( u64           )( psm  ) << 20  ) | (  ( u64           )( tw  ) << 26  ) | \
     (  ( u64           )( th   ) << 30  ) | (  ( u64           )( tcc ) << 34  ) | \
     (  ( u64           )( tfx  ) << 35  ) | (  ( u64           )( cbp ) << 37  ) | \
     (  ( u64           )( cpsm ) << 51  ) | (  ( u64           )( csm ) << 55  ) | \
     (  ( u64           )( csa  ) << 56  ) | (  ( u64           )( cld ) << 61  )   \
 )

#define GS_SET_TEX1_1 GS_SET_TEX1
#define GS_SET_TEX1_2 GS_SET_TEX1
#define GS_SET_TEX1( lcm, mxl, mmag, mmin, mtba, l, k ) \
 (   (  ( u64           )( lcm  ) <<  0  ) |             \
     (  ( u64           )( mxl  ) <<  2  ) |             \
     (  ( u64           )( mmag ) <<  5  ) |             \
     (  ( u64           )( mmin ) <<  6  ) |             \
     (  ( u64           )( mtba ) <<  9  ) |             \
     (  ( u64           )( l    ) << 19  ) |             \
     (  ( u64           )( k    ) << 32  )               \
 )

#define GS_SET_TEXFLUSH( v ) (  ( u64           )( v )  )

#define GS_SET_TEXA( ta0, aem, ta1 )       \
 (   (  ( u64           )( ta0 ) <<  0  ) | \
     (  ( u64           )( aem ) << 15  ) | \
     (  ( u64           )( ta1 ) << 32  )   \
 )

#define GS_SET_TRXDIR( xdr ) (  ( u64           )( xdr )  )

#define GS_SET_TRXPOS( ssax, ssay, dsax, dsay, dir )                              \
 (   (  ( u64           )( ssax ) <<  0  ) | (  ( u64           )( ssay ) << 16) | \
     (  ( u64           )( dsax ) << 32  ) | (  ( u64           )( dsay ) << 48) | \
     (  ( u64           )( dir  ) << 59  )                                         \
 )

#define GS_SET_TRXREG( rrw, rrh ) \
 (   ( u64           )( rrw ) | (  ( u64           )( rrh ) << 32  )   )

#define GS_SET_UV( u, v ) (   ( u64           )( u ) | (  ( u64           )( v ) << 16  )   )

#define GS_SET_XYOFFSET_1 GS_SET_XYOFFSET
#define GS_SET_XYOFFSET_2 GS_SET_XYOFFSET
#define GS_SET_XYOFFSET( ofx, ofy )        \
 (   (  ( u64           )( ofx ) <<  0  ) | \
     (  ( u64           )( ofy ) << 32 )    \
 )

#define GS_SET_XYZ3 GS_SET_XYZ
#define GS_SET_XYZ2 GS_SET_XYZ
#define GS_SET_XYZ( x, y, z )            \
 (   (  ( u64           )( x ) <<  0  ) | \
     (  ( u64           )( y ) << 16  ) | \
     (  ( u64           )( z ) << 32  )   \
 )

#define GS_SET_ZBUF_1 GS_SET_ZBUF
#define GS_SET_ZBUF_2 GS_SET_ZBUF
#define GS_SET_ZBUF( zbp, psm, zmsk )       \
 (   (  ( u64           )( zbp  ) <<  0  ) | \
     (  ( u64           )( psm  ) << 24  ) | \
     (  ( u64           )( zmsk ) << 32  )   \
 )

#define GS_SET_DIMX(                                                                 \
         dm00, dm01, dm02, dm03, dm10, dm11, dm12, dm13,                             \
         dm20, dm21, dm22, dm23, dm30, dm31, dm32, dm33                              \
        )                                                                            \
 (   (  ( u64           )( dm00 ) <<  0  ) | (  ( u64           )( dm01 ) <<  4  ) | \
     (  ( u64           )( dm02 ) <<  8  ) | (  ( u64           )( dm03 ) << 12  ) | \
     (  ( u64           )( dm10 ) << 16  ) | (  ( u64           )( dm11 ) << 20  ) | \
     (  ( u64           )( dm12 ) << 24  ) | (  ( u64           )( dm13 ) << 28  ) | \
     (  ( u64           )( dm20 ) << 32  ) | (  ( u64           )( dm21 ) << 36  ) | \
     (  ( u64           )( dm22 ) << 40  ) | (  ( u64           )( dm23 ) << 44  ) | \
     (  ( u64           )( dm30 ) << 48  ) | (  ( u64           )( dm31 ) << 52  ) | \
     (  ( u64           )( dm32 ) << 56  ) | (  ( u64           )( dm33 ) << 60  )   \
 )

#define GS_PMODE_EN_OFF   0
#define GS_PMODE_EN_ON    1

#define GS_PMODE_MMOD_RC1 0
#define GS_PMODE_MMOD_ALP 1

#define GS_PMODE_AMOD_RC1 0
#define GS_PMODE_AMOD_RC2 1

#define GS_PMODE_SLBG_RC2 0
#define GS_PMODE_SLBG_BG  1

typedef union GSRegPMODE {

 struct {

  u64           EN1    :  1 __attribute__(  ( packed )  );
  u64           EN2    :  1 __attribute__(  ( packed )  );
  u64           CRTMD  :  3 __attribute__(  ( packed )  );
  u64           MMOD   :  1 __attribute__(  ( packed )  );
  u64           AMOD   :  1 __attribute__(  ( packed )  );
  u64           SLBG   :  1 __attribute__(  ( packed )  );
  u64           ALP    :  8 __attribute__(  ( packed )  );
  u64           m_Undef: 17 __attribute__(  ( packed )  );
  u64           m_Pad  : 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegPMODE;

#define GS_SMODE2_INT_NOINTERLACE 0
#define GS_SMODE2_INT_INTERLACE   1

#define GS_SMODE2_FFMD_FIELD      0
#define GS_SMODE2_FFMD_FRAME      1

#define GS_SMODE2_DPMS_ON         0
#define GS_SMODE2_DPMS_STANDBY    1
#define GS_SMODE2_DPMS_SUSPEND    2
#define GS_SMODE2_DPMS_OFF        3

typedef union GSRegSMODE2 {

 struct {

  u64           INT  :  1 __attribute__(  ( packed )  );
  u64           FFMD :  1 __attribute__(  ( packed )  );
  u64           DPMS :  2 __attribute__(  ( packed )  );
  u64           m_Pad: 60 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegSMODE2;

typedef union GSRegDISFB {

 struct {

  u64           FBP   :  9 __attribute__(  ( packed )  );
  u64           FBW   :  6 __attribute__(  ( packed )  );
  u64           PSM   :  5 __attribute__(  ( packed )  );
  u64           m_Pad0: 12 __attribute__(  ( packed )  );
  u64           DBX   : 11 __attribute__(  ( packed )  );
  u64           DBY   : 11 __attribute__(  ( packed )  );
  u64           m_Pad1: 10 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegDISFB;

typedef union GSRegDISPLAY {

 struct {

  u64           DX    : 12 __attribute__(  ( packed )  );
  u64           DY    : 11 __attribute__(  ( packed )  );
  u64           MAGH  :  4 __attribute__(  ( packed )  );
  u64           MAGV  :  2 __attribute__(  ( packed )  );
  u64           m_Pad0:  3 __attribute__(  ( packed )  );
  u64           DW    : 12 __attribute__(  ( packed )  );
  u64           DH    : 11 __attribute__(  ( packed )  );
  u64           m_Pad1:  9 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegDISPLAY;

typedef union GSRegEXTDATA {

 struct {

  u64           SX    : 12 __attribute__(  ( packed )  );
  u64           SY    : 11 __attribute__(  ( packed )  );
  u64           SMPH  :  4 __attribute__(  ( packed )  );
  u64           SMPV  :  2 __attribute__(  ( packed )  );
  u64           m_Pad0:  3 __attribute__(  ( packed )  );
  u64           WW    : 12 __attribute__(  ( packed )  );
  u64           WH    : 11 __attribute__(  ( packed )  );
  u64           m_Pad1:  9 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegEXTDATA;

typedef union GSRegFRAME {

 struct {

  u64           FBP   :  9 __attribute__(  ( packed )  );
  u64           m_Pad0:  7 __attribute__(  ( packed )  );
  u64           FBW   :  6 __attribute__(  ( packed )  );
  u64           m_Pad1:  2 __attribute__(  ( packed )  );
  u64           PSM   :  6 __attribute__(  ( packed )  );
  u64           m_Pad2:  2 __attribute__(  ( packed )  );
  u64           FBMSK : 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegFRAME;

typedef union GSRegZBUF {

 struct {

  u64           ZBP   :  9 __attribute__(  ( packed )  );
  u64           m_Pad0: 15 __attribute__(  ( packed )  );
  u64           PSM   :  4 __attribute__(  ( packed )  );
  u64           m_Pad1:  4 __attribute__(  ( packed )  );
  u64           ZMSK  :  1 __attribute__(  ( packed )  );
  u64           m_Pad2: 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegZBUF;

typedef union GSRegXYOFFSET {

 struct {

  u64           OFX   : 16 __attribute__(  ( packed )  );
  u64           m_Pad0: 16 __attribute__(  ( packed )  );
  u64           OFY   : 16 __attribute__(  ( packed )  );
  u64           m_Pad1: 16 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegXYOFFSET;

typedef union GSRegSCISSOR {

 struct {

  u64           SCAX0 : 11 __attribute__(  ( packed )  );
  u64           m_Pad0:  5 __attribute__(  ( packed )  );
  u64           SCAX1 : 11 __attribute__(  ( packed )  );
  u64           m_Pad1:  5 __attribute__(  ( packed )  );
  u64           SCAY0 : 11 __attribute__(  ( packed )  );
  u64           m_Pad2:  5 __attribute__(  ( packed )  );
  u64           SCAY1 : 11 __attribute__(  ( packed )  );
  u64           m_Pad3:  5 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegSCISSOR;

#define GS_PRMODECONT_PRMODE 0
#define GS_PRMODECONT_PRIM   1

typedef union GSRegPRMODECONT {

 struct {

  u64           AC   :  1 __attribute__(  ( packed )  );
  u64           m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegPRMODECONT;

#define GS_PABE_OFF 0
#define GS_PABE_ON  1

typedef struct GSRegPABE {

 u64           m_Value __attribute__(  ( packed )  );

} GSRegPABE;

#define GS_COLCLAMP_MASK  0
#define GS_COLCLAMP_CLAMP 1

typedef union GSRegCOLCLAMP {

 struct {

  u64           CLAMP:  1 __attribute__(  ( packed )  );
  u64           m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value  __attribute__(  ( packed )  );

} GSRegCOLCLAMP;

#define GS_DTHE_OFF 0
#define GS_DTHE_ON  1

typedef union GSRegDTHE {

 struct {

  u64           DTHE :  1 __attribute__(  ( packed )  );
  u64           m_Pad: 63 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegDTHE;

#define GS_TEST_ATE_OFF        0
#define GS_TEST_ATE_ON         1

#define GS_TEST_ATST_NEVER     0
#define GS_TEST_ATST_ALWAYS    1
#define GS_TEST_ATST_LESS      2
#define GS_TEST_ATST_LEQUAL    3
#define GS_TEST_ATST_EQUAL     4
#define GS_TEST_ATST_GEQUAL    5
#define GS_TEST_ATST_GREATER   6
#define GS_TEST_ATST_NOTEQUAL  7

#define GS_TEST_AFAIL_KEEP     0
#define GS_TEST_AFAIL_FB_ONLY  1
#define GS_TEST_AFAIL_ZB_ONLY  2
#define GS_TEST_AFAIL_RGB_ONLY 3

#define GS_TEST_DATE_OFF       0
#define GS_TEST_DATE_ON        1

#define GS_TEST_DATM_0PASS     0
#define GS_TEST_DATM_1PASS     1

#define GS_TEST_ZTE_OFF        0
#define GS_TEST_ZTE_ON         1
#define GS_TEST_ZTST_NEVER     0
#define GS_TEST_ZTST_ALWAYS    1
#define GS_TEST_ZTST_GEQUAL    2
#define GS_TEST_ZTST_GREATER   3

typedef union GSRegTEST {

 struct {

  u64           ATE  :  1 __attribute__(  ( packed )  );
  u64           ATST :  3 __attribute__(  ( packed )  );
  u64           AREF :  8 __attribute__(  ( packed )  );
  u64           AFAIL:  2 __attribute__(  ( packed )  );
  u64           DATE :  1 __attribute__(  ( packed )  );
  u64           DATM :  1 __attribute__(  ( packed )  );
  u64           ZTE  :  1 __attribute__(  ( packed )  );
  u64           ZTST :  2 __attribute__(  ( packed )  );
  u64           m_Pad: 45 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTEST;

#define GS_ALPHA_A_CS      0
#define GS_ALPHA_A_CD      1
#define GS_ALPHA_A_ZERO    2
#define GS_ALPHA_B_CS      0
#define GS_ALPHA_B_CD      1
#define GS_ALPHA_B_ZERO    2
#define GS_ALPHA_C_AS      0
#define GS_ALPHA_C_AD      1
#define GS_ALPHA_C_FIX     2
#define GS_ALPHA_D_CS      0
#define GS_ALPHA_D_CD      1
#define GS_ALPHA_D_ZERO    2

typedef union GSRegALPHA {

 struct {

  u64           A     :  2 __attribute__(  ( packed )  );
  u64           B     :  2 __attribute__(  ( packed )  );
  u64           C     :  2 __attribute__(  ( packed )  );
  u64           D     :  2 __attribute__(  ( packed )  );
  u64           m_Pad0: 24 __attribute__(  ( packed )  );
  u64           FIX   :  8 __attribute__(  ( packed )  );
  u64           m_Pad1: 24 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegALPHA;

#define GS_PRIM_PRIM_POINT     0
#define GS_PRIM_PRIM_LINE      1
#define GS_PRIM_PRIM_LINESTRIP 2
#define GS_PRIM_PRIM_TRIANGLE  3
#define GS_PRIM_PRIM_TRISTRIP  4
#define GS_PRIM_PRIM_TRIFAN    5
#define GS_PRIM_PRIM_SPRITE    6

#define GS_PRIM_IIP_FLAT       0
#define GS_PRIM_IIP_GOURAUD    1

#define GS_PRIM_TME_OFF        0
#define GS_PRIM_TME_ON         1

#define GS_PRIM_FGE_OFF        0
#define GS_PRIM_FGE_ON         1

#define GS_PRIM_ABE_OFF        0
#define GS_PRIM_ABE_ON         1

#define GS_PRIM_AA1_OFF        0
#define GS_PRIM_AA1_ON         1

#define GS_PRIM_FST_STQ        0
#define GS_PRIM_FST_UV         1

#define GS_PRIM_CTXT_1         0
#define GS_PRIM_CTXT_2         1

#define GS_PRIM_FIX_UNFIXED    0
#define GS_PRIM_FIX_FIXED      1

typedef union GSRegPRIM {

 struct {

  u64           PRIM :  3 __attribute__(  ( packed )  );
  u64           IIP  :  1 __attribute__(  ( packed )  );
  u64           TME  :  1 __attribute__(  ( packed )  );
  u64           FGE  :  1 __attribute__(  ( packed )  );
  u64           ABE  :  1 __attribute__(  ( packed )  );
  u64           AA1  :  1 __attribute__(  ( packed )  );
  u64           FST  :  1 __attribute__(  ( packed )  );
  u64           CTXT :  1 __attribute__(  ( packed )  );
  u64           FIX  :  1 __attribute__(  ( packed )  );
  u64           m_Pad: 53 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegPRIM;

typedef union GSRegBITBLTBUF {

 struct {

  u64           SBP   : 14 __attribute__(  ( packed )  );
  u64           m_Pad0:  2 __attribute__(  ( packed )  );
  u64           SBW   :  6 __attribute__(  ( packed )  );
  u64           m_Pad1:  2 __attribute__(  ( packed )  );
  u64           SPSM  :  6 __attribute__(  ( packed )  );
  u64           m_Pad2:  2 __attribute__(  ( packed )  );
  u64           DBP   : 14 __attribute__(  ( packed )  );
  u64           m_Pad3:  2 __attribute__(  ( packed )  );
  u64           DBW   :  6 __attribute__(  ( packed )  );
  u64           m_Pad4:  2 __attribute__(  ( packed )  );
  u64           DPSM  :  6 __attribute__(  ( packed )  );
  u64           m_Pad5:  2 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegBITBLTBUF;

#define GS_TRXPOS_DIR_LR_UD 0
#define GS_TRXPOS_DIR_LR_DU 1
#define GS_TRXPOS_DIR_RL_UD 2
#define GS_TRXPOS_DIR_RL_DU 3

typedef union GSRegTRXPOS {

 struct {

  u64           SSAX  : 11 __attribute__(  ( packed )  );
  u64           m_Pad0:  5 __attribute__(  ( packed )  );
  u64           SSAY  : 11 __attribute__(  ( packed )  );
  u64           m_Pad1:  5 __attribute__(  ( packed )  );
  u64           DSAX  : 11 __attribute__(  ( packed )  );
  u64           m_Pad2:  5 __attribute__(  ( packed )  );
  u64           DSAY  : 11 __attribute__(  ( packed )  );
  u64           DIR   :  2 __attribute__(  ( packed )  );
  u64           m_Pad3:  3 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTRXPOS;

typedef union GSRegTRXREG {

 struct {

  u64           RRW   : 12 __attribute__(  ( packed )  );
  u64           m_Pad0: 20 __attribute__(  ( packed )  );
  u64           RRH   : 12 __attribute__(  ( packed )  );
  u64           m_Pad1: 20 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTRXREG;

#define GS_TRXDIR_HOST_TO_LOCAL  0
#define GS_TRXDIR_LOCAL_TO_HOST  1
#define GS_TRXDIR_LOCAL_TO_LOCAL 2

typedef union GSRegTRXDIR {

 struct {

  u64           XDR  :  2 __attribute__(  ( packed )  );
  u64           m_Pad: 62 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTRXDIR;

typedef struct GSRegFINISH {

 u64           m_Value __attribute__(  ( packed )  );

} GSRegFINISH;

#define GS_ZBUF_ZMSK_NOMASK 0
#define GS_ZBUF_ZMSK_MASK   1

typedef union GSRefZBUF {

 struct {

  u64           ZBP   :  9 __attribute__(  ( packed )  );
  u64           m_Pad0: 15 __attribute__(  ( packed )  );
  u64           PSM   :  4 __attribute__(  ( packed )  );
  u64           m_Pad1:  4 __attribute__(  ( packed )  );
  u64           ZMSK  :  1 __attribute__(  ( packed )  );
  u64           m_Pad2: 31 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRefZBUF;

#define GS_TEX_TCC_RGB              0
#define GS_TEX_TCC_RGBA             1
#define GS_TEX_TFX_MODULATE         0
#define GS_TEX_TFX_DECAL            1
#define GS_TEX_TFX_HIGHLIGHT        2
#define GS_TEX_TFX_HIGHLIGHT2       3
#define GS_TEX_CSM_CSM1             0
#define GS_TEX_CSM_CSM2             1
#define GS_TEX_CLD_NOUPDATE         0
#define GS_TEX_CLD_LOAD             1
#define GS_TEX_CLD_LOAD_COPY0       2
#define GS_TEX_CLD_LOAD_COPY1       3
#define GS_TEX_CLD_TEST0_LOAD_COPY0 4
#define GS_TEX_CLD_TEST1_LOAD_COPY1 5

typedef union GSRegTEX0 {

 struct {

  u64           TBP0: 14 __attribute__(  ( packed )  );
  u64           TBW :  6 __attribute__(  ( packed )  );
  u64           PSM :  6 __attribute__(  ( packed )  );
  u64           TW  :  4 __attribute__(  ( packed )  );
  u64           TH  :  4 __attribute__(  ( packed )  );
  u64           TCC :  1 __attribute__(  ( packed )  );
  u64           TFX :  2 __attribute__(  ( packed )  );
  u64           CBP : 14 __attribute__(  ( packed )  );
  u64           CPSM:  4 __attribute__(  ( packed )  );
  u64           CSM :  1 __attribute__(  ( packed )  );
  u64           CSA :  5 __attribute__(  ( packed )  );
  u64           CLD :  3 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTEX0;

#define GS_TEX1_LCM_CALC                    0
#define GS_TEX1_LCM_K                       1
#define GS_TEX1_MMAG_NEAREST                0
#define GS_TEX1_MMAG_LINEAR                 1
#define GS_TEX1_MMIN_NEAREST                0
#define GS_TEX1_MMIN_LINEAR                 1
#define GS_TEX1_MMIN_NEAREST_MIPMAP_NEAREST 2
#define GS_TEX1_MMIN_NEAREST_MIPMAP_LINEAR  3
#define GS_TEX1_MMIN_LINEAR_MIPMAP_NEAREST  4
#define GS_TEX1_MMIN_LINEAR_MIPMAP_LINEAR   5
#define GS_TEX1_MTBA_NOAUTO                 0
#define GS_TEX1_MTBA_AUTO                   1

typedef union GSRegTEX1 {

 struct {

  u64           LCM   :  1 __attribute__(  ( packed )  );
  u64           m_Pad0:  1 __attribute__(  ( packed )  );
  u64           MXL   :  3 __attribute__(  ( packed )  );
  u64           MMAG  :  1 __attribute__(  ( packed )  );
  u64           MMIN  :  3 __attribute__(  ( packed )  );
  u64           MTBA  :  1 __attribute__(  ( packed )  );
  u64           m_Pad1:  9 __attribute__(  ( packed )  );
  u64           L     :  2 __attribute__(  ( packed )  );
  u64           m_Pad2: 11 __attribute__(  ( packed )  );
  u64           K     : 12 __attribute__(  ( packed )  );
  u64           m_Pad3: 20 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTEX1;

#define GS_CSR_SIGNAL 0x0000000000000001L
#define GS_CSR_FINISH 0x0000000000000002L
#define GS_CSR_HSINT  0x0000000000000004L
#define GS_CSR_VSINT  0x0000000000000008L
#define GS_CSR_EDWINT 0x0000000000000010L
#define GS_CSR_FLUSH  0x0000000000000100L
#define GS_CSR_RESET  0x0000000000000200L
#define GS_CSR_NFIELD 0x0000000000001000L
#define GS_CSR_FIELD  0x0000000000002000L

typedef union GSRegCSR {

 struct {

  u64           SIGNAL:  1 __attribute__(  ( packed )  );
  u64           FINISH:  1 __attribute__(  ( packed )  );
  u64           HSINT :  1 __attribute__(  ( packed )  );
  u64           VSINT :  1 __attribute__(  ( packed )  );
  u64           EDWINT:  1 __attribute__(  ( packed )  );
  u64           m_Pad0:  1 __attribute__(  ( packed )  );
  u64           m_Pad1:  1 __attribute__(  ( packed )  );
  u64           m_Pad2:  1 __attribute__(  ( packed )  );
  u64           FLUSH :  1 __attribute__(  ( packed )  );
  u64           RESET :  1 __attribute__(  ( packed )  );
  u64           m_Pad3:  1 __attribute__(  ( packed )  );
  u64           m_Pad4:  1 __attribute__(  ( packed )  );
  u64           NFIELD:  1 __attribute__(  ( packed )  );
  u64           FIELD :  1 __attribute__(  ( packed )  );
  u64           FIFO  :  2 __attribute__(  ( packed )  );
  u64           REV   :  8 __attribute__(  ( packed )  );
  u64           ID    :  8 __attribute__(  ( packed )  );
  u64           m_Pad5: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegCSR __attribute__(  (  aligned( 16 )  )  );

typedef union GSRegRGBAQ {

 struct {

  u64           R:  8 __attribute__(  ( packed )  );
  u64           G:  8 __attribute__(  ( packed )  );
  u64           B:  8 __attribute__(  ( packed )  );
  u64           A:  8 __attribute__(  ( packed )  );
  u64           Q: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegRGBAQ;

typedef union GSRegXYZ {

 struct {

  u64           X: 16 __attribute__(  ( packed )  );
  u64           Y: 16 __attribute__(  ( packed )  );
  u64           Z: 32 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegXYZ;

typedef struct GSRegTEXFLUSH {

 u64           m_Value __attribute__(  ( packed )  );

} GSRegTEXFLUSH;

typedef union GSRegUV {

 struct {

  u64           U     : 14 __attribute__(  ( packed )  );
  u64           m_Pad0:  2 __attribute__(  ( packed )  );
  u64           V     : 14 __attribute__(  ( packed )  );
  u64           m_Pad1: 34 __attribute__(  ( packed )  );

 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegUV;

typedef union GSRegDIMX {

 struct {
  u64           m_DIMX00 : 3 __attribute__(  ( packed )  );
  u64           m_Pad00  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX01 : 3 __attribute__(  ( packed )  );
  u64           m_Pad01  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX02 : 3 __attribute__(  ( packed )  );
  u64           m_Pad02  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX03 : 3 __attribute__(  ( packed )  );
  u64           m_Pad03  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX10 : 3 __attribute__(  ( packed )  );
  u64           m_Pad10  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX11 : 3 __attribute__(  ( packed )  );
  u64           m_Pad11  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX12 : 3 __attribute__(  ( packed )  );
  u64           m_Pad12  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX13 : 3 __attribute__(  ( packed )  );
  u64           m_Pad13  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX20 : 3 __attribute__(  ( packed )  );
  u64           m_Pad20  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX21 : 3 __attribute__(  ( packed )  );
  u64           m_Pad21  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX22 : 3 __attribute__(  ( packed )  );
  u64           m_Pad22  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX23 : 3 __attribute__(  ( packed )  );
  u64           m_Pad23  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX30 : 3 __attribute__(  ( packed )  );
  u64           m_Pad30  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX31 : 3 __attribute__(  ( packed )  );
  u64           m_Pad31  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX32 : 3 __attribute__(  ( packed )  );
  u64           m_Pad32  : 1 __attribute__(  ( packed )  );
  u64           m_DIMX33 : 3 __attribute__(  ( packed )  );
  u64           m_Pad33  : 1 __attribute__(  ( packed )  );
 } __attribute__(  ( packed )  );

 u64           m_Value __attribute__(  ( packed )  );

} GSRegDIMX;

typedef struct GSDC {

 GSRegPMODE   m_PMODE;
 GSRegSMODE2  m_SMODE2;
 GSRegDISFB   m_DISPFB;
 GSRegDISPLAY m_DISPLAY;

} GSDC __attribute__(   (  aligned( 16 )  )   );

typedef struct GSGC {

 GIFTag          m_Tag;
 GSRegFRAME      m_FRAMEVal;
 u64             m_FRAMETag;
 GSRegZBUF       m_ZBUFVal;
 u64             m_ZBUFTag;
 GSRegXYOFFSET   m_XYOFFSETVal;
 u64             m_XYOFFSETTag;
 GSRegSCISSOR    m_SCISSORVal;
 u64             m_SCISSORTag;
 GSRegPRMODECONT m_PRMODECONTVal;
 u64             m_PRMODECONTTag;
 GSRegCOLCLAMP   m_COLCLAMPVal;
 u64             m_COLCLAMPTag;
 GSRegDTHE       m_DTHEVal;
 u64             m_DTHETag;
 GSRegTEST       m_TESTVal;
 u64             m_TESTTag;
 GSRegALPHA      m_ALPHAVal;
 u64             m_ALPHATag;
 GSRegPABE       m_PABEVal;
 u64             m_PABETag;
 GSRegTEX1       m_TEX1Val;
 u64             m_TEX1Tag;
 GSRegDIMX       m_DIMXVal;
 u64             m_DIMXTag;

} GSGC __attribute__(   (  aligned( 16 )  )   );

typedef struct GSClearPacket {

 u64           m_VIFCodes[ 2 ];
 GIFTag        m_Tag;
 GSRegTEST     m_TESTSVal;
 u64           m_TESTSTag;
 GSRegPRIM     m_PRIMVal;
 u64           m_PRIMTag;
 GSRegRGBAQ    m_RGBAQVal;
 u64           m_RGBAQTag;
 GSRegXYZ      m_XYZ20Val;
 u64           m_XYZ20Tag;
 GSRegXYZ      m_XYZ21Val;
 u64           m_XYZ21Tag;
 GSRegTEST     m_TESTRVal;
 u64           m_TESTRTag;

} GSClearPacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSStoreImage {

 u64            m_VIFCodes[ 2 ];
 GIFTag         m_Tag;
 GSRegBITBLTBUF m_BITBLTBUFVal;
 u64            m_BITBLTBUFTag;
 GSRegTRXREG    m_TRXREGVal;
 u64            m_TRXREGTag;
 GSRegTRXPOS    m_TRXPOSVal;
 u64            m_TRXPOSTag;
 GSRegFINISH    m_FINISHVal;
 u64            m_FINISHTag;
 GSRegTRXDIR    m_TRXDIRVal;
 u64            m_TRXDIRTag;

} GSStoreImage __attribute__(   (  aligned( 16 )  )   );

typedef struct GSRoundRectPacket {

 u64           m_VIFCodes[ 2 ];
 GIFTag        m_Tag0;
 GSRegPRIM     m_PRIM;
 GSRegRGBAQ    m_RGBAQ;
 GIFTag        m_Tag1;
 GSRegXYZ      m_XYZ[ 28 ];

} GSRoundRectPacket __attribute__(   (  aligned( 16 )  )   );

typedef struct GSTexSpritePacket {

 u64           m_VIFCodes[ 2 ];
 GIFTag        m_Tag0;
 GSRegTEXFLUSH m_TEXFLUSHVal;
 u64           m_TEXFLUSHTag;
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

 u64            m_DMATag1[ 2 ];
 GIFTag         m_Tag1;
 GSRegTEXFLUSH  m_TexFlushReg;
 u64            m_TexFlushTag;
 GSRegBITBLTBUF m_BitBltBufReg;
 u64            m_BitBltBufTag;
 GSRegTRXPOS    m_TrxPosReg;
 u64            m_TrxPosTag;
 GSRegTRXREG    m_TrxRegReg;
 u64            m_TrxRegTag;
 GSRegTRXDIR    m_TrxDirReg;
 u64            m_TrxDirTag;
 GIFTag         m_Tag2;
 u64            m_DMATag2[ 2 ];
 u64            m_DMATag3[ 2 ];

} GSLoadImage __attribute__(   (  aligned( 64 )  )   );

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
 float          m_PAR480P;
 float          m_PARVESA;
 float          m_PAR720P;
 float          m_PAR1080I;

} GSParams __attribute__(   (  aligned( 16 )  )   );

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
 u64*           m_pDisplayList[ 2 ];
 u64*           m_pLastTag[ 2 ];
 u64            m_BkColor;
          int   m_PutIndex[ 2 ];
 GSDC           m_DispCtx;
 GSGC           m_DrawCtx[ 2 ];
 GSClearPacket  m_ClearPkt;
 GSCodePage     m_CodePage;
 unsigned int   m_VRAMFontPtr;
          int   m_TextColor;
 unsigned int   m_VRAMTexPtr;
 unsigned int   m_TBW;
 unsigned int   m_TW;
 unsigned int   m_TH;
          int   m_FontTexFmt;
 unsigned char* m_pDBuf;
          int   m_LWidth;
          int   m_PixSize;
          int   m_DrawDelay;
          int   m_DrawDelay2;

} GSContext;

typedef struct GSMTKFontHeader {

 char           m_ID [ 3 ];
 char           m_ClrType;
 char           m_Unk[ 3 ];
 unsigned short m_nGlyphs     __attribute__(  ( packed )  );
 unsigned char  m_GlyphWidth;
 unsigned char  m_GlyphHeight;

} GSMTKFontHeader;

extern GSContext        g_GSCtx;
extern char             g_GSCharWidth[ 224 ];
extern unsigned int*    g_MBFont;
extern GSMTKFontHeader* g_pASCII;
extern GSMTKFontHeader* g_Fonts[ 4 ];

#define GS_CSR      (  ( volatile u64*           )0x12001000  )
#define GS_PMODE    (  ( volatile u64*           )0x12000000  )
#define GS_DISPFB1  (  ( volatile u64*           )0x12000070  )
#define GS_DISPLAY1 (  ( volatile u64*           )0x12000080  )
#define GS_DISPFB2  (  ( volatile u64*           )0x12000090  )
#define GS_DISPLAY2 (  ( volatile u64*           )0x120000A0  )

#define GS_TXT_PACKET_SIZE( n ) (   ( n << 2 ) + 6 + (  ( g_GSCtx.m_FontTexFmt != GSPixelFormat_PSMT4HL ) << 3  )   )
#define GS_RRT_PACKET_SIZE()    ( 34 )
#define GS_BBT_PACKET_SIZE()    ( 12 )
#define GS_TSP_PACKET_SIZE()    ( 14 )
#define GS_VGR_PACKET_SIZE()    ( 10 )
#define GS_LDI_PACKET_SIZE()    ( 22 )

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

GSParams*     GS_Params          ( void                                                                        );
void          GS_Reset           ( GSInterlaceMode, GSVideoMode, GSFieldMode                                   );
void          GS_InitDC          ( GSDC*, GSPixelFormat, int, int, int, int                                    );
void          GS_SetDC           ( GSDC*, int                                                                  );
unsigned int  GS_InitGC          ( int, GSGC*, GSPixelFormat, int, int, GSZTest                                );
void          GS_SetGC           ( GSGC*                                                                       );
u64           GS_XYZ             ( int, int, int                                                               );
u64           GS_XYZF            ( int, int, int, int                                                          );
void          GS_XYZv            ( u64*          , float*, int, float*, int                                    );
void          GS_InitClear       ( GSClearPacket*, int, int, int, int, u64          , GSZTest                  );
void          GS_Clear           ( GSClearPacket*                                                              );
void          GS_InitLoadImage   ( GSLoadImage*, unsigned int, unsigned int, GSPixelFormat, int, int, int, int );
void          GS_LoadImage       ( GSLoadImage*, void*                                                         );
void          GS_RRV             ( u64*          , int, int, int, int, int                                     );
void          GS_RenderRoundRect ( GSRoundRectPacket*, int, int, int, int, int, s64                            );
void          GS_VSync           ( void                                                                        );
void          GS_InitStoreImage  ( GSStoreImage*, unsigned int, int, int, int, int                             );
void          GS_StoreImage      ( GSStoreImage*, void*                                                        );
u64           GS_L2P             ( int, int, int, int                                                          );
void          GS_VSync2          ( int                                                                         );
int           GS_VMode2Index     ( GSVideoMode                                                                 );

void           GSContext_Init            ( GSVideoMode, GSZTest, GSDoubleBuffer                             );
u64*           GSContext_NewPacket       ( int, int, GSPaintMethod                                          );
void           GSContext_Flush           ( int, GSFlushMethod                                               );
u64*           GSContext_NewList         ( unsigned int                                                     );
void           GSContext_DeleteList      ( u64*                                                             );
void           GSContext_CallList        ( int, u64*                                                        );
void           GSContext_CallList2       ( int, u64*                                                        );
void           GSContext_SetTextColor    ( unsigned int, u64                                                );
void           GSContext_RenderTexSprite ( GSTexSpritePacket*, int, int, int, int, int, int, int, int       );
void           GSContext_RenderVGRect    ( u64*          , int, int, int, int, u64          , u64           );

void  GSFont_Init      ( void                                                    );
int   GSFont_Width     ( char*, int                                              );
int   GSFont_WidthEx   ( char*, int, int                                         );
void  GSFont_Render    ( char*, int, int, int, u64*                              );
void  GSFont_RenderEx  ( char*, int, int, int, u64*, int, int                    );
void  GSFont_Set       ( unsigned int, void*                                     );
void* GSFont_Get       ( unsigned int, unsigned int*                             );
int   GSFont_UnpackChr ( GSMTKFontHeader*, unsigned int, void*                   );
void* GSFont_Load      ( const char*                                             );
void  GSFont_Unload    ( void                                                    );

static int inline GSFont_CharWidth ( GSMTKFontHeader* apHdr, unsigned int aChr ) {
 return (   (  ( unsigned char* )apHdr  ) + 11   )[ aChr ];
}  /* end GSFont_CharWidth */

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_GS_H */
