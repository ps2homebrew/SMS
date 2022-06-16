/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000,2001 Fabrice Bellard.
# Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
# Copyright (c) 2005 Eugene Plotnikov (PS2 specific code)
# This code is licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/
#include "SMS_MPEG.h"
#include "SMS_H263.h"
#include "SMS_VideoBuffer.h"
#include "SMS_DMA.h"

#include <malloc.h>
#include <string.h>
#include <limits.h>

void SMS_MPEG2_DCTUnquantizeIntra ( SMS_DCTELEM* );
void SMS_MPEG2_DCTUnquantizeInter ( SMS_DCTELEM* );

SMS_MPEGContext g_MPEGCtx __attribute__(   (  aligned( 64 )  )   );

static SMS_INLINE void _mpeg_alloc_picture ( SMS_Frame* apPic ) {

 int i;

 SMS_CodecGetBuffer ( g_MPEGCtx.m_pParentCtx, apPic );

 if ( apPic -> m_pQScaleTbl == NULL ) {

  const int lBigMBNr   = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBH + 1 ) + 1;
  const int lMBArrSize = g_MPEGCtx.m_MBStride * g_MPEGCtx.m_MBH;
  const int lB8ArrSize = g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2;

  apPic -> m_pMBSkipTbl = calloc (  1, lMBArrSize * sizeof ( uint8_t  ) + 2  );
  apPic -> m_pQScaleTbl = calloc (  1, lMBArrSize * sizeof ( uint8_t  )      );
  apPic -> m_pMBType    = calloc (  1, lBigMBNr   * sizeof ( uint32_t )      );

  for ( i = 0; i < 2; ++i ) {

   apPic -> m_pMotionValBase[ i ] = calloc (  1, 2 * ( lB8ArrSize + 2 ) * sizeof ( int16_t )  );
   apPic -> m_pMotionVal    [ i ] = apPic -> m_pMotionValBase[ i ] + 2;
   apPic -> m_pRefIdx       [ i ] = calloc (  1, lB8ArrSize * sizeof ( uint8_t )  );

  }  /* end for */

 }  /* end if */

 apPic -> m_Width  = g_MPEGCtx.m_Width;
 apPic -> m_Height = g_MPEGCtx.m_Height;

}  /* end _mpeg_alloc_picture */

void SMS_MPEG_SetQScale ( int aQScale ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "addiu    $v0, $zero,  1\n\t"
  "addiu    $v1, $zero, 31\n\t"
  "pmaxw    $a0, $a0, $v0\n\t"
  "pminw    $a0, $a0, $v1\n\t"
  "lw       $v0, %2\n\t"
  "sb       $a0, %0\n\t"
  "addu     $v0, $v0, $a0\n\t"
  "lw       $a1, %4\n\t"
  "lbu      $v1, 0($v0)\n\t"
  "addu     $a1, $a1, $a0\n\t"
  "lw       $v0, %6\n\t"
  "sb       $v1, %1\n\t"
  "lbu      $a1, 0($a1)\n\t"
  "addu     $v0, $v0, $v1\n\t"
  "lbu      $v0, 0($v0)\n\t"
  "sb       $a1, %3\n\t"
  "sb       $v0, %5\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "m"( g_MPEGCtx.m_QScale           ),
     "m"( g_MPEGCtx.m_ChromaQScale     ),
     "m"( g_MPEGCtx.m_pChromaQScaleTbl ),
     "m"( g_MPEGCtx.m_Y_DCScale        ),
     "m"( g_MPEGCtx.m_pY_DCScaleTbl    ),
     "m"( g_MPEGCtx.m_C_DCScale        ),
     "m"( g_MPEGCtx.m_pC_DCScaleTbl    ),
     "m"( g_MPEGCtx.m_MPEGQuant        )
 );
}  /* end SMS_MPEG_SetQScale */

void SMS_MPEG_CleanIntraTblEntries ( void ) {

 int lWrap = g_MPEGCtx.m_B8Stride;
 int lXY0  = g_MPEGCtx.m_BlockIdx[ 0 ];
 int lXY1  = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;

 g_MPEGCtx.m_pDCVal[ 0 ][ lXY0             ] = 1024;
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY0 + 1         ] = 1024;
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY0 + lWrap     ] = 1024;
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY0 + 1 + lWrap ] = 1024;
 g_MPEGCtx.m_pDCVal[ 1 ][ lXY1             ] = 1024;
 g_MPEGCtx.m_pDCVal[ 2 ][ lXY1             ] = 1024;

 __asm__(
  "sq   $zero,  0(%0)\n\t"
  "sq   $zero, 16(%0)\n\t"
  "sq   $zero, 32(%0)\n\t"
  "sq   $zero, 48(%0)\n\t"
  "sq   $zero,  0(%1)\n\t"
  "sq   $zero, 16(%1)\n\t"
  "sq   $zero, 32(%1)\n\t"
  "sq   $zero, 48(%1)\n\t"
  "addu %4, %4, %5\n\t"
  "sq   $zero,  0(%2)\n\t"
  "sq   $zero, 16(%2)\n\t"
  "sq   $zero,  0(%3)\n\t"
  "sq   $zero, 16(%3)\n\t"
  "sb   $zero,  0(%4)\n\t"
  :: "r"( g_MPEGCtx.m_pACVal[ 0 ][ lXY0 ] ), "r"( g_MPEGCtx.m_pACVal[ 0 ][ lXY0 + lWrap ] ),
     "r"( g_MPEGCtx.m_pACVal[ 1 ][ lXY1 ] ), "r"( g_MPEGCtx.m_pACVal[ 2 ][ lXY1 ] ),
     "r"( g_MPEGCtx.m_pMBIntraTbl ), "r"( lXY1 )
 );

}  /* end SMS_MPEG_CleanIntraTblEntries */

static SMS_INLINE SMS_Frame* SMS_MPEGContext_FindUnusedPic ( void ) {

 int i;
    
 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

  if ( g_MPEGCtx.m_pPic[ i ].m_pBuf  == NULL &&
       g_MPEGCtx.m_pPic[ i ].m_Type  != 0
  ) return &g_MPEGCtx.m_pPic[ i ];

 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )
  if ( g_MPEGCtx.m_pPic[ i ].m_pBuf == NULL ) return &g_MPEGCtx.m_pPic[ i ];

 return NULL;

}  /* end SMS_MPEGContext_FindUnusedPic */

