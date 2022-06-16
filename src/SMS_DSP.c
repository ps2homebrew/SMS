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
#include <malloc.h>
#include <math.h>

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

unsigned int s_Mask[ 12 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   ) = {
 0x00000000, 0x00000000, 0x80000000, 0x80000000,
 0x00000000, 0x00000000, 0x80000000, 0x00000000,
 0x80000000, 0x00000000, 0x80000000, 0x00000000
};

unsigned int s_SinCosConst[ 12 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   ) = {
 0x3FC90FD8, 0xBE22F987, 0x4B400000, 0x3F000000,
 0x3E800000, 0xC2992661, 0xC2255DE0, 0x42A33457,
 0x421ED7B7, 0x40C90FDA, 0x00000000, 0x00000000
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
  "sq       $zero, 0x0590($a0)\n\t"
  "sq       $zero, 0x05A0($a0)\n\t"
  "sq       $zero, 0x05B0($a0)\n\t"
  "sq       $zero, 0x05C0($a0)\n\t"
  "addiu    $a1, $a1, -1\n\t"
  "sq       $zero, 0x05D0($a0)\n\t"
  "sq       $zero, 0x05E0($a0)\n\t"
  "sq       $zero, 0x05F0($a0)\n\t"
  "addiu    $a0, 128\n\t"
  "bgtz     $a1, 1b\n\t"
  "sq       $zero, 0x0580($a0)\n\t"
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

__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 ".globl DSP_FFT\n\t"
 ".globl DSP_IMDCT\n\t"
 ".globl DSP_VecMULA\n\t"
 ".globl DSP_VecMULR\n\t"
 ".globl DSP_CosSin\n\t"
 "DSP_FFT:\n\t"
 "_ee_fft:\n\t"
 "lui       $a3, %hi( s_Mask )\n\t"
 "addiu     $a3, $a3, %lo( s_Mask )\n\t"
 "lq        $at,  0($a3)\n\t"
 "lq        $v0, 16($a3)\n\t"
 "lw        " ASM_REG_T4 ", 0($a0)\n\t"
 "addiu     $a3, $zero, 1\n\t"
 "mtsah     $zero, -2\n\t"
 "sllv      $a3, $a3, " ASM_REG_T4 "\n\t"
 "srl       $a3, $a3, 2\n\t"
 "addu      $a2, $zero, $a1\n\t"
 "1:\n\t"
 "lq        " ASM_REG_T0 ",  0($a2)\n\t"
 "lq        " ASM_REG_T1 ", 16($a2)\n\t"
 "pcpyud    " ASM_REG_T2 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pcpyld    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pcpyud    " ASM_REG_T3 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "pcpyld    " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "pxor      " ASM_REG_T2 ", " ASM_REG_T2 ", $at\n\t"
 "pxor      " ASM_REG_T3 ", " ASM_REG_T3 ", $at\n\t"
 "qmtc2     " ASM_REG_T1 ", $vf3\n\t"
 "qmtc2     " ASM_REG_T3 ", $vf4\n\t"
 "qmtc2     " ASM_REG_T0 ", $vf1\n\t"
 "qmtc2     " ASM_REG_T2 ", $vf2\n\t"
 "vadd.xyzw $vf3, $vf3, $vf4\n\t"
 "vadd.xyzw $vf1, $vf1, $vf2\n\t"
 "qmfc2     " ASM_REG_T0 ", $vf3\n\t"
 "qfsrv     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "prot3w    " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pxor      " ASM_REG_T1 ", " ASM_REG_T0 ", $v0\n\t"
 "qmtc2     " ASM_REG_T1 ", $vf2\n\t"
 "vadd.xyzw $vf3, $vf1, $vf2\n\t"
 "vsub.xyzw $vf1, $vf1, $vf2\n\t"
 "addiu     $a3, $a3, -1\n\t"
 "addiu     $a2, $a2, 32\n\t"
 "sqc2      $vf3, -32($a2)\n\t"
 "bgtz      $a3, 1b\n\t"
 "sqc2      $vf1, -16($a2)\n\t"
 "addiu     $a2, $zero, 1\n\t"
 "addiu     " ASM_REG_T4 ", " ASM_REG_T4 ", -3\n\t"
 "pcpyud    $at, $at, $at\n\t"
 "sllv      $a2, $a2, " ASM_REG_T4 "\n\t"
 "addiu     $a3, $zero, 4\n\t"
 "lw        $v1, 8($a0)\n\t"       // v1 = cptr (exptab)
 "1:\n\t"
 "addu      " ASM_REG_T0 ", $zero, $a1\n\t"
 "addu      " ASM_REG_T1 ", $zero, $a2\n\t"
 "2:\n\t"
 "sll       " ASM_REG_T7 ", $a3, 3\n\t"
 "sll       " ASM_REG_T8 ", $a3, 4\n\t"
 "addu      " ASM_REG_T2 ", " ASM_REG_T0 ", " ASM_REG_T7 "\n\t"
 "addu      " ASM_REG_T4 ", $v1, " ASM_REG_T8 "\n\t"
 "addu      " ASM_REG_T3 ", " ASM_REG_T2 ", " ASM_REG_T7 "\n\t"
 "3:\n\t"
 "lq        " ASM_REG_T5 ",   -16(" ASM_REG_T3 ")\n\t"
 "lqc2      $vf1, -16(" ASM_REG_T2 ")\n\t"
 "lqc2      $vf4, -32(" ASM_REG_T4 ")\n\t"
 "lqc2      $vf5, -16(" ASM_REG_T4 ")\n\t"
 "pexcw     " ASM_REG_T6 ", " ASM_REG_T5 "\n\t"
 "pextlw    " ASM_REG_T5 ", " ASM_REG_T6 ", " ASM_REG_T6 "\n\t"
 "pextuw    " ASM_REG_T6 ", " ASM_REG_T6 ", " ASM_REG_T6 "\n\t"
 "qmtc2     " ASM_REG_T5 ", $vf2\n\t"
 "qmtc2     " ASM_REG_T6 ", $vf3\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf2, $vf4\n\t"
 "vmadda.xyzw   " ASM_REG_ACC ", $vf3, $vf5\n\t"
 "vmsubw.xyzw   $vf3, $vf1, $vf0\n\t"
 "vmaddw.xyzw   $vf2, $vf1, $vf0\n\t"
 "addiu     " ASM_REG_T2 ", " ASM_REG_T2 ", -16\n\t"
 "addiu     " ASM_REG_T3 ", " ASM_REG_T3 ", -16\n\t"
 "addiu     " ASM_REG_T4 ", " ASM_REG_T4 ", -32\n\t"
 "qmfc2     " ASM_REG_T5 ", $vf3\n\t"
 "sqc2      $vf2, 0(" ASM_REG_T2 ")\n\t"
 "pxor      " ASM_REG_T5 ", " ASM_REG_T5 ", $at\n\t"
 "bne       " ASM_REG_T2 ", " ASM_REG_T0 ", 3b\n\t"
 "sq        " ASM_REG_T5 ", 0(" ASM_REG_T3 ")\n\t"
 "addiu     " ASM_REG_T1 ", " ASM_REG_T1 ", -1\n\t"
 "sll       " ASM_REG_T7 ", $a3, 4\n\t"
 "bgtz      " ASM_REG_T1 ", 2b\n\t"
 "addu      " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T7 "\n\t"
 "srl       $a2, $a2, 1\n\t"
 "addu      $v1, $v1, " ASM_REG_T7 "\n\t"
 "bgtz      $a2, 1b\n\t"
 "sll       $a3, $a3, 1\n\t"
 "jr        $ra\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 "DSP_IMDCT:\n\t"
 "addiu     " ASM_REG_T9 ", $zero, 1\n\t"
 "lw        " ASM_REG_T8 ",  4($a0)\n\t"       // t8 = s -> nbits
 "lw        $a3, 28($a0)\n\t"       // a3 = s -> m_pTmp
 "pcpyld    $ra, $ra, $ra\n\t"
 "pcpyld    $a0, $a0, $a0\n\t"
 "pcpyld    $a1, $a1, $a1\n\t"
 "pcpyld    $a2, $a2, $a2\n\t"
 "pcpyld    $a3, $a3, $a3\n\t"
 "sllv      " ASM_REG_T9 ", " ASM_REG_T9 ", " ASM_REG_T8 "\n\t"      // t9 = n
 "addu      $a1, $zero, $a2\n\t"    // a1 = input
 "srl       $a2, " ASM_REG_T9 ", 1\n\t"        // a2 = n >> 1
 "addiu     $a2, $a2, -4\n\t"       // a2 = ( n >> 1 ) - 4
 "sll       $a2, $a2, 2\n\t"
 "addiu     " ASM_REG_T0 ", $a1, -16\n\t"
 "lui       $at, %hi( s_Mask )\n\t"
 "addu      $a2, $a2, $a1\n\t"
 "lq        $at, %lo( s_Mask )+32($at)\n\t"
 "mtsah     $zero, -2\n\t"
 "lw        " ASM_REG_T1 ",  8($a0)\n\t"
 "lw        " ASM_REG_T2 ", 12($a0)\n\t"
 "lw        " ASM_REG_T6 ", 20($a0)\n\t"
 "1:\n\t"
 "lq        $v0, 0($a2)\n\t"
 "lq        $v1, 0($a1)\n\t"
 "ld        " ASM_REG_T3 ", 0(" ASM_REG_T1 ")\n\t"
 "ld        " ASM_REG_T4 ", 0(" ASM_REG_T2 ")\n\t"
 "addiu     $a2, $a2, -16\n\t"
 "addiu     $a1, $a1,  16\n\t"
 "addiu     " ASM_REG_T1 ", " ASM_REG_T1 ",   8\n\t"
 "addiu     " ASM_REG_T2 ", " ASM_REG_T2 ",   8\n\t"
 "pexcw     $v1, $v1\n\t"
 "prot3w    $v0, $v0\n\t"
 "pextlw    $v1, $v1, $v1\n\t"
 "qfsrv     $v0, $v0, $v0\n\t"
 "pextlw    " ASM_REG_T5 ", " ASM_REG_T4 ", " ASM_REG_T3 "\n\t"
 "pextlw    $v0, $v0, $v0\n\t"
 "pextlw    " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T4 "\n\t"
 "pxor      $v1, $at, $v1\n\t"
 "qmtc2     $v0, $vf1\n\t"
 "qmtc2     " ASM_REG_T5 ", $vf2\n\t"
 "qmtc2     $v1, $vf3\n\t"
 "qmtc2     " ASM_REG_T4 ", $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf2\n\t"
 "vmadd.xyzw    $vf1, $vf3, $vf4\n\t"
 "lhu       $v0, 0(" ASM_REG_T6 ")\n\t"
 "lhu       $v1, 2(" ASM_REG_T6 ")\n\t"
 "addiu     " ASM_REG_T6 ", " ASM_REG_T6 ", 4\n\t"
 "sll       $v0, $v0, 3\n\t"
 "sll       $v1, $v1, 3\n\t"
 "qmfc2     " ASM_REG_T4 ", $vf1\n\t"
 "addu      $v0, $v0, $a3\n\t"
 "addu      $v1, $v1, $a3\n\t"
 "pcpyud    " ASM_REG_T5 ", " ASM_REG_T4 ", " ASM_REG_T4 "\n\t"
 "sd        " ASM_REG_T4 ", 0($v0)\n\t"
 "bne       $a2, " ASM_REG_T0 ", 1b\n\t"
 "sd        " ASM_REG_T5 ", 0($v1)\n\t"
 "addiu     $a0, $a0, 16\n\t"       // a0 = &s -> fft
 "bgezal    $zero, _ee_fft\n\t"
 "pcpyud    $a1, $a3, $a1\n\t"      // a1 = s -> m_pTmp
 "pcpyud    $a0, $a0, $a0\n\t"
 "pcpyud    $a1, $a3, $a1\n\t"
 "srl       $a2, " ASM_REG_T9 ", 2\n\t"
 "lui       $at, %hi( s_Mask )\n\t"
 "sll       $a2, $a2, 3\n\t"
 "lq        $at, %lo( s_Mask )+32($at)\n\t"
 "addu      $a3, $a1, $a2\n\t"
 "lw        " ASM_REG_T0 ",  8($a0)\n\t"
 "lw        " ASM_REG_T1 ", 12($a0)\n\t"
 "1:\n\t"
 "lq        $v0,  0($a1)\n\t"
 "lq        $v1, 16($a1)\n\t"
 "lq        " ASM_REG_T2 ",  0(" ASM_REG_T0 ")\n\t"
 "lq        " ASM_REG_T3 ",  0(" ASM_REG_T1 ")\n\t"
 "pexcw     $v0, $v0\n\t"
 "pexcw     $v1, $v1\n\t"
 "pextlw    " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
 "pextlw    " ASM_REG_T5 ", " ASM_REG_T2 ", " ASM_REG_T3 "\n\t"
 "pextlw    " ASM_REG_T6 ", $v0, $v0\n\t"
 "pextlw    " ASM_REG_T7 ", $v1, $v1\n\t"
 "pextuw    " ASM_REG_T8 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
 "pextuw    " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T3 "\n\t"
 "pextuw    $v0, $v0, $v0\n\t"
 "pextuw    $v1, $v1, $v1\n\t"
 "pxor      $v0, $at, $v0\n\t"
 "pxor      $v1, $at, $v1\n\t"
 "qmtc2     " ASM_REG_T6 ", $vf1\n\t"
 "qmtc2     " ASM_REG_T4 ", $vf2\n\t"
 "qmtc2     $v0, $vf3\n\t"
 "qmtc2     " ASM_REG_T5 ", $vf4\n\t"
 "qmtc2     " ASM_REG_T7 ", $vf5\n\t"
 "qmtc2     " ASM_REG_T8 ", $vf6\n\t"
 "qmtc2     $v1, $vf7\n\t"
 "qmtc2     " ASM_REG_T2 ", $vf8\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf2\n\t"
 "vmadd.xyzw    $vf1, $vf3, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf5, $vf6\n\t"
 "vmadd.xyzw    $vf2, $vf7, $vf8\n\t"
 "addiu     $a1, $a1, 32\n\t"
 "addiu     " ASM_REG_T0 ", " ASM_REG_T0 ", 16\n\t"
 "addiu     " ASM_REG_T1 ", " ASM_REG_T1 ", 16\n\t"
 "sqc2      $vf1, -32($a1)\n\t"
 "bne       $a1, $a3, 1b\n\t"
 "sqc2      $vf2, -16($a1)\n\t"
 "pcpyud    $a0, $a1, $a0\n\t"
 "pcpyud    $a1, $a3, $a1\n\t"
 "addu      $a2, $zero, " ASM_REG_T9 "\n\t"
 "pnor      $at, $zero, $zero\n\t"
 "sll       $v0, $a2, 2\n\t"
 "sll       $v1, $a2, 1\n\t"
 "addu      $a3, $a1, $a2\n\t"
 "addu      " ASM_REG_T0 ", $a0, $a2\n\t"
 "addu      " ASM_REG_T1 ", $a0, $v1\n\t"
 "addu      $a3, $a3, $a2\n\t"
 "addu      " ASM_REG_T1 ", " ASM_REG_T1 ", $a2\n\t"
 "addu      " ASM_REG_T2 ", $a0, $v1\n\t"
 "addu      $a0, $a0, $v0\n\t"
 "subu      " ASM_REG_T2 ", " ASM_REG_T2 ", $a2\n\t"
 "subu      $a0, $a0, $a2\n\t"
 "psllw     $at, $at, 31\n\t"
 "1:\n\t"
 "lq        $v0,   0($a1)\n\t"
 "lq        $v1, -16($a3)\n\t"
 "addiu     " ASM_REG_T2 ", " ASM_REG_T2 ",  16\n\t"
 "addiu     $a0, $a0,  16\n\t"
 "addiu     $a1, $a1,  16\n\t"
 "addiu     $a3, $a3, -16\n\t"
 "addiu     " ASM_REG_T0 ", " ASM_REG_T0 ", -16\n\t"
 "addiu     " ASM_REG_T1 ", " ASM_REG_T1 ", -16\n\t"
 "mtsah     $zero, 2\n\t"
 "pxor      $v0, $v0, $at\n\t"
 "pexcw     $v0, $v0\n\t"
 "prot3w    $v1, $v1\n\t"
 "qfsrv     $v0, $v0, $v0\n\t"
 "qfsrv     $v1, $v1, $v1\n\t"
 "qfsrv     $v0, $v0, $v0\n\t"
 "pextlw    " ASM_REG_T8 ", $v1, $v0\n\t"
 "pextuw    $v1, $v1, $v0\n\t"
 "pxor      " ASM_REG_T8 ", " ASM_REG_T8 ", $at\n\t"
 "mtsah     $zero, -2\n\t"
 "pexew     $v0, $v1\n\t"
 "pexew     " ASM_REG_T7 ", " ASM_REG_T8 "\n\t"
 "pxor      $v0, $v0, $at\n\t"
 "qfsrv     " ASM_REG_T7 ", " ASM_REG_T7 ", " ASM_REG_T7 "\n\t"
 "qfsrv     $v0, $v0, $v0\n\t"
 "sq        $v1, -16(" ASM_REG_T2 ")\n\t"
 "sq        " ASM_REG_T8 ", -16($a0)\n\t"
 "sq        $v0,   0(" ASM_REG_T0 ")\n\t"
 "bne       $a1, $a3, 1b\n\t"
 "sq        " ASM_REG_T7 ",   0(" ASM_REG_T1 ")\n\t"
 "jr        $ra\n\t"
 "DSP_VecMULA:\n\t"
 "srl       $at, " ASM_REG_T0 ", 3\n\t"
 "vmaxw.xyzw    $vf1, $vf0, $vf0\n\t"
 "beq       $at, $zero, 2f\n\t"
 "andi      " ASM_REG_T0 ", " ASM_REG_T0 ", 7\n\t"
 "1:\n\t"
 "lqc2      $vf2,  0($a1)\n\t"
 "lqc2      $vf3, 16($a1)\n\t"
 "lqc2      $vf4,  0($a2)\n\t"
 "lqc2      $vf5, 16($a2)\n\t"
 "lqc2      $vf6,  0($a3)\n\t"
 "lqc2      $vf7, 16($a3)\n\t"
 "addiu     $at, $at, -1\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf2, $vf4\n\t"
 "vmadd.xyzw    $vf2, $vf6, $vf1\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf3, $vf5\n\t"
 "vmadd.xyzw    $vf3, $vf7, $vf1\n\t"
 "addi      $a0, $a0, 32\n\t"
 "addiu     $a1, $a1, 32\n\t"
 "addiu     $a2, $a2, 32\n\t"
 "addiu     $a3, $a3, 32\n\t"
 "sqc2      $vf2, -32($a0)\n\t"
 "bgtz      $at, 1b\n\t"
 "sqc2      $vf3, -16($a0)\n\t"
 "2:\n\t"
 "beq       " ASM_REG_T0 ", $zero, 3f\n\t"
 "lui       $v0, 0x3F80\n\t"
 "mtc1      $v0, $f0\n\t"
 "4:\n\t"
 "lwc1      $f1, 0($a1)\n\t"
 "lwc1      $f2, 0($a2)\n\t"
 "lwc1      $f3, 0($a3)\n\t"
 "addiu     " ASM_REG_T0 ", " ASM_REG_T0 ", -1\n\t"
 "mula.s    $f1, $f2\n\t"
 "madd.s    $f1, $f3, $f0\n\t"
 "addiu     $a0, $a0, 4\n\t"
 "addiu     $a1, $a1, 4\n\t"
 "addiu     $a2, $a2, 4\n\t"
 "addiu     $a3, $a3, 4\n\t"
 "bgtz      " ASM_REG_T0 ", 4b\n\t"
 "swc1      $f1, -4($a0)\n\t"
 "3:\n\t"
 "jr        $ra\n\t"
 "DSP_VecMULR:\n\t"
 "sll   $a3, $a3, 2\n\t"
 "addu  $at, $a1, $a3\n\t"
 "addu  $a2, $a2, $a3\n\t"
 "mtsah $zero, -2\n\t"
 "1:\n\t"
 "lq    $v0,  -32($a2)\n\t"
 "lq    $v1,  -16($a2)\n\t"
 "lqc2  $vf1,  0($a1)\n\t"
 "lqc2  $vf2, 16($a1)\n\t"
 "pexew $v0, $v0\n\t"
 "pexew $v1, $v1\n\t"
 "qfsrv $v0, $v0, $v0\n\t"
 "qfsrv $v1, $v1, $v1\n\t"
 "qmtc2 $v0, $vf3\n\t"
 "qmtc2 $v1, $vf4\n\t"
 "vmul.xyzw $vf2, $vf2, $vf3\n\t"
 "vmul.xyzw $vf1, $vf1, $vf4\n\t"
 "addiu $a2, $a2, -32\n\t"
 "addiu $a1, $a1,  32\n\t"
 "addiu $a0, $a0,  32\n\t"
 "sqc2  $vf1, -32($a0)\n\t"
 "bne   $a1, $at, 1b\n\t"
 "sqc2  $vf2, -16($a0)\n\t"
 "jr    $ra\n\t"
 "DSP_CosSin:\n\t"
 "lui       $a1, %hi( s_SinCosConst )\n\t"
 "mfc1      " ASM_REG_T0 ", $f12\n\t"
 "mfc1      " ASM_REG_T1 ", $f13\n\t"
 "mtsah     $zero, -2\n\t"
 "addiu     $a1, $a1, %lo( s_SinCosConst )\n\t"
 "lqc2      $vf7,  0($a1)\n\t"
 "lqc2      $vf8, 16($a1)\n\t"
 "lqc2      $vf9, 32($a1)\n\t"
 "pextlw    " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "pextlw    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "qmtc2     " ASM_REG_T0 ", $vf1\n\t"
 "vsubx.xz      $vf1, $vf1, $vf7\n\t"
 "vabs.xyzw     $vf2, $vf1\n\t"
 "vmaxw.xyzw    $vf1, $vf0, $vf0\n\t"
 "vmulay.xyzw   " ASM_REG_ACC ", $vf2, $vf7\n\t"
 "vmsubaz.xyzw  " ASM_REG_ACC ", $vf1, $vf7\n\t"
 "vmaddaz.xyzw  " ASM_REG_ACC ", $vf1, $vf7\n\t"
 "vmsubay.xyzw  " ASM_REG_ACC ", $vf2, $vf7\n\t"
 "vmsubw.xyzw   $vf1, $vf1, $vf7\n\t"
 "vabs.xyzw     $vf1, $vf1\n\t"
 "vsubx.xyzw    $vf1, $vf1, $vf8\n\t"
 "vmul.xyzw     $vf4, $vf1, $vf1\n\t"
 "vmuly.xyzw    $vf6, $vf1, $vf8\n\t"
 "vmulz.xyzw    $vf5, $vf1, $vf8\n\t"
 "vmulw.xyzw    $vf2, $vf1, $vf8\n\t"
 "vmul.xyzw     $vf3, $vf4, $vf4\n\t"
 "vmul.xyzw     $vf6, $vf6, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf5, $vf4\n\t"
 "vmulx.xyzw    $vf4, $vf1, $vf9\n\t"
 "vmul.xyzw     $vf5, $vf3, $vf3\n\t"
 "vmadda.xyzw   " ASM_REG_ACC ", $vf6, $vf3\n\t"
 "vmadda.xyzw   " ASM_REG_ACC ", $vf2, $vf3\n\t"
 "vmadday.xyzw  " ASM_REG_ACC ", $vf1, $vf9\n\t"
 "vmadd.xyzw    $vf1, $vf4, $vf5\n\t"
 "vmuli.xyzw    $vf1, $vf1, " ASM_REG_I "\n\t"
 "qmfc2     $v0, $vf1\n\t"
 "qfsrv     $v0, $v0, $v0\n\t"
 "pexew     $v0, $v0\n\t"
 "jr        $ra\n\t"
 "sq        $v0, 0($a0)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

int DSP_iFFTInit ( SMS_FFTContext *s, int nbits ) {

 int    i, j, l, m, n, o;
 float  lMul;
 float* lpPtr;

 s -> m_nBits   = nbits;
 n              = 1 << nbits;
 m              = n >> 3;
 o              = n >> 1;
 s -> m_pExpTab = memalign (  16, n * 2 * sizeof ( SMS_Complex ) + n * sizeof ( unsigned short )  );
 s -> m_pRevTab = ( unsigned short* )( s -> m_pExpTab + n * 2 );

 if ( !s -> m_pExpTab ) return 0;

 lMul  = 2.0F * ( float )M_PI * 1.0F / n;
 lpPtr = ( float* )s -> m_pExpTab;

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui  $at, 0x3F80\n\t"
  "ctc2 $at, $21\n\t"
  ".set at\n\t"
 );

 do {
  for ( l = 0; l < o; l += 2 * m, lpPtr += 8 ) {
   float lAlp0, lAlp1;
   float lCosSin[ 4 ] __attribute__(   (  aligned( 16 )  )   );
   lAlp0 = lMul * ( float )l;
   lAlp1 = lMul * ( float )( l + m );
   DSP_CosSin ( lAlp0, lAlp1, lCosSin );
   lpPtr[ 0 ] =  lCosSin[ 0 ];
   lpPtr[ 1 ] =  lCosSin[ 1 ];
   lpPtr[ 2 ] =  lCosSin[ 2 ];
   lpPtr[ 3 ] =  lCosSin[ 3 ];
   lpPtr[ 4 ] = -lCosSin[ 1 ];
   lpPtr[ 5 ] =  lCosSin[ 0 ];
   lpPtr[ 6 ] = -lCosSin[ 3 ];
   lpPtr[ 7 ] =  lCosSin[ 2 ];
  }  /* end for */
  m = m >> 1;
 } while ( m );

 for ( i = 0; i < n; ++i ) {
  m = 0;
  for ( j = 0; j < nbits; ++j ) m |= (  ( i >> j ) & 1  ) << ( nbits - j - 1 );
  s -> m_pRevTab[ i ] = m;
 }  /* end for */

 return 1;

}  /* end DSP_iFFTInit */

