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
#include "SMS_GS.h"
#include "SMS_DMA.h"

#include <kernel.h>
#include <string.h>
#include <limits.h>

typedef struct _MTKFontHeader {

 char           m_ID [ 3 ]    __attribute__(  ( packed )  );
 char           m_ClrType     __attribute__(  ( packed )  );
 char           m_Unk[ 3 ]    __attribute__(  ( packed )  );
 unsigned short m_nGlyphs     __attribute__(  ( packed )  );
 unsigned char  m_GlyphWidth  __attribute__(  ( packed )  );
 unsigned char  m_GlyphHeight __attribute__(  ( packed )  );

} _MTKFontHeader;

typedef struct _Unaligned32 {

 unsigned int m_Val __attribute__(  ( packed )  );

} _Unaligned32;

GSCharIndent g_GSCharIndent[ 224 ];

extern unsigned char g_ASCII[ 4572 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1250 [ 7736 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1251 [ 7856 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1252 [ 7732 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_1253 [ 7588 ] __attribute__(   (  section( ".data" )  )   );

static _MTKFontHeader* s_pASCII     = ( _MTKFontHeader* )g_ASCII;
static _MTKFontHeader* s_Fonts[ 4 ] = {
 ( _MTKFontHeader* )g_1250,
 ( _MTKFontHeader* )g_1251,
 ( _MTKFontHeader* )g_1252,
 ( _MTKFontHeader* )g_1253
};

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

static void _font_character ( _MTKFontHeader* apHdr, unsigned int aChr, void* apBuf, char* apIndent ) {

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

  int lGap = -1;

  for ( j = 0; j < 16; ++j ) {

   lColor = lpBuff[ j ];

   if (  !( lColor & 0xF0 )  ) {
    ++lGap;
   } else break;

   if (  !( lColor & 0x0F )  )
    ++lGap;
   else break;

  }  /* end for */

  apIndent[ 0 ] = lGap;
  lGap          = -1;

  for ( j = 15; j >= 0; --j ) { 

   lColor = lpBuff[ j ];

   if (  !( lColor & 0x0F )  )
    ++lGap;
   else break;

   if (  !( lColor & 0xF0 )  )
    ++lGap;
   else break;

  }  /* end for */

  apIndent[ 32 ] = lGap; ++apIndent;

 }  /* end for */

}  /* end _font_character */

static void _set_indent ( void* apDst, unsigned short anIndent ) {

 __asm__ __volatile__ (
  "srl      $v1,  %1, 8\n\t"
  "pextlb    %1,  %1, %1\n\t"
  "pextlb   $v1, $v1, $v1\n\t"
  "pcpyh     %1,  %1\n\t"
  "pcpyh    $v1, $v1\n\t"
  "pcpyld    %1,  %1,  %1\n\t"
  "pcpyld   $v1, $v1, $v1\n\t"
  "sq       $v1,  0(%0)\n\t"
  "sq       $v1, 16(%0)\n\t"
  "sq        %1, 32(%0)\n\t"
  "sq        %1, 48(%0)\n\t"
  :: "r"( apDst ), "r"( anIndent )
 );

}  /* end _set_indent */

void GSFont_Init ( void ) {

 unsigned int    lX, lY, lIdx, lCharIdx = 0, lStrideIdx = 0;
 GSLoadImage     lLoadImage;
 unsigned char   lCharBuf[ 512 ] __attribute__(   (  aligned( 16 )  )   );
 _MTKFontHeader* lpFont = s_Fonts[ g_GSCtx.m_CodePage ];
 GSLoadImage*    lpLoadImage = UNCACHED_SEG( &lLoadImage );

 GS_InitLoadImage (
  &lLoadImage, g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 0, 0, 32, 32
 );

 lY = 0;
 lX = 0;

 SyncDCache ( &lLoadImage, &lLoadImage + 1 );

 for ( lIdx = 0; lIdx < 96; ++lIdx, ++lCharIdx ) {

  _font_character (  s_pASCII, lIdx, lCharBuf, ( char* )&g_GSCharIndent[ lCharIdx ]  );

  lpLoadImage -> m_TrxPosReg.m_Value = GS_SET_TRXPOS( 0, 0, lX, lY, 0 );

  SyncDCache ( lCharBuf, lCharBuf + 512 );
  GS_LoadImage ( &lLoadImage, lCharBuf );

  if ( ++lStrideIdx == 16 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 for ( lIdx = 0; lIdx < 128; ++lIdx, ++lCharIdx ) {

  _font_character (  lpFont, lIdx, lCharBuf, ( char* )&g_GSCharIndent[ lCharIdx ]  );

  lpLoadImage -> m_TrxPosReg.m_Value = GS_SET_TRXPOS( 0, 0, lX, lY, 0 );

  SyncDCache ( lCharBuf, lCharBuf + 512 );
  GS_LoadImage ( &lLoadImage, lCharBuf );

  if ( ++lStrideIdx == 16 ) {

   lY        += 32;
   lX         =  0;
   lStrideIdx =  0;

  } else lX += 32;

 }  /* end for */

 _set_indent ( &g_GSCharIndent[ ' ' - ' ' ], 0x0B0B );
 _set_indent ( &g_GSCharIndent[ '.' - ' ' ], 0x0018 );
 _set_indent ( &g_GSCharIndent[ ',' - ' ' ], 0x0018 );

 g_GSCtx.m_VRAMPtr += 496;

 DMA_Wait ( DMAC_GIF );

}  /* end GSFont_Init */

int GSFont_Width ( unsigned char* apStr, int aLen ) {

 int i, lKern, lX[ 32 ];
 int lChr = apStr[ 0 ] - ' ';

 for ( i = 0; i < 32; ++i ) lX[ i ] = g_GSCharIndent[ lChr ].m_Left[ i ];

 while ( aLen-- ) {

  lChr  = *apStr++ - ' ';
  lKern = -SHRT_MAX;

  for ( i = 0; i < 32; ++i ) {

   int lOffset = lX[ i ] - g_GSCharIndent[ lChr ].m_Left[ i ];

   __asm__ __volatile__(
    "pmaxw %0, %1, %2\n\t"
    : "=r"( lKern ) : "r"( lKern ), "r"( lOffset )
   );

  }  /* end for */

  for ( i = 0; i < 32; ++i ) lX[ i ] = lKern + 31 - g_GSCharIndent[ lChr ].m_Right[ i ];

 }  /* end for */

 lKern = -SHRT_MAX;

 for ( i = 0; i < 32; ++i )

  __asm__ __volatile__(
   "pmaxw %0, %1, %2\n\t"
   : "=r"( lKern ) : "r"( lKern ), "r"( lX[ i ] )
  );

 return lKern;

}  /* end GSFont_Width */

int GSFont_WidthEx ( unsigned char* apStr, int aLen, int aDW ) {

 int   i, lKern, lX[ 32 ];
 int   lChr = apStr[ 0 ] - ' ';
 float lAR  = ( 32.0F + aDW ) / 32.0F;

 for ( i = 0; i < 32; ++i ) lX[ i ] = g_GSCharIndent[ lChr ].m_Left[ i ] * lAR;

 while ( aLen-- ) {

  lChr  = *apStr++ - ' ';
  lKern = -SHRT_MAX;

  for ( i = 0; i < 32; ++i ) {

   int lOffset = lX[ i ] - g_GSCharIndent[ lChr ].m_Left[ i ] * lAR;

   __asm__ __volatile__(
    "pmaxw %0, %1, %2\n\t"
    : "=r"( lKern ) : "r"( lKern ), "r"( lOffset )
   );

  }  /* end for */

  for ( i = 0; i < 32; ++i ) lX[ i ] = lKern + ( 31 - g_GSCharIndent[ lChr ].m_Right[ i ] ) * lAR;

 }  /* end for */

 lKern = -SHRT_MAX;

 for ( i = 0; i < 32; ++i )

  __asm__ __volatile__(
   "pmaxw %0, %1, %2\n\t"
   : "=r"( lKern ) : "r"( lKern ), "r"( lX[ i ] )
  );

 return lKern;

}  /* end GSFont_WidthEx */

void GSFont_Render ( unsigned char* apStr, int aLen, int aX, int anY, unsigned long* apDMA ) {

 int lXV[ 32 ];
 int i;
 int lY1;
 int lY2;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "move     $t9, $ra\n\t"
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
  "move     $ra, $t9\n\t"
  ".set reorder\n\t"
  : "=r"( lY1 ), "=r"( lY2 ) : "r"( anY ) : "a0", "a1", "a2", "v0", "v1", "t9"
 );

 for ( i = 0; i < 32; ++i ) lXV[ i ] = aX;

 *apDMA++ = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 *apDMA++ = GS_TEX0_1 | ( GS_PRIM << 4 );
 *apDMA++ = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, g_GSCtx.m_CLUT[ g_GSCtx.m_TextColor ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD );
 *apDMA++ = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_ON, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 *apDMA++ = GIF_TAG( aLen, 1, 0, 0, 1, 4 );
 *apDMA++ = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 while ( aLen-- ) {

  int lChr  = *apStr++ - ' ';
  int lCurX = -INT_MAX;
  int lX, lU, lV;

  for ( i = 0; i < 32; ++i ) {

   int lOffset = lXV[ i ] - g_GSCharIndent[ lChr ].m_Left[ i ];

   __asm__ __volatile__(
    "pmaxw %0, %1, %2\n\t"
    : "=r"( lCurX ) : "r"( lCurX ), "r"( lOffset )
   );

  }  /* end for */

  lX = lCurX << 4;

  for ( i = 0; i < 32; ++i ) lXV[ i ] = lCurX + 31 - g_GSCharIndent[ lChr ].m_Right[ i ];

  lU = ( lChr & 0x0000000F ) << 9;
  lV = ( lChr & 0xFFFFFFF0 ) << 5;

  *apDMA++ = GS_SET_UV( lU + 8, lV + 8 );
  *apDMA++ = lX | lY1;
  *apDMA++ = GS_SET_UV( lU + 504, lV + 504 );
  *apDMA++ = ( lX + 512 ) | lY2;

 }  /* end while */

}  /* end GSFont_Render */

void GSFont_RenderEx ( unsigned char* apStr, int aLen, int aX, int anY, unsigned long* apDMA, int aDW, int aDH ) {

 int   lXV[ 32 ];
 int   i;
 int   lY1;
 int   lY2;
 int   lDW  = aDW << 4;
 float lAR  = ( 32.0F + aDW ) / 32.0F;
 int   lDTY = 0;
 int   lH   = 32;

 if ( anY < 0 ) {

  lDTY   = -anY;
  lDTY <<= 4;
  lH    +=  anY;
  anY    = 0;

 }  /* end if */

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "move     $t9, $ra\n\t"
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
  "move     $ra, $t9\n\t"
  ".set reorder\n\t"
  : "=r"( lY1 ), "=r"( lY2 ) : "r"( anY ), "r"( aDH ), "r"( lH ) : "a0", "a1", "a2", "v0", "v1", "t9"
 );

 for ( i = 0; i < 32; ++i ) lXV[ i ] = aX;

 *apDMA++ = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 *apDMA++ = GS_TEX0_1 | ( GS_PRIM << 4 );
 *apDMA++ = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, g_GSCtx.m_CLUT[ g_GSCtx.m_TextColor ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD );
 *apDMA++ = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_ON, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 *apDMA++ = GIF_TAG( aLen, 1, 0, 0, 1, 4 );
 *apDMA++ = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 while ( aLen-- ) {

  int lChr  = *apStr++ - ' ';
  int lCurX = -INT_MAX;
  int lX, lU, lV;

  for ( i = 0; i < 32; ++i ) {

   int lOffset = lXV[ i ] - g_GSCharIndent[ lChr ].m_Left[ i ] * lAR;

   __asm__ __volatile__(
    "pmaxw %0, %1, %2\n\t"
    : "=r"( lCurX ) : "r"( lCurX ), "r"( lOffset )
   );

  }  /* end for */

  lX = lCurX << 4;

  for ( i = 0; i < 32; ++i ) lXV[ i ] = lCurX + ( 31 - g_GSCharIndent[ lChr ].m_Right[ i ] ) * lAR;

  lU = ( lChr & 0x0000000F ) << 9;
  lV = ( lChr & 0xFFFFFFF0 ) << 5;

  *apDMA++ = GS_SET_UV( lU + 8, lV + lDTY + 8 );
  *apDMA++ = lX | lY1;
  *apDMA++ = GS_SET_UV( lU + 504, lV + 504 );
  *apDMA++ = ( lX + 512 + lDW ) | lY2;

 }  /* end while */

}  /* end GSFont_RenderEx */

void GSFont_Set ( unsigned int anIndex, void* apData ) {

 if ( !anIndex-- )

  s_pASCII = ( _MTKFontHeader* )apData;

 else s_Fonts[ anIndex ] = ( _MTKFontHeader* )apData;

}  /* end GSFont_Set */