void SMS_MPEG_FrameStart ( void ) {

 int        i;
 SMS_Frame* lpPic;

 g_MPEGCtx.m_MBSkiped = 0;

 if ( g_MPEGCtx.m_PicType  != SMS_FT_B_TYPE        &&
      g_MPEGCtx.m_pLastPic != NULL                 &&
      g_MPEGCtx.m_pLastPic != g_MPEGCtx.m_pNextPic &&
      g_MPEGCtx.m_pLastPic -> m_pBuf
 ) {

  SMS_CodecReleaseBuffer ( g_MPEGCtx.m_pParentCtx, g_MPEGCtx.m_pLastPic );

  for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

   if (  g_MPEGCtx.m_pPic[ i ].m_pBuf                  &&
        &g_MPEGCtx.m_pPic[ i ] != g_MPEGCtx.m_pNextPic &&
         g_MPEGCtx.m_pPic[ i ].m_Ref
   ) SMS_CodecReleaseBuffer ( g_MPEGCtx.m_pParentCtx, &g_MPEGCtx.m_pPic[ i ] );

 }  /* end if */

 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

  if (  g_MPEGCtx.m_pPic[ i ].m_pBuf &&
       !g_MPEGCtx.m_pPic[ i ].m_Ref
  ) SMS_CodecReleaseBuffer ( g_MPEGCtx.m_pParentCtx, &g_MPEGCtx.m_pPic[ i ] );

 if ( g_MPEGCtx.m_pCurPic && g_MPEGCtx.m_pCurPic -> m_pBuf == NULL )

  lpPic = g_MPEGCtx.m_pCurPic;

 else lpPic = SMS_MPEGContext_FindUnusedPic ();

 lpPic -> m_Ref = g_MPEGCtx.m_PicType != SMS_FT_B_TYPE;

 _mpeg_alloc_picture ( lpPic );

 g_MPEGCtx.m_pCurPic = lpPic;
 lpPic -> m_Type     = g_MPEGCtx.m_PicType;

 g_MPEGCtx.m_CurPic = *g_MPEGCtx.m_pCurPic;

 if ( g_MPEGCtx.m_PicType != SMS_FT_B_TYPE ) {

  g_MPEGCtx.m_pLastPic = g_MPEGCtx.m_pNextPic;
  g_MPEGCtx.m_pNextPic = g_MPEGCtx.m_pCurPic;

 }  /* end if */

 if ( g_MPEGCtx.m_pLastPic ) g_MPEGCtx.m_LastPic = *g_MPEGCtx.m_pLastPic;
 if ( g_MPEGCtx.m_pNextPic ) g_MPEGCtx.m_NextPic = *g_MPEGCtx.m_pNextPic;

 if ( !g_MPEGCtx.m_MPEGQuant ) {

  g_MPEGCtx.DCT_UnquantizeIntra = SMS_H263_DCTUnquantizeIntra;
  g_MPEGCtx.DCT_UnquantizeInter = SMS_H263_DCTUnquantizeInter;

 } else {

  g_MPEGCtx.DCT_UnquantizeIntra = SMS_MPEG2_DCTUnquantizeIntra;
  g_MPEGCtx.DCT_UnquantizeInter = SMS_MPEG2_DCTUnquantizeInter;

 }  /* end else */

 __asm__(  "sd   $zero, %0\n\t" :: "m"( g_MPEGCtx.m_MBX )  );

 g_MPEGCtx.MBCallback = SMS_MPEG_DummyCB;

}  /* end SMS_MPEG_FrameStart */

void SMS_MPEG_FrameEnd ( void ) {

 if ( g_MPEGCtx.m_CurPic.m_Ref ) {

  int             i, j;
  SMS_MacroBlock* lpMBDstL = g_MPEGCtx.m_CurPic.m_pBuf -> m_pBase;
  SMS_MacroBlock* lpMBSrcL = g_MPEGCtx.m_CurPic.m_pBuf -> m_pData;
  SMS_MacroBlock* lpMBDstR;
  SMS_MacroBlock* lpMBSrcR;
  u128*           lpDstY;
  uint64_t*       lpDstUV;
  uint64_t        lSrcUV;
  u128            lSrcY;
/* left top */
  memset ( &lpMBDstL -> m_Y [ 0 ][ 0 ], lpMBSrcL -> m_Y [ 0 ][ 0 ], 256 );
  memset ( &lpMBDstL -> m_Cb[ 0 ][ 0 ], lpMBSrcL -> m_Cb[ 0 ][ 0 ],  64 );
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[ 0 ][ 0 ],  64 );
/* top */
  ++lpMBDstL;

  for ( i = 0; i < g_MPEGCtx.m_MBW; ++i ) {

   lSrcY  = *( u128* )&lpMBSrcL -> m_Y[ 0 ][ 0 ];
   lpDstY =  ( u128* )&lpMBDstL -> m_Y[ 0 ][ 0 ];

   lpDstY[  0 ] = lSrcY;
   lpDstY[  1 ] = lSrcY;
   lpDstY[  2 ] = lSrcY;
   lpDstY[  3 ] = lSrcY;
   lpDstY[  4 ] = lSrcY;
   lpDstY[  5 ] = lSrcY;
   lpDstY[  6 ] = lSrcY;
   lpDstY[  7 ] = lSrcY;
   lpDstY[  8 ] = lSrcY;
   lpDstY[  9 ] = lSrcY;
   lpDstY[ 10 ] = lSrcY;
   lpDstY[ 11 ] = lSrcY;
   lpDstY[ 12 ] = lSrcY;
   lpDstY[ 13 ] = lSrcY;
   lpDstY[ 14 ] = lSrcY;
   lpDstY[ 15 ] = lSrcY;

   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cb[ 0 ][ 0 ];
   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cb[ 0 ][ 0 ];

   __asm__(
    ".set noat\n\t"
    "pcpyld $at, %1, %1\n\t"
    "sq     $at,  0(%0)\n\t"
    "sq     $at, 16(%0)\n\t"
    "sq     $at, 32(%0)\n\t"
    "sq     $at, 48(%0)\n\t"
    ".set at\n\t"
    :: "r"( lpDstUV ), "r"( lSrcUV ) : "at"
   );

   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cr[ 0 ][ 0 ];
   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cr[ 0 ][ 0 ];

   __asm__(
    ".set noat\n\t"
    "pcpyld $at, %1, %1\n\t"
    "sq     $at,  0(%0)\n\t"
    "sq     $at, 16(%0)\n\t"
    "sq     $at, 32(%0)\n\t"
    "sq     $at, 48(%0)\n\t"
    ".set at\n\t"
    :: "r"( lpDstUV ), "r"( lSrcUV ) : "at"
   );

   ++lpMBSrcL;
   ++lpMBDstL;

  }  /* end for */

  --lpMBSrcL;
/* right top */
  memset ( &lpMBDstL -> m_Y [ 0 ][ 0 ], lpMBSrcL -> m_Y [ 0 ][ 15 ], 256 );
  memset ( &lpMBDstL -> m_Cb[ 0 ][ 0 ], lpMBSrcL -> m_Cb[ 0 ][  7 ],  64 );
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[ 0 ][  7 ],  64 );

  lpMBSrcL = g_MPEGCtx.m_CurPic.m_pBuf -> m_pData;
  lpMBDstL = lpMBSrcL - 1;
  lpMBSrcR = lpMBSrcL + g_MPEGCtx.m_MBW - 1;
  lpMBDstR = lpMBSrcR + 1;
