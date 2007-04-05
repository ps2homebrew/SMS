/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2006 hjx (widescreen support)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include <kernel.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SMS_INTC.h"
#include "SMS_DMA.h"
#include "SMS_Config.h"
#include "SMS_IPU.h"
#include "SMS_GS.h"
#include "SMS.h"
#include "SMS_VideoBuffer.h"
#include "SMS_Timer.h"
#include "SMS_DXSB.h"

IPUContext g_IPUCtx;

# define VIF_PIDX (  sizeof ( g_IPUCtx.m_DMAVIFDraw ) / sizeof ( g_IPUCtx.m_DMAVIFDraw[ 0 ] ) - 2  )
# define ViF_PIDX (  sizeof ( g_IPUCtx.m_DMAViFDraw ) / sizeof ( g_IPUCtx.m_DMAViFDraw[ 0 ] ) - 2  )
# define VIP_PIDX (  sizeof ( g_IPUCtx.m_DMAVIPDraw ) / sizeof ( g_IPUCtx.m_DMAVIPDraw[ 0 ] ) - 2  )

static unsigned long int* s_pVIFPacket;
static unsigned long int* s_pViFPacket;
static unsigned long int* s_pVIPPacket;

static SMS_DXSBFrame* s_pDXSBDFrm;
static SMS_DXSBFrame* s_pDXSBAFrm;
static SMS_DXSBFrame* s_pDXSBRFrm;
static int            s_SyncS;
static int            s_fStopSync;

static SMS_DXSBErasePack s_DXSBEPack = {
 .m_GIFTag.m_HiLo.m_Lo = GIF_TAG( 6, 1, 0, 0, GIFTAG_FLG_PACKED, 1 ),
 .m_GIFTag.m_HiLo.m_Hi = GIFTAG_REGS_AD,
 .m_TestOff.m_Value    = GS_SET_TEST(
                          GS_TEST_ATE_OFF,
                          GS_TEST_ATST_NEVER,
                          0,
                          GS_TEST_AFAIL_KEEP,
                          GS_TEST_DATE_OFF,
                          GS_TEST_DATM_0PASS,
                          GS_TEST_ZTE_ON,
                          GS_TEST_ZTST_ALWAYS
                         ),
 .m_TestOffID          = GS_TEST_1,
 .m_Prim.m_Value       = GS_SET_PRIM(
                          GS_PRIM_PRIM_SPRITE,
                          GS_PRIM_IIP_FLAT,
                          GS_PRIM_TME_OFF,
                          GS_PRIM_FGE_OFF,
                          GS_PRIM_ABE_OFF,
                          GS_PRIM_AA1_OFF,
                          GS_PRIM_FST_UV,
                          GS_PRIM_CTXT_1,
                          GS_PRIM_FIX_UNFIXED
                         ),
 .m_PrimID             = GS_PRIM,
 .m_RGBAQ.m_Value      = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 ),
 .m_RGBAQID            = GS_RGBAQ,
 .m_XYZLeftTopID       = GS_XYZ2,
 .m_XYZRightBottomID   = GS_XYZ2,
 .m_TestOn.m_Value     = GS_SET_TEST(
                          GS_TEST_ATE_OFF,
                          GS_TEST_ATST_NEVER,
                          0,
                          GS_TEST_AFAIL_KEEP,
                          GS_TEST_DATE_OFF,
                          GS_TEST_DATM_0PASS,
                          GS_TEST_ZTE_ON,
                          GS_TEST_ZTST_GEQUAL
                         ),
 .m_TestOnID           = GS_TEST_1
};

