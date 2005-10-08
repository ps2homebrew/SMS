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

#ifndef _WIN32
# include "DMA.h"
#endif  /* _WIN32 */

#include <malloc.h>
#include <string.h>
#include <limits.h>

#define MAX_PICTURE_COUNT           15
#define PREV_PICT_TYPES_BUFFER_SIZE 256
#define BITSTREAM_BUFFER_SIZE       1024 * 256

SMS_MPEGContext g_MPEGCtx;

void SMS_MPEGContext_Init ( int aWidth, int aHeight ) {

 int lX, lY;

 memset (  &g_MPEGCtx, 0, sizeof ( g_MPEGCtx )  );

 SMS_DSPContext_Init ( &g_MPEGCtx.m_DSPCtx );

 g_MPEGCtx.m_IdxRes         = 0;
 g_MPEGCtx.m_Bugs           = SMS_BUG_AUTODETECT;
 g_MPEGCtx.m_Width          = aWidth;
 g_MPEGCtx.m_Height         = aHeight;
 g_MPEGCtx.m_MBW            = ( aWidth  + 15 ) / 16;
 g_MPEGCtx.m_MBH            = ( aHeight + 15 ) / 16;
 g_MPEGCtx.m_MBStride       = g_MPEGCtx.m_MBW     + 1;
 g_MPEGCtx.m_B8Stride       = g_MPEGCtx.m_MBW * 2 + 1;
 g_MPEGCtx.m_B4Stride       = g_MPEGCtx.m_MBW * 4 + 1;
 g_MPEGCtx.m_HEdgePos       = g_MPEGCtx.m_MBW * 16;
 g_MPEGCtx.m_VEdgePos       = g_MPEGCtx.m_MBH * 16;
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
#ifdef _WIN32
 g_MPEGCtx.m_pMBIntraTbl      = calloc (  1, lX ); memset ( g_MPEGCtx.m_pMBIntraTbl, 1, lX );
 g_MPEGCtx.m_pBlocks          = calloc (  1, 64 * 6 * sizeof( SMS_DCTELEM )  );
 g_MPEGCtx.m_pBlock           = g_MPEGCtx.m_pBlocks[ 0 ];
 g_MPEGCtx.m_pMacroBlock[ 0 ] = calloc (  1, sizeof ( SMS_MacroBlock )  );
 g_MPEGCtx.m_pMacroBlock[ 1 ] = calloc (  1, sizeof ( SMS_MacroBlock )  );
 g_MPEGCtx.m_pMCBuffer        = calloc (  4, sizeof ( SMS_MacroBlock )  );
 g_MPEGCtx.m_pMCYBuf          = calloc ( 16, 256                        );
 g_MPEGCtx.m_pMCCbBuf         = calloc ( 16,  64                        );
 g_MPEGCtx.m_pMCCrBuf         = calloc ( 16,  64                        );
 g_MPEGCtx.m_pEdgeEmuBufY     = calloc ( 32, 17 );
 g_MPEGCtx.m_pEdgeEmuBufCb    = calloc ( 16,  9 );
 g_MPEGCtx.m_pEdgeEmuBufCr    = calloc ( 16,  9 );
#else  /* PS2 */
 g_MPEGCtx.m_pBlocks          = ( void* )SMS_MPEG_SPR_BLOCKS;
 g_MPEGCtx.m_pBlock           = g_MPEGCtx.m_pBlocks[ 0 ];
 g_MPEGCtx.m_pMBIntraTbl      = SMS_SPR_FREE;
 g_MPEGCtx.m_pMacroBlock[ 0 ] = ( SMS_MacroBlock* )SMS_MPEG_SPR_MB_0;
 g_MPEGCtx.m_pMacroBlock[ 1 ] = ( SMS_MacroBlock* )SMS_MPEG_SPR_MB_1;
 g_MPEGCtx.m_pMCBuffer        = SMS_MPEG_SPR_MB_BUF;
 g_MPEGCtx.m_pMCYBuf          = ( uint8_t* )SMS_MPEG_SPR_Y_BUF;
 g_MPEGCtx.m_pMCCbBuf         = ( uint8_t* )SMS_MPEG_SPR_Cb_BUF;
 g_MPEGCtx.m_pMCCrBuf         = ( uint8_t* )SMS_MPEG_SPR_Cr_BUF;
 g_MPEGCtx.m_pEdgeEmuBufY     = ( uint8_t* )g_MPEGCtx.m_pMCBuffer;
 g_MPEGCtx.m_pEdgeEmuBufCb    = g_MPEGCtx.m_pEdgeEmuBufY  + 32 * 17;
 g_MPEGCtx.m_pEdgeEmuBufCr    = g_MPEGCtx.m_pEdgeEmuBufCb + 16 *  9;
#endif  /* _WIN32 */
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
#ifdef _WIN32
 free ( g_MPEGCtx.m_pMBIntraTbl      );
 free ( g_MPEGCtx.m_pBlocks          );
 free ( g_MPEGCtx.m_pMacroBlock[ 0 ] );
 free ( g_MPEGCtx.m_pMacroBlock[ 1 ] );
 free ( g_MPEGCtx.m_pMCBuffer        );
 free ( g_MPEGCtx.m_pMCYBuf          );
 free ( g_MPEGCtx.m_pMCCbBuf         );
 free ( g_MPEGCtx.m_pMCCrBuf         );
 free ( g_MPEGCtx.m_pEdgeEmuBufY     );
 free ( g_MPEGCtx.m_pEdgeEmuBufCb    );
 free ( g_MPEGCtx.m_pEdgeEmuBufCr    );
#endif  /* _WIN32 */
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

static SMS_INLINE void _add_dct (
                        SMS_DCTELEM* apBlock, int anIdx, uint8_t* apDst, int aLineSize
                       ) {

 if ( g_MPEGCtx.m_BlockLastIdx[ anIdx ] >= 0 )

  g_MPEGCtx.m_DSPCtx.IDCT_Add ( apDst, aLineSize, apBlock );

}  /* end _add_dct */

static SMS_INLINE void _add_dequant_dct (
                        SMS_DCTELEM* apBlock, int anI, uint8_t* apDest, int aLineSize, int aQScale
                       ) {

 if ( g_MPEGCtx.m_BlockLastIdx[ anI ] >= 0 ) {

  g_MPEGCtx.DCT_UnquantizeInter ( apBlock, anI, aQScale );
  g_MPEGCtx.m_DSPCtx.IDCT_Add ( apDest, aLineSize, apBlock );

 }  /* end if */

}  /* end _add_dequant_dct */

static SMS_INLINE void _put_dct (
                        SMS_DCTELEM* apBlock, int anIdx, uint8_t* apDst, int aLineSize, int aQScale
                       ) {

 g_MPEGCtx.DCT_UnquantizeIntra ( apBlock, anIdx, aQScale );
 g_MPEGCtx.m_DSPCtx.IDCT_Put ( apDst, aLineSize, apBlock );

}  /* end _put_dct */

static void _mpeg_emulated_edge_mc (
             uint8_t* apBuf, uint8_t* apSrc, int aLinesize,
             int aBlockW, int aBlockH,
             int aSrcX, int aSrcY,
             int aW, int aH
            ) {

 int lX, lY;
 int lStartY, lStartX, lEndY, lEndX;

 if ( aSrcY >= aH )

  aSrcY  = aH - 1;

 else if ( aSrcY <= -aBlockH ) aSrcY  = 1 - aBlockH;

 if ( aSrcX >= aW )

  aSrcX  = aW - 1;

 else if ( aSrcX <= -aBlockW ) aSrcX  = 1 - aBlockW;

 lStartY = SMS_MAX( 0, -aSrcY );
 lStartX = SMS_MAX( 0, -aSrcX );

 lEndY = SMS_MIN( aBlockH, aH - aSrcY );
 lEndX = SMS_MIN( aBlockW, aW - aSrcX );
/* copy existing part */
 for ( lY = lStartY; lY < lEndY; ++lY )

  for ( lX = lStartX; lX < lEndX; ++lX )

   apBuf[ lX + lY * aLinesize ] = apSrc[ lX + lY * aLinesize ];
/* top */
 for ( lY = 0; lY < lStartY; ++lY )

  for ( lX = lStartX; lX < lEndX; ++lX )

   apBuf[ lX + lY * aLinesize ] = apBuf[ lX + lStartY * aLinesize ];
/* bottom */
 for ( lY = lEndY; lY < aBlockH; ++lY )

  for ( lX = lStartX; lX < lEndX; ++lX )

   apBuf[ lX + lY * aLinesize ] = apBuf[ lX + ( lEndY - 1 ) * aLinesize ];

 for ( lY = 0; lY < aBlockH; ++lY ) {
/* left */
  for ( lX = 0; lX < lStartX; ++lX )

   apBuf[ lX + lY * aLinesize ] = apBuf[ lStartX + lY * aLinesize ];
/* right */
  for ( lX = lEndX; lX < aBlockW; ++lX )

   apBuf[ lX + lY * aLinesize ] = apBuf[ lEndX - 1 + lY * aLinesize ];

 }  /* end for */

}  /* end _mpeg_emulated_edge_mc */

static SMS_INLINE void _mpeg_fill_mc_buffer_start ( SMS_MacroBlock* apRefPic, int aSrcX, int aSrcY ) {
#ifdef _WIN32
 int             lMBX   = ( g_MPEGCtx.m_MBX + ( aSrcX >> 4 )  );
 int             lMBY   = ( g_MPEGCtx.m_MBY + ( aSrcY >> 4 )  );
 SMS_MacroBlock* lpMB;

 if ( lMBY < -1 ) lMBY = -1;

 lpMB  = apRefPic + lMBX;
 lpMB += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

 if ( lpMB != g_MPEGCtx.m_pCache ) {

  g_MPEGCtx.m_pMCBuffer[ 0 ] = lpMB[ 0 ];
  g_MPEGCtx.m_pMCBuffer[ 1 ] = lpMB[ 1 ];
  g_MPEGCtx.m_pMCBuffer[ 2 ] = lpMB[ g_MPEGCtx.m_CurPic.m_Linesize     ];
  g_MPEGCtx.m_pMCBuffer[ 3 ] = lpMB[ g_MPEGCtx.m_CurPic.m_Linesize + 1 ];
  g_MPEGCtx.m_pCache         = lpMB;
  g_MPEGCtx.m_fDirtyCache    =    1;

 } else g_MPEGCtx.m_fDirtyCache = 0;
#else  /* PS2 */
 int             i;
 u128*           lpMCY;
 u64*            lpMCCb;
 u64*            lpMCCr;
 int             lMBX   = ( g_MPEGCtx.m_MBX + ( aSrcX >> 4 )  );
 int             lMBY   = ( g_MPEGCtx.m_MBY + ( aSrcY >> 4 )  );
 SMS_MacroBlock* lpMB   = apRefPic + lMBX;

 if ( lMBY < -1 ) lMBY = -1;
 if ( lMBX < -1 ) lMBX = -1;

 lpMB += lMBY * g_MPEGCtx.m_CurPic.m_Linesize;

 if ( lpMB != g_MPEGCtx.m_pCache ) {

  DMA_RecvSPR( g_MPEGCtx.m_pMCBuffer, lpMB, 48 );

  lpMCY  = ( u128* )g_MPEGCtx.m_pMCYBuf;
  lpMCCb = ( u64*  )g_MPEGCtx.m_pMCCbBuf;
  lpMCCr = ( u64*  )g_MPEGCtx.m_pMCCrBuf;

  g_MPEGCtx.m_pCache      = lpMB;
  g_MPEGCtx.m_fDirtyCache = 1;

  DMA_WaitToSPR();
  DMA_RecvSPR( g_MPEGCtx.m_pMCBuffer + 2, lpMB + g_MPEGCtx.m_CurPic.m_Linesize, 48 );

  for ( i = 0; i < 2; ++i ) {

   u128* lpSrcY  = ( u128* )&g_MPEGCtx.m_pMCBuffer[ i ].m_Y [ 0 ][ 0 ];
   u64*  lpSrcCb = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cb[ 0 ][ 0 ];
   u64*  lpSrcCr = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cr[ 0 ][ 0 ];

   __asm__ __volatile__ (
    ".set noat\n\t"
    "lq     $v0,   0(%0)\n\t"
    "lq     $v1,  16(%0)\n\t"
    "lq     $t5,  32(%0)\n\t"
    "lq     $t6,  48(%0)\n\t"
    "lq     $t7,  64(%0)\n\t"
    "lq     $t8,  80(%0)\n\t"
    "lq     $t9,  96(%0)\n\t"
    "lq     $at, 112(%0)\n\t"
    "sq     $v0,   0(%1)\n\t"
    "sq     $v1,  32(%1)\n\t"
    "sq     $t5,  64(%1)\n\t"
    "sq     $t6,  96(%1)\n\t"
    "sq     $t7, 128(%1)\n\t"
    "sq     $t8, 160(%1)\n\t"
    "sq     $t9, 192(%1)\n\t"
    "sq     $at, 224(%1)\n\t"
    "lq     $v0, 128(%0)\n\t"
    "lq     $v1, 144(%0)\n\t"
    "lq     $t5, 160(%0)\n\t"
    "lq     $t6, 176(%0)\n\t"
    "lq     $t7, 192(%0)\n\t"
    "lq     $t8, 208(%0)\n\t"
    "lq     $t9, 224(%0)\n\t"
    "lq     $at, 240(%0)\n\t"
    "sq     $v0, 256(%1)\n\t"
    "sq     $v1, 288(%1)\n\t"
    "sq     $t5, 320(%1)\n\t"
    "sq     $t6, 352(%1)\n\t"
    "sq     $t7, 384(%1)\n\t"
    "sq     $t8, 416(%1)\n\t"
    "sq     $t9, 448(%1)\n\t"
    "sq     $at, 480(%1)\n\t"
    ".set at\n\t"
    ::"r"( lpSrcY ), "r"( lpMCY ) : "v0", "v1", "t5", "t6", "t7", "t8", "t9", "at", "memory"
   );

   ++lpMCY;

   __asm__ __volatile__ (
    ".set noat\n\t"
    "lq     $v0,  0(%0)\n\t"
    "lq     $v1, 16(%0)\n\t"
    "lq     $t5, 32(%0)\n\t"
    "lq     $t6, 48(%0)\n\t"
    "pcpyud $t7, $v0, $zero\n\t"
    "pcpyud $t8, $v1, $zero\n\t"
    "pcpyud $t9, $t5, $zero\n\t"
    "pcpyud $at, $t6, $zero\n\t"
    "sd     $v0,   0(%1)\n\t"
    "sd     $t7,  16(%1)\n\t"
    "sd     $v1,  32(%1)\n\t"
    "sd     $t8,  48(%1)\n\t"
    "sd     $t5,  64(%1)\n\t"
    "sd     $t9,  80(%1)\n\t"
    "sd     $t6,  96(%1)\n\t"
    "sd     $at, 112(%1)\n\t"
    ".set at\n\t"
    :: "r"( lpSrcCb ), "r"( lpMCCb ): "v0", "v1", "t5", "t6", "t7", "t8", "t9", "at", "memory"
   );

   ++lpMCCb;

   __asm__ __volatile__ (
    ".set noat\n\t"
    "lq     $v0,  0(%0)\n\t"
    "lq     $v1, 16(%0)\n\t"
    "lq     $t5, 32(%0)\n\t"
    "lq     $t6, 48(%0)\n\t"
    "pcpyud $t7, $v0, $zero\n\t"
    "pcpyud $t8, $v1, $zero\n\t"
    "pcpyud $t9, $t5, $zero\n\t"
    "pcpyud $at, $t6, $zero\n\t"
    "sd     $v0,   0(%1)\n\t"
    "sd     $t7,  16(%1)\n\t"
    "sd     $v1,  32(%1)\n\t"
    "sd     $t8,  48(%1)\n\t"
    "sd     $t5,  64(%1)\n\t"
    "sd     $t9,  80(%1)\n\t"
    "sd     $t6,  96(%1)\n\t"
    "sd     $at, 112(%1)\n\t"
    ".set at\n\t"
    :: "r"( lpSrcCr ), "r"( lpMCCr ): "v0", "v1", "t5", "t6", "t7", "t8", "t9", "at", "memory"
   );

   ++lpMCCr;

  }  /* end for */

 } else g_MPEGCtx.m_fDirtyCache = 0;
#endif  /* WIN32 */
}  /* end _mpeg_fill_mc_buffer_start */

static SMS_INLINE void _mpeg_fill_mc_buffer_end ( void ) {

 if ( g_MPEGCtx.m_fDirtyCache ) {
#ifdef _WIN32
  int      i;
  int64_t* lpMCY[ 4 ] = {
   ( int64_t* )( g_MPEGCtx.m_pMCYBuf       ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 16       ),
   ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 512 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 512 + 16 )
  };
  int64_t* lpMCCb[ 4 ] = {
   ( int64_t* )( g_MPEGCtx.m_pMCCbBuf       ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 8       ),
   ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 128 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 128 + 8 )
  };
  int64_t* lpMCCr[ 4 ] = {
   ( int64_t* )( g_MPEGCtx.m_pMCCrBuf       ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 8       ),
   ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 128 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 128 + 8 )
  };

  for ( i = 0; i < 4; ++i ) {

   int64_t* lpSrcY  = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ i ].m_Y [ 0 ][ 0 ];
   int64_t* lpSrcCb = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cb[ 0 ][ 0 ];
   int64_t* lpSrcCr = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cr[ 0 ][ 0 ];

   lpMCY[ i ][  0 ] = lpSrcY[  0 ]; lpMCY[ i ][  1 ] = lpSrcY[  1 ];
   lpMCY[ i ][  4 ] = lpSrcY[  2 ]; lpMCY[ i ][  5 ] = lpSrcY[  3 ];
   lpMCY[ i ][  8 ] = lpSrcY[  4 ]; lpMCY[ i ][  9 ] = lpSrcY[  5 ];
   lpMCY[ i ][ 12 ] = lpSrcY[  6 ]; lpMCY[ i ][ 13 ] = lpSrcY[  7 ];
   lpMCY[ i ][ 16 ] = lpSrcY[  8 ]; lpMCY[ i ][ 17 ] = lpSrcY[  9 ];
   lpMCY[ i ][ 20 ] = lpSrcY[ 10 ]; lpMCY[ i ][ 21 ] = lpSrcY[ 11 ];
   lpMCY[ i ][ 24 ] = lpSrcY[ 12 ]; lpMCY[ i ][ 25 ] = lpSrcY[ 13 ];
   lpMCY[ i ][ 28 ] = lpSrcY[ 14 ]; lpMCY[ i ][ 29 ] = lpSrcY[ 15 ];
   lpMCY[ i ][ 32 ] = lpSrcY[ 16 ]; lpMCY[ i ][ 33 ] = lpSrcY[ 17 ];
   lpMCY[ i ][ 36 ] = lpSrcY[ 18 ]; lpMCY[ i ][ 37 ] = lpSrcY[ 19 ];
   lpMCY[ i ][ 40 ] = lpSrcY[ 20 ]; lpMCY[ i ][ 41 ] = lpSrcY[ 21 ];
   lpMCY[ i ][ 44 ] = lpSrcY[ 22 ]; lpMCY[ i ][ 45 ] = lpSrcY[ 23 ];
   lpMCY[ i ][ 48 ] = lpSrcY[ 24 ]; lpMCY[ i ][ 49 ] = lpSrcY[ 25 ];
   lpMCY[ i ][ 52 ] = lpSrcY[ 26 ]; lpMCY[ i ][ 53 ] = lpSrcY[ 27 ];
   lpMCY[ i ][ 56 ] = lpSrcY[ 28 ]; lpMCY[ i ][ 57 ] = lpSrcY[ 29 ];
   lpMCY[ i ][ 60 ] = lpSrcY[ 30 ]; lpMCY[ i ][ 61 ] = lpSrcY[ 31 ];

   lpMCCb[ i ][  0 ] = lpSrcCb[ 0 ];
   lpMCCb[ i ][  2 ] = lpSrcCb[ 1 ];
   lpMCCb[ i ][  4 ] = lpSrcCb[ 2 ];
   lpMCCb[ i ][  6 ] = lpSrcCb[ 3 ];
   lpMCCb[ i ][  8 ] = lpSrcCb[ 4 ];
   lpMCCb[ i ][ 10 ] = lpSrcCb[ 5 ];
   lpMCCb[ i ][ 12 ] = lpSrcCb[ 6 ];
   lpMCCb[ i ][ 14 ] = lpSrcCb[ 7 ];

   lpMCCr[ i ][  0 ] = lpSrcCr[ 0 ];
   lpMCCr[ i ][  2 ] = lpSrcCr[ 1 ];
   lpMCCr[ i ][  4 ] = lpSrcCr[ 2 ];
   lpMCCr[ i ][  6 ] = lpSrcCr[ 3 ];
   lpMCCr[ i ][  8 ] = lpSrcCr[ 4 ];
   lpMCCr[ i ][ 10 ] = lpSrcCr[ 5 ];
   lpMCCr[ i ][ 12 ] = lpSrcCr[ 6 ];
   lpMCCr[ i ][ 14 ] = lpSrcCr[ 7 ];

  }  /* end for */
