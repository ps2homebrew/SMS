/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
# Copyright 2004 - Lord_Kiron (font unpack routines - http://www.mpcclub.com/)
# Copyright 2005 - font management routines, some SMS specifics: Eugene Plotnikov <e-plotnikov@operamail.com>
# Copyright 2005 - font management routines: BraveDog
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "GS.h"
# ifdef _WIN32

static GSContext g_GSCtx;

static void GS_DestroyContext ( void ) {

 if ( g_GSCtx.m_pVideo ) {

  UnmapViewOfFile ( g_GSCtx.m_pVideo );
  g_GSCtx.m_pVideo = NULL;

 }  /* end if */

 if ( g_GSCtx.m_hMap ) {

  CloseHandle ( g_GSCtx.m_hMap );
  g_GSCtx.m_hMap = NULL;

 }  /* end if */

}  /* end GS_DestroyContext */

static void GS_InitScreen ( void ) {

}  /* end GS_InitScreen */

static void GS_ClearScreen ( DWORD aColor ) {

}  /* end GS_ClearScreen */

GSContext* GS_InitContext ( GSDisplayMode aMode ) {

 unsigned int lWidth;
 unsigned int lHeight;

 g_GSCtx.m_hMap   = NULL;
 g_GSCtx.m_pVideo = NULL;
 g_GSCtx.m_hWnd   = FindWindow (  TEXT( "DDrawSrv_Class" ), TEXT( "DDrawSrv_Window" )  );

 switch ( aMode ) {

  case GSDisplayMode_AutoDetect      :
  case GSDisplayMode_NTSC            :
  case GSDisplayMode_NTSC_I          :
  case GSDisplayMode_VGA_640x480_60Hz:
  case GSDisplayMode_VGA_640x480_72Hz:
  case GSDisplayMode_VGA_640x480_75Hz:
  case GSDisplayMode_VGA_640x480_85Hz:

   lWidth  = 640;
   lHeight = 480;

  break;

  case GSDisplayMode_PAL  :
  case GSDisplayMode_PAL_I:

   lWidth  = 640;
   lHeight = 576;

  break;

  case GSDisplayMode_VGA_800x600_56Hz:
  case GSDisplayMode_VGA_800x600_60Hz:
  case GSDisplayMode_VGA_800x600_72Hz:
  case GSDisplayMode_VGA_800x600_75Hz:
  case GSDisplayMode_VGA_800x600_85Hz:

   lWidth  = 800;
   lHeight = 600;

  break;

  case GSDisplayMode_VGA_1024x768_60Hz:
  case GSDisplayMode_VGA_1024x768_70Hz:
  case GSDisplayMode_VGA_1024x768_75Hz:
  case GSDisplayMode_VGA_1024x768_85Hz:

   lWidth  = 1024;
   lHeight =  768;

  break;

  case GSDisplayMode_VGA_1280x1024_60Hz:
  case GSDisplayMode_VGA_1280x1024_75Hz:

   lWidth  = 1280;
   lHeight = 1024;

  break;

  case GSDisplayMode_DTV_720x480P:

   lWidth  = 720;
   lHeight = 480;

  break;

  case GSDisplayMode_DTV_1920x1080I:

   lWidth  = 1920;
   lHeight = 1080;

  break;

  case GSDisplayMode_DTV_1280x720P:

   lWidth  = 1280;
   lHeight =  720;

  break;

  case GSDisplayMode_DVD_NTSC:

   lWidth  = 640;
   lHeight = 480;

  break;

  case GSDisplayMode_DVD_PAL:

   lWidth  = 640;
   lHeight = 576;

  break;

 }  /* end switch */

 g_GSCtx.m_Width  = lWidth;
 g_GSCtx.m_Height = lHeight;

 if (   g_GSCtx.m_hWnd && SendMessage (  g_GSCtx.m_hWnd, WM_APP, 0, MAKELPARAM( lHeight, lWidth )  )   ) {

  g_GSCtx.m_hMap = OpenFileMapping (  FILE_MAP_WRITE, FALSE, TEXT( "DDrawSrv_Image" )  );

  if ( g_GSCtx.m_hMap != NULL ) g_GSCtx.m_pVideo = ( unsigned char* )MapViewOfFile ( g_GSCtx.m_hMap, FILE_MAP_WRITE, 0, 0, 0 );

 }  /* end if */

 g_GSCtx.Destroy     = GS_DestroyContext;
 g_GSCtx.InitScreen  = GS_InitScreen;
 g_GSCtx.ClearScreen = GS_ClearScreen;

 return &g_GSCtx;

}  /* end GS_InitContext */
# else  /* PS2 */
#  include "DMA.h"
#  include "VIF.h"
#  include "Config.h"

#  include <kernel.h>
#  include <malloc.h>
#  include <string.h>
#  include <stdio.h>
#  include <limits.h>

GSContext g_GSCtx;

static GSRectangle s_GSRect;
static GSFan       s_GSFan;
static GSLineStrip s_GSLineStrip;
static GSSprite    s_GSSprite;

CharKern g_Kerns[ 224 ];

static float s_Sin[ 36 ] = {
  0.000000F,  0.173648F,  0.342020F,  0.500000F,  0.642787F,  0.766044F,
  0.866025F,  0.939692F,  0.984808F,  1.000000F,  0.984808F,  0.939693F,
  0.866026F,  0.766046F,  0.642789F,  0.500002F,  0.342022F,  0.173651F,
  0.000000F, -0.173645F, -0.342017F, -0.499997F, -0.642785F, -0.766042F,
 -0.866024F, -0.939691F, -0.984807F, -1.000000F, -0.984808F, -0.939694F,
 -0.866028F, -0.766047F, -0.642791F, -0.500004F, -0.342025F, -0.173653F
};

static float s_Cos[ 36 ] = {
  1.000000F,  0.984808F,  0.939693F,  0.866026F,  0.766045F,  0.642788F,
  0.500000F,  0.342021F,  0.173649F,  0.000000F, -0.173647F, -0.342019F,
 -0.499998F, -0.642786F, -0.766043F, -0.866024F, -0.939692F, -0.984807F,
 -1.000000F, -0.984808F, -0.939694F, -0.866027F, -0.766047F, -0.642790F,
 -0.500000F, -0.342024F, -0.173652F,  0.000000F,  0.173644F,  0.342016F,
  0.499996F,  0.642784F,  0.766041F,  0.866023F,  0.939691F,  0.984807F
};

typedef struct FontHeader {

 char           m_ID [ 3 ]    __attribute__(  ( packed )  );
 char           m_ClrType     __attribute__(  ( packed )  );
 char           m_Unk[ 3 ]    __attribute__(  ( packed )  );
 unsigned short m_nGlyphs     __attribute__(  ( packed )  );
 unsigned char  m_GlyphWidth  __attribute__(  ( packed )  );
 unsigned char  m_GlyphHeight __attribute__(  ( packed )  );

} FontHeader;

typedef struct _Unaligned32 {

 unsigned int m_Val __attribute__(  ( packed )  );

} _Unaligned32;

static inline unsigned int _unaligned32 ( const void* apData ) {

 return (  ( const _Unaligned32* )apData  ) -> m_Val;

}  /* end _unaligned32 */

static inline unsigned char _swap_nibble ( unsigned char c ) {

 unsigned char l = c & 0x0F;
 unsigned char h = c & 0xF0;

 return ( l << 4 ) + ( h >> 4 );

}  /* end _swap_nibble */

static inline void _swap_nibbles ( unsigned char* apBuff ) {

 apBuff[ 3 ] = _swap_nibble ( apBuff[ 3 ] );
 apBuff[ 2 ] = _swap_nibble ( apBuff[ 2 ] );
 apBuff[ 1 ] = _swap_nibble ( apBuff[ 1 ] );
 apBuff[ 0 ] = _swap_nibble ( apBuff[ 0 ] );

}  /* end _swap_nibbles */

static inline void _swap_bytes ( unsigned char* apBuff ) {

 unsigned char lVal = apBuff[ 0 ];

 apBuff[ 0 ] = apBuff[ 1 ];
 apBuff[ 1 ] = lVal;
 apBuff[ 2 ] = 0;
 apBuff[ 3 ] = 0;

}  /* end _swap_bytes */

static unsigned int _next_block ( unsigned char* apBuf, int aBufPos, unsigned int* apColor, unsigned int* apLen ) {

 unsigned int   lBytePos = aBufPos / 8;
 unsigned int   lHiLo    = aBufPos % 8;
 unsigned int   lData;
 unsigned char* lpData;
 unsigned char  lTest;

 lpData = apBuf + lBytePos;
 lData  = _unaligned32 ( lpData );

 if ( lHiLo > 0 ) {

  _swap_nibbles (  ( unsigned char* )&lData  );
  lData = lData >> 4;
  _swap_nibbles (  ( unsigned char* )&lData  );

 }  /* end if */

 lTest = lData & 0xFF;

 _swap_bytes (  ( unsigned char* )&lData  );


 if (  !( lTest & 0xFC )  ) {

  *apColor = lData & 0x03;
  *apLen   = ( lData & 0x03FC ) >> 2;
  aBufPos += 16;

 } else  if (  !( lTest & 0xF0 )  ) {

  *apColor = ( lData & 0x30  ) >> 4;
  *apLen   = ( lData & 0xFC0 ) >> 6;
  aBufPos += 12;

 } else if (  !( lTest & 0xC0 )  ) {

  *apColor = lTest & 0x03;
  *apLen   = ( lTest & 0x3C ) >> 2;
  aBufPos += 8;

 } else {

  lTest  >>= 4;
  *apColor = lTest & 0x03;
  *apLen   = ( lTest & 0x0C ) >> 2;
  aBufPos += 4;

 }  /* end else */

 *apColor -= 1;

 return aBufPos;    

}  /* end _next_block */