int DSP_MDCTInit ( SMS_MDCTContext* s, int nbits, float aScale ) {

 int n, n2, n4, i;

 memset (  s, 0, sizeof ( *s )  );

 n            = 1 << nbits;
 s -> m_nBits = nbits;
 s -> m_N     = n;
 n2           = n >> 1;
 n4           = n >> 2;

 s -> m_pCos = ( float* )memalign (  16, n4 * sizeof ( float ) * 2 + n2 * sizeof ( float )  );

 __asm__ __volatile__(
  ".set noat\n\t"
  "mfc1 $at, %0\n\t"
  :: "f"( aScale )
 );

 s -> m_pSin = s -> m_pCos + n4;
 s -> m_pTmp = s -> m_pSin + n4;

 __asm__ __volatile__(
  "ctc2 $at, $21\n\t"
  ".set at\n\t"
 );

 for ( i = 0; i < n4; i += 2 ) {
  float lCosSin[ 4 ] __attribute__(   (  aligned( 16 )  )   );
  float lAlp0 = 2.0F * ( float )M_PI * (  ( i + 0 ) + 1.0F / 8.0F  ) / ( float )n;
  float lAlp1 = 2.0F * ( float )M_PI * (  ( i + 1 ) + 1.0F / 8.0F  ) / ( float )n;
  DSP_CosSin ( lAlp0, lAlp1, lCosSin );
  s -> m_pCos[ i + 0 ] = -lCosSin[ 0 ];
  s -> m_pSin[ i + 0 ] = -lCosSin[ 1 ];
  s -> m_pCos[ i + 1 ] = -lCosSin[ 2 ];
  s -> m_pSin[ i + 1 ] = -lCosSin[ 3 ];
 }  /* end for */

 if (  !DSP_iFFTInit ( &s -> m_FFT, s -> m_nBits - 2 ) < 0  ) {
  DSP_MDCTDestroy ( s );
  return 0;
 }  /* end if */

 return 1;

}  /* end DSP_MDCTInit */

