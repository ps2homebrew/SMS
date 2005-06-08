/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
#           2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __GS_H
# define __GS_H

typedef enum GSDisplayMode {

 GSDisplayMode_NTSC               = 0x02,
 GSDisplayMode_PAL                = 0x03,
 GSDisplayMode_NTSC_I             = 0x04,
 GSDisplayMode_PAL_I              = 0x05,
 GSDisplayMode_VGA_640x480_60Hz   = 0x1A,
 GSDisplayMode_VGA_640x480_72Hz   = 0x1B,
 GSDisplayMode_VGA_640x480_75Hz   = 0x1C,
 GSDisplayMode_VGA_640x480_85Hz   = 0x1D,
 GSDisplayMode_VGA_800x600_56Hz   = 0x2A,
 GSDisplayMode_VGA_800x600_60Hz   = 0x2B,
 GSDisplayMode_VGA_800x600_72Hz   = 0x2C,
 GSDisplayMode_VGA_800x600_75Hz   = 0x2D,
 GSDisplayMode_VGA_800x600_85Hz   = 0x2E,
 GSDisplayMode_VGA_1024x768_60Hz  = 0x3B,
 GSDisplayMode_VGA_1024x768_70Hz  = 0x3C,
 GSDisplayMode_VGA_1024x768_75Hz  = 0x3D,
 GSDisplayMode_VGA_1024x768_85Hz  = 0x3E,
 GSDisplayMode_VGA_1280x1024_60Hz = 0x4A,
 GSDisplayMode_VGA_1280x1024_75Hz = 0x4B,
 GSDisplayMode_DTV_720x480P       = 0x50,
 GSDisplayMode_DTV_1920x1080I     = 0x51,
 GSDisplayMode_DTV_1280x720P      = 0x52,
 GSDisplayMode_DVD_NTSC           = 0x72,
 GSDisplayMode_DVD_PAL            = 0x73

} GSDisplayMode;

# ifdef _WIN32

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

# define GS_SETREG_RGBA( r, g, b, a ) RGB( r, g, b )

typedef struct GSContext {

 HWND         m_hWnd;
 HANDLE       m_hMap;
 LPVOID       m_pVideo;
 unsigned int m_Width;
 unsigned int m_Height;

 void ( *Destroy     ) ( void  );
 void ( *InitScreen  ) ( void  );
 void ( *ClearScreen ) ( DWORD );

} GSContext;

# else  /* PS2 */

#  define GS_VRAM_BLOCKSIZE 8192

#  define GS_SETREG_RGBA( r, g, b, a )     \
 (  ( unsigned long int )( r )          |\
  (  ( unsigned long int )( g ) <<  8 ) |\
  (  ( unsigned long int )( b ) << 16 ) |\
  (  ( unsigned long int )( a ) << 24 )  \
 )

#  define GS_CSR      (  ( volatile unsigned long int* )0x12001000  )
#  define GS_PMODE    (  ( volatile unsigned long int* )0x12000000  )
#  define GS_DISPFB1  (  ( volatile unsigned long int* )0x12000070  )
#  define GS_DISPFB2  (  ( volatile unsigned long int* )0x12000090  )
#  define GS_DISPLAY1 (  ( volatile unsigned long int* )0x12000080  )
#  define GS_DISPLAY2 (  ( volatile unsigned long int* )0x120000a0  )
#  define GS_BGCOLOR  (  ( volatile unsigned long int* )0x120000e0  )

#  define GS_RESET() *GS_CSR = (  ( unsigned long int )( 1 ) << 9  )

#  define GS_SET_PMODE( EN1, EN2, MMOD, AMOD, SLBG, ALP ) \
 *GS_PMODE = (  ( unsigned long int )( EN1  ) << 0  ) |  \
             (  ( unsigned long int )( EN2  ) << 1  ) |  \
             (  ( unsigned long int )( 001  ) << 2  ) |  \
             (  ( unsigned long int )( MMOD ) << 5  ) |  \
             (  ( unsigned long int )( AMOD ) << 6  ) |  \
             (  ( unsigned long int )( SLBG ) << 7  ) |  \
             (  ( unsigned long int )( ALP  ) << 8  )

