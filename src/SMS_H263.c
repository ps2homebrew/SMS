/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000,2001 Fabrice Bellard.
# MMI optimization by Leon van Stuivenberg
# Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_H263.h"
#include "SMS_MPEG.h"
#include "SMS_VLC.h"

#include <limits.h>

static const uint8_t s_h263_chroma_roundtab[ 16 ] = {
 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2
};

int SMS_H263_DecodeMotion ( int aPred, int aFCode ) {

 int             lCode, lVal, lSign, lShift, lL;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

 lCode = SMS_GetVLC2 ( lpBitCtx, g_SMS_mv_vlc.m_pTable, SMS_MV_VLC_BITS, 2 );

 if ( lCode == 0 ) return aPred;
 if ( lCode  < 0 ) return 0xFFFF;

 lSign  = SMS_GetBit ( lpBitCtx );
 lShift = aFCode - 1;
 lVal   = lCode;

 if ( lShift ) {

  lVal  = ( lVal - 1 ) << lShift;
  lVal |= SMS_GetBits ( lpBitCtx, lShift );
  ++lVal;

 }  /* end if */

 if ( lSign ) lVal = -lVal;

 lVal += aPred;

 if ( !g_MPEGCtx.m_H263LongVectors ) {

  lL   = 32 - 5 - aFCode;
  lVal = ( lVal << lL ) >> lL;

 } else {

  if ( aPred < -31 && lVal < -63 ) lVal += 64;
  if ( aPred >  32 && lVal >  63 ) lVal -= 64;

 }  /* end else */

 return lVal;

}  /* end SMS_H263_DecodeMotion */

int16_t* SMS_H263_PredMotion ( int aBlock, int aDir, int* apX, int* apY ) {

 static const int lOff[ 4 ]= { 2, 1, 1, -1 };

 int      lWrap;
 int16_t* lpA, *lpB, *lpC, ( *lpMotVal )[ 2 ];

 lWrap    = g_MPEGCtx.m_B8Stride;
 lpMotVal = g_MPEGCtx.m_CurPic.m_pMotionVal[ aDir ] + g_MPEGCtx.m_BlockIdx[ aBlock ];
 lpA      = lpMotVal[ -1 ];

 if ( g_MPEGCtx.m_FirstSliceLine && aBlock < 3 ) {

  if ( aBlock == 0 ) {

   if ( g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX ) {

    *apX = *apY = 0;

   } else if ( g_MPEGCtx.m_MBX + 1 == g_MPEGCtx.m_ResyncMBX && g_MPEGCtx.m_H263Pred ) {

    lpC = lpMotVal[  lOff[ aBlock ] - lWrap  ];

    if ( g_MPEGCtx.m_MBX == 0 ) {

     *apX = lpC[ 0 ];
     *apY = lpC[ 1 ];

    } else {

     *apX = SMS_mid_pred ( lpA[ 0 ], 0, lpC[ 0 ] );
     *apY = SMS_mid_pred ( lpA[ 1 ], 0, lpC[ 1 ] );

    }  /* end else */

   } else {

    *apX = lpA[ 0 ];
    *apY = lpA[ 1 ];

   }  /* end else */

  } else if ( aBlock == 1 ) {

   if ( g_MPEGCtx.m_MBX + 1 == g_MPEGCtx.m_ResyncMBX && g_MPEGCtx.m_H263Pred ) {

    lpC = lpMotVal[  lOff[ aBlock ] - lWrap  ];

    *apX = SMS_mid_pred ( lpA[ 0 ], 0, lpC[ 0 ] );
    *apY = SMS_mid_pred ( lpA[ 1 ], 0, lpC[ 1 ] );

   } else {

    *apX = lpA[ 0 ];
    *apY = lpA[ 1 ];

   }  /* end else */

  } else {

   lpB = lpMotVal[ -lWrap ];
   lpC = lpMotVal[  lOff[ aBlock ] - lWrap  ];

   if ( g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX ) lpA[ 0 ] = lpA[ 1 ] = 0;
    
   *apX = SMS_mid_pred ( lpA[ 0 ], lpB[ 0 ], lpC[ 0 ] );
   *apY = SMS_mid_pred ( lpA[ 1 ], lpB[ 1 ], lpC[ 1 ] );

  }  /* end else */

 } else {

  lpB = lpMotVal[ -lWrap ];
  lpC = lpMotVal[  lOff[ aBlock ] - lWrap  ];

  *apX = SMS_mid_pred ( lpA[ 0 ], lpB[ 0 ], lpC[ 0 ] );
  *apY = SMS_mid_pred ( lpA[ 1 ], lpB[ 1 ], lpC[ 1 ] );

 }  /* end else */

 return *lpMotVal;

}  /* end SMS_H263_PredMotion */

