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
#include "SMS_MPEG2.h"
#include "SMS_VideoBuffer.h"
#include "SMS_DMA.h"

#include <malloc.h>
#include <string.h>
#include <limits.h>

#define MAX_PICTURE_COUNT           15
#define PREV_PICT_TYPES_BUFFER_SIZE 256
#define BITSTREAM_BUFFER_SIZE       1024 * 256

SMS_MPEGContext g_MPEGCtx;

static void _mpeg_alloc_picture ( SMS_Frame* apPic ) {

 const int lBigMBNr   = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBH + 1 ) + 1;
 const int lMBArrSize = g_MPEGCtx.m_MBStride * g_MPEGCtx.m_MBH;
 const int lB8ArrSize = g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2;

 int i;

 SMS_CodecGetBuffer ( g_MPEGCtx.m_pParentCtx, apPic );

 if ( apPic -> m_pQScaleTbl == NULL ) {

  apPic -> m_pMBSkipTbl = calloc (  1, lMBArrSize * sizeof ( uint8_t  ) + 2  );
  apPic -> m_pQScaleTbl = calloc (  1, lMBArrSize * sizeof ( uint8_t  )      );
  apPic -> m_pMBType    = calloc (  1, lBigMBNr   * sizeof ( uint32_t )      );

  for ( i = 0; i < 2; ++i ) {

   apPic -> m_pMotionValBase[ i ] = calloc (  1, 2 * ( lB8ArrSize + 2 ) * sizeof ( int16_t )  );
   apPic -> m_pMotionVal    [ i ] = apPic -> m_pMotionValBase[ i ] + 2;
   apPic -> m_pRefIdx       [ i ] = calloc (  1, lB8ArrSize * sizeof ( uint8_t )  );

  }  /* end for */

  apPic -> m_MotionSubsampleLog2 = 3;

 }  /* end if */

 memmove (
  g_MPEGCtx.m_pPrevPicTypes + 1, g_MPEGCtx.m_pPrevPicTypes, PREV_PICT_TYPES_BUFFER_SIZE - 1
 );

 g_MPEGCtx.m_pPrevPicTypes[ 0 ] = g_MPEGCtx.m_PicType;

 if ( apPic -> m_Age < PREV_PICT_TYPES_BUFFER_SIZE &&
      g_MPEGCtx.m_pPrevPicTypes[ apPic -> m_Age ] == SMS_FT_B_TYPE
 ) apPic -> m_Age = INT_MAX;

 apPic -> m_Width  = g_MPEGCtx.m_Width;
 apPic -> m_Height = g_MPEGCtx.m_Height;

}  /* end _mpeg_alloc_picture */

void SMS_MPEG_CleanIntraTblEntries ( void ) {

 int lWrap = g_MPEGCtx.m_B8Stride;
 int lXY   = g_MPEGCtx.m_BlockIdx[ 0 ];
    
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY             ] = 
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY + 1         ] = 
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY + lWrap     ] =
 g_MPEGCtx.m_pDCVal[ 0 ][ lXY + 1 + lWrap ] = 1024;

 memset (  g_MPEGCtx.m_pACVal[ 0 ][ lXY         ], 0, 32 * sizeof ( int16_t )  );
 memset (  g_MPEGCtx.m_pACVal[ 0 ][ lXY + lWrap ], 0, 32 * sizeof ( int16_t )  );

 lWrap = g_MPEGCtx.m_MBStride;
 lXY   = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * lWrap;

 g_MPEGCtx.m_pDCVal[ 1 ][ lXY ] =
 g_MPEGCtx.m_pDCVal[ 2 ][ lXY ] = 1024;

 memset (  g_MPEGCtx.m_pACVal[ 1 ][ lXY ], 0, 16 * sizeof ( int16_t )  );
 memset (  g_MPEGCtx.m_pACVal[ 2 ][ lXY ], 0, 16 * sizeof ( int16_t )  );
    
 g_MPEGCtx.m_pMBIntraTbl[ lXY ] = 0;

}  /* end SMS_MPEG_CleanIntraTblEntries */