#else
  int   i;
  u128* lpMCY  = ( u128* )( g_MPEGCtx.m_pMCYBuf  + 512 );
  u64*  lpMCCb = ( u64*  )( g_MPEGCtx.m_pMCCbBuf + 128 );
  u64*  lpMCCr = ( u64*  )( g_MPEGCtx.m_pMCCrBuf + 128 );

  DMA_WaitToSPR();

  for ( i = 2; i < 4; ++i ) {

   u128* lpSrcY  = ( u128* )&g_MPEGCtx.m_pMCBuffer[ i ].m_Y [ 0 ][ 0 ];
   u64*  lpSrcCb = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cb[ 0 ][ 0 ];
   u64*  lpSrcCr = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ i ].m_Cr[ 0 ][ 0 ];

   lpMCY[  0 ] = lpSrcY[  0 ]; 
   lpMCY[  2 ] = lpSrcY[  1 ]; 
   lpMCY[  4 ] = lpSrcY[  2 ]; 
   lpMCY[  6 ] = lpSrcY[  3 ]; 
   lpMCY[  8 ] = lpSrcY[  4 ]; 
   lpMCY[ 10 ] = lpSrcY[  5 ]; 
   lpMCY[ 12 ] = lpSrcY[  6 ]; 
   lpMCY[ 14 ] = lpSrcY[  7 ]; 
   lpMCY[ 16 ] = lpSrcY[  8 ]; 
   lpMCY[ 18 ] = lpSrcY[  9 ]; 
   lpMCY[ 20 ] = lpSrcY[ 10 ]; 
   lpMCY[ 22 ] = lpSrcY[ 11 ]; 
   lpMCY[ 24 ] = lpSrcY[ 12 ]; 
   lpMCY[ 26 ] = lpSrcY[ 13 ]; 
   lpMCY[ 28 ] = lpSrcY[ 14 ]; 
   lpMCY[ 30 ] = lpSrcY[ 15 ];

   ++lpMCY;

   __asm__ __volatile__ (
    ".set noat\n\t"
    "lq     $v0,  0(%0)\n\t"
    "lq     $v1, 16(%0)\n\t"
    "lq     $t5, 32(%0)\n\t"
    "lq     $t6, 48(%0)\n\t"
    "pcpyud $t7, $v0, $zero\n\t"
    "pcpyud $t8, $v1, $zero\n\t"
    "pcpyud $t9, $t5, $zero\n\t"
    "pcpyud $at, $t6, $zero\n\t"
    "sd     $v0,   0(%1)\n\t"
    "sd     $t7,  16(%1)\n\t"
    "sd     $v1,  32(%1)\n\t"
    "sd     $t8,  48(%1)\n\t"
    "sd     $t5,  64(%1)\n\t"
    "sd     $t9,  80(%1)\n\t"
    "sd     $t6,  96(%1)\n\t"
    "sd     $at, 112(%1)\n\t"
    ".set at\n\t"
    :: "r"( lpSrcCb ), "r"( lpMCCb ): "v0", "v1", "t5", "t6", "t7", "t8", "t9", "at", "memory"
   );

   ++lpMCCb;

   __asm__ __volatile__ (
    ".set noat\n\t"
    "lq     $v0,  0(%0)\n\t"
    "lq     $v1, 16(%0)\n\t"
    "lq     $t5, 32(%0)\n\t"
    "lq     $t6, 48(%0)\n\t"
    "pcpyud $t7, $v0, $zero\n\t"
    "pcpyud $t8, $v1, $zero\n\t"
    "pcpyud $t9, $t5, $zero\n\t"
    "pcpyud $at, $t6, $zero\n\t"
    "sd     $v0,   0(%1)\n\t"
    "sd     $t7,  16(%1)\n\t"
    "sd     $v1,  32(%1)\n\t"
    "sd     $t8,  48(%1)\n\t"
    "sd     $t5,  64(%1)\n\t"
    "sd     $t9,  80(%1)\n\t"
    "sd     $t6,  96(%1)\n\t"
    "sd     $at, 112(%1)\n\t"
    ".set at\n\t"
    :: "r"( lpSrcCr ), "r"( lpMCCr ): "v0", "v1", "t5", "t6", "t7", "t8", "t9", "at", "memory"
   );

   ++lpMCCr;

  }  /* end for */
