/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "IPU.h"
#include "GS.h"
#include "SMS.h"
#include "SMS_VideoBuffer.h"

IPUContext g_IPUCtx;

#ifdef _WIN32
static void IPU_Destroy ( void ) {

}  /* end IPU_Destroy */

static void IPU_Sync ( void ) {

}  /* end IPU_Sync */

static void IPU_Display ( struct SMS_FrameBuffer* apMB ) {

 if ( g_IPUCtx.m_pGSCtx -> m_pVideo ) {

  uint32_t        lY, lX;
  uint8_t*        lpVideoY  = ( uint8_t* )g_IPUCtx.m_pGSCtx -> m_pVideo;
  uint8_t*        lpVideoCr = lpVideoY  + g_IPUCtx.m_Width * g_IPUCtx.m_Height;
  uint8_t*        lpVideoCb = lpVideoCr + (  ( g_IPUCtx.m_Width * g_IPUCtx.m_Height ) >> 2  );
  SMS_MacroBlock* lpData    = apMB -> m_pData;

  for ( lY = 0; lY < g_IPUCtx.m_MBHeight; ++lY ) {

   SMS_MacroBlock* lpMB = lpData;

   for ( lX = 0; lX < g_IPUCtx.m_MBWidth; ++lX, ++lpMB ) {

    int      i;
    uint8_t* lpVideo = lpVideoY;

    for ( i = 0; i < 16; ++i ) {

     memcpy ( lpVideo, &lpMB -> m_Y[ i ][ 0 ], 16 );
     lpVideo += g_IPUCtx.m_Width;

    }  /* end for */

    lpVideoY += 16;
    lpVideo   = lpVideoCb;

    for ( i = 0; i < 8; ++i ) {

     memcpy ( lpVideo, &lpMB -> m_Cb[ i ][ 0 ], 8 );
     lpVideo += g_IPUCtx.m_UVWidth;

    }  /* end for */

    lpVideoCb += 8;
    lpVideo    = lpVideoCr;

    for ( i = 0; i < 8; ++i ) {

     memcpy ( lpVideo, &lpMB -> m_Cr[ i ][ 0 ], 8 );
     lpVideo += g_IPUCtx.m_UVWidth;

    }  /* end for */

    lpVideoCr += 8;

   }  /* end for */

   lpVideoY  += 15 * g_IPUCtx.m_Width;
   lpVideoCb +=  7 * g_IPUCtx.m_UVWidth;
   lpVideoCr +=  7 * g_IPUCtx.m_UVWidth;

   lpData += g_IPUCtx.m_Linesize;

  }  /* end for */

  PostMessage ( g_IPUCtx.m_pGSCtx -> m_hWnd, WM_APP, 1, 0 );

 }  /* end if */

}  /* end IPU_Display */

IPUContext* IPU_InitContext ( GSContext* apGSCtx, int aWidth, int aHeight ) {

 unsigned int lLeft,  lTop,  lRight,  lBottom;
 float        lAR   = (  ( float )aWidth  ) / (  ( float )aHeight  );

 SMS_Linesize ( aWidth, &g_IPUCtx.m_Linesize );

 if ( apGSCtx -> m_Width < apGSCtx -> m_Height * lAR ) {

  int lH = ( int )( apGSCtx -> m_Width / lAR );

  lLeft  = 0;
  lRight = apGSCtx -> m_Width;

  lTop    = ( apGSCtx -> m_Height - lH ) >> 1;
  lBottom = lTop + lH;

 } else {

  int lW = ( int )( apGSCtx -> m_Height * lAR );

  lTop    = 0;
  lBottom = apGSCtx -> m_Height;

  lLeft  = ( apGSCtx -> m_Width - lW ) >> 1;
  lRight = lLeft + lW;

 }  /* end else */

 if ( apGSCtx -> m_hWnd ) {

  LPARAM lParam = *( LPARAM* )&lAR;

  SendMessage (  apGSCtx -> m_hWnd, WM_APP, 2, lParam );
  SendMessage (  apGSCtx -> m_hWnd, WM_APP, 3, MAKELPARAM( aHeight, aWidth )  );

 }  /* end if */

 g_IPUCtx.m_MBWidth  = aWidth  >> 4;
 g_IPUCtx.m_MBHeight = aHeight >> 4;
 g_IPUCtx.m_Width    = aWidth;
 g_IPUCtx.m_UVWidth  = aWidth  >> 1;
 g_IPUCtx.m_Height   = aHeight;
 g_IPUCtx.m_UVHeight = aHeight >> 1;
 g_IPUCtx.m_pGSCtx   = apGSCtx;
 g_IPUCtx.Destroy    = IPU_Destroy;
 g_IPUCtx.Display    = IPU_Display;
 g_IPUCtx.Sync       = IPU_Sync;

 return &g_IPUCtx;

}  /* end IPU_InitContext */
#else  /* PS2 */
# include "DMA.h"
# include <kernel.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>

