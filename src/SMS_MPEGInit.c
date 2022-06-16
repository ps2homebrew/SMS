/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_MPEG.h"

#include <string.h>
#include <malloc.h>

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
 g_MPEGCtx.m_ProgSeq        = 
 g_MPEGCtx.m_ProgFrm        =
 g_MPEGCtx.m_FCode          =
 g_MPEGCtx.m_BCode          = 1;
 g_MPEGCtx.m_QuantPrec      = 5;
 g_MPEGCtx.m_pMBIdx2XY      = calloc (  1, ( g_MPEGCtx.m_MBNum + 1 ) * sizeof ( int )  );

 for ( lY = 0; lY < g_MPEGCtx.m_MBH; ++lY )

  for ( lX = 0; lX < g_MPEGCtx.m_MBW; ++lX )

   g_MPEGCtx.m_pMBIdx2XY[ lX + lY * g_MPEGCtx.m_MBW ] = lX + lY * g_MPEGCtx.m_MBStride;

 g_MPEGCtx.m_pMBIdx2XY[ g_MPEGCtx.m_MBNum ] = ( g_MPEGCtx.m_MBH - 1 ) * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBW;

 g_MPEGCtx.m_pPic       = calloc (  1, MAX_PICTURE_COUNT * sizeof ( SMS_Frame )  );
 g_MPEGCtx.m_pMBSkipTbl = calloc (  1, ( lX = g_MPEGCtx.m_MBH * g_MPEGCtx.m_MBStride ) + 2 );
 g_MPEGCtx.m_pBSBuf     = calloc ( 1, BITSTREAM_BUFFER_SIZE );

 g_MPEGCtx.m_pBlocks     = ( void* )( SMS_MPEG_SPR_BLOCKS + 8 );
 g_MPEGCtx.m_pBlock      = g_MPEGCtx.m_pBlocks[ 0 ];
 g_MPEGCtx.m_pMBIntraTbl = g_pSPRTop;

 g_pSPRTop += g_MPEGCtx.m_MBH * g_MPEGCtx.m_MBStride;
 g_pSPRTop  = ( unsigned char* )(  ( unsigned int )( g_pSPRTop + 15 ) & 0xFFFFFFF0  );

 g_MPEGCtx.m_LineSize   = (  ( g_MPEGCtx.m_Width + 15 ) >> 4  ) + 2;
 g_MPEGCtx.m_LineStride = g_MPEGCtx.m_LineSize * 384;

 g_MPEGCtx.m_MCBlkIdx    = 0;
 g_MPEGCtx.m_pMCBlk[ 0 ] = SMS_MPEG_SPR_MB_0;
 g_MPEGCtx.m_pMCBlk[ 1 ] = SMS_MPEG_SPR_MB_1;

 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[   0 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[   1 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[   2 ] = 0x01000404;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[   3 ] = 0x6D620010;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 196 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 197 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 198 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 199 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 200 ] = 0x14000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 201 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 202 ] = 0x00000000;
 (  ( unsigned int* )SMS_MPEG_SPR_BLOCKS  )[ 203 ] = 0x00000000;

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
 free ( g_MPEGCtx.m_pMBSkipTbl    );
 free ( g_MPEGCtx.m_pBSBuf        );

}  /* end SMS_MPEGContext_Destroy */
