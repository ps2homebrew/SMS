/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
# Copyright (c) 2002 - 2003 Pascal Massimino <skal@planet-d.net> (GMC routines/XviD project)
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_DSP.h"

#include <string.h>

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

SMS_DSPGMCData g_GMCData;

static const uint32_t s_MTab[ 16 ] = {
 0x00100000, 0x000F0001, 0x000E0002, 0x000D0003,
 0x000C0004, 0x000B0005, 0x000A0006, 0x00090007,
 0x00080008, 0x00070009, 0x0006000A, 0x0005000B,
 0x0004000C, 0x0003000D, 0x0002000E, 0x0001000F
};

void DSP_GMCn_16 ( uint8_t* apDst, const SMS_MacroBlock* apSrc, int aX, int anY, int aRounding, int aStride ) {

 const int lW       = g_GMCData.m_Width;
 const int lH       = g_GMCData.m_Height;
 const int lRHO     = 3 - g_GMCData.m_Accuracy;
 const int lRounder = (   ( 1 << 7 ) - (  aRounding << ( 2 * lRHO )  )  ) << 16;

 const int lDUx = g_GMCData.m_dU[ 0 ];
 const int lDVx = g_GMCData.m_dV[ 0 ];
 const int lDUy = g_GMCData.m_dU[ 1 ];
 const int lDVy = g_GMCData.m_dV[ 1 ];

 int lUo = g_GMCData.m_Uo + 16 * ( lDUy * anY + lDUx * aX );
 int lVo = g_GMCData.m_Vo + 16 * ( lDVy * anY + lDVx * aX );

 int i, j = 16;

 while ( j-- ) {

  int lU = lUo;
  int lV = lVo;

  i    =   16;
  lUo += lDUy;
  lVo += lDVy;

  while ( i-- ) {

   unsigned int          lF0, lF1, lRi = 16, lRj = 16;
   int                   lMBX, lMBY;
   int                   lX,   lY;
   int                   lu = ( lU >> 16 ) << lRHO;
   int                   lv = ( lV >> 16 ) << lRHO;
   int                   lSrc,  lSrcX;
   int                   lSrcY, lSrcXY;
   const SMS_MacroBlock* lpMB;

   lU += lDUx;
   lV += lDVx;

   if ( lu < 0 )
    lu = 0;
   else if ( lu > lW ) lu = lW & ~15;

   if ( lv < 0 )
    lv = 0;
   else if ( lv > lH ) lv = lH & ~15;

   lX  = lu >> 4;
   lY  = lv >> 4;
   lu &= 15;
   lv &= 15;

   lRi  = s_MTab[ lu ];
   lRj  = s_MTab[ lv ];
   lMBX = lX >> 4;
   lMBY = lY >> 4;
   lX  &= 15;
   lY  &= 15;

   lpMB  = apSrc + lMBX;
   lpMB += lMBY * aStride;

   lSrc = lpMB -> m_Y[ lY ][ lX ];

   if ( lX != 15 ) {

    lSrcX = lpMB -> m_Y[ lY ][ lX + 1 ];

    if ( lY != 15 ) {

     lSrcY  = lpMB -> m_Y[ lY + 1 ][ lX + 0 ];
     lSrcXY = lpMB -> m_Y[ lY + 1 ][ lX + 1 ];

    } else {

     lSrcY  = lpMB[ aStride ].m_Y[ 0 ][ lX + 0 ];
     lSrcXY = lpMB[ aStride ].m_Y[ 0 ][ lX + 1 ];

    }  /* end else */

   } else {

    lSrcX = lpMB[ 1 ].m_Y[ lY ][ 0 ];

    if ( lY != 15 ) {

     lSrcY  = lpMB[ 0 ].m_Y[ lY + 1 ][ lX ];
     lSrcXY = lpMB[ 1 ].m_Y[ lY + 1 ][  0 ];

    } else {

     lSrcY  = lpMB[ aStride + 0 ].m_Y[ 0 ][ lX ];
     lSrcXY = lpMB[ aStride + 1 ].m_Y[ 0 ][  0 ];

    }  /* end else */

   }  /* end else */

   lF0  = lSrc  | ( lSrcX  << 16 );
   lF1  = lSrcY | ( lSrcXY << 16 );
   lF0  = ( lRi * lF0 ) >> 16;
   lF1  = ( lRi * lF1 ) & 0x0FFF0000;
   lF0 |= lF1;
   lF0  = ( lRj * lF0 + lRounder ) >> 24;

   *apDst++ = ( uint8_t )lF0;

  }  /* end for */

 }  /* end for */

}  /* end DSP_GMCn_16 */