#endif  /* _WIN32 */
 }  /* end if */

}  /* end _mpeg_fill_mc_buffer_end */

static SMS_INLINE void _mpeg_fill_gmc_buffer ( SMS_MacroBlock* apRefPic, int aMBX, int aMBY, int aSrcX, int aSrcY ) {
#ifdef _WIN32
 int      i, j, k;
 int      lMBX = ( aMBX + ( aSrcX >> 4 )  );
 int      lMBY = ( aMBY + ( aSrcY >> 4 )  );
 int64_t* lpMCY[ 16 ] = {
  ( int64_t* )( g_MPEGCtx.m_pMCYBuf        ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 16        ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 32        ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 48        ),
  ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 1024 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 1024 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 1024 + 32 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 1024 + 48 ),
  ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 2048 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 2048 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 2048 + 32 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 2048 + 48 ),
  ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 3072 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 3072 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 3072 + 32 ), ( int64_t* )( g_MPEGCtx.m_pMCYBuf + 3072 + 48 )
 };
 int64_t* lpMCCb[ 16 ] = {
  ( int64_t* )( g_MPEGCtx.m_pMCCbBuf       ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 8       ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 16       ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 24       ),
  ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 256 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 256 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 256 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 256 + 24 ),
  ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 512 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 512 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 512 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 512 + 24 ),
  ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 768 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 768 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 768 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCbBuf + 768 + 24 )
 };
 int64_t* lpMCCr[ 16 ] = {
  ( int64_t* )( g_MPEGCtx.m_pMCCrBuf       ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 8       ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 16       ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 24       ),
  ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 256 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 256 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 256 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 256 + 24 ),
  ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 512 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 512 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 512 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 512 + 24 ),
  ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 768 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 768 + 8 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 768 + 16 ), ( int64_t* )( g_MPEGCtx.m_pMCCrBuf + 768 + 24 )
 };
 SMS_MacroBlock* lpMB;

 lMBX = SMS_clip ( --lMBX, -1, g_MPEGCtx.m_MBW - 1 );
 lMBY = SMS_clip ( --lMBY, -1, g_MPEGCtx.m_MBH - 1 );

 lpMB  = apRefPic + lMBY * g_MPEGCtx.m_CurPic.m_Linesize;
 lpMB += lMBX;

 for ( i = 0, k = 0; i < 4; ++i ) {

  memcpy ( g_MPEGCtx.m_pMCBuffer, lpMB, 384 * 4 );

  for ( j = 0; j < 4; ++j, ++k ) {

   int64_t* lpSrcY  = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Y [ 0 ][ 0 ];
   int64_t* lpSrcCb = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cb[ 0 ][ 0 ];
   int64_t* lpSrcCr = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cr[ 0 ][ 0 ];

   lpMCY[ k ][   0 ] = lpSrcY[  0 ]; lpMCY[ k ][   1 ] = lpSrcY[  1 ];
   lpMCY[ k ][   8 ] = lpSrcY[  2 ]; lpMCY[ k ][   9 ] = lpSrcY[  3 ];
   lpMCY[ k ][  16 ] = lpSrcY[  4 ]; lpMCY[ k ][  17 ] = lpSrcY[  5 ];
   lpMCY[ k ][  24 ] = lpSrcY[  6 ]; lpMCY[ k ][  25 ] = lpSrcY[  7 ];
   lpMCY[ k ][  32 ] = lpSrcY[  8 ]; lpMCY[ k ][  33 ] = lpSrcY[  9 ];
   lpMCY[ k ][  40 ] = lpSrcY[ 10 ]; lpMCY[ k ][  41 ] = lpSrcY[ 11 ];
   lpMCY[ k ][  48 ] = lpSrcY[ 12 ]; lpMCY[ k ][  49 ] = lpSrcY[ 13 ];
   lpMCY[ k ][  56 ] = lpSrcY[ 14 ]; lpMCY[ k ][  57 ] = lpSrcY[ 15 ];
   lpMCY[ k ][  64 ] = lpSrcY[ 16 ]; lpMCY[ k ][  65 ] = lpSrcY[ 17 ];
   lpMCY[ k ][  72 ] = lpSrcY[ 18 ]; lpMCY[ k ][  73 ] = lpSrcY[ 19 ];
   lpMCY[ k ][  80 ] = lpSrcY[ 20 ]; lpMCY[ k ][  81 ] = lpSrcY[ 21 ];
   lpMCY[ k ][  88 ] = lpSrcY[ 22 ]; lpMCY[ k ][  89 ] = lpSrcY[ 23 ];
   lpMCY[ k ][  96 ] = lpSrcY[ 24 ]; lpMCY[ k ][  97 ] = lpSrcY[ 25 ];
   lpMCY[ k ][ 104 ] = lpSrcY[ 26 ]; lpMCY[ k ][ 105 ] = lpSrcY[ 27 ];
   lpMCY[ k ][ 112 ] = lpSrcY[ 28 ]; lpMCY[ k ][ 113 ] = lpSrcY[ 29 ];
   lpMCY[ k ][ 120 ] = lpSrcY[ 30 ]; lpMCY[ k ][ 121 ] = lpSrcY[ 31 ];

   lpMCCb[ k ][  0 ] = lpSrcCb[ 0 ];
   lpMCCb[ k ][  4 ] = lpSrcCb[ 1 ];
   lpMCCb[ k ][  8 ] = lpSrcCb[ 2 ];
   lpMCCb[ k ][ 12 ] = lpSrcCb[ 3 ];
   lpMCCb[ k ][ 16 ] = lpSrcCb[ 4 ];
   lpMCCb[ k ][ 20 ] = lpSrcCb[ 5 ];
   lpMCCb[ k ][ 24 ] = lpSrcCb[ 6 ];
   lpMCCb[ k ][ 28 ] = lpSrcCb[ 7 ];

   lpMCCr[ k ][  0 ] = lpSrcCr[ 0 ];
   lpMCCr[ k ][  4 ] = lpSrcCr[ 1 ];
   lpMCCr[ k ][  8 ] = lpSrcCr[ 2 ];
   lpMCCr[ k ][ 12 ] = lpSrcCr[ 3 ];
   lpMCCr[ k ][ 16 ] = lpSrcCr[ 4 ];
   lpMCCr[ k ][ 20 ] = lpSrcCr[ 5 ];
   lpMCCr[ k ][ 24 ] = lpSrcCr[ 6 ];
   lpMCCr[ k ][ 28 ] = lpSrcCr[ 7 ];

  }  /* end for */

  lpMB += g_MPEGCtx.m_CurPic.m_Linesize;

 }  /* end for */
