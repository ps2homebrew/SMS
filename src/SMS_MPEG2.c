/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000,2001 Fabrice Bellard.
# Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
# Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_MPEG.h"
#include "SMS_MPEG2.h"

void SMS_MPEG2_DCTUnquantizeIntra ( SMS_DCTELEM* apBlock ) {

 const uint16_t* lpQuantMatrix;

 int i, j, lLevel, lnCoeffs, lQScale[ 6 ], lDCScale[ 6 ];

 lQScale[ 0 ] = 
 lQScale[ 1 ] = 
 lQScale[ 2 ] = 
 lQScale[ 3 ] = g_MPEGCtx.m_QScale;
 lQScale[ 4 ] =
 lQScale[ 5 ] = g_MPEGCtx.m_ChromaQScale;

 lDCScale[ 0 ] = 
 lDCScale[ 1 ] = 
 lDCScale[ 2 ] = 
 lDCScale[ 3 ] = g_MPEGCtx.m_Y_DCScale;
 lDCScale[ 4 ] = 
 lDCScale[ 5 ] = g_MPEGCtx.m_C_DCScale;

 lpQuantMatrix = g_MPEGCtx.m_IntraMatrix;

 for ( j = 0; j < 6; ++j ) {

  const int lQS = lQScale[ j ];

  apBlock[ 0 ] *= lDCScale[ j ];

  lnCoeffs = g_MPEGCtx.m_AltScan ? 63 : g_MPEGCtx.m_BlockLastIdx[ j ];

  for ( i = 1; i <= lnCoeffs; ++i ) {

   int j = g_MPEGCtx.m_IntraScanTbl.m_pScantable[ i ];

   lLevel = apBlock[ j ];

   if ( lLevel ) {

    if ( lLevel < 0 ) {

     lLevel = -lLevel;
     lLevel = ( int )( lLevel * lQS * lpQuantMatrix[ j ] ) >> 3;
     lLevel = -lLevel;

    } else lLevel = ( int )( lLevel * lQS * lpQuantMatrix[ j ] ) >> 3;

    apBlock[ j ] = lLevel;

   }  /* end if */

  }  /* end for */

  apBlock += 64;

 }  /* end for */

}  /* end SMS_MPEG2_DCTUnquantizeIntra */

void SMS_MPEG2_DCTUnquantizeInter ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 const uint16_t* lpQuantMatrix;

 int i, lLevel, lnCoeffs;
 int lSum = -1;

 lnCoeffs      = g_MPEGCtx.m_AltScan ? 63 : g_MPEGCtx.m_BlockLastIdx[ aN ];
 lpQuantMatrix = g_MPEGCtx.m_InterMatrix;

 for ( i = 0; i <= lnCoeffs; ++i ) {

  int j = g_MPEGCtx.m_IntraScanTbl.m_pScantable[ i ];

  lLevel = apBlock[ j ];

  if ( lLevel ) {

   if ( lLevel < 0 ) {

    lLevel = -lLevel;
    lLevel = (   (  ( lLevel << 1 ) + 1  ) * aQScale * (  ( int )( lpQuantMatrix[ j ] )  )   ) >> 4;
    lLevel = -lLevel;

   } else lLevel = (   (  ( lLevel << 1 ) + 1  ) * aQScale * (  ( int )( lpQuantMatrix[ j ] )  )   ) >> 4;

   apBlock[ j ] = lLevel;
   lSum        += lLevel;

  }  /* end if */

 }  /* end for */

 apBlock[ 63 ] ^= lSum & 1;

}  /* end SMS_MPEG2_DCTUnquantizeInter */
