/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001 Fabrice Bellard.
# Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_VLC.h"

#include <malloc.h>
#include <string.h>

SMS_VLC g_SMS_mv_vlc;

#define SMS_GET_DATA( v, table, i, wrap, size ) {          \
 const uint8_t* lPtr = ( const uint8_t* )table + i * wrap; \
 switch ( size ) {                                         \
  case 1:                                                  \
   v = *( const uint8_t*  )lPtr;                           \
  break;                                                   \
  case 2:                                                  \
   v = *( const uint16_t* )lPtr;                           \
  break;                                                   \
  default:                                                 \
   v = *( const uint32_t* )lPtr;                           \
  break;                                                   \
 }                                                         \
}

static int _SMS_VLC_AllocTable ( SMS_VLC* apVLC, int aSize ) {

 int lIdx = apVLC -> m_TableSize;

 apVLC -> m_TableSize += aSize;

 if ( apVLC -> m_TableSize > apVLC -> m_TableAlloc ) {

  apVLC -> m_TableAlloc += ( 1 << apVLC -> m_Bits );
  apVLC -> m_pTable = realloc (
   apVLC -> m_pTable, sizeof ( int16_t ) * 2 * apVLC -> m_TableAlloc
  );

  if ( !apVLC -> m_pTable ) return -1;

 }  /* end if */

 return lIdx;

}  /* end _SMS_VLC_AllocTable */

static int _SMS_VLC_BuildTable (
 SMS_VLC* apVLC, int anBits, int anCodes,
 const void* apBits,  int aBitsWrap,  int aBitsSize,
 const void* apCodes, int aCodesWrap, int aCodesSize,
 uint32_t aCodePrefix, int aPrefix
) {

 int      i, j, k, n, n1, lTableSize, lTableIdx, lNb, lIdx;
 uint32_t lCode;
 int16_t  ( *lpTable )[ 2 ];

 lTableSize = 1 << anBits;
 lTableIdx  = _SMS_VLC_AllocTable ( apVLC, lTableSize );

 if ( lTableIdx < 0 ) return -1;

 lpTable = &apVLC -> m_pTable[ lTableIdx ];

 for ( i = 0; i < lTableSize; ++i ) {

  lpTable[ i ][ 1 ] =  0;
  lpTable[ i ][ 0 ] = -1;

 }  /* end for */

 for ( i = 0; i < anCodes; ++i ) {

  SMS_GET_DATA( n,     apBits,  i, aBitsWrap,  aBitsSize  );
  SMS_GET_DATA( lCode, apCodes, i, aCodesWrap, aCodesSize );

  if ( n <= 0 ) continue;

  n -= aPrefix;

  if (  n > 0 && ( lCode >> n ) == aCodePrefix  ) {

   if ( n <= anBits ) {

    j   = (  lCode << ( anBits - n )  ) & ( lTableSize - 1 );
    lNb = 1 << ( anBits - n );

    for ( k = 0; k < lNb; ++k ) {

     lpTable[ j ][ 0 ] = i;
     lpTable[ j ][ 1 ] = n;
     ++j;

    }  /* end for */

   } else {

    n -= anBits;
    j  = ( lCode >> n ) & (  ( 1 << anBits ) - 1  );
    n1 = -lpTable[ j ][ 1 ];

    if ( n > n1 ) n1 = n;

    lpTable[ j ][ 1 ] = -n1;

   }  /* end else */

  }  /* end if */

 }  /* end for */

 for ( i = 0; i < lTableSize; ++i ) {

  n = lpTable[ i ][ 1 ];

  if ( n < 0 ) {

   n = -n;

   if ( n > anBits ) {

    n                 = anBits;
    lpTable[ i ][ 1 ] = -n;

   }  /* end if */

   lIdx = _SMS_VLC_BuildTable (
    apVLC, n, anCodes,
    apBits,  aBitsWrap,  aBitsSize,
    apCodes, aCodesWrap, aCodesSize,
    ( aCodePrefix << anBits ) | i, aPrefix + anBits
   );

   if ( lIdx < 0 ) return -1;

   lpTable           = &apVLC -> m_pTable[ lTableIdx ];
   lpTable[ i ][ 0 ] = lIdx;

  }  /* end if */

 }  /* end for */

 return lTableIdx;

}  /* end _SMS_VLC_BuildTable */

int SMS_VLC_Init (
 SMS_VLC* apVLC, int anBits, int anCodes,
 const void* apBits,  int aBitsWrap,  int aBitsSize,
 const void* apCodes, int aCodesWrap, int aCodesSize
) {

 apVLC -> m_Bits       = anBits;
 apVLC -> m_pTable     = 0;
 apVLC -> m_TableAlloc = 0;
 apVLC -> m_TableSize  = 0;

 if (  _SMS_VLC_BuildTable (
        apVLC, anBits, anCodes,
        apBits,  aBitsWrap,  aBitsSize,
        apCodes, aCodesWrap, aCodesSize,
        0, 0
       ) < 0
 ) {

  free ( apVLC -> m_pTable );
  return 0;

 }  /* end if */

 return 1;

}  /* end SMS_VLC_Init */