int SMS_MPEG_FrameStart ( void ) {

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
alloc:
 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

  if (  g_MPEGCtx.m_pPic[ i ].m_pBuf &&
       !g_MPEGCtx.m_pPic[ i ].m_Ref
  ) SMS_CodecReleaseBuffer ( g_MPEGCtx.m_pParentCtx, &g_MPEGCtx.m_pPic[ i ] );

 if ( g_MPEGCtx.m_pCurPic && g_MPEGCtx.m_pCurPic -> m_pBuf == NULL )

  lpPic = g_MPEGCtx.m_pCurPic;

 else lpPic = &g_MPEGCtx.m_pPic[ SMS_MPEGContext_FindUnusedPic () ];

 lpPic -> m_Ref = g_MPEGCtx.m_PicType != SMS_FT_B_TYPE && !g_MPEGCtx.m_Dropable ? 3 : 0;

 lpPic -> m_CodedPicNr = g_MPEGCtx.m_CodedPicNr++;

 _mpeg_alloc_picture ( lpPic );

 g_MPEGCtx.m_pCurPic = lpPic;

 g_MPEGCtx.m_pCurPic -> m_Type     = g_MPEGCtx.m_PicType;
 g_MPEGCtx.m_pCurPic -> m_KeyFrame = g_MPEGCtx.m_PicType == SMS_FT_I_TYPE;

 g_MPEGCtx.m_CurPic = *g_MPEGCtx.m_pCurPic;

 if ( g_MPEGCtx.m_PicType != SMS_FT_B_TYPE ) {

  g_MPEGCtx.m_pLastPic = g_MPEGCtx.m_pNextPic;

  if ( !g_MPEGCtx.m_Dropable ) g_MPEGCtx.m_pNextPic = g_MPEGCtx.m_pCurPic;

 }  /* end if */

 if ( g_MPEGCtx.m_pLastPic ) g_MPEGCtx.m_LastPic = *g_MPEGCtx.m_pLastPic;
 if ( g_MPEGCtx.m_pNextPic ) g_MPEGCtx.m_NextPic = *g_MPEGCtx.m_pNextPic;

 if (  g_MPEGCtx.m_PicType != SMS_FT_I_TYPE &&
       ( g_MPEGCtx.m_pLastPic           == NULL ||
         g_MPEGCtx.m_pLastPic -> m_pBuf == NULL
       )
 ) goto alloc;

 if ( !g_MPEGCtx.m_MPEGQuant ) {

  g_MPEGCtx.DCT_UnquantizeInter = SMS_H263_DCTUnquantizeInter;
  g_MPEGCtx.DCT_UnquantizeIntra = SMS_H263_DCTUnquantizeIntra;

 } else {

  g_MPEGCtx.DCT_UnquantizeInter = SMS_MPEG2_DCTUnquantizeInter;
  g_MPEGCtx.DCT_UnquantizeIntra = SMS_MPEG2_DCTUnquantizeIntra;

 }  /* end else */

 return 0;

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

   lpDstY =  ( u128* )&lpMBDstL -> m_Y[ 0 ][ 0 ];
   lSrcY  = *( u128* )&lpMBSrcL -> m_Y[ 0 ][ 0 ];

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

   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cb[ 0 ][ 0 ];
   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cb[ 0 ][ 0 ];

   lpDstUV[ 0 ] = lSrcUV;
   lpDstUV[ 1 ] = lSrcUV;
   lpDstUV[ 2 ] = lSrcUV;
   lpDstUV[ 3 ] = lSrcUV;
   lpDstUV[ 4 ] = lSrcUV;
   lpDstUV[ 5 ] = lSrcUV;
   lpDstUV[ 6 ] = lSrcUV;
   lpDstUV[ 7 ] = lSrcUV;

   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cr[ 0 ][ 0 ];
   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cr[ 0 ][ 0 ];

   lpDstUV[ 0 ] = lSrcUV;
   lpDstUV[ 1 ] = lSrcUV;
   lpDstUV[ 2 ] = lSrcUV;
   lpDstUV[ 3 ] = lSrcUV;
   lpDstUV[ 4 ] = lSrcUV;
   lpDstUV[ 5 ] = lSrcUV;
   lpDstUV[ 6 ] = lSrcUV;
   lpDstUV[ 7 ] = lSrcUV;

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

   lpMBDstL += g_MPEGCtx.m_CurPic.m_Linesize;
   lpMBSrcL  = lpMBDstL + 1;

   lpMBDstR += g_MPEGCtx.m_CurPic.m_Linesize;
   lpMBSrcR  = lpMBDstR - 1;

  }  /* end for */

  lpMBSrcL -= g_MPEGCtx.m_CurPic.m_Linesize;
  lpMBSrcR -= g_MPEGCtx.m_CurPic.m_Linesize;