/* left and right */
  for ( i = 0; i < g_MPEGCtx.m_MBH; ++i ) {

   for ( j = 0; j < 16; ++j ) {

    memset ( &lpMBDstL -> m_Y[ j ][ 0 ], lpMBSrcL -> m_Y[ j ][  0 ], 16 );
    memset ( &lpMBDstR -> m_Y[ j ][ 0 ], lpMBSrcR -> m_Y[ j ][ 15 ], 16 );

   }  /* end for */

   for ( j = 0; j < 8; ++j ) {

    memset ( &lpMBDstL -> m_Cb[ j ][ 0 ], lpMBSrcL -> m_Cb[ j ][ 0 ], 8 );
    memset ( &lpMBDstR -> m_Cb[ j ][ 0 ], lpMBSrcR -> m_Cb[ j ][ 7 ], 8 );

    memset ( &lpMBDstL -> m_Cr[ j ][ 0 ], lpMBSrcL -> m_Cr[ j ][ 0 ], 8 );
    memset ( &lpMBDstR -> m_Cr[ j ][ 0 ], lpMBSrcR -> m_Cr[ j ][ 7 ], 8 );

   }  /* end for */

   lpMBDstL += g_MPEGCtx.m_LineSize;
   lpMBSrcL  = lpMBDstL + 1;

   lpMBDstR += g_MPEGCtx.m_LineSize;
   lpMBSrcR  = lpMBDstR - 1;

  }  /* end for */

  lpMBSrcL -= g_MPEGCtx.m_LineSize;
  lpMBSrcR -= g_MPEGCtx.m_LineSize;
/* left bottom */
  memset ( &lpMBDstL -> m_Y [ 0 ][ 0 ], lpMBSrcL -> m_Y [ 15 ][ 0 ], 256 );
  memset ( &lpMBDstL -> m_Cb[ 0 ][ 0 ], lpMBSrcL -> m_Cb[  7 ][ 0 ],  64 );
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[  7 ][ 0 ],  64 );
/* bottom */
  ++lpMBDstL;

  for ( i = 0; i < g_MPEGCtx.m_MBW; ++i ) {

   lSrcY  = *( u128* )&lpMBSrcL -> m_Y[ 15 ][ 0 ];
   lpDstY =  ( u128* )&lpMBDstL -> m_Y[  0 ][ 0 ];

   lpDstY[  0 ] = lSrcY;
   lpDstY[  1 ] = lSrcY;
   lpDstY[  2 ] = lSrcY;
   lpDstY[  3 ] = lSrcY;
   lpDstY[  4 ] = lSrcY;
   lpDstY[  5 ] = lSrcY;
   lpDstY[  6 ] = lSrcY;
   lpDstY[  7 ] = lSrcY;
   lpDstY[  8 ] = lSrcY;
   lpDstY[  9 ] = lSrcY;
   lpDstY[ 10 ] = lSrcY;
   lpDstY[ 11 ] = lSrcY;
   lpDstY[ 12 ] = lSrcY;
   lpDstY[ 13 ] = lSrcY;
   lpDstY[ 14 ] = lSrcY;
   lpDstY[ 15 ] = lSrcY;

   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cb[ 7 ][ 0 ];
   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cb[ 0 ][ 0 ];

   __asm__(
    ".set noat\n\t"
    "pcpyld $at, %1, %1\n\t"
    "sq     $at,  0(%0)\n\t"
    "sq     $at, 16(%0)\n\t"
    "sq     $at, 32(%0)\n\t"
    "sq     $at, 48(%0)\n\t"
    ".set at\n\t"
    :: "r"( lpDstUV ), "r"( lSrcUV ) : "at"
   );

   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cr[ 7 ][ 0 ];
   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cr[ 0 ][ 0 ];

   __asm__(
    ".set noat\n\t"
    "pcpyld $at, %1, %1\n\t"
    "sq     $at,  0(%0)\n\t"
    "sq     $at, 16(%0)\n\t"
    "sq     $at, 32(%0)\n\t"
    "sq     $at, 48(%0)\n\t"
    ".set at\n\t"
    :: "r"( lpDstUV ), "r"( lSrcUV ) : "at"
   );

   ++lpMBSrcL;
   ++lpMBDstL;

  }  /* end for */

  --lpMBSrcL;
/* right bottom */
  memset ( &lpMBDstL -> m_Y [ 0 ][ 0 ], lpMBSrcL -> m_Y [ 15 ][ 15 ], 256 );
  memset ( &lpMBDstL -> m_Cb[ 0 ][ 0 ], lpMBSrcL -> m_Cb[  7 ][  7 ],  64 );
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[  7 ][  7 ],  64 );

 }  /* end if */

}  /* end SMS_MPEG_FrameEnd */

void SMS_MPEG_InitBlockIdx ( void ) {

 const int lLinesize = g_MPEGCtx.m_LineSize;
        
 g_MPEGCtx.m_BlockIdx[ 0 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2     ) - 2 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 1 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2     ) - 1 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 2 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2 + 1 ) - 2 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 3 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2 + 1 ) - 1 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 4 ] = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBY     + 1 )   + g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2 + g_MPEGCtx.m_MBX - 1;
 g_MPEGCtx.m_BlockIdx[ 5 ] = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBY + g_MPEGCtx.m_MBH + 2 ) + g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2 + g_MPEGCtx.m_MBX - 1;

 g_MPEGCtx.m_pDest  = g_MPEGCtx.m_CurPic.m_pBuf -> m_pData + g_MPEGCtx.m_MBX - 1;
 g_MPEGCtx.m_pDest += g_MPEGCtx.m_MBY * lLinesize;

}  /* end SMS_MPEG_InitBlockIdx */

void SMS_MPEG_InitScanTable ( SMS_ScanTable* apTbl, const uint8_t* apSrc ) {

 int i, lEnd = -1;
    
 apTbl -> m_pScantable = apSrc;

 for ( i = 0; i < 64; ++i ) {

  const int j = apSrc[ i ];

  if ( j > lEnd ) lEnd = j;

  apTbl -> m_RasterEnd[ i ] = lEnd;

 }  /* end for */

}  /* end SMS_MPEG_InitScanTable */

