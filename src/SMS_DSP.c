/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_DSP.h"
#include "SMS_Data.h"

#include <string.h>

#define MAX_NEG_CROP 1024
#define ROW_SHIFT      11
#define COL_SHIFT      20
#define W1          22725
#define W2          21407
#define W3          19266
#define W4          16383
#define W5          12873
#define W6           8867
#define W7           4520

#define BLOCK_STRIDE_UV  8
#define BLOCK_STRIDE_Y  16

#define	BYTE_VEC32( c )	(  ( c ) * 0x01010101UL  )

uint8_t s_CropTbl[ 256 + 2 * MAX_NEG_CROP ];

const uint8_t g_SMS_DSP_zigzag_direct[ 64 ] = {
  0,  1,  8, 16,  9,  2,  3, 10,
 17, 24, 32, 25, 18, 11,  4,  5,
 12, 19, 26, 33, 40, 48, 41, 34,
 27, 20, 13,  6,  7, 14, 21, 28,
 35, 42, 49, 56, 57, 50, 43, 36,
 29, 22, 15, 23, 30, 37, 44, 51,
 58, 59, 52, 45, 38, 31, 39, 46,
 53, 60, 61, 54, 47, 55, 62, 63
};

const uint8_t g_SMS_DSP_alternate_horizontal_scan[ 64 ] = {
 0,   1,  2,  3,  8,  9, 16, 17,
 10, 11,  4,  5,  6,  7, 15, 14,
 13, 12, 19, 18, 24, 25, 32, 33,
 26, 27, 20, 21, 22, 23, 28, 29,
 30, 31, 34, 35, 40, 41, 48, 49,
 42, 43, 36, 37, 38, 39, 44, 45,
 46, 47, 50, 51, 56, 57, 58, 59,
 52, 53, 54, 55, 60, 61, 62, 63
};

const uint8_t g_SMS_DSP_alternate_vertical_scan[ 64 ] = {
  0,  8, 16, 24,  1,  9,  2, 10,
 17, 25, 32, 40, 48, 56, 57, 49,
 41, 33, 26, 18,  3, 11,  4, 12,
 19, 27, 34, 42, 50, 58, 35, 43,
 51, 59, 20, 28,  5, 13,  6, 14,
 21, 29, 36, 44, 52, 60, 37, 45,
 53, 61, 22, 30,  7, 15, 23, 31,
 38, 46, 54, 62, 39, 47, 55, 63
};
#ifdef _WIN32
SMS_ALIGN( uint8_t s_Full  [ 416 ], 16 );
SMS_ALIGN( uint8_t s_Half  [ 272 ], 16 );
SMS_ALIGN( uint8_t s_HalfHV[ 256 ], 16 );
#else  /* PS2 */
# define s_Full   SMS_DSP_SPR_FULL
# define s_Half   SMS_DSP_SPR_HALF
# define s_HalfHV SMS_DSP_SPR_HALF_HV
#endif  /* _WIN32 */

static SMS_INLINE void _copy_block9 ( uint8_t* apSrc, int aSrcStride ) {

 int      i;
 uint8_t* lpDst = s_Full;

 for ( i = 0; i < 9; ++i ) {

  *( uint32_t* )( lpDst + 0 ) = SMS_unaligned32 ( apSrc     );
  *( uint32_t* )( lpDst + 4 ) = SMS_unaligned32 ( apSrc + 4 );

  lpDst[ 8 ] = apSrc[ 8 ];

  lpDst += 16;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _copy_block9 */

static SMS_INLINE void _copy_block17 ( uint8_t* apSrc, int aSrcStride ) {

 int      i;
 uint8_t* lpDst = s_Full;

 for ( i = 0; i < 17; ++i ) {

  *( uint32_t* )( lpDst +  0 ) = SMS_unaligned32 ( apSrc      );
  *( uint32_t* )( lpDst +  4 ) = SMS_unaligned32 ( apSrc +  4 );
  *( uint32_t* )( lpDst +  8 ) = SMS_unaligned32 ( apSrc +  8 );
  *( uint32_t* )( lpDst + 12 ) = SMS_unaligned32 ( apSrc + 12 );

  lpDst[ 16 ] = apSrc[ 16 ];

  lpDst += 24;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _copy_block17 */
#ifndef _WIN32
extern void IDCT_Put ( uint8_t*, int, SMS_DCTELEM* );
extern void IDCT_Add ( uint8_t*, int, SMS_DCTELEM* );

static void IDCT_ClrBlocks ( SMS_DCTELEM* apBlocks ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set volatile\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "addiu    $a1, $zero, 12\n\t"
  "1:\n\t"
  "sq       $zero,  0($a0)\n\t"
  "sq       $zero, 16($a0)\n\t"
  "add      $a1, -1\n\t"
  "sq       $zero, 32($a0)\n\t"
  "sq       $zero, 48($a0)\n\t"
  "bgtzl    $a1, 1b\n\t"
  "addiu    $a0, 64\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set novolatile\n\t"
  ".set reorder\n\t"
  ::: "$5", "memory"
 );

}  /* end IDCT_ClrBlocks */
#endif  /* _WIN32 */
#ifdef _WIN32
static SMS_INLINE void _idctRowCondDC ( SMS_DCTELEM* row ) {

 int      a0, a1, a2, a3, b0, b1, b2, b3;
 uint32_t temp;

 if (    !(   (  ( uint32_t* )row  )[ 1 ] |
              (  ( uint32_t* )row  )[ 2 ] |
              (  ( uint32_t* )row  )[ 3 ] | 
              row[ 1 ]
          )
 ) {

  temp  = ( row[ 0 ] << 3 ) & 0xFFFF;
  temp += temp << 16;

  (  ( uint32_t* )row  )[ 0 ] = (  ( uint32_t* )row  )[ 1 ] =
  (  ( uint32_t* )row  )[ 2 ] = (  ( uint32_t* )row  )[ 3 ] = temp;

  return;

 }  /* end if */

 a0 = ( W4 * row[ 0 ] ) + (  1 << ( ROW_SHIFT - 1 )  );
 a1 = a0;
 a2 = a0;
 a3 = a0;

 a0 += W2 * row[ 2 ];
 a1 += W6 * row[ 2 ];
 a2 -= W6 * row[ 2 ];
 a3 -= W2 * row[ 2 ];

 b0  =  W1 * row[ 1 ];
 b0 +=  W3 * row[ 3 ];
 b1  =  W3 * row[ 1 ];
 b1 += -W7 * row[ 3 ];
 b2  =  W5 * row[ 1 ];
 b2 += -W1 * row[ 3 ];
 b3  =  W7 * row[ 1 ];
 b3 += -W5 * row[ 3 ];

 temp = (  ( uint32_t* )row  )[ 2 ] | (  ( uint32_t* )row  )[ 3 ];

 if ( temp != 0 ) {

  a0 +=  W4 * row[ 4 ] + W6 * row[ 6 ];
  a1 += -W4 * row[ 4 ] - W2 * row[ 6 ];
  a2 += -W4 * row[ 4 ] + W2 * row[ 6 ];
  a3 +=  W4 * row[ 4 ] - W6 * row[ 6 ];

  b0 += W5 * row[ 5 ];
  b0 += W7 * row[ 7 ];
            
  b1 += -W1 * row[ 5 ];
  b1 += -W5 * row[ 7 ];
            
  b2 += W7 * row[ 5 ];
  b2 += W3 * row[ 7 ];
            
  b3 +=  W3 * row[ 5 ];
  b3 += -W1 * row[ 7 ];

 }  /* end if */

 row[ 0 ] = ( a0 + b0 ) >> ROW_SHIFT;
 row[ 1 ] = ( a1 + b1 ) >> ROW_SHIFT;
 row[ 2 ] = ( a2 + b2 ) >> ROW_SHIFT;
 row[ 3 ] = ( a3 + b3 ) >> ROW_SHIFT;
 row[ 4 ] = ( a3 - b3 ) >> ROW_SHIFT;
 row[ 5 ] = ( a2 - b2 ) >> ROW_SHIFT;
 row[ 6 ] = ( a1 - b1 ) >> ROW_SHIFT;
 row[ 7 ] = ( a0 - b0 ) >> ROW_SHIFT;

}  /* end _idctRowCondDC */

static SMS_INLINE void _idctSparseColPut ( uint8_t* dest, int line_size, SMS_DCTELEM* col ) {

 int      a0, a1, a2, a3, b0, b1, b2, b3;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 a0 = W4 * (    col[ 0 ] + (   (  1 << ( COL_SHIFT - 1 )  ) / W4   )    );
 a1 = a0;
 a2 = a0;
 a3 = a0;

 a0 +=  W2 * col[ 16 ];
 a1 +=  W6 * col[ 16 ];
 a2 += -W6 * col[ 16 ];
 a3 += -W2 * col[ 16 ];

 b0 = W1 * col[ 8 ];
 b1 = W3 * col[ 8 ];
 b2 = W5 * col[ 8 ];
 b3 = W7 * col[ 8 ];

 b0 +=  W3 * col[ 24 ];
 b1 += -W7 * col[ 24 ];
 b2 += -W1 * col[ 24 ];
 b3 += -W5 * col[ 24 ];

 if ( col[ 32 ] ) {

  a0 +=  W4 * col[ 32 ];
  a1 += -W4 * col[ 32 ];
  a2 += -W4 * col[ 32 ];
  a3 +=  W4 * col[ 32 ];

 }  /* end if */

 if ( col[ 40 ] ) {

  b0 +=  W5 * col[ 40 ];
  b1 += -W1 * col[ 40 ];
  b2 +=  W7 * col[ 40 ];
  b3 +=  W3 * col[ 40 ];

 }  /* end if */

 if ( col[ 48 ] ) {

  a0 +=  W6 * col[ 48 ];
  a1 += -W2 * col[ 48 ];
  a2 +=  W2 * col[ 48 ];
  a3 += -W6 * col[ 48 ];

 }  /* end if */

 if ( col[ 56 ] ) {

  b0 +=  W7 * col[ 56 ];
  b1 += -W5 * col[ 56 ];
  b2 +=  W3 * col[ 56 ];
  b3 += -W1 * col[ 56 ];

 }  /* end if */

 dest[ 0 ] = lpCM[ ( a0 + b0 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a1 + b1 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a2 + b2 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a3 + b3 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a3 - b3 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a2 - b2 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a1 - b1 ) >> COL_SHIFT ];
 dest     += line_size;
 dest[ 0 ] = lpCM[ ( a0 - b0 ) >> COL_SHIFT ];

}  /* end _idctSparseColPut */

static SMS_INLINE void _idctSparseColAdd ( uint8_t* dest, int line_size, SMS_DCTELEM* col ) {

 int      a0, a1, a2, a3, b0, b1, b2, b3;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 a0 = W4 * (    col[ 0 ] + (   (  1 << ( COL_SHIFT - 1 )  ) / W4   )    );
 a1 = a0;
 a2 = a0;
 a3 = a0;

 a0 +=  W2 * col[ 16 ];
 a1 +=  W6 * col[ 16 ];
 a2 += -W6 * col[ 16 ];
 a3 += -W2 * col[ 16 ];

 b0 = W1 * col[ 8 ];
 b1 = W3 * col[ 8 ];
 b2 = W5 * col[ 8 ];
 b3 = W7 * col[ 8 ];

 b0 +=  W3 * col[ 24 ];
 b1 += -W7 * col[ 24 ];
 b2 += -W1 * col[ 24 ];
 b3 += -W5 * col[ 24 ];

 if ( col[ 32 ] ) {

  a0 +=  W4 * col[ 32 ];
  a1 += -W4 * col[ 32 ];
  a2 += -W4 * col[ 32 ];
  a3 +=  W4 * col[ 32 ];

 }  /* end if */

 if ( col[ 40 ] ) {

  b0 +=  W5 * col[ 40 ];
  b1 += -W1 * col[ 40 ];
  b2 +=  W7 * col[ 40 ];
  b3 +=  W3 * col[ 40 ];

 }  /* end if */

 if ( col[ 48 ] ) {

  a0 +=  W6 * col[ 48 ];
  a1 += -W2 * col[ 48 ];
  a2 +=  W2 * col[ 48 ];
  a3 += -W6 * col[ 48 ];

 }  /* end if */

 if ( col[ 56 ] ) {

  b0 +=  W7 * col[ 56 ];
  b1 += -W5 * col[ 56 ];
  b2 +=  W3 * col[ 56 ];
  b3 += -W1 * col[ 56 ];

 }  /* end if */

 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a0 + b0 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a1 + b1 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a2 + b2 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a3 + b3 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a3 - b3 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a2 - b2 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a1 - b1 ) >> COL_SHIFT  )  ];
 dest     += line_size;
 dest[ 0 ] = lpCM[  dest[ 0 ] + (  ( a0 - b0 ) >> COL_SHIFT  )  ];

}  /* end _idctSparseColAdd */

static void IDCT_Put ( uint8_t* apDst, int aLineSize, SMS_DCTELEM* apBlock ) {

 int i;
 for ( i = 0; i < 8; ++i ) _idctRowCondDC ( apBlock + i * 8 );
 for ( i = 0; i < 8; ++i ) _idctSparseColPut ( apDst + i, aLineSize, apBlock + i );

}  /* end IDCT_Put */

static void IDCT_Add ( uint8_t* apDest, int aLineSize, SMS_DCTELEM* apBlock ) {

 int i;
 for ( i = 0; i < 8; ++i ) _idctRowCondDC ( apBlock + i * 8 );
 for ( i = 0; i < 8; ++i ) _idctSparseColAdd ( apDest + i, aLineSize, apBlock + i );

}  /* end IDCT_Add */

static void IDCT_ClrBlocks ( SMS_DCTELEM* apBlocks ) {

 memset (  apBlocks, 0, sizeof( SMS_DCTELEM ) * 6 * 64  );

}  /* end IDCT_ClrBlocks */
#endif  /* _WIN32 */
void SMS_DSPInit ( void ) {

 int i;

 for ( i = 0; i < 256; ++i ) s_CropTbl[ i + MAX_NEG_CROP ] = i;

 for ( i = 0; i < MAX_NEG_CROP; ++i ) {

  s_CropTbl[ i                      ] = 0;
  s_CropTbl[ i + MAX_NEG_CROP + 256 ] = 255;

 }  /* end for */
#ifndef _WIN32
 memcpy ( SMS_DSP_SPR_CONST, &g_DataBuffer[ SMS_IDCT_CONST_OFFSET ], SMS_IDCT_CONST_SIZE );
#endif  /* _WIN32 */
}  /* end SMS_DSPInit */