/* left bottom */
  memset ( &lpMBDstL -> m_Y [ 0 ][ 0 ], lpMBSrcL -> m_Y [ 15 ][ 0 ], 256 );
  memset ( &lpMBDstL -> m_Cb[ 0 ][ 0 ], lpMBSrcL -> m_Cb[  7 ][ 0 ],  64 );
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[  7 ][ 0 ],  64 );
/* bottom */
  ++lpMBDstL;

  for ( i = 0; i < g_MPEGCtx.m_MBW; ++i ) {

   lpDstY =  ( u128* )&lpMBDstL -> m_Y[  0 ][ 0 ];
   lSrcY  = *( u128* )&lpMBSrcL -> m_Y[ 15 ][ 0 ];

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

   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cb[ 0 ][ 0 ];
   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cb[ 7 ][ 0 ];

   lpDstUV[ 0 ] = lSrcUV;
   lpDstUV[ 1 ] = lSrcUV;
   lpDstUV[ 2 ] = lSrcUV;
   lpDstUV[ 3 ] = lSrcUV;
   lpDstUV[ 4 ] = lSrcUV;
   lpDstUV[ 5 ] = lSrcUV;
   lpDstUV[ 6 ] = lSrcUV;
   lpDstUV[ 7 ] = lSrcUV;

   lpDstUV =  ( uint64_t* )&lpMBDstL -> m_Cr[ 0 ][ 0 ];
   lSrcUV  = *( uint64_t* )&lpMBSrcL -> m_Cr[ 7 ][ 0 ];

   lpDstUV[ 0 ] = lSrcUV;
   lpDstUV[ 1 ] = lSrcUV;
   lpDstUV[ 2 ] = lSrcUV;
   lpDstUV[ 3 ] = lSrcUV;
   lpDstUV[ 4 ] = lSrcUV;
   lpDstUV[ 5 ] = lSrcUV;
   lpDstUV[ 6 ] = lSrcUV;
   lpDstUV[ 7 ] = lSrcUV;

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

 const int lLinesize = g_MPEGCtx.m_CurPic.m_Linesize;
        
 g_MPEGCtx.m_BlockIdx[ 0 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2     ) - 2 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 1 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2     ) - 1 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 2 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2 + 1 ) - 2 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 3 ] = g_MPEGCtx.m_B8Stride * ( g_MPEGCtx.m_MBY * 2 + 1 ) - 1 + g_MPEGCtx.m_MBX * 2;
 g_MPEGCtx.m_BlockIdx[ 4 ] = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBY     + 1 )   + g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2 + g_MPEGCtx.m_MBX - 1;
 g_MPEGCtx.m_BlockIdx[ 5 ] = g_MPEGCtx.m_MBStride * ( g_MPEGCtx.m_MBY + g_MPEGCtx.m_MBH + 2 ) + g_MPEGCtx.m_B8Stride * g_MPEGCtx.m_MBH * 2 + g_MPEGCtx.m_MBX - 1;

 g_MPEGCtx.m_pDest  = g_MPEGCtx.m_CurPic.m_pBuf -> m_pData + g_MPEGCtx.m_MBX - 1;
 g_MPEGCtx.m_pDest += g_MPEGCtx.m_MBY * lLinesize;

}  /* end SMS_MPEG_InitBlockIdx */