# define VIF_PIDX (  sizeof ( g_IPUCtx.m_DMAVIFDraw ) / sizeof ( g_IPUCtx.m_DMAVIFDraw[ 0 ] ) - 2  )
# define ViF_PIDX (  sizeof ( g_IPUCtx.m_DMAViFDraw ) / sizeof ( g_IPUCtx.m_DMAViFDraw[ 0 ] ) - 2  )

static unsigned long int* s_pVIFPacket;
static unsigned long int* s_pViFPacket;

static void IPU_DestroyContext ( void ) {

 if ( g_IPUCtx.m_pResult != NULL ) {

  DisableDmac ( DMA_CHANNEL_GIF      );
  DisableDmac ( DMA_CHANNEL_FROM_IPU );
  RemoveDmacHandler ( DMA_CHANNEL_FROM_IPU, g_IPUCtx.m_DMAHandlerID_IPU );
  RemoveDmacHandler ( DMA_CHANNEL_GIF,      g_IPUCtx.m_DMAHandlerID_GIF );
#ifdef VB_SYNC
  DisableIntc ( 2 );
  DisableIntc ( 3 );
  RemoveIntcHandler ( 2, g_IPUCtx.m_VBlankStartHandlerID );
  RemoveIntcHandler ( 3, g_IPUCtx.m_VBlankEndHandlerID   );
#endif  /* VB_SYNC */
  DeleteSema ( g_IPUCtx.m_SyncS );

  free ( g_IPUCtx.m_pResult );
  g_IPUCtx.m_pResult = NULL;

 }  /* end if */

}  /* end IPU_DestroyContext */

static void IPU_Sync ( void ) {

 WaitSema ( g_IPUCtx.m_SyncS );
 SignalSema ( g_IPUCtx.m_SyncS );

}  /* end IPU_Sync */

static void IPU_SetTEX ( void ) {

 g_IPUCtx.m_DMAGIFTX[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 g_IPUCtx.m_DMAGIFTX[ 1 ] = GIF_AD;

 g_IPUCtx.m_DMAGIFTX[ 2 ] = GS_SETREG_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 g_IPUCtx.m_DMAGIFTX[ 3 ] = GS_TEX1_1;

 DMA_Wait ( DMA_CHANNEL_GIF );
 DMA_Send ( DMA_CHANNEL_GIF, g_IPUCtx.m_DMAGIFTX, 2 );

}  /* end IPU_SetTEX */

int IPU_DMAHandlerFromIPU ( int aChan ) {

 int32_t  i, j = 6, k = 14, lX = g_IPUCtx.m_DestX;
 uint8_t* lpRes = g_IPUCtx.m_pCurRes;

 for ( i = 0; i < 8; ++i ) {

  SMS_IPU_SPR_PKT_BUF[ j ] = GS_SETREG_TRXPOS( 0, 0, lX, g_IPUCtx.m_DestY, 0 );
  lX += 16;
  SMS_IPU_SPR_PKT_BUF[ k ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )lpRes, 0  );

  j     +=   16;
  k     +=   16;
  lpRes += 1024;

 }  /* end for */

 g_IPUCtx.m_GIFlag = 1;

 DMA_SendChainToGIF_SPR( SMS_IPU_SPR_DMA_PKT_BUF );

 return 0;

}  /* end IPU_DMAHandlerFromIPU */