void DSP_GMCn_8 ( uint8_t* apDstCb, const SMS_MacroBlock* apSrc, int aX, int anY, int aRounding, int aStride ) {

 const int lW       = g_GMCData.m_Width  >> 1;
 const int lH       = g_GMCData.m_Height >> 1;
 const int lRHO     = 3 - g_GMCData.m_Accuracy;
 const int lRounder = (   128 - (  aRounding << ( 2 * lRHO )  )   ) << 16;

 const int lDUx = g_GMCData.m_dU[ 0 ];
 const int lDVx = g_GMCData.m_dV[ 0 ];
 const int lDUy = g_GMCData.m_dU[ 1 ];
 const int lDVy = g_GMCData.m_dV[ 1 ];

 int lUo = g_GMCData.m_UCo + 8 * ( lDUy * anY + lDUx * aX );
 int lVo = g_GMCData.m_VCo + 8 * ( lDVy * anY + lDVx * aX );

 int i, j = 8;

 uint8_t* lpDstCr = apDstCb + 64;

 while ( j-- ) {

  int lU = lUo;
  int lV = lVo;

  i    =    8;
  lUo += lDUy;
  lVo += lDVy;

  while ( i-- ) {

   unsigned int          lF0Cb, lF1Cb, lRi, lRj, lF0Cr, lF1Cr;
   int                   lMBX, lMBY;
   int                   lX,   lY;
   int                   lu,   lv;
   int                   lSrcCb,  lSrcCbX;
   int                   lSrcCbY, lSrcCbXY;
   int                   lSrcCr,  lSrcCrX;
   int                   lSrcCrY, lSrcCrXY;
   const SMS_MacroBlock* lpMB;

   lu = ( lU >> 16 ) << lRHO;
   lv = ( lV >> 16 ) << lRHO;

   lU += lDUx;
   lV += lDVx;

   if ( lu < 0 )
    lu = 0;
   else if ( lu > lW ) lu = lW & ~15;

   if ( lv < 0 )
    lv = 0;
   else if ( lv > lH ) lv = lH & ~15;

   lX  = lu >> 4;
   lY  = lv >> 4;
   lu &= 15;
   lv &= 15;

   lRi  = s_MTab[ lu ];
   lRj  = s_MTab[ lv ];
   lMBX = lX >> 3;
   lMBY = lY >> 3;
   lX  &= 7;
   lY  &= 7;

   lpMB  = apSrc + lMBX;
   lpMB += lMBY * aStride;

   lSrcCb = lpMB -> m_Cb[ lY ][ lX ];
   lSrcCr = lpMB -> m_Cr[ lY ][ lX ];

   if ( lX != 7 ) {

    lSrcCbX = lpMB -> m_Cb[ lY ][ lX + 1 ];
    lSrcCrX = lpMB -> m_Cr[ lY ][ lX + 1 ];

    if ( lY != 7 ) {

     lSrcCbY  = lpMB -> m_Cb[ lY + 1 ][ lX + 0 ];
     lSrcCrY  = lpMB -> m_Cr[ lY + 1 ][ lX + 0 ];
     lSrcCbXY = lpMB -> m_Cb[ lY + 1 ][ lX + 1 ];
     lSrcCrXY = lpMB -> m_Cr[ lY + 1 ][ lX + 1 ];

    } else {

     lSrcCbY  = lpMB[ aStride ].m_Cb[ 0 ][ lX + 0 ];
     lSrcCrY  = lpMB[ aStride ].m_Cr[ 0 ][ lX + 0 ];
     lSrcCbXY = lpMB[ aStride ].m_Cb[ 0 ][ lX + 1 ];
     lSrcCrXY = lpMB[ aStride ].m_Cr[ 0 ][ lX + 1 ];

    }  /* end else */

   } else {

    lSrcCbX = lpMB[ 1 ].m_Cb[ lY ][ 0 ];
    lSrcCrX = lpMB[ 1 ].m_Cr[ lY ][ 0 ];

    if ( lY != 7 ) {

     lSrcCbY  = lpMB[ 0 ].m_Cb[ lY + 1 ][ lX ];
     lSrcCrY  = lpMB[ 0 ].m_Cr[ lY + 1 ][ lY ];
     lSrcCbXY = lpMB[ 1 ].m_Cb[ lY + 1 ][  0 ];
     lSrcCrXY = lpMB[ 1 ].m_Cr[ lY + 1 ][  0 ];

    } else {

     lSrcCbY  = lpMB[ aStride + 0 ].m_Cb[ 0 ][ lX ];
     lSrcCrY  = lpMB[ aStride + 0 ].m_Cr[ 0 ][ lX ];
     lSrcCbXY = lpMB[ aStride + 1 ].m_Cb[ 0 ][  0 ];
     lSrcCrXY = lpMB[ aStride + 1 ].m_Cr[ 0 ][  0 ];

    }  /* end else */

   }  /* end else */

   lF0Cb  = lSrcCb  | ( lSrcCbX  << 16 );
   lF0Cr  = lSrcCr  | ( lSrcCrX  << 16 );
   lF1Cb  = lSrcCbY | ( lSrcCbXY << 16 );
   lF1Cr  = lSrcCrY | ( lSrcCrXY << 16 );
   lF0Cb  = ( lRi * lF0Cb ) >> 16;
   lF0Cr  = ( lRi * lF0Cr ) >> 16;
   lF1Cb  = ( lRi * lF1Cb ) & 0x0FFF0000;
   lF1Cr  = ( lRi * lF1Cr ) & 0x0FFF0000;
   lF0Cb |= lF1Cb;
   lF0Cr |= lF1Cr;
   lF0Cb  = ( lRj * lF0Cb + lRounder ) >> 24;
   lF0Cr  = ( lRj * lF0Cr + lRounder ) >> 24;

   *apDstCb++ = ( uint8_t )lF0Cb;
   *lpDstCr++ = ( uint8_t )lF0Cr;

  }  /* end for */

 }  /* end for */

}  /* end DSP_GMCn_8 */