static void GMC ( const SMS_DSPGMCData* apData ) {

 const int lS         = 1 << apData -> m_Shift;
 const int lDstStride = apData -> m_H;
 const int lRounder   = apData -> m_Rounder;
 const int lShift     = apData -> m_Shift << 1;
 const int lWidth     = apData -> m_Width  - 1;
 const int lHeight    = apData -> m_Height - 1;

 int lX, lY, lVX, lVY;
 int lOX = apData -> m_OX;
 int lOY = apData -> m_OY;

       uint8_t* lpDst = apData -> m_pDst;
 const uint8_t* lpSrc = apData -> m_pSrc;

 for ( lY = 0; lY < apData -> m_H; ++lY ) {

  lVX = lOX;
  lVY = lOY;

  for ( lX = 0; lX < 8; ++lX ) {

   int lSrcX, lSrcY, lFracX, lFracY, lIndex;

   lSrcX = lVX >> 16;
   lSrcY = lVY >> 16;

   lFracX = lSrcX & ( lS - 1 );
   lFracY = lSrcY & ( lS - 1 );

   lSrcX >>= apData -> m_Shift;
   lSrcY >>= apData -> m_Shift;

   if (  ( unsigned )lSrcX < ( unsigned )lWidth  ) {

    lSrcX -= apData -> m_DeltaX;
    lSrcX  = SMS_ModP2 ( lSrcX, lRounder );

    if (  ( unsigned )lSrcY < ( unsigned )lHeight  ) {

     lSrcY -= apData -> m_DeltaY;
     lSrcY  = SMS_ModP2 ( lSrcY, lRounder );

     lIndex = lSrcX + lSrcY * apData -> m_Stride;
     lpDst[ lY * lDstStride + lX ] = (  (  lpSrc[ lIndex     ] * ( lS - lFracX ) +
                                           lpSrc[ lIndex + 1 ] * lFracX
                                        ) * ( lS - lFracY ) +
                                        (  lpSrc[ lIndex + apData -> m_Stride     ] * ( lS - lFracX ) +
                                           lpSrc[ lIndex + apData -> m_Stride + 1 ] * lFracX
                                        ) * lFracY + apData -> m_R
                                     ) >> lShift;
    } else {

     lSrcY  = SMS_clip ( lSrcY, 0, lHeight );
     lSrcY -= apData -> m_DeltaY;
     lSrcY  = SMS_ModP2 ( lSrcY, lRounder );
     lIndex = lSrcX + lSrcY * apData -> m_Stride;

     lpDst[ lY * lDstStride + lX ] = (   ( lpSrc[ lIndex     ] * ( lS - lFracX ) +
                                           lpSrc[ lIndex + 1 ] * lFracX
                                         ) * lS + apData -> m_R
                                     ) >> lShift;

    }  /* end else */

   } else {

    if (  ( unsigned )lSrcY < ( unsigned )lHeight  ) {

     lSrcX  = SMS_clip ( lSrcX, 0, lWidth );
     lSrcX -= apData -> m_DeltaX;
     lSrcX  = SMS_ModP2 ( lSrcX, lRounder );
     lSrcY -= apData -> m_DeltaY;
     lSrcY  = SMS_ModP2 ( lSrcY, lRounder );
     lIndex = lSrcX + lSrcY * apData -> m_Stride;

     lpDst[ lY * lDstStride + lX ] = (  ( lpSrc[ lIndex                      ] * ( lS - lFracY ) +
                                          lpSrc[ lIndex + apData -> m_Stride ] * lFracY
                                        ) * lS + apData -> m_R
                                     ) >> lShift;

    } else {

     lSrcX  = SMS_clip ( lSrcX, 0, lWidth  );
     lSrcX -= apData -> m_DeltaX;
     lSrcX  = SMS_ModP2 ( lSrcX, lRounder );
     lSrcY  = SMS_clip ( lSrcY, 0, lHeight );
     lSrcY -= apData -> m_DeltaY;
     lSrcY  = SMS_ModP2 ( lSrcY, lRounder );
     lIndex = lSrcX + lSrcY * apData -> m_Stride;

     lpDst[ lY * lDstStride + lX ] = lpSrc[ lIndex ];

    }  /* end else */

   }  /* end else */

   lVX += apData -> m_DxX;
   lVY += apData -> m_DyX;

  }  /* end for */

  lOX += apData -> m_DxY;
  lOY += apData -> m_DyY;

 }  /* end for */

}  /* end GMC */
#ifdef _WIN32
static void GMC1 ( uint8_t* apDst, uint8_t* apSrc, int aStride, int aH, int aX16, int aY16, int aRounder ) {

 const short lA = ( 16 - aX16 ) * ( 16 - aY16 );
 const short lB = (      aX16 ) * ( 16 - aY16 );
 const short lC = ( 16 - aX16 ) * (      aY16 );
 const short lD = (      aX16 ) * (      aY16 );
 const short lH = aH;

 do {

  const uint8_t lSrc0 = apSrc[ aStride + 0 ];
  const uint8_t lSrc1 = apSrc[ aStride + 1 ];
  const uint8_t lSrc2 = apSrc[ aStride + 2 ];
  const uint8_t lSrc3 = apSrc[ aStride + 3 ];
  const uint8_t lSrc4 = apSrc[ aStride + 4 ];
  const uint8_t lSrc5 = apSrc[ aStride + 5 ];
  const uint8_t lSrc6 = apSrc[ aStride + 6 ];
  const uint8_t lSrc7 = apSrc[ aStride + 7 ];
  const uint8_t lSrc8 = apSrc[ aStride + 8 ];

  apDst[ 0 ] = ( lA * apSrc[ 0 ] + lB * apSrc[ 1 ] + lC * lSrc0 + lD * lSrc1 + ( short )aRounder ) >> 8;
  apDst[ 1 ] = ( lA * apSrc[ 1 ] + lB * apSrc[ 2 ] + lC * lSrc1 + lD * lSrc2 + ( short )aRounder ) >> 8;
  apDst[ 2 ] = ( lA * apSrc[ 2 ] + lB * apSrc[ 3 ] + lC * lSrc2 + lD * lSrc3 + ( short )aRounder ) >> 8;
  apDst[ 3 ] = ( lA * apSrc[ 3 ] + lB * apSrc[ 4 ] + lC * lSrc3 + lD * lSrc4 + ( short )aRounder ) >> 8;
  apDst[ 4 ] = ( lA * apSrc[ 4 ] + lB * apSrc[ 5 ] + lC * lSrc4 + lD * lSrc5 + ( short )aRounder ) >> 8;
  apDst[ 5 ] = ( lA * apSrc[ 5 ] + lB * apSrc[ 6 ] + lC * lSrc5 + lD * lSrc6 + ( short )aRounder ) >> 8;
  apDst[ 6 ] = ( lA * apSrc[ 6 ] + lB * apSrc[ 7 ] + lC * lSrc6 + lD * lSrc7 + ( short )aRounder ) >> 8;
  apDst[ 7 ] = ( lA * apSrc[ 7 ] + lB * apSrc[ 8 ] + lC * lSrc7 + lD * lSrc8 + ( short )aRounder ) >> 8;

  apDst += lH;
  apSrc += aStride;

 } while ( --aH );

}  // end GMC1
#else  /* PS2 */
extern void GMC1 ( uint8_t*, uint8_t*, int, int, int, int, int );
#endif  /* _WIN32 */
static SMS_INLINE uint32_t _rnd_avg32 ( uint32_t a, uint32_t b ) {
 return ( a | b ) - (   (  ( a ^ b ) & ~BYTE_VEC32( 0x01 )  ) >> 1   );
}  // end _rnd_avg32

static SMS_INLINE uint32_t _no_rnd_avg32 ( uint32_t a, uint32_t b ) {
 return ( a & b ) + (   (  ( a ^ b ) & ~BYTE_VEC32( 0x01 )  ) >> 1   );
}
/******************************************************************************/
/* Pixel 8 routines                                                           */
/******************************************************************************/
static SMS_INLINE void _put_pixels8_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  uint32_t lA, lB;

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );
  *( uint32_t* )( apDst + 0 ) = _rnd_avg32 ( lA, lB );

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );
  *( uint32_t* )( apDst + 4 ) = _rnd_avg32 ( lA, lB );

  apDst += aBlockStride;

 }  /* end for */

}  /* end _put_pixels8_l2 */

static SMS_INLINE void _put_no_rnd_pixels8_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  uint32_t lA, lB;

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );

  *( uint32_t* )&apDst[ i * aBlockStride + 0 ] = _no_rnd_avg32 ( lA, lB );

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );

  *( uint32_t* )&apDst[ i * aBlockStride + 4 ] = _no_rnd_avg32 ( lA, lB );

 }  /* end for */

}  /* end _put_no_rnd_pixels8_l2 */

static SMS_INLINE void _avg_pixels8_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  uint32_t lA, lB;

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );
  *( uint32_t* )&apDst[ i * aBlockStride + 0 ] = _rnd_avg32 (  *( uint32_t* )&apDst[ i * aBlockStride + 0 ], _rnd_avg32 ( lA, lB )  );

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );
  *( uint32_t* )&apDst[ i * aBlockStride + 4 ] = _rnd_avg32 (  *( uint32_t* )&apDst[ i * aBlockStride + 4 ], _rnd_avg32 ( lA, lB )  );

 }  /* end for */

}  /* end _avg_pixels8_l2 */

static SMS_INLINE void _avg_no_rnd_pixels8_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  uint32_t lA, lB;

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );

  *( uint32_t* )&apDst[ i * aBlockStride ] = _rnd_avg32 (
   *( uint32_t* )&apDst[ i * aBlockStride ], _no_rnd_avg32 ( lA, lB )
  );

  lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
  lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );

  *( uint32_t* )&apDst[ i * aBlockStride + 4 ] = _rnd_avg32 (
   *( uint32_t* )&apDst[ i * aBlockStride + 4 ], _no_rnd_avg32 ( lA, lB )
  );

 }  /* end for */

}  /* end _avg_no_rnd_pixels8_l2 */
#ifdef _WIN32
static void DSP_PutPixels8 ( uint8_t *apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock     )  ) = SMS_unaligned32 ( apPixels     );
  *(  ( uint32_t* )( apBlock + 4 )  ) = SMS_unaligned32 ( apPixels + 4 );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_UV;

 }  /* end for */

}  /* end DSP_PutPixels8 */
#else  /* PS2 */
extern void DSP_PutPixels8 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
#ifdef _WIN32
static void DSP_PutNoRndPixels8 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_PutPixels8 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_PutNoRndPixels8 */
#else  /* _PS2 */
extern void DSP_PutNoRndPixels8 ( uint8_t*, const uint8_t*, int, int );
#endif
static void DSP_AvgPixels8 ( uint8_t *apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock     )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock + 0 )  ), SMS_unaligned32 ( apPixels + 0 )   );
  *(  ( uint32_t* )( apBlock + 4 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock + 4 )  ), SMS_unaligned32 ( apPixels + 4 )   );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_UV;

 }  /* end for */

}  /* end DSP_AvgPixels8 */

static void DSP_AvgNoRndPixels8 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_AvgPixels8 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_AvgNoRndPixels8 */

static void DSP_PutPixels8X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels8X2 */

static void DSP_AvgPixels8X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels8X2 */

static void DSP_PutPixels8Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels8Y2 */

static void DSP_AvgPixels8Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgPixels8Y2 */
#ifdef _WIN32
static void DSP_PutPixels8XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x02020202UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                       (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA = SMS_unaligned32 ( apPixels     );
   uint32_t lB = SMS_unaligned32 ( apPixels + 1 );

   lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
   lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

   lA = SMS_unaligned32 ( apPixels     );
   lB = SMS_unaligned32 ( apPixels + 1 );

   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x02020202UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
         (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

  }  /* end for */

  apPixels += 4 - aBlockStride * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_UV * aH;

 }  /* end for */

}  /* end DSP_PutPixels8XY2 */
#else  /* PS2 */
extern void DSP_PutPixels8XY2 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
static void DSP_AvgPixels8XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x02020202UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                       (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA = SMS_unaligned32 ( apPixels     );
   uint32_t lB = SMS_unaligned32 ( apPixels + 1 );

   lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
   lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

   lA = SMS_unaligned32 ( apPixels     );
   lB = SMS_unaligned32 ( apPixels + 1 );

   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x02020202UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
         (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_UV * aH;

 }  /* end for */

}  /* end DSP_AvgPixels8XY2 */

static void DSP_PutNoRndPixels8X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutNoRndPixels8X2 */

static void DSP_AvgNoRndPixels8X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgNoRndPixels8X2 */

static void DSP_PutNoRndPixels8Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutNoRndPixels8Y2 */

static void DSP_AvgNoRndPixels8Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_UV, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgNoRndPixels8Y2 */

static void DSP_PutNoRndPixels8XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x01010101UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA  = SMS_unaligned32 ( apPixels     );
   uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
            lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
            lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

   lA  = SMS_unaligned32 ( apPixels     );
   lB  = SMS_unaligned32 ( apPixels + 1 );
   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x01010101UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_UV * aH;

 }  /* end for */

}  /* end DSP_PutNoRndPixels8XY2 */

static void DSP_AvgNoRndPixels8XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x01010101UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA  = SMS_unaligned32 ( apPixels     );
   uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
            lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
            lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

   lA  = SMS_unaligned32 ( apPixels     );
   lB  = SMS_unaligned32 ( apPixels + 1 );
   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x01010101UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_UV;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_UV * aH;

 }  /* end for */

}  /* end DSP_AvgNoRndPixels8XY2 */
/******************************************************************************/
/* Pixel 16 routines                                                          */
/******************************************************************************/
static SMS_INLINE void _put_pixels16_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int      i, j;
 uint8_t* lpDst  = apDst;

 for ( j = 0; j < 2; ++j ) {

  for ( i = 0; i < aH; ++i ) {

   uint32_t lA, lB;

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );
   *( uint32_t* )( lpDst + 0 ) = _rnd_avg32 ( lA, lB );

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );
   *( uint32_t* )( lpDst + 4 ) = _rnd_avg32 ( lA, lB );

   lpDst += BLOCK_STRIDE_Y;

  }  /* end for */

  lpDst   = apDst + 8;
  apSrc1 += 8;
  apSrc2 += 8;

 }  /* end for */

}  /* end _put_pixels16_l2 */

static SMS_INLINE void _put_no_rnd_pixels16_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  for ( i = 0; i < aH; ++i ) {

   uint32_t lA, lB;

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );

   *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y + 0 ] = _no_rnd_avg32 ( lA, lB );

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );

   *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y + 4 ] = _no_rnd_avg32 ( lA, lB );

  }  /* end for */

  apDst  += 8;
  apSrc1 += 8;
  apSrc2 += 8;

 }  /* end for */

}  /* end _put_no_rnd_pixels16_l2 */

static SMS_INLINE void _avg_pixels16_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int      i, j;
 uint8_t* lpDst  = apDst;

 for ( j = 0; j < 2; ++j ) {

  for ( i = 0; i < aH; ++i ) {

   uint32_t lA, lB;

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );
   *( uint32_t* )( lpDst + 0 ) = _rnd_avg32 (  *( uint32_t* )( lpDst + 0 ), _rnd_avg32 ( lA, lB )  );

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );
   *( uint32_t* )( lpDst + 4 ) = _rnd_avg32 (  *( uint32_t* )( lpDst + 4 ), _rnd_avg32 ( lA, lB )  );

   lpDst += BLOCK_STRIDE_Y;

  }  /* end for */

  lpDst   = apDst + 8;
  apSrc1 += 8;
  apSrc2 += 8;

 }  /* end for */

}  /* end _avg_pixels16_l2 */

static SMS_INLINE void _avg_no_rnd_pixels16_l2 ( uint8_t* apDst, const uint8_t* apSrc1, const uint8_t* apSrc2, int aBlockStride, int aSrcStride1, int aSrcStride2, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  for ( i = 0; i < aH; ++i ) {

   uint32_t lA, lB;

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 ] );

   *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y ] = _rnd_avg32 (
    *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y ], _no_rnd_avg32 ( lA, lB )
   );

   lA = SMS_unaligned32 ( &apSrc1[ i * aSrcStride1 + 4 ] );
   lB = SMS_unaligned32 ( &apSrc2[ i * aSrcStride2 + 4 ] );

   *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y + 4 ] = _rnd_avg32 (
    *( uint32_t* )&apDst[ i * BLOCK_STRIDE_Y + 4 ], _no_rnd_avg32 ( lA, lB )
   );

  }  /* end for */

  apDst  += 8;
  apSrc1 += 8;
  apSrc2 += 8;

 }  /* end for */

}  /* end _put_no_rnd_pixels16_l2 */
#ifdef _WIN32
static void DSP_PutPixels16 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock +  0 )  ) = SMS_unaligned32 ( apPixels +  0 );
  *(  ( uint32_t* )( apBlock +  4 )  ) = SMS_unaligned32 ( apPixels +  4 );
  *(  ( uint32_t* )( apBlock +  8 )  ) = SMS_unaligned32 ( apPixels +  8 );
  *(  ( uint32_t* )( apBlock + 12 )  ) = SMS_unaligned32 ( apPixels + 12 );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_PutPixels16 */
#else  /* PS2 */
extern void DSP_PutPixels16 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
#ifdef _WIN32
static void DSP_PutNoRndPixels16 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_PutPixels16 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_PutNoRndPixels16 */
#else  /* PS2 */
extern void DSP_PutNoRndPixels16 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
#ifdef _WIN32
static void DSP_AvgPixels16 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock +  0 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock +  0 )  ), SMS_unaligned32 ( apPixels +  0 )   );
  *(  ( uint32_t* )( apBlock +  4 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock +  4 )  ), SMS_unaligned32 ( apPixels +  4 )   );
  *(  ( uint32_t* )( apBlock +  8 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock +  8 )  ), SMS_unaligned32 ( apPixels +  8 )   );
  *(  ( uint32_t* )( apBlock + 12 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock + 12 )  ), SMS_unaligned32 ( apPixels + 12 )   );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_AvgPixels16 */
#else  /* PS2 */
extern void DSP_AvgPixels16 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
#ifdef _WIN32
static void DSP_AvgNoRndPixels16 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_AvgPixels16 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_AvgPixels16 */
#else  /* PS2 */
extern void DSP_AvgNoRndPixels16 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
static void DSP_PutPixels16X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels16_l2 (
  apBlock, apPixels, apPixels + 1, aBlockStride, aBlockStride, aBlockStride, aH
 );

}  /* end DSP_PutPixels16X2 */

