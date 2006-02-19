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
#include "SMS_IPU.h"
#include "SMS_GS.h"
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
# include <kernel.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include "SMS_INTC.h"
# include "SMS_DMA.h"
# include "SMS_Config.h"

# define VIF_PIDX (  sizeof ( g_IPUCtx.m_DMAVIFDraw ) / sizeof ( g_IPUCtx.m_DMAVIFDraw[ 0 ] ) - 2  )
# define ViF_PIDX (  sizeof ( g_IPUCtx.m_DMAViFDraw ) / sizeof ( g_IPUCtx.m_DMAViFDraw[ 0 ] ) - 2  )
# define VIP_PIDX (  sizeof ( g_IPUCtx.m_DMAVIPDraw ) / sizeof ( g_IPUCtx.m_DMAVIPDraw[ 0 ] ) - 2  )

static unsigned long int* s_pVIFPacket;
static unsigned long int* s_pViFPacket;
static unsigned long int* s_pVIPPacket;

static void IPU_DestroyContext ( void ) {

 DisableDmac ( DMAC_I_GIF );
 RemoveDmacHandler ( DMAC_I_GIF, g_IPUCtx.m_DMAHandlerID_GIF );
#ifdef VB_SYNC
 DisableIntc ( INTC_VB_ON  );
 DisableIntc ( INTC_VB_OFF );
 RemoveIntcHandler ( INTC_VB_ON,  g_IPUCtx.m_VBlankStartHandlerID );
 RemoveIntcHandler ( INTC_VB_OFF, g_IPUCtx.m_VBlankEndHandlerID   );
#endif  /* VB_SYNC */
 DeleteSema ( g_IPUCtx.m_SyncS );

 if ( g_IPUCtx.m_pResult != NULL ) {

  DisableDmac ( DMAC_I_FROM_IPU );
  RemoveDmacHandler ( DMAC_I_FROM_IPU, g_IPUCtx.m_DMAHandlerID_IPU );

  free ( g_IPUCtx.m_pDMAPacket );
  free ( g_IPUCtx.m_pResult    );
  g_IPUCtx.m_pResult = NULL;

 }  /* end if */

}  /* end IPU_DestroyContext */

static void IPU_Sync ( void ) {

 WaitSema ( g_IPUCtx.m_SyncS );
 SignalSema ( g_IPUCtx.m_SyncS );

}  /* end IPU_Sync */

static void IPU_Flush ( void ) {

 if ( g_IPUCtx.m_VIFQueueSize != VIF_PIDX + 2 ) {

  DMA_SendChainA ( DMAC_VIF1, &g_IPUCtx.m_DMAVIFDraw[ g_IPUCtx.m_VIFQueueSize ] );
  g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;

 } else if ( g_IPUCtx.m_ViFQueueSize != ViF_PIDX + 2 ) {

  DMA_SendChainA ( DMAC_VIF1, &g_IPUCtx.m_DMAViFDraw[ g_IPUCtx.m_ViFQueueSize ] );
  g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;

 }  /* end if */

}  /* end IPU_Flush */

static void IPU_SetTEX ( void ) {

 unsigned long* lpDMA = UNCACHED_SEG( g_IPUCtx.m_DMAGIFTX );

 lpDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lpDMA[ 1 ] = GIFTAG_REGS_AD;
 lpDMA[ 2 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 lpDMA[ 3 ] = GS_TEX1_2;
 DMA_Send ( DMAC_GIF, g_IPUCtx.m_DMAGIFTX, 2 );

}  /* end IPU_SetTEX */

static int IPU_DMAHandlerFromIPU ( int aChan ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set noat\n\t"
  "lui      $a2, 0x1000\n\t"
  "lw       $v0, %2\n\t"
  "ori      $a2, 0xA000\n\t"
  "ld       $a0, %0\n\t"
  "lw       $a1, %1\n\t"
  "li       $t0, 0x8000\n\t"
  "lui      $v1, 0x2000\n\t"
  "sw       $zero, 32($a2)\n\t"
  "sw       $v0, 48($a2)\n\t"
  "or       $v0, $v0, $v1\n\t"
  "dsll     $t0, $t0, 21\n\t"
  "addiu    $v0, $v0, 48\n\t"
  "1:\n\t"
  "subu     $a1, $a1, 1\n\t"
  "sd       $a0, 0($v0)\n\t"
  "daddu    $a0, $a0, $t0\n\t"
  "addu     $v0, $v0, 128\n\t"
  "nop\n\t"
  "bgtz     $a1, 1b\n\t"
  "li       $at, 0x0105\n\t"
  "sw       $v0, %3\n\t"
  "sw       $at, 0($a2)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  :: "m"( g_IPUCtx.m_DestY      ),
     "m"( g_IPUCtx.m_nMBSlice   ),
     "m"( g_IPUCtx.m_pDMAPacket ),
     "m"( g_IPUCtx.m_GIFlag     )
 );

 return 0;

}  /* end IPU_DMAHandlerFromIPU */