void SMS_MPEGContext_Init ( int aWidth, int aHeight ) {

 int lX, lY;

 memset (  &g_MPEGCtx, 0, sizeof ( g_MPEGCtx )  );

 SMS_DSPContextInit ( &g_MPEGCtx.m_DSPCtx );

 g_MPEGCtx.m_Bugs           = SMS_BUG_AUTODETECT;
 g_MPEGCtx.m_Width          = aWidth;
 g_MPEGCtx.m_Height         = aHeight;
 g_MPEGCtx.m_MBW            = ( aWidth  + 15 ) / 16;
 g_MPEGCtx.m_MBH            = ( aHeight + 15 ) / 16;
 g_MPEGCtx.m_MBStride       = g_MPEGCtx.m_MBW     + 1;
 g_MPEGCtx.m_B8Stride       = g_MPEGCtx.m_MBW * 2 + 1;
 g_MPEGCtx.m_B4Stride       = g_MPEGCtx.m_MBW * 4 + 1;
 g_MPEGCtx.m_HEdgePos       = g_MPEGCtx.m_MBW - 1;
 g_MPEGCtx.m_VEdgePos       = g_MPEGCtx.m_MBH - 1;
 g_MPEGCtx.m_MBNum          = g_MPEGCtx.m_MBW * g_MPEGCtx.m_MBH;
 g_MPEGCtx.m_BlockWrap[ 0 ] =
 g_MPEGCtx.m_BlockWrap[ 1 ] =
 g_MPEGCtx.m_BlockWrap[ 2 ] =
 g_MPEGCtx.m_BlockWrap[ 3 ] = g_MPEGCtx.m_B8Stride;
 g_MPEGCtx.m_BlockWrap[ 4 ] =
 g_MPEGCtx.m_BlockWrap[ 5 ] = g_MPEGCtx.m_MBStride;
 g_MPEGCtx.m_PicStruct      = SMS_PICT_FRAME;
 g_MPEGCtx.m_pMBIdx2XY      = calloc (  1, ( g_MPEGCtx.m_MBNum + 1 ) * sizeof ( int )  );

 for ( lY = 0; lY < g_MPEGCtx.m_MBH; ++lY )

  for ( lX = 0; lX < g_MPEGCtx.m_MBW; ++lX )

   g_MPEGCtx.m_pMBIdx2XY[ lX + lY * g_MPEGCtx.m_MBW ] = lX + lY * g_MPEGCtx.m_MBStride;

 g_MPEGCtx.m_pMBIdx2XY[ g_MPEGCtx.m_MBNum ] = ( g_MPEGCtx.m_MBH - 1 ) * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBW;

 g_MPEGCtx.m_pPic          = calloc (  1, MAX_PICTURE_COUNT * sizeof ( SMS_Frame )  );
 g_MPEGCtx.m_pErrStatTbl   = calloc (  1, ( lX = g_MPEGCtx.m_MBH * g_MPEGCtx.m_MBStride ) * sizeof ( uint8_t )  );
 g_MPEGCtx.m_pMBSkipTbl    = calloc (  1, lX + 2 );
 g_MPEGCtx.m_pPrevPicTypes = calloc (  1, PREV_PICT_TYPES_BUFFER_SIZE );
 g_MPEGCtx.m_pBSBuf        = calloc ( 1, BITSTREAM_BUFFER_SIZE );

 g_MPEGCtx.m_pBlocks     = ( void* )SMS_MPEG_SPR_BLOCKS;
 g_MPEGCtx.m_pBlock      = g_MPEGCtx.m_pBlocks[ 0 ];
 g_MPEGCtx.m_pMBIntraTbl = g_pSPRTop;

 g_pSPRTop += lX;
 g_pSPRTop  = ( unsigned char* )(  ( unsigned int )( g_pSPRTop + 15 ) & 0xFFFFFFF0  );

 SMS_Linesize ( g_MPEGCtx.m_Width, &g_MPEGCtx.m_LineStride );
 g_MPEGCtx.m_LineStride *= 384;

}  /* end SMS_MPEGContext_Init */

void SMS_MPEGContext_Destroy ( void ) {

 int i, j;

 for ( i = 0; i < MAX_PICTURE_COUNT; ++i ) {

  SMS_Frame* lpFrame = &g_MPEGCtx.m_pPic[ i ];

  if ( lpFrame -> m_pMBSkipTbl != NULL ) free ( lpFrame -> m_pMBSkipTbl );
  if ( lpFrame -> m_pQScaleTbl != NULL ) free ( lpFrame -> m_pQScaleTbl );
  if ( lpFrame -> m_pMBType    != NULL ) free ( lpFrame -> m_pMBType    );

  for ( j = 0; j < 2; ++j ) {

   if ( lpFrame -> m_pMotionVal[ j ] != NULL ) free ( lpFrame -> m_pMotionValBase[ j ] );
   if ( lpFrame -> m_pRefIdx   [ j ] != NULL ) free ( lpFrame -> m_pRefIdx       [ j ] );

  }  /* end for */

 }  /* end for */

 free ( g_MPEGCtx.m_pMBIdx2XY     );
 free ( g_MPEGCtx.m_pPic          );
 free ( g_MPEGCtx.m_pErrStatTbl   );
 free ( g_MPEGCtx.m_pMBSkipTbl    );
 free ( g_MPEGCtx.m_pPrevPicTypes );
 free ( g_MPEGCtx.m_pBSBuf        );

}  /* end SMS_MPEGContext_Destroy */