#  define GS_SET_DISPFB1( FBP, FBW, PSM, DBX, DBY )        \
 *GS_DISPFB1 = (  ( unsigned long int )( FBP ) <<  0  ) | \
               (  ( unsigned long int )( FBW ) <<  9  ) | \
               (  ( unsigned long int )( PSM ) << 15  ) | \
               (  ( unsigned long int )( DBX ) << 32  ) | \
               (  ( unsigned long int )( DBY ) << 43  )

#  define GS_SET_DISPFB2( FBP, FBW, PSM, DBX, DBY )        \
 *GS_DISPFB2 = (  ( unsigned long int )( FBP ) <<  0  ) | \
               (  ( unsigned long int )( FBW ) <<  9  ) | \
               (  ( unsigned long int )( PSM ) << 15  ) | \
               (  ( unsigned long int )( DBX ) << 32  ) | \
               (  ( unsigned long int )( DBY ) << 43  )

#  define GS_SET_DISPLAY1( DX, DY, MAGH, MAGV, DW, DH )      \
 *GS_DISPLAY1 = (  ( unsigned long int )( DX   ) <<  0  ) | \
                (  ( unsigned long int )( DY   ) << 12  ) | \
                (  ( unsigned long int )( MAGH ) << 23  ) | \
                (  ( unsigned long int )( MAGV ) << 27  ) | \
                (  ( unsigned long int )( DW   ) << 32  ) | \
                (  ( unsigned long int )( DH   ) << 44  )

#  define GS_SET_DISPLAY2( DX, DY, MAGH, MAGV, DW, DH )      \
 *GS_DISPLAY2 = (  ( unsigned long int )( DX   ) <<  0  ) | \
                (  ( unsigned long int )( DY   ) << 12  ) | \
                (  ( unsigned long int )( MAGH ) << 23  ) | \
                (  ( unsigned long int )( MAGV ) << 27  ) | \
                (  ( unsigned long int )( DW   ) << 32  ) | \
                (  ( unsigned long int )( DH   ) << 44  )

#  define GS_SET_BGCOLOR( R, G, B )                      \
 *GS_BGCOLOR = (  ( unsigned long int )( R ) <<  0  ) | \
               (  ( unsigned long int )( G ) <<  8  ) | \
               (  ( unsigned long int )( B ) << 16  )

#  define GS_SETREG_FRAME_1 GS_SETREG_FRAME
#  define GS_SETREG_FRAME_2 GS_SETREG_FRAME
#  define GS_SETREG_FRAME( fbp, fbw, psm, fbmask ) \
 (  ( unsigned long int )( fbp    )           |   \
  (  ( unsigned long int )( fbw    ) << 16  ) |   \
  (  ( unsigned long int )( psm    ) << 24  ) |   \
  (  ( unsigned long int )( fbmask ) << 32  )     \
 )

#  define GS_SETREG_XYOFFSET_1 GS_SETREG_XYOFFSET
#  define GS_SETREG_XYOFFSET_2 GS_SETREG_XYOFFSET
#  define GS_SETREG_XYOFFSET( ofx, ofy ) \
 (  ( unsigned long int )( ofx ) | (  ( unsigned long int )( ofy ) << 32 )  )

#  define GS_SETREG_SCISSOR_1 GS_SETREG_SCISSOR
#  define GS_SETREG_SCISSOR_2 GS_SETREG_SCISSOR
#  define GS_SETREG_SCISSOR( scax0, scax1, scay0, scay1 ) \
 (   ( unsigned long int )( scax0 )          |           \
  (  ( unsigned long int )( scax1 ) << 16  ) |           \
  (  ( unsigned long int )( scay0 ) << 32  ) |           \
  (  ( unsigned long int )( scay1 ) << 48  )             \
 )