static void IPU_GIFHandlerDraw ( void ) {

 iSignalSema ( g_IPUCtx.m_SyncS );

}  /* end IPU_GIFHandlerDraw */
#ifdef VB_SYNC
static int IPU_VBlankStartHandler ( int aCause ) {

 if ( g_IPUCtx.m_fDraw ) {

  if ( g_IPUCtx.m_VIFQueueSize != VIF_PIDX + 2 ) {

   DMA_SendChainToVIF1 ( &g_IPUCtx.m_DMAVIFDraw[ g_IPUCtx.m_VIFQueueSize ] );
   g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;

  }  /* end if */

  if ( g_IPUCtx.m_ViFQueueSize != ViF_PIDX + 2 ) {

   DMA_SendChainToVIF1 ( &g_IPUCtx.m_DMAViFDraw[ g_IPUCtx.m_ViFQueueSize ] );
   g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;

  }  /* end if */

  g_IPUCtx.m_fDraw    = 0;
  g_IPUCtx.m_GIFlag   = 1;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
  DMA_SendToGIF( g_IPUCtx.m_DMAGIFDraw, 8 );

 } else g_IPUCtx.m_fBlank = 1;

 return -1;

}  /* end IPU_VBlankStartHandler */

static int IPU_VBlankEndHandler ( int aCause ) {

 g_IPUCtx.m_fBlank = 0;

 return -1;

}  /* end IPU_VBlankEndHandler */
#endif  /* VB_SYNC */
void IPU_GIFHandlerSend ( void ) {

 if (  ( g_IPUCtx.m_MB += 8 ) >= g_IPUCtx.m_nMBSlice  ) {

  if ( !--g_IPUCtx.m_Slice ) {
#ifdef VB_SYNC
   if ( !g_IPUCtx.m_fBlank )

    g_IPUCtx.m_fDraw = 1;

   else {
#endif  /* VB_SYNC */
  if ( g_IPUCtx.m_VIFQueueSize != VIF_PIDX + 2 ) {

   DMA_SendChainToVIF1 ( &g_IPUCtx.m_DMAVIFDraw[ g_IPUCtx.m_VIFQueueSize ] );
   g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;

  }  /* end if */

  if ( g_IPUCtx.m_ViFQueueSize != ViF_PIDX + 2 ) {

   DMA_SendChainToVIF1 ( &g_IPUCtx.m_DMAViFDraw[ g_IPUCtx.m_ViFQueueSize ] );
   g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;

  }  /* end if */

  g_IPUCtx.m_GIFlag   = 1;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
  DMA_SendToGIF( g_IPUCtx.m_DMAGIFDraw, 8 );
#ifdef VB_SYNC
   }  /* end else */
#endif  /* VB_SYNC */
   return;

  }  /* end if */

  g_IPUCtx.m_DestX   =  0;
  g_IPUCtx.m_DestY  += 16;
  g_IPUCtx.m_MB      =  0;
  g_IPUCtx.m_pCurRes = g_IPUCtx.m_pResult;
  g_IPUCtx.m_pMB    += g_IPUCtx.m_MBStride;

  DMA_SendToIPU( g_IPUCtx.m_pMB, g_IPUCtx.m_QWCToIPUSlice );
  IPU_CMD_CSC( g_IPUCtx.m_nMBSlice, 0, 0 );
  DMA_RecvFromIPU( g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

 } else {

  g_IPUCtx.m_DestX   +=  128;
  g_IPUCtx.m_pCurRes += 8192;

  SMS_IPU_SPR_PKT_BUF[   6 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX,      g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  14 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes,        0  );
  SMS_IPU_SPR_PKT_BUF[  22 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 16, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  30 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 1024, 0  );
  SMS_IPU_SPR_PKT_BUF[  38 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 32, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  46 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 2048, 0  );
  SMS_IPU_SPR_PKT_BUF[  54 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 48, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  62 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 3072, 0  );
  SMS_IPU_SPR_PKT_BUF[  70 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 64, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  78 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 4096, 0  );
  SMS_IPU_SPR_PKT_BUF[  86 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 80, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[  94 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 5120, 0  );
  SMS_IPU_SPR_PKT_BUF[ 102 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 96, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[ 110 ] = DMA_TAG( 64, 1, DMA_REF, 0, ( u32 )g_IPUCtx.m_pCurRes + 6144, 0  );
  SMS_IPU_SPR_PKT_BUF[ 118 ] = GS_SETREG_TRXPOS( 0, 0, g_IPUCtx.m_DestX + 112, g_IPUCtx.m_DestY, 0 );
  SMS_IPU_SPR_PKT_BUF[ 126 ] = DMA_TAG( 64, 1, DMA_REFE, 0, ( u32 )g_IPUCtx.m_pCurRes + 7168, 0  );

  g_IPUCtx.m_GIFlag = 1;
  DMA_SendChainToGIF_SPR( SMS_IPU_SPR_DMA_PKT_BUF );

 }  /* end else */

}  /* end IPU_GIFHandlerSend */

int IPU_DMAHandlerToGIF ( int aChan ) {

 if ( g_IPUCtx.m_GIFlag ) {

  g_IPUCtx.m_GIFlag = 0;
  g_IPUCtx.GIFHandler ();

 }  /* end if */

 return 0;

}  /* end IPU_DMAHandlerToGIF */

static void IPU_Display ( struct SMS_FrameBuffer* apMB ) {

 WaitSema ( g_IPUCtx.m_SyncS );

 g_IPUCtx.m_DestX    = 0;
 g_IPUCtx.m_DestY    = 0;
 g_IPUCtx.m_MB       = 0;
 g_IPUCtx.m_Slice    = g_IPUCtx.m_nMBSlices;
 g_IPUCtx.m_pCurRes  = g_IPUCtx.m_pResult;
 g_IPUCtx.m_pMB      = apMB -> m_pData;
 g_IPUCtx.m_pBuffer  = apMB;
 g_IPUCtx.m_GIFlag   = 0;
#ifdef VB_SYNC
 g_IPUCtx.m_fDraw    = 0;
#endif  /* VB_SYNC */
 g_IPUCtx.GIFHandler = IPU_GIFHandlerSend;

 DMA_SendToIPU( apMB -> m_pData, g_IPUCtx.m_QWCToIPUSlice );
 IPU_CMD_CSC( g_IPUCtx.m_nMBSlice, 0, 0 );
 DMA_RecvFromIPU( g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

}  /* end IPU_Display */

static void IPU_Reset ( struct GSContext* apGSCtx ) {

 float lAR = (  ( float )g_IPUCtx.m_Width ) / (  ( float )g_IPUCtx.m_Height  );

 g_IPUCtx.m_TxtLeft   =
 g_IPUCtx.m_TxtTop    = g_IPUCtx.m_ScrLeft; 
 g_IPUCtx.m_TxtRight  = ( g_IPUCtx.m_Width  << 4 ) + g_IPUCtx.m_TxtLeft;
 g_IPUCtx.m_TxtBottom = ( g_IPUCtx.m_Height << 4 ) + g_IPUCtx.m_TxtLeft;

 if ( g_IPUCtx.m_ScrWidth < g_IPUCtx.m_ScrHeight * lAR ) {

  int lH = ( int )( g_IPUCtx.m_ScrWidth / lAR );

  g_IPUCtx.m_ImgLeft  = g_IPUCtx.m_TxtLeft;
  g_IPUCtx.m_ImgRight = ( g_IPUCtx.m_ScrWidth << 4 ) + g_IPUCtx.m_TxtLeft;

  g_IPUCtx.m_ImgTop    = ( g_IPUCtx.m_ScrHeight - lH ) >> 1;
  g_IPUCtx.m_ImgBottom = g_IPUCtx.m_ImgTop + lH;

  g_IPUCtx.m_ImgTop <<= 3;
  g_IPUCtx.m_ImgTop  += g_IPUCtx.m_ScrTop;

  g_IPUCtx.m_ImgBottom <<= 3;
  g_IPUCtx.m_ImgBottom  += g_IPUCtx.m_ScrTop;

 } else {

  int lW = ( int )( g_IPUCtx.m_ScrHeight * lAR );

  g_IPUCtx.m_ImgTop    = g_IPUCtx.m_ScrTop;
  g_IPUCtx.m_ImgBottom = ( g_IPUCtx.m_ScrHeight << 3 ) + g_IPUCtx.m_ScrTop;

  g_IPUCtx.m_ImgLeft  = ( g_IPUCtx.m_ScrWidth - lW ) >> 1;
  g_IPUCtx.m_ImgRight = g_IPUCtx.m_ImgLeft + lW;

  g_IPUCtx.m_ImgLeft <<= 4;
  g_IPUCtx.m_ImgLeft  += g_IPUCtx.m_TxtLeft;

  g_IPUCtx.m_ImgRight <<= 4;
  g_IPUCtx.m_ImgRight  += g_IPUCtx.m_TxtLeft;

  g_IPUCtx.m_ImgLeft  = g_IPUCtx.m_TxtLeft;
  g_IPUCtx.m_ImgRight = ( g_IPUCtx.m_ScrWidth << 4 ) + g_IPUCtx.m_TxtLeft;

 }  /* end else */
#ifdef BIMBO69
   if ( apGSCtx -> m_DisplayMode == GSDisplayMode_PAL   ||
        apGSCtx -> m_DisplayMode == GSDisplayMode_PAL_I
   ) {

    g_IPUCtx.m_ImgTop    -= ( 12 << 4 );
    g_IPUCtx.m_ImgBottom += ( 12 << 4 );

   } else if (  ( apGSCtx -> m_DisplayMode == GSDisplayMode_NTSC   ||
                  apGSCtx -> m_DisplayMode == GSDisplayMode_NTSC_I
                ) && lAR >= 1.43F
          ) {

    g_IPUCtx.m_ImgTop    += ( 12 << 4 );
    g_IPUCtx.m_ImgBottom -= ( 12 << 4 );

   }  /* end if */
#endif  /* BIMBO69 */
 g_IPUCtx.m_DMAGIFDraw[  0 ] = GIF_TAG( 1, 0, 0, 0, 1, 8 );
 g_IPUCtx.m_DMAGIFDraw[  1 ] = GS_TEX0_1 | ( GS_PRIM << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 ) | ( GS_UV << 16 ) | ( GS_XYZ2 << 20 ) | ( GS_RGBAQ << 24 ) | ( GS_PRIM << 28 );
 g_IPUCtx.m_DMAGIFDraw[  2 ] = GS_SETREG_TEX0( g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, GSPSM_32, g_IPUCtx.m_TW, g_IPUCtx.m_TH, 0, 1, 0, 0, 0, 0, 0 );
 g_IPUCtx.m_DMAGIFDraw[  3 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0 );
 g_IPUCtx.m_DMAGIFDraw[  4 ] = GS_SETREG_UV( g_IPUCtx.m_TxtLeft, g_IPUCtx.m_TxtTop );
 g_IPUCtx.m_DMAGIFDraw[  5 ] = GS_SETREG_XYZ( g_IPUCtx.m_ImgLeft, g_IPUCtx.m_ImgTop, 0 );
 g_IPUCtx.m_DMAGIFDraw[  6 ] = GS_SETREG_UV( g_IPUCtx.m_TxtRight, g_IPUCtx.m_TxtBottom );
 g_IPUCtx.m_DMAGIFDraw[  7 ] = GS_SETREG_XYZ( g_IPUCtx.m_ImgRight, g_IPUCtx.m_ImgBottom, 0 );
 g_IPUCtx.m_DMAGIFDraw[  8 ] = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );
 g_IPUCtx.m_DMAGIFDraw[  9 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 g_IPUCtx.m_DMAGIFDraw[ 10 ] = GIF_TAG( 2, 1, 0, 0, 1, 2 );
 g_IPUCtx.m_DMAGIFDraw[ 11 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
 g_IPUCtx.m_DMAGIFDraw[ 12 ] = GS_SETREG_XYZ( g_IPUCtx.m_ScrLeft,  g_IPUCtx.m_ScrTop,    0 );
 g_IPUCtx.m_DMAGIFDraw[ 13 ] = GS_SETREG_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ImgTop,    0 );
 g_IPUCtx.m_DMAGIFDraw[ 14 ] = GS_SETREG_XYZ( g_IPUCtx.m_ScrLeft,  g_IPUCtx.m_ImgBottom, 0 );
 g_IPUCtx.m_DMAGIFDraw[ 15 ] = GS_SETREG_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
 SyncDCache ( g_IPUCtx.m_DMAGIFDraw, &g_IPUCtx.m_DMAGIFDraw[ 15 ] );

}  /* end IPU_Reset */

static void IPU_iQueuePacket ( int aQWC, void* apData ) {

 if ( !g_IPUCtx.m_ViFQueueSize ) return;

 g_IPUCtx.m_ViFQueueSize -= 2;
 s_pViFPacket[ g_IPUCtx.m_ViFQueueSize ] = g_IPUCtx.m_ViFQueueSize == ViF_PIDX ? DMA_TAG(  aQWC, 1, DMA_REFE, 0, ( u32 )apData, 0  ) : DMA_TAG(  aQWC, 1, DMA_REF, 0, ( u32 )apData, 0  );

}  /* end IPU_iQueuePacket */

static void IPU_QueuePacket ( int aQWC, void* apData ) {

 if ( !g_IPUCtx.m_VIFQueueSize ) return;

 DIntr ();
  g_IPUCtx.m_VIFQueueSize -= 2;
  s_pVIFPacket[ g_IPUCtx.m_VIFQueueSize ] = g_IPUCtx.m_VIFQueueSize == VIF_PIDX ? DMA_TAG(  aQWC, 1, DMA_REFE, 0, ( u32 )apData, 0  ) : DMA_TAG(  aQWC, 1, DMA_REF, 0, ( u32 )apData, 0  );
 EIntr ();

}  /* end IPU_QueuePacket */

IPUContext* IPU_InitContext ( GSContext* apGSCtx, int aWidth, int aHeight ) {

 ee_sema_t lSema;

 if ( g_IPUCtx.m_pResult == NULL ) {

  SMS_Linesize ( aWidth, &g_IPUCtx.m_MBStride );

  s_pVIFPacket = ( unsigned long int* )(  ( unsigned int  )&g_IPUCtx.m_DMAVIFDraw[ 0 ] | 0x20000000  );
  s_pViFPacket = ( unsigned long int* )(  ( unsigned int  )&g_IPUCtx.m_DMAViFDraw[ 0 ] | 0x20000000  );

  g_IPUCtx.m_nMBSlice        = ( aWidth  + 15 ) >> 4;
  g_IPUCtx.m_nMBSlices       = ( aHeight + 15 ) >> 4;
  g_IPUCtx.m_QWCToIPUSlice   = g_IPUCtx.m_nMBSlice * 24;
  g_IPUCtx.m_QWCFromIPUSlice = g_IPUCtx.m_nMBSlice * 64;
  g_IPUCtx.m_pBuffer         = NULL;
  g_IPUCtx.m_Width           = aWidth;
  g_IPUCtx.m_Height          = aHeight;
  g_IPUCtx.m_ScrWidth        = apGSCtx -> m_Width;
  g_IPUCtx.m_ScrHeight       = apGSCtx -> m_Height;
  g_IPUCtx.m_ScrLeft         = apGSCtx -> m_OffsetX << 4;
  g_IPUCtx.m_ScrTop          = apGSCtx -> m_OffsetY << 4;
  g_IPUCtx.m_ScrRight        = g_IPUCtx.m_ScrLeft + ( apGSCtx -> m_Width  << 4 );
  g_IPUCtx.m_ScrBottom       = g_IPUCtx.m_ScrTop  + ( apGSCtx -> m_Height << 3 );
  g_IPUCtx.m_pResult         = ( unsigned char* )malloc ( 1024 * g_IPUCtx.m_nMBSlice );

  if ( g_IPUCtx.m_pResult != NULL ) {

   unsigned int lTW   = GS_PowerOf2 ( aWidth  );
   unsigned int lTH   = GS_PowerOf2 ( aHeight );
   unsigned int lTBW  = ( aWidth + 63 ) >> 6;
   unsigned int lVRAM = apGSCtx -> m_VRAMPtr >> 8;

   g_IPUCtx.m_VRAM         = lVRAM;
   g_IPUCtx.m_TBW          = lTBW;
   g_IPUCtx.m_TW           = lTW;
   g_IPUCtx.m_TH           = lTH;
   g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;
   g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;

   g_IPUCtx.Destroy      = IPU_DestroyContext;
   g_IPUCtx.Display      = IPU_Display;
   g_IPUCtx.Sync         = IPU_Sync;
   g_IPUCtx.SetTEX       = IPU_SetTEX;
   g_IPUCtx.Reset        = IPU_Reset;
   g_IPUCtx.QueuePacket  = IPU_QueuePacket;
   g_IPUCtx.iQueuePacket = IPU_iQueuePacket;

   lSema.init_count  = 1;
   lSema.max_count   = 1;
   g_IPUCtx.m_SyncS = CreateSema ( &lSema );
   g_IPUCtx.SetTEX ();
   DMA_Wait ( DMA_CHANNEL_GIF );

   IPU_Reset ( apGSCtx );

   for ( lTW = 0; lTW < 8; ++lTW ) {

    SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 0 ] = DMA_TAG( 6, 0, DMA_CNT, 0, 0, 0 );
    SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 1 ] = 0;

     SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 2 ] = GIF_TAG( 4, 1, 0, 0, 0, 1 );
     SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 3 ] = GIF_AD;

      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 4 ] = GS_SETREG_BITBLTBUF( 0, 0, 0, lVRAM, lTBW, GSPSM_32 );
      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 5 ] = GS_BITBLTBUF;

      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 7 ] = GS_TRXPOS;

      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 8 ] = GS_SETREG_TRXREG( 16, 16 );
      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 9 ] = GS_TRXREG;

      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 10 ] = GS_SETREG_TRXDIR( 0 );
      SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 11 ] = GS_TRXDIR;

     SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 12 ] = GIF_TAG( 64, 1, 0, 0, 2, 1 );
     SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 13 ] = 0;

    SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 15 ] = 0;

   }  /* end for */

   SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 0 ] = DMA_TAG( 0, 0, DMA_END, 0, 0, 0 );
   SMS_IPU_SPR_PKT_BUF[ lTW * 16 + 1 ] = 0;

   g_IPUCtx.m_DMAHandlerID_IPU = AddDmacHandler ( DMA_CHANNEL_FROM_IPU, IPU_DMAHandlerFromIPU, 0 );
   g_IPUCtx.m_DMAHandlerID_GIF = AddDmacHandler ( DMA_CHANNEL_GIF,      IPU_DMAHandlerToGIF,   0 );
#ifdef VB_SYNC
   g_IPUCtx.m_VBlankStartHandlerID = AddIntcHandler ( 2, IPU_VBlankStartHandler, 0 );
   g_IPUCtx.m_VBlankEndHandlerID   = AddIntcHandler ( 3, IPU_VBlankEndHandler,   0 );
   EnableIntc ( 2 );
   EnableIntc ( 3 );
#endif  /* VB_SYNC */
   IPU_RESET();
   IPU_CMD_SETTH( 0, 0 );
   IPU_WAIT();
   IPU_RESET();

   EnableDmac ( DMA_CHANNEL_FROM_IPU );
   EnableDmac ( DMA_CHANNEL_GIF      );

  }  /* end if */

 }  /* end if */

 return &g_IPUCtx;

}  /* end IPU_InitContext */
#endif  /* _WIN32 */