#define CLIP_MV() if ( lMBX < -1 ) {                          \
                   lMBX  = -1;                                \
                   lSrcX =  0;                                \
                  } else if ( lMBX > g_MPEGCtx.m_HEdgePos ) { \
                   lMBX  = g_MPEGCtx.m_HEdgePos;              \
                   lSrcX = 15;                                \
                  }  /* end if */                             \
                  if ( lMBY < -1 ) {                          \
                   lMBY  = -1;                                \
                   lSrcY =  0;                                \
                  } else if ( lMBY > g_MPEGCtx.m_VEdgePos ) { \
                   lMBY  = g_MPEGCtx.m_VEdgePos;              \
                   lSrcY = 15;                                \
                  }  /* end if */

#define CLIP_MVC() if ( lMBX < -1 ) {                          \
                    lMBX  = -1;                                \
                    lSrcX =  0;                                \
                   } else if ( lMBX > g_MPEGCtx.m_HEdgePos ) { \
                    lMBX  = g_MPEGCtx.m_HEdgePos;              \
                    lSrcX = 7;                                 \
                   }  /* end if */                             \
                   if ( lMBY < -1 ) {                          \
                    lMBY  = -1;                                \
                    lSrcY =  0;                                \
                   } else if ( lMBY > g_MPEGCtx.m_VEdgePos ) { \
                    lMBY  = g_MPEGCtx.m_VEdgePos;              \
                    lSrcY = 7;                                 \
                   }  /* end if */

static void _gmc_motion ( SMS_MacroBlock* apDst, SMS_MacroBlock* apRefPic ) {

 DSP_GMCn_16 ( &apDst -> m_Y [ 0 ][ 0 ], apRefPic, g_MPEGCtx.m_MBX, g_MPEGCtx.m_MBY, g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_LineSize );
 DSP_GMCn_8  ( &apDst -> m_Cb[ 0 ][ 0 ], apRefPic, g_MPEGCtx.m_MBX, g_MPEGCtx.m_MBY, g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_LineSize );

}  /* end _gmc_motion */

static void _gmc1_motion ( SMS_MacroBlock* apDest, SMS_MacroBlock* apRefPic ) {

 int lMBX,     lMBY;
 int lSrcX,    lSrcY;
 int lMotionX, lMotionY;

 lMotionX = g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ];
 lMotionY = g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ];

 lSrcX = lMotionX >> ( g_MPEGCtx.m_SpriteWarpAccuracy + 1 );
 lSrcY = lMotionY >> ( g_MPEGCtx.m_SpriteWarpAccuracy + 1 );

 lMotionX <<= ( 3 - g_MPEGCtx.m_SpriteWarpAccuracy );
 lMotionY <<= ( 3 - g_MPEGCtx.m_SpriteWarpAccuracy );
 
 lSrcX = SMS_clip ( lSrcX, -16, 16 );

 if ( lSrcX == 16 ) lMotionX = 0;

 lSrcY = SMS_clip ( lSrcY, -16, 16 );

 if ( lSrcY == 16 ) lMotionY = 0;

 lMBX = ( g_MPEGCtx.m_MBX + ( lSrcX >> 4 )  );
 lMBY = ( g_MPEGCtx.m_MBY + ( lSrcY >> 4 )  );

 lSrcX &= 0xF;
 lSrcY &= 0xF;

 apRefPic += lMBX;
 apRefPic += lMBY * g_MPEGCtx.m_LineSize;
    
 if (  ( lMotionX | lMotionY ) & 7  )

  DSP_GMC1_16 ( &apDest -> m_Y[ 0 ][ 0 ], &apRefPic -> m_Y[ 0 ][ 0 ], lSrcX, lSrcY, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_LineStride );

 else {

  int lDXY = (  ( lMotionX >> 3 ) & 1  ) | (  ( lMotionY >> 2 ) & 2  );

  if ( g_MPEGCtx.m_NoRounding )

   g_MPEGCtx.m_DSPCtx.m_PutNoRndPixTab[ 0 ][ lDXY ] ( &apDest -> m_Y[ 0 ][ 0 ], &apRefPic -> m_Y[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride );

  else g_MPEGCtx.m_DSPCtx.m_PutPixTab[ 0 ][ lDXY ] ( &apDest -> m_Y[ 0 ][ 0 ], &apRefPic -> m_Y[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride );

 }  /* end else */

 lMotionX = g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ];
 lMotionY = g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ];

 lSrcX = (  lMotionX >> ( g_MPEGCtx.m_SpriteWarpAccuracy + 1 )  );
 lSrcY = (  lMotionY >> ( g_MPEGCtx.m_SpriteWarpAccuracy + 1 )  );

 lMotionX <<= ( 3 - g_MPEGCtx.m_SpriteWarpAccuracy ) / 2;
 lMotionY <<= ( 3 - g_MPEGCtx.m_SpriteWarpAccuracy ) / 2;

 lSrcX = SMS_clip ( lSrcX, -8, 8 );

 if ( lSrcX == 8 ) lMotionX = 0;

 lSrcY = SMS_clip ( lSrcY, -8, 8 );

 if ( lSrcY == 8 ) lMotionY = 0;

 lSrcX &= 7;
 lSrcY &= 7;

 DSP_GMC1_8 (  &apDest -> m_Cb[ 0 ][ 0 ], &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX, lSrcY, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_LineStride );

}  /* end _gmc1_motion */

static SMS_INLINE void _qpel_motion (
                        SMS_MacroBlock*          apDestMB,
                        SMS_MacroBlock*          apRefPic,
                        SMS_OpPixFunc  ( *aPicOp  )[  4 ],
                        SMS_QPelMCFunc ( *aQPelOp )[ 16 ],
                        int                      aMotionX,
                        int                      aMotionY,
                        int                            aH
                       ) {

 int             lDXY;
 int             lMBX,  lMBY;
 int             lMX,   lMY;
 int             lSrcX, lSrcY;
 SMS_MacroBlock* lpMB;

 lDXY = (  ( aMotionY & 3 ) << 2  ) | ( aMotionX & 3 );

 lSrcX = aMotionX >> 2;
 lSrcY = aMotionY >> 2;

 lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 4 );
 lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 4 );

 CLIP_MV()

 lpMB  = apRefPic + lMBX;
 lpMB += lMBY * g_MPEGCtx.m_LineSize;

 lSrcX &= 0xF;
 lSrcY &= 0xF;

 aQPelOp[ 0 ][ lDXY ] (
  &apDestMB -> m_Y[ 0 ][ 0 ], &lpMB -> m_Y[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride
 );

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_QPEL_CHROMA2 ) {

  static const char lRTab[ 8 ]= { 0, 0, 1, 1, 0, 0, 0, 1 };

  lMX = ( aMotionX >> 1 ) + lRTab[ aMotionX & 7 ];
  lMY = ( aMotionY >> 1 ) + lRTab[ aMotionY & 7 ];

 } else if ( g_MPEGCtx.m_Bugs & SMS_BUG_QPEL_CHROMA ) {

  lMX = ( aMotionX >> 1 ) | ( aMotionX & 1 );
  lMY = ( aMotionY >> 1 ) | ( aMotionY & 1 );

 } else {

  lMX = aMotionX / 2;
  lMY = aMotionY / 2;

 }  /* end else */

 lMX = ( lMX >> 1 ) | ( lMX & 1 );
 lMY = ( lMY >> 1 ) | ( lMY & 1 );

 lDXY = ( lMX & 1 ) | (  ( lMY & 1 ) << 1  );

 lSrcX = lMX >> 1;
 lSrcY = lMY >> 1;

 lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 3 );
 lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 3 );

 CLIP_MVC()

 apRefPic += lMBX;
 apRefPic += lMBY * g_MPEGCtx.m_LineSize;

 lSrcX &= 7;
 lSrcY &= 7;

 aPicOp[ 1 ][ lDXY ] (
  &apDestMB -> m_Cb[ 0 ][ 0 ], &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride
 );

}  /* end _qpel_motion */