#else  /* PS2 */
 int             i, j;
 int             lMBX = ( aMBX + ( aSrcX >> 4 )  );
 int             lMBY = ( aMBY + ( aSrcY >> 4 )  );
 u128*           lpMCY;
 u64*            lpMCCb;
 u64*            lpMCCr;
 SMS_MacroBlock* lpMB;

 lMBX = SMS_clip ( --lMBX, -1, g_MPEGCtx.m_MBW - 1 );
 lMBY = SMS_clip ( --lMBY, -1, g_MPEGCtx.m_MBH - 1 );

 lpMB  = apRefPic + lMBY * g_MPEGCtx.m_CurPic.m_Linesize;
 lpMB += lMBX;

 DMA_RecvSPR( g_MPEGCtx.m_pMCBuffer, lpMB, 48 );

 lpMCY  = ( u128* )g_MPEGCtx.m_pMCYBuf;
 lpMCCb = ( u64*  )g_MPEGCtx.m_pMCCbBuf;
 lpMCCr = ( u64*  )g_MPEGCtx.m_pMCCrBuf;

 for ( i = 0; i < 4; ++i ) {

  DMA_WaitToSPR();
  DMA_RecvSPR( g_MPEGCtx.m_pMCBuffer + 2, lpMB + 2, 48 );

  for ( j = 0; j < 2; ++j ) {

   u128* lpSrcY  = ( u128* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Y [ 0 ][ 0 ];
   u64*  lpSrcCb = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cb[ 0 ][ 0 ];
   u64*  lpSrcCr = ( u64*  )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cr[ 0 ][ 0 ];

   lpMCY[  0 ] = lpSrcY[  0 ];
   lpMCY[  4 ] = lpSrcY[  1 ];
   lpMCY[  8 ] = lpSrcY[  2 ];
   lpMCY[ 12 ] = lpSrcY[  3 ];
   lpMCY[ 16 ] = lpSrcY[  4 ];
   lpMCY[ 20 ] = lpSrcY[  5 ];
   lpMCY[ 24 ] = lpSrcY[  6 ];
   lpMCY[ 28 ] = lpSrcY[  7 ];
   lpMCY[ 32 ] = lpSrcY[  8 ];
   lpMCY[ 36 ] = lpSrcY[  9 ];
   lpMCY[ 40 ] = lpSrcY[ 10 ];
   lpMCY[ 44 ] = lpSrcY[ 11 ];
   lpMCY[ 48 ] = lpSrcY[ 12 ];
   lpMCY[ 52 ] = lpSrcY[ 13 ];
   lpMCY[ 56 ] = lpSrcY[ 14 ];
   lpMCY[ 60 ] = lpSrcY[ 15 ];

   ++lpMCY;

   lpMCCb[  0 ] = lpSrcCb[ 0 ];
   lpMCCb[  4 ] = lpSrcCb[ 1 ];
   lpMCCb[  8 ] = lpSrcCb[ 2 ];
   lpMCCb[ 12 ] = lpSrcCb[ 3 ];
   lpMCCb[ 16 ] = lpSrcCb[ 4 ];
   lpMCCb[ 20 ] = lpSrcCb[ 5 ];
   lpMCCb[ 24 ] = lpSrcCb[ 6 ];
   lpMCCb[ 28 ] = lpSrcCb[ 7 ];

   ++lpMCCb;

   lpMCCr[  0 ] = lpSrcCr[ 0 ];
   lpMCCr[  4 ] = lpSrcCr[ 1 ];
   lpMCCr[  8 ] = lpSrcCr[ 2 ];
   lpMCCr[ 12 ] = lpSrcCr[ 3 ];
   lpMCCr[ 16 ] = lpSrcCr[ 4 ];
   lpMCCr[ 20 ] = lpSrcCr[ 5 ];
   lpMCCr[ 24 ] = lpSrcCr[ 6 ];
   lpMCCr[ 28 ] = lpSrcCr[ 7 ];

   ++lpMCCr;

  }  /* end for */

  DMA_WaitToSPR();

  if ( i < 3 ) {

   lpMB += g_MPEGCtx.m_CurPic.m_Linesize;
   DMA_RecvSPR( g_MPEGCtx.m_pMCBuffer, lpMB, 48 );

  }  // end if

  for ( j = 2; j < 4; ++j ) {

   u128*    lpSrcY  = ( u128*    )&g_MPEGCtx.m_pMCBuffer[ j ].m_Y [ 0 ][ 0 ];
   int64_t* lpSrcCb = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cb[ 0 ][ 0 ];
   int64_t* lpSrcCr = ( int64_t* )&g_MPEGCtx.m_pMCBuffer[ j ].m_Cr[ 0 ][ 0 ];

   lpMCY[  0 ] = lpSrcY[  0 ];
   lpMCY[  4 ] = lpSrcY[  1 ];
   lpMCY[  8 ] = lpSrcY[  2 ];
   lpMCY[ 12 ] = lpSrcY[  3 ];
   lpMCY[ 16 ] = lpSrcY[  4 ];
   lpMCY[ 20 ] = lpSrcY[  5 ];
   lpMCY[ 24 ] = lpSrcY[  6 ];
   lpMCY[ 28 ] = lpSrcY[  7 ];
   lpMCY[ 32 ] = lpSrcY[  8 ];
   lpMCY[ 36 ] = lpSrcY[  9 ];
   lpMCY[ 40 ] = lpSrcY[ 10 ];
   lpMCY[ 44 ] = lpSrcY[ 11 ];
   lpMCY[ 48 ] = lpSrcY[ 12 ];
   lpMCY[ 52 ] = lpSrcY[ 13 ];
   lpMCY[ 56 ] = lpSrcY[ 14 ];
   lpMCY[ 60 ] = lpSrcY[ 15 ];

   ++lpMCY;

   lpMCCb[  0 ] = lpSrcCb[ 0 ];
   lpMCCb[  4 ] = lpSrcCb[ 1 ];
   lpMCCb[  8 ] = lpSrcCb[ 2 ];
   lpMCCb[ 12 ] = lpSrcCb[ 3 ];
   lpMCCb[ 16 ] = lpSrcCb[ 4 ];
   lpMCCb[ 20 ] = lpSrcCb[ 5 ];
   lpMCCb[ 24 ] = lpSrcCb[ 6 ];
   lpMCCb[ 28 ] = lpSrcCb[ 7 ];

   ++lpMCCb;

   lpMCCr[  0 ] = lpSrcCr[ 0 ];
   lpMCCr[  4 ] = lpSrcCr[ 1 ];
   lpMCCr[  8 ] = lpSrcCr[ 2 ];
   lpMCCr[ 12 ] = lpSrcCr[ 3 ];
   lpMCCr[ 16 ] = lpSrcCr[ 4 ];
   lpMCCr[ 20 ] = lpSrcCr[ 5 ];
   lpMCCr[ 24 ] = lpSrcCr[ 6 ];
   lpMCCr[ 28 ] = lpSrcCr[ 7 ];

   ++lpMCCr;

  }  /* end for */

  lpMCY  += 60;
  lpMCCb += 28;
  lpMCCr += 28;

 }  /* end for */
#endif  /* _WIN32 */
}  /* end _mpeg_fill_gmc_buffer */