static void _font_character ( FontHeader* apHdr, unsigned int aChr, void* apBuf, short* apKerns ) {

 unsigned char* lpFont   = ( unsigned char* )apHdr;
 unsigned char* lpBuff   = ( unsigned char* )apBuf;
 unsigned int   lWidth   = ( lpFont + 11 )[ aChr ];
 unsigned int*  lpOffs   = ( unsigned int* )( lpFont + 11 + apHdr -> m_nGlyphs + 1 );
 unsigned char* lpOffset = ( unsigned char* )lpOffs + ( apHdr -> m_nGlyphs << 2 ) + lpOffs[ aChr ] + 4;
 unsigned int   lBegin   = 0;
 unsigned int   lEnd     = lWidth * apHdr -> m_GlyphHeight;
 unsigned int   lBufPos  = 0;
 unsigned int   lFlag    = 0;
 unsigned int   lDIdx    = 0;
 unsigned int   lSIdx    = 0;
 unsigned char  lPixel   = 0;
 unsigned int   lColor;
 unsigned int   lLength;
 unsigned int   i;
          int   j;

 memset ( apBuf, 0, 512 );

 while ( lBegin < lEnd ) {

  lBufPos = _next_block ( lpOffset, lBufPos, &lColor, &lLength );

  for ( i = 0; i < lLength; ++i ) {

   if ( !lFlag ) {

    lFlag  = 1;
    lPixel = lColor;

   } else {

    lFlag             = 0;
    lPixel           |= ( lColor & 0x0F ) << 4;
    lpBuff[ lDIdx++ ] = lPixel;

   }  /* end else */

   if ( ++lSIdx == lWidth ) {

    if ( lFlag ) {

     lFlag           = 0;
     lpBuff[ lDIdx ] = lPixel;

    }  /* end if */

    lSIdx   =  0;
    lpBuff += 16;
    lDIdx   =  0;

   }  /* end if */

  }  /* end for */

  lBegin += lLength;

 }  /* end while */

 lpBuff = ( unsigned char* )apBuf;

 for ( i = 0; i < 32; ++i, lpBuff += 16 ) {

  int   lGap  = 0;
  short lKern = 0;

  for ( j = 0; j < 16; ++j ) {

   lColor = lpBuff[ j ];

   if (  !( lColor & 0xF0 )  ) {
    ++lGap;
   } else break;

   if (  !( lColor & 0x0F )  )
    ++lGap;
   else break;

  }  /* end for */

  lKern = lGap;
  lGap  = 0;

  for ( j = 15; j >= 0; --j ) { 

   lColor = lpBuff[ j ];

   if (  !( lColor & 0x0F )  )
    ++lGap;
   else break;

   if (  !( lColor & 0xF0 )  )
    ++lGap;
   else break;

  }  /* end for */

  lKern     |= ( lGap + 2 ) << 8;
  *apKerns++ = lKern;

 }  /* end for */

}  /* end _font_character */