static SMS_INLINE void _mpeg_motion (
                        SMS_MacroBlock*         apDest,
                        SMS_MacroBlock*       apRefPic,
                        SMS_OpPixFunc ( *aPicOp )[ 4 ],
                        int                   aMotionX,
                        int                   aMotionY
                       ) {

 int lDXY, lUVXY;
 int lSrcX, lSrcY;
 int lMBX, lMBY;

 lDXY = (  ( aMotionY & 1 ) << 1  ) | ( aMotionX & 1 );

 lSrcX = aMotionX >> 1;
 lSrcY = aMotionY >> 1;

 lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 4 );
 lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 4 );

 CLIP_MV()

 apRefPic += lMBX;
 apRefPic += lMBY * g_MPEGCtx.m_LineSize;

 lSrcX &= 0xF;
 lSrcY &= 0xF;

 __asm__ __volatile__(
  "pref 0, 768(%0)\n\t"
  :: "r"( apRefPic )
 );

 lUVXY = lDXY | ( aMotionY & 2 ) | (  ( aMotionX & 2 ) >> 1  );

 aPicOp[ 0 ][ lDXY  ] ( &apDest -> m_Y [ 0 ][ 0 ], &apRefPic -> m_Y [ 0 ][ 0 ], lSrcX,      lSrcY,      g_MPEGCtx.m_LineStride );
 aPicOp[ 1 ][ lUVXY ] ( &apDest -> m_Cb[ 0 ][ 0 ], &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX >> 1, lSrcY >> 1, g_MPEGCtx.m_LineStride );

}  /* end _mpeg_motion */

static SMS_INLINE void _chroma_4mv_motion (
                        uint8_t*        apDestCb,
                        SMS_MacroBlock* apRefPic,
                        SMS_OpPixFunc*    aPixOp,
                        int                  aMX,
                        int                  aMY
                       ) {

 int      lMBX, lMBY;
 int      lDXY, lSrcX, lSrcY;

 aMX = SMS_H263_RoundChroma ( aMX );
 aMY = SMS_H263_RoundChroma ( aMY );

 lDXY  = (  ( aMY & 1 ) << 1  ) | ( aMX & 1 );

 lSrcX = aMX >> 1;
 lSrcY = aMY >> 1;

 lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 3 );
 lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 3 );

 CLIP_MVC()

 apRefPic += lMBX;
 apRefPic += lMBY * g_MPEGCtx.m_LineSize;
  
 aPixOp[ lDXY ] ( apDestCb, &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX & 7, lSrcY & 7, g_MPEGCtx.m_LineStride );

}  /* end _chroma_4mv_motion */

static SMS_INLINE void _MPEG_Motion (
                        SMS_MacroBlock*          apDestMB,
                        int                          aDir,
                        SMS_MacroBlock*          apRefPic, 
                        SMS_OpPixFunc  ( *aPixOp  )[  4 ],
                        SMS_QPelMCFunc ( *aQPelOp )[ 16 ]
                       ) {

 int i;
 int lMBX, lMBY;

 lMBX = g_MPEGCtx.m_MBX;
 lMBY = g_MPEGCtx.m_MBY;

 switch ( g_MPEGCtx.m_MVType ) {

  case SMS_MV_TYPE_16X16:

   if ( g_MPEGCtx.m_MCSel ) {

    if ( g_MPEGCtx.m_RealSpriteWarpPts == 1 )

     _gmc1_motion ( apDestMB, apRefPic );

    else _gmc_motion ( apDestMB, apRefPic );

   } else if ( g_MPEGCtx.m_QuarterSample ) {

    _qpel_motion (
     apDestMB, apRefPic, aPixOp, aQPelOp,
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 0 ],
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 1 ], 16
    );

   } else _mpeg_motion (
           apDestMB, apRefPic, aPixOp,
           g_MPEGCtx.m_MV[ aDir ][ 0 ][ 0 ],
           g_MPEGCtx.m_MV[ aDir ][ 0 ][ 1 ]
          );
           
  break;

  case SMS_MV_TYPE_8X8: {

   int             lMotionX, lMotionY;
   int             lDXY;
   int             lSrcX, lSrcY;
   int             lMX = 0;
   int             lMY = 0;
   uint8_t*        lpDest;
   SMS_MacroBlock* lpMBSrc;

   if ( g_MPEGCtx.m_QuarterSample )

    for ( i = 0; i < 4; ++i ) {

     lMotionX = g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
     lMotionY = g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

     lDXY = (  ( lMotionY & 3 ) << 2  ) | ( lMotionX & 3 );

     lSrcX = ( lMotionX >> 2 ) + ( i  & 1 ) * 8;
     lSrcY = ( lMotionY >> 2 ) + ( i >> 1 ) * 8;
                
     lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 4 );
     lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 4 );

     CLIP_MV()

     lpMBSrc  = apRefPic + lMBX;
     lpMBSrc += lMBY * g_MPEGCtx.m_LineSize;

     lpDest = (  ( uint8_t* )apDestMB  ) + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

     aQPelOp[ 1 ][ lDXY ] ( lpDest, &lpMBSrc -> m_Y[ 0 ][ 0 ], lSrcX & 0xF, lSrcY & 0xF, g_MPEGCtx.m_LineStride );

     lMX += g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ] / 2;
     lMY += g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ] / 2;

    }  /* end for */

   else for ( i = 0; i < 4; ++i ) {

    lMotionX = g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
    lMotionY = g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

    lDXY = (  ( lMotionY & 1 ) << 1 ) | ( lMotionX & 1 );

    lSrcX = ( lMotionX >> 1 ) + ( i  & 1 ) * 8;
    lSrcY = ( lMotionY >> 1 ) + ( i >> 1 ) * 8;

    lMBX = g_MPEGCtx.m_MBX + ( lSrcX >> 4 );
    lMBY = g_MPEGCtx.m_MBY + ( lSrcY >> 4 );

    CLIP_MV()

    lpMBSrc  = apRefPic + lMBX;
    lpMBSrc += lMBY * g_MPEGCtx.m_LineSize;

    lpDest = (  ( uint8_t* )apDestMB  ) + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

    aPixOp[ 2 ][ lDXY ] ( lpDest, &lpMBSrc -> m_Y[ 0 ][ 0 ], lSrcX & 0xF, lSrcY & 0xF, g_MPEGCtx.m_LineStride );

    lMX += g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
    lMY += g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

   }  /* end for */

   _chroma_4mv_motion ( &apDestMB -> m_Cb[ 0 ][ 0 ], apRefPic, aPixOp[ 1 ], lMX, lMY );

  } break;

 }  /* end switch */

}  /* end _MPEG_Motion */

