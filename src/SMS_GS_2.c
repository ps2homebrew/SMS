/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2004 Lord_Kiron (font unpack routines - http://www.mpcclub.com/)
# (c) 2005 BraveDog
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_MC.h"

#include <kernel.h>
#include <string.h>
#include <limits.h>
#include <fileio.h>
#include <malloc.h>

#include <lzma2.h>

typedef struct _Unaligned32 {

 unsigned int m_Val __attribute__(  ( packed )  );

} _Unaligned32;

char          g_GSCharWidth[ 224 ];
unsigned int* g_MBFont;

extern unsigned char g_ASCII[ 4572 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1250 [ 7736 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1251 [ 7856 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1252 [ 7732 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1253 [ 7588 ] __attribute__(   (  section( ".data" )  )   );

extern int g_XShift;

GSMTKFontHeader* g_pASCII     = ( GSMTKFontHeader* )g_ASCII;
GSMTKFontHeader* g_Fonts[ 4 ] = {
 ( GSMTKFontHeader* )g_1250,
 ( GSMTKFontHeader* )g_1251,
 ( GSMTKFontHeader* )g_1252,
 ( GSMTKFontHeader* )g_1253
};
static unsigned int s_FontSizes[ 5 ] = {
 sizeof ( g_ASCII ), sizeof ( g_1250 ), sizeof ( g_1251 ),
 sizeof ( g_1252  ), sizeof ( g_1253 )
};

static inline unsigned int _unaligned32 ( const void* apData ) {
 return (  ( const _Unaligned32* )apData  ) -> m_Val;
}  /* end _unaligned32 */

static inline unsigned int _swap_nibbles ( unsigned int aVal ) {
 return (  ( aVal & 0xF0F0F0F0 ) >> 4  ) |
        (  ( aVal & 0x0F0F0F0F ) << 4  );
}  /* end _swap_nibbles */

static inline unsigned int _swap_bytes ( unsigned int aVal ) {
 return (  ( aVal & 0x00FF ) << 8 ) |
        (  ( aVal & 0xFF00 ) >> 8 );
}  /* end _swap_bytes */

static unsigned int _next_block ( unsigned char* apBuf, int aBufPos ) {

 unsigned int   lData;
 unsigned int   lLen;
 unsigned int   lClr;
 unsigned char* lpData;
 unsigned char  lTest;

 lpData = apBuf + ( aBufPos >> 3 );
 lData  = _unaligned32 ( lpData );

 if ( aBufPos & 7 ) lData = _swap_nibbles (  _swap_nibbles ( lData ) >> 4  );

 lTest = lData & 0xFF;
 lData = _swap_bytes ( lData );

 if (  !( lTest & 0xFC )  ) {

  lClr     = lData & 0x03;
  lLen     = ( lData & 0x03FC ) >> 2;
  aBufPos += 16;

 } else  if (  !( lTest & 0xF0 )  ) {

  lClr     = ( lData & 0x30  ) >> 4;
  lLen     = ( lData & 0xFC0 ) >> 6;
  aBufPos += 12;

 } else if (  !( lTest & 0xC0 )  ) {

  lClr     = lTest & 0x03;
  lLen     = ( lTest & 0x3C ) >> 2;
  aBufPos += 8;

 } else {

  lTest  >>= 4;
  lClr     = lTest & 0x03;
  lLen     = ( lTest & 0x0C ) >> 2;
  aBufPos += 4;

 }  /* end else */

 __asm__ __volatile__(
  ".set noat\n\t"
  "or       $v1, $zero, %0\n\t"
  "addiu    $at, %1, -1\n\t"
  ".set at\n\t"
  :: "r"( lLen ), "r"( lClr )
 );

 return aBufPos;    

}  /* end _next_block */

int GSFont_UnpackChr ( GSMTKFontHeader* apHdr, unsigned int aChr, void* apBuf ) {

 unsigned char* lpFont   = ( unsigned char* )apHdr;
 unsigned char* lpBuff   = ( unsigned char* )apBuf;
 unsigned int   lWidth   = ( lpFont + 11 )[ aChr ];
 unsigned int   lBegin;
 unsigned int   lEnd;
 unsigned int   lBufPos;
 unsigned int   lFlag;
 unsigned int   lDIdx;
 unsigned int   lSIdx;
 unsigned char  lPixel;
 unsigned int*  lpOffs;
 unsigned char* lpOffset;
 unsigned int   lColor;
 unsigned int   lLength;
 unsigned int   i;

 memset ( apBuf, 0, 512 );

 if ( lWidth ) {

  lBegin   = 0;
  lBufPos  = 0;
  lFlag    = 0;
  lDIdx    = 0;
  lSIdx    = 0;
  lPixel   = 0;
  lEnd     = lWidth * apHdr -> m_GlyphHeight;
  lpOffs   = ( unsigned int*  )( lpFont + 11 + apHdr -> m_nGlyphs + 1 );
  lpOffset = ( unsigned char* )lpOffs + ( apHdr -> m_nGlyphs << 2 ) + SMS_unaligned32 ( &lpOffs[ aChr ] ) + 4;

  while ( lBegin < lEnd ) {

   lBufPos = _next_block ( lpOffset, lBufPos );

   __asm__ __volatile__(
    ".set noat\n\t"
    "or %0, $zero, $v1\n\t"
    "or %1, $zero, $at\n\t"    
    ".set at\n\t"
    : "=r"( lLength ), "=r"( lColor )
   );

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

 }  /* end if */

 return lWidth;

}  /* end GSFont_UnpackChr */

void GSFont_Init ( void ) {

 unsigned int     lX, lY, lIdx, lCharIdx = 0, lStrideIdx = 0;
 GSLoadImage      lLoadImage;
 unsigned char    lCharBuf[ 512 ] __attribute__(   (  aligned( 16 )  )   );
 GSMTKFontHeader* lpFont = g_Fonts[ g_GSCtx.m_CodePage ];
 GSLoadImage*     lpLoadImage = UNCACHED_SEG( &lLoadImage );

 if ( g_GSCtx.m_VRAMFontPtr ) {
  g_GSCtx.m_VRAMPtr   += 496;
  g_GSCtx.m_FontTexFmt = GSPixelFormat_PSMT4;
 } else g_GSCtx.m_FontTexFmt = GSPixelFormat_PSMT4HL;

 GS_InitLoadImage (
  &lLoadImage, g_GSCtx.m_VRAMFontPtr, 8, g_GSCtx.m_FontTexFmt, 0, 0, 32, 32
 );

 lY = 0;
 lX = 0;

 SyncDCache ( &lLoadImage, &lLoadImage + 1 );

 for ( lIdx = 0; lIdx < 96; ++lIdx, ++lCharIdx ) {

  g_GSCharWidth[ lCharIdx ] = GSFont_UnpackChr ( g_pASCII, lIdx, lCharBuf );

  lpLoadImage -> m_TrxPosReg.m_Value = GS_SET_TRXPOS( 0, 0, lX, lY, 0 );

  SyncDCache ( lCharBuf, lCharBuf + 512 );
  GS_LoadImage ( &lLoadImage, lCharBuf );
  DMA_Wait ( DMAC_GIF );

  if ( ++lStrideIdx == 16 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 for ( lIdx = 0; lIdx < 128; ++lIdx, ++lCharIdx ) {

  g_GSCharWidth[ lCharIdx ] = GSFont_UnpackChr ( lpFont, lIdx, lCharBuf );

  lpLoadImage -> m_TrxPosReg.m_Value = GS_SET_TRXPOS( 0, 0, lX, lY, 0 );

  SyncDCache ( lCharBuf, lCharBuf + 512 );
  GS_LoadImage ( &lLoadImage, lCharBuf );
  DMA_Wait ( DMAC_GIF );

  if ( ++lStrideIdx == 16 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 g_GSCharWidth[ ' ' - ' ' ] = g_GSCharWidth[ ':' - ' ' ];

}  /* end GSFont_Init */

int GSFont_Width ( unsigned char* apStr, int aLen ) {

 int lW   = 0;
 int lChr = apStr[ 0 ] - ' ';

 while ( aLen-- ) {

  lChr  = *apStr++ - ' ';
  lW   += g_GSCharWidth[ lChr ];

 }  /* end for */

 return lW;

}  /* end GSFont_Width */

int GSFont_WidthEx ( unsigned char* apStr, int aLen, int aDW ) {

 int   lChr;
 int   retVal = 0;;
 float lAR    = ( 32 + aDW ) / 32.0F;

 while ( aLen-- ) {

  lChr    = *apStr++ - ' ';
  retVal += ( int )(  ( float )g_GSCharWidth[ lChr ] * lAR + 0.5F  );

 }  /* end while */

 return retVal;

}  /* end GSFont_WidthEx */

void GSFont_Render ( unsigned char* apStr, int aLen, int aX, int anY, u64*           apDMA ) {

 int lDelta;
 int lY1;
 int lY2;
 int lf16   = g_GSCtx.m_FontTexFmt != GSPixelFormat_PSMT4HL;
 int lShift = g_XShift;
 int lCurX  = aX;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "pcpyld   $ra, $ra, $ra\n\t"
  "addiu    $v0, %2, 32\n\t"
  "move     $a1, %2\n\t"
  "move     $a0, $zero\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "srl      $v0, $v0, 16\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "sll      $v0, $v0, 16\n\t"
  "move     %0, $v0\n\t"
  "move     %1, $a1\n\t"
  "pcpyud   $ra, $ra, $ra\n\t"
  ".set reorder\n\t"
  : "=r"( lY1 ), "=r"( lY2 ) : "r"( anY ) : "a0", "a1", "a2", "v0", "v1"
 );

 lDelta = ( 512 >> lShift ) + 4;

 if ( lf16 ) {

  apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); 
  apDMA[ 1 ] = GIFTAG_REGS_AD;
  apDMA[ 2 ] = GS_SET_DTHE( 0 );
  apDMA[ 3 ] = GS_DTHE;
  apDMA += 4;

 }  /* end if */

 apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apDMA[ 1 ] = GS_TEX0_1 | ( GS_PRIM << 4 );
 apDMA[ 2 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, g_GSCtx.m_FontTexFmt, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, g_GSCtx.m_TextColor, GS_TEX_CLD_NOUPDATE );
 apDMA[ 3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_ON, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 apDMA[ 4 ] = GIF_TAG( aLen, !lf16, 0, 0, 1, 4 );
 apDMA[ 5 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );
 apDMA     += 6;

 while ( aLen-- ) {

  int lChr = *apStr++ - ' ';
  int lX, lU, lV;

  lX     = lCurX << 4;
  lCurX += g_GSCharWidth[ lChr ];

  lU   = ( lChr & 0x0000000F ) << 9;
  lV   = ( lChr & 0xFFFFFFF0 ) << 5;
  lX >>= lShift;

  apDMA[ 0 ] = GS_SET_UV( lU + 8, lV + 8 );
  apDMA[ 1 ] = lX | lY1;
  apDMA[ 2 ] = GS_SET_UV( lU + 506, lV + 506 );
  apDMA[ 3 ] = ( lX + lDelta ) | lY2;
  apDMA     += 4;

 }  /* end while */

 if ( lf16 ) {

  apDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 ); 
  apDMA[ 1 ] = GIFTAG_REGS_AD;
  apDMA[ 2 ] = GS_SET_DTHE( 1 );
  apDMA[ 3 ] = GS_DTHE;

 }  /* end if */

}  /* end GSFont_Render */

void GSFont_RenderEx ( unsigned char* apStr, int aLen, int aX, int anY, u64*           apDMA, int aDW, int aDH ) {

 int   lDW    = aDW << 4;
 float lAR    = ( 32 + aDW ) / 32.0F;
 int   lDTY   = 0;
 int   lH     = 32;
 int   lf16   = g_GSCtx.m_FontTexFmt != GSPixelFormat_PSMT4HL;
 int   lShift = g_XShift;
 int   lX, lY1, lY2, lU, lV, lDelta, lCurX = aX;

 if ( anY < 0 ) {

  lDTY   = -anY;
  lDTY <<= 4;
  lH    +=  anY;
  anY    = 0;

 }  /* end if */

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "pcpyld   $ra, $ra, $ra\n\t"
  "addu     $v0, %2, %4\n\t"
  "addu     $v0, $v0, %3\n\t"
  "move     $a1, %2\n\t"
  "move     $a0, $zero\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "srl      $v0, $v0, 16\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "sll      $v0, $v0, 16\n\t"
  "move     %0, $v0\n\t"
  "move     %1, $a1\n\t"
  "pcpyud   $ra, $ra, $ra\n\t"
  ".set reorder\n\t"
  : "=r"( lY1 ), "=r"( lY2 ) : "r"( anY ), "r"( aDH ), "r"( lH ) : "a0", "a1", "a2", "v0", "v1"
 );

 lDelta = ( 512 >> lShift ) + 4;
 lDW  >>= lShift;

 if ( lf16 ) {

  apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); 
  apDMA[ 1 ] = GIFTAG_REGS_AD;
  apDMA[ 2 ] = GS_SET_DTHE( 0 );
  apDMA[ 3 ] = GS_DTHE;
  apDMA += 4;

 }  /* end if */

 apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apDMA[ 1 ] = GS_TEX0_1 | ( GS_PRIM << 4 );
 apDMA[ 2 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, g_GSCtx.m_FontTexFmt, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, g_GSCtx.m_TextColor, GS_TEX_CLD_NOUPDATE );
 apDMA[ 3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_ON, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 apDMA[ 4 ] = GIF_TAG( aLen, !lf16, 0, 0, 1, 4 );
 apDMA[ 5 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );
 apDMA     += 6;

 while ( aLen-- ) {

  int lChr = *apStr++ - ' ';

  lX     = lCurX << 4;
  lCurX += ( int )(  ( float )g_GSCharWidth[ lChr ] * lAR + 0.5F  );
  lX   >>= lShift;

  lU = ( lChr & 0x0000000F ) << 9;
  lV = ( lChr & 0xFFFFFFF0 ) << 5;

  apDMA[ 0 ] = GS_SET_UV( lU + 8, lV + lDTY + 8 );
  apDMA[ 1 ] = lX | lY1;
  apDMA[ 2 ] = GS_SET_UV( lU + 506, lV + 506 );
  apDMA[ 3 ] = ( lX + lDelta + lDW ) | lY2;
  apDMA     += 4;

 }  /* end while */

 if ( lf16 ) {

  apDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 ); 
  apDMA[ 1 ] = GIFTAG_REGS_AD;
  apDMA[ 2 ] = GS_SET_DTHE( 1 );
  apDMA[ 3 ] = GS_DTHE;

 }  /* end if */

}  /* end GSFont_RenderEx */

void GSFont_Set ( unsigned int anIndex, void* apData ) {

 if ( !anIndex-- )
  g_pASCII = ( GSMTKFontHeader* )apData;
 else g_Fonts[ anIndex ] = ( GSMTKFontHeader* )apData;

}  /* end GSFont_Set */

void* GSFont_Get ( unsigned int anIndex, unsigned int* apSize ) {

 *apSize = s_FontSizes[ anIndex ];

 return !anIndex-- ? g_pASCII : g_Fonts[ anIndex ];

}  /* end GSFont_Get */

void* GSFont_Load ( const char* apFileName ) {

 static SMS_GUID sl_FontGUID = {
  0x59, 0x80, 0x9B, 0xAA, 0xBE, 0x82, 0x31, 0x46, 0x8F, 0xA5, 0x40, 0x7D, 0x34, 0x63, 0x31, 0xA5
 };

 SMS_GUID lGUID;
 int      lFD, lFileSize;

 lFD = MC_OpenS ( g_MCSlot, 0, apFileName + 4, O_RDONLY );

 if ( lFD >= 0 ) {

  lFileSize = MC_SeekS ( lFD, 0, SEEK_END );
  MC_SeekS ( lFD, 0, SEEK_SET );

  if (   MC_ReadS (  lFD, lGUID, sizeof ( lGUID )  ) == sizeof ( lGUID ) &&
         !memcmp (  lGUID, sl_FontGUID, sizeof ( lGUID )  )
  ) {

   unsigned int lDataSize;

   if (  MC_ReadS ( lFD, &lDataSize, 4 ) == 4  ) {

    char* lpFile;

    g_MBFont = ( unsigned int* )malloc ( lDataSize       );
    lpFile   = ( char*         )malloc ( lFileSize -= 20 );

    if (  MC_ReadS ( lFD, lpFile, lFileSize ) == lFileSize  ) {

     lpFile[ 0 ] ^= 'E';
     lpFile[ 1 ] ^= 'E';
     lpFile[ 2 ] ^= 'U';
     lpFile[ 3 ] ^= 'G';
     lpFile[ 4 ] ^= 'X';

     if ( lzma2_get_uncompressed_size(lpFile, lFileSize) != lDataSize )  goto error;
     
     lzma2_uncompress(lpFile, lFileSize, ( unsigned char* )g_MBFont, lDataSize);
		 
    } else {
error:
     free ( g_MBFont );
     g_MBFont = NULL;

    }  /* end else */

    free ( lpFile );

   }  /* end if */

  }  /* end if */

  MC_CloseS ( lFD );

 }  /* end if */

 return g_MBFont;

}  /* end GSFont_Load */

void GSFont_Unload ( void ) {

 if ( g_MBFont ) {
  free ( g_MBFont );
  g_MBFont = NULL;
 }  /* end if */

}  /* end GSFont_Unload */