static void _font_set_text_color ( u32 aCLUTIdx, u32 aColor ) {

 u32 lCLUT[ 16 ] __attribute__(  (  aligned( 16 )  )   );
 u64 lDMA [ 22 ] __attribute__(  (  aligned( 16 )  )   );

 lCLUT[ 0 ] = 0;
 lCLUT[ 1 ] = GS_SETREG_RGBA( 0, 0, 0, aColor >> 24 );
 lCLUT[ 2 ] = aColor;

 *GS_CSR = *GS_CSR | 2;

 lDMA[ 0 ] = DMA_TAG( 6, 1, DMA_CNT, 0, 0, 0 );
 lDMA[ 1 ] = 0;

  lDMA[ 2 ] = GIF_TAG( 4, 0, 0, 0, 0, 1 );
  lDMA[ 3 ] = GIF_AD;

   lDMA[ 4 ] = GS_SETREG_BITBLTBUF( 0, 0, 0, g_GSCtx.m_Font.m_CLUT[ aCLUTIdx ], 1, GSPSM_32 );
   lDMA[ 5 ] = GS_BITBLTBUF;

   lDMA[ 6 ] = GS_SETREG_TRXPOS( 0, 0, 0, 0, 0 );
   lDMA[ 7 ] = GS_TRXPOS;

   lDMA[ 8 ] = GS_SETREG_TRXREG( 8, 2 );
   lDMA[ 9 ] = GS_TRXREG;

   lDMA[ 10 ] = GS_SETREG_TRXDIR( 0 );
   lDMA[ 11 ] = GS_TRXDIR;

  lDMA[ 12 ] = GIF_TAG( 4, 0, 0, 0, 2, 1 );
  lDMA[ 13 ] = 0;

 lDMA[ 14 ] = DMA_TAG(  4, 1, DMA_REF, 0, ( u32 )lCLUT, 0  );
 lDMA[ 15 ] = 0;

 lDMA[ 16 ] = DMA_TAG(  2, 1, DMA_END, 0, 0, 0  );
 lDMA[ 17 ] = 0;

  lDMA[ 18 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
  lDMA[ 19 ] = GIF_AD;

  lDMA[ 20 ] = 0;
  lDMA[ 21 ] = GS_FINISH;

 SyncDCache ( lCLUT, lCLUT + 48 );

 DMA_SendChain ( DMA_CHANNEL_GIF, lDMA, 11 );
 DMA_Wait ( DMA_CHANNEL_GIF );

 while (  !( *GS_CSR & 2 )  );

}  /* end _font_set_text_color */

extern unsigned char g_ASCII[ 4572 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1250 [ 7736 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1251 [ 7856 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1252 [ 7732 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1253 [ 7588 ] __attribute__(   (  section( ".data" )  )   );

static FontHeader* s_pASCII     = ( FontHeader* )g_ASCII;
static FontHeader* s_Fonts[ 4 ] = {
 ( FontHeader* )g_1250,
 ( FontHeader* )g_1251,
 ( FontHeader* )g_1252,
 ( FontHeader* )g_1253
};

static void _font_init ( void ) {

 unsigned int   lX, lY, lIdx, lCharIdx = 0, lStrideIdx = 0;
 unsigned char  lCharBuf[ 512 ] __attribute__(   (  aligned( 16 )  )   );
 u64            lDMA    [  16 ] __attribute__(   (  aligned( 16 )  )   );
 FontHeader*    lpFont = s_Fonts[ g_GSCtx.m_Font.m_CodePage ];

 g_GSCtx.m_Font.m_BkColor = 0x80000000;
 g_GSCtx.m_Font.m_BkMode  = GSBkMode_Transparent;

 lDMA[  0 ] = DMA_TAG( 6, 1, DMA_CNT, 0, 0, 0 );
 lDMA[  1 ] = 0;
 lDMA[  2 ] = GIF_TAG( 4, 1, 0, 0, 0, 1 );
 lDMA[  3 ] = GIF_AD;
 lDMA[  4 ] = GS_SETREG_BITBLTBUF( 0, 0, 0, g_GSCtx.m_Font.m_Text, 16, GSPSM_4 );
 lDMA[  5 ] = GS_BITBLTBUF;
 lDMA[  8 ] = GS_SETREG_TRXREG( 32, 32 );
 lDMA[  9 ] = GS_TRXREG;
 lDMA[ 10 ] = GS_SETREG_TRXDIR( 0 );
 lDMA[ 11 ] = GS_TRXDIR;
 lDMA[ 12 ] = GIF_TAG( 32, 1, 0, 0, 2, 1 );
 lDMA[ 13 ] = 0;
 lDMA[ 14 ] = DMA_TAG(  32, 1, DMA_REFE, 0, ( u32 )lCharBuf, 0  );
 lDMA[ 15 ] = 0;

 lY = 0;
 lX = 0;

 for ( lIdx = 0; lIdx < 96; ++lIdx, ++lCharIdx ) {

  _font_character (  s_pASCII, lIdx, lCharBuf, ( short* )&g_Kerns[ lCharIdx ]  );

  lDMA[ 6 ] = GS_SETREG_TRXPOS( 0, 0, lX, lY, 0 );
  lDMA[ 7 ] = GS_TRXPOS;

  SyncDCache ( lCharBuf, lCharBuf + 512 );

  DMA_SendChain ( DMA_CHANNEL_GIF, lDMA, 8 );
  DMA_Wait ( DMA_CHANNEL_GIF );

  if ( ++lStrideIdx == 31 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 for ( lIdx = 0; lIdx < 128; ++lIdx, ++lCharIdx ) {

  _font_character ( lpFont, lIdx, lCharBuf, ( short* )&g_Kerns[ lCharIdx ] );

  lDMA[ 6 ] = GS_SETREG_TRXPOS( 0, 0, lX, lY, 0 );
  lDMA[ 7 ] = GS_TRXPOS;

  SyncDCache ( lCharBuf, lCharBuf + 512 );

  DMA_SendChain ( DMA_CHANNEL_GIF, lDMA, 8 );
  DMA_Wait ( DMA_CHANNEL_GIF );

  if ( ++lStrideIdx == 31 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 for ( lX = 0; lX < 32; ++lX ) {

  g_Kerns[ 0 ].m_Kern[ lX ].m_Left  =
  g_Kerns[ 0 ].m_Kern[ lX ].m_Right = 11;

 }  /* end for */

 lDMA[ 0 ] = GIF_TAG( 2, 1, 0, 0, 0, 1 );
 lDMA[ 1 ] = GIF_AD;

 lDMA[ 2 ] = ALPHA_BLEND_NORMAL;
 lDMA[ 3 ] = GS_ALPHA_1 + !g_GSCtx.m_PrimCtx;

 lDMA[ 4 ] = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 lDMA[ 5 ] = GS_TEX1_1 + !g_GSCtx.m_PrimCtx;

 DMA_Send( DMA_CHANNEL_GIF, lDMA, 3 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end _font_init */

static unsigned int _font_text_width ( char* apStr, int anChars ) {

 int      lKern, lX[ 32 ];
 unsigned int i, j, lnChars = anChars ? anChars : strlen ( apStr );
 unsigned int lChr = ( unsigned char )apStr[ 0 ] - ' ';

 for ( i = 0; i < 32; ++i ) lX[ i ] = g_Kerns[ lChr ].m_Kern[ i ].m_Left;

 for ( i = 0; i < lnChars; ++i ) {

  lChr = ( unsigned char )apStr[ i ] - ' ';

  lKern = -INT_MAX;

  for ( j = 0; j < 32; ++j ) {

   int lOffset = lX[ j ] - g_Kerns[ lChr ].m_Kern[ j ].m_Left;

   if ( lOffset > lKern ) lKern = lOffset;

  }  /* end for */

  for ( j = 0; j < 32; ++j )

   lX[ j ] = lKern + 32 - g_Kerns[ lChr ].m_Kern[ j ].m_Right;

 }  /* end for */

 lKern = -INT_MAX;

 for ( i = 0; i < 32; ++i ) if ( lX[ i ] > lKern ) lKern = lX[ i ];

 return lKern;

}  /* end _font_text_width */

static void _font_draw_text ( int aX, int aY, int aZ, unsigned char* apStr, int aLen, int aColorIdx ) {

 int  i, j, lnChars = aLen ? aLen : strlen ( apStr );
 int  lIncr = g_GSCtx.m_Font.m_BkMode == GSBkMode_Opaque;
 u64  lDMA[ ( lnChars << 2 ) + 12 ] __attribute__(  (  aligned( 16 )  )   );
 u64* lpDMA   = &lDMA[ 2 ];
 int  lDMALen = 1;
 int  lX1, lX2;
 int  lY1, lY2;
 int  lU1, lU2;
 int  lV1, lV2;
 int  lXIncr = g_GSCtx.m_OffsetX << 4;
 int  lYIncr = g_GSCtx.m_OffsetY << 4;
 int  lTX, lTY;
 int  lCurX;
 int  lX[ 32 ];

 for ( i = 0; i < 32; ++i ) lX[ i ] = aX;

 lY1 = ( aY << 3 ) + lYIncr;
 lY2 = (  ( aY + 32 ) << 3  ) + lYIncr;

 if ( lIncr ) {

  lX1 = ( aX << 4 ) + lXIncr;
  lX2 = (   (  aX + _font_text_width ( apStr, lnChars )  ) << 4   ) + lXIncr;

  *lpDMA++ = GIF_TAG( 1, 0, 0, 0, 1, 4 );
  *lpDMA++ = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
  *lpDMA++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, g_GSCtx.m_fFog, g_GSCtx.m_fAlpha, g_GSCtx.m_fAntiAlias, 0, !g_GSCtx.m_PrimCtx, 0 );
  *lpDMA++ = g_GSCtx.m_Font.m_BkColor;
  *lpDMA++ = GS_SETREG_XYZ( lX1, lY1, aZ );
  *lpDMA++ = GS_SETREG_XYZ( lX2, lY2, aZ );

  lDMALen += 3;

 }  /* end if */

 lDMALen += 3 + ( lnChars << 1 );

 lDMA[ 0 ] = 0;
 lDMA[ 1 ] = VIF_DIRECT( lDMALen - 1 );

 *lpDMA++ = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 *lpDMA++ = ( GS_TEX0_1 + !g_GSCtx.m_PrimCtx ) | ( GS_PRIM << 4 );
 *lpDMA++ = GS_SETREG_TEX0( g_GSCtx.m_Font.m_Text, 16, GSPSM_4, 10, 8, 1, 1, g_GSCtx.m_Font.m_CLUT[ aColorIdx ], 0, 0, 0, 1 );
 *lpDMA++ = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, !g_GSCtx.m_PrimCtx, 0 );

 *lpDMA++ = GIF_TAG( lnChars, 1, 0, 0, 1, 4 );
 *lpDMA++ = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 for ( i = 0; i < lnChars; ++i ) {

  unsigned char lChr = apStr[ i ] - ' ';

  lCurX = -INT_MAX;

  for ( j = 0; j < 32; ++j ) {

   int lOffset = lX[ j ] - g_Kerns[ lChr ].m_Kern[ j ].m_Left;

   if ( lOffset > lCurX ) lCurX = lOffset;

  }  /* end for */

  lX1  = ( lCurX << 4 ) + lXIncr;
  lX2  = (  ( lCurX + 32 ) << 4  ) + lXIncr;

  for ( j = 0; j < 32; ++j ) lX[ j ] = lCurX + 32 - g_Kerns[ lChr ].m_Kern[ j ].m_Right;

  lTY = 1;

  while ( lChr > 30 ) {

   lChr -= 31;
   lTY  += 32;

  }  /* end while */

  lTX = lChr * 32;

  lU1 = ( lTX << 4 ) + lXIncr;
  lU2 = (  ( lTX + 32 ) << 4  ) + lXIncr;

  lV1 = ( lTY << 4 ) + lXIncr;
  lV2 = (  ( lTY + 32 ) << 4  ) + lXIncr;

  *lpDMA++ = GS_SETREG_UV( lU1, lV1 );
  *lpDMA++ = GS_SETREG_XYZ( lX1, lY1, aZ );
  *lpDMA++ = GS_SETREG_UV( lU2, lV2 );
  *lpDMA++ = GS_SETREG_XYZ( lX2, lY2, aZ );

 }  /* end for */

 DMA_Send ( DMA_CHANNEL_VIF1, lDMA, lDMALen );
 DMA_Wait ( DMA_CHANNEL_VIF1 );

}  /* end _font_draw_text */

static int _font_gs_packet ( int aX, int aY, int aZ, unsigned char* apStr, int aLen, u64** appRetVal, int aColorIdx ) {

 int  lnChars = aLen ? aLen : strlen ( apStr );
 int  i, j, k, lLen = FONT_GSP_SIZE( lnChars );
 u64* lpDMA = *appRetVal ? *appRetVal : (   *appRetVal = ( u64* )malloc (  lLen * sizeof ( u64 )  )   );
 int  lX1, lX2;
 int  lY1, lY2;
 int  lU1, lU2;
 int  lV1, lV2;
 int  lXIncr = g_GSCtx.m_OffsetX << 4;
 int  lYIncr = g_GSCtx.m_OffsetY << 4;
 int  lTX, lTY;
 int  lCurX;
 int  lX[ 32 ];

 for ( i = 0; i < 32; ++i ) lX[ i ] = aX;

 lY1 = ( aY << 3 ) + lYIncr;
 lY2 = (  ( aY + 32 ) << 3  ) + lYIncr;

 lLen >>= 1;

 lpDMA[ 0 ] = 0;
 lpDMA[ 1 ] = VIF_DIRECT( lLen - 1 );
 lpDMA[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 lpDMA[ 3 ] = ( GS_TEX0_1 + !g_GSCtx.m_PrimCtx ) | ( GS_PRIM << 4 );
 lpDMA[ 4 ] = GS_SETREG_TEX0( g_GSCtx.m_Font.m_Text, 16, GSPSM_4, 10, 8, 1, 1, g_GSCtx.m_Font.m_CLUT[ aColorIdx ], 0, 0, 0, 1 );
 lpDMA[ 5 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, !g_GSCtx.m_PrimCtx, 0 );
 lpDMA[ 6 ] = GIF_TAG( lnChars, 1, 0, 0, 1, 4 );
 lpDMA[ 7 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 for ( i = 0, k = 8; i < lnChars; ++i, k += 4 ) {

  unsigned char lChr = apStr[ i ] - ' ';

  lCurX = -INT_MAX;

  for ( j = 0; j < 32; ++j ) {

   int lOffset = lX[ j ] - g_Kerns[ lChr ].m_Kern[ j ].m_Left;

   if ( lOffset > lCurX ) lCurX = lOffset;

  }  /* end for */

  lX1  = ( lCurX << 4 ) + lXIncr;
  lX2  = (  ( lCurX + 32 ) << 4  ) + lXIncr;

  for ( j = 0; j < 32; ++j ) lX[ j ] = lCurX + 32 - g_Kerns[ lChr ].m_Kern[ j ].m_Right;

  lTY = 1;

  while ( lChr > 30 ) {

   lChr -= 31;
   lTY  += 32;

  }  /* end while */

  lTX = lChr * 32;

  lU1 = ( lTX << 4 ) + lXIncr;
  lU2 = (  ( lTX + 32 ) << 4  ) + lXIncr;

  lV1 = ( lTY << 4 ) + lXIncr;
  lV2 = (  ( lTY + 32 ) << 4  ) + lXIncr;

  lpDMA[ k + 0 ] = GS_SETREG_UV( lU1, lV1 );
  lpDMA[ k + 1 ] = GS_SETREG_XYZ( lX1, lY1, aZ );
  lpDMA[ k + 2 ] = GS_SETREG_UV( lU2, lV2 );
  lpDMA[ k + 3 ] = GS_SETREG_XYZ( lX2, lY2, aZ );

 }  /* end for */

 return lLen;

}  /* end _font_gs_packet */

static void GS_InitScreen ( GSCodePage aCodePage ) {

 unsigned long int  lDMA[ 32 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned char      lMode;
 unsigned int       lFBH;
 unsigned int       lfSoft = g_Config.m_PlayerFlags & SMS_PF_BLUR ? 1 : 0;

 g_GSCtx.m_Font.m_CodePage = aCodePage;

 if ( g_GSCtx.m_DisplayMode == GSDisplayMode_NTSC_I ) {

  lMode = GSDisplayMode_NTSC;
  lFBH  = g_GSCtx.m_Height;

 } else if ( g_GSCtx.m_DisplayMode == GSDisplayMode_PAL_I ) {

  lMode = GSDisplayMode_PAL;
  lFBH  = g_GSCtx.m_Height;

 } else if ( g_GSCtx.m_DisplayMode == GSDisplayMode_PAL ||
             g_GSCtx.m_DisplayMode == GSDisplayMode_NTSC
        ) {

  lMode = g_GSCtx.m_DisplayMode;
  lFBH  = g_GSCtx.m_Height;

 } else {

  lMode = g_GSCtx.m_DisplayMode;
  lFBH  = g_GSCtx.m_Height;

 }  /* end else */

 g_GSCtx.m_VRAMPtr = 0;
 SetGsCrt ( g_GSCtx.m_fInterlace, lMode, g_GSCtx.m_FieldMode );

 if ( g_GSCtx.m_fZBuf == GS_OFF ) {

  g_GSCtx.m_Test.m_ZTE  = 1;
  g_GSCtx.m_Test.m_ZTST = 1;

 }  /* end if */

 if ( !lfSoft )

  GS_SET_PMODE( 1, 0, 1, 0, 0, 0xFF );

 else GS_SET_PMODE( 1, 1, 1, 0, 0, 0x70 );

 GS_SET_DISPFB1( 0, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM, 0, 0      );
 GS_SET_DISPFB2( 0, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM, 0, lfSoft );

 g_GSCtx.AdjustDisplay ( 0, 0 );

 GS_SET_BGCOLOR(
  g_GSCtx.m_BgClr.m_Red,
  g_GSCtx.m_BgClr.m_Green,
  g_GSCtx.m_BgClr.m_Blue
 );

 g_GSCtx.m_ScreenBufPtr[ 0 ] = g_GSCtx.FBAlloc (
  g_GSCtx.DataSize ( g_GSCtx.m_Width, lFBH, g_GSCtx.m_PSM )
 ); 

 g_GSCtx.m_ScreenBufPtr[ 1 ] = g_GSCtx.m_fDblBuf == GS_OFF ? g_GSCtx.m_ScreenBufPtr[ 0 ]
                                                           : g_GSCtx.FBAlloc (
                                                              g_GSCtx.DataSize ( g_GSCtx.m_Width, lFBH, g_GSCtx.m_PSM )
                                                             );
 if ( g_GSCtx.m_fZBuf == GS_ON )

  g_GSCtx.m_ZBufPtr = g_GSCtx.FBAlloc (
   g_GSCtx.DataSize (  g_GSCtx.m_Width, lFBH, ( GSPSM )g_GSCtx.m_ZSM  )
  );

 *lpDMA++ = GIF_TAG( 14, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 *lpDMA++ = 1;
 *lpDMA++ = GS_PRMODECONT;

 *lpDMA++ = GS_SETREG_FRAME_1( g_GSCtx.m_ScreenBufPtr[ 0 ], g_GSCtx.m_Width / 64, g_GSCtx.m_PSM, 0 );
 *lpDMA++ = GS_FRAME_1;

 *lpDMA++ = GS_SETREG_XYOFFSET_1( g_GSCtx.m_OffsetX << 4, g_GSCtx.m_OffsetY << 4 );
 *lpDMA++ = GS_XYOFFSET_1;

 *lpDMA++ = GS_SETREG_SCISSOR_1( 0, g_GSCtx.m_Width - 1, 0, g_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_1;
 *lpDMA++ = GS_SETREG_SCISSOR_2( 0, g_GSCtx.m_Width - 1, 0, g_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_2;

 if ( g_GSCtx.m_fZBuf == GS_ON ) {

  *lpDMA++ = GS_SETREG_ZBUF_1( g_GSCtx.m_ZBufPtr / 8192, g_GSCtx.m_ZSM, 0 );
  *lpDMA++ = GS_ZBUF_1;

 } else {

  *lpDMA++ = GS_SETREG_ZBUF_1( 0, g_GSCtx.m_ZSM, 1 );
  *lpDMA++ = GS_ZBUF_1;

 }  /* end else */

 *lpDMA++ = GS_SETREG_TEST_1(
  g_GSCtx.m_Test.m_ATE,  g_GSCtx.m_Test.m_ATST, 
  g_GSCtx.m_Test.m_AREF, g_GSCtx.m_Test.m_AFAIL, 
  g_GSCtx.m_Test.m_DATE, g_GSCtx.m_Test.m_DATM,
  g_GSCtx.m_Test.m_ZTE,  g_GSCtx.m_Test.m_ZTST
 );
 *lpDMA++ = GS_TEST_1;

 *lpDMA++ = GS_SETREG_CLAMP_1(
  g_GSCtx.m_Clamp.m_WMS,  g_GSCtx.m_Clamp.m_WMT, 
  g_GSCtx.m_Clamp.m_MINU, g_GSCtx.m_Clamp.m_MAXU, 
  g_GSCtx.m_Clamp.m_MINV, g_GSCtx.m_Clamp.m_MAXV
 );
 *lpDMA++ = GS_CLAMP_1;

 *lpDMA++ = GS_SETREG_COLCLAMP( 255 );
 *lpDMA++ = GS_COLCLAMP;

 *lpDMA++ = GS_SETREG_FRAME_2( g_GSCtx.m_ScreenBufPtr[ 0 ], g_GSCtx.m_Width / 64, g_GSCtx.m_PSM, 0 );
 *lpDMA++ = GS_FRAME_2;

 *lpDMA++ = GS_SETREG_XYOFFSET_2( g_GSCtx.m_OffsetX << 4, g_GSCtx.m_OffsetY << 4 );
 *lpDMA++ = GS_XYOFFSET_2;

 *lpDMA++ = GS_SETREG_SCISSOR_2( 0, g_GSCtx.m_Width - 1, 0, g_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_2;

 if ( g_GSCtx.m_fZBuf == GS_ON ) {

  *lpDMA++ = GS_SETREG_ZBUF_2( g_GSCtx.m_ZBufPtr / 8192, g_GSCtx.m_ZSM, 0 );
  *lpDMA++ = GS_ZBUF_2;

 } else {

  *lpDMA++ = GS_SETREG_ZBUF_2( NULL, g_GSCtx.m_ZSM, 1 );
  *lpDMA++ = GS_ZBUF_2;

 }  /* end else */

 *lpDMA++ = GS_SETREG_TEST_2(
  g_GSCtx.m_Test.m_ATE,  g_GSCtx.m_Test.m_ATST, 
  g_GSCtx.m_Test.m_AREF, g_GSCtx.m_Test.m_AFAIL, 
  g_GSCtx.m_Test.m_DATE, g_GSCtx.m_Test.m_DATM,
  g_GSCtx.m_Test.m_ZTE,  g_GSCtx.m_Test.m_ZTST
 );
 *lpDMA++ = GS_TEST_2;

 *lpDMA++ = GS_SETREG_CLAMP_2(
  g_GSCtx.m_Clamp.m_WMS,  g_GSCtx.m_Clamp.m_WMT, 
  g_GSCtx.m_Clamp.m_MINU, g_GSCtx.m_Clamp.m_MAXU, 
  g_GSCtx.m_Clamp.m_MINV, g_GSCtx.m_Clamp.m_MAXV
 );
 *lpDMA   = GS_CLAMP_2;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 15 );
 DMA_Wait ( DMA_CHANNEL_GIF );

 g_GSCtx.m_IconPtr   = g_GSCtx.m_VRAMPtr / 256;
 g_GSCtx.m_VRAMPtr  += 13312;
 g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x00 );
 g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );

 g_GSCtx.m_Font.m_Text = g_GSCtx.m_VRAMPtr / 256;
 g_GSCtx.m_VRAMPtr    += 129536;

 for ( lFBH = 0; lFBH < 4; ++lFBH ) {

  g_GSCtx.m_Font.m_CLUT[ lFBH ] = g_GSCtx.m_VRAMPtr / 256;
  _font_set_text_color ( lFBH, 0x80FFFFFF );
  g_GSCtx.m_VRAMPtr += 256;

 }  /* end for */

 _font_init ();

}  /* end GS_InitScreen */

static unsigned int GS_DataSize ( unsigned int aWidth, unsigned int aHeight, GSPSM aPSM ) {

 switch ( aPSM ) {

  case GSPSM_32 : return ( aWidth * aHeight ) << 2;
  case GSPSM_24 : return ( aWidth * aHeight )  * 3;
  case GSPSM_16 :
  case GSPSM_16S: return ( aWidth * aHeight ) << 1;
  case GSPSM_8  : return aWidth * aHeight;
  case GSPSM_4  : return ( aWidth * aHeight ) >> 1;

 }  /* end switch */

 return 0;

}  /* end GS_DataSize */

static unsigned int GS_FBAlloc ( unsigned int aSize ) {

 unsigned int retVal = g_GSCtx.m_VRAMPtr;

 if ( aSize % 8192 ) aSize += 8192 - ( aSize % 8192 );

 g_GSCtx.m_VRAMPtr += aSize;

 return retVal;

}  /* end GS_FBAlloc */

static unsigned int GS_TBAlloc ( unsigned int aSize ) {

 unsigned int retVal = g_GSCtx.m_VRAMPtr;

 if ( aSize % 256 ) aSize = ( aSize + 256 ) - ( aSize % 256 );

 g_GSCtx.m_VRAMPtr += aSize;

 return retVal;

}  /* end GS_TBAlloc */

static void GS_Scale ( GSVertex* apPoints, int aCount ) {

 int i;

 for ( i = 0; i < aCount; ++i ) {

  apPoints[ i ].m_X <<= 4;
  apPoints[ i ].m_X  += g_GSCtx.m_OffsetX << 4;

  apPoints[ i ].m_Y <<= 3;
  apPoints[ i ].m_Y  += g_GSCtx.m_OffsetY << 4;

 }  /* end for */

}  /* end GS_Scale */

static void GS_ScaleUV ( GSTexVertex* apPoints, int aCount ) {

 int i;

 for ( i = 0; i < aCount; ++i ) {

  apPoints[ i ].m_U <<= 4;
  apPoints[ i ].m_U  += g_GSCtx.m_OffsetX << 4;

  apPoints[ i ].m_V <<= 4;
  apPoints[ i ].m_V  += g_GSCtx.m_OffsetX << 4;

 }  /* end for */

}  /* end GS_ScaleUV */

static void GS_DrawRect ( void ) {

 unsigned long int  lDMA[ 22 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned int       lSize = 10;
 int                i;

 g_GSCtx.Scale ( s_GSRect.m_Points, 4 );

 if ( g_GSCtx.m_fAlpha ) ++lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( g_GSCtx.m_fAlpha ) {

  *lpDMA++ = g_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + g_GSCtx.m_PrimCtx;

 }  /* end if */
        
 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_TRISTRIP, 1, 0, g_GSCtx.m_fFog,
  g_GSCtx.m_fAlpha, g_GSCtx.m_fAntiAlias, 0,
  g_GSCtx.m_PrimCtx, 0
 );
 *lpDMA++ = GS_PRIM;

 for ( i = 0; i < 4; ++i ) {

  *lpDMA++ = s_GSRect.m_Color[ i ];
  *lpDMA++ = GS_RGBAQ;

  *lpDMA++ = GS_SETREG_XYZ( s_GSRect.m_Points[ i ].m_X, s_GSRect.m_Points[ i ].m_Y, s_GSRect.m_Points[ i ].m_Z );
  *lpDMA++ = GS_XYZ2;

 }  /* end for */

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, lSize );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_DrawRect */

static void GS_DrawFan ( void ) {

 int                i;
 unsigned int       lSize = 4 + s_GSFan.m_nPoints;
 unsigned long int  lDMA[ lSize << 1 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;

 g_GSCtx.Scale ( s_GSFan.m_pPoints, s_GSFan.m_nPoints );

 if ( !g_GSCtx.m_fAlpha ) --lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( g_GSCtx.m_fAntiAlias == 1 ) {

  *lpDMA++ = g_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + g_GSCtx.m_PrimCtx;

 }  /* end if */

 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_TRIFAN, 0, 0, g_GSCtx.m_fFog,
  g_GSCtx.m_fAlpha, g_GSCtx.m_fAntiAlias, 0,
  g_GSCtx.m_PrimCtx, 0
 );
 *lpDMA++ = GS_PRIM;

 *lpDMA++ = s_GSFan.m_Color;
 *lpDMA++ = GS_RGBAQ;

 for ( i = 0; i < s_GSFan.m_nPoints; ++i ) {

  *lpDMA++ = GS_SETREG_XYZ( s_GSFan.m_pPoints[ i ].m_X, s_GSFan.m_pPoints[ i ].m_Y, s_GSFan.m_pPoints[ i ].m_Z );
  *lpDMA++ = GS_XYZ2;

 }  /* end for */

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, lSize );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_DrawFan */

static void GS_DrawLineStrip ( void ) {

 int                i;
 unsigned int       lSize = 4 + s_GSLineStrip.m_nPoints;
 unsigned long int  lDMA[ lSize << 1 ] __attribute__ (   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;

 g_GSCtx.Scale ( s_GSLineStrip.m_pPoints, s_GSLineStrip.m_nPoints );

 if ( !g_GSCtx.m_fAlpha ) --lSize;
        
 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;
        
 if ( g_GSCtx.m_fAlpha ) {

  *lpDMA++ = g_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + g_GSCtx.m_PrimCtx;

 }  /* end if */
        
 *lpDMA++ = GS_SETREG_PRIM( GS_PRIM_PRIM_LINESTRIP, 0, 0, 0, 0, 0, 0, g_GSCtx.m_PrimCtx, 0 );
 *lpDMA++ = GS_PRIM;

 *lpDMA++ = s_GSLineStrip.m_Color;
 *lpDMA++ = GS_RGBAQ;

 for ( i = 0; i < s_GSLineStrip.m_nPoints; ++i ) {

  *lpDMA++ = GS_SETREG_XYZ( s_GSLineStrip.m_pPoints[ i ].m_X, s_GSLineStrip.m_pPoints[ i ].m_Y, s_GSLineStrip.m_pPoints[ i ].m_Z );
  *lpDMA++ = GS_XYZ2;

 }  /* end for */

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, lSize );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_DrawLineStrip */

static void GS_RoundRect ( int aXLeft, int anYTop, int aXRight, int anYBottom, int aRadius ) {

 int          lXTmp[ 2 ];
 int          lYTmp[ 2 ];
 int          lXVal;
 int          lYVal;
 int          i;
 GSRectangle* lpRect  = g_GSCtx.InitRectangle ();
 GSFan*       lpFan   = g_GSCtx.InitFan ( 11 );
 GSLineStrip* lpStrip = g_GSCtx.InitLineStrip ( 41 );
 GSVertex*    lpPoint = lpFan   -> m_pPoints;
 GSVertex*    lpLine  = lpStrip -> m_pPoints;

 lpRect -> m_Points[ 0 ].m_X = lXTmp[ 1 ] = aXLeft + aRadius;
 lpRect -> m_Points[ 0 ].m_Y = anYTop;
 lpRect -> m_Points[ 1 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 1 ].m_Y = lYTmp[ 0 ] = anYTop  + aRadius;
 lpRect -> m_Points[ 2 ].m_X = lXTmp[ 0 ] = aXRight - aRadius;
 lpRect -> m_Points[ 2 ].m_Y = anYTop;
 lpRect -> m_Points[ 3 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 3 ].m_Y = lYTmp[ 0 ];
 lpRect -> Draw ();

 lpRect -> m_Points[ 0 ].m_X = aXLeft;
 lpRect -> m_Points[ 0 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 2 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 2 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 1 ].m_X = aXLeft;
 lpRect -> m_Points[ 1 ].m_Y = lYTmp[ 1 ] = anYBottom - aRadius;
 lpRect -> m_Points[ 3 ].m_X = lpRect -> m_Points[ 2 ].m_X;
 lpRect -> m_Points[ 3 ].m_Y = lpRect -> m_Points[ 1 ].m_Y;
 lpRect -> Draw ();

 lpRect -> m_Points[ 0 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 0 ].m_Y = lYTmp[ 1 ];
 lpRect -> m_Points[ 2 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 2 ].m_Y = lYTmp[ 1 ];
 lpRect -> m_Points[ 1 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 1 ].m_Y = anYBottom;
 lpRect -> m_Points[ 3 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 3 ].m_Y = anYBottom;
 lpRect -> Draw ();

 lpRect -> m_Points[ 0 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 0 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 1 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 1 ].m_Y = lYTmp[ 1 ];
 lpRect -> m_Points[ 2 ].m_X = aXRight;
 lpRect -> m_Points[ 2 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 3 ].m_X = aXRight;
 lpRect -> m_Points[ 3 ].m_Y = lYTmp[ 1 ];
 lpRect -> Draw ();

 lpRect -> m_Points[ 0 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 0 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 1 ].m_X = lXTmp[ 1 ];
 lpRect -> m_Points[ 1 ].m_Y = lYTmp[ 1 ];
 lpRect -> m_Points[ 2 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 2 ].m_Y = lYTmp[ 0 ];
 lpRect -> m_Points[ 3 ].m_X = lXTmp[ 0 ];
 lpRect -> m_Points[ 3 ].m_Y = lYTmp[ 1 ];
 lpRect -> Draw ();

 lpPoint -> m_X = lXTmp[ 1 ];
 lpPoint -> m_Y = lYTmp[ 0 ]; ++lpPoint;

 for ( i = 18; i >= 9; --i, ++lpPoint, ++lpLine ) {

  lXVal = lXTmp[ 1 ] + aRadius * s_Cos[ i ];
  lYVal = lYTmp[ 0 ] - aRadius * s_Sin[ i ];

  lpPoint -> m_X = lXVal; lpLine -> m_X = lXVal;
  lpPoint -> m_Y = lYVal; lpLine -> m_Y = lYVal;

 }  /* end for */

 lpFan -> Draw ();

 lpPoint = lpFan -> m_pPoints;

 lpPoint -> m_X = lXTmp[ 0 ];
 lpPoint -> m_Y = lYTmp[ 0 ]; ++lpPoint;

 for ( i = 9; i >= 0; --i, ++lpPoint, ++lpLine ) {

  lXVal = lXTmp[ 0 ] + aRadius * s_Cos[ i ];
  lYVal = lYTmp[ 0 ] - aRadius * s_Sin[ i ];

  lpPoint -> m_X = lXVal; lpLine -> m_X = lXVal;
  lpPoint -> m_Y = lYVal; lpLine -> m_Y = lYVal;

 }  /* end for */

 lpFan -> Draw ();

 lpPoint = lpFan -> m_pPoints;

 lpPoint -> m_X = lXTmp[ 0 ];
 lpPoint -> m_Y = lYTmp[ 1 ]; ++lpPoint;

 lpPoint -> m_X = lpLine -> m_X = lXTmp[ 0 ] + aRadius * s_Cos[ 0 ];
 lpPoint -> m_Y = lpLine -> m_Y = lYTmp[ 1 ] - aRadius * s_Sin[ 0 ];
 ++lpPoint; ++lpLine;

 for ( i = 35; i >= 27; --i, ++lpPoint, ++lpLine ) {

  lpPoint -> m_X = lpLine -> m_X = lXVal = lXTmp[ 0 ] + aRadius * s_Cos[ i ];
  lpPoint -> m_Y = lpLine -> m_Y = lYVal = lYTmp[ 1 ] - aRadius * s_Sin[ i ];

 }  /* end for */

 lpFan -> Draw ();

 lpPoint = lpFan -> m_pPoints;

 lpPoint -> m_X = lXTmp[ 1 ];
 lpPoint -> m_Y = lYTmp[ 1 ]; ++lpPoint;

 for ( i = 27; i >= 18; --i, ++lpPoint, ++lpLine ) {

  lpPoint -> m_X = lpLine -> m_X = lXTmp[ 1 ] + aRadius * s_Cos[ i ];
  lpPoint -> m_Y = lpLine -> m_Y = lYTmp[ 1 ] - aRadius * s_Sin[ i ];

 }  /* end for */

 lpLine -> m_X = lpStrip -> m_pPoints[ 0 ].m_X;
 lpLine -> m_Y = lpStrip -> m_pPoints[ 0 ].m_Y;

 lpFan   -> Draw ();
 lpStrip -> Draw ();

}  /* end GS_RoundRect */

static void GS_DrawIcon ( int aX, int anY, GSIconSize aSize, void* apData ) {

 u64         lDMA[ 38 ] __attribute(   (  aligned( 16 )  )   );
 u32         lQWC  = ( aSize * aSize * 4 ) >> 4;
 GSVertex    lPoints   [ 2 ];
 GSTexVertex lTexPoints[ 2 ];

 lPoints[ 0 ].m_X =  aX;
 lPoints[ 0 ].m_Y = anY;
 lPoints[ 0 ].m_Z = 0;
 lPoints[ 1 ].m_X =  aX + aSize;
 lPoints[ 1 ].m_Y = anY + aSize;
 lPoints[ 1 ].m_Z = 0;

 lTexPoints[ 0 ].m_U = 0;
 lTexPoints[ 0 ].m_V = 0;
 lTexPoints[ 1 ].m_U = aSize;
 lTexPoints[ 1 ].m_V = aSize;

 g_GSCtx.Scale   ( lPoints,    2 );
 g_GSCtx.ScaleUV ( lTexPoints, 2 );

 lDMA[ 0 ] = DMA_TAG( 6, 1, DMA_CNT, 0, 0, 0 );
 lDMA[ 1 ] = 0;

  lDMA[ 2 ] = GIF_TAG( 4, 1, 0, 0, 0, 1 );
  lDMA[ 3 ] = GIF_AD;

   lDMA[ 4 ] = GS_SETREG_BITBLTBUF( 0, 0, 0, g_GSCtx.m_IconPtr, 1, GSPSM_32 );
   lDMA[ 5 ] = GS_BITBLTBUF;

   lDMA[ 6 ] = GS_SETREG_TRXPOS( 0, 0, 0, 0, 0 );
   lDMA[ 7 ] = GS_TRXPOS;

   lDMA[ 8 ] = GS_SETREG_TRXREG( aSize, aSize );
   lDMA[ 9 ] = GS_TRXREG;

   lDMA[ 10 ] = GS_SETREG_TRXDIR( 0 );
   lDMA[ 11 ] = GS_TRXDIR;

   lDMA[ 12 ] = GIF_TAG( lQWC, 1, 0, 0, 2, 1 );
   lDMA[ 13 ] = 0;

  lDMA[ 14 ] = DMA_TAG(  lQWC, 1, DMA_REF, 0, ( u32 )apData, 0  );
  lDMA[ 15 ] = 0;

  lDMA[ 16 ] = DMA_TAG( 10, 1, DMA_END, 0, 0, 0 );
  lDMA[ 17 ] = 0;

   lDMA[ 18 ] = GIF_TAG( 9, 1, 0, 0, 0, 1 );
   lDMA[ 19 ] = GIF_AD;

   lDMA[ 20 ] = 0;
   lDMA[ 21 ] = GS_TEXFLUSH;

   lDMA[ 22 ] = GS_SETREG_TEX0(
                 g_GSCtx.m_IconPtr, 1, GSPSM_32, 6, 6,
                 1, 1, 0, 0, 0, 0, 1
                );
   lDMA[ 23 ] = GS_TEX0_1 + g_GSCtx.m_PrimCtx;

   lDMA[ 24 ] = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
   lDMA[ 25 ] = GS_TEX1_1 + g_GSCtx.m_PrimCtx;

   lDMA[ 26 ] = ALPHA_BLEND_NORMAL;
   lDMA[ 27 ] = GS_ALPHA_1 + g_GSCtx.m_PrimCtx;

   lDMA[ 28 ] = GS_SETREG_PRIM(
                 GS_PRIM_PRIM_SPRITE, 0, 1, 0,
                 1, 1, 1, g_GSCtx.m_PrimCtx, 0
                );
   lDMA[ 29 ] = GS_PRIM;

   lDMA[ 30 ] = GS_SETREG_UV( lTexPoints[ 0 ].m_U, lTexPoints[ 0 ].m_V );
   lDMA[ 31 ] = GS_UV;

   lDMA[ 32 ] = GS_SETREG_XYZ( lPoints[ 0 ].m_X, lPoints[ 0 ].m_Y, lPoints[ 0 ].m_Z );
   lDMA[ 33 ] = GS_XYZ2;

   lDMA[ 34 ] = GS_SETREG_UV( lTexPoints[ 1 ].m_U, lTexPoints[ 1 ].m_V );
   lDMA[ 35 ] = GS_UV;

   lDMA[ 36 ] = GS_SETREG_XYZ( lPoints[ 1 ].m_X, lPoints[ 1 ].m_Y, lPoints[ 1 ].m_Z );
   lDMA[ 37 ] = GS_XYZ2;

 DMA_SendChain ( DMA_CHANNEL_GIF, lDMA, 19 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_DrawIcon */

static GSRectangle* GS_InitRectangle ( void ) {

 s_GSRect.m_Points[ 0 ].m_Z =
 s_GSRect.m_Points[ 1 ].m_Z =
 s_GSRect.m_Points[ 2 ].m_Z =
 s_GSRect.m_Points[ 3 ].m_Z = 0;

 s_GSRect.m_Color[ 0 ] =
 s_GSRect.m_Color[ 1 ] =
 s_GSRect.m_Color[ 2 ] =
 s_GSRect.m_Color[ 3 ] = g_GSCtx.m_FillColor;

 s_GSRect.Draw = GS_DrawRect;

 return &s_GSRect;

}  /* end GS_InitRectangle */

static GSFan* GS_InitFan ( unsigned int aCount ) {

 int i = aCount * sizeof ( GSVertex );

 if ( s_GSFan.m_nAlloc < aCount ) {

  if ( s_GSFan.m_pPoints != NULL ) free ( s_GSFan.m_pPoints );

  s_GSFan.m_pPoints = malloc ( i );
  s_GSFan.m_nAlloc  = aCount;
  s_GSFan.Draw      = GS_DrawFan;

 }  /* end if */

 s_GSFan.m_nPoints = aCount;
 s_GSFan.m_Color   = g_GSCtx.m_FillColor;

 memset ( s_GSFan.m_pPoints, 0, i );

 return &s_GSFan;

}  /* end GS_InitFan */

static GSLineStrip* GS_InitLineStrip ( unsigned int aCount ) {

 int i = aCount * sizeof ( GSVertex );

 if ( s_GSLineStrip.m_nAlloc < aCount ) {

  if ( s_GSLineStrip.m_pPoints != NULL ) free ( s_GSLineStrip.m_pPoints );

  s_GSLineStrip.m_pPoints = malloc ( i );
  s_GSLineStrip.m_nAlloc  = aCount;
  s_GSLineStrip.Draw      = GS_DrawLineStrip;

 }  /* end if */

 s_GSLineStrip.m_nPoints = aCount;
 s_GSLineStrip.m_Color   = g_GSCtx.m_LineColor;

 memset ( s_GSLineStrip.m_pPoints, 0, i );

 return &s_GSLineStrip;

}  /* end GS_InitLineStrip */

static void GS_DrawSprite ( void ) {

 unsigned long int  lDMA[ 12 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned int       lSize = 5;

 g_GSCtx.Scale ( s_GSSprite.m_Points, 2 );

 if ( g_GSCtx.m_fAlpha ) ++lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( g_GSCtx.m_fAlpha == 1 ) {

  *lpDMA++ = g_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + g_GSCtx.m_PrimCtx;

 }  /* end if */

 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_SPRITE, 0, 0, g_GSCtx.m_fFog,
  g_GSCtx.m_fAlpha, g_GSCtx.m_fAntiAlias,
  0, g_GSCtx.m_PrimCtx, 0
 );
 *lpDMA++ = GS_PRIM;

 *lpDMA++ = s_GSSprite.m_Color;
 *lpDMA++ = GS_RGBAQ;

 *lpDMA++ = GS_SETREG_XYZ(
  s_GSSprite.m_Points[ 0 ].m_X,
  s_GSSprite.m_Points[ 0 ].m_Y,
  s_GSSprite.m_Points[ 0 ].m_Z
 );
 *lpDMA++ = GS_XYZ2;

 *lpDMA++ = GS_SETREG_XYZ(
  s_GSSprite.m_Points[ 1 ].m_X,
  s_GSSprite.m_Points[ 1 ].m_Y,
  s_GSSprite.m_Points[ 1 ].m_Z
 );
 *lpDMA++ = GS_XYZ2;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, lSize );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_DrawSprite */

static GSSprite* GS_InitSprite ( unsigned long int aColor ) {

 s_GSSprite.m_Points[ 0 ].m_Z = 0;
 s_GSSprite.m_Points[ 1 ].m_Z = 0;
 s_GSSprite.m_Color           = aColor;
 s_GSSprite.Draw              = GS_DrawSprite;

 return &s_GSSprite;

}  /* end GS_InitSprite */

static void GS_VSync ( void ) {

 *GS_CSR = *GS_CSR & 8;

 while (  !( *GS_CSR & 8 )  );

}  /* end GS_VSync */

static void GS_SetTest ( void ) {

 unsigned long int lDMA[ 4 ] __attribute__(   (  aligned( 16 )  )   );

 g_GSCtx.m_Test.m_ZTST = g_GSCtx.m_fZBuf  ? 2 : 1;
 g_GSCtx.m_Test.m_ATE  = g_GSCtx.m_fAlpha ? 1 : 0;

 lDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lDMA[ 1 ] = GIF_AD;

 lDMA[ 2 ] = GS_SETREG_TEST(
  g_GSCtx.m_Test.m_ATE,  g_GSCtx.m_Test.m_ATST,
  g_GSCtx.m_Test.m_AREF, g_GSCtx.m_Test.m_AFAIL, 
  g_GSCtx.m_Test.m_DATE, g_GSCtx.m_Test.m_DATM, 
  g_GSCtx.m_Test.m_ZTE,  g_GSCtx.m_Test.m_ZTST
 );
 lDMA[ 3 ] = GS_TEST_1 + g_GSCtx.m_PrimCtx;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 2 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_SetTest */

static void GS_ClearScreen ( unsigned long int aColor ) {

 GSOnOff   lZTest   = g_GSCtx.m_fZBuf;
 GSSprite* lpSprite = g_GSCtx.InitSprite ( aColor );

 if ( lZTest ) {

  g_GSCtx.m_fZBuf = GS_OFF;
  g_GSCtx.SetTest ();

 }  /* end if */

 lpSprite -> m_Points[ 0 ].m_X = 0;
 lpSprite -> m_Points[ 0 ].m_Y = 0;
 lpSprite -> m_Points[ 1 ].m_X = g_GSCtx.m_Width;
 lpSprite -> m_Points[ 1 ].m_Y = g_GSCtx.m_Height;
 lpSprite -> Draw ();

 if ( lZTest ) {

  g_GSCtx.m_fZBuf = lZTest;
  g_GSCtx.SetTest ();

 }  /* end if */

}  /* end GS_ClearScreen */

static void GS_CopyFBuffer ( int aDest, int aX, int anY, int aWidth, int aHeight ) {

 u64 lDMA[ 12 ] __attribute__(   (  aligned ( 16 )  )   );

 *GS_CSR = *GS_CSR | 2;

 lDMA[  0 ] = GIF_TAG( 5, 1, 0, 0, 0, 1 );
 lDMA[  1 ] = GIF_AD;
 lDMA[  2 ] = GS_SETREG_BITBLTBUF(
   g_GSCtx.m_ScreenBufPtr[ !aDest ] / 256, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM,
   g_GSCtx.m_ScreenBufPtr[  aDest ] / 256, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM
  );
 lDMA[  3 ] = GS_BITBLTBUF;
 lDMA[  4 ] = GS_SETREG_TRXPOS( aX, anY, aX, anY, 0 );
 lDMA[  5 ] = GS_TRXPOS;
 lDMA[  6 ] = GS_SETREG_TRXREG( aWidth, aHeight );
 lDMA[  7 ] = GS_TRXREG;
 lDMA[  8 ] = GS_SETREG_TRXDIR( 2 );
 lDMA[  9 ] = GS_TRXDIR;
 lDMA[ 10 ] = 0;
 lDMA[ 11 ] = GS_FINISH;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 6 );
 DMA_Wait ( DMA_CHANNEL_GIF );

 while (  !( *GS_CSR & 2 )  );

}  /* end GS_CopyFBuffer */

static void GS_CopyBuffer ( int aBuf, int aXSrc, int anYSrc, int aWidth, int aHeight, int aXDst, int anYDst ) {

 u64 lDMA[ 12 ] __attribute__(   (  aligned ( 16 )  )   );

 *GS_CSR = *GS_CSR | 2;

 lDMA[  0 ] = GIF_TAG( 5, 1, 0, 0, 0, 1 );
 lDMA[  1 ] = GIF_AD;
 lDMA[  2 ] = GS_SETREG_BITBLTBUF(
   g_GSCtx.m_ScreenBufPtr[ aBuf ] / 256, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM,
   g_GSCtx.m_ScreenBufPtr[ aBuf ] / 256, g_GSCtx.m_Width / 64, g_GSCtx.m_PSM
  );
 lDMA[  3 ] = GS_BITBLTBUF;
 lDMA[  4 ] = GS_SETREG_TRXPOS( aXSrc, anYSrc, aXDst, anYDst, 0 );
 lDMA[  5 ] = GS_TRXPOS;
 lDMA[  6 ] = GS_SETREG_TRXREG( aWidth, aHeight );
 lDMA[  7 ] = GS_TRXREG;
 lDMA[  8 ] = GS_SETREG_TRXDIR( 2 );
 lDMA[  9 ] = GS_TRXDIR;
 lDMA[ 10 ] = 0;
 lDMA[ 11 ] = GS_FINISH;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 6 );
 DMA_Wait ( DMA_CHANNEL_GIF );

 while (  !( *GS_CSR & 2 )  );

}  /* end GS_CopyBuffer */

static void GS_AdjustDisplay ( int aDX, int aDY ) {

 unsigned int lfSoft = g_Config.m_PlayerFlags & SMS_PF_BLUR ? 1 : 0;

 g_GSCtx.m_StartX += aDX;
 g_GSCtx.m_StartY += aDY;

 GS_SET_DISPLAY1(
  g_GSCtx.m_StartX, g_GSCtx.m_StartY,
  g_GSCtx.m_MagX,   g_GSCtx.m_MagY,
  ( g_GSCtx.m_Width * 4 ) - 1,
  g_GSCtx.m_Height - 1
 );

 GS_SET_DISPLAY2(
  g_GSCtx.m_StartX, g_GSCtx.m_StartY,
  g_GSCtx.m_MagX,   g_GSCtx.m_MagY,
  ( g_GSCtx.m_Width * 4 ) - 1,
  ( g_GSCtx.m_Height - lfSoft - 2 )
 );

}  /* end GS_AdjustDisplay */

static void GS_ZTest ( int afOn ) {

 static unsigned long int lDMA[ 8 ] __attribute__(   (  aligned( 16 )  )   ) = {
  0, VIF_DIRECT( 3 ), GIF_TAG( 2, 1, 0, 0, 0, 1 ), GIF_AD
 };

 lDMA[ 4 ] = GS_SETREG_TEST_1(
  g_GSCtx.m_Test.m_ATE,  g_GSCtx.m_Test.m_ATST, 
  g_GSCtx.m_Test.m_AREF, g_GSCtx.m_Test.m_AFAIL, 
  g_GSCtx.m_Test.m_DATE, g_GSCtx.m_Test.m_DATM,
  afOn,  g_GSCtx.m_Test.m_ZTST
 );
 lDMA[ 5 ] = GS_TEST_1;

 lDMA[ 6 ] = GS_SETREG_TEST_2(
  g_GSCtx.m_Test.m_ATE,  g_GSCtx.m_Test.m_ATST, 
  g_GSCtx.m_Test.m_AREF, g_GSCtx.m_Test.m_AFAIL, 
  g_GSCtx.m_Test.m_DATE, g_GSCtx.m_Test.m_DATM,
  afOn,  g_GSCtx.m_Test.m_ZTST
 );
 lDMA[ 7 ] = GS_TEST_2;

 DMA_Send ( DMA_CHANNEL_VIF1, lDMA, 4 );

}  /* end GS_ZTest */

int GS_printf ( const char* apFmt, ... ) {

 static char s_lBuf[ 1024 ] __attribute__(   (  section( ".data" )  )   );
 static int  s_lY = 0;
 static int  s_LastWidth;

 va_list lArgs;
 int     lSize;
 int     lY = s_lY;

 unsigned int lBkColor = g_GSCtx.m_Font.m_BkColor;
 GSBkMode     lBkMode  = g_GSCtx.m_Font.m_BkMode;

 va_start ( lArgs, apFmt );
  lSize = vsnprintf ( s_lBuf, 1024, apFmt, lArgs );
 va_end ( lArgs );

 if ( s_lBuf[ lSize - 1 ] == '\n' ) {

  s_lBuf[ lSize - 1 ] = '\0';
  s_lY += 32;

 }  /* end if */

 g_GSCtx.m_Font.m_BkColor = 0;
 g_GSCtx.m_Font.m_BkMode  = GSBkMode_Opaque;
 g_GSCtx.DrawText ( 0, lY, 100, s_lBuf, lSize, 0 );

 lSize = g_GSCtx.TextWidth ( s_lBuf, lSize );

 if ( lSize > s_LastWidth ) s_LastWidth = lSize;

 if ( s_lY > g_GSCtx.m_Height ) {

  GSSprite* lpSprite = g_GSCtx.InitSprite ( 0 );

  g_GSCtx.CopyBuffer (
   0, 0, 16, g_GSCtx.m_Width, ( s_lY - 32 ) >> 1, 0, 0
  );

  lpSprite -> m_Points[ 0 ].m_X = 1;
  lpSprite -> m_Points[ 0 ].m_Y = s_lY - 32;
  lpSprite -> m_Points[ 0 ].m_Z = 100;
  lpSprite -> m_Points[ 1 ].m_X = s_LastWidth;
  lpSprite -> m_Points[ 1 ].m_Y = s_lY + 32;
  lpSprite -> m_Points[ 1 ].m_Z = 100;

  lpSprite -> Draw ();

  s_lY -= 32;

 }  /* end if */

 g_GSCtx.m_Font.m_BkColor = lBkColor;
 g_GSCtx.m_Font.m_BkMode  = lBkMode;

 return lSize;

}  /* end GS_printf */

static void GS_SetCodePage ( GSCodePage aCodePage ) {

 g_GSCtx.m_Font.m_CodePage = aCodePage;

 _font_init ();

}  /* end GS_SetCodePage */

static void GS_SetFontPtr ( unsigned int anIndex, unsigned char* apFont ) {

 if ( !anIndex-- )

  s_pASCII = ( FontHeader* )apFont;

 else s_Fonts[ anIndex ] = ( FontHeader* )apFont;

}  /* end GS_SetFontPtr */

static void GS_DestroyContext ( void ) {

}  /* end GS_DestroyContext */

GSContext* GS_InitContext ( GSDisplayMode aMode ) {

 if ( aMode == GSDisplayMode_AutoDetect )

  aMode = *( volatile char* )0x1FC7FF52 == 'E' ? GSDisplayMode_PAL_I
                                               : GSDisplayMode_NTSC_I;
 g_GSCtx.m_DisplayMode = aMode;

 if ( aMode == GSDisplayMode_NTSC ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 640;	
  g_GSCtx.m_Height     = 448;
  g_GSCtx.m_StartX     = 652;
  g_GSCtx.m_StartY     =  26;
  g_GSCtx.m_MagX       =   3;
  g_GSCtx.m_MagY       =   0;

 } else if ( aMode == GSDisplayMode_NTSC_I ) {

  g_GSCtx.m_fInterlace = GS_ON;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 640;
  g_GSCtx.m_Height     = 448;
  g_GSCtx.m_StartX     = 652;
  g_GSCtx.m_StartY     =  48;
  g_GSCtx.m_MagX       =   3;
  g_GSCtx.m_MagY       =   0;

 } else if ( aMode == GSDisplayMode_PAL ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 640;

  if ( g_Config.m_ResMode == 0 )

   g_GSCtx.m_Height = 512;

  else g_GSCtx.m_Height = 540;

  g_GSCtx.m_StartX = 690;
  g_GSCtx.m_StartY =  30;
  g_GSCtx.m_MagX   =   3;
  g_GSCtx.m_MagY   =   0;

 } else if ( aMode == GSDisplayMode_PAL_I ) {
setPal_I:
  g_GSCtx.m_fInterlace = GS_ON;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 640;

  if ( g_Config.m_ResMode == 0 )

   g_GSCtx.m_Height = 512;

  else g_GSCtx.m_Height = 540;

  g_GSCtx.m_StartX = 690;
  g_GSCtx.m_StartY =  64;
  g_GSCtx.m_MagX   =   3;
  g_GSCtx.m_MagY   =   0;

 } else if ( aMode == GSDisplayMode_VGA_640x480_60Hz ||
             aMode == GSDisplayMode_VGA_640x480_72Hz ||
             aMode == GSDisplayMode_VGA_640x480_75Hz ||
             aMode == GSDisplayMode_VGA_640x480_85Hz
        ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 640;
  g_GSCtx.m_Height     = 480;

  if ( aMode == GSDisplayMode_VGA_640x480_60Hz )

   g_GSCtx.m_StartX = 280;

  else if ( aMode == GSDisplayMode_VGA_640x480_72Hz )

   g_GSCtx.m_StartX = 330;

  else if ( aMode == GSDisplayMode_VGA_640x480_75Hz )

   g_GSCtx.m_StartX = 360;

  else g_GSCtx.m_StartX = 260;

  g_GSCtx.m_StartY = 18;
  g_GSCtx.m_MagX   =  1;
  g_GSCtx.m_MagY   =  1;

 } else if ( aMode == GSDisplayMode_VGA_800x600_56Hz ||
             aMode == GSDisplayMode_VGA_800x600_60Hz || 
             aMode == GSDisplayMode_VGA_800x600_72Hz ||
             aMode == GSDisplayMode_VGA_800x600_75Hz || 
             aMode == GSDisplayMode_VGA_800x600_85Hz
        ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 800;
  g_GSCtx.m_Height     = 600;

  if ( aMode == GSDisplayMode_VGA_800x600_56Hz )

   g_GSCtx.m_StartX = 450;

  else if ( aMode == GSDisplayMode_VGA_800x600_60Hz )

   g_GSCtx.m_StartX = 465;

  else if ( aMode == GSDisplayMode_VGA_800x600_72Hz )

   g_GSCtx.m_StartX = 465;

  else if ( aMode == GSDisplayMode_VGA_800x600_75Hz )

   g_GSCtx.m_StartX = 510;

  else g_GSCtx.m_StartX = 500;

  g_GSCtx.m_StartY = 25;
  g_GSCtx.m_MagX   =  1;
  g_GSCtx.m_MagY   =  1;

 } else if ( aMode == GSDisplayMode_VGA_1024x768_60Hz ||
             aMode == GSDisplayMode_VGA_1024x768_70Hz ||
             aMode == GSDisplayMode_VGA_1024x768_75Hz ||
             aMode == GSDisplayMode_VGA_1024x768_85Hz
        ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_OFF;
  g_GSCtx.m_fZBuf      = GS_OFF;
  g_GSCtx.m_Width      = 1024;
  g_GSCtx.m_Height     =  768;
  g_GSCtx.m_StartY     =   30;
  g_GSCtx.m_MagY       =    1;

  if ( aMode == GSDisplayMode_VGA_1024x768_60Hz ) {

   g_GSCtx.m_MagX   =   1;
   g_GSCtx.m_StartX = 580;

  } else if ( aMode == GSDisplayMode_VGA_1024x768_70Hz ) {

   g_GSCtx.m_MagX   =   0;
   g_GSCtx.m_StartX = 266;

  } else if ( aMode == GSDisplayMode_VGA_1024x768_75Hz ) {

   g_GSCtx.m_MagX   =   0;
   g_GSCtx.m_StartX = 260;

  } else {

   g_GSCtx.m_MagX   =   0;
   g_GSCtx.m_StartX = 290;

  }  /* end else */

 } else if ( aMode == GSDisplayMode_VGA_1280x1024_60Hz ||
             aMode == GSDisplayMode_VGA_1280x1024_75Hz
        ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_OFF;
  g_GSCtx.m_fZBuf      = GS_OFF;
  g_GSCtx.m_Width      = 1280;
  g_GSCtx.m_Height     = 1024;
  g_GSCtx.m_StartX     =  350;
  g_GSCtx.m_StartY     =   40;
  g_GSCtx.m_MagX       =    0;
  g_GSCtx.m_MagY       =    0;

 } else if ( aMode == GSDisplayMode_DTV_720x480P ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 720;
  g_GSCtx.m_Height     = 480;
  g_GSCtx.m_StartX     = 232;
  g_GSCtx.m_StartY     =  35;
  g_GSCtx.m_MagX       =   1;
  g_GSCtx.m_MagX       =   0;

 } else if ( aMode == GSDisplayMode_DTV_1280x720P ) {

  g_GSCtx.m_fInterlace = GS_OFF;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_OFF;
  g_GSCtx.m_fZBuf      = GS_OFF;
  g_GSCtx.m_Width      = 1280;
  g_GSCtx.m_Height     =  720;
  g_GSCtx.m_StartX     =  302;
  g_GSCtx.m_StartY     =   24;
  g_GSCtx.m_MagX       =    0;
  g_GSCtx.m_MagY       =    0;

 } else if( aMode == GSDisplayMode_DTV_1920x1080I ) {

  g_GSCtx.m_fInterlace = GS_ON;
  g_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  g_GSCtx.m_fDblBuf    = GS_ON;
  g_GSCtx.m_fZBuf      = GS_ON;
  g_GSCtx.m_Width      = 1920;
  g_GSCtx.m_Height     = 1080;
  g_GSCtx.m_StartX     =  238;
  g_GSCtx.m_StartY     =   40;
  g_GSCtx.m_MagX       =    0;
  g_GSCtx.m_MagY       =    0;

 } else goto setPal_I;

 g_GSCtx.m_OffsetX    = 2048;
 g_GSCtx.m_OffsetY    = 2048;
 g_GSCtx.m_PSM        = GSPSM_24;
 g_GSCtx.m_ZSM        = GSZSM_16S;
 g_GSCtx.m_ActiveBuf  = 1;
 g_GSCtx.m_fFog       = 0;
 g_GSCtx.m_fAntiAlias = 0;
 g_GSCtx.m_fAlpha     = 1;
 g_GSCtx.m_PrimAlpha  = GSAlphaBlend_Back2Front;
 g_GSCtx.m_PrimCtx    = 0;

 g_GSCtx.m_BgClr.m_Red   = 0x00;
 g_GSCtx.m_BgClr.m_Green = 0x00;
 g_GSCtx.m_BgClr.m_Blue  = 0x00;

 g_GSCtx.m_Test.m_ATE   = 0;
 g_GSCtx.m_Test.m_ATST  = 1;
 g_GSCtx.m_Test.m_AREF  = 0x80;
 g_GSCtx.m_Test.m_AFAIL = 0;
 g_GSCtx.m_Test.m_DATE  = 0;
 g_GSCtx.m_Test.m_DATM  = 0;
 g_GSCtx.m_Test.m_ZTE   = 1;
 g_GSCtx.m_Test.m_ZTST  = 2;

 g_GSCtx.m_Clamp.m_WMS  = GSClampMode_Clamp;
 g_GSCtx.m_Clamp.m_WMT  = GSClampMode_Clamp;
 g_GSCtx.m_Clamp.m_MINU = 0;
 g_GSCtx.m_Clamp.m_MAXU = 0;
 g_GSCtx.m_Clamp.m_MINV = 0;
 g_GSCtx.m_Clamp.m_MAXV = 0;

 g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );
 g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x00 );

 g_GSCtx.InitScreen    = GS_InitScreen;
 g_GSCtx.DataSize      = GS_DataSize;
 g_GSCtx.FBAlloc       = GS_FBAlloc;
 g_GSCtx.TBAlloc       = GS_TBAlloc;
 g_GSCtx.Scale         = GS_Scale;
 g_GSCtx.ScaleUV       = GS_ScaleUV;
 g_GSCtx.DrawIcon      = GS_DrawIcon;
 g_GSCtx.RoundRect     = GS_RoundRect;
 g_GSCtx.InitRectangle = GS_InitRectangle;
 g_GSCtx.InitFan       = GS_InitFan;
 g_GSCtx.InitLineStrip = GS_InitLineStrip;
 g_GSCtx.InitSprite    = GS_InitSprite;
 g_GSCtx.VSync         = GS_VSync;
 g_GSCtx.SetTest       = GS_SetTest;
 g_GSCtx.ClearScreen   = GS_ClearScreen;
 g_GSCtx.TextWidth     = _font_text_width;
 g_GSCtx.DrawText      = _font_draw_text;
 g_GSCtx.TextGSPacket  = _font_gs_packet;
 g_GSCtx.SetTextColor  = _font_set_text_color;
 g_GSCtx.Destroy       = GS_DestroyContext;
 g_GSCtx.CopyFBuffer   = GS_CopyFBuffer;
 g_GSCtx.AdjustDisplay = GS_AdjustDisplay;
 g_GSCtx.ZTest         = GS_ZTest;
 g_GSCtx.CopyBuffer    = GS_CopyBuffer;
 g_GSCtx.printf        = GS_printf;
 g_GSCtx.SetCodePage   = GS_SetCodePage;
 g_GSCtx.SetFontPtr    = GS_SetFontPtr;

 return &g_GSCtx;

}  /* end GS_InitContext */
#endif  /* _WIN32 */