#  define GS_SETREG_TEST_1 GS_SETREG_TEST
#  define GS_SETREG_TEST_2 GS_SETREG_TEST
#  define GS_SETREG_TEST( ate, atst, aref, afail, date, datm, zte, ztst ) \
 (  ( unsigned long int )( ate )             |                           \
  (  ( unsigned long int )( atst  ) <<  1  ) |                           \
  (  ( unsigned long int )( aref  ) <<  4  ) |                           \
  (  ( unsigned long int )( afail ) << 12  ) |                           \
  (  ( unsigned long int )( date  ) << 14  ) |                           \
  (  ( unsigned long int )( datm  ) << 15  ) |                           \
  (  ( unsigned long int )( zte   ) << 16  ) |                           \
  (  ( unsigned long int )( ztst  ) << 17  )                             \
 )

#  define GS_SETREG_ZBUF_1 GS_SETREG_ZBUF
#  define GS_SETREG_ZBUF_2 GS_SETREG_ZBUF
#  define GS_SETREG_ZBUF( zbp, psm, zmsk )     \
 (  ( unsigned long int )( zbp )            | \
  (  ( unsigned long int )( psm  ) << 24  ) | \
  (  ( unsigned long int )( zmsk ) << 32  )   \
 )

#  define GS_SETREG_CLAMP_1 GS_SETREG_CLAMP
#  define GS_SETREG_CLAMP_2 GS_SETREG_CLAMP
#  define GS_SETREG_CLAMP( wms, wmt, minu, maxu, minv, maxv ) \
 (  ( unsigned long int )( wms  )           |                \
  (  ( unsigned long int )( wmt  ) <<  2  ) |                \
  (  ( unsigned long int )( minu ) <<  4  ) |                \
  (  ( unsigned long int )( maxu ) << 14  ) |                \
  (  ( unsigned long int )( minv ) << 24  ) |                \
  (  ( unsigned long int )( maxv ) << 34  )                  \
 )

#  define GS_SETREG_COLCLAMP( clamp ) (  ( unsigned long int )clamp  )

#  define GS_SETREG_PRIM( prim, iip, tme, fge, abe, aa1, fst, ctxt, fix ) \
 (  ( unsigned long int )( prim )           |                            \
  (  ( unsigned long int )( iip  ) <<  3  ) |                            \
  (  ( unsigned long int )( tme  ) <<  4  ) |                            \
  (  ( unsigned long int )( fge  ) <<  5  ) |                            \
  (  ( unsigned long int )( abe  ) <<  6  ) |                            \
  (  ( unsigned long int )( aa1  ) <<  7  ) |                            \
  (  ( unsigned long int )( fst  ) <<  8  ) |                            \
  (  ( unsigned long int )( ctxt ) <<  9  ) |                            \
  (  ( unsigned long int )( fix  ) << 10  )                              \
 )

#  define GS_SETREG_XYZ( x, y, z )          \
 (  ( unsigned long int )( x )           | \
  (  ( unsigned long int )( y ) << 16  ) | \
  (  ( unsigned long int )( z ) << 32  )   \
 )

#  define GS_SETREG_BITBLTBUF( sbp, sbw, spsm, dbp, dbw, dpsm ) \
 (  ( unsigned long int )( sbp )            |                  \
  (  ( unsigned long int )( sbw  ) << 16  ) |                  \
  (  ( unsigned long int )( spsm ) << 24  ) |                  \
  (  ( unsigned long int )( dbp  ) << 32  ) |                  \
  (  ( unsigned long int )( dbw  ) << 48  ) |                  \
  (  ( unsigned long int )( dpsm ) << 56  )                    \
 )

#  define GS_SETREG_TRXPOS( ssax, ssay, dsax, dsay, dir ) \
 (  ( unsigned long int )( ssax )           |            \
  (  ( unsigned long int )( ssay ) << 16  ) |            \
  (  ( unsigned long int )( dsax ) << 32  ) |            \
  (  ( unsigned long int )( dsay ) << 48  ) |            \
  (  ( unsigned long int )( dir  ) << 59  )              \
 )

#  define GS_SETREG_TRXREG( rrw, rrh ) \
 (   ( unsigned long int )( rrw ) | (  ( unsigned long int )( rrh ) << 32  )   )

#  define GS_SETREG_TRXDIR( xdr ) (  ( unsigned long int )( xdr )  )