static void _gmc_motion (
             uint8_t*         apDestY,
             uint8_t*        apDestCb,
             uint8_t*        apDestCr,
             SMS_MacroBlock* apRefPic
            ) {

 const int lA = g_MPEGCtx.m_SpriteWarpAccuracy;

 int            lMBX, lMBY, lSrcX, lSrcY;
 SMS_DSPGMCData lData;

 lData.m_OX = g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] + g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] * g_MPEGCtx.m_MBX * 16 + g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] * g_MPEGCtx.m_MBY * 16;
 lData.m_OY = g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] + g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] * g_MPEGCtx.m_MBX * 16 + g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] * g_MPEGCtx.m_MBY * 16;

 lSrcX = lData.m_OX >> 16;
 lSrcY = lData.m_OY >> 16;

 lSrcX >>= lA + 1;
 lSrcY >>= lA + 1;

 lMBX = lSrcX >> 4;
 lMBY = lSrcY >> 4;

 lSrcX %= 16;
 lSrcY %= 16;

 _mpeg_fill_gmc_buffer ( apRefPic, lMBX, lMBY, lSrcX, lSrcY );

 lData.m_pDst    = apDestY;
 lData.m_pSrc    = g_MPEGCtx.m_pMCYBuf + 1040;
 lData.m_Stride  = 64;
 lData.m_H       = 16; 
 lData.m_DxX     = g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ];
 lData.m_DxY     = g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ];
 lData.m_DyX     = g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ];
 lData.m_DyY     = g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ];
 lData.m_Shift   = lA + 1;
 lData.m_R       = (  1 << ( 2 * lA + 1 )  ) - g_MPEGCtx.m_NoRounding;
 lData.m_Width   = g_MPEGCtx.m_HEdgePos;
 lData.m_Height  = g_MPEGCtx.m_VEdgePos;
 lData.m_DeltaX  = SMS_clip ( lMBX << 4, 0, g_MPEGCtx.m_MBW << 4 );
 lData.m_DeltaY  = SMS_clip ( lMBY << 4, 0, g_MPEGCtx.m_MBH << 4 );
 lData.m_Rounder = 31;

 g_MPEGCtx.m_DSPCtx.GMC ( &lData );

 lData.m_pDst += 8;
 lData.m_OX   += g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] * 8;
 lData.m_OY   += g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] * 8,

 g_MPEGCtx.m_DSPCtx.GMC ( &lData );

 lData.m_pDst     = apDestCb;
 lData.m_pSrc     = g_MPEGCtx.m_pMCCbBuf + 264;
 lData.m_Stride   = 32;
 lData.m_H        =  8;
 lData.m_OX       = g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] + g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] * g_MPEGCtx.m_MBX * 8 + g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] * g_MPEGCtx.m_MBY * 8;
 lData.m_OY       = g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] + g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] * g_MPEGCtx.m_MBX * 8 + g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] * g_MPEGCtx.m_MBY * 8;
 lData.m_Width  >>=  1;
 lData.m_Height >>=  1;
 lData.m_DeltaX >>=  1;
 lData.m_DeltaY >>=  1;
 lData.m_Rounder  = 15;

 g_MPEGCtx.m_DSPCtx.GMC ( &lData );

 lData.m_pDst = apDestCr;
 lData.m_pSrc = g_MPEGCtx.m_pMCCrBuf + 264;

 g_MPEGCtx.m_DSPCtx.GMC ( &lData );

}  /* end _gmc_motion */