void SMS_H263_UpdateMotionVal ( void ) {

 const int lMBXY = g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBX;
 const int lWrap = g_MPEGCtx.m_B8Stride;
 const int lXY   = g_MPEGCtx.m_BlockIdx[ 0 ];

 g_MPEGCtx.m_CurPic.m_pMBSkipTbl[ lMBXY ] = g_MPEGCtx.m_MBSkiped; 

 if ( g_MPEGCtx.m_MVType != SMS_MV_TYPE_8X8 ) {

  int lMotionX, lMotionY;

  if ( g_MPEGCtx.m_MBIntra ) {

   lMotionX = 0;
   lMotionY = 0;

  } else if ( g_MPEGCtx.m_MVType == SMS_MV_TYPE_16X16 ) {

   lMotionX = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ];
   lMotionY = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ];

  } else {

   int i;

   lMotionX = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] + g_MPEGCtx.m_MV[ 0 ][ 1 ][ 0 ];
   lMotionY = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] + g_MPEGCtx.m_MV[ 0 ][ 1 ][ 1 ];
   lMotionX = ( lMotionX >> 1 ) | ( lMotionX & 1 );

   for ( i = 0; i < 2; ++i ) {

    g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBXY ][ 0 ] = g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ];
    g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBXY ][ 1 ] = g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ];

   } /* end for */

   g_MPEGCtx.m_CurPic.m_pRefIdx[ 0 ][ lXY             ] =
   g_MPEGCtx.m_CurPic.m_pRefIdx[ 0 ][ lXY + 1         ] = g_MPEGCtx.m_FieldSelect[ 0 ][ 0 ];
   g_MPEGCtx.m_CurPic.m_pRefIdx[ 0 ][ lXY + lWrap     ] =
   g_MPEGCtx.m_CurPic.m_pRefIdx[ 0 ][ lXY + lWrap + 1 ] = g_MPEGCtx.m_FieldSelect[ 0 ][ 1 ];

  }  /* end else */

  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY             ][ 0 ] = lMotionX;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY             ][ 1 ] = lMotionY;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + 1         ][ 0 ] = lMotionX;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + 1         ][ 1 ] = lMotionY;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + lWrap     ][ 0 ] = lMotionX;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + lWrap     ][ 1 ] = lMotionY;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + 1 + lWrap ][ 0 ] = lMotionX;
  g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][ lXY + 1 + lWrap ][ 1 ] = lMotionY;

 }  /* end if */

}  /* end SMS_H263_UpdateMotionVal */

int SMS_H263_RoundChroma ( int aX ) {
 int retVal;
 __asm__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "la       %0, s_h263_chroma_roundtab\n\t"   /* v0 = s_h263_chroma_roundtab */
  "pextlw   $at, $zero, $a0\n\t"
  "addiu    $a1, $zero, -2\n\t"               /* a1 = ~1                     */
  "pabsw    $at, $at\n\t"                     /* at = abs ( aX )             */
  "andi     $v1, $at, 0x000F\n\t"             /* v1 = aX & 0xF               */
  "sra      $at, $at, 3\n\t"                  /* at = aX >> 3                */
  "addu     %0, %0, $v1\n\t"                  /* v0 = result                 */
  "lbu      %0,  0(%0)\n\t"
  "and      $at, $at, $a1\n\t"                /* at = ( aX >> 3 ) & ~1       */
  "addu     %0, %0, $at\n\t"
  "bltzl    $a0, 1f\n\t"
  "negu     %0, %0\n\t"
  "1:\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end SMS_H263_RoundChroma */