void DSP_FFTDestroy ( SMS_FFTContext* s ) {

 if ( s -> m_pExpTab ) free ( s -> m_pExpTab ), s -> m_pExpTab = NULL;

}  /* end DSP_FFTDestroy */

void DSP_MDCTDestroy ( SMS_MDCTContext* s ) {

 if ( s -> m_pCos ) free ( s -> m_pCos ), s -> m_pCos = NULL;

 DSP_FFTDestroy ( &s -> m_FFT );

}  /* end DSP_MDCTDestroy */

static const unsigned int s_DCT4Const[ 272 ] __attribute__(   (  aligned( 16 ), section( ".rodata" )  )   ) = {
 0x3F7FFB11, 0x3F7F84AB, 0x3F7E70B0, 0x3F7CBFC9,
 0x3F7A7302, 0x3F778BC5, 0x3F740BDD, 0x3F6FF573,
 0x3F6B4B0C, 0x3F660F88, 0x3F604621, 0x3F59F26A,
 0x3F531848, 0x3F4BBBF8, 0x3F43E200, 0x3F3B8F3B,
 0x3F32C8C9, 0x3F299414, 0x3F1FF6CA, 0x3F15F6D9,
 0x3F0B9A6B, 0x3F00E7E5, 0x3EEBCBBB, 0x3ED53641,
 0x3EBE1D48, 0x3EA68F10, 0x3E8E9A1F, 0x3E6C9A81,
 0x3E3B6ECF, 0x3E09CF85, 0x3DAFB67B, 0x3D16C31C,
 0xBF818FA6, 0xBF879BB1, 0xBF8D541B, 0xBF92B55B,
 0xBF97BC21, 0xBF9C6553, 0xBFA0AE10, 0xBFA493B5,
 0xBFA813DB, 0xBFAB2C58, 0xBFADDB43, 0xBFB01EF6,
 0xBFB1F60A, 0xBFB35F5E, 0xBFB45A12, 0xBFB4E58C,
 0xBFB50176, 0xBFB4ADBE, 0xBFB3EA98, 0xBFB2B87C,
 0xBFB11828, 0xBFAF0A9C, 0xBFAC911C, 0xBFA9AD2E,
 0xBFA6609C, 0xBFA2AD6E, 0xBF9E95EB, 0xBF9A1C9C,
 0xBF954440, 0xBF900FD7, 0xBF8A8294, 0xBF849FE5,
 0xBF7CD6D7, 0xBF6FD1F4, 0xBF62392A, 0xBF5414DC,
 0xBF456DC2, 0xBF364CE4, 0xBF26BB9A, 0xBF16C37C,
 0xBF066E62, 0xBEEB8CC1, 0xBEC9AB78, 0xBEA74DCE,
 0xBE8488F6, 0xBE42E4CC, 0xBDF87ED8, 0xBD5535D0,
 0x3C8E2B40, 0x3DB19A98, 0x3E1F9E70, 0x3E660D18,
 0x3E95F6F4, 0x3EB88ADC, 0x3EDAACF9, 0x3EFC4837,
 0x3F0EA3EF, 0x3F1ECBCB, 0x3F2E91B8, 0x3F3DEBF8,
 0x3F4CD119, 0x3F5B37EB, 0x3F691789, 0x3F766766,
 0x3F800000, 0x3F7FB10F, 0x3F7EC46D, 0x3F7D3AAC,
 0x3F7B14BE, 0x3F7853F8, 0x3F74FA0B, 0x3F710908,
 0x3F6C835E, 0x3F676BD8, 0x3F61C597, 0x3F5B941A,
 0x3F54DB31, 0x3F4D9F02, 0x3F45E403, 0x3F3DAEF9,
 0x3F3504F3, 0x3F2BEB49, 0x3F226799, 0x3F187FC0,
 0x3F0E39D9, 0x3F039C3C, 0x3EF15AE7, 0x3EDAE881,
 0x3EC3EF15, 0x3EAC7CD3, 0x3E94A030, 0x3E78CFC8,
 0x3E47C5BC, 0x3E164085, 0x3DC8BD35, 0x3D48FB29,
 0xBF800000, 0xBF862061, 0xBF8BEE0A, 0xBF916566,
 0xBF968317, 0xBF9B43F6, 0xBF9FA512, 0xBFA3A3B9,
 0xBFA73D74, 0xBFAA700C, 0xBFAD3986, 0xBFAF982C,
 0xBFB18A86, 0xBFB30F61, 0xBFB425CE, 0xBFB4CD22,
 0x00000000, 0xBFB4CD22, 0xBFB425CE, 0xBFB30F61,
 0xBFB18A86, 0xBFAF982C, 0xBFAD3986, 0xBFAA700C,
 0xBFA73D74, 0xBFA3A3B9, 0xBF9FA512, 0xBF9B43F5,
 0xBF968317, 0xBF916567, 0xBF8BEE0A, 0xBF862061,
 0xBF800000, 0xBF73215C, 0xBF65ACC6, 0xBF57AA8B,
 0xBF49234E, 0xBF3A2005, 0xBF2AA9F2, 0xBF1ACA9E,
 0xBF0A8BD3, 0xBEF3EF30, 0xBED23044, 0xBEAFEFBA,
 0xBE8D42AE, 0xBE547D08, 0xBE0DF1A4, 0xBD8E1D78,
 0x00000000, 0x3D8E1D88, 0x3E0DF1A8, 0x3E547D08,
 0x3E8D42B2, 0x3EAFEFBE, 0x3ED23049, 0x3EF3EF2F,
 0x3F0A8BD4, 0x3F1ACA9E, 0x3F2AA9F3, 0x3F3A2006,
 0x3F492350, 0x3F57AA8B, 0x3F65ACC6, 0x3F73215C,
 0x3F800000, 0x3F7B14BE, 0x3F6C835E, 0x3F54DB31,
 0x3F3504F3, 0x3F0E39D9, 0x3EC3EF14, 0x3E47C5BF,
 0x00000000, 0xBE47C5C5, 0xBEC3EF17, 0xBF0E39DB,
 0xBF3504F4, 0xBF54DB32, 0xBF6C835F, 0xBF7B14BF,
 0x3F3504F3, 0x3F3504F3, 0x3F3504F4, 0x3F3504F4,
 0xBF3504F3, 0xBF3504F3, 0xBF3504F4, 0xBF3504F4,
 0x3F800000, 0x3F3504F3, 0x3F800000, 0xBF3504F4,
 0x00000000, 0x00000000, 0x80000000, 0x80000000,
 0x00000000, 0xBE47C5C2, 0xBEC3EF16, 0xBF0E39DA,
 0xBF3504F3, 0xBF54DB32, 0xBF6C835F, 0xBF7B14BF,
 0xBF800000, 0xBF7B14BE, 0xBF6C835E, 0xBF54DB31,
 0xBF3504F2, 0xBF0E39D9, 0xBEC3EF13, 0xBE47C5BC,
 0x00000000, 0x00000040, 0x00000020, 0x00000060,
 0x00000010, 0x00000050, 0x00000030, 0x00000070,
 0x00000008, 0x00000048, 0x00000028, 0x00000068,
 0x00000018, 0x00000058, 0x00000038, 0x00000078,
 0x00000004, 0x00000044, 0x00000024, 0x00000064,
 0x00000014, 0x00000054, 0x00000034, 0x00000074,
 0x0000000C, 0x0000004C, 0x0000002C, 0x0000006C,
 0x0000001C, 0x0000005C, 0x0000003C, 0x0000007C
};