static SMS_DXSBDrawPack s_DXSBDPack = {
 .m_DMATagUpload.m_Value     = DMA_TAG( 6, 0, DMATAG_ID_CNT, 0, 0, 0 ),
 .m_Pad0                     = 0,
 .m_GIFTagUpload.m_HiLo.m_Lo = GIF_TAG( 4, 0, 0, 0, GIFTAG_FLG_PACKED, 1 ),
 .m_GIFTagUpload.m_HiLo.m_Hi = GIFTAG_REGS_AD,
 .m_BitBlt.m_Value           = GS_SET_BITBLTBUF( 0, 0, 0, 0, 0, GSPixelFormat_PSMT8H ),
 .m_BitBltID                 = GS_BITBLTBUF,
 .m_TrxPos.m_Value           = GS_SET_TRXPOS( 0, 0, 0, 0, GS_TRXPOS_DIR_LR_UD ),
 .m_TrxPosID                 = GS_TRXPOS,
 .m_TrxRegID                 = GS_TRXREG,
 .m_TrxDir.m_Value           = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL ),
 .m_TrxDirID                 = GS_TRXDIR,
 .m_GIFTagData.m_HiLo.m_Lo   = GIF_TAG( 0, 0, 0, 0, GIFTAG_FLG_IMAGE, 0 ),
 .m_GIFTagData.m_HiLo.m_Hi   = 0,
 .m_DMATagData.m_Value       = DMA_TAG( 0, 0, DMATAG_ID_REF, 0, 0, 0 ),
 .m_Pad1                     = 0,
 .m_DMATagDraw.m_Value       = DMA_TAG( 10, 0, DMATAG_ID_RET, 0, 0, 0 ),
 .m_Pad2                     = 0,
 .m_GIFTagDraw0.m_HiLo.m_Lo  = GIF_TAG( 3, 0, 0, 0, GIFTAG_FLG_PACKED, 1 ),
 .m_GIFTagDraw0.m_HiLo.m_Hi  = GIFTAG_REGS_AD,
 .m_TexFlush.m_Value         = GS_SET_TEXFLUSH( 0 ),
 .m_TexFlushID               = GS_TEXFLUSH,
 .m_Prim.m_Value             = GS_SET_PRIM(
                                GS_PRIM_PRIM_SPRITE,
                                GS_PRIM_IIP_FLAT,
                                GS_PRIM_TME_ON,
                                GS_PRIM_FGE_OFF,
                                GS_PRIM_ABE_OFF,
                                GS_PRIM_AA1_OFF,
                                GS_PRIM_FST_UV,
                                GS_PRIM_CTXT_1,
                                GS_PRIM_FIX_UNFIXED
                               ),
 .m_PrimID                   = GS_PRIM,
 .m_TestOn.m_Value           = GS_SET_TEST(
                                GS_TEST_ATE_ON,
                                GS_TEST_ATST_NOTEQUAL,
                                0,
                                GS_TEST_AFAIL_KEEP,
                                GS_TEST_DATE_OFF,
                                GS_TEST_DATM_0PASS,
                                GS_TEST_ZTE_ON,
                                GS_TEST_ZTST_GEQUAL
                               ),
 .m_TestOnID                 = GS_TEST_1,
 .m_GIFTagDraw1.m_HiLo.m_Lo  = GIF_TAG( 1, 0, 0, 0, GIFTAG_FLG_REGLIST, 6 ),
 .m_GIFTagDraw1.m_HiLo.m_Hi  = ( GIFTAG_REGS_RGBAQ  <<  0 ) |
                               ( GIFTAG_REGS_TEX0_1 <<  4 ) |
                               ( GIFTAG_REGS_UV     <<  8 ) |
                               ( GIFTAG_REGS_XYZ2   << 12 ) |
                               ( GIFTAG_REGS_UV     << 16 ) |
                               ( GIFTAG_REGS_XYZ2   << 20 ),
 .m_RGBAQ.m_Value            = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x80, 0x00 ),
 .m_UVLeftTop.m_Value        = GS_SET_UV( 8, 8 ),
 .m_GIFTagDraw2.m_HiLo.m_Lo  = GIF_TAG( 1, 1, 0, 0, GIFTAG_FLG_PACKED, 1 ),
 .m_GIFTagDraw2.m_HiLo.m_Hi  = GIFTAG_REGS_AD,
 .m_TestOff.m_Value          = GS_SET_TEST(
                                GS_TEST_ATE_OFF,
                                GS_TEST_ATST_NEVER,
                                0,
                                GS_TEST_AFAIL_KEEP,
                                GS_TEST_DATE_OFF,
                                GS_TEST_DATM_0PASS,
                                GS_TEST_ZTE_ON,
                                GS_TEST_ZTST_GEQUAL
                               ),
 .m_TestOffID                = GS_TEST_1
};

extern void IPU_Sync     ( void );
extern void IPU_StopSync ( int  );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "IPU_Sync:\n\t"
 "pcpyld    $s0, $s0, $s0\n\t"
 "lw        $s0, s_SyncS\n\t"
 "pcpyld    $s1, $s1, $s1\n\t"
 "lw        $s1, s_pDXSBDFrm\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "jal       WaitSema\n\t"
 "addiu     $a0, $s0, 0\n\t"
 "beql      $s1, $zero, 1f\n\t"
 "addiu     $a0, $s0, 0\n\t"
 "jal       free\n\t"
 "addiu     $a0, $s1, 0\n\t"
 "sw        $zero, s_pDXSBDFrm\n\t"
 "addiu     $a0, $s0, 0\n\t"
 "1:\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 "pcpyud    $s1, $s1, $s1\n\t"
 "j         SignalSema\n\t"
 "pcpyud    $s0, $s0, $s0\n\t"
 "IPU_StopSync:\n\t"
 "jr        $ra\n\t"
 "sw        $a0, s_fStopSync\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void IPU_DestroyContext ( void ) {

 DisableDmac ( DMAC_I_GIF );
 RemoveDmacHandler ( DMAC_I_GIF, g_IPUCtx.m_DMAHandlerID_GIF );
 DisableIntc ( INTC_VB_ON );
 RemoveIntcHandler ( INTC_VB_ON, g_IPUCtx.m_VBlankStartHandlerID );

 DeleteSema ( s_SyncS );

 if ( g_IPUCtx.m_pResult ) {

  DisableDmac ( DMAC_I_FROM_IPU );
  RemoveDmacHandler ( DMAC_I_FROM_IPU, g_IPUCtx.m_DMAHandlerID_IPU );

  free ( g_IPUCtx.m_pResult );
  g_IPUCtx.m_pResult = NULL;

 }  /* end if */

 IPU_RESET();

 if ( s_pDXSBDFrm ) free ( s_pDXSBDFrm );
 if ( s_pDXSBAFrm ) free ( s_pDXSBAFrm );
 if ( s_pDXSBRFrm ) free ( s_pDXSBRFrm );

}  /* end IPU_DestroyContext */