#  define GS_SETREG_UV( u, v ) (   ( unsigned long int  )( u ) | (  ( unsigned long int )( v ) << 16  )   )

#  define GS_SETREG_TEX0_1 GS_SETREG_TEX0
#  define GS_SETREG_TEX0_2 GS_SETREG_TEX0
#  define GS_SETREG_TEX0(                                            \
          tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld \
         )                                                          \
 (  ( unsigned long int )( tbp )            |                       \
  (  ( unsigned long int )( tbw  ) << 14  ) |                       \
  (  ( unsigned long int )( psm  ) << 20  ) |                       \
  (  ( unsigned long int )( tw   ) << 26  ) |                       \
  (  ( unsigned long int )( th   ) << 30  ) |                       \
  (  ( unsigned long int )( tcc  ) << 34  ) |                       \
  (  ( unsigned long int )( tfx  ) << 35  ) |                       \
  (  ( unsigned long int )( cbp  ) << 37  ) |                       \
  (  ( unsigned long int )( cpsm ) << 51  ) |                       \
  (  ( unsigned long int )( csm  ) << 55  ) |                       \
  (  ( unsigned long int )( csa  ) << 56  ) |                       \
  (  ( unsigned long int )( cld  ) << 61  )                         \
 )

#  define GS_SETREG_TEX1_1 GS_SETREG_TEX1
#  define GS_SETREG_TEX1_2 GS_SETREG_TEX1
#  define GS_SETREG_TEX1( lcm, mxl, mmag, mmin, mtba, l, k ) \
 (  ( unsigned long int )( lcm )            |               \
  (  ( unsigned long int )( mxl  ) <<  2  ) |               \
  (  ( unsigned long int )( mmag ) <<  5  ) |               \
  (  ( unsigned long int )( mmin ) <<  6  ) |               \
  (  ( unsigned long int )( mtba ) <<  9  ) |               \
  (  ( unsigned long int )( l    ) << 19  ) |               \
  (  ( unsigned long int )( k    ) << 32  )                 \
 )

#  define GS_ALPHA_1    0x42
#  define GS_ALPHA_2    0x43
#  define GS_BITBLTBUF  0x50
#  define GS_CLAMP_1    0x08
#  define GS_CLAMP_2    0x09
#  define GS_COLCLAMP   0x46
#  define GS_FRAME_1    0x4C
#  define GS_FRAME_2    0x4D
#  define GS_PRIM       0x00
#  define GS_PRMODECONT 0x1A
#  define GS_RGBAQ      0x01
#  define GS_SCISSOR_1  0x40
#  define GS_SCISSOR_2  0x41
#  define GS_TEST_1     0x47
#  define GS_TEST_2     0x48
#  define GS_TEX1_1     0x14
#  define GS_TEX1_2     0x15
#  define GS_TEXFLUSH   0x3F
#  define GS_TRXDIR     0x53
#  define GS_TRXPOS     0x51
#  define GS_TRXREG     0x52
#  define GS_UV         0x03
#  define GS_XYOFFSET_1 0x18
#  define GS_XYOFFSET_2 0x19
#  define GS_XYZ2       0x05
#  define GS_ZBUF_1     0x4E
#  define GS_ZBUF_2     0x4F

#  define GS_TEX0_1 0x06
#  define GS_TEX0_2 0x07

#  define GS_PRIM_PRIM_POINT     0
#  define GS_PRIM_PRIM_LINE      1
#  define GS_PRIM_PRIM_LINESTRIP 2
#  define GS_PRIM_PRIM_TRIANGLE  3
#  define GS_PRIM_PRIM_TRISTRIP  4
#  define GS_PRIM_PRIM_TRIFAN    5
#  define GS_PRIM_PRIM_SPRITE    6

#  define GIF_AD 0x0E