void SMS_H263_DCTUnquantizeIntra ( SMS_DCTELEM* apBlock ) {

 int lQMul   = g_MPEGCtx.m_QScale << 1;
 int lQAdd   = ( g_MPEGCtx.m_QScale - 1 ) | 1;
 int lLevel0 = apBlock[   0 ] * g_MPEGCtx.m_Y_DCScale;
 int lLevel1 = apBlock[  64 ] * g_MPEGCtx.m_Y_DCScale;
 int lLevel2 = apBlock[ 128 ] * g_MPEGCtx.m_Y_DCScale;
 int lLevel3 = apBlock[ 192 ] * g_MPEGCtx.m_Y_DCScale;
 int lLevel4 = apBlock[ 256 ] * g_MPEGCtx.m_C_DCScale;
 int lLevel5 = apBlock[ 320 ] * g_MPEGCtx.m_C_DCScale;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "addiu    $at, $zero, 32\n\t"
  "pcpyld   %0, %0, %0\n\t"	
  "pcpyld   %1, %1, %1\n\t"
  "pcpyh    %0, %0\n\t"
  "pcpyh    %1, %1\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0(%2)\n\t"
  "addiu    %2, %2, 16\n\t"
  "addiu    $at, $at, -1\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", %0\n\t"	
  "pcgth    " ASM_REG_T1 ", " ASM_REG_T0 ", $zero\n\t"
  "pcgth    " ASM_REG_T0 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T1 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "paddh    " ASM_REG_T2 ", %1, " ASM_REG_T0 "\n\t"
  "pxor     " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "bgtz     $at, 1b\n\t"
  "sq       " ASM_REG_T0 ", -16(%2)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( lQMul ), "r"( lQAdd ), "r"( apBlock )
   : ASM_REG_T0, ASM_REG_T1, ASM_REG_T2, "memory"
 );

 apBlock[ -256 ] = lLevel0;
 apBlock[ -192 ] = lLevel1;
 apBlock[ -128 ] = lLevel2;
 apBlock[  -64 ] = lLevel3;

 lQMul = g_MPEGCtx.m_ChromaQScale << 1;
 lQAdd = ( g_MPEGCtx.m_ChromaQScale - 1 ) | 1;

 __asm__ __volatile__ (

  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "addiu    $at, $zero, 16\n\t"
  "pcpyld   %0, %0, %0\n\t"	
  "pcpyld   %1, %1, %1\n\t"
  "pcpyh    %0, %0\n\t"
  "pcpyh    %1, %1\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0(%2)\n\t"
  "addiu    %2, %2, 16\n\t"
  "addiu    $at, $at, -1\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", %0\n\t"
  "pcgth    " ASM_REG_T1 ", " ASM_REG_T0 ", $zero\n\t"
  "pcgth    " ASM_REG_T0 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T1 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "paddh    " ASM_REG_T2 ", %1, " ASM_REG_T0 "\n\t"
  "pxor     " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "bgtz     $at, 1b\n\t"
  "sq       " ASM_REG_T0 ", -16(%2)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( lQMul ), "r"( lQAdd ), "r"( apBlock )
   : ASM_REG_T0, ASM_REG_T1, ASM_REG_T2, "memory"
 );

 apBlock[ -128 ] = lLevel4;
 apBlock[  -64 ] = lLevel5;

}  /* end SMS_H263_DCTUnquantizeIntra */

void SMS_H263_DCTUnquantizeInter ( SMS_DCTELEM* apBlock ) {
#if 0
 const int lQAdd    = ( aQScale - 1 ) | 1;
 const int lQMul    = aQScale << 1;
 const int lnCoeffs = g_MPEGCtx.m_InterScanTbl.m_RasterEnd[  g_MPEGCtx.m_BlockLastIdx[ aN ]  ];

 __asm__ __volatile__ (

  "pcpyld   %0, %0, %0\n\t"	
  "pcpyld   %1, %1, %1\n\t"
  "pcpyh    %0, %0\n\t"
  "pcpyh    %1, %1\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0(%3)\n\t"
  "addi     %3, %3, 16\n\t"
  "addi     %2, %2, -8\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", %0\n\t"	
  "pcgth    " ASM_REG_T1 ", " ASM_REG_T0 ", $zero\n\t"
  "pcgth    " ASM_REG_T0 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T1 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "paddh    " ASM_REG_T2 ", %1, " ASM_REG_T0 "\n\t"
  "pxor     " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "sq       " ASM_REG_T0 ", -16(%3)\n\t"
  "bgez     %2, 1b\n\t"
  :: "r"( lQMul ), "r"( lQAdd ), "r"( lnCoeffs ), "r"( apBlock )
   : ASM_REG_T0, ASM_REG_T1, ASM_REG_T2, "memory"
 );
#endif
}  /* end SMS_H263_DCTUnquantizeInter */