static SMS_INLINE void _gmc1_motion (
                        uint8_t*         apDestY,
                        uint8_t*        apDestCb,
                        uint8_t*        apDestCr,
                        SMS_MacroBlock* apRefPic
                       ) {

 uint8_t* lpPtr;
 int      lOffset, lSrcX, lSrcY;
 int      lMotionX, lMotionY;

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

 _mpeg_fill_mc_buffer_start ( apRefPic, lSrcX, lSrcY );

 if ( lSrcX < 0 ) lSrcX = 16 + lSrcX;
 if ( lSrcY < 0 ) lSrcY = 16 + lSrcY;

 lpPtr = g_MPEGCtx.m_pMCYBuf + lSrcY * 32 + lSrcX;
    
 _mpeg_fill_mc_buffer_end ();

 if (  ( lMotionX | lMotionY ) & 7  ) {

  g_MPEGCtx.m_DSPCtx.GMC1 ( apDestY,     lpPtr,     32, 16, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding );
  g_MPEGCtx.m_DSPCtx.GMC1 ( apDestY + 8, lpPtr + 8, 32, 16, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding );

 } else {

  int lDXY = (  ( lMotionX >> 3 ) & 1  ) | (  ( lMotionY >> 2 ) & 2  );

  if ( g_MPEGCtx.m_NoRounding )

   g_MPEGCtx.m_DSPCtx.m_PutNoRndPixTab[ 0 ][ lDXY ] ( apDestY, lpPtr, 32, 16 );

  else g_MPEGCtx.m_DSPCtx.m_PutPixTab[ 0 ][ lDXY ] ( apDestY, lpPtr, 32, 16 );

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

 if ( lSrcX < 0 ) lSrcX = 8 + lSrcX;
 if ( lSrcY < 0 ) lSrcY = 8 + lSrcY;

 lOffset = lSrcY * 16          + lSrcX;
 lpPtr   = g_MPEGCtx.m_pMCCbBuf + lOffset;

 g_MPEGCtx.m_DSPCtx.GMC1 (
  apDestCb, lpPtr, 16, 8, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding
 );

 lpPtr = g_MPEGCtx.m_pMCCrBuf + lOffset;

 g_MPEGCtx.m_DSPCtx.GMC1 (
  apDestCr, lpPtr, 16, 8, lMotionX & 15, lMotionY & 15, 128 - g_MPEGCtx.m_NoRounding
 );

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

 uint8_t* lpY, *lpCb, *lpCr;
 int      lDXY, lUVXY;
 int      lMX,  lMY;
 int      lSrcX,   lSrcXOrg,   lSrcY,   lSrcYOrg;
 int      lUVSrcX, lUVSrcXOrg, lUVSrcY, lUVSrcYOrg;
 int      lVEdgePos;
 int      lfEmu = 0;

 lVEdgePos = g_MPEGCtx.m_VEdgePos;
 lDXY      = (  ( aMotionY & 3 ) << 2  ) | ( aMotionX & 3 );

 lSrcX = aMotionX >> 2;
 lSrcY = aMotionY >> 2;

 _mpeg_fill_mc_buffer_start ( apRefPic, lSrcX, lSrcY );

 lSrcXOrg = g_MPEGCtx.m_MBX * 16 + ( aMotionX >> 2 );
 lSrcYOrg = g_MPEGCtx.m_MBY * 16 + ( aMotionY >> 2 );

 lSrcX %= 16;
 lSrcY %= 16;

 if ( lSrcX < 0 ) lSrcX = 16 + lSrcX;
 if ( lSrcY < 0 ) lSrcY = 16 + lSrcY;

 lpY = g_MPEGCtx.m_pMCYBuf  + lSrcY * 32 + lSrcX;

 _mpeg_fill_mc_buffer_end ();

 if (   ( unsigned )lSrcXOrg > ( unsigned )(  g_MPEGCtx.m_HEdgePos - ( aMotionX & 3 ) - 16  ) ||
        ( unsigned )lSrcYOrg > ( unsigned )(  lVEdgePos - ( aMotionY & 3 ) - aH            )
 ) {

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufY, lpY, 32, 17, 17,
   lSrcXOrg, lSrcYOrg, g_MPEGCtx.m_HEdgePos, g_MPEGCtx.m_VEdgePos
  );
  lpY   = g_MPEGCtx.m_pEdgeEmuBufY;
  lfEmu = 1;

 }  /* end if */

 aQPelOp[ 0 ][ lDXY ] ( apDestY, lpY, 32 );

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

 lUVXY = ( lMX & 1 ) | (  ( lMY & 1 ) << 1  );

 lUVSrcX = lMX >> 1;
 lUVSrcY = lMY >> 1;

 _mpeg_fill_mc_buffer_start ( apRefPic, lMX, lMY );

 lUVSrcX %= 8;
 lUVSrcY %= 8;

 lUVSrcXOrg = g_MPEGCtx.m_MBX * 8 + lUVSrcX;
 lUVSrcYOrg = g_MPEGCtx.m_MBY * 8 + lUVSrcY;

 if ( lUVSrcX < 0 ) lUVSrcX = 8 + lUVSrcX;
 if ( lUVSrcY < 0 ) lUVSrcY = 8 + lUVSrcY;

 lpCb = g_MPEGCtx.m_pMCCbBuf + lUVSrcY * 16 + lUVSrcX;
 lpCr = g_MPEGCtx.m_pMCCrBuf + lUVSrcY * 16 + lUVSrcX;

 _mpeg_fill_mc_buffer_end ();

 if ( lfEmu ) {

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufCb, lpCb, 16, 9, 9,
   lUVSrcXOrg, lUVSrcYOrg, g_MPEGCtx.m_HEdgePos >> 1, g_MPEGCtx.m_VEdgePos >> 1
  );
  lpCb = g_MPEGCtx.m_pEdgeEmuBufCb;

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufCr, lpCr, 16, 9, 9,
   lUVSrcXOrg, lUVSrcYOrg, g_MPEGCtx.m_HEdgePos >> 1, g_MPEGCtx.m_VEdgePos >> 1
  );
  lpCr = g_MPEGCtx.m_pEdgeEmuBufCr;

 }  /* end if */

 aPicOp [ 1 ][ lUVXY ] ( apDestCr, lpCr, 16, aH >> 1 );
 aPicOp [ 1 ][ lUVXY ] ( apDestCb, lpCb, 16, aH >> 1 );

}  /* end _qpel_motion */