#  define GIF_TAG( NLOOP, EOP, PRE, PRIM, FLG, NREG ) \
 (  ( unsigned long int )( NLOOP ) <<  0  ) |        \
 (  ( unsigned long int )( EOP   ) << 15  ) |        \
 (  ( unsigned long int )( PRE   ) << 46  ) |        \
 (  ( unsigned long int )( PRIM  ) << 47  ) |        \
 (  ( unsigned long int )( FLG   ) << 58  ) |        \
 (  ( unsigned long int )( NREG  ) << 60  )

#  define ALPHA_SRC  0
#  define ALPHA_DST  1
#  define ALPHA_ZERO 2
#  define ALPHA_FIX  2
#  define ALPHA( A, B, C, D, FIX )                                 \
 (    (   (  ( unsigned long int )( A )  ) & 3   )              | \
  (    (   (  ( unsigned long int )( B   )  ) & 3   ) << 2    ) | \
  (    (   (  ( unsigned long int )( C   )  ) & 3   ) << 4    ) | \
  (    (   (  ( unsigned long int )( D   )  ) & 3   ) << 6    ) | \
  (    (   (  ( unsigned long int )( FIX )  )   )     << 32UL )   \
 )

#  define ALPHA_BLEND_NORMAL      (  ALPHA( ALPHA_SRC, ALPHA_DST,  ALPHA_SRC, ALPHA_DST, 0x00 )  )
#  define ALPHA_BLEND_ADD_NOALPHA (  ALPHA( ALPHA_SRC, ALPHA_ZERO, ALPHA_FIX, ALPHA_DST, 0x80 )  )
#  define ALPHA_BLEND_ADD         (  ALPHA( ALPHA_SRC, ALPHA_ZERO, ALPHA_SRC, ALPHA_DST, 0x00 )  )

typedef enum GSFieldMode {

 GSFieldMode_Field = 0x00,
 GSFieldMode_Frame = 0x01

} GSFieldMode;

typedef enum GSAxis {

 GSAxis_X = 0x00,
 GSAxis_Y = 0x01,
 GSAxis_U = 0x03,
 GSAxis_V = 0x04

} GSAxis;

typedef enum GSPSM {

 GSPSM_32  = 0x00,
 GSPSM_24  = 0x01,
 GSPSM_16  = 0x02,
 GSPSM_16S = 0x0A,
 GSPSM_8   = 0x13,
 GSPSM_4   = 0x14

} GSPSM;

typedef enum GSZSM {

 GSZSM_32  = 0x00,
 GSZSM_24  = 0x01,
 GSZSM_16  = 0x02,
 GSZSM_16S = 0x0A

} GSZSM;

typedef enum GSOnOff {

 GS_OFF = 0x00,
 GS_ON  = 0x01

} GSOnOff;

typedef enum GSIconSize {

 GSIS_48x48 = 48,
 GSIS_32x32 = 32

} GSIconSize;

typedef enum GSAlphaBlend {

 GSAlphaBlend_Front2Back = 0x12,
 GSAlphaBlend_Back2Front = 0x01

} GSAlphaBlend;

typedef enum GSClampMode {

 GSClampMode_Repeat       = 0x00,
 GSClampMode_Clamp        = 0x01,
 GSClampMode_RegionClamp  = 0x02,
 GSClampMode_RegionRepeat = 0x03

} GSClampMode;

typedef enum GSBkMode {

 GSBkMode_Transparent = 0x00,
 GSBkMode_Opaque      = 0x01

} GSBkMode;

typedef struct GSBgClr {

 unsigned char m_Red;
 unsigned char m_Green;
 unsigned char m_Blue;

} GSBgClr;

typedef struct GSTest {

 unsigned char m_ATE;
 unsigned char m_ATST;
 unsigned char m_AREF;
 unsigned char m_AFAIL;
 unsigned char m_DATE;
 unsigned char m_DATM;
 unsigned char m_ZTE;
 unsigned char m_ZTST;

} GSTest;

typedef struct GSClamp {

 GSClampMode m_WMS;
 GSClampMode m_WMT;
 int         m_MINU;
 int         m_MAXU;
 int         m_MINV;
 int         m_MAXV;

} GSClamp;

typedef struct GSVertex {

 int m_X;
 int m_Y;
 int m_Z;

} GSVertex;