void IDCT_ClrBlocks ( void ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set volatile\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui      $v1, 0x1001\n\t"
  "lui      $a0, 0x7000\n\t"
  "addiu    $a1, $zero, 6\n\t"
  "1:\n\t"
  "lw       $at, -32768($v1)\n\t"
  "andi     $at, $at, 0x100\n\t"
  "bne      $at, $zero, 1b\n\t"
  "nop\n\t"
  "1:\n\t"
  "sq       $zero, 0x0580($a0)\n\t"
  "sq       $zero, 0x0590($a0)\n\t"
  "sq       $zero, 0x05A0($a0)\n\t"
  "sq       $zero, 0x05B0($a0)\n\t"
  "add      $a1, -1\n\t"
  "sq       $zero, 0x05C0($a0)\n\t"
  "sq       $zero, 0x05D0($a0)\n\t"
  "sq       $zero, 0x05E0($a0)\n\t"
  "addiu    $a0, 128\n\t"
  "bgtz     $a1, 1b\n\t"
  "sq       $zero, 0x0570($a0)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set novolatile\n\t"
  ".set reorder\n\t"
 );

}  /* end IDCT_ClrBlocks */

static unsigned short s_QPelConst[] __attribute__(   (  section( ".data" )  )   ) = {
 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014, 0x0014,
 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010, 0x0010,
 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F, 0x000F,
 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006, 0x0006,
 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF, 0x00FF,
 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001
};