static SMS_INLINE void _mpeg_motion (
                        uint8_t*                aDestY,
                        uint8_t*               aDestCb,
                        uint8_t*               aDestCr,
                        SMS_MacroBlock*       apRefPic,
                        SMS_OpPixFunc ( *aPicOp )[ 4 ],
                        int                   aMotionX,
                        int                   aMotionY,
                        int                         aH
                       ) {

 uint8_t* lpY, *lpCb, *lpCr;
 int      lDXY, lUVXY;
 int      lSrcX,   lSrcXOrg,   lSrcY,   lSrcYOrg;
 int      lUVSrcX, lUVSrcXOrg, lUVSrcY, lUVSrcYOrg;
 int      lVEdgePos;

 lVEdgePos = g_MPEGCtx.m_VEdgePos;
 lDXY      = (  ( aMotionY & 1 ) << 1  ) | ( aMotionX & 1 );

 lSrcX = aMotionX >> 1;
 lSrcY = aMotionY >> 1;

 lSrcXOrg = ( g_MPEGCtx.m_MBX << 4 ) + ( aMotionX >> 1 );
 lSrcYOrg = ( g_MPEGCtx.m_MBY << 4 ) + ( aMotionY >> 1 );

 _mpeg_fill_mc_buffer_start ( apRefPic, lSrcX, lSrcY );

 lSrcX %= 16;
 lSrcY %= 16;

 if ( lSrcX < 0 ) lSrcX = 16 + lSrcX;
 if ( lSrcY < 0 ) lSrcY = 16 + lSrcY;

 lUVXY = lDXY | ( aMotionY & 2 ) | (  ( aMotionX & 2 ) >> 1  );

 lUVSrcX = lSrcX >> 1;
 lUVSrcY = lSrcY >> 1;

 lUVSrcXOrg = lSrcXOrg >> 1;
 lUVSrcYOrg = lSrcYOrg >> 1;

 lpY  = g_MPEGCtx.m_pMCYBuf  + lSrcY   * 32 + lSrcX;
 lpCb = g_MPEGCtx.m_pMCCbBuf + lUVSrcY * 16 + lUVSrcX;
 lpCr = g_MPEGCtx.m_pMCCrBuf + lUVSrcY * 16 + lUVSrcX;

 _mpeg_fill_mc_buffer_end ();

 if  (   ( unsigned )lSrcXOrg > ( unsigned )(  g_MPEGCtx.m_HEdgePos - ( aMotionX & 1 ) - 16  ) ||
         ( unsigned )lSrcYOrg > ( unsigned )(  lVEdgePos - ( aMotionY & 1 ) - aH            )
 ) {

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufY, lpY, 32, 17, 17,
   lSrcXOrg, lSrcYOrg, g_MPEGCtx.m_HEdgePos, g_MPEGCtx.m_VEdgePos
  );
  lpY = g_MPEGCtx.m_pEdgeEmuBufY;

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufCb, lpCb, 16, 9, 9,
   lUVSrcXOrg, lUVSrcYOrg, g_MPEGCtx.m_HEdgePos >> 1, g_MPEGCtx.m_VEdgePos >> 1
  );
  lpCb = g_MPEGCtx.m_pEdgeEmuBufCb;

  _mpeg_emulated_edge_mc (
   g_MPEGCtx.m_pEdgeEmuBufCr, lpCr, 16, 9, 9,
   lUVSrcXOrg, lUVSrcYOrg, g_MPEGCtx.m_HEdgePos >> 1, g_MPEGCtx.m_VEdgePos >> 1
  );
  lpCr = g_MPEGCtx.m_pEdgeEmuBufCr;

 }  /* end if */

 aPicOp[ 0 ][ lDXY  ] ( aDestY,  lpY,  32, aH      );
 aPicOp[ 1 ][ lUVXY ] ( aDestCb, lpCb, 16, aH >> 1 );
 aPicOp[ 1 ][ lUVXY ] ( aDestCr, lpCr, 16, aH >> 1 );

}  /* end _mpeg_motion */