void SMS_MPEG2_DCTUnquantizeIntra ( short* apBlock ) {

 short lLevel0;
 short lLevel1;

 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lb       $v1, %2\n\t"
  "lui      $a3, 0x7000\n\t"
  "lh       " ASM_REG_T5 ",   0($a0)\n\t"
  "lh       " ASM_REG_T6 ", 128($a0)\n\t"
  "mult     " ASM_REG_T8 ", " ASM_REG_T5 ", $v1\n\t"
  "lh       $a1, 256($a0)\n\t"
  "mult     " ASM_REG_T7 ", " ASM_REG_T6 ", $v1\n\t"
  "lh       $a2, 384($a0)\n\t"
  "mult     $a1, $a1, $v1\n\t"
  "lb       $v0, %4\n\t"
  "mult     $a2, $a2, $v1\n\t"
  "pcpyld   $v0, $v0, $v0\n\t"
  "pcpyh    $v0, $v0\n\t"
  "lui      " ASM_REG_T4 ", 0xFFFF\n\t"
  "addiu    $at, $zero, 32\n\t"
  "ori      " ASM_REG_T4 ", " ASM_REG_T4 ", 0xFF7F\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0($a0)\n\t"
  "lq       " ASM_REG_T1 ", 0x0400($a3)\n\t"
  "pcgth    " ASM_REG_T2 ", " ASM_REG_T0 ", $zero\n\t"
  "pmulth   $zero, " ASM_REG_T1 ", $v0\n\t"
  "pcgth    " ASM_REG_T3 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pmfhl.lh " ASM_REG_T1 "\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "addiu    $a3, $a3, 16\n\t"
  "addiu    $a0, $a0, 16\n\t"
  "addiu    $at, $at, -1\n\t"
  "and      $a3, $a3, " ASM_REG_T4 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "psrah    " ASM_REG_T0 ", " ASM_REG_T0 ", 3\n\t"
  "bgtz     $at, 1b\n\t"
  "sq       " ASM_REG_T0 ", -16($a0)\n\t"
  "sh       " ASM_REG_T8 ", -512($a0)\n\t"
  "sh       " ASM_REG_T7 ", -384($a0)\n\t"
  "sh       $a1, -256($a0)\n\t"
  "sh       $a2, -128($a0)\n\t"
  "lb       " ASM_REG_T0 ", %3\n\t"
  "lb       " ASM_REG_T5 ", %5\n\t"
  "lh       " ASM_REG_T1 ",   0($a0)\n\t"
  "lh       " ASM_REG_T2 ", 128($a0)\n\t"
  "mult     %0, " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
  "mult1    %1, " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
  "pcpyld   " ASM_REG_T5 ", " ASM_REG_T5 ", " ASM_REG_T5 "\n\t"
  "pcpyh    " ASM_REG_T5 ", " ASM_REG_T5 "\n\t"
  "addiu    $at, $zero, 16\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0($a0)\n\t"
  "lq       " ASM_REG_T1 ", 0x0400($a3)\n\t"
  "pcgth    " ASM_REG_T2 ", " ASM_REG_T0 ", $zero\n\t"
  "pmulth   $zero, " ASM_REG_T1 ", " ASM_REG_T5 "\n\t"
  "pcgth    " ASM_REG_T3 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pmfhl.lh " ASM_REG_T1 "\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "addiu    $a3, $a3, 16\n\t"
  "addiu    $a0, $a0, 16\n\t"
  "addiu    $at, $at, -1\n\t"
  "and      $a3, $a3, " ASM_REG_T4 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "psrah    " ASM_REG_T0 ", " ASM_REG_T0 ", 3\n\t"
  "bgtz     $at, 1b\n\t"
  "sq       " ASM_REG_T0 ", -16($a0)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( lLevel0 ),                    // 0
    "=r"( lLevel1 )                     // 1
  : "m"( g_MPEGCtx.m_Y_DCScale    ),    // 2
    "m"( g_MPEGCtx.m_C_DCScale    ),    // 3
    "m"( g_MPEGCtx.m_QScale       ),    // 4
    "m"( g_MPEGCtx.m_ChromaQScale )     // 5
 );

 apBlock[  -64 ] = lLevel1;
 apBlock[ -128 ] = lLevel0;

}  /* end SMS_MPEG2_DCTUnquantizeIntra */

void SMS_MPEG2_DCTUnquantizeInter ( SMS_DCTELEM* apBlock ) {

 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "la       $v1, %2\n\t"
  "lbu      " ASM_REG_T6 ", %0\n\t"
  "addiu    $a3, $v1, 12\n\t"
  "lbu      " ASM_REG_T7 ", %1\n\t"
  "mtsah    $zero, 1\n\t"
  "pcpyld   " ASM_REG_T6 ", " ASM_REG_T7 ", " ASM_REG_T6 "\n\t"
  "pcpyh    " ASM_REG_T6 ", " ASM_REG_T6 "\n\t"
  "lui      " ASM_REG_T4 ", 0xFFFF\n\t"
  "lui      $a1, 0x7000\n\t"
  "pnor     " ASM_REG_T5 ", $zero, $zero\n\t"
  "ori      " ASM_REG_T4 ", " ASM_REG_T4 ", 0xFF7F\n\t"
  "psrlh    " ASM_REG_T5 ", " ASM_REG_T5 ", 15\n\t"
  "3:\n\t"
  "lb       " ASM_REG_T7 ", 0($v1)\n\t"
  "pcpyld   $a2, " ASM_REG_T6 ", " ASM_REG_T6 "\n\t"
  "bltzl    " ASM_REG_T7 ", 2f\n\t"
  "addiu    $a0, $a0, 128\n\t"
  "pxor     $v0, $v0, $v0\n\t"
  "pcpyh    $a2, $a2\n\t"
  "addiu    $at, $zero, 8\n\t"
  "1:\n\t"
  "lq       " ASM_REG_T0 ", 0($a0)\n\t"
  "lq       " ASM_REG_T1 ", 0x0500($a1)\n\t"
  "pcgth    " ASM_REG_T2 ", " ASM_REG_T0 ", $zero\n\t"
  "pmulth   $zero, " ASM_REG_T1 ", $a2\n\t"
  "pcgth    " ASM_REG_T3 ", $zero, " ASM_REG_T0 "\n\t"
  "por      " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "psllh    " ASM_REG_T0 ", " ASM_REG_T0 ", 1\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T5 "\n\t"
  "pmfhl.lh " ASM_REG_T1 "\n\t"
  "pmulth   $zero, " ASM_REG_T0 ", " ASM_REG_T1 "\n\t"
  "addiu    $a0, $a0, 16\n\t"
  "addiu    $a1, $a1, 16\n\t"
  "addiu    $at, $at, -1\n\t"
  "and      $a1, $a1, " ASM_REG_T4 "\n\t"
  "pmfhl.lh " ASM_REG_T0 "\n\t"
  "psrah    " ASM_REG_T0 ", " ASM_REG_T0 ", 4\n\t"
  "paddh    " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pxor     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T3 "\n\t"
  "pand     " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
  "paddh    $v0, $v0, " ASM_REG_T0 "\n\t"
  "bgtz     $at, 1b\n\t"
  "sq       " ASM_REG_T0 ", -16($a0)\n\t"
  "pcpyud   $at, $v0, $at\n\t"
  "paddh    $v0, $v0, $at\n\t"
  "dsrl32   $at, $v0, 0\n\t"
  "paddh    $v0, $v0, $at\n\t"
  "srl      $at, $v0, 16\n\t"
  "paddh    $v0, $v0, $at\n\t"
  "psubh    $v0, $v0, " ASM_REG_T5 "\n\t"
  "lh       $at, -2($a0)\n\t"
  "andi     $v0, $v0, 1\n\t"
  "xor      $v0, $at, $v0\n\t"
  "sh       $v0, -2($a0)\n\t"
  "2:\n\t"
  "addiu    $v1, $v1, 2\n\t"
  "bne      $v1, $a3, 3b\n\t"
  "qfsrv    " ASM_REG_T6 ", " ASM_REG_T6 ", " ASM_REG_T6 "\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "m"( g_MPEGCtx.m_QScale ), "m"( g_MPEGCtx.m_ChromaQScale ), "g"( g_MPEGCtx.m_BlockLastIdx )
 );

}  /* end SMS_MPEG2_DCTUnquantizeInter */