int SMS_MPEGContext_FindUnusedPic ( void ) {

 int i;
    
 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

  if ( g_MPEGCtx.m_pPic[ i ].m_pBuf  == NULL &&
       g_MPEGCtx.m_pPic[ i ].m_Type   != 0
  ) return i;

 for ( i = 0; i < MAX_PICTURE_COUNT; ++i )

  if ( g_MPEGCtx.m_pPic[ i ].m_pBuf == NULL ) return i;

 return -1;

}  /* end SMS_MPEGContext_FindUnusedPic */

void SMS_MPEG_InitScanTable ( uint8_t* apPermutation, SMS_ScanTable* apTbl, const uint8_t* apSrc ) {

 int i, lEnd;
    
 apTbl -> m_pScantable = apSrc;

 for ( i = 0; i < 64; ++i ) apTbl -> m_Permutated[ i ] = apPermutation[  apSrc[ i ]  ];

 lEnd =- 1;

 for ( i = 0; i < 64; ++i ) {

  int j = apTbl -> m_Permutated[ i ];

  if ( j > lEnd ) lEnd = j;

  apTbl -> m_RasterEnd[ i ] = lEnd;

 }  /* end for */

}  /* end SMS_MPEG_InitScanTable */

void SMS_MPEG_SetQScale ( int aQScale ) {

 if ( aQScale < 1 )

  aQScale = 1;

 else if ( aQScale > 31 ) aQScale = 31;
        
 g_MPEGCtx.m_QScale = aQScale;
 g_MPEGCtx.m_ChromaQScale = g_MPEGCtx.m_pChromaQScaleTbl[ aQScale ];

 g_MPEGCtx.m_Y_DCScale = g_MPEGCtx.m_pY_DCScaleTbl[ aQScale                  ];
 g_MPEGCtx.m_C_DCScale = g_MPEGCtx.m_pC_DCScaleTbl[ g_MPEGCtx.m_ChromaQScale ];

}  /* end SMS_MPEG_SetQScale */

static SMS_INLINE void _add_dct (
                        SMS_DCTELEM* apBlock, int anIdx, uint8_t* apDst, int aLineSize
                       ) {

 if ( g_MPEGCtx.m_BlockLastIdx[ anIdx ] >= 0 ) IDCT ( apDst, aLineSize, apBlock, 1 );

}  /* end _add_dct */

static SMS_INLINE void _add_dequant_dct (
                        SMS_DCTELEM* apBlock, int anI, uint8_t* apDest, int aLineSize, int aQScale
                       ) {

 if ( g_MPEGCtx.m_BlockLastIdx[ anI ] >= 0 ) {

  g_MPEGCtx.DCT_UnquantizeInter ( apBlock, anI, aQScale );
  IDCT ( apDest, aLineSize, apBlock, 1 );

 }  /* end if */

}  /* end _add_dequant_dct */