void SMS_VLC_Free ( SMS_VLC* apVLC ) {

 free ( apVLC -> m_pTable );

}  /* end SMS_VLC_Free */

void SMS_RL_Init ( SMS_RLTable* apRL ) {

 int8_t lMaxLevel[ SMS_MAX_RUN + 1 ], lMaxRun[ SMS_MAX_LEVEL + 1 ];
 int    i, lLast, lRun, lLevel, lStart, lEnd;

 for ( lLast = 0; lLast < 2; ++lLast ) {

  if ( lLast == 0 ) {

   lStart = 0;
   lEnd   = apRL -> m_Last;

  } else {

   lStart = apRL -> m_Last;
   lEnd   = apRL -> m_n;

  }  /* end else */

  memset ( lMaxLevel, 0, SMS_MAX_RUN   + 1 );
  memset ( lMaxRun,   0, SMS_MAX_LEVEL + 1 );

  for ( i = lStart; i < lEnd; ++i ) {

   lRun   = apRL -> m_pTableRun  [ i ];
   lLevel = apRL -> m_pTableLevel[ i ];

   if ( lLevel > lMaxLevel[ lRun   ] ) lMaxLevel[ lRun   ] = lLevel;
   if ( lRun   > lMaxRun  [ lLevel ] ) lMaxRun  [ lLevel ] = lRun;

  }  /* end for */

  apRL -> m_pMaxLevel[ lLast ] = malloc ( SMS_MAX_RUN + 1 );
  memcpy ( apRL -> m_pMaxLevel[ lLast], lMaxLevel, SMS_MAX_RUN + 1 );

  apRL -> m_pMaxRun[ lLast ] = malloc ( SMS_MAX_LEVEL + 1 );
  memcpy ( apRL -> m_pMaxRun[ lLast ], lMaxRun, SMS_MAX_LEVEL + 1 );

 }  /* end for */

}  /* end SMS_RL_Init */

void SMS_RL_Free ( SMS_RLTable* apRL ) {

 int i;

 for ( i = 0; i < 2; ++i ) {

  free ( apRL -> m_pMaxLevel[ i ] );
  free ( apRL -> m_pMaxRun  [ i ] );

 }  /* end for */

}  /* end SMS_RL_Free */

void SMS_VLC_RL_Init ( SMS_RLTable* apRL ) {

 int i, q;
    
 SMS_VLC_Init (
  &apRL -> m_VLC, 9, apRL -> m_n + 1, 
  &apRL -> m_pTableVLC[ 0 ][ 1 ], 4, 2,
  &apRL -> m_pTableVLC[ 0 ][ 0 ], 4, 2
 );
    
 for ( q = 0; q < 32; ++q ) {

  int lQMul = q * 2;
  int lQAdd = ( q - 1 ) | 1;
        
  if ( q == 0 ) {

   lQMul = 1;
   lQAdd = 0;

  }  /* end if */
        
  apRL -> m_pRLVLC[ q ] = malloc (
   apRL -> m_VLC.m_TableSize * sizeof ( SMS_RL_VLC_ELEM )
  );

  for ( i = 0; i < apRL -> m_VLC.m_TableSize; ++i ) {

   int lCode = apRL -> m_VLC.m_pTable[ i ][ 0 ];
   int lLen  = apRL -> m_VLC.m_pTable[ i ][ 1 ];
   int lLevel, lRun;
        
   if ( lLen == 0 ) {

    lRun   = 66;
    lLevel = SMS_MAX_LEVEL;

   } else if ( lLen < 0 ) {

    lRun   = 0;
    lLevel = lCode;

   } else {

    if ( lCode == apRL -> m_n ) {

     lRun   = 66;
     lLevel = 0;

    } else {

     lRun   = apRL -> m_pTableRun  [ lCode ] + 1;
     lLevel = apRL -> m_pTableLevel[ lCode ] * lQMul + lQAdd;

     if ( lCode >= apRL -> m_Last ) lRun += 192;

    }  /* end else */

   }  /* end else */

   apRL -> m_pRLVLC[ q ][ i ].m_Len   = lLen;
   apRL -> m_pRLVLC[ q ][ i ].m_Level = lLevel;
   apRL -> m_pRLVLC[ q ][ i ].m_Run   = lRun;

  }  /* end for */

 }  /* end for */

}  /* end SMS_VLC_RL_Init */

void SMS_VLC_RL_Free ( SMS_RLTable* apRL ) {

 int i;

 SMS_VLC_Free ( &apRL -> m_VLC );

 for ( i = 0; i < 32; ++i ) free ( apRL -> m_pRLVLC[ i ] );

}  /* end SMS_VLC_RL_Free */