static uint8_t* s_pDest;

void SMS_MPEG_DecodeMB ( SMS_DCTELEM aBlock[ 12 ][ 64 ] ) {

 static u64           s_DMATag[ 2 ] __attribute__(   (  aligned( 16 )  )   ) = {
  0x8000058004000033UL, 0x0000000000000000UL
 };

 const int     lMBXY = g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBX;
 u64           lDword;
 unsigned int  lWord;

 SMS_MacroBlock* lpDestMB;
 uint8_t*        lpMBSkip = &g_MPEGCtx.m_pMBSkipTbl[ lMBXY ];

 SMS_OpPixFunc  ( *lOpPix  )[  4 ];
 SMS_QPelMCFunc ( *lOpQPel )[ 16 ];

 g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lMBXY ] = g_MPEGCtx.m_QScale;

 if ( !g_MPEGCtx.m_MBIntra ) {

  if ( g_MPEGCtx.m_pMBIntraTbl[ lMBXY ] ) SMS_MPEG_CleanIntraTblEntries ();

 } else g_MPEGCtx.m_pMBIntraTbl[ lMBXY ] = 1;

 g_MPEGCtx.MBCallback ( g_MPEGCtx.m_pDestCB );
 g_MPEGCtx.MBCallback = SMS_MPEG_DummyCB;

 lDword   = (  ( u64*           )&g_MPEGCtx.m_BlockLastIdx  )[ 0 ];
 lWord    = (  ( unsigned int*  )&g_MPEGCtx.m_BlockLastIdx  )[ 2 ];
 lpDestMB = g_MPEGCtx.m_pMCBlk[ g_MPEGCtx.m_MCBlkIdx ];
 (  ( u64*           )SMS_MPEG_SPR_BLOCKS  )[  98 ] = lDword;
 (  ( unsigned int*  )SMS_MPEG_SPR_BLOCKS  )[ 198 ] = lWord;

 if ( g_MPEGCtx.m_MBSkiped ) {

  g_MPEGCtx.m_MBSkiped = 0;
 
 } else *lpMBSkip = 0;

 if ( !g_MPEGCtx.m_MBIntra ) {

  if ( g_MPEGCtx.m_BlockSum ) {
   if ( g_MPEGCtx.m_MPEGQuant ) SMS_MPEG2_DCTUnquantizeInter ( g_MPEGCtx.m_pBlock[ 0 ] );
   DMA_SendChainA ( DMAC_VIF0, s_DMATag );
   g_MPEGCtx.MBCallback = SMS_MPEG_MCB;
   g_MPEGCtx.m_pDestCB  = g_MPEGCtx.m_pMCBlk[ g_MPEGCtx.m_MCBlkIdx ];
   s_pDest = ( uint8_t* )g_MPEGCtx.m_pDest;
  }  /* end if */

  if ( !g_MPEGCtx.m_NoRounding || g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ) {

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_PutPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_PutQPelPixTab;

  } else {

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_PutNoRndPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_PutNoRndQPelPixTab;

  }  /* end else */

  if ( g_MPEGCtx.m_MVDir & SMS_MV_DIR_FORWARD ) {

   _MPEG_Motion (
    lpDestMB, 0, g_MPEGCtx.m_LastPic.m_pBuf -> m_pData, lOpPix, lOpQPel
   );

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_AvgPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_AvgQPelPixTab;

  }  /* end if */

  if ( g_MPEGCtx.m_MVDir & SMS_MV_DIR_BACKWARD )

   _MPEG_Motion (
    lpDestMB, 1, g_MPEGCtx.m_NextPic.m_pBuf -> m_pData, lOpPix, lOpQPel
   );

  if ( g_MPEGCtx.m_BlockSum ) return;

  DMA_SendS( DMAC_FROM_SPR, ( uint8_t* )g_MPEGCtx.m_pDest, lpDestMB, 24 );
  g_MPEGCtx.m_MCBlkIdx ^= 1;

 } else {

  g_MPEGCtx.DCT_UnquantizeIntra ( g_MPEGCtx.m_pBlock[ 0 ] );

  DMA_SendChainA ( DMAC_VIF0, s_DMATag );

  g_MPEGCtx.m_pDestCB  = g_MPEGCtx.m_pDest;
  g_MPEGCtx.MBCallback = DSP_PackMB;

 }  /* end else */

}  /* end SMS_MPEG_DecodeMB */

void SMS_MPEG_DummyCB ( void* apMB ) {}

void SMS_MPEG_MCB ( void* apParam ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "pcpyld   $ra, $ra, $ra\n\t"
  "pcpyld   $s0, $s0, $s0\n\t"
  "lui      $a1, 0x7000\n\t"
  "jal      DSP_PackAddMB\n\t"
  "addu     $s0, $zero, $a0\n\t"
  "lh       $v0, %1\n\t"
  "lui      $at, 0x1001\n\t"
  "lw       $a1, %0\n\t"
  "addiu    $v1, $zero, 0x0100\n\t"
  "addiu    $a2, $zero, 24\n\t"
  "sw       $a1, -0x2FF0($at)\n\t"
  "sw       $a2, -0x2FE0($at)\n\t"
  "xori     $v0, $v0, 1\n\t"
  "sw       $s0, -0x2F80($at)\n\t"
  "sw       $v1, -0x3000($at)\n\t"
  "sh       $v0, %1\n\t"
  "pcpyud   $ra, $ra, $ra\n\t"
  "pcpyud   $s0, $s0, $s0\n\t"
  ".set reorder\n\t"
  ".set at\n\t"
  :: "g"( s_pDest ), "g"( g_MPEGCtx.m_MCBlkIdx )
 );
}  /* end SMS_MPEG_MCB */