typedef struct GSTexVertex {

 int m_U;
 int m_V;

} GSTexVertex;

typedef struct GSLineStrip {

 int               m_nAlloc;
 int               m_nPoints;
 GSVertex*         m_pPoints;
 unsigned long int m_Color;

 void ( *Draw ) ( void );

} GSLineStrip;

typedef struct GSRectangle {

 GSVertex          m_Points[ 4 ];
 unsigned long int m_Color [ 4 ];

 void ( *Draw ) ( void );

} GSRectangle;

typedef struct GSSprite {

 GSVertex          m_Points[ 2 ];
 unsigned long int m_Color;

 void ( *Draw ) ( void );

} GSSprite;

typedef struct GSFan {

 int               m_nAlloc;
 int               m_nPoints;
 GSVertex*         m_pPoints;
 unsigned long int m_Color;

 void ( *Draw ) ( void );

} GSFan;

typedef struct GSFont {

 unsigned int m_Text;
 unsigned int m_CLUT;
 unsigned int m_BkColor;
 GSBkMode     m_BkMode;

} GSFont;

typedef struct GSContext {

 GSDisplayMode     m_DisplayMode;
 GSFieldMode       m_FieldMode;
 GSOnOff           m_fInterlace;
 GSOnOff           m_fDblBuf;
 GSOnOff           m_fZBuf;
 GSOnOff           m_fFog;  
 GSOnOff           m_fAntiAlias;
 GSOnOff           m_fAlpha;
 unsigned int      m_VRAMPtr;
 unsigned int      m_ActiveBuf;
 unsigned int      m_ScreenBufPtr[ 2 ];
 unsigned int      m_ZBufPtr;
 unsigned int      m_Width;
 unsigned int      m_Height;
 int               m_OffsetX;
 int               m_OffsetY;
 int               m_StartX;
 int               m_StartY;
 int               m_MagX;
 int               m_MagY;
 GSBgClr           m_BgClr;
 GSTest            m_Test;
 GSClamp           m_Clamp;
 GSPSM             m_PSM;
 GSZSM             m_ZSM;
 unsigned int      m_PrimCtx;
 GSAlphaBlend      m_PrimAlpha;
 unsigned long int m_FillColor;
 unsigned long int m_LineColor;
 unsigned int      m_IconPtr;
 GSFont            m_Font;

 unsigned int ( *DataSize      ) ( unsigned int, unsigned int, GSPSM );
 unsigned int ( *FBAlloc       ) ( unsigned int                      );
 unsigned int ( *TBAlloc       ) ( unsigned int                      );
 void         ( *Scale         ) ( GSVertex*, int                    );
 void         ( *ScaleUV       ) ( GSTexVertex*, int                 );
 void         ( *VSync         ) ( void                              );
 void         ( *SetTest       ) ( void                              );
 void         ( *ClearScreen   ) ( unsigned long int                 );
 void         ( *SwapBuffers   ) ( int                               );
 GSRectangle* ( *InitRectangle ) ( void                              );
 GSFan*       ( *InitFan       ) ( unsigned int                      );
 GSLineStrip* ( *InitLineStrip ) ( unsigned int                      );
 GSSprite*    ( *InitSprite    ) ( unsigned long int                 );
 void         ( *InitScreen    ) ( void                              );
 void         ( *DrawIcon      ) ( int, int, GSIconSize, void*       );
 void         ( *RoundRect     ) ( int, int, int, int, int           );
 unsigned int ( *TextWidth     ) ( char*, int                        );
 void         ( *DrawText      ) ( int, int, int, unsigned char*     );
 void         ( *SetTextColor  ) ( unsigned int                      );
 void         ( *Destroy       ) ( void                              );

} GSContext;

static int inline GS_PowerOf2 ( int aVal ) {
 int i;
 for ( i = 0; ( 1 << i ) < aVal; ++i );
 return i;
}  /* end GS_PowerOf2 */
# endif  /* _WIN32 */

# ifdef __cplusplus
extern "C"{
# endif  /* __cplusplus */

GSContext* GS_InitContext ( GSDisplayMode );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __GS_H */