static void IPU_Flush ( void ) {

 if ( g_IPUCtx.m_VIFQueueSize != VIF_PIDX + 2 ) {

  DMA_SendChain ( DMAC_VIF1, &g_IPUCtx.m_DMAVIFDraw[ g_IPUCtx.m_VIFQueueSize ] );
  g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;

 } else if ( g_IPUCtx.m_ViFQueueSize != ViF_PIDX + 2 ) {

  DMA_SendChain ( DMAC_VIF1, &g_IPUCtx.m_DMAViFDraw[ g_IPUCtx.m_ViFQueueSize ] );
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
  "lui      $t1, 0x8000\n\t"
  "li       $v1, 0x3FFF\n\t"
  "sw       $zero, 32($a2)\n\t"
  "and      $v1, $v1, $v0\n\t"
  "or       $v1, $v1, $t1\n\t"
  "sw       $v1, 48($a2)\n\t"
  "dsll     $t0, $t0, 21\n\t"
  "addiu    $v0, $v0, 96\n\t"
  "1:\n\t"
  "subu     $a1, $a1, 1\n\t"
  "sd       $a0, 0($v0)\n\t"
  "daddu    $a0, $a0, $t0\n\t"
  "addu     $v0, $v0, 96\n\t"
  "nop\n\t"
  "bgtz     $a1, 1b\n\t"
  "li       $at, 0x0105\n\t"
  "sw       $at, 0($a2)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  :: "m"( g_IPUCtx.m_DestY      ),
     "m"( g_IPUCtx.m_nMBSlice   ),
     "m"( g_IPUCtx.m_pDMAPacket )
 );

 return -1;

}  /* end IPU_DMAHandlerFromIPU */

static void IPU_GIFHandlerDraw ( void ) {

 if (  g_IPUCtx.m_VIPQueueSize != VIP_PIDX + 2  ) {

  DMA_SendChain ( DMAC_VIF1, &g_IPUCtx.m_DMAVIPDraw[ g_IPUCtx.m_VIPQueueSize ] );
  g_IPUCtx.m_VIPQueueSize = VIP_PIDX + 2;

 }  /* end if */

 iSignalSema ( s_SyncS );

}  /* end IPU_GIFHandlerDraw */

static unsigned long* IPU_EraseSub ( unsigned long* apPkt ) {

 unsigned int lSrc = ( unsigned int )&s_DXSBDPack | 0x20000000;
 unsigned int lDst = ( unsigned int )&s_DXSBEPack | 0x20000000;

 s_pDXSBDFrm = s_pDXSBAFrm;
 s_pDXSBAFrm = NULL;

 SMS_DXSB_EP_XYZ_LTE( lDst ) = (  SMS_DXSB_DP_XYZ_LT( lSrc ) << 32  ) >> 32;
 SMS_DXSB_EP_XYZ_RBE( lDst ) = (  SMS_DXSB_DP_XYZ_RB( lSrc ) << 32  ) >> 32;

 apPkt[ 0 ] = DMA_TAG( 7, 1, DMATAG_ID_REF, 0, &s_DXSBEPack, 0 );

 return apPkt + 2;

}  /* end IPU_EraseSub */