static SMS_INLINE void _put_dct (
                        SMS_DCTELEM* apBlock, int anIdx, uint8_t* apDst, int aLineSize, int aQScale
                       ) {

 g_MPEGCtx.DCT_UnquantizeIntra ( apBlock, anIdx, aQScale );
 IDCT ( apDst, aLineSize, apBlock, 0 );

}  /* end _put_dct */

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

 DSP_GMCn_16 ( &apDst -> m_Y [ 0 ][ 0 ], apRefPic, g_MPEGCtx.m_MBX, g_MPEGCtx.m_MBY, g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_CurPic.m_Linesize );
 DSP_GMCn_8  ( &apDst -> m_Cb[ 0 ][ 0 ], apRefPic, g_MPEGCtx.m_MBX, g_MPEGCtx.m_MBY, g_MPEGCtx.m_NoRounding, g_MPEGCtx.m_CurPic.m_Linesize );

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
 apRefPic += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;
    
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
                        uint8_t*                  apDestY,
                        uint8_t*                 apDestCb,
                        uint8_t*                 apDestCr,
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
 lpMB += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

 lSrcX &= 0xF;
 lSrcY &= 0xF;

 aQPelOp[ 0 ][ lDXY ] ( apDestY, &lpMB -> m_Y[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride );

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_QPEL_CHROMA2 ) {

  static const int lRTab[ 8 ]= { 0, 0, 1, 1, 0, 0, 0, 1 };

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
 apRefPic += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

 lSrcX &= 7;
 lSrcY &= 7;

 aPicOp[ 1 ][ lDXY ] ( apDestCb, &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX, lSrcY, g_MPEGCtx.m_LineStride );

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
 apRefPic += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

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
 apRefPic += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;
  
 aPixOp[ lDXY ] ( apDestCb, &apRefPic -> m_Cb[ 0 ][ 0 ], lSrcX & 7, lSrcY & 7, g_MPEGCtx.m_LineStride );

}  /* end _chroma_4mv_motion */

static SMS_INLINE void _MPEG_Motion (
                        uint8_t*                   aDestY,
                        uint8_t*                  aDestCb,
                        uint8_t*                  aDestCr,
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

     _gmc1_motion (  ( SMS_MacroBlock* )aDestY, apRefPic  );

    else _gmc_motion (  ( SMS_MacroBlock* )aDestY, apRefPic  );

   } else if ( g_MPEGCtx.m_QuarterSample ) {

    _qpel_motion (
     aDestY, aDestCb, aDestCr,
     apRefPic, aPixOp, aQPelOp,
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 0 ],
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 1 ], 16
    );

   } else _mpeg_motion (
           ( SMS_MacroBlock* )aDestY,
           apRefPic, aPixOp,
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
     lpMBSrc += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

     lpDest = aDestY + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

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
    lpMBSrc += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

    lpDest = aDestY + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

    aPixOp[ 2 ][ lDXY ] ( lpDest, &lpMBSrc -> m_Y[ 0 ][ 0 ], lSrcX & 0xF, lSrcY & 0xF, g_MPEGCtx.m_LineStride );

    lMX += g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
    lMY += g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

   }  /* end for */

   _chroma_4mv_motion ( aDestCb, apRefPic, aPixOp[ 1 ], lMX, lMY );

  } break;

 }  /* end switch */

}  /* end _MPEG_Motion */

