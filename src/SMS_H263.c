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

 if ( aX >= 0 )

  return s_h263_chroma_roundtab[ aX & 0xF ] + (  ( aX >> 3 ) & ~1  );

 else {

  aX = -aX;

  return -(   s_h263_chroma_roundtab[ aX & 0xF ] + (  ( aX >> 3 ) & ~1  )   );

 }  /* end else */

}  /* end SMS_H263_RoundChroma */
#ifdef _WIN32
void SMS_H263_DCTUnquantizeIntra ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 int i;
 int lLevel, lQMul, lQAdd, lnCoeffs;

 lQMul    = aQScale << 1;
 lQAdd    = ( aQScale - 1 ) | 1;
 lnCoeffs = g_MPEGCtx.m_ACPred ? 63 : g_MPEGCtx.m_InterScanTbl.m_RasterEnd[  g_MPEGCtx.m_BlockLastIdx[ aN ]  ];

 apBlock[ 0 ] *= aN < 4 ? g_MPEGCtx.m_Y_DCScale : g_MPEGCtx.m_C_DCScale;

 for ( i = 1; i <= lnCoeffs; ++i ) {

  lLevel = apBlock[ i ];

  if ( lLevel ) {

   lLevel = lLevel < 0 ? ( lLevel * lQMul - lQAdd ) : ( lLevel * lQMul + lQAdd );
   apBlock[ i ] = lLevel;

  }  /* end if */

 }  /* end for */

}  /* end SMS_H263_DCTUnquantizeIntra */

void SMS_H263_DCTUnquantizeInter ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 int i, lLevel;

 int lQMul, lQAdd, lnCoeffs;

 lQAdd = ( aQScale - 1 ) | 1;
 lQMul = aQScale << 1;
    
 lnCoeffs = g_MPEGCtx.m_InterScanTbl.m_RasterEnd[  g_MPEGCtx.m_BlockLastIdx[ aN ]  ];

 for ( i = 0; i <= lnCoeffs; ++i ) {

  lLevel = apBlock[ i ];

  if ( lLevel ) {

   if ( lLevel < 0 )

    lLevel = lLevel * lQMul - lQAdd;

   else lLevel = lLevel * lQMul + lQAdd;

   apBlock[ i ] = lLevel;

  }  /* end if */

 }  /* end for */

}  /* end SMS_H263_DCTUnquantizeInter */
#else  /* PS2 */
void SMS_H263_DCTUnquantizeIntra ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 const int lQMul    = aQScale << 1;
 const int lQAdd    = ( aQScale - 1 ) | 1;
 const int lnCoeffs = 63;
 const int lLevel   = aN < 4 ? apBlock[ 0 ] * g_MPEGCtx.m_Y_DCScale
                             : apBlock[ 0 ] * g_MPEGCtx.m_C_DCScale;
 __asm__ __volatile__ (

  "add      $14, $0, %3\n\t"
  "pcpyld   $8, %0, %0\n\t"	
  "pcpyh    $8, $8\n\t"
  "pcpyld   $9, %1, %1\n\t"
  "pcpyh    $9, $9\n\t"
  "1:\n\t"
  "lq       $10, 0($14)\n\t"
  "addi     $14, $14, 16\n\t"
  "addi     %2, %2, -8\n\t"
  "pcgth    $11, $0, $10\n\t"
  "pcgth    $12, $10, $0\n\t"
  "por      $12, $11, $12\n\t"
  "pmulth   $10, $10, $8\n\t"	
  "paddh    $13, $9, $11\n\t"
  "pxor     $13, $13, $11\n\t"
  "pmfhl.uw $11\n\t"
  "pinteh   $10, $11, $10\n\t"
  "paddh    $10, $10, $13\n\t"
  "pand     $10, $10, $12\n\t"
  "sq       $10, -16($14)\n\t"
  "bgez     %2, 1b\n\t"
  :: "r"( lQMul ), "r"( lQAdd ), "r"( lnCoeffs ), "r"( apBlock )
   : "$8", "$9", "$10", "$11", "$12", "$13", "$14", "memory"
 );

 apBlock[ 0 ] = lLevel;

}  /* end SMS_H263_DCTUnquantizeIntra */

void SMS_H263_DCTUnquantizeInter ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 const int lQAdd    = ( aQScale - 1 ) | 1;
 const int lQMul    = aQScale << 1;
 const int lnCoeffs = g_MPEGCtx.m_InterScanTbl.m_RasterEnd[  g_MPEGCtx.m_BlockLastIdx[ aN ]  ];

 __asm__ __volatile__ (

  "add      $14, $0, %3\n\t"
  "pcpyld   $8, %0, %0\n\t"	
  "pcpyh    $8, $8\n\t"
  "pcpyld   $9, %1, %1\n\t"
  "pcpyh    $9, $9\n\t"
  "1:\n\t"
  "lq       $10, 0($14)\n\t"
  "addi     $14, $14, 16\n\t"
  "addi     %2, %2, -8\n\t"
  "pcgth    $11, $0, $10\n\t"
  "pcgth    $12, $10, $0\n\t"
  "por      $12, $11, $12\n\t"
  "pmulth   $10, $10, $8\n\t"	
  "paddh    $13, $9, $11\n\t"
  "pxor     $13, $13, $11\n\t"
  "pmfhl.uw $11\n\t"
  "pinteh   $10, $11, $10\n\t"
  "paddh    $10, $10, $13\n\t"
  "pand     $10, $10, $12\n\t"
  "sq       $10, -16($14)\n\t"
  "bgez     %2, 1b\n\t"
  :: "r"( lQMul ), "r"( lQAdd ), "r"( lnCoeffs ), "r"( apBlock )
   : "$8", "$9", "$10", "$11", "$12", "$13", "$14", "memory"
 );

}  /* end SMS_H263_DCTUnquantizeInter */
#endif  /* _WIN32 */