static SMS_INLINE void _chroma_4mv_motion (
                        uint8_t*        apDestCb,
                        uint8_t*        apDestCr,
                        SMS_MacroBlock* apRefPic,
                        SMS_OpPixFunc*    aPixOp,
                        int                  aMX,
                        int                  aMY
                       ) {

 int      lDXY, lSrcX, lSrcY, lOffset;
 uint8_t* lpPtr;

 aMX = SMS_H263_RoundChroma ( aMX );
 aMY = SMS_H263_RoundChroma ( aMY );

 lDXY  = (  ( aMY & 1 ) << 1  ) | ( aMX & 1 );

 lSrcX = aMX >> 1;
 lSrcY = aMY >> 1;

 aMX = lSrcX << 1;
 aMY = lSrcY << 1;

 _mpeg_fill_mc_buffer_start ( apRefPic, aMX, aMY );

 lSrcX %= 8;
 lSrcY %= 8;
    
 if ( lSrcX < 0 ) lSrcX = 8 + lSrcX;
 if ( lSrcY < 0 ) lSrcY = 8 + lSrcY;

 lOffset = lSrcY * 16 + lSrcX;
 lpPtr   = g_MPEGCtx.m_pMCCbBuf + lOffset;

 _mpeg_fill_mc_buffer_end ();

 aPixOp[ lDXY ] ( apDestCb, lpPtr, 16, 8 );

 lpPtr = g_MPEGCtx.m_pMCCrBuf + lOffset;

 aPixOp[ lDXY ] ( apDestCr, lpPtr, 16, 8 );

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

     _gmc1_motion ( aDestY, aDestCb, aDestCr, apRefPic );

    else _gmc_motion ( aDestY, aDestCb, aDestCr, apRefPic );

   } else if ( g_MPEGCtx.m_QuarterSample ) {

    _qpel_motion (
     aDestY, aDestCb, aDestCr,
     apRefPic, aPixOp, aQPelOp,
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 0 ],
     g_MPEGCtx.m_MV[ aDir ][ 0 ][ 1 ], 16
    );

   } else _mpeg_motion (
           aDestY, aDestCb, aDestCr,
           apRefPic, aPixOp,
           g_MPEGCtx.m_MV[ aDir ][ 0 ][ 0 ],
           g_MPEGCtx.m_MV[ aDir ][ 0 ][ 1 ], 16
          );
           
  break;

  case SMS_MV_TYPE_8X8: {

   int      lMotionX, lMotionY;
   int      lDXY;
   int      lSrcX, lSrcY;
   int      lMX = 0;
   int      lMY = 0;
   uint8_t* lpPtr;
   uint8_t* lpDest;

   if ( g_MPEGCtx.m_QuarterSample )

    for ( i = 0; i < 4; ++i ) {

     lMotionX = g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
     lMotionY = g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

     lDXY = (  ( lMotionY & 3 ) << 2  ) | ( lMotionX & 3 );

     lSrcX = ( lMotionX >> 2 ) + ( i  & 1 ) * 8;
     lSrcY = ( lMotionY >> 2 ) + ( i >> 1 ) * 8;
                    
     _mpeg_fill_mc_buffer_start ( apRefPic, lSrcX, lSrcY );

     lSrcX %= 16;
     lSrcY %= 16;

     if ( lSrcX < 0 ) lSrcX = 16 + lSrcX;
     if ( lSrcY < 0 ) lSrcY = 16 + lSrcY;

     lpPtr  = g_MPEGCtx.m_pMCYBuf  + lSrcY * 32 + lSrcX;
     lpDest = aDestY + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

     _mpeg_fill_mc_buffer_end ();

     aQPelOp[ 2 ][ lDXY ] ( lpDest, lpPtr, 32 );

     lMX += g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ] / 2;
     lMY += g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ] / 2;

    }  /* end for */

   else for ( i = 0; i < 4; ++i ) {

    lMotionX = g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
    lMotionY = g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

    lDXY = (  ( lMotionY & 1 ) << 1 ) | ( lMotionX & 1 );

    lSrcX = ( lMotionX >> 1 ) + ( i  & 1 ) * 8;
    lSrcY = ( lMotionY >> 1 ) + ( i >> 1 ) * 8;

    _mpeg_fill_mc_buffer_start ( apRefPic, lSrcX, lSrcY );

    lSrcX %= 16;
    lSrcY %= 16;

    if ( lSrcX < 0 ) lSrcX = 16 + lSrcX;
    if ( lSrcY < 0 ) lSrcY = 16 + lSrcY;

    lpPtr  = g_MPEGCtx.m_pMCYBuf  + lSrcY * 32 + lSrcX;
    lpDest = aDestY + (  ( i & 1 ) * 8  ) + ( i >> 1 ) * 8 * 16;

    _mpeg_fill_mc_buffer_end ();

    aPixOp[ 2 ][ lDXY ] ( lpDest, lpPtr, 32, 8 );

    lMX += g_MPEGCtx.m_MV[ aDir ][ i ][ 0 ];
    lMY += g_MPEGCtx.m_MV[ aDir ][ i ][ 1 ];

   }  /* end for */

   _chroma_4mv_motion (
    aDestCb, aDestCr, apRefPic, aPixOp[ 1 ], lMX, lMY
   );

  } break;

 }  /* end switch */

}  /* end _MPEG_Motion */

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
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[ 0 ][  4 ],  64 );

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
  memset ( &lpMBDstL -> m_Cr[ 0 ][ 0 ], lpMBSrcL -> m_Cr[  7 ][  4 ],  64 );

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

static SMS_INLINE void MPEG_CopyBlock ( void ) {
#ifdef _WIN32
 *g_MPEGCtx.m_pDest = *g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ];
#else  /* PS2 */
 DMA_SendSPRToMem(
  ( uint8_t* )g_MPEGCtx.m_pDest, ( uint8_t* )g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ], 24
 );
 g_MPEGCtx.m_IdxRes = !g_MPEGCtx.m_IdxRes;
#endif  /* _WIN32 */
}  /* end SMS_MPEG_CopyBlocks */

void SMS_MPEG_DecodeMB ( SMS_DCTELEM aBlock[ 12 ][ 64 ] ) {

 const int lMBXY       = g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBX;
 const int lLinesize   = 16;
 const int lUVLinesize =  8;
 const int lBlockSize  =  8;
 const int lAge        = g_MPEGCtx.m_CurPic.m_Age;

 int      lDCTLineSize, lDCTOffset;
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
#ifdef _WIN32
   *g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ] = *lpMB;
#else  /* PS2 */
   DMA_RecvSPR(  ( uint8_t* )g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ], lpMB, 24 );
   DMA_WaitToSPR();
#endif  /* _WIN32 */
   goto end;

  }  /* end if */

 } else if ( !g_MPEGCtx.m_CurPic.m_Ref ) {

  if ( ++*lpMBSkip > 99 ) *lpMBSkip = 99;

 } else *lpMBSkip = 0;

 lDCTLineSize = lLinesize << g_MPEGCtx.m_InterlacedDCT;
 lDCTOffset   = ( g_MPEGCtx.m_InterlacedDCT ) ? lLinesize : lLinesize * lBlockSize;

 lpDestY  = &g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ] -> m_Y [ 0 ][ 0 ];
 lpDestCb = &g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ] -> m_Cb[ 0 ][ 0 ];
 lpDestCr = &g_MPEGCtx.m_pMacroBlock[ g_MPEGCtx.m_IdxRes ] -> m_Cr[ 0 ][ 0 ];

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

   _add_dequant_dct ( aBlock[ 0 ], 0, lpDestY,                           lDCTLineSize, g_MPEGCtx.m_QScale );
   _add_dequant_dct ( aBlock[ 1 ], 1, lpDestY + lBlockSize,              lDCTLineSize, g_MPEGCtx.m_QScale );
   _add_dequant_dct ( aBlock[ 2 ], 2, lpDestY + lDCTOffset,              lDCTLineSize, g_MPEGCtx.m_QScale );
   _add_dequant_dct ( aBlock[ 3 ], 3, lpDestY + lDCTOffset + lBlockSize, lDCTLineSize, g_MPEGCtx.m_QScale );

   _add_dequant_dct ( aBlock[ 4 ], 4, lpDestCb, lUVLinesize, g_MPEGCtx.m_ChromaQScale );
   _add_dequant_dct ( aBlock[ 5 ], 5, lpDestCr, lUVLinesize, g_MPEGCtx.m_ChromaQScale );

  } else {

   _add_dct ( aBlock[ 0 ], 0, lpDestY,                           lDCTLineSize );
   _add_dct ( aBlock[ 1 ], 1, lpDestY + lBlockSize,              lDCTLineSize );
   _add_dct ( aBlock[ 2 ], 2, lpDestY + lDCTOffset,              lDCTLineSize );
   _add_dct ( aBlock[ 3 ], 3, lpDestY + lDCTOffset + lBlockSize, lDCTLineSize );

   _add_dct ( aBlock[ 4 ], 4, lpDestCb, lUVLinesize );
   _add_dct ( aBlock[ 5 ], 5, lpDestCr, lUVLinesize );

  }  /* end else */

 } else {

  _put_dct ( aBlock[ 0 ], 0, lpDestY,                           lDCTLineSize, g_MPEGCtx.m_QScale );
  _put_dct ( aBlock[ 1 ], 1, lpDestY + lBlockSize,              lDCTLineSize, g_MPEGCtx.m_QScale );
  _put_dct ( aBlock[ 2 ], 2, lpDestY + lDCTOffset,              lDCTLineSize, g_MPEGCtx.m_QScale );
  _put_dct ( aBlock[ 3 ], 3, lpDestY + lDCTOffset + lBlockSize, lDCTLineSize, g_MPEGCtx.m_QScale );

  _put_dct ( aBlock[ 4 ], 4, lpDestCb, lUVLinesize, g_MPEGCtx.m_ChromaQScale );
  _put_dct ( aBlock[ 5 ], 5, lpDestCr, lUVLinesize, g_MPEGCtx.m_ChromaQScale );

 }  /* end else */
end:
 MPEG_CopyBlock ();

}  /* end SMS_MPEG_DecodeMB */

void SMS_MPEG_CleanBuffers ( void ) {

 g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 0 ] =
 g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 1 ] =
 g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 0 ] =
 g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 1 ] = 0;

}  /* end SMS_MPEG_CleanBuffers */