void SMS_MPEG_DecodeMB ( SMS_DCTELEM aBlock[ 12 ][ 64 ] ) {

 const int lMBXY = g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBX;
 const int lAge  = g_MPEGCtx.m_CurPic.m_Age;

 uint8_t* lpDestY;
 uint8_t* lpDestCb;
 uint8_t* lpDestCr;
 uint8_t* lpMBSkip = &g_MPEGCtx.m_pMBSkipTbl[ lMBXY ];

 SMS_OpPixFunc  ( *lOpPix  )[  4 ];
 SMS_QPelMCFunc ( *lOpQPel )[ 16 ];

 g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lMBXY ] = g_MPEGCtx.m_QScale;

 if ( !g_MPEGCtx.m_MBIntra ) {

  if ( g_MPEGCtx.m_pMBIntraTbl[ lMBXY ] ) SMS_MPEG_CleanIntraTblEntries ();

 } else g_MPEGCtx.m_pMBIntraTbl[ lMBXY ] = 1;

 if ( g_MPEGCtx.m_MBSkiped ) {

  g_MPEGCtx.m_MBSkiped = 0;
 
  if ( ++*lpMBSkip > 99 ) *lpMBSkip = 99;

  if ( *lpMBSkip >= lAge && g_MPEGCtx.m_CurPic.m_Ref ) {

   int             lMBX = g_MPEGCtx.m_MBX;
   int             lMBY = g_MPEGCtx.m_MBY;
   SMS_MacroBlock* lpMB;

   lpMB  = g_MPEGCtx.m_LastPic.m_pBuf -> m_pData + lMBX;
   lpMB += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

   DSP_GetMB ( lpMB );

   goto end;

  }  /* end if */

 } else if ( !g_MPEGCtx.m_CurPic.m_Ref ) {

  if ( ++*lpMBSkip > 99 ) *lpMBSkip = 99;

 } else *lpMBSkip = 0;

 lpDestY  = &SMS_MPEG_SPR_MB -> m_Y [ 0 ][ 0 ];
 lpDestCb = &SMS_MPEG_SPR_MB -> m_Cb[ 0 ][ 0 ];
 lpDestCr = &SMS_MPEG_SPR_MB -> m_Cr[ 0 ][ 0 ];

 if ( !g_MPEGCtx.m_MBIntra ) {

  if ( !g_MPEGCtx.m_NoRounding || g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ) {

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_PutPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_PutQPelPixTab;

  } else {

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_PutNoRndPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_PutNoRndQPelPixTab;

  }  /* end else */

  if ( g_MPEGCtx.m_MVDir & SMS_MV_DIR_FORWARD ) {

   _MPEG_Motion (
    lpDestY, lpDestCb, lpDestCr, 0,
    g_MPEGCtx.m_LastPic.m_pBuf -> m_pData, lOpPix, lOpQPel
   );

   lOpPix  = g_MPEGCtx.m_DSPCtx.m_AvgPixTab;
   lOpQPel = g_MPEGCtx.m_DSPCtx.m_AvgQPelPixTab;

  }  /* end if */

  if ( g_MPEGCtx.m_MVDir & SMS_MV_DIR_BACKWARD )

   _MPEG_Motion (
    lpDestY, lpDestCb, lpDestCr, 1,
    g_MPEGCtx.m_NextPic.m_pBuf -> m_pData, lOpPix, lOpQPel
   );

  if ( g_MPEGCtx.m_pParentCtx -> m_HurryUp > 1 ) return;

  if ( g_MPEGCtx.m_MPEGQuant ) {

   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 0 ], 0, lpDestY,       16, g_MPEGCtx.m_QScale );
   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 1 ], 1, lpDestY + 8,   16, g_MPEGCtx.m_QScale );
   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 2 ], 2, lpDestY + 128, 16, g_MPEGCtx.m_QScale );
   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 3 ], 3, lpDestY + 136, 16, g_MPEGCtx.m_QScale );

   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 4 ], 4, lpDestCb, 8, g_MPEGCtx.m_ChromaQScale );
   _add_dequant_dct (  g_MPEGCtx.m_pBlock[ 5 ], 5, lpDestCr, 8, g_MPEGCtx.m_ChromaQScale );

  } else {

   _add_dct (  g_MPEGCtx.m_pBlock[ 0 ], 0, lpDestY,       16 );
   _add_dct (  g_MPEGCtx.m_pBlock[ 1 ], 1, lpDestY + 8,   16 );
   _add_dct (  g_MPEGCtx.m_pBlock[ 2 ], 2, lpDestY + 128, 16 );
   _add_dct (  g_MPEGCtx.m_pBlock[ 3 ], 3, lpDestY + 136, 16 );

   _add_dct (  g_MPEGCtx.m_pBlock[ 4 ], 4, lpDestCb, 8 );
   _add_dct (  g_MPEGCtx.m_pBlock[ 5 ], 5, lpDestCr, 8 );

  }  /* end else */

 } else {

  _put_dct ( g_MPEGCtx.m_pBlock[ 0 ], 0, lpDestY,       16, g_MPEGCtx.m_QScale );
  _put_dct ( g_MPEGCtx.m_pBlock[ 1 ], 1, lpDestY + 8,   16, g_MPEGCtx.m_QScale );
  _put_dct ( g_MPEGCtx.m_pBlock[ 2 ], 2, lpDestY + 128, 16, g_MPEGCtx.m_QScale );
  _put_dct ( g_MPEGCtx.m_pBlock[ 3 ], 3, lpDestY + 136, 16, g_MPEGCtx.m_QScale );

  _put_dct ( g_MPEGCtx.m_pBlock[ 4 ], 4, lpDestCb, 8, g_MPEGCtx.m_ChromaQScale );
  _put_dct ( g_MPEGCtx.m_pBlock[ 5 ], 5, lpDestCr, 8, g_MPEGCtx.m_ChromaQScale );

 }  /* end else */
end:
 DMA_SendS( DMAC_FROM_SPR, ( uint8_t* )g_MPEGCtx.m_pDest, SMS_MPEG_SPR_MB, 24 );

}  /* end SMS_MPEG_DecodeMB */