void DSP_DCT4Kernel ( float*, float* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 ".globl DSP_DCT4Kernel\n\t"
 "DSP_DCT4Kernel:\n\t"
 "lui       $v0, %hi( s_DCT4Const )\n\t"
 "addiu     $at, $a0, 128\n\t"
 "addiu     $v0, $v0, %lo( s_DCT4Const )\n\t"
 "1:\n\t"
 "lqc2          $vf1,   0($a0)\n\t"
 "lqc2          $vf2, 128($a0)\n\t"
 "lqc2          $vf3,   0($v0)\n\t"
 "lqc2          $vf5, 128($v0)\n\t"
 "lqc2          $vf4, 256($v0)\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf3\n\t"
 "vmadd.xyzw    $vf6, $vf2, $vf3\n\t"
 "addiu         $a0, $a0, 16\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf2, $vf4\n\t"
 "vmaddw.xyzw   $vf2, $vf6, $vf0\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf5\n\t"
 "vmaddw.xyzw   $vf1, $vf6, $vf0\n\t"
 "addiu         $v0, $v0, 16\n\t"
 "sqc2          $vf2, -16($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf1, 112($a0)\n\t"
 "addiu         $at, $a0, 64\n\t"
 "1:\n\t"
 "lqc2          $vf1, -128($a0)\n\t"
 "lqc2          $vf2,    0($a0)\n\t"
 "lqc2          $vf3,  -64($a0)\n\t"
 "lqc2          $vf4,   64($a0)\n\t"
 "vsub.xyzw     $vf7, $vf1, $vf3\n\t"
 "lqc2          $vf5, 640($v0)\n\t"
 "vsub.xyzw     $vf8, $vf2, $vf4\n\t"
 "lqc2          $vf6, 768($v0)\n\t"
 "vadd.xyzw     $vf1, $vf1, $vf3\n\t"
 "vadd.xyzw     $vf2, $vf2, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf5\n\t"
 "vmsub.xyzw    $vf3, $vf8, $vf6\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf6\n\t"
 "vmadd.xyzw    $vf4, $vf8, $vf5\n\t"
 "addiu         $a0, $a0, 16\n\t"
 "addiu         $v0, $v0, 16\n\t"
 "sqc2          $vf1, -144($a0)\n\t"
 "sqc2          $vf2,  -16($a0)\n\t"
 "sqc2          $vf3,  -80($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf4,   48($a0)\n\t"
 "addiu         $at, $a0, 32\n\t"
 "1:\n\t"
 "lq            " ASM_REG_T0 ", 576($v0)\n\t"
 "lq            " ASM_REG_T1 ", 592($v0)\n\t"
 "lq            " ASM_REG_T2 ", 704($v0)\n\t"
 "lq            " ASM_REG_T3 ", 720($v0)\n\t"
 "lqc2          $vf1, -192($a0)\n\t"
 "ppacw         " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "lqc2          $vf2,  -64($a0)\n\t"
 "ppacw         " ASM_REG_T1 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
 "lqc2          $vf3, -160($a0)\n\t"
 "lqc2          $vf4,  -32($a0)\n\t"
 "vsub.xyzw     $vf7, $vf1, $vf3\n\t"
 "vsub.xyzw     $vf8, $vf2, $vf4\n\t"
 "qmtc2         " ASM_REG_T0 ", $vf5\n\t"
 "qmtc2         " ASM_REG_T1 ", $vf6\n\t"
 "vadd.xyzw     $vf1, $vf1, $vf3\n\t"
 "vadd.xyzw     $vf2, $vf2, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf5\n\t"
 "vmsub.xyzw    $vf3, $vf8, $vf6\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf6\n\t"
 "vmadd.xyzw    $vf4, $vf8, $vf5\n\t"
 "addiu         $a0, $a0, 16\n\t"
 "sqc2          $vf1, -208($a0)\n\t"
 "sqc2          $vf2,  -80($a0)\n\t"
 "sqc2          $vf3, -176($a0)\n\t"
 "sqc2          $vf4,  -48($a0)\n\t"
 "lqc2          $vf1, -144($a0)\n\t"
 "lqc2          $vf2,  -16($a0)\n\t"
 "lqc2          $vf3, -112($a0)\n\t"
 "lqc2          $vf4,   16($a0)\n\t"
 "vsub.xyzw     $vf7, $vf1, $vf3\n\t"
 "vsub.xyzw     $vf8, $vf2, $vf4\n\t"
 "vadd.xyzw     $vf1, $vf1, $vf3\n\t"
 "vadd.xyzw     $vf2, $vf2, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf5\n\t"
 "vmsub.xyzw    $vf3, $vf8, $vf6\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf7, $vf6\n\t"
 "vmadd.xyzw    $vf4, $vf8, $vf5\n\t"
 "addiu         $v0, $v0, 32\n\t"
 "sqc2          $vf1, -144($a0)\n\t"
 "sqc2          $vf2,  -16($a0)\n\t"
 "sqc2          $vf3, -112($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf4,   16($a0)\n\t"
 "lqc2          $vf7, 576($v0)\n\t"
 "lqc2          $vf8, 592($v0)\n\t"
 "lqc2          $vf9, 608($v0)\n\t"
 "vmaxw.xyzw    $vf11, $vf7, $vf0\n\t"
 "vmaxw.xyzw    $vf12, $vf7, $vf0\n\t"
 "addiu         $at, $a0, 128\n\t"
 "1:\n\t"
 "lqc2          $vf1, -224($a0)\n\t"
 "lqc2          $vf2,  -96($a0)\n\t"
 "lqc2          $vf3, -208($a0)\n\t"
 "lqc2          $vf4,  -80($a0)\n\t"
 "vadd.xyzw     $vf5, $vf1, $vf3\n\t"
 "vadd.xyzw     $vf6, $vf2, $vf4\n\t"
 "vmove.z       $vf10, $vf1\n\t"
 "vmove.z       $vf1, $vf2\n\t"
 "vmove.z       $vf2, $vf3\n\t"
 "vmove.z       $vf3, $vf4\n\t"
 "vmove.z       $vf4, $vf10\n\t"
 "sqc2          $vf5, -224($a0)\n\t"
 "sqc2          $vf6,  -96($a0)\n\t"
 "vsub.xyzw     $vf1, $vf1, $vf3\n\t"
 "vsub.xyzw     $vf2, $vf2, $vf4\n\t"
 "addiu         $a0, $a0, 32\n\t"
 "vmove.yw      $vf11, $vf1\n\t"
 "vmove.yw      $vf12, $vf2\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf9\n\t"
 "vmadd.xyzw    $vf1, $vf7, $vf12\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf2, $vf9\n\t"
 "vmadd.xyzw    $vf2, $vf8, $vf11\n\t"
 "sqc2          $vf1, -240($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf2, -112($a0)\n\t"
 "lq            " ASM_REG_T9 ", 624($v0)\n\t"
 "addiu         $at, $a0, 128\n\t"
 "1:\n\t"
 "lq            " ASM_REG_T0 ", -352($a0)\n\t"
 "lq            " ASM_REG_T1 ", -224($a0)\n\t"
 "pcpyld        " ASM_REG_T2 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pcpyld        " ASM_REG_T3 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "pxor          " ASM_REG_T4 ", " ASM_REG_T2 ", " ASM_REG_T9 "\n\t"
 "qmtc2         " ASM_REG_T2 ", $vf1\n\t"
 "pcpyud        " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "qmtc2         " ASM_REG_T3 ", $vf2\n\t"
 "pcpyud        " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf3\n\t"
 "qmtc2         " ASM_REG_T0 ", $vf6\n\t"
 "pxor          " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T9 "\n\t"
 "qmtc2         " ASM_REG_T0 ", $vf4\n\t"
 "pxor          " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T9 "\n\t"
 "qmtc2         " ASM_REG_T1 ", $vf5\n\t"
 "vmove.w       $vf1, $vf2\n\t"
 "vmove.w       $vf4, $vf5\n\t"
 "vmove.w       $vf2, $vf6\n\t"
 "vmove.w       $vf5, $vf3\n\t"
 "vadd.xyzw     $vf1, $vf1, $vf4\n\t"
 "vadd.xyzw     $vf2, $vf2, $vf5\n\t"
 "addiu         $a0, $a0, 16\n\t"
 "sqc2          $vf1, -368($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf2, -240($a0)\n\t"
 "pexcw         " ASM_REG_T9 ", " ASM_REG_T9 "\n\t"
 "addiu         $at, $a0, 128\n\t"
 "1:\n\t"
 "lq            " ASM_REG_T0 ", -480($a0)\n\t"
 "lq            " ASM_REG_T1 ", -464($a0)\n\t"
 "lq            " ASM_REG_T2 ", -352($a0)\n\t"
 "lq            " ASM_REG_T3 ", -336($a0)\n\t"
 "pexcw         " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pexcw         " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "pexcw         " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "pexcw         " ASM_REG_T3 ", " ASM_REG_T3 "\n\t"
 "pextuw        " ASM_REG_T4 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "pxor          " ASM_REG_T4 ", " ASM_REG_T4 ", " ASM_REG_T9 "\n\t"
 "pextlw        " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
 "qmtc2         " ASM_REG_T0 ", $vf1\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf2\n\t"
 "vadd.xyzw     $vf1, $vf1, $vf2\n\t"
 "pextuw        " ASM_REG_T4 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "pxor          " ASM_REG_T4 ", " ASM_REG_T4 ", " ASM_REG_T9 "\n\t"
 "pextlw        " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
 "qmtc2         " ASM_REG_T1 ", $vf2\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf3\n\t"
 "vadd.xyzw     $vf2, $vf2, $vf3\n\t"
 "pextuw        " ASM_REG_T4 ", " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "pxor          " ASM_REG_T4 ", " ASM_REG_T4 ", " ASM_REG_T9 "\n\t"
 "pextlw        " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "qmtc2         " ASM_REG_T2 ", $vf3\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf4\n\t"
 "vadd.xyzw     $vf3, $vf3, $vf4\n\t"
 "pextuw        " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T3 "\n\t"
 "pxor          " ASM_REG_T4 ", " ASM_REG_T4 ", " ASM_REG_T9 "\n\t"
 "pextlw        " ASM_REG_T3 ", " ASM_REG_T3 ", " ASM_REG_T3 "\n\t"
 "qmtc2         " ASM_REG_T3 ", $vf4\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf5\n\t"
 "vadd.xyzw     $vf4, $vf4, $vf5\n\t"
 "addiu         $a0, $a0, 32\n\t"
 "sqc2          $vf1, -512($a0)\n\t"
 "sqc2          $vf2, -496($a0)\n\t"
 "sqc2          $vf3, -384($a0)\n\t"
 "bne           $a0, $at, 1b\n\t"
 "sqc2          $vf4, -368($a0)\n\t"
 "lwc1          $f0, 192($v0)\n\t"
 "addiu         $at, $a1, 128\n\t"
 "lwc1          $f1, -604($a0)\n\t"
 "mtsah         $zero, 2\n\t"
 "lwc1          $f2, -476($a0)\n\t"
 "mula.s        $f1, $f0\n\t"
 "pextlw        $a0, $a0, $a0\n\t"
 "madd.s        $f3, $f2, $f0\n\t"
 "mula.s        $f2, $f0\n\t"
 "pcpyld        $a0, $a0, $a0\n\t"
 "msub.s        $f2, $f1, $f0\n\t"
 "1:\n\t"
 "lq            " ASM_REG_T1 ", 704($v0)\n\t"
 "addiu         $v0, $v0, 16\n\t"
 "paddw         " ASM_REG_T2 ", $a0, " ASM_REG_T1 "\n\t"
 "lw            " ASM_REG_T3 ", -608(" ASM_REG_T2 ")\n\t"
 "lw            " ASM_REG_T5 ", -480(" ASM_REG_T2 ")\n\t"
 "qfsrv         " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "qfsrv         " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T4 "\n\t"
 "qfsrv         " ASM_REG_T6 ", " ASM_REG_T5 ", " ASM_REG_T6 "\n\t"
 "lw            " ASM_REG_T3 ", -608(" ASM_REG_T2 ")\n\t"
 "lw            " ASM_REG_T5 ", -480(" ASM_REG_T2 ")\n\t"
 "qfsrv         " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "qfsrv         " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T4 "\n\t"
 "qfsrv         " ASM_REG_T6 ", " ASM_REG_T5 ", " ASM_REG_T6 "\n\t"
 "lw            " ASM_REG_T3 ", -608(" ASM_REG_T2 ")\n\t"
 "lw            " ASM_REG_T5 ", -480(" ASM_REG_T2 ")\n\t"
 "qfsrv         " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T2 "\n\t"
 "qfsrv         " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T4 "\n\t"
 "qfsrv         " ASM_REG_T6 ", " ASM_REG_T5 ", " ASM_REG_T6 "\n\t"
 "lw            " ASM_REG_T3 ", -608(" ASM_REG_T2 ")\n\t"
 "lw            " ASM_REG_T5 ", -480(" ASM_REG_T2 ")\n\t"
 "qfsrv         " ASM_REG_T4 ", " ASM_REG_T3 ", " ASM_REG_T4 "\n\t"
 "qfsrv         " ASM_REG_T6 ", " ASM_REG_T5 ", " ASM_REG_T6 "\n\t"
 "qmtc2         " ASM_REG_T4 ", $vf1\n\t"
 "qmtc2         " ASM_REG_T6 ", $vf2\n\t"
 "lqc2          $vf3, 112($v0)\n\t"
 "lqc2          $vf5, 240($v0)\n\t"
 "lqc2          $vf4, 368($v0)\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf3\n\t"
 "vmadd.xyzw    $vf6, $vf2, $vf3\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf2, $vf4\n\t"
 "vmaddw.xyzw   $vf2, $vf6, $vf0\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf5\n\t"
 "vmaddw.xyzw   $vf1, $vf6, $vf0\n\t"
 "addiu         $a1, $a1, 16\n\t"
 "sqc2          $vf2, -16($a1)\n\t"
 "bne           $a1, $at, 1b\n\t"
 "sqc2          $vf1, 112($a1)\n\t"
 "swc1          $f3, -64($a1)\n\t"
 "jr            $ra\n\t"
 "swc1          $f2,  64($a1)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void DST4_32 ( float* y, float* x ) {
 float   f0,   f1,  f2,   f3,    f4,   f5,   f6,   f7,   f8,  f9;
 float  f10,  f11,  f12,  f13,  f14,  f15,  f16,  f17,  f18,  f19;
 float  f20,  f21,  f22,  f23,  f24,  f25,  f26,  f27,  f28,  f29;
 float  f30,  f31,  f32,  f33,  f34,  f35,  f36,  f37,  f38,  f39;
 float  f40,  f41,  f42,  f43,  f44,  f45,  f46,  f47,  f48,  f49;
 float  f50,  f51,  f52,  f53,  f54,  f55,  f56,  f57,  f58,  f59;
 float  f60,  f61,  f62,  f63,  f64,  f65,  f66,  f67,  f68,  f69;
 float  f70,  f71,  f72,  f73,  f74,  f75,  f76,  f77,  f78,  f79;
 float  f80,  f81,  f82,  f83,  f84,  f85,  f86,  f87,  f88,  f89;
 float  f90,  f91,  f92,  f93,  f94,  f95,  f96,  f97,  f98,  f99;
 float f100, f101, f102, f103, f104, f105, f106, f107, f108, f109;
 float f110, f111, f112, f113, f114, f115, f116, f117, f118, f119;
 float f120, f121, f122, f123, f124, f125, f126, f127, f128, f129;
 float f130, f131, f132, f133, f134, f135, f136, f137, f138, f139;
 float f140, f141, f142, f143, f144, f145, f146, f147, f148, f149;
 float f150, f151, f152, f153, f154, f155, f156, f157, f158, f159;
 float f160, f161, f162, f163, f164, f165, f166, f167, f168, f169;
 float f170, f171, f172, f173, f174, f175, f176, f177, f178, f179;
 float f180, f181, f182, f183, f184, f185, f186, f187, f188, f189;
 float f190, f191, f192, f193, f194, f195, f196, f197, f198, f199;
 float f200, f201, f202, f203, f204, f205, f206, f207, f208, f209;
 float f210, f211, f212, f213, f214, f215, f216, f217, f218, f219;
 float f220, f221, f222, f223, f224, f225, f226, f227, f228, f229;
 float f230, f231, f232, f233, f234, f235, f236, f237, f238, f239;
 float f240, f241, f242, f243, f244, f245, f246, f247, f248, f249;
 float f250, f251, f252, f253, f254, f255, f256, f257, f258, f259;
 float f260, f261, f262, f263, f264, f265, f266, f267, f268, f269;
 float f270, f271, f272, f273, f274, f275, f276, f277, f278, f279;
 float f280, f281, f282, f283, f284, f285, f286, f287, f288, f289;
 float f290, f291, f292, f293, f294, f295, f296, f297, f298, f299;
 float f300, f301, f302, f303, f304, f305, f306, f307, f308, f309;
 float f310, f311, f312, f313, f314, f315, f316, f317, f318, f319;
 float f320, f321, f322, f323, f324, f325, f326, f327, f328, f329;
 float f330, f331, f332, f333, f334, f335;
   f0 = x[  0 ] - x[  1 ];
   f1 = x[  2 ] - x[  1 ];
   f2 = x[  2 ] - x[  3 ];
   f3 = x[  4 ] - x[  3 ];
   f4 = x[  4 ] - x[  5 ];
   f5 = x[  6 ] - x[  5 ];
   f6 = x[  6 ] - x[  7 ];
   f7 = x[  8 ] - x[  7 ];
   f8 = x[  8 ] - x[  9 ];
   f9 = x[ 10 ] - x[  9 ];
  f10 = x[ 10 ] - x[ 11 ];
  f11 = x[ 12 ] - x[ 11 ];
  f12 = x[ 12 ] - x[ 13 ];
  f13 = x[ 14 ] - x[ 13 ];
  f14 = x[ 14 ] - x[ 15 ];
  f15 = x[ 16 ] - x[ 15 ];
  f16 = x[ 16 ] - x[ 17 ];
  f17 = x[ 18 ] - x[ 17 ];
  f18 = x[ 18 ] - x[ 19 ];
  f19 = x[ 20 ] - x[ 19 ];
  f20 = x[ 20 ] - x[ 21 ];
  f21 = x[ 22 ] - x[ 21 ];
  f22 = x[ 22 ] - x[ 23 ];
  f23 = x[ 24 ] - x[ 23 ];
  f24 = x[ 24 ] - x[ 25 ];
  f25 = x[ 26 ] - x[ 25 ];
  f26 = x[ 26 ] - x[ 27 ];
  f27 = x[ 28 ] - x[ 27 ];
  f28 = x[ 28 ] - x[ 29 ];
  f29 = x[ 30 ] - x[ 29 ];
  f30 = x[ 30 ] - x[ 31 ];
  f31 = 0.7071067811865476F * f15;
  f32 = x[  0 ] - f31;
  f33 = x[  0 ] + f31;
  f34 = f7 + f23;
  f35 =  1.3065629648763766F *  f7;
  f36 = -0.9238795325112866F * f34;
  f37 = -0.5411961001461967F * f23;
  f38 = f35 + f36;
  f39 = f37 - f36;
  f40 = f33 - f39;
  f41 = f33 + f39;
  f42 = f32 - f38;
  f43 = f32 + f38;
  f44 = f11 - f19;
  f45 = f11 + f19;
  f46 = 0.7071067811865476F * f45;
  f47 = f3 - f46;
  f48 = f3 + f46;
  f49 = 0.7071067811865476F * f44;
  f50 = f49 - f27;
  f51 = f49 + f27;
  f52 = f51 + f48;
  f53 = -0.7856949583871021F * f51;
  f54 =  0.9807852804032304F * f52;
  f55 =  1.1758756024193588F * f48;
  f56 = f53 + f54;
  f57 = f55 - f54;
  f58 = f50 + f47;
  f59 = -0.2758993792829430F * f50;
  f60 =  0.8314696123025452F * f58;
  f61 =  1.3870398453221475F * f47;
  f62 = f59 + f60;
  f63 = f61 - f60;
  f64 = f41 - f56;
  f65 = f41 + f56;
  f66 = f43 - f62;
  f67 = f43 + f62;
  f68 = f42 - f63;
  f69 = f42 + f63;
  f70 = f40 - f57;
  f71 = f40 + f57;
  f72 =  f5 -  f9;
  f73 =  f5 +  f9;
  f74 = f13 - f17;
  f75 = f13 + f17;
  f76 = f21 - f25;
  f77 = f21 + f25;
  f78 = 0.7071067811865476F * f75;
  f79 = f1 - f78;
  f80 = f1 + f78;
  f81 = f73 + f77;
  f82 =  1.3065629648763766F * f73;
  f83 = -0.9238795325112866F * f81;
  f84 = -0.5411961001461967F * f77;
  f85 = f82 + f83;
  f86 = f84 - f83;
  f87 = f80 - f86;
  f88 = f80 + f86;
  f89 = f79 - f85;
  f90 = f79 + f85;
  f91 = 0.7071067811865476F * f74;
  f92 = f29 - f91;
  f93 = f29 + f91;
  f94 = f76 + f72;
  f95 =  1.3065629648763766F * f76;
  f96 = -0.9238795325112866F * f94;
  f97 = -0.5411961001461967F * f72;
  f98 = f95 + f96;
  f99 = f97 - f96;
 f100 = f93 - f99;
 f101 = f93 + f99;
 f102 = f92 - f98;
 f103 = f92 + f98;
 f104 = f101 + f88;
 f105 = -0.8971675863426361F * f101;
 f106 =  0.9951847266721968F * f104;
 f107 =  1.0932018670017576F *  f88;
 f108 = f105 + f106;
 f109 = f107 - f106;
 f110 = f90 - f103;
 f111 = -0.6666556584777466F * f103;
 f112 =  0.9569403357322089F * f110;
 f113 =  1.2472250129866713F *  f90;
 f114 = f112 - f111;
 f115 = f113 - f112;
 f116 = f102 + f89;
 f117 = -0.4105245275223571F * f102;
 f118 =  0.8819212643483549F * f116;
 f119 =  1.3533180011743529F *  f89;
 f120 = f117 + f118;
 f121 = f119 - f118;
 f122 = f87 - f100;
 f123 = -0.1386171691990915F * f100;
 f124 =  0.7730104533627370F * f122;
 f125 =  1.4074037375263826F *  f87;
 f126 = f124 - f123;
 f127 = f125 - f124;
 f128 = f65 - f108;
 f129 = f65 + f108;
 f130 = f67 - f114;
 f131 = f67 + f114;
 f132 = f69 - f120;
 f133 = f69 + f120;
 f134 = f71 - f126;
 f135 = f71 + f126;
 f136 = f70 - f127;
 f137 = f70 + f127;
 f138 = f68 - f121;
 f139 = f68 + f121;
 f140 = f66 - f115;
 f141 = f66 + f115;
 f142 = f64 - f109;
 f143 = f64 + f109;
 f144 = f0 + f30;
 f145 =  1.0478631305325901F *   f0;
 f146 = -0.9987954562051724F * f144;
 f147 = -0.9497277818777548F *  f30;
 f148 = f145 + f146;
 f149 = f147 - f146;
 f150 = f4 + f26;
 f151 =  1.2130114330978077F *   f4;
 f152 = -0.9700312531945440F * f150;
 f153 = -0.7270510732912803F *  f26;
 f154 = f151 + f152;
 f155 = f153 - f152;
 f156 = f8 + f22;
 f157 =  1.3315443865537255F *   f8;
 f158 = -0.9039892931234433F * f156;
 f159 = -0.4764341996931612F *  f22;
 f160 = f157 + f158;
 f161 = f159 - f158;
 f162 = f12 + f18;
 f163 =  1.3989068359730781F *  f12;
 f164 = -0.8032075314806453F * f162;
 f165 = -0.2075082269882124F *  f18;
 f166 = f163 + f164;
 f167 = f165 - f164;
 f168 = f16 + f14;
 f169 =  1.4125100802019777F *  f16;
 f170 = -0.6715589548470187F * f168;
 f171 =  0.0693921705079402F *  f14;
 f172 = f169 + f170;
 f173 = f171 - f170;
 f174 = f20 + f10;
 f175 =  1.3718313541934939F *  f20;
 f176 = -0.5141027441932219F * f174;
 f177 =  0.3436258658070501F *  f10;
 f178 = f175 + f176;
 f179 = f177 - f176;
 f180 = f24 + f6;
 f181 =  1.2784339185752409F *  f24;
 f182 = -0.3368898533922200F * f180;
 f183 =  0.6046542117908008F *   f6;
 f184 = f181 + f182;
 f185 = f183 - f182;
 f186 = f28 + f2;
 f187 =  1.1359069844201433F *  f28;
 f188 = -0.1467304744553624F * f186;
 f189 =  0.8424460355094185F *   f2;
 f190 = f187 + f188;
 f191 = f189 - f188;
 f192 = f149 - f173;
 f193 = f149 + f173;
 f194 = f148 - f172;
 f195 = f148 + f172;
 f196 = f155 - f179;
 f197 = f155 + f179;
 f198 = f154 - f178;
 f199 = f154 + f178;
 f200 = f161 - f185;
 f201 = f161 + f185;
 f202 = f160 - f184;
 f203 = f160 + f184;
 f204 = f167 - f191;
 f205 = f167 + f191;
 f206 = f166 - f190;
 f207 = f166 + f190;
 f208 = f192 + f194;
 f209 =  1.1758756024193588F * f192;
 f210 = -0.9807852804032304F * f208;
 f211 = -0.7856949583871021F * f194;
 f212 = f209 + f210;
 f213 = f211 - f210;
 f214 = f196 + f198;
 f215 =  1.3870398453221475F * f196;
 f216 = -0.5555702330196022F * f214;
 f217 =  0.2758993792829431F * f198;
 f218 = f215 + f216;
 f219 = f217 - f216;
 f220 = f200 + f202;
 f221 = 0.7856949583871022F * f200;
 f222 = 0.1950903220161283F * f220;
 f223 = 1.1758756024193586F * f202;
 f224 = f221 + f222;
 f225 = f223 - f222;
 f226 = f204 + f206;
 f227 = -0.2758993792829430F * f204;
 f228 =  0.8314696123025452F * f226;
 f229 =  1.3870398453221475F * f206;
 f230 = f227 + f228;
 f231 = f229 - f228;
 f232 = f193 - f201;
 f233 = f193 + f201;
 f234 = f195 - f203;
 f235 = f195 + f203;
 f236 = f197 - f205;
 f237 = f197 + f205;
 f238 = f199 - f207;
 f239 = f199 + f207;
 f240 = f213 - f225;
 f241 = f213 + f225;
 f242 = f212 - f224;
 f243 = f212 + f224;
 f244 = f219 - f231;
 f245 = f219 + f231;
 f246 = f218 - f230;
 f247 = f218 + f230;
 f248 = f232 + f234;
 f249 =  1.3065629648763766F * f232;
 f250 = -0.9238795325112866F * f248;
 f251 = -0.5411961001461967F * f234;
 f252 = f249 + f250;
 f253 = f251 - f250;
 f254 = f236 + f238;
 f255 = 0.5411961001461969F * f236;
 f256 = 0.3826834323650898F * f254;
 f257 = 1.3065629648763766F * f238;
 f258 = f255 + f256;
 f259 = f257 - f256;
 f260 = f240 + f242;
 f261 =  1.3065629648763766F * f240;
 f262 = -0.9238795325112866F * f260;
 f263 = -0.5411961001461967F * f242;
 f264 = f261 + f262;
 f265 = f263 - f262;
 f266 = f244 + f246;
 f267 = 0.5411961001461969F * f244;
 f268 = 0.3826834323650898F * f266;
 f269 = 1.3065629648763766F * f246;
 f270 = f267 + f268;
 f271 = f269 - f268;
 f272 = f233 - f237;
 f273 = f233 + f237;
 f274 = f235 - f239;
 f275 = f235 + f239;
 f276 = f253 - f259;
 f277 = f253 + f259;
 f278 = f252 - f258;
 f279 = f252 + f258;
 f280 = f241 - f245;
 f281 = f241 + f245;
 f282 = f243 - f247;
 f283 = f243 + f247;
 f284 = f265 - f271;
 f285 = f265 + f271;
 f286 = f264 - f270;
 f287 = f264 + f270;
 f288 = f272 - f274;
 f289 = f272 + f274;
 f290 = 0.7071067811865474F * f288;
 f291 = 0.7071067811865474F * f289;
 f292 = f276 - f278;
 f293 = f276 + f278;
 f294 = 0.7071067811865474F * f292;
 f295 = 0.7071067811865474F * f293;
 f296 = f280 - f282;
 f297 = f280 + f282;
 f298 = 0.7071067811865474F * f296;
 f299 = 0.7071067811865474F * f297;
 f300 = f284 - f286;
 f301 = f284 + f286;
 f302 = 0.7071067811865474F * f300;
 f303 = 0.7071067811865474F * f301;
 f304 = f129 - f273;
 f305 = f129 + f273;
 f306 = f131 - f281;
 f307 = f131 + f281;
 f308 = f133 - f285;
 f309 = f133 + f285;
 f310 = f135 - f277;
 f311 = f135 + f277;
 f312 = f137 - f295;
 f313 = f137 + f295;
 f314 = f139 - f303;
 f315 = f139 + f303;
 f316 = f141 - f299;
 f317 = f141 + f299;
 f318 = f143 - f291;
 f319 = f143 + f291;
 f320 = f142 - f290;
 f321 = f142 + f290;
 f322 = f140 - f298;
 f323 = f140 + f298;
 f324 = f138 - f302;
 f325 = f138 + f302;
 f326 = f136 - f294;
 f327 = f136 + f294;
 f328 = f134 - f279;
 f329 = f134 + f279;
 f330 = f132 - f287;
 f331 = f132 + f287;
 f332 = f130 - f283;
 f333 = f130 + f283;
 f334 = f128 - f275;
 f335 = f128 + f275;
 y[ 31 ] =  0.5001506360206510F * f305;
 y[ 30 ] =  0.5013584524464084F * f307;
 y[ 29 ] =  0.5037887256810443F * f309;
 y[ 28 ] =  0.5074711720725553F * f311;
 y[ 27 ] =  0.5124514794082247F * f313;
 y[ 26 ] =  0.5187927131053328F * f315;
 y[ 25 ] =  0.5265773151542700F * f317;
 y[ 24 ] =  0.5359098169079920F * f319;
 y[ 23 ] =  0.5469204379855088F * f321;
 y[ 22 ] =  0.5597698129470802F * f323;
 y[ 21 ] =  0.5746551840326600F * f325;
 y[ 20 ] =  0.5918185358574165F * f327;
 y[ 19 ] =  0.6115573478825099F * f329;
 y[ 18 ] =  0.6342389366884031F * f331;
 y[ 17 ] =  0.6603198078137061F * f333;
 y[ 16 ] =  0.6903721282002123F * f335;
 y[ 15 ] =  0.7251205223771985F * f334;
 y[ 14 ] =  0.7654941649730891F * f332;
 y[ 13 ] =  0.8127020908144905F * f330;
 y[ 12 ] =  0.8683447152233481F * f328;
 y[ 11 ] =  0.9345835970364075F * f326;
 y[ 10 ] =  1.0144082649970547F * f324;
 y[  9 ] =  1.1120716205797176F * f322;
 y[  8 ] =  1.2338327379765710F * f320;
 y[  7 ] =  1.3892939586328277F * f318;
 y[  6 ] =  1.5939722833856311F * f316;
 y[  5 ] =  1.8746759800084078F * f314;
 y[  4 ] =  2.2820500680051619F * f312;
 y[  3 ] =  2.9246284281582162F * f310;
 y[  2 ] =  4.0846110781292477F * f308;
 y[  1 ] =  6.7967507116736332F * f306;
 y[  0 ] = 20.3738781672314530F * f304;

}  /* end DST4_32 */