static int IPU_VBlankStartHandler ( int aCause ) {

 if ( g_IPUCtx.m_fDraw ) {

  long           lVPTS = g_IPUCtx.m_VideoPTS;
  unsigned long* lpGIFPack;

  if ( !s_fStopSync && g_IPUCtx.m_pAudioPTS ) {

   int64_t lAudioPTS = *g_IPUCtx.m_pAudioPTS;
   int64_t lDiff     = lVPTS - lAudioPTS;

   if ( lDiff > 80 || !lAudioPTS )
    goto end;
   else if (   lDiff > -200 && (  ( *GS_CSR >> 13 ) & 1  )   ) goto end;

  }  /* end if */

  IPU_Flush ();

  lpGIFPack = ( unsigned long int* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );

  g_IPUCtx.m_fDraw    = 0;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;

  if ( s_pDXSBAFrm && s_pDXSBAFrm -> m_EndPTS <= lVPTS ) lpGIFPack = IPU_EraseSub ( lpGIFPack );

  if ( s_pDXSBRFrm && s_pDXSBRFrm -> m_StartPTS >= lVPTS && !s_pDXSBAFrm ) {

   int            lTW, lTH;
   SMS_DXSBFrame* lpFrm = s_pDXSBRFrm;
   unsigned int   lDst  = ( unsigned int )&s_DXSBDPack | 0x20000000;
   unsigned short lRW   = lpFrm -> m_RWidth;
   unsigned int   lTBW  = (  ( lRW + 63 ) & ~63  ) >> 6;

   s_pDXSBAFrm = lpFrm;
   s_pDXSBRFrm = NULL;

   GS_TWTH( lpFrm -> m_Width, lpFrm -> m_Height, &lTW, &lTH );

   SMS_DXSB_DP_BB_TBW( lDst )  = lTBW;
   SMS_DXSB_DP_TX_W( lDst )    = lRW;
   SMS_DXSB_DP_TX_H( lDst )    = lpFrm -> m_RHeight;
   SMS_DXSB_DP_QWC_GIF( lDst ) = lpFrm -> m_QWCPixmap;
   SMS_DXSB_DP_QWC_DMA( lDst ) = lpFrm -> m_QWCPixmap;
   SMS_DXSB_DP_PTR_DMA( lDst ) = ( unsigned int )lpFrm -> m_pPixmap;
   SMS_DXSB_DP_TEX0( lDst )    = GS_SET_TEX0_1(
    g_IPUCtx.m_SVRAM, lTBW, GSPixelFormat_PSMT8H, lTW, lTH, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL,
    g_GSCtx.m_CLUT[ 0 ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD
   );
   SMS_DXSB_DP_XYZ_LT( lDst )  = GS_XYZ ( lpFrm -> m_Left  * g_GSCtx.m_Width, lpFrm -> m_Top    * g_GSCtx.m_Height, 1 );
   SMS_DXSB_DP_UV_R( lDst )    = ( lpFrm -> m_Width  << 4 ) - 8;
   SMS_DXSB_DP_UV_B( lDst )    = ( lpFrm -> m_Height << 4 ) - 8;
   SMS_DXSB_DP_XYZ_RB( lDst )  = GS_XYZ ( lpFrm -> m_Right * g_GSCtx.m_Width, lpFrm -> m_Bottom * g_GSCtx.m_Height, 1 );

   lpGIFPack[ 0 ] = DMA_TAG( 0, 0, DMATAG_ID_CALL, 0, &s_DXSBDPack, 0 );
   lpGIFPack     += 2;

  }  /* end if */

  lpGIFPack[ 0 ] = DMA_TAG(  14, 1, DMATAG_ID_REFE, 0, g_IPUCtx.m_DMAGIFDraw, 0  );
  __asm__ __volatile__( "sync.l\n\t" );
  DMA_SendChain ( DMAC_GIF, g_IPUCtx.m_DMAGIFPack );

 }  /* end if */
end:
 return -1;

}  /* end IPU_VBlankStartHandler */

static void IPU_GIFHandlerSend ( void ) {

 if ( !--g_IPUCtx.m_Slice )

  g_IPUCtx.m_fDraw = 1;

 else {

  g_IPUCtx.m_DestY += 0x0010000000000000L;
  g_IPUCtx.m_pMB   += g_IPUCtx.m_MBStride;

  DMA_SendA ( DMAC_TO_IPU, g_IPUCtx.m_pMB, g_IPUCtx.m_QWCToIPUSlice );
  IPU -> m_CMD = g_IPUCtx.m_CSCmd;
  DMA_RecvA ( DMAC_FROM_IPU, g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

 }  /* end else */

}  /* end IPU_GIFHandlerSend */

static void IPU_GIFHandlerSendNoCSC ( void ) {

 g_IPUCtx.m_fDraw = 1;

}  /* end IPU_GIFHandlerSendNoCSC */

static int IPU_DMAHandlerToGIF ( int aChan ) {

 g_IPUCtx.GIFHandler ();

 return 0;

}  /* end IPU_DMAHandlerToGIF */

static void IPU_Display ( void* apFB, long aVideoPTS ) {

 WaitSema ( s_SyncS );

 g_IPUCtx.m_DestY = 0;
 g_IPUCtx.m_Slice = g_IPUCtx.m_nMBSlices;
 g_IPUCtx.m_pMB   = (  ( SMS_FrameBuffer* )apFB  ) -> m_pData;
 g_IPUCtx.m_fDraw = 0;

 g_IPUCtx.GIFHandler = IPU_GIFHandlerSend;
 g_IPUCtx.m_VideoPTS = aVideoPTS;

 DMA_SendA (   DMAC_TO_IPU, (  ( SMS_FrameBuffer* )apFB  ) -> m_pData, g_IPUCtx.m_QWCToIPUSlice   );
 IPU -> m_CMD = g_IPUCtx.m_CSCmd;
 DMA_RecvA ( DMAC_FROM_IPU, g_IPUCtx.m_pResult, g_IPUCtx.m_QWCFromIPUSlice );

}  /* end IPU_Display */

static void IPU_DisplayNoCSC ( void* apFB, long aVideoPTS ) {

 WaitSema ( s_SyncS );

 g_IPUCtx.m_fDraw    = 0;
 g_IPUCtx.GIFHandler = IPU_GIFHandlerSendNoCSC;
 g_IPUCtx.m_VideoPTS = aVideoPTS;

 DMA_SendChainA (  DMAC_GIF, (  ( SMS_FrameBuffer* )apFB  ) -> m_pData  );

}  /* end IPU_DisplayNoCSC */

static void IPU_ChangeMode ( unsigned int anIdx ) {

 if ( anIdx < 8 ) {

  g_IPUCtx.m_ModeIdx = anIdx;

  DIntr ();
   g_IPUCtx.m_DMAGIFDraw[  0 ] = GIF_TAG( 3, 0, 0, 0, 0, 1 );
   g_IPUCtx.m_DMAGIFDraw[  1 ] = GIFTAG_REGS_AD;
   g_IPUCtx.m_DMAGIFDraw[  2 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
   g_IPUCtx.m_DMAGIFDraw[  3 ] = GS_TEX1_2;
   g_IPUCtx.m_DMAGIFDraw[  4 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 1, 0 );
   g_IPUCtx.m_DMAGIFDraw[  5 ] = GS_PRIM;
   g_IPUCtx.m_DMAGIFDraw[  6 ] = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 );
   g_IPUCtx.m_DMAGIFDraw[  7 ] = GS_RGBAQ;
   g_IPUCtx.m_DMAGIFDraw[  8 ] = GIF_TAG( 2, 0, 0, 0, 1, 2 );
   g_IPUCtx.m_DMAGIFDraw[  9 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
   g_IPUCtx.m_DMAGIFDraw[ 10 ] = GS_SET_XYZ( 0, 0, 0 );
   if ( !g_IPUCtx.m_ImgLeft[ anIdx ] ) {
    g_IPUCtx.m_DMAGIFDraw[ 11 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ImgTop[ anIdx ], 0 );
    g_IPUCtx.m_DMAGIFDraw[ 12 ] = GS_SET_XYZ( 0,  g_IPUCtx.m_ImgBottom[ anIdx ], 0 );
    g_IPUCtx.m_DMAGIFDraw[ 13 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
   } else {
    g_IPUCtx.m_DMAGIFDraw[ 11 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], g_IPUCtx.m_ScrBottom, 0 );
    g_IPUCtx.m_DMAGIFDraw[ 12 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], 0, 0 );
    g_IPUCtx.m_DMAGIFDraw[ 13 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
   }  /* end else */
   g_IPUCtx.m_DMAGIFDraw[ 14 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
   g_IPUCtx.m_DMAGIFDraw[ 15 ] = GS_RGBAQ | ( GS_XYZ2 << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_NOP << 12 );
   g_IPUCtx.m_DMAGIFDraw[ 16 ] = g_IPUCtx.m_BRGBAQ;
   g_IPUCtx.m_DMAGIFDraw[ 17 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], g_IPUCtx.m_ImgTop   [ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 18 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], g_IPUCtx.m_ImgBottom[ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 19 ] = 0;
   g_IPUCtx.m_DMAGIFDraw[ 20 ] = GIF_TAG( 1, 1, 0, 0, 1, 6 );
   g_IPUCtx.m_DMAGIFDraw[ 21 ] = GS_TEX0_2 | ( GS_PRIM << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 ) | ( GS_UV << 16 ) | ( GS_XYZ2 << 20 );
   g_IPUCtx.m_DMAGIFDraw[ 22 ] = GS_SET_TEX0( g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, g_IPUCtx.m_TexFmt, g_IPUCtx.m_TW, g_IPUCtx.m_TH, 0, 2, 0, 0, 0, 0, 0 );
   g_IPUCtx.m_DMAGIFDraw[ 23 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 1, 0 );
   g_IPUCtx.m_DMAGIFDraw[ 24 ] = GS_SET_UV( g_IPUCtx.m_TxtLeft  [ anIdx ], g_IPUCtx.m_TxtTop   [ anIdx ] );
   g_IPUCtx.m_DMAGIFDraw[ 25 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], g_IPUCtx.m_ImgTop   [ anIdx ], 0 );
   g_IPUCtx.m_DMAGIFDraw[ 26 ] = GS_SET_UV( g_IPUCtx.m_TxtRight [ anIdx ], g_IPUCtx.m_TxtBottom[ anIdx ] );
   g_IPUCtx.m_DMAGIFDraw[ 27 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], g_IPUCtx.m_ImgBottom[ anIdx ], 0 );
   SyncDCache ( g_IPUCtx.m_DMAGIFDraw, &g_IPUCtx.m_DMAGIFDraw[ 28 ] );
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

  int lH;
  int lTop;
  int lBottom;

  if ( anIdx > 4 ) lAR *= 3.0F / 4.0F;

  lH      = ( int )( g_GSCtx.m_Width / lAR );
  lTop    = ( g_GSCtx.m_Height > lH ? g_GSCtx.m_Height - lH : lH - g_GSCtx.m_Height ) >> 1;
  lBottom = lTop + lH;

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

  int lW = ( int )(  ( float )g_GSCtx.m_Height * lAR + 0.5F  );

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

 lAR    = 16.0f/9.0f;
 lWP    = ( int )(  ( float )g_IPUCtx.m_Height * lAR  );
 lDelta = (  ( int )g_IPUCtx.m_Width - lWP  ) / 2;

 _ipu_compute_fields ( 5, g_IPUCtx.m_Width );  /* widescreen  */

 if ( lDelta > 0 ) {
  _ipu_compute_fields ( 6, g_IPUCtx.m_Width - lDelta );  /* widescreen pan-scan 1 */
  _ipu_compute_fields ( 7, lWP                       );  /* widescreen pan-scan 2 */
 } else {
  _ipu_compute_fields ( 6, g_IPUCtx.m_Width );  /* widescreen pan-scan 3 */
  _ipu_compute_fields ( 7, g_IPUCtx.m_Width );  /* widescreen pan-scan 3 */
 }  /* end else */

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

 DisableDmac ( DMAC_I_GIF );
 DisableDmac ( DMAC_I_FROM_IPU );
 DisableIntc ( INTC_VB_ON );

}  /* end IPU_Suspend */

static void IPU_SuspendNoCSC ( void ) {

 DisableDmac ( DMAC_I_GIF );
 DisableIntc ( INTC_VB_ON );

}  /* end IPU_SuspendNoCSC */

static void IPU_Resume ( void ) {

 DMAC -> m_STAT = 12;

 EnableDmac ( DMAC_I_FROM_IPU );
 EnableDmac ( DMAC_I_GIF      );
 EnableIntc ( INTC_VB_ON );

}  /* end IPU_Resume */

static void IPU_ResumeNoCSC ( void ) {

 DMAC -> m_STAT = 4;

 EnableDmac ( DMAC_I_GIF );
 EnableIntc ( INTC_VB_ON );

}  /* end IPU_ResumeNoCSC */

static void IPU_Repaint ( void ) {

 unsigned long int* lpGIFPack = ( unsigned long int* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );

 if ( s_pDXSBAFrm ) lpGIFPack = IPU_EraseSub ( lpGIFPack );

 lpGIFPack[ 0 ] = DMA_TAG(  14, 1, DMATAG_ID_REFE, 0, g_IPUCtx.m_DMAGIFDraw, 0  );
 __asm__ __volatile__( "sync.l\n\t" );
 DMA_SendChain ( DMAC_GIF, g_IPUCtx.m_DMAGIFPack );

 DMA_Wait ( DMAC_GIF );

}  /* end IPU_Repaint */

static void IPU_DummySuspend ( void ) {

 DisableDmac ( DMAC_I_GIF );

}  /* end IPU_Suspend */

static void IPU_DummyResume ( void ) {

 DMAC -> m_STAT = 4;

 EnableDmac ( DMAC_I_GIF );

}  /* end IPU_DummyResume */

static void IPU_DummyDisplay ( void* apParam, long aVideoPTS ) {

 WaitSema ( s_SyncS );

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

static int IPU_DummyVBlankStartHandler ( int aCause ) {

 if (  (  ( *GS_CSR >> 13 ) & 1  ) && g_IPUCtx.m_fDraw  ) {

  IPU_Flush ();

  g_IPUCtx.m_fDraw = 0;
  DMA_SendChainA ( DMAC_GIF, g_IPUCtx.m_pDMAPacket );

 }  /* end if */

 return -1;

}  /* end IPU_DummyVBlankStartHandler */

static void IPU_SetBrightness ( unsigned int aBrightness ) {

 unsigned long* lRGBAQ = UNCACHED_SEG( &g_IPUCtx.m_DMAGIFDraw[ 16 ] );
 unsigned int   lAlpha = aBrightness == 255 ? 0x20 : 0x00;

 lRGBAQ[ 0 ] = g_IPUCtx.m_BRGBAQ = GS_SET_RGBAQ( aBrightness, aBrightness, aBrightness, lAlpha, 0x00 );

}  /* end IPU_SetBrightness */

static void IPU_QueueSubtitle ( void* apSub ) {

 SMS_DXSBFrame* lpFrame = ( SMS_DXSBFrame* )apSub;

 if ( !s_pDXSBRFrm )
  s_pDXSBRFrm = lpFrame;
 else free ( lpFrame );

}  /* end IPU_QueueSubtitle */

IPUContext* IPU_InitContext ( int aWidth, int aHeight, long* apAudioPTS, int afNoCSC ) {

 ee_sema_t lSema;

 lSema.init_count = 1;
 s_SyncS = CreateSema ( &lSema );

 s_pVIFPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAVIFDraw[ 0 ] );
 s_pViFPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAViFDraw[ 0 ] );
 s_pVIPPacket = ( unsigned long int* )UNCACHED_SEG( &g_IPUCtx.m_DMAVIPDraw[ 0 ] );

 g_IPUCtx.m_VIFQueueSize = VIF_PIDX + 2;
 g_IPUCtx.m_ViFQueueSize = ViF_PIDX + 2;
 g_IPUCtx.m_VIPQueueSize = VIP_PIDX + 2;

 g_IPUCtx.Destroy       = IPU_DestroyContext;
 g_IPUCtx.QueuePacket   = IPU_QueuePacket;
 g_IPUCtx.iQueuePacket  = IPU_iQueuePacket;
 g_IPUCtx.PQueuePacket  = IPU_PQueuePacket;
 g_IPUCtx.Sync          = IPU_Sync;
 g_IPUCtx.Flush         = IPU_Flush;
 g_IPUCtx.SetBrightness = IPU_SetBrightness;
 g_IPUCtx.StopSync      = IPU_StopSync;
 g_IPUCtx.m_pAudioPTS   = apAudioPTS;

 s_pDXSBDFrm =
 s_pDXSBAFrm =
 s_pDXSBRFrm = NULL;

 if ( aWidth && aHeight ) {

  unsigned int lTBW  = ( aWidth + 63 ) >> 6;
  unsigned int lVRAM = g_GSCtx.m_VRAMPtr;

  g_IPUCtx.m_Width     = aWidth;
  g_IPUCtx.m_Height    = aHeight;
  g_IPUCtx.m_ScrRight  = g_GSCtx.m_PWidth  << 4;
  g_IPUCtx.m_ScrBottom = g_GSCtx.m_PHeight << 4;
  g_IPUCtx.m_VRAM      = lVRAM;
  g_IPUCtx.m_TBW       = lTBW;
  g_IPUCtx.m_ModeIdx   = 0;
  GS_TWTH ( aWidth, aHeight, &g_IPUCtx.m_TW, &g_IPUCtx.m_TH );

  g_IPUCtx.SetTEX     = IPU_SetTEX;
  g_IPUCtx.Reset      = IPU_Reset;
  g_IPUCtx.ChangeMode = IPU_ChangeMode;
  g_IPUCtx.Pan        = IPU_Pan;
  g_IPUCtx.Repaint    = IPU_Repaint;

  if ( !afNoCSC ) {

   unsigned int i;
   unsigned int lMBSz;
   unsigned int lMBQWC;
   unsigned int lSVRAM;
   uint8_t*     lpRes;
   uint64_t*    lpBuf;

   IPU_RESET();

   SMS_Linesize ( aWidth, &g_IPUCtx.m_MBStride );

   g_IPUCtx.m_nMBSlice      = ( aWidth  + 15 ) >> 4;
   g_IPUCtx.m_nMBSlices     = ( aHeight + 15 ) >> 4;
   g_IPUCtx.m_QWCToIPUSlice = g_IPUCtx.m_nMBSlice * 24;

   i = ( unsigned int )&s_DXSBDPack | 0x20000000;

   if (     (    (   lVRAM + (  ( aWidth * aHeight ) >> 6  ) > 0x4000   ) ||
                 ( g_Config.m_PlayerFlags & SMS_PF_C16 )
            ) && !( g_Config.m_PlayerFlags & SMS_PF_C32 )
   ) {

    lMBSz  = 512;
    lMBQWC =  32;
    g_IPUCtx.m_CSCmd  = 0x7C000000 | g_IPUCtx.m_nMBSlice;
    g_IPUCtx.m_PixFmt =
    g_IPUCtx.m_TexFmt = GSPixelFormat_PSMCT16;

    lSVRAM = lVRAM + (
     (  ( 1 << g_IPUCtx.m_TW ) * aHeight  ) >> 7
    );

   } else {

    lMBSz  = 1024;
    lMBQWC =   64;
    g_IPUCtx.m_CSCmd  = 0x70000000 | g_IPUCtx.m_nMBSlice;
    g_IPUCtx.m_PixFmt = GSPixelFormat_PSMCT32;
    g_IPUCtx.m_TexFmt = GSPixelFormat_PSMCT24;

    lSVRAM = lVRAM;

   }  /* end else */

   g_IPUCtx.m_QWCFromIPUSlice = g_IPUCtx.m_nMBSlice * lMBQWC;
   g_IPUCtx.m_pResult         = ( unsigned char* )memalign ( 64, lMBSz * g_IPUCtx.m_nMBSlice );
   g_IPUCtx.m_pDMAPacket      = ( uint64_t* )g_pSPRTop;
   FlushCache ( 0 );

   SMS_DXSB_DP_BB_VRAM( i ) = g_IPUCtx.m_SVRAM = lSVRAM;

   g_pSPRTop += ( g_IPUCtx.m_nMBSlice * 12 + 8 ) * 8;
   lpRes      = g_IPUCtx.m_pResult;
   lpBuf      = g_IPUCtx.m_pDMAPacket;

   g_IPUCtx.Display       = IPU_Display;
   g_IPUCtx.Suspend       = IPU_Suspend;
   g_IPUCtx.Resume        = IPU_Resume;
   g_IPUCtx.QueueSubtitle = IPU_QueueSubtitle;

   lpBuf[ 0 ] = DMA_TAG( 3, 0, DMATAG_ID_CNT, 0, 0, 0 );
   lpBuf[ 1 ] = 0;
   lpBuf[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
   lpBuf[ 3 ] = GIFTAG_REGS_AD;
   lpBuf[ 4 ] = GS_SET_TRXREG( 16, 16 );
   lpBuf[ 5 ] = GS_TRXREG;
   lpBuf[ 6 ] = GS_SET_BITBLTBUF( 0, 0, g_IPUCtx.m_PixFmt, lVRAM, lTBW, g_IPUCtx.m_PixFmt );
   lpBuf[ 7 ] = GS_BITBLTBUF;
   lpBuf += 8;

   for ( i = 0; i < g_IPUCtx.m_nMBSlice; ++i, lpRes += lMBSz, lpBuf += 12 ) {

    lpBuf[ 0 ] = DMA_TAG( 4, 0, DMATAG_ID_CNT, 0, 0, 0 );
    lpBuf[ 1 ] = 0;
     lpBuf[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
     lpBuf[ 3 ] = GIFTAG_REGS_AD;
      lpBuf[ 5 ] = GS_TRXPOS;
      lpBuf[ 6 ] = GS_SET_TRXDIR( 0 );
      lpBuf[ 7 ] = GS_TRXDIR;
     lpBuf[ 8 ] = GIF_TAG( lMBQWC, 1, 0, 0, 2, 1 );
     lpBuf[ 9 ] = 0;
    lpBuf[ 10 ] = DMA_TAG( lMBQWC, 1, DMATAG_ID_REF, 0, ( u32 )lpRes, 0  );
    lpBuf[ 11 ] = 0;

   }  /* end for */

   lpBuf[ -2 ] = DMA_TAG(  lMBQWC, 1, DMATAG_ID_REFE, 0, ( u32 )( lpRes - lMBSz ), 0  );
   lpBuf[ -1 ] = 0;

   g_IPUCtx.m_DMAHandlerID_IPU = AddDmacHandler ( DMAC_I_FROM_IPU, IPU_DMAHandlerFromIPU, 0 );

  } else {

   g_IPUCtx.m_PixFmt  = GSPixelFormat_PSMCT32;
   g_IPUCtx.m_TexFmt  = GSPixelFormat_PSMCT32;
   g_IPUCtx.m_pResult = NULL;
   g_IPUCtx.Suspend   = IPU_SuspendNoCSC;
   g_IPUCtx.Resume    = IPU_ResumeNoCSC;
   g_IPUCtx.Display   = IPU_DisplayNoCSC;

  }  /* end else */

  g_IPUCtx.SetTEX ();
  DMA_Wait ( DMAC_GIF );

  IPU_SetBrightness ( 0x80 );
  IPU_Reset ();

  g_IPUCtx.m_VBlankStartHandlerID = AddIntcHandler ( INTC_VB_ON,  IPU_VBlankStartHandler, 0 );

 } else {

  g_IPUCtx.m_pResult  = NULL;
  g_IPUCtx.Display    = IPU_DummyDisplay;
  g_IPUCtx.SetTEX     = IPU_Dummy;
  g_IPUCtx.Reset      = IPU_Dummy;
  g_IPUCtx.Suspend    = IPU_DummySuspend;
  g_IPUCtx.Resume     = IPU_DummyResume;
  g_IPUCtx.ChangeMode = IPU_DummyChangeMode;
  g_IPUCtx.Pan        = IPU_DummyPan;
  g_IPUCtx.GIFHandler = IPU_GIFHandlerDraw;
  g_IPUCtx.Repaint    = IPU_DummyRepaint;

  g_IPUCtx.m_VBlankStartHandlerID = AddIntcHandler ( INTC_VB_ON,  IPU_DummyVBlankStartHandler, 0 );

 }  /* end else */

 g_IPUCtx.m_DMAHandlerID_GIF = AddDmacHandler ( DMAC_I_GIF, IPU_DMAHandlerToGIF, 0 );

 EnableIntc ( INTC_VB_ON );
 g_IPUCtx.Resume ();

 return &g_IPUCtx;

}  /* end IPU_InitContext */
