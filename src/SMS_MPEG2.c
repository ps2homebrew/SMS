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

void SMS_MPEG2_DCTUnquantizeIntra ( SMS_DCTELEM* apBlock, int aN, int aQScale ) {

 const uint16_t* lpQuantMatrix;

 int i, lLevel, lnCoeffs;

 lnCoeffs = g_MPEGCtx.m_AltScan ? 63 : g_MPEGCtx.m_BlockLastIdx[ aN ];

 apBlock[ 0 ] *= aN < 4 ? g_MPEGCtx.m_Y_DCScale : g_MPEGCtx.m_C_DCScale;

 lpQuantMatrix = g_MPEGCtx.m_IntraMatrix;

 for ( i = 1; i <= lnCoeffs; ++i ) {

  int j = g_MPEGCtx.m_IntraScanTbl.m_pScantable[ i ];

  lLevel = apBlock[ j ];

  if ( lLevel ) {

   if ( lLevel < 0 ) {

    lLevel = -lLevel;
    lLevel = ( int )( lLevel * aQScale * lpQuantMatrix[ j ] ) >> 3;
    lLevel = -lLevel;

   } else lLevel = ( int )( lLevel * aQScale * lpQuantMatrix[ j ] ) >> 3;

   apBlock[ j ] = lLevel;

  }  /* end if */

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