static void IPU_GIFHandlerDraw ( void ) {

 if ( g_IPUCtx.m_VIPQueueSize != VIP_PIDX + 2 ) {

  DMA_SendChainA ( DMAC_VIF1, &g_IPUCtx.m_DMAVIPDraw[ g_IPUCtx.m_VIPQueueSize ] );
  g_IPUCtx.m_VIPQueueSize = VIP_PIDX + 2;

 }  /* end if */

 iSignalSema ( g_IPUCtx.m_SyncS );

}  /* end IPU_GIFHandlerDraw */
#ifdef VB_SYNC
static int IPU_VBlankStartHandler ( int aCause ) {

 if ( g_IPUCtx.m_fDraw ) {

  IPU_Flush ();

  g_IPUCtx.m_fDraw    = 0;
  g_IPUCtx.m_GIFlag   = 1;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
  DMA_SendA ( DMAC_GIF, g_IPUCtx.m_DMAGIFDraw, 10 );

 } else g_IPUCtx.m_fBlank = 1;

 return -1;

}  /* end IPU_VBlankStartHandler */

static int IPU_VBlankEndHandler ( int aCause ) {

 g_IPUCtx.m_fBlank = 0;

 return -1;

}  /* end IPU_VBlankEndHandler */
#endif  /* VB_SYNC */
static void IPU_GIFHandlerSend ( void ) {

 if ( !--g_IPUCtx.m_Slice ) {
#ifdef VB_SYNC
  if ( !g_IPUCtx.m_fBlank )

   g_IPUCtx.m_fDraw = 1;

  else {
#endif  /* VB_SYNC */
   IPU_Flush ();

   g_IPUCtx.m_GIFlag   = 1;
   g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
   DMA_SendA ( DMAC_GIF, g_IPUCtx.m_DMAGIFDraw, 10 );
#ifdef VB_SYNC
  }  /* end else */
#endif  /* VB_SYNC */
  return;

 }  /* end if */

 g_IPUCtx.m_DestY += 0x0010000000000000L;
 g_IPUCtx.m_pMB   += g_IPUCtx.m_MBStride;

 DMA_SendA ( DMAC_TO_IPU, g_IPUCtx.m_pMB, g_IPUCtx.m_QWCToIPUSlice );
 IPU_CMD_CSC( g_IPUCtx.m_nMBSlice, 0, 0 );
 DMA_RecvA ( DMAC_FROM_IPU, g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

}  /* end IPU_GIFHandlerSend */

static int IPU_DMAHandlerToGIF ( int aChan ) {

 if ( g_IPUCtx.m_GIFlag ) {

  g_IPUCtx.m_GIFlag = 0;
  g_IPUCtx.GIFHandler ();

 }  /* end if */

 return 0;

}  /* end IPU_DMAHandlerToGIF */

static void IPU_Display ( void* apFB ) {

 WaitSema ( g_IPUCtx.m_SyncS );

 g_IPUCtx.m_DestY    = 0;
 g_IPUCtx.m_Slice    = g_IPUCtx.m_nMBSlices;
 g_IPUCtx.m_pMB      = (  ( SMS_FrameBuffer* )apFB  ) -> m_pData;
 g_IPUCtx.m_GIFlag   = 0;
#ifdef VB_SYNC
 g_IPUCtx.m_fDraw    = 0;
#endif  /* VB_SYNC */
 g_IPUCtx.GIFHandler = IPU_GIFHandlerSend;

 DMA_SendA (   DMAC_TO_IPU, (  ( SMS_FrameBuffer* )apFB  ) -> m_pData, g_IPUCtx.m_QWCToIPUSlice   );
 IPU_CMD_CSC( g_IPUCtx.m_nMBSlice, 0, 0 );
 DMA_RecvA ( DMAC_FROM_IPU, g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

}  /* end IPU_Display */

static void IPU_ChangeMode ( unsigned int anIdx ) {

 if ( anIdx < 5 ) {

  g_IPUCtx.m_ModeIdx = anIdx;

  DIntr ();
   g_IPUCtx.m_DMAGIFDraw[  0 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
   g_IPUCtx.m_DMAGIFDraw[  1 ] = GIFTAG_REGS_AD;
   g_IPUCtx.m_DMAGIFDraw[  2 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
   g_IPUCtx.m_DMAGIFDraw[  3 ] = GS_TEX1_2;
   g_IPUCtx.m_DMAGIFDraw[  4 ] = GIF_TAG( 1, 0, 0, 0, 1, 8 );
   g_IPUCtx.m_DMAGIFDraw[  5 ] = GS_TEX0_2 | ( GS_PRIM << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 ) | ( GS_UV << 16 ) | ( GS_XYZ2 << 20 ) | ( GS_RGBAQ << 24 ) | ( GS_PRIM << 28 );
   g_IPUCtx.m_DMAGIFDraw[  6 ] = GS_SET_TEX0( g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, 0, g_IPUCtx.m_TW, g_IPUCtx.m_TH, 0, 1, 0, 0, 0, 0, 0 );
   g_IPUCtx.m_DMAGIFDraw[  7 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 1, 0 );
   g_IPUCtx.m_DMAGIFDraw[  8 ] = GS_SET_UV( g_IPUCtx.m_TxtLeft  [ anIdx ], g_IPUCtx.m_TxtTop   [ anIdx ] );
   g_IPUCtx.m_DMAGIFDraw[  9 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], g_IPUCtx.m_ImgTop   [ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 10 ] = GS_SET_UV( g_IPUCtx.m_TxtRight [ anIdx ], g_IPUCtx.m_TxtBottom[ anIdx ] );
   g_IPUCtx.m_DMAGIFDraw[ 11 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], g_IPUCtx.m_ImgBottom[ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 12 ] = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 );
   g_IPUCtx.m_DMAGIFDraw[ 13 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 1, 0 );
   g_IPUCtx.m_DMAGIFDraw[ 14 ] = GIF_TAG( 2, 1, 0, 0, 1, 2 );
   g_IPUCtx.m_DMAGIFDraw[ 15 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
   g_IPUCtx.m_DMAGIFDraw[ 16 ] = GS_SET_XYZ( 0, 0, 0 );
   g_IPUCtx.m_DMAGIFDraw[ 17 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ImgTop[ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 18 ] = GS_SET_XYZ( 0,  g_IPUCtx.m_ImgBottom[ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 19 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
   SyncDCache ( g_IPUCtx.m_DMAGIFDraw, &g_IPUCtx.m_DMAGIFDraw[ 20 ] );
  EIntr ();

 }  /* end if */

}  /* end IPU_ChangeMode */

static void _ipu_compute_fields ( unsigned int anIdx, unsigned int aWidth ) {

 float lAR    = ( float )aWidth / ( float )g_IPUCtx.m_Height;
 int   lDelta = ( g_IPUCtx.m_Width - aWidth ) / 2;

 g_IPUCtx.m_TxtLeft  [ anIdx ] = ( lDelta << 4 ) + 8;
 g_IPUCtx.m_TxtTop   [ anIdx ] = 8;
 g_IPUCtx.m_TxtRight [ anIdx ] = ( aWidth << 4 ) + g_IPUCtx.m_TxtLeft[ anIdx ] - 16;
 g_IPUCtx.m_TxtBottom[ anIdx ] = ( g_IPUCtx.m_Height << 4 ) - 8;

 if ( g_GSCtx.m_Width < g_GSCtx.m_Height * lAR ) {

  int lH      = ( int )( g_GSCtx.m_Width / lAR );
  int lTop    = ( g_GSCtx.m_Height - lH ) >> 1;
  int lBottom = lTop + lH;

  __asm__ __volatile__(
   ".set noreorder\n\t"
   "move    $t9, $ra\n\t"
   "dsll32  %3, %3, 0\n\t"
   "move    $a0, $zero\n\t"
   "move    $a1, %2\n\t"
   "move    $a2, $zero\n\t"
   "jal     GS_XYZ\n\t"
   "or      $a1, $a1, %3\n\t"
   "srl     %0, $v0, 16\n\t"
   "dsrl32  %1, $a1, 16\n\t"
   "move    $ra, $t9\n\t"
   ".set reorder\n\t"
   : "=r"( lTop ), "=r"( lBottom )
   :  "r"( lTop ),  "r"( lBottom )
   : "a0", "a1", "a2", "v0", "v1", "t9"
  );

  g_IPUCtx.m_ImgLeft  [ anIdx ] = 0;
  g_IPUCtx.m_ImgRight [ anIdx ] = g_GSCtx.m_Width << 4;
  g_IPUCtx.m_ImgTop   [ anIdx ] = lTop;
  g_IPUCtx.m_ImgBottom[ anIdx ] = lBottom;

 } else {

  int lW = ( int )( g_GSCtx.m_Height * lAR );

  g_IPUCtx.m_ImgTop   [ anIdx ] = 0;
  g_IPUCtx.m_ImgBottom[ anIdx ] = g_IPUCtx.m_ScrBottom;
  g_IPUCtx.m_ImgLeft  [ anIdx ] = (  ( g_GSCtx.m_Width - lW ) >> 1  ) << 4;
  g_IPUCtx.m_ImgRight [ anIdx ] = g_IPUCtx.m_ImgLeft[ anIdx ] + ( lW << 4 );

 }  /* end else */

}  /* end _ipu_compute_fields */

static void IPU_Pan ( int aDir ) {

 int lDelta = 64 << 4;

 if ( aDir > 0 ) {

  int lRight = g_IPUCtx.m_TxtRight[ g_IPUCtx.m_ModeIdx ] + lDelta;

  if ( lRight > g_IPUCtx.m_TxtRight[ 0 ] ) lDelta -= lRight - ( int )g_IPUCtx.m_TxtRight[ 0 ];

 } else {

  int lLeft = ( int )g_IPUCtx.m_TxtLeft[ g_IPUCtx.m_ModeIdx ] - lDelta;

  if ( lLeft < ( int )g_IPUCtx.m_TxtLeft[ 0 ] ) lDelta -= ( int )g_IPUCtx.m_TxtLeft[ 0 ] - lLeft;

  lDelta = -lDelta;

 }  /* end else */

 if ( lDelta ) {

  g_IPUCtx.m_TxtLeft [ g_IPUCtx.m_ModeIdx ] += lDelta;
  g_IPUCtx.m_TxtRight[ g_IPUCtx.m_ModeIdx ] += lDelta;

  IPU_ChangeMode ( g_IPUCtx.m_ModeIdx );

 }  /* end if */

}  /* end IPU_Pan */

static void IPU_Reset ( void ) {

 float lAR    = (  ( float )g_GSCtx.m_Width  ) / (  ( float )g_GSCtx.m_Height  );
 int   lWP    = ( int )(  ( float )g_IPUCtx.m_Height * lAR  );
 int   lDelta = (  ( int )g_IPUCtx.m_Width - lWP  ) / 3;

 _ipu_compute_fields ( 0, g_IPUCtx.m_Width );  /* letterbox  */

 if ( lDelta > 0 ) {

  _ipu_compute_fields ( 1, g_IPUCtx.m_Width - lDelta          );  /* pan-scan 1 */
  _ipu_compute_fields ( 2, g_IPUCtx.m_Width - lDelta - lDelta );  /* pan-scan 2 */
  _ipu_compute_fields ( 3, lWP                                );  /* pan-scan 3 */

 } else {

 _ipu_compute_fields ( 1, g_IPUCtx.m_Width );  /* pan-scan 1 */
 _ipu_compute_fields ( 2, g_IPUCtx.m_Width );  /* pan-scan 2 */
 _ipu_compute_fields ( 3, g_IPUCtx.m_Width );  /* pan-scan 3 */

 }  /* end else */

 g_IPUCtx.m_TxtLeft  [ 4 ] = 8;
 g_IPUCtx.m_TxtTop   [ 4 ] = 8;
 g_IPUCtx.m_TxtRight [ 4 ] = ( g_IPUCtx.m_Width  << 4 ) - 8;
 g_IPUCtx.m_TxtBottom[ 4 ] = ( g_IPUCtx.m_Height << 4 ) - 8;

 g_IPUCtx.m_ImgLeft  [ 4 ] = 0;
 g_IPUCtx.m_ImgTop   [ 4 ] = 0;
 g_IPUCtx.m_ImgRight [ 4 ] = g_GSCtx.m_PWidth  << 4;
 g_IPUCtx.m_ImgBottom[ 4 ] = g_GSCtx.m_PHeight << 4;

 IPU_ChangeMode ( g_Config.m_PlayerFlags >> 28 );

}  /* end IPU_Reset */

static void IPU_iQueuePacket ( int aQWC, void* apData ) {

 if ( !g_IPUCtx.m_ViFQueueSize ) return;

 g_IPUCtx.m_ViFQueueSize -= 2;
 s_pViFPacket[ g_IPUCtx.m_ViFQueueSize ] = g_IPUCtx.m_ViFQueueSize == ViF_PIDX ? DMA_TAG(  aQWC, 1, DMATAG_ID_REFE, 0, ( u32 )apData, 0  ) : DMA_TAG(  aQWC, 1, DMATAG_ID_REF, 0, ( u32 )apData, 0  );

}  /* end IPU_iQueuePacket */

static void IPU_QueuePacket ( int aQWC, void* apData ) {

 if ( !g_IPUCtx.m_VIFQueueSize ) return;

 DIntr ();
  g_IPUCtx.m_VIFQueueSize -= 2;
  s_pVIFPacket[ g_IPUCtx.m_VIFQueueSize ] = g_IPUCtx.m_VIFQueueSize == VIF_PIDX ? DMA_TAG(  aQWC, 1, DMATAG_ID_REFE, 0, ( u32 )apData, 0  ) : DMA_TAG(  aQWC, 1, DMATAG_ID_REF, 0, ( u32 )apData, 0  );
 EIntr ();

}  /* end IPU_QueuePacket */

static void IPU_PQueuePacket ( int aQWC, void* apData ) {

 if ( !g_IPUCtx.m_VIPQueueSize ) return;

 g_IPUCtx.m_VIPQueueSize -= 2;
 s_pVIPPacket[ g_IPUCtx.m_VIPQueueSize ] = g_IPUCtx.m_VIPQueueSize == VIP_PIDX ? DMA_TAG(  aQWC, 1, DMATAG_ID_REFE, 0, ( u32 )apData, 0  ) : DMA_TAG(  aQWC, 1, DMATAG_ID_REF, 0, ( u32 )apData, 0  );

}  /* end IPU_PQueuePacket */

static void IPU_Suspend ( void ) {

 DisableDmac ( DMAC_I_GIF      );
 DisableDmac ( DMAC_I_FROM_IPU );
#ifdef VB_SYNC
 DisableIntc ( INTC_VB_ON  );
 DisableIntc ( INTC_VB_OFF );
#endif  /* VB_SYNC */
}  /* end IPU_Suspend */

static void IPU_Resume ( void ) {

 EnableDmac ( DMAC_I_FROM_IPU );
 EnableDmac ( DMAC_I_GIF      );
#ifdef VB_SYNC
 EnableIntc ( INTC_VB_ON  );
 EnableIntc ( INTC_VB_OFF );
#endif  /* VB_SYNC */
}  /* end IPU_Resume */

static void IPU_Repaint ( void ) {

 DMA_Send ( DMAC_GIF, g_IPUCtx.m_DMAGIFDraw, 10 );

}  /* end IPU_Repaint */

static void IPU_DummySuspend ( void ) {

 DisableDmac ( DMAC_I_GIF );

}  /* end IPU_Suspend */

static void IPU_DummyResume ( void ) {

 EnableDmac ( DMAC_I_GIF );

}  /* end IPU_DummyResume */

static void IPU_DummyDisplay ( void* apParam ) {

 WaitSema ( g_IPUCtx.m_SyncS );

 g_IPUCtx.m_pDMAPacket = ( unsigned long* )apParam;
 g_IPUCtx.m_fDraw = 1;

}  /* end IPU_DummyDisplay */

static void IPU_Dummy ( void ) {

}  /* end IPU_Dummy */

static void IPU_DummyChangeMode ( unsigned int aMode ) {

}  /* end IPU_DummyChangeMode */

static void IPU_DummyPan ( int aDir ) {

}  /* end IPU_DummyPan */

static void IPU_DummyRepaint ( void ) {

 DMA_SendChain ( DMAC_GIF, g_IPUCtx.m_pDMAPacket );

}  /* end IPU_DummyRepaint */
#ifdef VB_SYNC
static int IPU_DummyVBlankStartHandler ( int aCause ) {

 if ( g_IPUCtx.m_fDraw ) {

  IPU_Flush ();

  g_IPUCtx.m_fDraw  = 0;
  g_IPUCtx.m_GIFlag = 1;
  DMA_SendChainA ( DMAC_GIF, g_IPUCtx.m_pDMAPacket );

 } else g_IPUCtx.m_fBlank = 1;

 return -1;

}  /* end IPU_DummyVBlankStartHandler */
#endif  /* VB_SYNC */
IPUContext* IPU_InitContext ( int aWidth, int aHeight ) {

 ee_sema_t lSema;

 lSema.init_count = 1;
 lSema.max_count  = 1;
 g_IPUCtx.m_SyncS = CreateSema ( &lSema );

 s_pVIFPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAVIFDraw[ 0 ] );
 s_pViFPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAViFDraw[ 0 ] );
 s_pVIPPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAVIPDraw[ 0 ] );

 g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;
 g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;
 g_IPUCtx.m_VIPQueueSize = VIP_PIDX + 2;

 g_IPUCtx.Destroy      = IPU_DestroyContext;
 g_IPUCtx.QueuePacket  = IPU_QueuePacket;
 g_IPUCtx.iQueuePacket = IPU_iQueuePacket;
 g_IPUCtx.PQueuePacket = IPU_PQueuePacket;
 g_IPUCtx.Sync         = IPU_Sync;
 g_IPUCtx.Flush        = IPU_Flush;

 if ( aWidth && aHeight ) {

  unsigned int lTW   = GS_PowerOf2 ( aWidth  );
  unsigned int lTH   = GS_PowerOf2 ( aHeight );
  unsigned int lTBW  = ( aWidth + 63 ) >> 6;
  unsigned int lVRAM = g_GSCtx.m_VRAMPtr;
  uint8_t*     lpRes;
  uint64_t*    lpBuf;

  SMS_Linesize ( aWidth, &g_IPUCtx.m_MBStride );

  g_IPUCtx.m_nMBSlice        = ( aWidth  + 15 ) >> 4;
  g_IPUCtx.m_nMBSlices       = ( aHeight + 15 ) >> 4;
  g_IPUCtx.m_QWCToIPUSlice   = g_IPUCtx.m_nMBSlice * 24;
  g_IPUCtx.m_QWCFromIPUSlice = g_IPUCtx.m_nMBSlice * 64;
  g_IPUCtx.m_Width           = aWidth;
  g_IPUCtx.m_Height          = aHeight;
  g_IPUCtx.m_ScrRight        = g_GSCtx.m_PWidth  << 4;
  g_IPUCtx.m_ScrBottom       = g_GSCtx.m_PHeight << 4;
  g_IPUCtx.m_pResult         = ( unsigned char* )malloc ( 1024 * g_IPUCtx.m_nMBSlice );

  g_IPUCtx.m_pDMAPacket = ( uint64_t* )malloc ( g_IPUCtx.m_nMBSlice * 16 * 8 );
  FlushCache ( 0 );

  lpRes = g_IPUCtx.m_pResult;
  lpBuf = ( uint64_t* )UNCACHED_SEG( g_IPUCtx.m_pDMAPacket );

  g_IPUCtx.m_VRAM    = lVRAM;
  g_IPUCtx.m_TBW     = lTBW;
  g_IPUCtx.m_TW      = lTW;
  g_IPUCtx.m_TH      = lTH;
  g_IPUCtx.m_ModeIdx = 0;

  g_IPUCtx.Display    = IPU_Display;
  g_IPUCtx.SetTEX     = IPU_SetTEX;
  g_IPUCtx.Reset      = IPU_Reset;
  g_IPUCtx.Suspend    = IPU_Suspend;
  g_IPUCtx.Resume     = IPU_Resume;
  g_IPUCtx.ChangeMode = IPU_ChangeMode;
  g_IPUCtx.Pan        = IPU_Pan;
  g_IPUCtx.Repaint    = IPU_Repaint;

  g_IPUCtx.SetTEX ();
  DMA_Wait ( DMAC_GIF );

  IPU_Reset ();

  for ( lTW = 0; lTW < g_IPUCtx.m_nMBSlice; ++lTW, lpRes += 1024, lpBuf += 16 ) {

   lpBuf[ 0 ] = DMA_TAG( 6, 0, DMATAG_ID_CNT, 0, 0, 0 );
   lpBuf[ 1 ] = 0;

    lpBuf[ 2 ] = GIF_TAG( 4, 1, 0, 0, 0, 1 );
    lpBuf[ 3 ] = GIFTAG_REGS_AD;

     lpBuf[ 4 ] = GS_SET_BITBLTBUF( 0, 0, 0, lVRAM, lTBW, 0 );
     lpBuf[ 5 ] = GS_BITBLTBUF;

     lpBuf[ 7 ] = GS_TRXPOS;

     lpBuf[ 8 ] = GS_SET_TRXREG( 16, 16 );
     lpBuf[ 9 ] = GS_TRXREG;

     lpBuf[ 10 ] = GS_SET_TRXDIR( 0 );
     lpBuf[ 11 ] = GS_TRXDIR;

    lpBuf[ 12 ] = GIF_TAG( 64, 1, 0, 0, 2, 1 );
    lpBuf[ 13 ] = 0;

   lpBuf[ 14 ] = DMA_TAG( 64, 1, DMATAG_ID_REF, 0, ( u32 )lpRes, 0  );
   lpBuf[ 15 ] = 0;

  }  /* end for */

  lpBuf[ -2 ] = DMA_TAG(  64, 1, DMATAG_ID_REFE, 0, ( u32 )( lpRes - 1024 ), 0  );
  lpBuf[ -1 ] = 0;

  IPU_RESET();
  IPU_CMD_SETTH( 0, 0 );
  IPU_WAIT();

  g_IPUCtx.m_DMAHandlerID_IPU = AddDmacHandler ( DMAC_I_FROM_IPU, IPU_DMAHandlerFromIPU, 0 );
  EnableDmac ( DMAC_I_FROM_IPU );
#ifdef VB_SYNC
  g_IPUCtx.m_VBlankStartHandlerID = AddIntcHandler ( 2, IPU_VBlankStartHandler, 0 );
  g_IPUCtx.m_VBlankEndHandlerID   = AddIntcHandler ( 3, IPU_VBlankEndHandler,   0 );
#endif  /* VB_SYNC */
 } else {

  g_IPUCtx.m_pResult = NULL;

  g_IPUCtx.Display    = IPU_DummyDisplay;
  g_IPUCtx.SetTEX     = IPU_Dummy;
  g_IPUCtx.Reset      = IPU_Dummy;
  g_IPUCtx.Suspend    = IPU_DummySuspend;
  g_IPUCtx.Resume     = IPU_DummyResume;
  g_IPUCtx.ChangeMode = IPU_DummyChangeMode;
  g_IPUCtx.Pan        = IPU_DummyPan;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
  g_IPUCtx.Repaint    = IPU_DummyRepaint;
#ifdef VB_SYNC
  g_IPUCtx.m_VBlankStartHandlerID = AddIntcHandler ( INTC_VB_ON,  IPU_DummyVBlankStartHandler, 0 );
  g_IPUCtx.m_VBlankEndHandlerID   = AddIntcHandler ( INTC_VB_OFF, IPU_VBlankEndHandler,        0 );
#endif  /* VB_SYNC */
 }  /* end else */

 g_IPUCtx.m_DMAHandlerID_GIF = AddDmacHandler ( DMAC_I_GIF, IPU_DMAHandlerToGIF, 0 );
 EnableDmac ( DMAC_I_GIF );
#ifdef VB_SYNC
 EnableIntc ( INTC_VB_ON  );
 EnableIntc ( INTC_VB_OFF );
#endif  /* VB_SYNC */
 return &g_IPUCtx;

}  /* end IPU_InitContext */
#endif  /* _WIN32 */