static void DSP_AvgPixels16X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels16_l2 (
  apBlock, apPixels, apPixels + 1, aBlockStride, aBlockStride, aBlockStride, aH
 );

}  /* end DSP_AvgPixels16X2 */

static void DSP_PutPixels16Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels16_l2 (
  apBlock, apPixels, apPixels + aBlockStride, aBlockStride, aBlockStride, aBlockStride, aH
 );

}  /* end DSP_PutPixels16Y2 */

static void DSP_AvgPixels16Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels16_l2 (
  apBlock, apPixels, apPixels + aBlockStride, aBlockStride, aBlockStride, aBlockStride, aH
 );

}  /* end DSP_AvgPixels16Y2 */
#ifdef _WIN32
static void DSP_PutPixels16XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j, k;

 for ( k = 0; k < 2; ++k ) {

        uint8_t* lpBlock  = apBlock;
  const uint8_t* lpPixels = apPixels;

  for ( j = 0; j < 2; ++j ) {

   const uint32_t lA  = SMS_unaligned32 ( lpPixels     );
   const uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
         uint32_t lL0 = ( lA & 0x03030303UL ) +
                        ( lB & 0x03030303UL ) + 0x02020202UL;
         uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                        (  ( lB & 0xFCFCFCFCUL ) >> 2  );
         uint32_t lL1;
         uint32_t lH1;

   lpPixels += aBlockStride;

   for ( i = 0; i < aH; i += 2 ) {

    uint32_t lA = SMS_unaligned32 ( lpPixels     );
    uint32_t lB = SMS_unaligned32 ( lpPixels + 1 );

    lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
    lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

    lA = SMS_unaligned32 ( lpPixels     );
    lB = SMS_unaligned32 ( lpPixels + 1 );

    lL0 = ( lA & 0x03030303UL ) +
          ( lB & 0x03030303UL ) + 0x02020202UL;
    lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
          (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

   }  /* end for */

   lpPixels += 4 - aBlockStride   * ( aH + 1 );
   lpBlock  += 4 - BLOCK_STRIDE_Y * aH;

  }  /* end for */

  apBlock  += 8;
  apPixels += 8;

 }  /* end for */

}  /* end DSP_PutPixels16XY2 */
#else  /* PS2 */
extern void DSP_PutPixels16XY2 ( uint8_t*, const uint8_t*, int, int );
#endif  /* _WIN32 */
static void DSP_AvgPixels16XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j, k;

 for ( k = 0; k < 2; ++k ) {

        uint8_t* lpBlock  = apBlock;
  const uint8_t* lpPixels = apPixels;

  for ( j = 0; j < 2; ++j ) {

   const uint32_t lA  = SMS_unaligned32 ( lpPixels     );
   const uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
         uint32_t lL0 = ( lA & 0x03030303UL ) +
                        ( lB & 0x03030303UL ) + 0x02020202UL;
         uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                        (  ( lB & 0xFCFCFCFCUL ) >> 2  );
         uint32_t lL1;
         uint32_t lH1;

   lpPixels += aBlockStride;

   for ( i = 0; i < aH; i += 2 ) {

    uint32_t lA = SMS_unaligned32 ( lpPixels     );
    uint32_t lB = SMS_unaligned32 ( lpPixels + 1 );

    lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
    lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = _rnd_avg32 (
     *( uint32_t* )lpBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
    );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

    lA = SMS_unaligned32 ( lpPixels     );
    lB = SMS_unaligned32 ( lpPixels + 1 );

    lL0 = ( lA & 0x03030303UL ) +
          ( lB & 0x03030303UL ) + 0x02020202UL;
    lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
          (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = _rnd_avg32 (
     *( uint32_t* )lpBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
    );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

   }  /* end for */

   lpPixels += 4 - aBlockStride    * ( aH + 1 );
   lpBlock  += 4 - BLOCK_STRIDE_Y * aH;

  }  /* end for */

  apBlock  += 8;
  apPixels += 8;

 }  /* end for */

}  /* end DSP_AvgPixels16XY2 */

static void DSP_PutNoRndPixels16X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 _put_no_rnd_pixels16_l2 ( apBlock, apPixels, apPixels + 1, aLineSize, aLineSize, aLineSize, aH );

}  /* end DSP_PutNoRndPixels16X2 */

static void DSP_AvgNoRndPixels16X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 _avg_no_rnd_pixels16_l2 ( apBlock, apPixels, apPixels + 1, aLineSize, aLineSize, aLineSize, aH );

}  /* end DSP_AvgNoRndPixels16X2 */

static void DSP_PutNoRndPixels16Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 _put_no_rnd_pixels16_l2 ( apBlock, apPixels, apPixels + aLineSize, aLineSize, aLineSize, aLineSize, aH );

}  /* end DSP_PutNoRndPixels16Y2 */

static void DSP_AvgNoRndPixels16Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 _avg_no_rnd_pixels16_l2 ( apBlock, apPixels, apPixels + aLineSize, aLineSize, aLineSize, aLineSize, aH );

}  /* end DSP_AvgNoRndPixels16Y2 */

static void DSP_PutNoRndPixels16XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j, k;

 for ( k = 0; k < 2; ++k ) {

        uint8_t* lpBlock  = apBlock;
  const uint8_t* lpPixels = apPixels;

  for ( j = 0; j < 2; ++j ) {

   const uint32_t lA  = SMS_unaligned32 ( lpPixels     );
   const uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
         uint32_t lL0 = ( lA & 0x03030303UL ) +
                        ( lB & 0x03030303UL ) + 0x01010101UL;
         uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
         uint32_t lL1;
         uint32_t lH1;

   lpPixels += aBlockStride;

   for ( i = 0; i < aH; i += 2 ) {

    uint32_t lA  = SMS_unaligned32 ( lpPixels     );
    uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
             lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
             lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

    lA  = SMS_unaligned32 ( lpPixels     );
    lB  = SMS_unaligned32 ( lpPixels + 1 );
    lL0 = ( lA & 0x03030303UL ) +
          ( lB & 0x03030303UL ) + 0x01010101UL;
    lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

   }  /* end for */

   lpPixels += 4 - aBlockStride    * ( aH + 1 );
   lpBlock  += 4 - BLOCK_STRIDE_Y * aH;

  }  /* end for */

  apBlock  += 8;
  apPixels += 8;

 }  /* end for */

}  /* end DSP_PutNoRndPixels16XY2 */

static void DSP_AvgNoRndPixels16XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j, k;

 for ( k = 0; k < 2; ++k ) {

        uint8_t* lpBlock  = apBlock;
  const uint8_t* lpPixels = apPixels;

  for ( j = 0; j < 2; ++j ) {

   const uint32_t lA  = SMS_unaligned32 ( lpPixels     );
   const uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
         uint32_t lL0 = ( lA & 0x03030303UL ) +
                        ( lB & 0x03030303UL ) + 0x01010101UL;
         uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
         uint32_t lL1;
         uint32_t lH1;

   lpPixels += aBlockStride;

   for ( i = 0; i < aH; i += 2 ) {

    uint32_t lA  = SMS_unaligned32 ( lpPixels     );
    uint32_t lB  = SMS_unaligned32 ( lpPixels + 1 );
             lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
             lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = _rnd_avg32 (
     *( uint32_t* )lpBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
    );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

    lA  = SMS_unaligned32 ( lpPixels     );
    lB  = SMS_unaligned32 ( lpPixels + 1 );
    lL0 = ( lA & 0x03030303UL ) +
          ( lB & 0x03030303UL ) + 0x01010101UL;
    lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

    *( uint32_t* )lpBlock = _rnd_avg32 (
     *( uint32_t* )lpBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
    );

    lpPixels += aBlockStride;
    lpBlock  += BLOCK_STRIDE_Y;

   }  /* end for */

   lpPixels += 4 - aBlockStride    * ( aH + 1 );
   lpBlock  += 4 - BLOCK_STRIDE_Y * aH;

  }  /* end for */

  apBlock  += 8;
  apPixels += 8;

 }  /* end for */

}  /* end DSP_AvgNoRndPixels16XY2 */
/******************************************************************************/
/* Pixel 8_16 routines                                                           */
/******************************************************************************/
static void DSP_PutPixels816 ( uint8_t *apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock     )  ) = SMS_unaligned32 ( apPixels     );
  *(  ( uint32_t* )( apBlock + 4 )  ) = SMS_unaligned32 ( apPixels + 4 );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_PutPixels816 */

static void DSP_PutNoRndPixels816 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_PutPixels816 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_PutNoRndPixels816 */

static void DSP_AvgPixels816 ( uint8_t *apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i;

 for ( i = 0; i < aH; ++i ) {

  *(  ( uint32_t* )( apBlock     )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock + 0 )  ), SMS_unaligned32 ( apPixels + 0 )   );
  *(  ( uint32_t* )( apBlock + 4 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apBlock + 4 )  ), SMS_unaligned32 ( apPixels + 4 )   );
  apPixels += aBlockStride;
  apBlock  += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_AvgPixels816 */

static void DSP_AvgNoRndPixels816 ( uint8_t* apBlock, const uint8_t* apPixels, int aLineSize, int aH ) {

 DSP_AvgPixels816 ( apBlock, apPixels, aLineSize, aH );

}  /* end DSP_AvgNoRndPixels816 */

static void DSP_PutPixels816X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels816X2 */

static void DSP_AvgPixels816X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels816X2 */

static void DSP_PutPixels816Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutPixels816Y2 */

static void DSP_AvgPixels816Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgPixels816Y2 */

static void DSP_PutPixels816XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x02020202UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                       (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA = SMS_unaligned32 ( apPixels     );
   uint32_t lB = SMS_unaligned32 ( apPixels + 1 );

   lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
   lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

   lA = SMS_unaligned32 ( apPixels     );
   lB = SMS_unaligned32 ( apPixels + 1 );

   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x02020202UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
         (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_Y * aH;

 }  /* end for */

}  /* end DSP_PutPixels816XY2 */

static void DSP_AvgPixels816XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x02020202UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
                       (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA = SMS_unaligned32 ( apPixels     );
   uint32_t lB = SMS_unaligned32 ( apPixels + 1 );

   lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
   lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

   lA = SMS_unaligned32 ( apPixels     );
   lB = SMS_unaligned32 ( apPixels + 1 );

   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x02020202UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) +
         (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_Y * aH;

 }  /* end for */

}  /* end DSP_AvgPixels816XY2 */

static void DSP_PutNoRndPixels816X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutNoRndPixels816X2 */

static void DSP_AvgNoRndPixels816X2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + 1, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgNoRndPixels816X2 */

static void DSP_PutNoRndPixels816Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _put_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_PutNoRndPixels816Y2 */

static void DSP_AvgNoRndPixels816Y2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 _avg_no_rnd_pixels8_l2 ( apBlock, apPixels, apPixels + aBlockStride, BLOCK_STRIDE_Y, aBlockStride, aBlockStride, aH );

}  /* end DSP_AvgNoRndPixels816Y2 */

static void DSP_PutNoRndPixels816XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x01010101UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA  = SMS_unaligned32 ( apPixels     );
   uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
            lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
            lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

   lA  = SMS_unaligned32 ( apPixels     );
   lB  = SMS_unaligned32 ( apPixels + 1 );
   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x01010101UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_Y * aH;

 }  /* end for */

}  /* end DSP_PutNoRndPixels816XY2 */

static void DSP_AvgNoRndPixels816XY2 ( uint8_t* apBlock, const uint8_t* apPixels, int aBlockStride, int aH ) {

 int i, j;

 for ( j = 0; j < 2; ++j ) {

  const uint32_t lA  = SMS_unaligned32 ( apPixels     );
  const uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
        uint32_t lL0 = ( lA & 0x03030303UL ) +
                       ( lB & 0x03030303UL ) + 0x01010101UL;
        uint32_t lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );
        uint32_t lL1;
        uint32_t lH1;

  apPixels += aBlockStride;

  for ( i = 0; i < aH; i += 2 ) {

   uint32_t lA  = SMS_unaligned32 ( apPixels     );
   uint32_t lB  = SMS_unaligned32 ( apPixels + 1 );
            lL1 = ( lA & 0x03030303UL ) + ( lB & 0x03030303UL );
            lH1 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

   lA  = SMS_unaligned32 ( apPixels     );
   lB  = SMS_unaligned32 ( apPixels + 1 );
   lL0 = ( lA & 0x03030303UL ) +
         ( lB & 0x03030303UL ) + 0x01010101UL;
   lH0 = (  ( lA & 0xFCFCFCFCUL ) >> 2  ) + (  ( lB & 0xFCFCFCFCUL ) >> 2  );

   *( uint32_t* )apBlock = _rnd_avg32 (
    *( uint32_t* )apBlock, lH0 + lH1 + (   (  ( lL0 + lL1 ) >> 2  ) & 0x0F0F0F0FUL   )
   );

   apPixels += aBlockStride;
   apBlock  += BLOCK_STRIDE_Y;

  }  /* end for */

  apPixels += 4 - aBlockStride   * ( aH + 1 );
  apBlock  += 4 - BLOCK_STRIDE_Y * aH;

 }  /* end for */

}  /* end DSP_AvgNoRndPixels816XY2 */
/******************************************************************************/
/* QPel 8 routines                                                            */
/******************************************************************************/
static void _put_mpeg4_qpel8_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[ 0 ] = lpCM[ (  ( apSrc[ 0 ] + apSrc[ 1 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 2 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 3 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 4 ] ) + 16  ) >> 5 ];
  apDst[ 1 ] = lpCM[ (  ( apSrc[ 1 ] + apSrc[ 2 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 3 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 4 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 5 ] ) + 16  ) >> 5 ];
  apDst[ 2 ] = lpCM[ (  ( apSrc[ 2 ] + apSrc[ 3 ] ) * 20 - ( apSrc[ 1 ] + apSrc[ 4 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 5 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 6 ] ) + 16  ) >> 5 ];
  apDst[ 3 ] = lpCM[ (  ( apSrc[ 3 ] + apSrc[ 4 ] ) * 20 - ( apSrc[ 2 ] + apSrc[ 5 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 6 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 7 ] ) + 16  ) >> 5 ];
  apDst[ 4 ] = lpCM[ (  ( apSrc[ 4 ] + apSrc[ 5 ] ) * 20 - ( apSrc[ 3 ] + apSrc[ 6 ] ) * 6 + ( apSrc[ 2 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 8 ] ) + 16  ) >> 5 ];
  apDst[ 5 ] = lpCM[ (  ( apSrc[ 5 ] + apSrc[ 6 ] ) * 20 - ( apSrc[ 4 ] + apSrc[ 7 ] ) * 6 + ( apSrc[ 3 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 8 ] ) + 16  ) >> 5 ];
  apDst[ 6 ] = lpCM[ (  ( apSrc[ 6 ] + apSrc[ 7 ] ) * 20 - ( apSrc[ 5 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 4 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 3 ] + apSrc[ 7 ] ) + 16  ) >> 5 ];
  apDst[ 7 ] = lpCM[ (  ( apSrc[ 7 ] + apSrc[ 8 ] ) * 20 - ( apSrc[ 6 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 5 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 4 ] + apSrc[ 6 ] ) + 16  ) >> 5 ];

  apDst += aBlockStride;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _put_mpeg4_qpel8_h_lowpass */