void DCT4_32 ( float* y, float* x ) {
 float   f0,   f1,   f2,   f3,   f4,   f5,   f6,   f7,   f8,  f9, f10;
 float  f11,  f12,  f13,  f14,  f15,  f16,  f17,  f18,  f19,  f20;
 float  f21,  f22,  f23,  f24,  f25,  f26,  f27,  f28,  f29,  f30;
 float  f31,  f32,  f33,  f34,  f35,  f36,  f37,  f38,  f39,  f40;
 float  f41,  f42,  f43,  f44,  f45,  f46,  f47,  f48,  f49,  f50;
 float  f51,  f52,  f53,  f54,  f55,  f56,  f57,  f58,  f59,  f60;
 float  f61,  f62,  f63,  f64,  f65,  f66,  f67,  f68,  f69,  f70;
 float  f71,  f72,  f73,  f74,  f75,  f76,  f77,  f78,  f79,  f80;
 float  f81,  f82,  f83,  f84,  f85,  f86,  f87,  f88,  f89,  f90;
 float  f91,  f92,  f93,  f94,  f95,  f96,  f97,  f98,  f99, f100;
 float f101, f102, f103, f104, f105, f106, f107, f108, f109, f110;
 float f111, f112, f113, f114, f115, f116, f117, f118, f119, f120;
 float f121, f122, f123, f124, f125, f126, f127, f128, f129, f130;
 float f131, f132, f133, f134, f135, f136, f137, f138, f139, f140;
 float f141, f142, f143, f144, f145, f146, f147, f148, f149, f150;
 float f151, f152, f153, f154, f155, f156, f157, f158, f159, f160;
 float f161, f162, f163, f164, f165, f166, f167, f168, f169, f170;
 float f171, f172, f173, f174, f175, f176, f177, f178, f179, f180;
 float f181, f182, f183, f184, f185, f186, f187, f188, f189, f190;
 float f191, f192, f193, f194, f195, f196, f197, f198, f199, f200;
 float f201, f202, f203, f204, f205, f206, f207, f208, f209, f210;
 float f211, f212, f213, f214, f215, f216, f217, f218, f219, f220;
 float f221, f222, f223, f224, f225, f226, f227, f228, f229, f230;
 float f231, f232, f233, f234, f235, f236, f237, f238, f239, f240;
 float f241, f242, f243, f244, f245, f246, f247, f248, f249, f250;
 float f251, f252, f253, f254, f255, f256, f257, f258, f259, f260;
 float f261, f262, f263, f264, f265, f266, f267, f268, f269, f270;
 float f271, f272, f273, f274, f275, f276, f277, f278, f279, f280;
 float f281, f282, f283, f284, f285, f286, f287, f288, f289, f290;
 float f291, f292, f293, f294, f295, f296, f297, f298, f299, f300;
 float f301, f302, f303, f304, f305, f306, f307, f310, f311, f312;
 float f313, f316, f317, f318, f319, f322, f323, f324, f325, f328;
 float f329, f330, f331, f334, f335, f336, f337, f340, f341, f342;
 float f343, f346, f347, f348, f349, f352, f353, f354, f355, f358;
 float f359, f360, f361, f364, f365, f366, f367, f370, f371, f372;
 float f373, f376, f377, f378, f379, f382, f383, f384, f385, f388;
 float f389, f390, f391, f394, f395, f396, f397;

  f0 = x[ 15 ] - x[ 16 ];
  f1 = x[ 15 ] + x[ 16 ];
  f4 = x[  8 ] - x[ 23 ];
  f5 = x[  8 ] + x[ 23 ];
  f8 = x[ 12 ] - x[ 19 ];
  f9 = x[ 12 ] + x[ 19 ];
 f12 = x[ 11 ] - x[ 20 ];
 f13 = x[ 11 ] + x[ 20 ];
 f16 = x[ 14 ] - x[ 17 ];
 f17 = x[ 14 ] + x[ 17 ];
 f20 = x[  9 ] - x[ 22 ];
 f21 = x[  9 ] + x[ 22 ];
 f24 = x[ 13 ] - x[ 18 ];
 f25 = x[ 13 ] + x[ 18 ];
 f28 = x[ 10 ] - x[ 21 ];
 f29 = x[ 10 ] + x[ 21 ];
  f2 = 0.7071067811865476F *  f1;
  f3 = 0.7071067811865476F *  f0;
  f6 = 0.7071067811865476F *  f5;
  f7 = 0.7071067811865476F *  f4;
 f10 = 0.7071067811865476F *  f9;
 f11 = 0.7071067811865476F *  f8;
 f14 = 0.7071067811865476F * f13;
 f15 = 0.7071067811865476F * f12;
 f18 = 0.7071067811865476F * f17;
 f19 = 0.7071067811865476F * f16;
 f22 = 0.7071067811865476F * f21;
 f23 = 0.7071067811865476F * f20;
 f26 = 0.7071067811865476F * f25;
 f27 = 0.7071067811865476F * f24;
 f30 = 0.7071067811865476F * f29;
 f31 = 0.7071067811865476F * f28;
 f32 = x[  0 ] -  f2;
 f33 = x[  0 ] +  f2;
 f34 = x[ 31 ] -  f3;
 f35 = x[ 31 ] +  f3;
 f36 = x[  7 ] -  f6;
 f37 = x[  7 ] +  f6;
 f38 = x[ 24 ] -  f7;
 f39 = x[ 24 ] +  f7;
 f40 = x[  3 ] - f10;
 f41 = x[  3 ] + f10;
 f42 = x[ 28 ] - f11;
 f43 = x[ 28 ] + f11;
 f44 = x[  4 ] - f14;
 f45 = x[  4 ] + f14;
 f46 = x[ 27 ] - f15;
 f47 = x[ 27 ] + f15;
 f48 = x[  1 ] - f18;
 f49 = x[  1 ] + f18;
 f50 = x[ 30 ] - f19;
 f51 = x[ 30 ] + f19;
 f52 = x[  6 ] - f22;
 f53 = x[  6 ] + f22;
 f54 = x[ 25 ] - f23;
 f55 = x[ 25 ] + f23;
 f56 = x[  2 ] - f26;
 f57 = x[  2 ] + f26;
 f58 = x[ 29 ] - f27;
 f59 = x[ 29 ] + f27;
 f60 = x[  5 ] - f30;
 f61 = x[  5 ] + f30;
 f62 = x[ 26 ] - f31;
 f63 = x[ 26 ] + f31;
 f64 = f39 + f37;
 f65 = -0.5411961001461969F * f39;
 f66 =  0.9238795325112867F * f64;
 f67 =  1.3065629648763766F * f37;
 f68 = f65 + f66;
 f69 = f67 - f66;
 f70 = f38 + f36;
 f71 =  1.3065629648763770F * f38;
 f72 = -0.3826834323650904F * f70;
 f73 =  0.5411961001461961F * f36;
 f74 = f71 + f72;
 f75 = f73 - f72;
 f76 = f47 + f45;
 f77 = -0.5411961001461969F * f47;
 f78 =  0.9238795325112867F * f76;
 f79 =  1.3065629648763766F * f45;
 f80 = f77 + f78;
 f81 = f79 - f78;
 f82 = f46 + f44;
 f83 =  1.3065629648763770F * f46;
 f84 = -0.3826834323650904F * f82;
 f85 =  0.5411961001461961F * f44;
 f86 = f83 + f84;
 f87 = f85 - f84;
 f88 = f55 + f53;
 f89 = -0.5411961001461969F * f55;
 f90 =  0.9238795325112867F * f88;
 f91 =  1.3065629648763766F * f53;
 f92 = f89 + f90;
 f93 = f91 - f90;
 f94 = f54 + f52;
 f95 =  1.3065629648763770F * f54;
 f96 = -0.3826834323650904F * f94;
 f97 =  0.5411961001461961F * f52;
 f98 = f95 + f96;
 f99 = f97 - f96;
 f100 = f63 + f61;
 f101 = -0.5411961001461969F *  f63;
 f102 =  0.9238795325112867F * f100;
 f103 =  1.3065629648763766F *  f61;
 f104 = f101 + f102;
 f105 = f103 - f102;
 f106 = f62 + f60;
 f107 =  1.3065629648763770F *  f62;
 f108 = -0.3826834323650904F * f106;
 f109 =  0.5411961001461961F *  f60;
 f110 = f107 + f108;
 f111 = f109 - f108;
 f112 = f33 - f68;
 f113 = f33 + f68;
 f114 = f35 - f69;
 f115 = f35 + f69;
 f116 = f32 - f74;
 f117 = f32 + f74;
 f118 = f34 - f75;
 f119 = f34 + f75;
 f120 = f41 - f80;
 f121 = f41 + f80;
 f122 = f43 - f81;
 f123 = f43 + f81;
 f124 = f40 - f86;
 f125 = f40 + f86;
 f126 = f42 - f87;
 f127 = f42 + f87;
 f128 = f49 - f92;
 f129 = f49 + f92;
 f130 = f51 - f93;
 f131 = f51 + f93;
 f132 = f48 - f98;
 f133 = f48 + f98;
 f134 = f50 - f99;
 f135 = f50 + f99;
 f136 = f57 - f104;
 f137 = f57 + f104;
 f138 = f59 - f105;
 f139 = f59 + f105;
 f140 = f56 - f110;
 f141 = f56 + f110;
 f142 = f58 - f111;
 f143 = f58 + f111;
 f144 = f123 + f121;
 f145 = -0.7856949583871021F * f123;
 f146 =  0.9807852804032304F * f144;
 f147 =  1.1758756024193588F * f121;
 f148 = f145 + f146;
 f149 = f147 - f146;
 f150 = f127 + f125;
 f151 = 0.2758993792829431F * f127;
 f152 = 0.5555702330196022F * f150;
 f153 = 1.3870398453221475F * f125;
 f154 = f151 + f152;
 f155 = f153 - f152;
 f156 = f122 + f120;
 f157 =  1.1758756024193591F * f122;
 f158 = -0.1950903220161287F * f156;
 f159 =  0.7856949583871016F * f120;
 f160 = f157 + f158;
 f161 = f159 - f158;
 f162 = f126 + f124;
 f163 =  1.3870398453221473F * f126;
 f164 = -0.8314696123025455F * f162;
 f165 = -0.2758993792829436F * f124;
 f166 = f163 + f164;
 f167 = f165 - f164;
 f168 = f139 + f137;
 f169 = -0.7856949583871021F * f139;
 f170 =  0.9807852804032304F * f168;
 f171 =  1.1758756024193588F * f137;
 f172 = f169 + f170;
 f173 = f171 - f170;
 f174 = f143 + f141;
 f175 = 0.2758993792829431F * f143;
 f176 = 0.5555702330196022F * f174;
 f177 = 1.3870398453221475F * f141;
 f178 = f175 + f176;
 f179 = f177 - f176;
 f180 = f138 + f136;
 f181 =  1.1758756024193591F * f138;
 f182 = -0.1950903220161287F * f180;
 f183 =  0.7856949583871016F * f136;
 f184 = f181 + f182;
 f185 = f183 - f182;
 f186 = f142 + f140;
 f187 =  1.3870398453221473F * f142;
 f188 = -0.8314696123025455F * f186;
 f189 = -0.2758993792829436F * f140;
 f190 = f187 + f188;
 f191 = f189 - f188;
 f192 = f113 - f148;
 f193 = f113 + f148;
 f194 = f115 - f149;
 f195 = f115 + f149;
 f196 = f117 - f154;
 f197 = f117 + f154;
 f198 = f119 - f155;
 f199 = f119 + f155;
 f200 = f112 - f160;
 f201 = f112 + f160;
 f202 = f114 - f161;
 f203 = f114 + f161;
 f204 = f116 - f166;
 f205 = f116 + f166;
 f206 = f118 - f167;
 f207 = f118 + f167;
 f208 = f129 - f172;
 f209 = f129 + f172;
 f210 = f131 - f173;
 f211 = f131 + f173;
 f212 = f133 - f178;
 f213 = f133 + f178;
 f214 = f135 - f179;
 f215 = f135 + f179;
 f216 = f128 - f184;
 f217 = f128 + f184;
 f218 = f130 - f185;
 f219 = f130 + f185;
 f220 = f132 - f190;
 f221 = f132 + f190;
 f222 = f134 - f191;
 f223 = f134 + f191;
 f224 = f211 + f209;
 f225 = -0.8971675863426361F * f211;
 f226 =  0.9951847266721968F * f224;
 f227 =  1.0932018670017576F * f209;
 f228 = f225 + f226;
 f229 = f227 - f226;
 f230 = f215 + f213;
 f231 = -0.4105245275223571F * f215;
 f232 =  0.8819212643483549F * f230;
 f233 =  1.3533180011743529F * f213;
 f234 = f231 + f232;
 f235 = f233 - f232;
 f236 = f219 + f217;
 f237 = 0.1386171691990915F * f219;
 f238 = 0.6343932841636455F * f236;
 f239 = 1.4074037375263826F * f217;
 f240 = f237 + f238;
 f241 = f239 - f238;
 f242 = f223 + f221;
 f243 = 0.6666556584777466F * f223;
 f244 = 0.2902846772544623F * f242;
 f245 = 1.2472250129866711F * f221;
 f246 = f243 + f244;
 f247 = f245 - f244;
 f248 = f210 + f208;
 f249 =  1.0932018670017574F * f210;
 f250 = -0.0980171403295605F * f248;
 f251 =  0.8971675863426364F * f208;
 f252 = f249 + f250;
 f253 = f251 - f250;
 f254 = f214 + f212;
 f255 =  1.3533180011743529F * f214;
 f256 = -0.4713967368259979F * f254;
 f257 =  0.4105245275223569F * f212;
 f258 = f255 + f256;
 f259 = f257 - f256;
 f260 = f218 + f216;
 f261 =  1.4074037375263826F * f218;
 f262 = -0.7730104533627369F * f260;
 f263 = -0.1386171691990913F * f216;
 f264 = f261 + f262;
 f265 = f263 - f262;
 f266 = f222 + f220;
 f267 =  1.2472250129866711F * f222;
 f268 = -0.9569403357322089F * f266;
 f269 = -0.6666556584777469F * f220;
 f270 = f267 + f268;
 f271 = f269 - f268;
 f272 = f193 - f228;
 f273 = f193 + f228;
 f274 = f195 - f229;
 f275 = f195 + f229;
 f276 = f197 - f234;
 f277 = f197 + f234;
 f278 = f199 - f235;
 f279 = f199 + f235;
 f280 = f201 - f240;
 f281 = f201 + f240;
 f282 = f203 - f241;
 f283 = f203 + f241;
 f284 = f205 - f246;
 f285 = f205 + f246;
 f286 = f207 - f247;
 f287 = f207 + f247;
 f288 = f192 - f252;
 f289 = f192 + f252;
 f290 = f194 - f253;
 f291 = f194 + f253;
 f292 = f196 - f258;
 f293 = f196 + f258;
 f294 = f198 - f259;
 f295 = f198 + f259;
 f296 = f200 - f264;
 f297 = f200 + f264;
 f298 = f202 - f265;
 f299 = f202 + f265;
 f300 = f204 - f270;
 f301 = f204 + f270;
 f302 = f206 - f271;
 f303 = f206 + f271;
 f304 = f275 + f273;
 f305 = -0.9751575901732920F * f275;
 f306 =  0.9996988186962043F * f304;
 f307 =  1.0242400472191164F * f273;
 y[0] = f305 + f306;
 y[31] = f307 - f306;
 f310 = f279 + f277;
 f311 = -0.8700688593994936F * f279;
 f312 =  0.9924795345987100F * f310;
 f313 =  1.1148902097979263F * f277;
 y[2] = f311 + f312;
 y[29] = f313 - f312;
 f316 = f283 + f281;
 f317 = -0.7566008898816587F * f283;
 f318 =  0.9757021300385286F * f316;
 f319 =  1.1948033701953984F * f281;
 y[4] = f317 + f318;
 y[27] = f319 - f318;
 f322 = f287 + f285;
 f323 = -0.6358464401941451F * f287;
 f324 =  0.9495281805930367F * f322;
 f325 =  1.2632099209919283F * f285;
 y[6] = f323 + f324;
 y[25] = f325 - f324;
 f328 = f291 + f289;
 f329 = -0.5089684416985408F * f291;
 f330 =  0.9142097557035307F * f328;
 f331 =  1.3194510697085207F * f289;
 y[8] = f329 + f330;
 y[23] = f331 - f330;
 f334 = f295 + f293;
 f335 = -0.3771887988789273F * f295;
 f336 =  0.8700869911087114F * f334;
 f337 =  1.3629851833384954F * f293;
 y[10] = f335 + f336;
 y[21] = f337 - f336;
 f340 = f299 + f297;
 f341 = -0.2417766217337384F * f299;
 f342 =  0.8175848131515837F * f340;
 f343 =  1.3933930045694289F * f297;
 y[12] = f341 + f342;
 y[19] = f343 - f342;
 f346 = f303 + f301;
 f347 = -0.1040360035527077F * f303;
 f348 =  0.7572088465064845F * f346;
 f349 =  1.4103816894602612F * f301;
 y[14] = f347 + f348;
 y[17] = f349 - f348;
 f352 = f274 + f272;
 f353 = 0.0347065382144002F * f274;
 f354 = 0.6895405447370668F * f352;
 f355 = 1.4137876276885337F * f272;
 y[16] = f353 + f354;
 y[15] = f355 - f354;
 f358 = f278 + f276;
 f359 = 0.1731148370459795F * f278;
 f360 = 0.6152315905806268F * f358;
 f361 = 1.4035780182072330F * f276;
 y[18] = f359 + f360;
 y[13] = f361 - f360;
 f364 = f282 + f280;
 f365 = 0.3098559453626100F * f282;
 f366 = 0.5349976198870972F * f364;
 f367 = 1.3798511851368043F * f280;
 y[20] = f365 + f366;
 y[11] = f367 - f366;
 f370 = f286 + f284;
 f371 = 0.4436129715409088F * f286;
 f372 = 0.4496113296546065F * f370;
 f373 = 1.3428356308501219F * f284;
 y[22] = f371 + f372;
 y[9] = f373 - f372;
 f376 = f290 + f288;
 f377 = 0.5730977622997509F * f290;
 f378 = 0.3598950365349881F * f376;
 f379 = 1.2928878353697271F * f288;
 y[24] = f377 + f378;
 y[7] = f379 - f378;
 f382 = f294 + f292;
 f383 = 0.6970633083205415F * f294;
 f384 = 0.2667127574748984F * f382;
 f385 = 1.2304888232703382F * f292;
 y[26] = f383 + f384;
 y[5] = f385 - f384;
 f388 = f298 + f296;
 f389 = 0.8143157536286401F * f298;
 f390 = 0.1709618887603012F * f388;
 f391 = 1.1562395311492424F * f296;
 y[28] = f389 + f390;
 y[3] = f391 - f390;
 f394 = f302 + f300;
 f395 = 0.9237258930790228F * f302;
 f396 = 0.0735645635996674F * f394;
 f397 = 1.0708550202783576F * f300;
 y[30] = f395 + f396;
 y[1] = f397 - f396;

}  /* end DCT4_32 */