void SMS_DSPContextInit ( SMS_DSPContext* apCtx ) {

 memcpy (  ( void* )0x70000220, s_QPelConst, sizeof ( s_QPelConst )  );

 apCtx -> m_PutPixTab[ 0 ][ 0 ] = DSP_PutPixels16;
 apCtx -> m_PutPixTab[ 0 ][ 1 ] = DSP_PutPixels16X;
 apCtx -> m_PutPixTab[ 0 ][ 2 ] = DSP_PutPixels16Y;
 apCtx -> m_PutPixTab[ 0 ][ 3 ] = DSP_PutPixels16XY;
 apCtx -> m_PutPixTab[ 1 ][ 0 ] = DSP_PutPixels8;
 apCtx -> m_PutPixTab[ 1 ][ 1 ] = DSP_PutPixels8X;
 apCtx -> m_PutPixTab[ 1 ][ 2 ] = DSP_PutPixels8Y;
 apCtx -> m_PutPixTab[ 1 ][ 3 ] = DSP_PutPixels8XY;
 apCtx -> m_PutPixTab[ 2 ][ 0 ] = DSP_PutPixels8_16;
 apCtx -> m_PutPixTab[ 2 ][ 1 ] = DSP_PutPixels8X_16;
 apCtx -> m_PutPixTab[ 2 ][ 2 ] = DSP_PutPixels8Y_16;
 apCtx -> m_PutPixTab[ 2 ][ 3 ] = DSP_PutPixels8XY_16;

 apCtx -> m_PutNoRndPixTab[ 0 ][ 0 ] = DSP_PutPixels16;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 1 ] = DSP_PutNoRndPixels16X;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 2 ] = DSP_PutNoRndPixels16Y;
 apCtx -> m_PutNoRndPixTab[ 0 ][ 3 ] = DSP_PutNoRndPixels16XY;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 0 ] = DSP_PutPixels8;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 1 ] = DSP_PutNoRndPixels8X;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 2 ] = DSP_PutNoRndPixels8Y;
 apCtx -> m_PutNoRndPixTab[ 1 ][ 3 ] = DSP_PutNoRndPixels8XY;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 0 ] = DSP_PutPixels8_16;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 1 ] = DSP_PutNoRndPixels8X_16;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 2 ] = DSP_PutNoRndPixels8Y_16;
 apCtx -> m_PutNoRndPixTab[ 2 ][ 3 ] = DSP_PutNoRndPixels8XY_16;

 apCtx -> m_AvgPixTab[ 0 ][ 0 ] = DSP_AvgPixels16;
 apCtx -> m_AvgPixTab[ 0 ][ 1 ] = DSP_AvgPixels16X;
 apCtx -> m_AvgPixTab[ 0 ][ 2 ] = DSP_AvgPixels16Y;
 apCtx -> m_AvgPixTab[ 0 ][ 3 ] = DSP_AvgPixels16XY;
 apCtx -> m_AvgPixTab[ 1 ][ 0 ] = DSP_AvgPixels8;
 apCtx -> m_AvgPixTab[ 1 ][ 1 ] = DSP_AvgPixels8X;
 apCtx -> m_AvgPixTab[ 1 ][ 2 ] = DSP_AvgPixels8Y;
 apCtx -> m_AvgPixTab[ 1 ][ 3 ] = DSP_AvgPixels8XY;
 apCtx -> m_AvgPixTab[ 2 ][ 0 ] = DSP_AvgPixels8_16;
 apCtx -> m_AvgPixTab[ 2 ][ 1 ] = DSP_AvgPixels8X_16;
 apCtx -> m_AvgPixTab[ 2 ][ 2 ] = DSP_AvgPixels8Y_16;
 apCtx -> m_AvgPixTab[ 2 ][ 3 ] = DSP_AvgPixels8XY_16;

 apCtx -> m_PutQPelPixTab[ 0 ][  0 ] = DSP_PutPixels16;
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

 apCtx -> m_PutNoRndQPelPixTab[ 0 ][  0 ] = DSP_PutPixels16;
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

 apCtx -> m_PutQPelPixTab[ 1 ][  0 ] = DSP_PutPixels8_16;
 apCtx -> m_PutQPelPixTab[ 1 ][  1 ] = DSP_PutQPel816MC10;
 apCtx -> m_PutQPelPixTab[ 1 ][  2 ] = DSP_PutQPel816MC20;
 apCtx -> m_PutQPelPixTab[ 1 ][  3 ] = DSP_PutQPel816MC30;
 apCtx -> m_PutQPelPixTab[ 1 ][  4 ] = DSP_PutQPel816MC01;
 apCtx -> m_PutQPelPixTab[ 1 ][  5 ] = DSP_PutQPel816MC11;
 apCtx -> m_PutQPelPixTab[ 1 ][  6 ] = DSP_PutQPel816MC21;
 apCtx -> m_PutQPelPixTab[ 1 ][  7 ] = DSP_PutQPel816MC31;
 apCtx -> m_PutQPelPixTab[ 1 ][  8 ] = DSP_PutQPel816MC02;
 apCtx -> m_PutQPelPixTab[ 1 ][  9 ] = DSP_PutQPel816MC12;
 apCtx -> m_PutQPelPixTab[ 1 ][ 10 ] = DSP_PutQPel816MC22;
 apCtx -> m_PutQPelPixTab[ 1 ][ 11 ] = DSP_PutQPel816MC32;
 apCtx -> m_PutQPelPixTab[ 1 ][ 12 ] = DSP_PutQPel816MC03;
 apCtx -> m_PutQPelPixTab[ 1 ][ 13 ] = DSP_PutQPel816MC13;
 apCtx -> m_PutQPelPixTab[ 1 ][ 14 ] = DSP_PutQPel816MC23;
 apCtx -> m_PutQPelPixTab[ 1 ][ 15 ] = DSP_PutQPel816MC33;

 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  0 ] = DSP_PutPixels8_16;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  1 ] = DSP_PutNoRndQPel816MC10;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  2 ] = DSP_PutNoRndQPel816MC20;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  3 ] = DSP_PutNoRndQPel816MC30;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  4 ] = DSP_PutNoRndQPel816MC01;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  5 ] = DSP_PutNoRndQPel816MC11;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  6 ] = DSP_PutNoRndQPel816MC21;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  7 ] = DSP_PutNoRndQPel816MC31;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  8 ] = DSP_PutNoRndQPel816MC02;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][  9 ] = DSP_PutNoRndQPel816MC12;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 10 ] = DSP_PutNoRndQPel816MC22;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 11 ] = DSP_PutNoRndQPel816MC32;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 12 ] = DSP_PutNoRndQPel816MC03;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 13 ] = DSP_PutNoRndQPel816MC13;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 14 ] = DSP_PutNoRndQPel816MC23;
 apCtx -> m_PutNoRndQPelPixTab[ 1 ][ 15 ] = DSP_PutNoRndQPel816MC33;

 apCtx -> m_AvgQPelPixTab[ 0 ][  0 ] = DSP_AvgPixels16;
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

 apCtx -> m_AvgQPelPixTab[ 1 ][  0 ] = DSP_AvgPixels8_16;
 apCtx -> m_AvgQPelPixTab[ 1 ][  1 ] = DSP_AvgQPel816MC10;
 apCtx -> m_AvgQPelPixTab[ 1 ][  2 ] = DSP_AvgQPel816MC20;
 apCtx -> m_AvgQPelPixTab[ 1 ][  3 ] = DSP_AvgQPel816MC30;
 apCtx -> m_AvgQPelPixTab[ 1 ][  4 ] = DSP_AvgQPel816MC01;
 apCtx -> m_AvgQPelPixTab[ 1 ][  5 ] = DSP_AvgQPel816MC11;
 apCtx -> m_AvgQPelPixTab[ 1 ][  6 ] = DSP_AvgQPel816MC21;
 apCtx -> m_AvgQPelPixTab[ 1 ][  7 ] = DSP_AvgQPel816MC31;
 apCtx -> m_AvgQPelPixTab[ 1 ][  8 ] = DSP_AvgQPel816MC02;
 apCtx -> m_AvgQPelPixTab[ 1 ][  9 ] = DSP_AvgQPel816MC12;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 10 ] = DSP_AvgQPel816MC22;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 11 ] = DSP_AvgQPel816MC32;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 12 ] = DSP_AvgQPel816MC03;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 13 ] = DSP_AvgQPel816MC13;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 14 ] = DSP_AvgQPel816MC23;
 apCtx -> m_AvgQPelPixTab[ 1 ][ 15 ] = DSP_AvgQPel816MC33;

}  /* end SMS_DSPContextInit */