static void _avg_mpeg4_qpel8_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[ 0 ] = ( apDst[ 0 ] + lpCM[ (  ( apSrc[ 0 ] + apSrc[ 1 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 2 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 3 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 4 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 1 ] = ( apDst[ 1 ] + lpCM[ (  ( apSrc[ 1 ] + apSrc[ 2 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 3 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 4 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 5 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 2 ] = ( apDst[ 2 ] + lpCM[ (  ( apSrc[ 2 ] + apSrc[ 3 ] ) * 20 - ( apSrc[ 1 ] + apSrc[ 4 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 5 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 6 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 3 ] = ( apDst[ 3 ] + lpCM[ (  ( apSrc[ 3 ] + apSrc[ 4 ] ) * 20 - ( apSrc[ 2 ] + apSrc[ 5 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 6 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 7 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 4 ] = ( apDst[ 4 ] + lpCM[ (  ( apSrc[ 4 ] + apSrc[ 5 ] ) * 20 - ( apSrc[ 3 ] + apSrc[ 6 ] ) * 6 + ( apSrc[ 2 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 8 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 5 ] = ( apDst[ 5 ] + lpCM[ (  ( apSrc[ 5 ] + apSrc[ 6 ] ) * 20 - ( apSrc[ 4 ] + apSrc[ 7 ] ) * 6 + ( apSrc[ 3 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 8 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 6 ] = ( apDst[ 6 ] + lpCM[ (  ( apSrc[ 6 ] + apSrc[ 7 ] ) * 20 - ( apSrc[ 5 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 4 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 3 ] + apSrc[ 7 ] ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 7 ] = ( apDst[ 7 ] + lpCM[ (  ( apSrc[ 7 ] + apSrc[ 8 ] ) * 20 - ( apSrc[ 6 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 5 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 4 ] + apSrc[ 6 ] ) + 16  ) >> 5 ] + 1 ) >> 1;

  apDst += aBlockStride;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _avg_mpeg4_qpel8_h_lowpass */

static void _put_no_rnd_mpeg4_qpel8_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[ 0 ] = lpCM[ (  ( apSrc[ 0 ] + apSrc[ 1 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 2 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 3 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 4 ] ) + 15  ) >> 5 ];
  apDst[ 1 ] = lpCM[ (  ( apSrc[ 1 ] + apSrc[ 2 ] ) * 20 - ( apSrc[ 0 ] + apSrc[ 3 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 4 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 5 ] ) + 15  ) >> 5 ];
  apDst[ 2 ] = lpCM[ (  ( apSrc[ 2 ] + apSrc[ 3 ] ) * 20 - ( apSrc[ 1 ] + apSrc[ 4 ] ) * 6 + ( apSrc[ 0 ] + apSrc[ 5 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 6 ] ) + 15  ) >> 5 ];
  apDst[ 3 ] = lpCM[ (  ( apSrc[ 3 ] + apSrc[ 4 ] ) * 20 - ( apSrc[ 2 ] + apSrc[ 5 ] ) * 6 + ( apSrc[ 1 ] + apSrc[ 6 ] ) * 3 - ( apSrc[ 0 ] + apSrc[ 7 ] ) + 15  ) >> 5 ];
  apDst[ 4 ] = lpCM[ (  ( apSrc[ 4 ] + apSrc[ 5 ] ) * 20 - ( apSrc[ 3 ] + apSrc[ 6 ] ) * 6 + ( apSrc[ 2 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 1 ] + apSrc[ 8 ] ) + 15  ) >> 5 ];
  apDst[ 5 ] = lpCM[ (  ( apSrc[ 5 ] + apSrc[ 6 ] ) * 20 - ( apSrc[ 4 ] + apSrc[ 7 ] ) * 6 + ( apSrc[ 3 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 2 ] + apSrc[ 8 ] ) + 15  ) >> 5 ];
  apDst[ 6 ] = lpCM[ (  ( apSrc[ 6 ] + apSrc[ 7 ] ) * 20 - ( apSrc[ 5 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 4 ] + apSrc[ 8 ] ) * 3 - ( apSrc[ 3 ] + apSrc[ 7 ] ) + 15  ) >> 5 ];
  apDst[ 7 ] = lpCM[ (  ( apSrc[ 7 ] + apSrc[ 8 ] ) * 20 - ( apSrc[ 6 ] + apSrc[ 8 ] ) * 6 + ( apSrc[ 5 ] + apSrc[ 7 ] ) * 3 - ( apSrc[ 4 ] + apSrc[ 6 ] ) + 15  ) >> 5 ];

  apDst += aBlockStride;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _put_no_rnd_mpeg4_qpel8_h_lowpass */

static void _put_mpeg4_qpel8_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 8; ++i ) {

  const int lSrc0 = apSrc[ 0 * aSrcStride ];
  const int lSrc1 = apSrc[ 1 * aSrcStride ];
  const int lSrc2 = apSrc[ 2 * aSrcStride ];
  const int lSrc3 = apSrc[ 3 * aSrcStride ];
  const int lSrc4 = apSrc[ 4 * aSrcStride ];
  const int lSrc5 = apSrc[ 5 * aSrcStride ];
  const int lSrc6 = apSrc[ 6 * aSrcStride ];
  const int lSrc7 = apSrc[ 7 * aSrcStride ];
  const int lSrc8 = apSrc[ 8 * aSrcStride ];

  apDst[ 0 * aBlockStride ] = lpCM[ (  ( lSrc0 + lSrc1 ) * 20 - ( lSrc0 + lSrc2 ) * 6 + ( lSrc1 + lSrc3 ) * 3 - ( lSrc2 + lSrc4 ) + 16  ) >> 5 ];
  apDst[ 1 * aBlockStride ] = lpCM[ (  ( lSrc1 + lSrc2 ) * 20 - ( lSrc0 + lSrc3 ) * 6 + ( lSrc0 + lSrc4 ) * 3 - ( lSrc1 + lSrc5 ) + 16  ) >> 5 ];
  apDst[ 2 * aBlockStride ] = lpCM[ (  ( lSrc2 + lSrc3 ) * 20 - ( lSrc1 + lSrc4 ) * 6 + ( lSrc0 + lSrc5 ) * 3 - ( lSrc0 + lSrc6 ) + 16  ) >> 5 ];
  apDst[ 3 * aBlockStride ] = lpCM[ (  ( lSrc3 + lSrc4 ) * 20 - ( lSrc2 + lSrc5 ) * 6 + ( lSrc1 + lSrc6 ) * 3 - ( lSrc0 + lSrc7 ) + 16  ) >> 5 ];
  apDst[ 4 * aBlockStride ] = lpCM[ (  ( lSrc4 + lSrc5 ) * 20 - ( lSrc3 + lSrc6 ) * 6 + ( lSrc2 + lSrc7 ) * 3 - ( lSrc1 + lSrc8 ) + 16  ) >> 5 ];
  apDst[ 5 * aBlockStride ] = lpCM[ (  ( lSrc5 + lSrc6 ) * 20 - ( lSrc4 + lSrc7 ) * 6 + ( lSrc3 + lSrc8 ) * 3 - ( lSrc2 + lSrc8 ) + 16  ) >> 5 ];
  apDst[ 6 * aBlockStride ] = lpCM[ (  ( lSrc6 + lSrc7 ) * 20 - ( lSrc5 + lSrc8 ) * 6 + ( lSrc4 + lSrc8 ) * 3 - ( lSrc3 + lSrc7 ) + 16  ) >> 5 ];
  apDst[ 7 * aBlockStride ] = lpCM[ (  ( lSrc7 + lSrc8 ) * 20 - ( lSrc6 + lSrc8 ) * 6 + ( lSrc5 + lSrc7 ) * 3 - ( lSrc4 + lSrc6 ) + 16  ) >> 5 ];

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _put_mpeg4_qpel8_v_lowpass */

static void _avg_mpeg4_qpel8_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 8; ++i ) {

  const int lSrc0 = apSrc[ 0 * aSrcStride ];
  const int lSrc1 = apSrc[ 1 * aSrcStride ];
  const int lSrc2 = apSrc[ 2 * aSrcStride ];
  const int lSrc3 = apSrc[ 3 * aSrcStride ];
  const int lSrc4 = apSrc[ 4 * aSrcStride ];
  const int lSrc5 = apSrc[ 5 * aSrcStride ];
  const int lSrc6 = apSrc[ 6 * aSrcStride ];
  const int lSrc7 = apSrc[ 7 * aSrcStride ];
  const int lSrc8 = apSrc[ 8 * aSrcStride ];

  apDst[ 0 * aBlockStride ] = (  apDst[ 0 * aBlockStride ] + lpCM[ (  ( lSrc0 + lSrc1 ) * 20 - ( lSrc0 + lSrc2 ) * 6 + ( lSrc1 + lSrc3 ) * 3 - ( lSrc2 + lSrc4 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 1 * aBlockStride ] = (  apDst[ 1 * aBlockStride ] + lpCM[ (  ( lSrc1 + lSrc2 ) * 20 - ( lSrc0 + lSrc3 ) * 6 + ( lSrc0 + lSrc4 ) * 3 - ( lSrc1 + lSrc5 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 2 * aBlockStride ] = (  apDst[ 2 * aBlockStride ] + lpCM[ (  ( lSrc2 + lSrc3 ) * 20 - ( lSrc1 + lSrc4 ) * 6 + ( lSrc0 + lSrc5 ) * 3 - ( lSrc0 + lSrc6 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 3 * aBlockStride ] = (  apDst[ 3 * aBlockStride ] + lpCM[ (  ( lSrc3 + lSrc4 ) * 20 - ( lSrc2 + lSrc5 ) * 6 + ( lSrc1 + lSrc6 ) * 3 - ( lSrc0 + lSrc7 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 4 * aBlockStride ] = (  apDst[ 4 * aBlockStride ] + lpCM[ (  ( lSrc4 + lSrc5 ) * 20 - ( lSrc3 + lSrc6 ) * 6 + ( lSrc2 + lSrc7 ) * 3 - ( lSrc1 + lSrc8 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 5 * aBlockStride ] = (  apDst[ 5 * aBlockStride ] + lpCM[ (  ( lSrc5 + lSrc6 ) * 20 - ( lSrc4 + lSrc7 ) * 6 + ( lSrc3 + lSrc8 ) * 3 - ( lSrc2 + lSrc8 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 6 * aBlockStride ] = (  apDst[ 6 * aBlockStride ] + lpCM[ (  ( lSrc6 + lSrc7 ) * 20 - ( lSrc5 + lSrc8 ) * 6 + ( lSrc4 + lSrc8 ) * 3 - ( lSrc3 + lSrc7 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 7 * aBlockStride ] = (  apDst[ 7 * aBlockStride ] + lpCM[ (  ( lSrc7 + lSrc8 ) * 20 - ( lSrc6 + lSrc8 ) * 6 + ( lSrc5 + lSrc7 ) * 3 - ( lSrc4 + lSrc6 ) + 16  ) >> 5 ] + 1 ) >> 1;

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _avg_mpeg4_qpel8_v_lowpass */

static void _put_no_rnd_mpeg4_qpel8_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 8; ++i ) {

  const int lSrc0 = apSrc[ 0 * aSrcStride ];
  const int lSrc1 = apSrc[ 1 * aSrcStride ];
  const int lSrc2 = apSrc[ 2 * aSrcStride ];
  const int lSrc3 = apSrc[ 3 * aSrcStride ];
  const int lSrc4 = apSrc[ 4 * aSrcStride ];
  const int lSrc5 = apSrc[ 5 * aSrcStride ];
  const int lSrc6 = apSrc[ 6 * aSrcStride ];
  const int lSrc7 = apSrc[ 7 * aSrcStride ];
  const int lSrc8 = apSrc[ 8 * aSrcStride ];

  apDst[ 0 * aBlockStride ] = lpCM[ (  ( lSrc0 + lSrc1 ) * 20 - ( lSrc0 + lSrc2 ) * 6 + ( lSrc1 + lSrc3 ) * 3 - ( lSrc2 + lSrc4 ) + 15  ) >> 5 ];
  apDst[ 1 * aBlockStride ] = lpCM[ (  ( lSrc1 + lSrc2 ) * 20 - ( lSrc0 + lSrc3 ) * 6 + ( lSrc0 + lSrc4 ) * 3 - ( lSrc1 + lSrc5 ) + 15  ) >> 5 ];
  apDst[ 2 * aBlockStride ] = lpCM[ (  ( lSrc2 + lSrc3 ) * 20 - ( lSrc1 + lSrc4 ) * 6 + ( lSrc0 + lSrc5 ) * 3 - ( lSrc0 + lSrc6 ) + 15  ) >> 5 ];
  apDst[ 3 * aBlockStride ] = lpCM[ (  ( lSrc3 + lSrc4 ) * 20 - ( lSrc2 + lSrc5 ) * 6 + ( lSrc1 + lSrc6 ) * 3 - ( lSrc0 + lSrc7 ) + 15  ) >> 5 ];
  apDst[ 4 * aBlockStride ] = lpCM[ (  ( lSrc4 + lSrc5 ) * 20 - ( lSrc3 + lSrc6 ) * 6 + ( lSrc2 + lSrc7 ) * 3 - ( lSrc1 + lSrc8 ) + 15  ) >> 5 ];
  apDst[ 5 * aBlockStride ] = lpCM[ (  ( lSrc5 + lSrc6 ) * 20 - ( lSrc4 + lSrc7 ) * 6 + ( lSrc3 + lSrc8 ) * 3 - ( lSrc2 + lSrc8 ) + 15  ) >> 5 ];
  apDst[ 6 * aBlockStride ] = lpCM[ (  ( lSrc6 + lSrc7 ) * 20 - ( lSrc5 + lSrc8 ) * 6 + ( lSrc4 + lSrc8 ) * 3 - ( lSrc3 + lSrc7 ) + 15  ) >> 5 ];
  apDst[ 7 * aBlockStride ] = lpCM[ (  ( lSrc7 + lSrc8 ) * 20 - ( lSrc6 + lSrc8 ) * 6 + ( lSrc5 + lSrc7 ) * 3 - ( lSrc4 + lSrc6 ) + 15  ) >> 5 ];

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _put_no_rnd_mpeg4_qpel8_v_lowpass */

static void DSP_PutQPel8MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_PutPixels8 ( apDst, apSrc, aStride, 8 );

}  /* end DSP_PutQPel8MC00 */

static void DSP_AvgQPel8MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_AvgPixels8  ( apDst, apSrc, aStride, 8 );

}  /* end DSP_AvgQPel8MC00 */

static void DSP_PutNoRndQPel8MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_PutNoRndPixels8 ( apDst, apSrc, aStride, 8 );

}  /* end DSP_PutNoRndQPel8MC00 */

static void DSP_PutQPel8MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_PutQPel8MC10 */

static void DSP_AvgQPel8MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _avg_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_AvgQPel8MC10 */

static void DSP_PutNoRndQPel8MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC10 */

static void DSP_PutQPel8MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_UV, aBlockStride, 8 );

}  /* end DSP_PutQPel8MC20 */

static void DSP_AvgQPel8MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _avg_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_UV, aBlockStride, 8 );

}  /* end DSP_AvgQPel8MC20 */

static void DSP_PutNoRndQPel8MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_UV, aBlockStride, 8 );

}  /* end DSP_PutNoRndQPel8MC20 */

static void DSP_PutQPel8MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_pixels8_l2 ( apDst, apSrc + 1, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_PutQPel8MC30 */

static void DSP_AvgQPel8MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _avg_pixels8_l2 ( apDst, apSrc + 1, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_AvgQPel8MC30 */

static void DSP_PutNoRndQPel8MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, apSrc+1, s_Half, BLOCK_STRIDE_UV, aBlockStride, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC30 */

static void DSP_PutQPel8MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* end DSP_PutQPel8MC01 */

static void DSP_AvgQPel8MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _avg_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* end DSP_AvgQPel8MC01 */

static void DSP_PutNoRndQPel8MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC01 */

static void DSP_PutQPel8MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC11 */

static void DSP_AvgQPel8MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC11 */

static void DSP_PutNoRndQPel8MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC11 */

static void DSP_PutQPel8MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC21 */

static void DSP_AvgQPel8MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC21 */

static void DSP_PutNoRndQPel8MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC21 */

static void DSP_PutQPel8MC31 ( uint8_t* apDst, uint8_t *apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC31 */

static void DSP_AvgQPel8MC31 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC31 */

static void DSP_PutNoRndQPel8MC31 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC31 */

static void DSP_PutQPel8MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_UV, 16 );

}  /* end DSP_PutQPel8MC02 */

static void DSP_AvgQPel8MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_UV, 16 );

}  /* end DSP_AvgQPel8MC02 */

static void DSP_PutNoRndQPel8MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_UV, 16 );

}  /* end DSP_PutNoRndQPel8MC02 */

static void DSP_PutQPel8MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2  ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutQPel8MC12 */

static void DSP_AvgQPel8MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_AvgQPel8MC12 */

static void DSP_PutNoRndQPel8MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutNoRndQPel8MC12 */

static void DSP_PutQPel8MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutQPel8MC22 */

static void DSP_AvgQPel8MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_AvgQPel8MC22 */

static void DSP_PutNoRndQPel8MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutNoRndQPel8MC22 */

static void DSP_PutQPel8MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutQPel8MC32 */

static void DSP_AvgQPel8MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_AvgQPel8MC32 */

static void DSP_PutNoRndQPel8MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2  (s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_UV, 8 );

}  /* end DSP_PutNoRndQPel8MC32 */

static void DSP_PutQPel8MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* ed DSP_PutQPel8MC03 */

static void DSP_AvgQPel8MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _avg_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* end DSP_AvgQPel8MC03 */

static void DSP_PutNoRndQPel8MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_UV, 16, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC03 */

static void DSP_PutQPel8MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC13 */

static void DSP_AvgQPel8MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC13 */

static void DSP_PutNoRndQPel8MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC13 */

static void DSP_PutQPel8MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC23 */

static void DSP_AvgQPel8MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC23 */

static void DSP_PutNoRndQPel8MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC23 */

static void DSP_PutQPel8MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutQPel8MC33 */

static void DSP_AvgQPel8MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_AvgQPel8MC33 */

static void DSP_PutNoRndQPel8MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_UV, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel8MC33 */
/******************************************************************************/
/* QPel 16 routines                                                           */
/******************************************************************************/
static void _put_mpeg4_qpel16_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[  0 ] = lpCM[ (  ( apSrc[  0 ] + apSrc[  1 ] ) * 20 - ( apSrc[  0 ] + apSrc[  2 ] ) * 6 + ( apSrc[  1 ] + apSrc[  3 ] ) * 3 - ( apSrc[  2 ] + apSrc[  4 ] ) + 16  ) >> 5 ];
  apDst[  1 ] = lpCM[ (  ( apSrc[  1 ] + apSrc[  2 ] ) * 20 - ( apSrc[  0 ] + apSrc[  3 ] ) * 6 + ( apSrc[  0 ] + apSrc[  4 ] ) * 3 - ( apSrc[  1 ] + apSrc[  5 ] ) + 16  ) >> 5 ];
  apDst[  2 ] = lpCM[ (  ( apSrc[  2 ] + apSrc[  3 ] ) * 20 - ( apSrc[  1 ] + apSrc[  4 ] ) * 6 + ( apSrc[  0 ] + apSrc[  5 ] ) * 3 - ( apSrc[  0 ] + apSrc[  6 ] ) + 16  ) >> 5 ];
  apDst[  3 ] = lpCM[ (  ( apSrc[  3 ] + apSrc[  4 ] ) * 20 - ( apSrc[  2 ] + apSrc[  5 ] ) * 6 + ( apSrc[  1 ] + apSrc[  6 ] ) * 3 - ( apSrc[  0 ] + apSrc[  7 ] ) + 16  ) >> 5 ];
  apDst[  4 ] = lpCM[ (  ( apSrc[  4 ] + apSrc[  5 ] ) * 20 - ( apSrc[  3 ] + apSrc[  6 ] ) * 6 + ( apSrc[  2 ] + apSrc[  7 ] ) * 3 - ( apSrc[  1 ] + apSrc[  8 ] ) + 16  ) >> 5 ];
  apDst[  5 ] = lpCM[ (  ( apSrc[  5 ] + apSrc[  6 ] ) * 20 - ( apSrc[  4 ] + apSrc[  7 ] ) * 6 + ( apSrc[  3 ] + apSrc[  8 ] ) * 3 - ( apSrc[  2 ] + apSrc[  9 ] ) + 16  ) >> 5 ];
  apDst[  6 ] = lpCM[ (  ( apSrc[  6 ] + apSrc[  7 ] ) * 20 - ( apSrc[  5 ] + apSrc[  8 ] ) * 6 + ( apSrc[  4 ] + apSrc[  9 ] ) * 3 - ( apSrc[  3 ] + apSrc[ 10 ] ) + 16  ) >> 5 ];
  apDst[  7 ] = lpCM[ (  ( apSrc[  7 ] + apSrc[  8 ] ) * 20 - ( apSrc[  6 ] + apSrc[  9 ] ) * 6 + ( apSrc[  5 ] + apSrc[ 10 ] ) * 3 - ( apSrc[  4 ] + apSrc[ 11 ] ) + 16  ) >> 5 ];
  apDst[  8 ] = lpCM[ (  ( apSrc[  8 ] + apSrc[  9 ] ) * 20 - ( apSrc[  7 ] + apSrc[ 10 ] ) * 6 + ( apSrc[  6 ] + apSrc[ 11 ] ) * 3 - ( apSrc[  5 ] + apSrc[ 12 ] ) + 16  ) >> 5 ];
  apDst[  9 ] = lpCM[ (  ( apSrc[  9 ] + apSrc[ 10 ] ) * 20 - ( apSrc[  8 ] + apSrc[ 11 ] ) * 6 + ( apSrc[  7 ] + apSrc[ 12 ] ) * 3 - ( apSrc[  6 ] + apSrc[ 13 ] ) + 16  ) >> 5 ];
  apDst[ 10 ] = lpCM[ (  ( apSrc[ 10 ] + apSrc[ 11 ] ) * 20 - ( apSrc[  9 ] + apSrc[ 12 ] ) * 6 + ( apSrc[  8 ] + apSrc[ 13 ] ) * 3 - ( apSrc[  7 ] + apSrc[ 14 ] ) + 16  ) >> 5 ];
  apDst[ 11 ] = lpCM[ (  ( apSrc[ 11 ] + apSrc[ 12 ] ) * 20 - ( apSrc[ 10 ] + apSrc[ 13 ] ) * 6 + ( apSrc[  9 ] + apSrc[ 14 ] ) * 3 - ( apSrc[  8 ] + apSrc[ 15 ] ) + 16  ) >> 5 ];
  apDst[ 12 ] = lpCM[ (  ( apSrc[ 12 ] + apSrc[ 13 ] ) * 20 - ( apSrc[ 11 ] + apSrc[ 14 ] ) * 6 + ( apSrc[ 10 ] + apSrc[ 15 ] ) * 3 - ( apSrc[  9 ] + apSrc[ 16 ] ) + 16  ) >> 5 ];
  apDst[ 13 ] = lpCM[ (  ( apSrc[ 13 ] + apSrc[ 14 ] ) * 20 - ( apSrc[ 12 ] + apSrc[ 15 ] ) * 6 + ( apSrc[ 11 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 10 ] + apSrc[ 16 ] ) + 16  ) >> 5 ];
  apDst[ 14 ] = lpCM[ (  ( apSrc[ 14 ] + apSrc[ 15 ] ) * 20 - ( apSrc[ 13 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 12 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 11 ] + apSrc[ 15 ] ) + 16  ) >> 5 ];
  apDst[ 15 ] = lpCM[ (  ( apSrc[ 15 ] + apSrc[ 16 ] ) * 20 - ( apSrc[ 14 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 13 ] + apSrc[ 15 ] ) * 3 - ( apSrc[ 12 ] + apSrc[ 14 ] ) + 16  ) >> 5 ];

  apDst += BLOCK_STRIDE_Y;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _put_mpeg4_qpel16_h_lowpass */

static void _avg_mpeg4_qpel16_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[  0 ] = ( apDst[  0 ] + lpCM[ (  ( apSrc[  0 ]+apSrc[  1 ] ) * 20 - ( apSrc[  0 ] + apSrc[  2 ] ) * 6 + ( apSrc[  1 ] + apSrc[  3 ] ) * 3 - ( apSrc[  2 ] + apSrc[  4 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  1 ] = ( apDst[  1 ] + lpCM[ (  ( apSrc[  1 ]+apSrc[  2 ] ) * 20 - ( apSrc[  0 ] + apSrc[  3 ] ) * 6 + ( apSrc[  0 ] + apSrc[  4 ] ) * 3 - ( apSrc[  1 ] + apSrc[  5 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  2 ] = ( apDst[  2 ] + lpCM[ (  ( apSrc[  2 ]+apSrc[  3 ] ) * 20 - ( apSrc[  1 ] + apSrc[  4 ] ) * 6 + ( apSrc[  0 ] + apSrc[  5 ] ) * 3 - ( apSrc[  0 ] + apSrc[  6 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  3 ] = ( apDst[  3 ] + lpCM[ (  ( apSrc[  3 ]+apSrc[  4 ] ) * 20 - ( apSrc[  2 ] + apSrc[  5 ] ) * 6 + ( apSrc[  1 ] + apSrc[  6 ] ) * 3 - ( apSrc[  0 ] + apSrc[  7 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  4 ] = ( apDst[  4 ] + lpCM[ (  ( apSrc[  4 ]+apSrc[  5 ] ) * 20 - ( apSrc[  3 ] + apSrc[  6 ] ) * 6 + ( apSrc[  2 ] + apSrc[  7 ] ) * 3 - ( apSrc[  1 ] + apSrc[  8 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  5 ] = ( apDst[  5 ] + lpCM[ (  ( apSrc[  5 ]+apSrc[  6 ] ) * 20 - ( apSrc[  4 ] + apSrc[  7 ] ) * 6 + ( apSrc[  3 ] + apSrc[  8 ] ) * 3 - ( apSrc[  2 ] + apSrc[  9 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  6 ] = ( apDst[  6 ] + lpCM[ (  ( apSrc[  6 ]+apSrc[  7 ] ) * 20 - ( apSrc[  5 ] + apSrc[  8 ] ) * 6 + ( apSrc[  4 ] + apSrc[  9 ] ) * 3 - ( apSrc[  3 ] + apSrc[ 10 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  7 ] = ( apDst[  7 ] + lpCM[ (  ( apSrc[  7 ]+apSrc[  8 ] ) * 20 - ( apSrc[  6 ] + apSrc[  9 ] ) * 6 + ( apSrc[  5 ] + apSrc[ 10 ] ) * 3 - ( apSrc[  4 ] + apSrc[ 11 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  8 ] = ( apDst[  8 ] + lpCM[ (  ( apSrc[  8 ]+apSrc[  9 ] ) * 20 - ( apSrc[  7 ] + apSrc[ 10 ] ) * 6 + ( apSrc[  6 ] + apSrc[ 11 ] ) * 3 - ( apSrc[  5 ] + apSrc[ 12 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[  9 ] = ( apDst[  9 ] + lpCM[ (  ( apSrc[  9 ]+apSrc[ 10 ] ) * 20 - ( apSrc[  8 ] + apSrc[ 11 ] ) * 6 + ( apSrc[  7 ] + apSrc[ 12 ] ) * 3 - ( apSrc[  6 ] + apSrc[ 13 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 10 ] = ( apDst[ 10 ] + lpCM[ (  ( apSrc[ 10 ]+apSrc[ 11 ] ) * 20 - ( apSrc[  9 ] + apSrc[ 12 ] ) * 6 + ( apSrc[  8 ] + apSrc[ 13 ] ) * 3 - ( apSrc[  7 ] + apSrc[ 14 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 11 ] = ( apDst[ 11 ] + lpCM[ (  ( apSrc[ 11 ]+apSrc[ 12 ] ) * 20 - ( apSrc[ 10 ] + apSrc[ 13 ] ) * 6 + ( apSrc[  9 ] + apSrc[ 14 ] ) * 3 - ( apSrc[  8 ] + apSrc[ 15 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 12 ] = ( apDst[ 12 ] + lpCM[ (  ( apSrc[ 12 ]+apSrc[ 13 ] ) * 20 - ( apSrc[ 11 ] + apSrc[ 14 ] ) * 6 + ( apSrc[ 10 ] + apSrc[ 15 ] ) * 3 - ( apSrc[  9 ] + apSrc[ 16 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 13 ] = ( apDst[ 13 ] + lpCM[ (  ( apSrc[ 13 ]+apSrc[ 14 ] ) * 20 - ( apSrc[ 12 ] + apSrc[ 15 ] ) * 6 + ( apSrc[ 11 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 10 ] + apSrc[ 16 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 14 ] = ( apDst[ 14 ] + lpCM[ (  ( apSrc[ 14 ]+apSrc[ 15 ] ) * 20 - ( apSrc[ 13 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 12 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 11 ] + apSrc[ 15 ] ) + 16 ) >> 5 ] + 1  ) >> 1;
  apDst[ 15 ] = ( apDst[ 15 ] + lpCM[ (  ( apSrc[ 15 ]+apSrc[ 16 ] ) * 20 - ( apSrc[ 14 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 13 ] + apSrc[ 15 ] ) * 3 - ( apSrc[ 12 ] + apSrc[ 14 ] ) + 16 ) >> 5 ] + 1  ) >> 1;

  apDst += BLOCK_STRIDE_Y;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _avg_mpeg4_qpel16_h_lowpass */

static void _put_no_rnd_mpeg4_qpel16_h_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride, int aH ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < aH; ++i ) {

  apDst[  0 ] = lpCM[ (  ( apSrc[  0 ] + apSrc[  1 ] ) * 20 - ( apSrc[  0 ] + apSrc[  2 ] ) * 6 + ( apSrc[  1 ] + apSrc[  3 ] ) * 3 - ( apSrc[  2 ] + apSrc[  4 ] ) + 15 ) >> 5 ];
  apDst[  1 ] = lpCM[ (  ( apSrc[  1 ] + apSrc[  2 ] ) * 20 - ( apSrc[  0 ] + apSrc[  3 ] ) * 6 + ( apSrc[  0 ] + apSrc[  4 ] ) * 3 - ( apSrc[  1 ] + apSrc[  5 ] ) + 15 ) >> 5 ];
  apDst[  2 ] = lpCM[ (  ( apSrc[  2 ] + apSrc[  3 ] ) * 20 - ( apSrc[  1 ] + apSrc[  4 ] ) * 6 + ( apSrc[  0 ] + apSrc[  5 ] ) * 3 - ( apSrc[  0 ] + apSrc[  6 ] ) + 15 ) >> 5 ];
  apDst[  3 ] = lpCM[ (  ( apSrc[  3 ] + apSrc[  4 ] ) * 20 - ( apSrc[  2 ] + apSrc[  5 ] ) * 6 + ( apSrc[  1 ] + apSrc[  6 ] ) * 3 - ( apSrc[  0 ] + apSrc[  7 ] ) + 15 ) >> 5 ];
  apDst[  4 ] = lpCM[ (  ( apSrc[  4 ] + apSrc[  5 ] ) * 20 - ( apSrc[  3 ] + apSrc[  6 ] ) * 6 + ( apSrc[  2 ] + apSrc[  7 ] ) * 3 - ( apSrc[  1 ] + apSrc[  8 ] ) + 15 ) >> 5 ];
  apDst[  5 ] = lpCM[ (  ( apSrc[  5 ] + apSrc[  6 ] ) * 20 - ( apSrc[  4 ] + apSrc[  7 ] ) * 6 + ( apSrc[  3 ] + apSrc[  8 ] ) * 3 - ( apSrc[  2 ] + apSrc[  9 ] ) + 15 ) >> 5 ];
  apDst[  6 ] = lpCM[ (  ( apSrc[  6 ] + apSrc[  7 ] ) * 20 - ( apSrc[  5 ] + apSrc[  8 ] ) * 6 + ( apSrc[  4 ] + apSrc[  9 ] ) * 3 - ( apSrc[  3 ] + apSrc[ 10 ] ) + 15 ) >> 5 ];
  apDst[  7 ] = lpCM[ (  ( apSrc[  7 ] + apSrc[  8 ] ) * 20 - ( apSrc[  6 ] + apSrc[  9 ] ) * 6 + ( apSrc[  5 ] + apSrc[ 10 ] ) * 3 - ( apSrc[  4 ] + apSrc[ 11 ] ) + 15 ) >> 5 ];
  apDst[  8 ] = lpCM[ (  ( apSrc[  8 ] + apSrc[  9 ] ) * 20 - ( apSrc[  7 ] + apSrc[ 10 ] ) * 6 + ( apSrc[  6 ] + apSrc[ 11 ] ) * 3 - ( apSrc[  5 ] + apSrc[ 12 ] ) + 15 ) >> 5 ];
  apDst[  9 ] = lpCM[ (  ( apSrc[  9 ] + apSrc[ 10 ] ) * 20 - ( apSrc[  8 ] + apSrc[ 11 ] ) * 6 + ( apSrc[  7 ] + apSrc[ 12 ] ) * 3 - ( apSrc[  6 ] + apSrc[ 13 ] ) + 15 ) >> 5 ];
  apDst[ 10 ] = lpCM[ (  ( apSrc[ 10 ] + apSrc[ 11 ] ) * 20 - ( apSrc[  9 ] + apSrc[ 12 ] ) * 6 + ( apSrc[  8 ] + apSrc[ 13 ] ) * 3 - ( apSrc[  7 ] + apSrc[ 14 ] ) + 15 ) >> 5 ];
  apDst[ 11 ] = lpCM[ (  ( apSrc[ 11 ] + apSrc[ 12 ] ) * 20 - ( apSrc[ 10 ] + apSrc[ 13 ] ) * 6 + ( apSrc[  9 ] + apSrc[ 14 ] ) * 3 - ( apSrc[  8 ] + apSrc[ 15 ] ) + 15 ) >> 5 ];
  apDst[ 12 ] = lpCM[ (  ( apSrc[ 12 ] + apSrc[ 13 ] ) * 20 - ( apSrc[ 11 ] + apSrc[ 14 ] ) * 6 + ( apSrc[ 10 ] + apSrc[ 15 ] ) * 3 - ( apSrc[  9 ] + apSrc[ 16 ] ) + 15 ) >> 5 ];
  apDst[ 13 ] = lpCM[ (  ( apSrc[ 13 ] + apSrc[ 14 ] ) * 20 - ( apSrc[ 12 ] + apSrc[ 15 ] ) * 6 + ( apSrc[ 11 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 10 ] + apSrc[ 16 ] ) + 15 ) >> 5 ];
  apDst[ 14 ] = lpCM[ (  ( apSrc[ 14 ] + apSrc[ 15 ] ) * 20 - ( apSrc[ 13 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 12 ] + apSrc[ 16 ] ) * 3 - ( apSrc[ 11 ] + apSrc[ 15 ] ) + 15 ) >> 5 ];
  apDst[ 15 ] = lpCM[ (  ( apSrc[ 15 ] + apSrc[ 16 ] ) * 20 - ( apSrc[ 14 ] + apSrc[ 16 ] ) * 6 + ( apSrc[ 13 ] + apSrc[ 15 ] ) * 3 - ( apSrc[ 12 ] + apSrc[ 14 ] ) + 15 ) >> 5 ];

  apDst += BLOCK_STRIDE_Y;
  apSrc += aSrcStride;

 }  /* end for */

}  /* end _put_no_rnd_mpeg4_qpel16_h_lowpass */

static void _put_mpeg4_qpel16_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 16; ++i ) {

  const int lSrc0  = apSrc[  0 * aSrcStride ];
  const int lSrc1  = apSrc[  1 * aSrcStride ];
  const int lSrc2  = apSrc[  2 * aSrcStride ];
  const int lSrc3  = apSrc[  3 * aSrcStride ];
  const int lSrc4  = apSrc[  4 * aSrcStride ];
  const int lSrc5  = apSrc[  5 * aSrcStride ];
  const int lSrc6  = apSrc[  6 * aSrcStride ];
  const int lSrc7  = apSrc[  7 * aSrcStride ];
  const int lSrc8  = apSrc[  8 * aSrcStride ];
  const int lSrc9  = apSrc[  9 * aSrcStride ];
  const int lSrc10 = apSrc[ 10 * aSrcStride ];
  const int lSrc11 = apSrc[ 11 * aSrcStride ];
  const int lSrc12 = apSrc[ 12 * aSrcStride ];
  const int lSrc13 = apSrc[ 13 * aSrcStride ];
  const int lSrc14 = apSrc[ 14 * aSrcStride ];
  const int lSrc15 = apSrc[ 15 * aSrcStride ];
  const int lSrc16 = apSrc[ 16 * aSrcStride ];

  apDst[  0 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc0  + lSrc1  ) * 20 - ( lSrc0  + lSrc2  ) * 6 + ( lSrc1  + lSrc3  ) * 3 - ( lSrc2  + lSrc4  ) + 16  ) >> 5 ];
  apDst[  1 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc1  + lSrc2  ) * 20 - ( lSrc0  + lSrc3  ) * 6 + ( lSrc0  + lSrc4  ) * 3 - ( lSrc1  + lSrc5  ) + 16  ) >> 5 ];
  apDst[  2 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc2  + lSrc3  ) * 20 - ( lSrc1  + lSrc4  ) * 6 + ( lSrc0  + lSrc5  ) * 3 - ( lSrc0  + lSrc6  ) + 16  ) >> 5 ];
  apDst[  3 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc3  + lSrc4  ) * 20 - ( lSrc2  + lSrc5  ) * 6 + ( lSrc1  + lSrc6  ) * 3 - ( lSrc0  + lSrc7  ) + 16  ) >> 5 ];
  apDst[  4 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc4  + lSrc5  ) * 20 - ( lSrc3  + lSrc6  ) * 6 + ( lSrc2  + lSrc7  ) * 3 - ( lSrc1  + lSrc8  ) + 16  ) >> 5 ];
  apDst[  5 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc5  + lSrc6  ) * 20 - ( lSrc4  + lSrc7  ) * 6 + ( lSrc3  + lSrc8  ) * 3 - ( lSrc2  + lSrc9  ) + 16  ) >> 5 ];
  apDst[  6 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc6  + lSrc7  ) * 20 - ( lSrc5  + lSrc8  ) * 6 + ( lSrc4  + lSrc9  ) * 3 - ( lSrc3  + lSrc10 ) + 16  ) >> 5 ];
  apDst[  7 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc7  + lSrc8  ) * 20 - ( lSrc6  + lSrc9  ) * 6 + ( lSrc5  + lSrc10 ) * 3 - ( lSrc4  + lSrc11 ) + 16  ) >> 5 ];
  apDst[  8 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc8  + lSrc9  ) * 20 - ( lSrc7  + lSrc10 ) * 6 + ( lSrc6  + lSrc11 ) * 3 - ( lSrc5  + lSrc12 ) + 16  ) >> 5 ];
  apDst[  9 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc9  + lSrc10 ) * 20 - ( lSrc8  + lSrc11 ) * 6 + ( lSrc7  + lSrc12 ) * 3 - ( lSrc6  + lSrc13 ) + 16  ) >> 5 ];
  apDst[ 10 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc10 + lSrc11 ) * 20 - ( lSrc9  + lSrc12 ) * 6 + ( lSrc8  + lSrc13 ) * 3 - ( lSrc7  + lSrc14 ) + 16  ) >> 5 ];
  apDst[ 11 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc11 + lSrc12 ) * 20 - ( lSrc10 + lSrc13 ) * 6 + ( lSrc9  + lSrc14 ) * 3 - ( lSrc8  + lSrc15 ) + 16  ) >> 5 ];
  apDst[ 12 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc12 + lSrc13 ) * 20 - ( lSrc11 + lSrc14 ) * 6 + ( lSrc10 + lSrc15 ) * 3 - ( lSrc9  + lSrc16 ) + 16  ) >> 5 ];
  apDst[ 13 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc13 + lSrc14 ) * 20 - ( lSrc12 + lSrc15 ) * 6 + ( lSrc11 + lSrc16 ) * 3 - ( lSrc10 + lSrc16 ) + 16  ) >> 5 ];
  apDst[ 14 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc14 + lSrc15 ) * 20 - ( lSrc13 + lSrc16 ) * 6 + ( lSrc12 + lSrc16 ) * 3 - ( lSrc11 + lSrc15 ) + 16  ) >> 5 ];
  apDst[ 15 * BLOCK_STRIDE_Y ] = lpCM[  ( ( lSrc15 + lSrc16 ) * 20 - ( lSrc14 + lSrc16 ) * 6 + ( lSrc13 + lSrc15 ) * 3 - ( lSrc12 + lSrc14 ) + 16  ) >> 5 ];

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _put_mpeg4_qpel16_v_lowpass */

static void _avg_mpeg4_qpel16_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 16; ++i ) {

  const int lSrc0  = apSrc[  0 * aSrcStride ];
  const int lSrc1  = apSrc[  1 * aSrcStride ];
  const int lSrc2  = apSrc[  2 * aSrcStride ];
  const int lSrc3  = apSrc[  3 * aSrcStride ];
  const int lSrc4  = apSrc[  4 * aSrcStride ];
  const int lSrc5  = apSrc[  5 * aSrcStride ];
  const int lSrc6  = apSrc[  6 * aSrcStride ];
  const int lSrc7  = apSrc[  7 * aSrcStride ];
  const int lSrc8  = apSrc[  8 * aSrcStride ];
  const int lSrc9  = apSrc[  9 * aSrcStride ];
  const int lSrc10 = apSrc[ 10 * aSrcStride ];
  const int lSrc11 = apSrc[ 11 * aSrcStride ];
  const int lSrc12 = apSrc[ 12 * aSrcStride ];
  const int lSrc13 = apSrc[ 13 * aSrcStride ];
  const int lSrc14 = apSrc[ 14 * aSrcStride ];
  const int lSrc15 = apSrc[ 15 * aSrcStride ];
  const int lSrc16 = apSrc[ 16 * aSrcStride ];

  apDst[  0 * BLOCK_STRIDE_Y ] = ( apDst[  0 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc0  + lSrc1  ) * 20 - ( lSrc0  + lSrc2  ) * 6 + ( lSrc1  + lSrc3  ) * 3 - ( lSrc2  + lSrc4  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  1 * BLOCK_STRIDE_Y ] = ( apDst[  1 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc1  + lSrc2  ) * 20 - ( lSrc0  + lSrc3  ) * 6 + ( lSrc0  + lSrc4  ) * 3 - ( lSrc1  + lSrc5  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  2 * BLOCK_STRIDE_Y ] = ( apDst[  2 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc2  + lSrc3  ) * 20 - ( lSrc1  + lSrc4  ) * 6 + ( lSrc0  + lSrc5  ) * 3 - ( lSrc0  + lSrc6  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  3 * BLOCK_STRIDE_Y ] = ( apDst[  3 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc3  + lSrc4  ) * 20 - ( lSrc2  + lSrc5  ) * 6 + ( lSrc1  + lSrc6  ) * 3 - ( lSrc0  + lSrc7  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  4 * BLOCK_STRIDE_Y ] = ( apDst[  4 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc4  + lSrc5  ) * 20 - ( lSrc3  + lSrc6  ) * 6 + ( lSrc2  + lSrc7  ) * 3 - ( lSrc1  + lSrc8  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  5 * BLOCK_STRIDE_Y ] = ( apDst[  5 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc5  + lSrc6  ) * 20 - ( lSrc4  + lSrc7  ) * 6 + ( lSrc3  + lSrc8  ) * 3 - ( lSrc2  + lSrc9  ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  6 * BLOCK_STRIDE_Y ] = ( apDst[  6 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc6  + lSrc7  ) * 20 - ( lSrc5  + lSrc8  ) * 6 + ( lSrc4  + lSrc9  ) * 3 - ( lSrc3  + lSrc10 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  7 * BLOCK_STRIDE_Y ] = ( apDst[  7 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc7  + lSrc8  ) * 20 - ( lSrc6  + lSrc9  ) * 6 + ( lSrc5  + lSrc10 ) * 3 - ( lSrc4  + lSrc11 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  8 * BLOCK_STRIDE_Y ] = ( apDst[  8 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc8  + lSrc9  ) * 20 - ( lSrc7  + lSrc10 ) * 6 + ( lSrc6  + lSrc11 ) * 3 - ( lSrc5  + lSrc12 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[  9 * BLOCK_STRIDE_Y ] = ( apDst[  9 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc9  + lSrc10 ) * 20 - ( lSrc8  + lSrc11 ) * 6 + ( lSrc7  + lSrc12 ) * 3 - ( lSrc6  + lSrc13 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 10 * BLOCK_STRIDE_Y ] = ( apDst[ 10 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc10 + lSrc11 ) * 20 - ( lSrc9  + lSrc12 ) * 6 + ( lSrc8  + lSrc13 ) * 3 - ( lSrc7  + lSrc14 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 11 * BLOCK_STRIDE_Y ] = ( apDst[ 11 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc11 + lSrc12 ) * 20 - ( lSrc10 + lSrc13 ) * 6 + ( lSrc9  + lSrc14 ) * 3 - ( lSrc8  + lSrc15 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 12 * BLOCK_STRIDE_Y ] = ( apDst[ 12 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc12 + lSrc13 ) * 20 - ( lSrc11 + lSrc14 ) * 6 + ( lSrc10 + lSrc15 ) * 3 - ( lSrc9  + lSrc16 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 13 * BLOCK_STRIDE_Y ] = ( apDst[ 13 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc13 + lSrc14 ) * 20 - ( lSrc12 + lSrc15 ) * 6 + ( lSrc11 + lSrc16 ) * 3 - ( lSrc10 + lSrc16 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 14 * BLOCK_STRIDE_Y ] = ( apDst[ 14 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc14 + lSrc15 ) * 20 - ( lSrc13 + lSrc16 ) * 6 + ( lSrc12 + lSrc16 ) * 3 - ( lSrc11 + lSrc15 ) + 16  ) >> 5 ] + 1 ) >> 1;
  apDst[ 15 * BLOCK_STRIDE_Y ] = ( apDst[ 15 * BLOCK_STRIDE_Y ] + lpCM[ (  ( lSrc15 + lSrc16 ) * 20 - ( lSrc14 + lSrc16 ) * 6 + ( lSrc13 + lSrc15 ) * 3 - ( lSrc12 + lSrc14 ) + 16  ) >> 5 ] + 1 ) >> 1;

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _avg_mpeg4_qpel16_v_lowpass */

static void _put_no_rnd_mpeg4_qpel16_v_lowpass ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride, int aSrcStride ) {

 int      i;
 uint8_t* lpCM = s_CropTbl + MAX_NEG_CROP;

 for ( i = 0; i < 16; ++i ) {

  const int lSrc0  = apSrc[  0 * aSrcStride ];
  const int lSrc1  = apSrc[  1 * aSrcStride ];
  const int lSrc2  = apSrc[  2 * aSrcStride ];
  const int lSrc3  = apSrc[  3 * aSrcStride ];
  const int lSrc4  = apSrc[  4 * aSrcStride ];
  const int lSrc5  = apSrc[  5 * aSrcStride ];
  const int lSrc6  = apSrc[  6 * aSrcStride ];
  const int lSrc7  = apSrc[  7 * aSrcStride ];
  const int lSrc8  = apSrc[  8 * aSrcStride ];
  const int lSrc9  = apSrc[  9 * aSrcStride ];
  const int lSrc10 = apSrc[ 10 * aSrcStride ];
  const int lSrc11 = apSrc[ 11 * aSrcStride ];
  const int lSrc12 = apSrc[ 12 * aSrcStride ];
  const int lSrc13 = apSrc[ 13 * aSrcStride ];
  const int lSrc14 = apSrc[ 14 * aSrcStride ];
  const int lSrc15 = apSrc[ 15 * aSrcStride ];
  const int lSrc16 = apSrc[ 16 * aSrcStride ];

  apDst[  0 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc0  + lSrc1  ) * 20 - ( lSrc0  + lSrc2  ) * 6 + ( lSrc1  + lSrc3  ) * 3 - ( lSrc2  + lSrc4  ) + 15  ) >> 5 ];
  apDst[  1 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc1  + lSrc2  ) * 20 - ( lSrc0  + lSrc3  ) * 6 + ( lSrc0  + lSrc4  ) * 3 - ( lSrc1  + lSrc5  ) + 15  ) >> 5 ];
  apDst[  2 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc2  + lSrc3  ) * 20 - ( lSrc1  + lSrc4  ) * 6 + ( lSrc0  + lSrc5  ) * 3 - ( lSrc0  + lSrc6  ) + 15  ) >> 5 ];
  apDst[  3 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc3  + lSrc4  ) * 20 - ( lSrc2  + lSrc5  ) * 6 + ( lSrc1  + lSrc6  ) * 3 - ( lSrc0  + lSrc7  ) + 15  ) >> 5 ];
  apDst[  4 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc4  + lSrc5  ) * 20 - ( lSrc3  + lSrc6  ) * 6 + ( lSrc2  + lSrc7  ) * 3 - ( lSrc1  + lSrc8  ) + 15  ) >> 5 ];
  apDst[  5 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc5  + lSrc6  ) * 20 - ( lSrc4  + lSrc7  ) * 6 + ( lSrc3  + lSrc8  ) * 3 - ( lSrc2  + lSrc9  ) + 15  ) >> 5 ];
  apDst[  6 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc6  + lSrc7  ) * 20 - ( lSrc5  + lSrc8  ) * 6 + ( lSrc4  + lSrc9  ) * 3 - ( lSrc3  + lSrc10 ) + 15  ) >> 5 ];
  apDst[  7 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc7  + lSrc8  ) * 20 - ( lSrc6  + lSrc9  ) * 6 + ( lSrc5  + lSrc10 ) * 3 - ( lSrc4  + lSrc11 ) + 15  ) >> 5 ];
  apDst[  8 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc8  + lSrc9  ) * 20 - ( lSrc7  + lSrc10 ) * 6 + ( lSrc6  + lSrc11 ) * 3 - ( lSrc5  + lSrc12 ) + 15  ) >> 5 ];
  apDst[  9 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc9  + lSrc10 ) * 20 - ( lSrc8  + lSrc11 ) * 6 + ( lSrc7  + lSrc12 ) * 3 - ( lSrc6  + lSrc13 ) + 15  ) >> 5 ];
  apDst[ 10 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc10 + lSrc11 ) * 20 - ( lSrc9  + lSrc12 ) * 6 + ( lSrc8  + lSrc13 ) * 3 - ( lSrc7  + lSrc14 ) + 15  ) >> 5 ];
  apDst[ 11 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc11 + lSrc12 ) * 20 - ( lSrc10 + lSrc13 ) * 6 + ( lSrc9  + lSrc14 ) * 3 - ( lSrc8  + lSrc15 ) + 15  ) >> 5 ];
  apDst[ 12 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc12 + lSrc13 ) * 20 - ( lSrc11 + lSrc14 ) * 6 + ( lSrc10 + lSrc15 ) * 3 - ( lSrc9  + lSrc16 ) + 15  ) >> 5 ];
  apDst[ 13 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc13 + lSrc14 ) * 20 - ( lSrc12 + lSrc15 ) * 6 + ( lSrc11 + lSrc16 ) * 3 - ( lSrc10 + lSrc16 ) + 15  ) >> 5 ];
  apDst[ 14 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc14 + lSrc15 ) * 20 - ( lSrc13 + lSrc16 ) * 6 + ( lSrc12 + lSrc16 ) * 3 - ( lSrc11 + lSrc15 ) + 15  ) >> 5 ];
  apDst[ 15 * BLOCK_STRIDE_Y ] = lpCM[ (  ( lSrc15 + lSrc16 ) * 20 - ( lSrc14 + lSrc16 ) * 6 + ( lSrc13 + lSrc15 ) * 3 - ( lSrc12 + lSrc14 ) + 15  ) >> 5 ];

  ++apDst;
  ++apSrc;

 }  /* end for */

}  /* end _put_no_rnd_mpeg4_qpel16_v_lowpass */

static void DSP_PutQPel16MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_PutPixels16 ( apDst, apSrc, aStride, 16 );

}  /* end DSP_PutQPel16MC00 */

static void DSP_AvgQPel16MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_AvgPixels16 ( apDst, apSrc, aStride, 16 );

}  /* end DSP_PutQPel16MC00 */

static void DSP_PutNoRndQPel16MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_PutNoRndPixels16 ( apDst, apSrc, aStride, 16 );

}  /* end DSP_PutNoRndQPel16MC00 */

static void DSP_PutQPel16MC10 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _put_pixels16_l2 ( apDst, apSrc, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_PutQPel16MC10 */

static void DSP_PutNoRndQPel16MC10 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _put_no_rnd_pixels16_l2 ( apDst, apSrc, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC10 */

static void DSP_AvgQPel16MC10 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _avg_pixels16_l2 ( apDst, apSrc, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_AvgQPel16MC10 */

static void DSP_PutQPel16MC20 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( apDst, apSrc, aStride, aStride, 16 );

}  /* end DSP_PutQPel16MC20 */

static void DSP_AvgQPel16MC20 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _avg_mpeg4_qpel16_h_lowpass ( apDst, apSrc, aStride, aStride, 16 );

}  /* end DSP_AvgQPel16MC20 */

static void DSP_PutNoRndQPel16MC20 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( apDst, apSrc, aStride, aStride, 16 );

}  /* end DSP_PutNoRndQPel16MC20 */

static void DSP_PutQPel16MC30 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _put_pixels16_l2 ( apDst, apSrc + 1, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_PutQPel16MC30 */

static void DSP_AvgQPel16MC30 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _avg_pixels16_l2 ( apDst, apSrc + 1, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_AvgQPel16MC30 */

static void DSP_PutNoRndQPel16MC30 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 16 );
 _put_no_rnd_pixels16_l2 ( apDst, apSrc + 1, s_Half, aStride, aStride, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC30 */

static void DSP_PutQPel16MC01 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _put_pixels16_l2 ( apDst, s_Full, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_PutQPel16MC01 */

static void DSP_AvgQPel16MC01 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _avg_pixels16_l2 ( apDst, s_Full, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_AvgQPel16MC01 */

static void DSP_PutNoRndQPel16MC01 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _put_no_rnd_pixels16_l2 ( apDst, s_Full, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC01 */

static void DSP_PutQPel16MC11 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2            ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC11 */

static void DSP_AvgQPel16MC11 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2            ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC11 */

static void DSP_PutNoRndQPel16MC11 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2            ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC11 */

static void DSP_PutQPel16MC21 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17);
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC21 */

static void DSP_AvgQPel16MC21 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC21 */

static void DSP_PutNoRndQPel16MC21 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC21 */

static void DSP_PutQPel16MC31 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2            ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC31 */

static void DSP_AvgQPel16MC31 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2            ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC31 */

static void DSP_PutNoRndQPel16MC31 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2            ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2            ( apDst, s_Half, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC31 */

static void DSP_PutQPel16MC02 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _put_mpeg4_qpel16_v_lowpass ( apDst, s_Full, aStride, 24 );

}  /* end DSP_PutQPel16MC02 */

static void DSP_AvgQPel16MC02 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _avg_mpeg4_qpel16_v_lowpass ( apDst, s_Full, aStride, 24 );

}  /* end DSP_AvgQPel16MC02 */

static void DSP_PutNoRndQPel16MC02 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( apDst, s_Full, aStride, 24 );

}  /* end DSP_PutNoRndQPel16MC02 */

static void DSP_PutQPel16MC12 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutQPel16MC12 */

static void DSP_AvgQPel16MC12 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _avg_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_AvgQPel16MC12 */

static void DSP_PutNoRndQPel16MC12 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutNoRndQPel16MC12 */

static void DSP_PutQPel16MC22 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17 );
 _put_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutQPel16MC22 */

static void DSP_AvgQPel16MC22 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17 );
 _avg_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_AvgQPel16MC22 */

static void DSP_PutNoRndQPel16MC22 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, aStride, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutNoRndQPel16MC22 */

static void DSP_PutQPel16MC32 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutQPel16MC32 */

static void DSP_AvgQPel16MC32 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _avg_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_AvgQPel16MC32 */

static void DSP_PutNoRndQPel16MC32 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( apDst, s_Half, aStride, 16 );

}  /* end DSP_PutNoRndQPel16MC32 */

static void DSP_PutQPel16MC03 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _put_pixels16_l2 ( apDst, s_Full + 24, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_PutQPel16MC03 */

static void DSP_AvgQPel16MC03 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _avg_pixels16_l2 ( apDst, s_Full + 24, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_AvgQPel16MC03 */

static void DSP_PutNoRndQPel16MC03 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_Half, s_Full, 16, 24 );
 _put_no_rnd_pixels16_l2 ( apDst, s_Full + 24, s_Half, aStride, 24, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC03 */

static void DSP_PutQPel16MC13 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC13 */

static void DSP_AvgQPel16MC13 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC13 */

static void DSP_PutNoRndQPel16MC13 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2 ( s_Half, s_Half, s_Full, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC13 */

static void DSP_PutQPel16MC23 ( uint8_t* apDst, uint8_t* apSrc, int apStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, apStride, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, apStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC23 */

static void DSP_AvgQPel16MC23 ( uint8_t* apDst, uint8_t* apSrc, int apStride ) {

 _put_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, apStride, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, apStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC23 */

static void DSP_PutNoRndQPel16MC23 ( uint8_t* apDst, uint8_t* apSrc, int apStride ) {

 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, apSrc, 16, apStride, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, apStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC23 */

static void DSP_PutQPel16MC33 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutQPel16MC33 */

static void DSP_AvgQPel16MC33 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );

 _put_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _avg_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_AvgQPel16MC33 */

static void DSP_PutNoRndQPel16MC33 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 _copy_block17 ( apSrc, aStride );
 _put_no_rnd_mpeg4_qpel16_h_lowpass ( s_Half, s_Full, 16, 24, 17 );
 _put_no_rnd_pixels16_l2 ( s_Half, s_Half, s_Full + 1, 16, 16, 24, 17 );
 _put_no_rnd_mpeg4_qpel16_v_lowpass ( s_HalfHV, s_Half, 16, 16 );
 _put_no_rnd_pixels16_l2 ( apDst, s_Half + 16, s_HalfHV, aStride, 16, 16, 16 );

}  /* end DSP_PutNoRndQPel16MC33 */
/******************************************************************************/
/* QPel 8_16 routines                                                         */
/******************************************************************************/
static void DSP_PutQPel816MC00 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 int i;

 for ( i = 0; i < 8; ++i ) {

  *(  ( uint32_t* )( apDst     )  ) = SMS_unaligned32 ( apSrc     );
  *(  ( uint32_t* )( apDst + 4 )  ) = SMS_unaligned32 ( apSrc + 4 );
  apSrc += aBlockStride;
  apDst += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_PutQPel816MC00 */

static void DSP_AvgQPel816MC00 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 int i;

 for ( i = 0; i < 8; ++i ) {

  *(  ( uint32_t* )( apDst     )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apDst + 0 )  ), SMS_unaligned32 ( apSrc + 0 )   );
  *(  ( uint32_t* )( apDst + 4 )  ) = _rnd_avg32 (   *(  ( uint32_t* )( apDst + 4 )  ), SMS_unaligned32 ( apSrc + 4 )   );
  apSrc += aBlockStride;
  apDst += BLOCK_STRIDE_Y;

 }  /* end for */

}  /* end DSP_AvgQPel816MC00 */

static void DSP_PutNoRndQPel816MC00 ( uint8_t* apDst, uint8_t* apSrc, int aStride ) {

 DSP_PutQPel816MC00 ( apDst, apSrc, aStride );

}  /* end DSP_PutNoRndQPel816MC00 */

static void DSP_PutQPel816MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_PutQPel816MC10 */

static void DSP_AvgQPel816MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _avg_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_AvgQPel816MC10 */

static void DSP_PutNoRndQPel816MC10 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, apSrc, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC10 */

static void DSP_PutQPel816MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_Y, aBlockStride, 8 );

}  /* end DSP_PutQPel816MC20 */

static void DSP_AvgQPel816MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _avg_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_Y, aBlockStride, 8 );

}  /* end DSP_AvgQPel816MC20 */

static void DSP_PutNoRndQPel816MC20 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( apDst, apSrc, BLOCK_STRIDE_Y, aBlockStride, 8 );

}  /* end DSP_PutNoRndQPel816MC20 */

static void DSP_PutQPel816MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_pixels8_l2 ( apDst, apSrc + 1, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_PutQPel816MC30 */

static void DSP_AvgQPel816MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _avg_pixels8_l2 ( apDst, apSrc + 1, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_AvgQPel816MC30 */

static void DSP_PutNoRndQPel816MC30 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, apSrc + 1, s_Half, BLOCK_STRIDE_Y, aBlockStride, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC30 */

static void DSP_PutQPel816MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* end DSP_PutQPel816MC01 */

static void DSP_AvgQPel816MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _avg_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* end DSP_AvgQPel816MC01 */

static void DSP_PutNoRndQPel816MC01 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Full, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC01 */

static void DSP_PutQPel816MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC11 */

static void DSP_AvgQPel816MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC11 */

static void DSP_PutNoRndQPel816MC11 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC11 */

static void DSP_PutQPel816MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC21 */

static void DSP_AvgQPel816MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC21 */

static void DSP_PutNoRndQPel816MC21 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC21 */

static void DSP_PutQPel816MC31 ( uint8_t* apDst, uint8_t *apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC31 */

static void DSP_AvgQPel816MC31 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC31 */

static void DSP_PutNoRndQPel816MC31 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC31 */

static void DSP_PutQPel816MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_Y, 16 );

}  /* end DSP_PutQPel8MC02 */

static void DSP_AvgQPel816MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_Y, 16 );

}  /* end DSP_AvgQPel816MC02 */

static void DSP_PutNoRndQPel816MC02 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Full, BLOCK_STRIDE_Y, 16 );

}  /* end DSP_PutNoRndQPel816MC02 */

static void DSP_PutQPel816MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2  ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutQPel816MC12 */

static void DSP_AvgQPel816MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_AvgQPel816MC12 */

static void DSP_PutNoRndQPel816MC12 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutNoRndQPel816MC12 */

static void DSP_PutQPel816MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutQPel816MC22 */

static void DSP_AvgQPel816MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_AvgQPel816MC22 */

static void DSP_PutNoRndQPel816MC22 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutNoRndQPel816MC22 */

static void DSP_PutQPel816MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutQPel816MC32 */

static void DSP_AvgQPel816MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _avg_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_AvgQPel816MC32 */

static void DSP_PutNoRndQPel816MC32 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2  ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( apDst, s_Half, BLOCK_STRIDE_Y, 8 );

}  /* end DSP_PutNoRndQPel816MC32 */

static void DSP_PutQPel816MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* ed DSP_PutQPel816MC03 */

static void DSP_AvgQPel816MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _avg_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* end DSP_AvgQPel816MC03 */

static void DSP_PutNoRndQPel816MC03 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_Half, s_Full, 8, 16 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Full + 16, s_Half, BLOCK_STRIDE_Y, 16, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC03 */

static void DSP_PutQPel816MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );
 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC13 */

static void DSP_AvgQPel816MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC13 */

static void DSP_PutNoRndQPel816MC13 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC13 */

static void DSP_PutQPel816MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC23 */

static void DSP_AvgQPel816MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC23 */

static void DSP_PutNoRndQPel816MC23 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, apSrc, 8, aBlockStride, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC23 */

static void DSP_PutQPel816MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutQPel816MC33 */

static void DSP_AvgQPel816MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _avg_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_AvgQPel816MC33 */

static void DSP_PutNoRndQPel816MC33 ( uint8_t* apDst, uint8_t* apSrc, int aBlockStride ) {

 _copy_block9 ( apSrc, aBlockStride );

 _put_no_rnd_mpeg4_qpel8_h_lowpass ( s_Half, s_Full, 8, 16, 9 );
 _put_no_rnd_pixels8_l2 ( s_Half, s_Half, s_Full + 1, 8, 8, 16, 9 );
 _put_no_rnd_mpeg4_qpel8_v_lowpass ( s_HalfHV, s_Half, 8, 8 );
 _put_no_rnd_pixels8_l2 ( apDst, s_Half + 8, s_HalfHV, BLOCK_STRIDE_Y, 8, 8, 8 );

}  /* end DSP_PutNoRndQPel816MC33 */
/******************************************************************************/
/* DSP initialization                                                         */
/******************************************************************************/
void SMS_DSPContextInit ( SMS_DSPContext* apCtx ) {

 int i;

 apCtx -> IDCT_Put       = IDCT_Put;
 apCtx -> IDCT_Add       = IDCT_Add;
 apCtx -> IDCT_ClrBlocks = IDCT_ClrBlocks;

 apCtx -> GMC  = GMC;
 apCtx -> GMC1 = GMC1;

 apCtx -> m_PutPixTab[ 0 ][ 0 ] = DSP_PutPixels16;
 apCtx -> m_PutPixTab[ 0 ][ 1 ] = DSP_PutPixels16X2;
 apCtx -> m_PutPixTab[ 0 ][ 2 ] = DSP_PutPixels16Y2;
 apCtx -> m_PutPixTab[ 0 ][ 3 ] = DSP_PutPixels16XY2;
 apCtx -> m_PutPixTab[ 1 ][ 0 ] = DSP_PutPixels8;
 apCtx -> m_PutPixTab[ 1 ][ 1 ] = DSP_PutPixels8X2;
 apCtx -> m_PutPixTab[ 1 ][ 2 ] = DSP_PutPixels8Y2;
 apCtx -> m_PutPixTab[ 1 ][ 3 ] = DSP_PutPixels8XY2;
 apCtx -> m_PutPixTab[ 2 ][ 0 ] = DSP_PutPixels816;
 apCtx -> m_PutPixTab[ 2 ][ 1 ] = DSP_PutPixels816X2;
 apCtx -> m_PutPixTab[ 2 ][ 2 ] = DSP_PutPixels816Y2;
 apCtx -> m_PutPixTab[ 2 ][ 3 ] = DSP_PutPixels816XY2;

 apCtx -> m_PutNoRndPixTab[ 0 ][ 0 ] = DSP_PutNoRndPixels16;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 1 ] = DSP_PutNoRndPixels16X2;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 2 ] = DSP_PutNoRndPixels16Y2;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 3 ] = DSP_PutNoRndPixels16XY2;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 0 ] = DSP_PutNoRndPixels8;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 1 ] = DSP_PutNoRndPixels8X2;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 2 ] = DSP_PutNoRndPixels8Y2;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 3 ] = DSP_PutNoRndPixels8XY2;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 0 ] = DSP_PutNoRndPixels816;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 1 ] = DSP_PutNoRndPixels816X2;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 2 ] = DSP_PutNoRndPixels816Y2;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 3 ] = DSP_PutNoRndPixels816XY2;

 apCtx -> m_AvgPixTab[ 0 ][ 0 ] = DSP_AvgPixels16;
 apCtx -> m_AvgPixTab[ 0 ][ 1 ] = DSP_AvgPixels16X2;
 apCtx -> m_AvgPixTab[ 0 ][ 2 ] = DSP_AvgPixels16Y2;
 apCtx -> m_AvgPixTab[ 0 ][ 3 ] = DSP_AvgPixels16XY2;
 apCtx -> m_AvgPixTab[ 1 ][ 0 ] = DSP_AvgPixels8;
 apCtx -> m_AvgPixTab[ 1 ][ 1 ] = DSP_AvgPixels8X2;
 apCtx -> m_AvgPixTab[ 1 ][ 2 ] = DSP_AvgPixels8Y2;
 apCtx -> m_AvgPixTab[ 1 ][ 3 ] = DSP_AvgPixels8XY2;
 apCtx -> m_AvgPixTab[ 2 ][ 0 ] = DSP_AvgPixels816;
 apCtx -> m_AvgPixTab[ 2 ][ 1 ] = DSP_AvgPixels816X2;
 apCtx -> m_AvgPixTab[ 2 ][ 2 ] = DSP_AvgPixels816Y2;
 apCtx -> m_AvgPixTab[ 2 ][ 3 ] = DSP_AvgPixels816XY2;

 apCtx -> m_AvgNoRndPixTab[ 0 ][ 0 ] = DSP_AvgNoRndPixels16;
 apCtx -> m_AvgNoRndPixTab[ 0 ][ 1 ] = DSP_AvgNoRndPixels16X2;
 apCtx -> m_AvgNoRndPixTab[ 0 ][ 2 ] = DSP_AvgNoRndPixels16Y2;
 apCtx -> m_AvgNoRndPixTab[ 0 ][ 3 ] = DSP_AvgNoRndPixels16XY2;
 apCtx -> m_AvgNoRndPixTab[ 1 ][ 0 ] = DSP_AvgNoRndPixels8;
 apCtx -> m_AvgNoRndPixTab[ 1 ][ 1 ] = DSP_AvgNoRndPixels8X2;
 apCtx -> m_AvgNoRndPixTab[ 1 ][ 2 ] = DSP_AvgNoRndPixels8Y2;
 apCtx -> m_AvgNoRndPixTab[ 1 ][ 3 ] = DSP_AvgNoRndPixels8XY2;
 apCtx -> m_AvgNoRndPixTab[ 2 ][ 0 ] = DSP_AvgNoRndPixels816;
 apCtx -> m_AvgNoRndPixTab[ 2 ][ 1 ] = DSP_AvgNoRndPixels816X2;
 apCtx -> m_AvgNoRndPixTab[ 2 ][ 2 ] = DSP_AvgNoRndPixels816Y2;
 apCtx -> m_AvgNoRndPixTab[ 2 ][ 3 ] = DSP_AvgNoRndPixels816XY2;

 apCtx -> m_PutQPelPixTab[ 0 ][  0 ] = DSP_PutQPel16MC00;
 apCtx -> m_PutQPelPixTab[ 0 ][  1 ] = DSP_PutQPel16MC10;
 apCtx -> m_PutQPelPixTab[ 0 ][  2 ] = DSP_PutQPel16MC20;
 apCtx -> m_PutQPelPixTab[ 0 ][  3 ] = DSP_PutQPel16MC30;
 apCtx -> m_PutQPelPixTab[ 0 ][  4 ] = DSP_PutQPel16MC01;
 apCtx -> m_PutQPelPixTab[ 0 ][  5 ] = DSP_PutQPel16MC11;
 apCtx -> m_PutQPelPixTab[ 0 ][  6 ] = DSP_PutQPel16MC21;
 apCtx -> m_PutQPelPixTab[ 0 ][  7 ] = DSP_PutQPel16MC31;
 apCtx -> m_PutQPelPixTab[ 0 ][  8 ] = DSP_PutQPel16MC02;
 apCtx -> m_PutQPelPixTab[ 0 ][  9 ] = DSP_PutQPel16MC12;
 apCtx -> m_PutQPelPixTab[ 0 ][ 10 ] = DSP_PutQPel16MC22;
 apCtx -> m_PutQPelPixTab[ 0 ][ 11 ] = DSP_PutQPel16MC32;
 apCtx -> m_PutQPelPixTab[ 0 ][ 12 ] = DSP_PutQPel16MC03;
 apCtx -> m_PutQPelPixTab[ 0 ][ 13 ] = DSP_PutQPel16MC13;
 apCtx -> m_PutQPelPixTab[ 0 ][ 14 ] = DSP_PutQPel16MC23;
 apCtx -> m_PutQPelPixTab[ 0 ][ 15 ] = DSP_PutQPel16MC33;

 apCtx -> m_PutQPelPixTab[ 1 ][  0 ] = DSP_PutQPel8MC00;
 apCtx -> m_PutQPelPixTab[ 1 ][  1 ] = DSP_PutQPel8MC10;
 apCtx -> m_PutQPelPixTab[ 1 ][  2 ] = DSP_PutQPel8MC20;
 apCtx -> m_PutQPelPixTab[ 1 ][  3 ] = DSP_PutQPel8MC30;
 apCtx -> m_PutQPelPixTab[ 1 ][  4 ] = DSP_PutQPel8MC01;
 apCtx -> m_PutQPelPixTab[ 1 ][  5 ] = DSP_PutQPel8MC11;
 apCtx -> m_PutQPelPixTab[ 1 ][  6 ] = DSP_PutQPel8MC21;
 apCtx -> m_PutQPelPixTab[ 1 ][  7 ] = DSP_PutQPel8MC31;
 apCtx -> m_PutQPelPixTab[ 1 ][  8 ] = DSP_PutQPel8MC02;
 apCtx -> m_PutQPelPixTab[ 1 ][  9 ] = DSP_PutQPel8MC12;
 apCtx -> m_PutQPelPixTab[ 1 ][ 10 ] = DSP_PutQPel8MC22;
 apCtx -> m_PutQPelPixTab[ 1 ][ 11 ] = DSP_PutQPel8MC32;
 apCtx -> m_PutQPelPixTab[ 1 ][ 12 ] = DSP_PutQPel8MC03;
 apCtx -> m_PutQPelPixTab[ 1 ][ 13 ] = DSP_PutQPel8MC13;
 apCtx -> m_PutQPelPixTab[ 1 ][ 14 ] = DSP_PutQPel8MC23;
 apCtx -> m_PutQPelPixTab[ 1 ][ 15 ] = DSP_PutQPel8MC33;

 apCtx -> m_PutQPelPixTab[ 2 ][  0 ] = DSP_PutQPel816MC00;
 apCtx -> m_PutQPelPixTab[ 2 ][  1 ] = DSP_PutQPel816MC10;
 apCtx -> m_PutQPelPixTab[ 2 ][  2 ] = DSP_PutQPel816MC20;
 apCtx -> m_PutQPelPixTab[ 2 ][  3 ] = DSP_PutQPel816MC30;
 apCtx -> m_PutQPelPixTab[ 2 ][  4 ] = DSP_PutQPel816MC01;
 apCtx -> m_PutQPelPixTab[ 2 ][  5 ] = DSP_PutQPel816MC11;
 apCtx -> m_PutQPelPixTab[ 2 ][  6 ] = DSP_PutQPel816MC21;
 apCtx -> m_PutQPelPixTab[ 2 ][  7 ] = DSP_PutQPel816MC31;
 apCtx -> m_PutQPelPixTab[ 2 ][  8 ] = DSP_PutQPel816MC02;
 apCtx -> m_PutQPelPixTab[ 2 ][  9 ] = DSP_PutQPel816MC12;
 apCtx -> m_PutQPelPixTab[ 2 ][ 10 ] = DSP_PutQPel816MC22;
 apCtx -> m_PutQPelPixTab[ 2 ][ 11 ] = DSP_PutQPel816MC32;
 apCtx -> m_PutQPelPixTab[ 2 ][ 12 ] = DSP_PutQPel816MC03;
 apCtx -> m_PutQPelPixTab[ 2 ][ 13 ] = DSP_PutQPel816MC13;
 apCtx -> m_PutQPelPixTab[ 2 ][ 14 ] = DSP_PutQPel816MC23;
 apCtx -> m_PutQPelPixTab[ 2 ][ 15 ] = DSP_PutQPel816MC33;

 apCtx -> m_AvgQPelPixTab[ 0 ][  0 ] = DSP_AvgQPel16MC00;
 apCtx -> m_AvgQPelPixTab[ 0 ][  1 ] = DSP_AvgQPel16MC10;
 apCtx -> m_AvgQPelPixTab[ 0 ][  2 ] = DSP_AvgQPel16MC20;
 apCtx -> m_AvgQPelPixTab[ 0 ][  3 ] = DSP_AvgQPel16MC30;
 apCtx -> m_AvgQPelPixTab[ 0 ][  4 ] = DSP_AvgQPel16MC01;
 apCtx -> m_AvgQPelPixTab[ 0 ][  5 ] = DSP_AvgQPel16MC11;
 apCtx -> m_AvgQPelPixTab[ 0 ][  6 ] = DSP_AvgQPel16MC21;
 apCtx -> m_AvgQPelPixTab[ 0 ][  7 ] = DSP_AvgQPel16MC31;
 apCtx -> m_AvgQPelPixTab[ 0 ][  8 ] = DSP_AvgQPel16MC02;
 apCtx -> m_AvgQPelPixTab[ 0 ][  9 ] = DSP_AvgQPel16MC12;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 10 ] = DSP_AvgQPel16MC22;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 11 ] = DSP_AvgQPel16MC32;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 12 ] = DSP_AvgQPel16MC03;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 13 ] = DSP_AvgQPel16MC13;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 14 ] = DSP_AvgQPel16MC23;
 apCtx -> m_AvgQPelPixTab[ 0 ][ 15 ] = DSP_AvgQPel16MC33;

 apCtx -> m_AvgQPelPixTab[ 1 ][  0 ] = DSP_AvgQPel8MC00;
 apCtx -> m_AvgQPelPixTab[ 1 ][  1 ] = DSP_AvgQPel8MC10;
 apCtx -> m_AvgQPelPixTab[ 1 ][  2 ] = DSP_AvgQPel8MC20;
 apCtx -> m_AvgQPelPixTab[ 1 ][  3 ] = DSP_AvgQPel8MC30;
 apCtx -> m_AvgQPelPixTab[ 1 ][  4 ] = DSP_AvgQPel8MC01;
 apCtx -> m_AvgQPelPixTab[ 1 ][  5 ] = DSP_AvgQPel8MC11;
 apCtx -> m_AvgQPelPixTab[ 1 ][  6 ] = DSP_AvgQPel8MC21;
 apCtx -> m_AvgQPelPixTab[ 1 ][  7 ] = DSP_AvgQPel8MC31;
 apCtx -> m_AvgQPelPixTab[ 1 ][  8 ] = DSP_AvgQPel8MC02;
 apCtx -> m_AvgQPelPixTab[ 1 ][  9 ] = DSP_AvgQPel8MC12;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 10 ] = DSP_AvgQPel8MC22;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 11 ] = DSP_AvgQPel8MC32;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 12 ] = DSP_AvgQPel8MC03;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 13 ] = DSP_AvgQPel8MC13;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 14 ] = DSP_AvgQPel8MC23;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 15 ] = DSP_AvgQPel8MC33;

 apCtx -> m_AvgQPelPixTab[ 2 ][  0 ] = DSP_AvgQPel816MC00;
 apCtx -> m_AvgQPelPixTab[ 2 ][  1 ] = DSP_AvgQPel816MC10;
 apCtx -> m_AvgQPelPixTab[ 2 ][  2 ] = DSP_AvgQPel816MC20;
 apCtx -> m_AvgQPelPixTab[ 2 ][  3 ] = DSP_AvgQPel816MC30;
 apCtx -> m_AvgQPelPixTab[ 2 ][  4 ] = DSP_AvgQPel816MC01;
 apCtx -> m_AvgQPelPixTab[ 2 ][  5 ] = DSP_AvgQPel816MC11;
 apCtx -> m_AvgQPelPixTab[ 2 ][  6 ] = DSP_AvgQPel816MC21;
 apCtx -> m_AvgQPelPixTab[ 2 ][  7 ] = DSP_AvgQPel816MC31;
 apCtx -> m_AvgQPelPixTab[ 2 ][  8 ] = DSP_AvgQPel816MC02;
 apCtx -> m_AvgQPelPixTab[ 2 ][  9 ] = DSP_AvgQPel816MC12;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 10 ] = DSP_AvgQPel816MC22;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 11 ] = DSP_AvgQPel816MC32;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 12 ] = DSP_AvgQPel816MC03;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 13 ] = DSP_AvgQPel816MC13;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 14 ] = DSP_AvgQPel816MC23;
 apCtx -> m_AvgQPelPixTab[ 2 ][ 15 ] = DSP_AvgQPel816MC33;

 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  0 ] = DSP_PutNoRndQPel16MC00;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  1 ] = DSP_PutNoRndQPel16MC10;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  2 ] = DSP_PutNoRndQPel16MC20;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  3 ] = DSP_PutNoRndQPel16MC30;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  4 ] = DSP_PutNoRndQPel16MC01;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  5 ] = DSP_PutNoRndQPel16MC11;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  6 ] = DSP_PutNoRndQPel16MC21;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  7 ] = DSP_PutNoRndQPel16MC31;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  8 ] = DSP_PutNoRndQPel16MC02;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  9 ] = DSP_PutNoRndQPel16MC12;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 10 ] = DSP_PutNoRndQPel16MC22;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 11 ] = DSP_PutNoRndQPel16MC32;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 12 ] = DSP_PutNoRndQPel16MC03;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 13 ] = DSP_PutNoRndQPel16MC13;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 14 ] = DSP_PutNoRndQPel16MC23;
 apCtx -> m_PutNoRndQPelPixTab[ 0 ][ 15 ] = DSP_PutNoRndQPel16MC33;

 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  0 ] = DSP_PutNoRndQPel8MC00;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  1 ] = DSP_PutNoRndQPel8MC10;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  2 ] = DSP_PutNoRndQPel8MC20;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  3 ] = DSP_PutNoRndQPel8MC30;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  4 ] = DSP_PutNoRndQPel8MC01;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  5 ] = DSP_PutNoRndQPel8MC11;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  6 ] = DSP_PutNoRndQPel8MC21;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  7 ] = DSP_PutNoRndQPel8MC31;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  8 ] = DSP_PutNoRndQPel8MC02;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  9 ] = DSP_PutNoRndQPel8MC12;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 10 ] = DSP_PutNoRndQPel8MC22;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 11 ] = DSP_PutNoRndQPel8MC32;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 12 ] = DSP_PutNoRndQPel8MC03;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 13 ] = DSP_PutNoRndQPel8MC13;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 14 ] = DSP_PutNoRndQPel8MC23;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 15 ] = DSP_PutNoRndQPel8MC33;

 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  0 ] = DSP_PutNoRndQPel816MC00;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  1 ] = DSP_PutNoRndQPel816MC10;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  2 ] = DSP_PutNoRndQPel816MC20;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  3 ] = DSP_PutNoRndQPel816MC30;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  4 ] = DSP_PutNoRndQPel816MC01;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  5 ] = DSP_PutNoRndQPel816MC11;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  6 ] = DSP_PutNoRndQPel816MC21;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  7 ] = DSP_PutNoRndQPel816MC31;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  8 ] = DSP_PutNoRndQPel816MC02;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][  9 ] = DSP_PutNoRndQPel816MC12;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 10 ] = DSP_PutNoRndQPel816MC22;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 11 ] = DSP_PutNoRndQPel816MC32;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 12 ] = DSP_PutNoRndQPel816MC03;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 13 ] = DSP_PutNoRndQPel816MC13;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 14 ] = DSP_PutNoRndQPel816MC23;
 apCtx -> m_PutNoRndQPelPixTab[ 2 ][ 15 ] = DSP_PutNoRndQPel816MC33;
#ifdef _WIN32
 for ( i = 0; i < 64; ++i ) apCtx -> m_Permutation[ i ] = i;
#else  /* PS2 */
 for ( i = 0; i < 64; ++i ) apCtx -> m_Permutation[ i ] = ( i & 0x38 ) | (  ( i & 6 ) >> 1  ) | (  ( i & 1 ) << 2  );
#endif  /* _WIN32 */
}  /* end SMS_DSPContextInit */
