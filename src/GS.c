/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
# Copyright 2005 - FONTM management routines, some SMS specifics: Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "GS.h"
# ifdef _WIN32

static GSContext s_GSCtx;

static void GS_DestroyContext ( void ) {

 if ( s_GSCtx.m_pVideo ) {

  UnmapViewOfFile ( s_GSCtx.m_pVideo );
  s_GSCtx.m_pVideo = NULL;

 }  /* end if */

 if ( s_GSCtx.m_hMap ) {

  CloseHandle ( s_GSCtx.m_hMap );
  s_GSCtx.m_hMap = NULL;

 }  /* end if */

}  /* end GS_DestroyContext */

static void GS_InitScreen ( void ) {

}  /* end GS_InitScreen */

static void GS_ClearScreen ( DWORD aColor ) {

}  /* end GS_ClearScreen */

GSContext* GS_InitContext ( GSDisplayMode aMode ) {

 unsigned int lWidth;
 unsigned int lHeight;

 s_GSCtx.m_hMap   = NULL;
 s_GSCtx.m_pVideo = NULL;
 s_GSCtx.m_hWnd   = FindWindow (  TEXT( "DDrawSrv_Class" ), TEXT( "DDrawSrv_Window" )  );

 switch ( aMode ) {

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

 s_GSCtx.m_Width  = lWidth;
 s_GSCtx.m_Height = lHeight;

 if (   s_GSCtx.m_hWnd && SendMessage (  s_GSCtx.m_hWnd, WM_APP, 0, MAKELPARAM( lHeight, lWidth )  )   ) {

  s_GSCtx.m_hMap = OpenFileMapping (  FILE_MAP_WRITE, FALSE, TEXT( "DDrawSrv_Image" )  );

  if ( s_GSCtx.m_hMap != NULL ) s_GSCtx.m_pVideo = ( unsigned char* )MapViewOfFile ( s_GSCtx.m_hMap, FILE_MAP_WRITE, 0, 0, 0 );

 }  /* end if */

 s_GSCtx.Destroy     = GS_DestroyContext;
 s_GSCtx.InitScreen  = GS_InitScreen;
 s_GSCtx.ClearScreen = GS_ClearScreen;

 return &s_GSCtx;

}  /* end GS_InitContext */
# else  /* PS2 */
#  include "DMA.h"

#  include <kernel.h>
#  include <malloc.h>
#  include <string.h>
#  include <stdio.h>

#  define CHAR_SIZE ( 13 * 26 )

typedef struct FontmFile {

 char          m_Sign[ 4 ];
 unsigned char m_Version[ 4 ];
 unsigned int  m_BitSize;
 unsigned int  m_BaseOffset;
 unsigned int  m_nEntries;
 unsigned int  m_PosEnd;

} FontmFile;

typedef struct Unpack {

 unsigned int	m_Size;
 unsigned int   m_Data;
 unsigned int   m_And;
 unsigned int   m_Diff;
 unsigned int   m_Shift;
 unsigned char* m_Ptr;

} Unpack;

static unsigned int s_CharMap[ 224 ] = {
   0/* ' '  */,   9/* '!' */,  40/* '"' */,  83/* '#'    */,  /*  32 -  35 */
  79/* '$'  */,  82/* '%' */,  84/* '&' */,  12/* '\''   */,  /*  36 -  39 */
  41/* '('  */,  42/* ')' */,  85/* '*' */,  59/* '+'    */,  /*  40 -  43 */
   3/* ','  */,  60/* '-' */,   4/* '.' */,  30/* '/'    */,  /*  44 -  47 */
 203/* '0'  */, 204/* '1' */, 205/* '2' */, 206/* '3'    */,  /*  48 -  51 */
 207/* '4'  */, 208/* '5' */, 209/* '6' */, 210/* '7'    */,  /*  52 -  55 */
 211/* '8'  */, 212/* '9' */,   6/* ':' */,   7/* ';'    */,  /*  56 -  59 */
  66/* '<'  */,  64/* '=' */,  67/* '>' */,   8/* '?'    */,  /*  60 -  63 */
  86/* '@'  */, 220/* 'A' */, 221/* 'B' */, 222/* 'C'    */,  /*  64 -  67 */
 223/* 'D'  */, 224/* 'E' */, 225/* 'F' */, 226/* 'G'    */,  /*  68 -  71 */
 227/* 'H'  */, 228/* 'I' */, 229/* 'J' */, 230/* 'K'    */,  /*  72 -  75 */
 231/* 'L'  */, 232/* 'M' */, 233/* 'N' */, 234/* 'O'    */,  /*  76 -  79 */
 235/* 'P'  */, 236/* 'Q' */, 237/* 'R' */, 238/* 'S'    */,  /*  80 -  83 */
 239/* 'T'  */, 240/* 'U' */, 241/* 'V' */, 242/* 'W'    */,  /*  84 -  87 */
 243/* 'X'  */, 244/* 'Y' */, 245/* 'Z' */,  45/* '['    */,  /*  88 -  91 */
  31/* '\\' */,  46/* ']' */,  15/* '^' */,  17/* '_'    */,  /*  92 -  95 */
  13/* '`'  */, 252/* 'a' */, 253/* 'b' */, 254/* 'c'    */,  /*  96 -  99 */
 255/* 'd'  */, 256/* 'e' */, 257/* 'f' */, 258/* 'g'    */,  /* 100 - 103 */
 259/* 'h'  */, 260/* 'i' */, 261/* 'j' */, 262/* 'k'    */,  /* 104 - 107 */
 263/* 'l'  */, 264/* 'm' */, 265/* 'n' */, 266/* 'o'    */,  /* 108 - 111 */
 267/* 'p'  */, 268/* 'q' */, 269/* 'r' */, 270/* 's'    */,  /* 112 - 115 */
 271/* 't'  */, 272/* 'u' */, 273/* 'v' */, 274/* 'w'    */,  /* 116 - 119 */
 275/* 'x'  */, 276/* 'y' */, 277/* 'z' */,  47/* '{'    */,  /* 120 - 123 */
  34/* '|'  */,  48/* '}' */,  32/* '~' */,  97/* '\x7F' */,  /* 124 - 127 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 128 - 131 */
   0/* ' '  */,  35/* '…' */,   0/* ' ' */,   0/* ' '    */,  /* 132 - 135 */
   0/* ' '  */, 176/* '‰' */,   0/* ' ' */,   0/* ' '    */,  /* 136 - 139 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 140 - 143 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 144 - 147 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 148 - 151 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 152 - 155 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 156 - 159 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 160 - 163 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,  87/* '§'    */,  /* 164 - 167 */
 570/* '¨'  */,   0/* ' ' */,   0/* ' ' */, 160/* '«'    */,  /* 168 - 171 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 172 - 175 */
   0/* ' '  */,  61/* '±' */,   0/* ' ' */,   0/* ' '    */,  /* 176 - 179 */
   0/* ' '  */,   0/* ' ' */, 182/* '¶' */,   0/* ' '    */,  /* 180 - 183 */
 618/* '¸'  */,   0/* ' ' */,   0/* ' ' */, 161/* '»'    */,  /* 184 - 187 */
   0/* ' '  */,   0/* ' ' */,   0/* ' ' */,   0/* ' '    */,  /* 188 - 191 */
 564/* 'À'  */, 565/* 'Á' */, 566/* 'Â' */, 567/* 'Ã'    */,  /* 192 - 195 */
 568/* 'Ä'  */, 569/* 'Å' */, 571/* 'Æ' */, 572/* 'Ç'    */,  /* 196 - 199 */
 573/* 'È'  */, 574/* 'É' */, 575/* 'Ê' */, 576/* 'Ë'    */,  /* 200 - 203 */
 577/* 'Ì'  */, 578/* 'Í' */, 579/* 'Î' */, 580/* 'Ï'    */,  /* 204 - 207 */
 581/* 'Ð'  */, 582/* 'Ñ' */, 583/* 'Ò' */, 584/* 'Ó'    */,  /* 207 - 211 */
 585/* 'Ô'  */, 586/* 'Õ' */, 587/* 'Ö' */, 588/* '×'    */,  /* 212 - 215 */
 589/* 'Ø'  */, 590/* 'Ù' */, 591/* 'Ú' */, 592/* 'Û'    */,  /* 216 - 219 */
 593/* 'Ü'  */, 594/* 'Ý' */, 595/* 'Þ' */, 596/* 'ß'    */,  /* 220 - 223 */
 612/* 'à'  */, 613/* 'á' */, 614/* 'â' */, 615/* 'ã'    */,  /* 224 - 227 */
 616/* 'ä'  */, 617/* 'å' */, 619/* 'æ' */, 620/* 'ç'    */,  /* 228 - 231 */
 621/* 'è'  */, 622/* 'é' */, 623/* 'ê' */, 624/* 'ë'    */,  /* 232 - 235 */
 625/* 'ì'  */, 626/* 'í' */, 627/* 'î' */, 628/* 'ï'    */,  /* 236 - 239 */
 629/* 'ð'  */, 630/* 'ñ' */, 631/* 'ò' */, 632/* 'ó'    */,  /* 240 - 243 */
 633/* 'ô'  */, 634/* 'õ' */, 635/* 'ö' */, 636/* '÷'    */,  /* 244 - 247 */
 637/* 'ø'  */, 638/* 'ù' */, 639/* 'ú' */, 640/* 'û'    */,  /* 248 - 251 */
 641/* 'ü'  */, 642/* 'ý' */, 643/* 'þ' */, 644/* 'ÿ'    */   /* 252 - 255 */
};

static unsigned int s_CharMetrics[ 224 ];

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

static GSContext   s_GSCtx;
static GSRectangle s_GSRect;
static GSFan       s_GSFan;
static GSLineStrip s_GSLineStrip;
static GSSprite    s_GSSprite;

static inline void _fontm_unpack ( Unpack* apUnpack, unsigned char* apDst ) {

 unsigned int   i, lVal, lCount;
 unsigned char* lpPtr;
 unsigned char* lpSrc;

 for ( i = 0, lpPtr = apDst;
       lpPtr < apDst + apUnpack -> m_Size;
       --i, apUnpack -> m_Data <<= 1
 ) {

  if ( i == 0 ) {

   i = 30;

   apUnpack -> m_Data  = *apUnpack -> m_Ptr++  << 8;
   apUnpack -> m_Data  = ( apUnpack -> m_Data | *apUnpack -> m_Ptr++ ) << 8;
   apUnpack -> m_Data  = ( apUnpack -> m_Data | *apUnpack -> m_Ptr++ ) << 8;
   apUnpack -> m_Data  =   apUnpack -> m_Data | *apUnpack -> m_Ptr++;
   apUnpack -> m_Shift = 0x3FFF >> ( apUnpack -> m_Data & 3 );
   apUnpack -> m_Diff  = 14 - ( apUnpack -> m_Data & 3 );
   apUnpack -> m_And   = apUnpack -> m_Data & 3;

  }  /* end if */

  if ( apUnpack -> m_Data & 0x80000000 ) {

   lVal   = *apUnpack -> m_Ptr++ << 8;
   lVal  |= *apUnpack -> m_Ptr++;
   lpSrc  = lpPtr - ( lVal & apUnpack -> m_Shift ) - 1;
   lCount = ( lVal >> apUnpack -> m_Diff ) + 2;

   do *lpPtr++ = *lpSrc++; while ( lCount-- );

  } else *lpPtr++ = *apUnpack -> m_Ptr++;

 }  /* end for */

}  /* end _fontm_unpack */

static inline unsigned char* _fontm_char ( FontmFile* apFile, long aChar, long anIdx ) {

 int            i, j;
 unsigned int   lPreGap;
 unsigned int   lPostGap;
 unsigned char  lVal;
 unsigned char* lpPtr;
 unsigned char* retVal = (
  ( unsigned char* )(
   (  ( unsigned int* )(
    (  ( unsigned char* )apFile  ) + sizeof ( FontmFile )
   )  )[ aChar ]
  )
 ) + apFile -> m_BaseOffset + ( unsigned int )apFile;

 lPreGap  = 0xFFFFFFFF;
 lPostGap = 0xFFFFFFFF;

 for ( i = 0; i < CHAR_SIZE; ++i ) retVal[ i ] = ( retVal[ i ] << 4 ) | ( retVal[ i ] >> 4 );

 lpPtr = retVal;

 for ( i = 0; i < 26; ++i, lpPtr += 13 ) {

  int lGap = 0;

  for ( j = 0; j < 13; ++j ) {

   lVal = lpPtr[ j ];

   if (  !( lVal & 0x0F )  )
    ++lGap;
   else break;

   if (  !( lVal & 0xF0 )  )
    ++lGap;
   else break;

  }  // end for

  if ( lGap < lPreGap ) lPreGap = lGap;

  lGap = 0;

  for ( j = 12; j >= 0; --j ) { 

   lVal = lpPtr[ j ];

   if (  !( lVal & 0xF0 )  )
    ++lGap;
   else break;

   if (  !( lVal & 0x0F )  )
    ++lGap;
   else break;

  }  /* end for */

  if ( lGap < lPostGap ) lPostGap = lGap;

 }  /* end for */

 lPostGap += 4;

 s_CharMetrics[ anIdx ] = ( lPreGap << 16 ) | ( 26 - lPostGap );

 return retVal;

}  /* end _fontm_char */

void _fontm_set_text_color ( u32 aColor ) {

 u32  lCLUT[ 16 ] __attribute__(  (  aligned( 16 )  )   );
 u64  lDMA [ 20 ] __attribute__(  (  aligned( 16 )  )   );
 u64* lpDMA = lDMA;
 s32  lRGB [  4 ];
 int  i;

 lRGB[ 0 ] = ( s32 )(  ( aColor >>  0 ) & 0x000000FF );
 lRGB[ 1 ] = ( s32 )(  ( aColor >>  8 ) & 0x000000FF );
 lRGB[ 2 ] = ( s32 )(  ( aColor >> 16 ) & 0x000000FF );
 lRGB[ 3 ] = ( s32 )(  ( aColor >> 24 ) & 0x000000FF );;

 for ( i = 15; i > 0; --i ) {

  lCLUT[ i ] = lRGB[ 0 ] | ( lRGB[ 1 ] << 8 ) | ( lRGB[ 2 ] << 16 ) | ( lRGB [ 3 ] << 24 );

  lRGB[ 0 ] -= 32; lRGB[ 0 ] = lRGB[ 0 ] & ~( lRGB[ 0 ] >> 7 );
  lRGB[ 1 ] -= 32; lRGB[ 1 ] = lRGB[ 1 ] & ~( lRGB[ 1 ] >> 7 );
  lRGB[ 2 ] -= 32; lRGB[ 2 ] = lRGB[ 2 ] & ~( lRGB[ 2 ] >> 7 );
  lRGB[ 3 ] -= 16; lRGB[ 3 ] = lRGB[ 3 ] & ~( lRGB[ 3 ] >> 7 );

 }  /* end for */

 lCLUT[ 0 ] = 0x00000000;

 *lpDMA++ = DMA_TAG( 6, 0, DMA_CNT, 0, 0, 0 );
 *lpDMA++ = 0;

  *lpDMA++ = GIF_TAG( 4, 1, 0, 0, 0, 1 );
  *lpDMA++ = GIF_AD;

   *lpDMA++ = GS_SETREG_BITBLTBUF( 0, 0, 0, s_GSCtx.m_Font.m_CLUT, 1, GSPSM_32 );
   *lpDMA++ = GS_BITBLTBUF;

   *lpDMA++ = GS_SETREG_TRXPOS( 0, 0, 0, 0, 0 );
   *lpDMA++ = GS_TRXPOS;

   *lpDMA++ = GS_SETREG_TRXREG( 8, 2 );
   *lpDMA++ = GS_TRXREG;

   *lpDMA++ = GS_SETREG_TRXDIR( 0 );
   *lpDMA++ = GS_TRXDIR;

  *lpDMA++ = GIF_TAG( 4, 1, 0, 0, 2, 1 );
  *lpDMA++ = 0;

 *lpDMA++ = DMA_TAG(  4, 1, DMA_REF, 0, ( u32 )lCLUT, 0  );
 *lpDMA++ = 0;

  *lpDMA++ = DMA_TAG( 0, 0, DMA_END, 0, 0, 0 );
  *lpDMA++ = 0;

 SyncDCache ( lCLUT, lCLUT + 16 );

 DMA_SendChain ( DMA_CHANNEL_GIF, lDMA, 9 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end _fontm_set_text_color */

static void _fontm_init ( void ) {

 Unpack         lUnpack;
 FILE*          lpFile;
 FontmFile*     lpFont;
 unsigned char* lpSrc;
 unsigned int   lX, lY, lSize;
 u64*           lpDMA;
 u64*           lpData;

 s_GSCtx.m_Font.m_BkColor = 0x80000000;
 s_GSCtx.m_Font.m_BkMode  = GSBkMode_Transparent;

 lpFile = fopen ( "rom0:FONTM", "rb" );

 fseek ( lpFile, 0L, SEEK_END );
 lSize = ftell ( lpFile );
 fseek ( lpFile, 0L, SEEK_SET );

 fread (  lpSrc = malloc ( lSize ), 1, lSize, lpFile  );

 lUnpack.m_Size = *( unsigned int* )lpSrc;
 lUnpack.m_Ptr  = lpSrc + 4;

 lpFont = malloc ( lUnpack.m_Size );

 _fontm_unpack (  &lUnpack, ( unsigned char* )lpFont  );

 s_GSCtx.m_Font.m_Text = s_GSCtx.m_VRAMPtr / 256;
 s_GSCtx.m_VRAMPtr    += 108544;
 s_GSCtx.m_Font.m_CLUT = s_GSCtx.m_VRAMPtr / 256;
 s_GSCtx.m_VRAMPtr += 256;

 lpDMA = ( u64* )lpSrc;
 lpSrc = ( unsigned char* )(   (  ( unsigned int )lpSrc + lUnpack.m_Size - 352  ) & 0xFFFFFFF0   );

 _fontm_set_text_color ( 0xCFFFFFFF );

 for ( lY = 0, lSize = 0; lY < 182; lY += 26 )

  for ( lX = 0; lX < 832; lX += 26, ++lSize ) {

   unsigned char* lpChr = _fontm_char ( lpFont, s_CharMap[ lSize ], lSize );
   memcpy ( lpSrc, lpChr, CHAR_SIZE );

   lpData = lpDMA;

   *lpData++ = DMA_TAG( 6, 0, DMA_CNT, 0, 0, 0 );
   *lpData++ = 0;

    *lpData++ = GIF_TAG( 4, 1, 0, 0, 0, 1 );
    *lpData++ = GIF_AD;

     *lpData++ = GS_SETREG_BITBLTBUF( 0, 0, 0, s_GSCtx.m_Font.m_Text, 14, GSPSM_4 );
     *lpData++ = GS_BITBLTBUF;

     *lpData++ = GS_SETREG_TRXPOS( 0, 0, lX, lY, 0 );
     *lpData++ = GS_TRXPOS;

     *lpData++ = GS_SETREG_TRXREG( 26, 26 );
     *lpData++ = GS_TRXREG;

     *lpData++ = GS_SETREG_TRXDIR( 0 );
     *lpData++ = GS_TRXDIR;

    *lpData++ = GIF_TAG( 22, 1, 0, 0, 2, 1 );
    *lpData++ = 0;

   *lpData++ = DMA_TAG(  22, 1, DMA_REF, 0, ( u32 )lpSrc, 0  );
   *lpData++ = 0;

   *lpData++ = DMA_TAG( 0, 0, DMA_END, 0, 0, 0 );
   *lpData   = 0;

   SyncDCache ( lpSrc, lpSrc + 352 );

   DMA_SendChain ( DMA_CHANNEL_GIF, lpDMA, 9 );
   DMA_Wait ( DMA_CHANNEL_GIF );

  }  /* end for */

 free   ( lpFont  );
 free   ( lpDMA   );
 fclose ( lpFile  );

 s_CharMetrics[ 0 ] = 6;

}  /* end _fontm_init */

static unsigned int _fontm_text_width ( char* apStr, int anChars ) {

 unsigned int i, retVal = 0, lnChars = anChars ? anChars : strlen ( apStr );

 for ( i = 0; i < lnChars; ++i ) {

  unsigned char lChr = apStr[ i ] - ' ';

  retVal += -( s_CharMetrics[ lChr ] >> 16 ) + ( int )(  ( s_CharMetrics[ lChr ] & 0x0000FFFF )  );

 }  // end for

 return retVal;

}  /* end _fontm_text_width */

void _fontm_draw_text ( int aX, int aY, int aZ, unsigned char* apStr ) {

 int  i, lnChars = strlen ( apStr );
 int  lIncr = s_GSCtx.m_Font.m_BkMode == GSBkMode_Opaque;
 u64  lDMA[ lnChars * 10 + 8 + ( lIncr << 3 ) ] __attribute__(  (  aligned( 16 )  )   );
 u64* lpDMA   = lDMA;
 int  lDMALen = 5 * lnChars + 4 + ( lIncr << 2 );
 int  lX1, lX2;
 int  lY1, lY2;
 int  lU1, lU2;
 int  lV1, lV2;
 int  lZ;
 int  lXIncr = s_GSCtx.m_OffsetX << 4;
 int  lYIncr = s_GSCtx.m_OffsetY << 4;
 int  lTX, lTY;

 *lpDMA++ = GIF_TAG( lDMALen - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 *lpDMA++ = GS_SETREG_TEX0(
             s_GSCtx.m_Font.m_Text, 14, GSPSM_4, 10, 8,
             1, 1, s_GSCtx.m_Font.m_CLUT, 0, 0, 0, 1
            );
 *lpDMA++ = GS_TEX0_1 + s_GSCtx.m_PrimCtx;

 *lpDMA++ = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 *lpDMA++ = GS_TEX1_1 + s_GSCtx.m_PrimCtx;

 *lpDMA++ = ALPHA_BLEND_NORMAL;
 *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

 lY1 = ( aY << 3 ) + lYIncr;
 lY2 = (  ( aY + 26 ) << 3  ) + lYIncr;
 lZ  = aZ << 4;

 if ( lIncr ) {

  lX1 = ( aX << 4 ) + lXIncr;
  lX2 = (   (  aX + _fontm_text_width ( apStr, lnChars )  ) << 4   ) + lXIncr;

  *lpDMA++ = GS_SETREG_PRIM(
              GS_PRIM_PRIM_SPRITE, 0, 0, s_GSCtx.m_fFog,
              s_GSCtx.m_fAlpha,
              s_GSCtx.m_fAntiAlias, 0, s_GSCtx.m_PrimCtx, 0
             );
  *lpDMA++ = GS_PRIM;

  *lpDMA++ = s_GSCtx.m_Font.m_BkColor;
  *lpDMA++ = GS_RGBAQ;

  *lpDMA++ = GS_SETREG_XYZ( lX1, lY1, lZ );
  *lpDMA++ = GS_XYZ2;

  *lpDMA++ = GS_SETREG_XYZ( lX2, lY2, lZ );
  *lpDMA++ = GS_XYZ2;

 }  // end if

 for ( i = 0; i < lnChars; ++i ) {

  unsigned char lChr     = apStr[ i ] - ' ';
  int           lPreGap  = s_CharMetrics[ lChr ] >> 16;
  int           lPostGap = s_CharMetrics[ lChr ] & 0x0000FFFF;

  aX  -= lPreGap;
  lX1  = ( aX << 4 ) + lXIncr;
  lX2  = (  ( aX + 20 ) << 4  ) + lXIncr;
  aX  += lPostGap;

  lTY = 0;

  while ( lChr > 31 ) {

   lChr -= 32;
   lTY  += 26;

  }  /* end while */

  lTX = lChr * 26;

  lU1 = ( lTX << 4 ) + lXIncr;
  lU2 = (  ( lTX + 26 ) << 4  ) + lXIncr;

  lV1 = ( lTY << 4 ) + lXIncr;
  lV2 = (  ( lTY + 26 ) << 4  ) + lXIncr;

  *lpDMA++ = GS_SETREG_PRIM(
              GS_PRIM_PRIM_SPRITE, 0, 1, 0,
              1, 1, 1, s_GSCtx.m_PrimCtx, 0
             );
  *lpDMA++ = GS_PRIM;

  *lpDMA++ = GS_SETREG_UV( lU1, lV1 );
  *lpDMA++ = GS_UV;

  *lpDMA++ = GS_SETREG_XYZ( lX1, lY1, lZ );
  *lpDMA++ = GS_XYZ2;

  *lpDMA++ = GS_SETREG_UV( lU2, lV2 );
  *lpDMA++ = GS_UV;

  *lpDMA++ = GS_SETREG_XYZ( lX2, lY2, lZ );
  *lpDMA++ = GS_XYZ2;

 }  /* end for */

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, lDMALen );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end _fontm_draw_text */

static void GS_InitScreen ( void ) {

 unsigned long int  lDMA[ 30 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned char      lMode;
 unsigned int       lFBH;

 if ( s_GSCtx.m_DisplayMode == GSDisplayMode_NTSC_I ) {

  lMode = GSDisplayMode_NTSC;
  lFBH  = s_GSCtx.m_Height;

 } else if ( s_GSCtx.m_DisplayMode == GSDisplayMode_PAL_I ) {

  lMode = GSDisplayMode_PAL;
  lFBH  = s_GSCtx.m_Height;

 } else if ( s_GSCtx.m_DisplayMode == GSDisplayMode_PAL ||
             s_GSCtx.m_DisplayMode == GSDisplayMode_NTSC
        ) {

  lMode = s_GSCtx.m_DisplayMode;
  lFBH  = s_GSCtx.m_Height;

 } else {

  lMode = s_GSCtx.m_DisplayMode;
  lFBH  = s_GSCtx.m_Height;

 }  /* end else */

 s_GSCtx.m_VRAMPtr = 0;

 GS_RESET();

 __asm__ __volatile__( "sync.p; nop; " );

 GsPutIMR ( 0x0000F700 );
 SetGsCrt ( s_GSCtx.m_fInterlace, lMode, s_GSCtx.m_FieldMode );

 if ( s_GSCtx.m_fZBuf == GS_OFF ) {

  s_GSCtx.m_Test.m_ZTE  = 1;
  s_GSCtx.m_Test.m_ZTST = 1;

 }  /* end if */

 GS_SET_PMODE( 0, 1, 0, 1, 0, 0x80 );
 GS_SET_DISPFB1( 0, s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0, 0 );
 GS_SET_DISPFB2( 0, s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0, 0 );
 GS_SET_DISPLAY1(
  s_GSCtx.m_StartX, s_GSCtx.m_StartY,
  s_GSCtx.m_MagX,   s_GSCtx.m_MagY,
  ( s_GSCtx.m_Width * 4 ) - 1,
  ( s_GSCtx.m_Height - 1 )
 );
 GS_SET_DISPLAY2(
  s_GSCtx.m_StartX, s_GSCtx.m_StartY,
  s_GSCtx.m_MagX,   s_GSCtx.m_MagY,
  ( s_GSCtx.m_Width * 4 ) - 1,
  ( s_GSCtx.m_Height - 1 )
 );
 GS_SET_BGCOLOR(
  s_GSCtx.m_BgClr.m_Red,
  s_GSCtx.m_BgClr.m_Green,
  s_GSCtx.m_BgClr.m_Blue
 );

 s_GSCtx.m_ScreenBufPtr[ 0 ] = s_GSCtx.FBAlloc (
  s_GSCtx.DataSize ( s_GSCtx.m_Width, lFBH, s_GSCtx.m_PSM )
 ); 

 s_GSCtx.m_ScreenBufPtr[ 1 ] = s_GSCtx.m_fDblBuf == GS_OFF ? s_GSCtx.m_ScreenBufPtr[ 0 ]
                                                           : s_GSCtx.FBAlloc (
                                                              s_GSCtx.DataSize ( s_GSCtx.m_Width, lFBH, s_GSCtx.m_PSM )
                                                             );
 if ( s_GSCtx.m_fZBuf == GS_ON )

  s_GSCtx.m_ZBufPtr = s_GSCtx.FBAlloc (
   s_GSCtx.DataSize (  s_GSCtx.m_Width, lFBH, ( GSPSM )s_GSCtx.m_ZSM  )
  );

 *lpDMA++ = GIF_TAG( 14, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 *lpDMA++ = 1;
 *lpDMA++ = GS_PRMODECONT;

 *lpDMA++ = GS_SETREG_FRAME_1( s_GSCtx.m_ScreenBufPtr[ 0 ], s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0 );
 *lpDMA++ = GS_FRAME_1;

 *lpDMA++ = GS_SETREG_XYOFFSET_1( s_GSCtx.m_OffsetX << 4, s_GSCtx.m_OffsetY << 4 );
 *lpDMA++ = GS_XYOFFSET_1;

 *lpDMA++ = GS_SETREG_SCISSOR_1( 0, s_GSCtx.m_Width - 1, 0, s_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_1;

 if ( s_GSCtx.m_fZBuf == GS_ON ) {

  *lpDMA++ = GS_SETREG_ZBUF_1( s_GSCtx.m_ZBufPtr / 8192, s_GSCtx.m_ZSM, 0 );
  *lpDMA++ = GS_ZBUF_1;

 } else {

  *lpDMA++ = GS_SETREG_ZBUF_1( 0, s_GSCtx.m_ZSM, 1 );
  *lpDMA++ = GS_ZBUF_1;

 }  /* end else */

 *lpDMA++ = GS_SETREG_TEST_1(
  s_GSCtx.m_Test.m_ATE,  s_GSCtx.m_Test.m_ATST, 
  s_GSCtx.m_Test.m_AREF, s_GSCtx.m_Test.m_AFAIL, 
  s_GSCtx.m_Test.m_DATE, s_GSCtx.m_Test.m_DATM,
  s_GSCtx.m_Test.m_ZTE,  s_GSCtx.m_Test.m_ZTST
 );
 *lpDMA++ = GS_TEST_1;

 *lpDMA++ = GS_SETREG_CLAMP_1(
  s_GSCtx.m_Clamp.m_WMS,  s_GSCtx.m_Clamp.m_WMT, 
  s_GSCtx.m_Clamp.m_MINU, s_GSCtx.m_Clamp.m_MAXU, 
  s_GSCtx.m_Clamp.m_MINV, s_GSCtx.m_Clamp.m_MAXV
 );
 *lpDMA++ = GS_CLAMP_1;

 *lpDMA++ = GS_SETREG_COLCLAMP( 255 );
 *lpDMA++ = GS_COLCLAMP;

 *lpDMA++ = GS_SETREG_FRAME_2( s_GSCtx.m_ScreenBufPtr[ 1 ], s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0 );
 *lpDMA++ = GS_FRAME_2;

 *lpDMA++ = GS_SETREG_XYOFFSET_2( s_GSCtx.m_OffsetX << 4, s_GSCtx.m_OffsetY << 4 );
 *lpDMA++ = GS_XYOFFSET_2;

 *lpDMA++ = GS_SETREG_SCISSOR_2( 0, s_GSCtx.m_Width - 1, 0, s_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_2;

 if ( s_GSCtx.m_fZBuf == GS_ON ) {

  *lpDMA++ = GS_SETREG_ZBUF_2( s_GSCtx.m_ZBufPtr / 8192, s_GSCtx.m_ZSM, 0 );
  *lpDMA++ = GS_ZBUF_2;

 } else {

  *lpDMA++ = GS_SETREG_ZBUF_2( NULL, s_GSCtx.m_ZSM, 1 );
  *lpDMA++ = GS_ZBUF_2;

 }  /* end else */

 *lpDMA++ = GS_SETREG_TEST_2(
  s_GSCtx.m_Test.m_ATE,  s_GSCtx.m_Test.m_ATST, 
  s_GSCtx.m_Test.m_AREF, s_GSCtx.m_Test.m_AFAIL, 
  s_GSCtx.m_Test.m_DATE, s_GSCtx.m_Test.m_DATM,
  s_GSCtx.m_Test.m_ZTE,  s_GSCtx.m_Test.m_ZTST
 );
 *lpDMA++ = GS_TEST_2;

 *lpDMA++ = GS_SETREG_CLAMP_2(
  s_GSCtx.m_Clamp.m_WMS,  s_GSCtx.m_Clamp.m_WMT, 
  s_GSCtx.m_Clamp.m_MINU, s_GSCtx.m_Clamp.m_MAXU, 
  s_GSCtx.m_Clamp.m_MINV, s_GSCtx.m_Clamp.m_MAXV
 );
 *lpDMA   = GS_CLAMP_2;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 15 );
 DMA_Wait ( DMA_CHANNEL_GIF );

 s_GSCtx.m_IconPtr   = s_GSCtx.m_VRAMPtr / 256;
 s_GSCtx.m_VRAMPtr  += 16384 - 2048 - 1024;
 s_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x00 );
 s_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );

 _fontm_init ();

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

 unsigned int retVal = s_GSCtx.m_VRAMPtr;

 if ( aSize % 8192 ) aSize += 8192 - ( aSize % 8192 );

 s_GSCtx.m_VRAMPtr += aSize;

 return retVal;

}  /* end GS_FBAlloc */

static unsigned int GS_TBAlloc ( unsigned int aSize ) {

 unsigned int retVal = s_GSCtx.m_VRAMPtr;

 if ( aSize % 256 ) aSize = ( aSize + 256 ) - ( aSize % 256 );

 s_GSCtx.m_VRAMPtr += aSize;

 return retVal;

}  /* end GS_TBAlloc */

static void GS_Scale ( GSVertex* apPoints, int aCount ) {

 int i;

 for ( i = 0; i < aCount; ++i ) {

  apPoints[ i ].m_X <<= 4;
  apPoints[ i ].m_X  += s_GSCtx.m_OffsetX << 4;

  apPoints[ i ].m_Y <<= 3;
  apPoints[ i ].m_Y  += s_GSCtx.m_OffsetY << 4;

  apPoints[ i ].m_Z <<= 4;

 }  /* end for */

}  /* end GS_Scale */

static void GS_ScaleUV ( GSTexVertex* apPoints, int aCount ) {

 int i;

 for ( i = 0; i < aCount; ++i ) {

  apPoints[ i ].m_U <<= 4;
  apPoints[ i ].m_U  += s_GSCtx.m_OffsetX << 4;

  apPoints[ i ].m_V <<= 4;
  apPoints[ i ].m_V  += s_GSCtx.m_OffsetX << 4;

 }  /* end for */

}  /* end GS_ScaleUV */

static void GS_DrawRect ( void ) {

 unsigned long int  lDMA[ 22 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned int       lSize = 10;
 int                i;

 s_GSCtx.Scale ( s_GSRect.m_Points, 4 );

 if ( s_GSCtx.m_fAlpha ) ++lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( s_GSCtx.m_fAlpha ) {

  *lpDMA++ = s_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

 }  /* end if */
        
 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_TRISTRIP, 1, 0, s_GSCtx.m_fFog,
  s_GSCtx.m_fAlpha, s_GSCtx.m_fAntiAlias, 0,
  s_GSCtx.m_PrimCtx, 0
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

 s_GSCtx.Scale ( s_GSFan.m_pPoints, s_GSFan.m_nPoints );

 if ( !s_GSCtx.m_fAlpha ) --lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( s_GSCtx.m_fAntiAlias == 1 ) {

  *lpDMA++ = s_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

 }  /* end if */

 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_TRIFAN, 0, 0, s_GSCtx.m_fFog,
  s_GSCtx.m_fAlpha, s_GSCtx.m_fAntiAlias, 0,
  s_GSCtx.m_PrimCtx, 0
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

 s_GSCtx.Scale ( s_GSLineStrip.m_pPoints, s_GSLineStrip.m_nPoints );

 if ( !s_GSCtx.m_fAlpha ) --lSize;
        
 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;
        
 if ( s_GSCtx.m_fAlpha ) {

  *lpDMA++ = s_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

 }  /* end if */
        
 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_LINESTRIP, 0, 0, s_GSCtx.m_fFog,
  s_GSCtx.m_fAlpha, s_GSCtx.m_fAntiAlias, 0,
  s_GSCtx.m_PrimCtx, 0
 );
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
 GSRectangle* lpRect  = s_GSCtx.InitRectangle ();
 GSFan*       lpFan   = s_GSCtx.InitFan ( 11 );
 GSLineStrip* lpStrip = s_GSCtx.InitLineStrip ( 41 );
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
 u64*        lpDMA = lDMA;
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

 s_GSCtx.Scale   ( lPoints,    2 );
 s_GSCtx.ScaleUV ( lTexPoints, 2 );

 *lpDMA++ = DMA_TAG( 6, 0, DMA_CNT, 0, 0, 0 );
 *lpDMA++ = 0;

  *lpDMA++ = GIF_TAG( 4, 1, 0, 0, 0, 1 );
  *lpDMA++ = GIF_AD;

   *lpDMA++ = GS_SETREG_BITBLTBUF( 0, 0, 0, s_GSCtx.m_IconPtr, 1, GSPSM_32 );
   *lpDMA++ = GS_BITBLTBUF;

   *lpDMA++ = GS_SETREG_TRXPOS( 0, 0, 0, 0, 0 );
   *lpDMA++ = GS_TRXPOS;

   *lpDMA++ = GS_SETREG_TRXREG( aSize, aSize );
   *lpDMA++ = GS_TRXREG;

   *lpDMA++ = GS_SETREG_TRXDIR( 0 );
   *lpDMA++ = GS_TRXDIR;

   *lpDMA++ = GIF_TAG( lQWC, 1, 0, 0, 2, 1 );
   *lpDMA++ = 0;

  *lpDMA++ = DMA_TAG(  lQWC, 1, DMA_REF, 0, ( u32 )apData, 0  );
  *lpDMA++ = 0;

  *lpDMA++ = DMA_TAG( 10, 0, DMA_END, 0, 0, 0 );
  *lpDMA++ = 0;

   *lpDMA++ = GIF_TAG( 9, 1, 0, 0, 0, 1 );
   *lpDMA++ = GIF_AD;

   *lpDMA++ = 0;
   *lpDMA++ = GS_TEXFLUSH;

   *lpDMA++ = GS_SETREG_TEX0(
               s_GSCtx.m_IconPtr, 1, GSPSM_32, 6, 6,
               1, 1, 0, 0, 0, 0, 1
              );
   *lpDMA++ = GS_TEX0_1 + s_GSCtx.m_PrimCtx;

   *lpDMA++ = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
   *lpDMA++ = GS_TEX1_1 + s_GSCtx.m_PrimCtx;

   *lpDMA++ = ALPHA_BLEND_NORMAL;
   *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

   *lpDMA++ = GS_SETREG_PRIM(
               GS_PRIM_PRIM_SPRITE, 0, 1, 0,
               1, 1, 1, s_GSCtx.m_PrimCtx, 0
              );
   *lpDMA++ = GS_PRIM;

   *lpDMA++ = GS_SETREG_UV( lTexPoints[ 0 ].m_U, lTexPoints[ 0 ].m_V );
   *lpDMA++ = GS_UV;

   *lpDMA++ = GS_SETREG_XYZ( lPoints[ 0 ].m_X, lPoints[ 0 ].m_Y, lPoints[ 0 ].m_Z );
   *lpDMA++ = GS_XYZ2;

   *lpDMA++ = GS_SETREG_UV( lTexPoints[ 1 ].m_U, lTexPoints[ 1 ].m_V );
   *lpDMA++ = GS_UV;

   *lpDMA++ = GS_SETREG_XYZ( lPoints[ 1 ].m_X, lPoints[ 1 ].m_Y, lPoints[ 1 ].m_Z );
   *lpDMA++ = GS_XYZ2;

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
 s_GSRect.m_Color[ 3 ] = s_GSCtx.m_FillColor;

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
 s_GSFan.m_Color   = s_GSCtx.m_FillColor;

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
 s_GSLineStrip.m_Color   = s_GSCtx.m_LineColor;

 memset ( s_GSLineStrip.m_pPoints, 0, i );

 return &s_GSLineStrip;

}  /* end GS_InitLineStrip */

static void GS_DrawSprite ( void ) {

 unsigned long int  lDMA[ 12 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;
 unsigned int       lSize = 5;

 s_GSCtx.Scale ( s_GSSprite.m_Points, 2 );

 if ( s_GSCtx.m_fAlpha ) ++lSize;

 *lpDMA++ = GIF_TAG( lSize - 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 if ( s_GSCtx.m_fAlpha == 1 ) {

  *lpDMA++ = s_GSCtx.m_PrimAlpha;
  *lpDMA++ = GS_ALPHA_1 + s_GSCtx.m_PrimCtx;

 }  /* end if */

 *lpDMA++ = GS_SETREG_PRIM(
  GS_PRIM_PRIM_SPRITE, 0, 0, s_GSCtx.m_fFog,
  s_GSCtx.m_fAlpha, s_GSCtx.m_fAntiAlias,
  0, s_GSCtx.m_PrimCtx, 0
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

 unsigned long int  lDMA[ 4 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;

 s_GSCtx.m_Test.m_ZTST = s_GSCtx.m_fZBuf  ? 2 : 1;
 s_GSCtx.m_Test.m_ATE  = s_GSCtx.m_fAlpha ? 1 : 0;

 *lpDMA++ = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 *lpDMA++ = GS_SETREG_TEST(
  s_GSCtx.m_Test.m_ATE,  s_GSCtx.m_Test.m_ATST,
  s_GSCtx.m_Test.m_AREF, s_GSCtx.m_Test.m_AFAIL, 
  s_GSCtx.m_Test.m_DATE, s_GSCtx.m_Test.m_DATM, 
  s_GSCtx.m_Test.m_ZTE,  s_GSCtx.m_Test.m_ZTST
 );
 *lpDMA++ = GS_TEST_1 + s_GSCtx.m_PrimCtx;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 2 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_SetTest */

static void GS_ClearScreen ( unsigned long int aColor ) {

 GSOnOff   lZTest   = s_GSCtx.m_fZBuf;
 GSSprite* lpSprite = s_GSCtx.InitSprite ( aColor );

 if ( lZTest ) {

  s_GSCtx.m_fZBuf = GS_OFF;
  s_GSCtx.SetTest ();

 }  /* end if */

 lpSprite -> m_Points[ 0 ].m_X = 0;
 lpSprite -> m_Points[ 0 ].m_Y = 0;
 lpSprite -> m_Points[ 1 ].m_X = s_GSCtx.m_Width;
 lpSprite -> m_Points[ 1 ].m_Y = s_GSCtx.m_Height;
 lpSprite -> Draw ();

 if ( lZTest ) {

  s_GSCtx.m_fZBuf = lZTest;
  s_GSCtx.SetTest ();

 }  /* end if */

}  /* end GS_ClearScreen */

static void GS_SwapBuffers ( int afSync ) {

 unsigned long int  lDMA[ 10 ] __attribute__(   (  aligned( 16 )  )   );
 unsigned long int* lpDMA = lDMA;

 if ( afSync ) s_GSCtx.VSync ();

 GS_SET_DISPFB2(
  s_GSCtx.m_ScreenBufPtr[ s_GSCtx.m_ActiveBuf & 1 ] / 8192,
  s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0, 0
 );
 s_GSCtx.m_ActiveBuf ^= 1;
 s_GSCtx.m_PrimCtx   ^= 1;

 *lpDMA++ = GIF_TAG( 4, 1, 0, 0, 0, 1 );
 *lpDMA++ = GIF_AD;

 *lpDMA++ = GS_SETREG_SCISSOR_1( 0, s_GSCtx.m_Width - 1, 0, s_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_1;

 *lpDMA++ = GS_SETREG_FRAME_1(
  s_GSCtx.m_ScreenBufPtr[ s_GSCtx.m_ActiveBuf & 1 ] / 8192,
  s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0
 );
 *lpDMA++ = GS_FRAME_1;

 *lpDMA++ = GS_SETREG_SCISSOR_1( 0, s_GSCtx.m_Width - 1, 0, s_GSCtx.m_Height - 1 );
 *lpDMA++ = GS_SCISSOR_2;

 *lpDMA++ = GS_SETREG_FRAME_1(
  s_GSCtx.m_ScreenBufPtr[ s_GSCtx.m_ActiveBuf & 1 ] / 8192,
  s_GSCtx.m_Width / 64, s_GSCtx.m_PSM, 0
 );
 *lpDMA++ = GS_FRAME_2;

 DMA_Send ( DMA_CHANNEL_GIF, lDMA, 5 );
 DMA_Wait ( DMA_CHANNEL_GIF );

}  /* end GS_SwapBuffers */

static void GS_DestroyContext ( void ) {

}  /* end GS_DestroyContext */

GSContext* GS_InitContext ( GSDisplayMode aMode ) {

 s_GSCtx.m_DisplayMode = aMode;

 if ( aMode == GSDisplayMode_NTSC ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 640;	
  s_GSCtx.m_Height     = 480;
  s_GSCtx.m_StartX     = 632;
  s_GSCtx.m_StartY     =  20;
  s_GSCtx.m_MagX       =   3;
  s_GSCtx.m_MagY       =   0;

 } else if ( aMode == GSDisplayMode_NTSC_I ) {

  s_GSCtx.m_fInterlace = GS_ON;
  s_GSCtx.m_FieldMode  = GSFieldMode_Field;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 640;
  s_GSCtx.m_Height     = 480;
  s_GSCtx.m_StartX     = 652;
  s_GSCtx.m_StartY     =  30;
  s_GSCtx.m_MagX       =   3;
  s_GSCtx.m_MagY       =   1;

 } else if ( aMode == GSDisplayMode_PAL ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 640;
  s_GSCtx.m_Height     = 576;
  s_GSCtx.m_StartX     = 652;
  s_GSCtx.m_StartY     =  42;
  s_GSCtx.m_MagX       =   3;
  s_GSCtx.m_MagY       =   0;

 } else if ( aMode == GSDisplayMode_PAL_I ) {
setPal_I:
  s_GSCtx.m_fInterlace = GS_ON;
  s_GSCtx.m_FieldMode  = GSFieldMode_Field;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 640;		
  s_GSCtx.m_Height     = 576;
  s_GSCtx.m_StartX     = 652;
  s_GSCtx.m_StartY     =  40;
  s_GSCtx.m_MagX       =   3;
  s_GSCtx.m_MagY       =   1;

 } else if ( aMode == GSDisplayMode_VGA_640x480_60Hz ||
             aMode == GSDisplayMode_VGA_640x480_72Hz ||
             aMode == GSDisplayMode_VGA_640x480_75Hz ||
             aMode == GSDisplayMode_VGA_640x480_85Hz
        ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 640;
  s_GSCtx.m_Height     = 480;

  if ( aMode == GSDisplayMode_VGA_640x480_60Hz )

   s_GSCtx.m_StartX = 280;

  else if ( aMode == GSDisplayMode_VGA_640x480_72Hz )

   s_GSCtx.m_StartX = 330;

  else if ( aMode == GSDisplayMode_VGA_640x480_75Hz )

   s_GSCtx.m_StartX = 360;

  else s_GSCtx.m_StartX = 260;

  s_GSCtx.m_StartY = 18;
  s_GSCtx.m_MagX   =  1;
  s_GSCtx.m_MagY   =  1;

 } else if ( aMode == GSDisplayMode_VGA_800x600_56Hz ||
             aMode == GSDisplayMode_VGA_800x600_60Hz || 
             aMode == GSDisplayMode_VGA_800x600_72Hz ||
             aMode == GSDisplayMode_VGA_800x600_75Hz || 
             aMode == GSDisplayMode_VGA_800x600_85Hz
        ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 800;
  s_GSCtx.m_Height     = 600;

  if ( aMode == GSDisplayMode_VGA_800x600_56Hz )

   s_GSCtx.m_StartX = 450;

  else if ( aMode == GSDisplayMode_VGA_800x600_60Hz )

   s_GSCtx.m_StartX = 465;

  else if ( aMode == GSDisplayMode_VGA_800x600_72Hz )

   s_GSCtx.m_StartX = 465;

  else if ( aMode == GSDisplayMode_VGA_800x600_75Hz )

   s_GSCtx.m_StartX = 510;

  else s_GSCtx.m_StartX = 500;

  s_GSCtx.m_StartY = 25;
  s_GSCtx.m_MagX   =  1;
  s_GSCtx.m_MagY   =  1;

 } else if ( aMode == GSDisplayMode_VGA_1024x768_60Hz ||
             aMode == GSDisplayMode_VGA_1024x768_70Hz ||
             aMode == GSDisplayMode_VGA_1024x768_75Hz ||
             aMode == GSDisplayMode_VGA_1024x768_85Hz
        ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_OFF;
  s_GSCtx.m_fZBuf      = GS_OFF;
  s_GSCtx.m_Width      = 1024;
  s_GSCtx.m_Height     =  768;
  s_GSCtx.m_StartY     =   30;
  s_GSCtx.m_MagY       =    1;

  if ( aMode == GSDisplayMode_VGA_1024x768_60Hz ) {

   s_GSCtx.m_MagX   =   1;
   s_GSCtx.m_StartX = 580;

  } else if ( aMode == GSDisplayMode_VGA_1024x768_70Hz ) {

   s_GSCtx.m_MagX   =   0;
   s_GSCtx.m_StartX = 266;

  } else if ( aMode == GSDisplayMode_VGA_1024x768_75Hz ) {

   s_GSCtx.m_MagX   =   0;
   s_GSCtx.m_StartX = 260;

  } else {

   s_GSCtx.m_MagX   =   0;
   s_GSCtx.m_StartX = 290;

  }  /* end else */

 } else if ( aMode == GSDisplayMode_VGA_1280x1024_60Hz ||
             aMode == GSDisplayMode_VGA_1280x1024_75Hz
        ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_OFF;
  s_GSCtx.m_fZBuf      = GS_OFF;
  s_GSCtx.m_Width      = 1280;
  s_GSCtx.m_Height     = 1024;
  s_GSCtx.m_StartX     =  350;
  s_GSCtx.m_StartY     =   40;
  s_GSCtx.m_MagX       =    0;
  s_GSCtx.m_MagY       =    0;

 } else if ( aMode == GSDisplayMode_DTV_720x480P ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 720;
  s_GSCtx.m_Height     = 480;
  s_GSCtx.m_StartX     = 232;
  s_GSCtx.m_StartY     =  35;
  s_GSCtx.m_MagX       =   1;
  s_GSCtx.m_MagX       =   0;

 } else if ( aMode == GSDisplayMode_DTV_1280x720P ) {

  s_GSCtx.m_fInterlace = GS_OFF;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_OFF;
  s_GSCtx.m_fZBuf      = GS_OFF;
  s_GSCtx.m_Width      = 1280;
  s_GSCtx.m_Height     =  720;
  s_GSCtx.m_StartX     =  302;
  s_GSCtx.m_StartY     =   24;
  s_GSCtx.m_MagX       =    0;
  s_GSCtx.m_MagY       =    0;

 } else if( aMode == GSDisplayMode_DTV_1920x1080I ) {

  s_GSCtx.m_fInterlace = GS_ON;
  s_GSCtx.m_FieldMode  = GSFieldMode_Frame;
  s_GSCtx.m_fDblBuf    = GS_ON;
  s_GSCtx.m_fZBuf      = GS_ON;
  s_GSCtx.m_Width      = 1920;
  s_GSCtx.m_Height     = 1080;
  s_GSCtx.m_StartX     =  238;
  s_GSCtx.m_StartY     =   40;
  s_GSCtx.m_MagX       =    0;
  s_GSCtx.m_MagY       =    0;

 } else goto setPal_I;

 s_GSCtx.m_OffsetX    = 2048;
 s_GSCtx.m_OffsetY    = 2048;
 s_GSCtx.m_PSM        = GSPSM_24;
 s_GSCtx.m_ZSM        = GSZSM_32;
 s_GSCtx.m_ActiveBuf  = 1;
 s_GSCtx.m_fFog       = 0;
 s_GSCtx.m_fAntiAlias = 0;
 s_GSCtx.m_fAlpha     = 1;
 s_GSCtx.m_PrimAlpha  = GSAlphaBlend_Back2Front;
 s_GSCtx.m_PrimCtx    = 0;

 s_GSCtx.m_BgClr.m_Red   = 0x00;
 s_GSCtx.m_BgClr.m_Green = 0x00;
 s_GSCtx.m_BgClr.m_Blue  = 0x00;

 s_GSCtx.m_Test.m_ATE   = 0;
 s_GSCtx.m_Test.m_ATST  = 1;
 s_GSCtx.m_Test.m_AREF  = 0x80;
 s_GSCtx.m_Test.m_AFAIL = 0;
 s_GSCtx.m_Test.m_DATE  = 0;
 s_GSCtx.m_Test.m_DATM  = 0;
 s_GSCtx.m_Test.m_ZTE   = 1;
 s_GSCtx.m_Test.m_ZTST  = 2;

 s_GSCtx.m_Clamp.m_WMS  = GSClampMode_Clamp;
 s_GSCtx.m_Clamp.m_WMT  = GSClampMode_Clamp;
 s_GSCtx.m_Clamp.m_MINU = 0;
 s_GSCtx.m_Clamp.m_MAXU = 0;
 s_GSCtx.m_Clamp.m_MINV = 0;
 s_GSCtx.m_Clamp.m_MAXV = 0;

 s_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );
 s_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x00 );

 s_GSCtx.InitScreen    = GS_InitScreen;
 s_GSCtx.DataSize      = GS_DataSize;
 s_GSCtx.FBAlloc       = GS_FBAlloc;
 s_GSCtx.TBAlloc       = GS_TBAlloc;
 s_GSCtx.Scale         = GS_Scale;
 s_GSCtx.ScaleUV       = GS_ScaleUV;
 s_GSCtx.DrawIcon      = GS_DrawIcon;
 s_GSCtx.RoundRect     = GS_RoundRect;
 s_GSCtx.InitRectangle = GS_InitRectangle;
 s_GSCtx.InitFan       = GS_InitFan;
 s_GSCtx.InitLineStrip = GS_InitLineStrip;
 s_GSCtx.InitSprite    = GS_InitSprite;
 s_GSCtx.VSync         = GS_VSync;
 s_GSCtx.SetTest       = GS_SetTest;
 s_GSCtx.ClearScreen   = GS_ClearScreen;
 s_GSCtx.SwapBuffers   = GS_SwapBuffers;
 s_GSCtx.TextWidth     = _fontm_text_width;
 s_GSCtx.DrawText      = _fontm_draw_text;
 s_GSCtx.SetTextColor  = _fontm_set_text_color;
 s_GSCtx.Destroy       = GS_DestroyContext;

 return &s_GSCtx;

}  /* end GS_InitContext */
#endif  /* _WIN32 */
