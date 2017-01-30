/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2008 Eugene Plotnikov <e-plotnikov@operamail.com>
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
#include "SMS_EE.h"
#include "SMS.h"
#include "SMS_VideoBuffer.h"
#include "SMS_Timer.h"
#include "SMS_DXSB.h"
#include "SMS_MPEG12.h"
#include "SMS_VSync.h"

#define IPUF_WS 0x00000001
#define IPUF_PG 0x00000002
#define IPUF_FL 0x00000040

IPUContext        g_IPUCtx;
unsigned long int s_DMAVIFDraw[ 16 ] __attribute__(   (  aligned( 64 )  )   );
unsigned long int s_DMAGIFDraw[ 22 ] __attribute__(   (  aligned( 64 )  )   );

static SMS_FrameBuffer* s_pFrame __attribute__(  ( unused )  );
static SMS_DXSBFrame*   s_pDXSBDFrm;
static SMS_DXSBFrame*   s_pDXSBAFrm;
static SMS_DXSBFrame*   s_pDXSBRFrm;
static int              s_SyncS;
       unsigned long    s_VIFQueue[ 16 ];
static unsigned char    s_QIdx;
static unsigned char    s_fStopSync;
static unsigned char    s_fRefresh;

static SMS_DXSBDrawPack s_DXSBDPack __attribute__(   (  section( ".data" )  )   ) = {
 .m_DMATagUpload.m_Value     = DMA_TAG( 6, 0, DMATAG_ID_CNT, 0, 0, 0 ),
 .m_Pad0                     = 0,
 .m_GIFTagUpload.m_HiLo.m_Lo = GIF_TAG( 4, 0, 0, 0, GIFTAG_FLG_PACKED, 1 ),
 .m_GIFTagUpload.m_HiLo.m_Hi = GIFTAG_REGS_AD,
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
 .m_DMATagDraw.m_Value       = DMA_TAG( 10, 0, DMATAG_ID_END, 0, 0, 0 ),
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
                                0x00,
                                GS_TEST_AFAIL_KEEP,
                                GS_TEST_DATE_OFF,
                                GS_TEST_DATM_0PASS,
                                GS_TEST_ZTE_ON,
                                GS_TEST_ZTST_ALWAYS
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
                                GS_TEST_ATST_ALWAYS,
                                0x80,
                                GS_TEST_AFAIL_KEEP,
                                GS_TEST_DATE_OFF,
                                GS_TEST_DATM_0PASS,
                                GS_TEST_ZTE_ON,
                                GS_TEST_ZTST_ALWAYS
                               ),
 .m_TestOffID                = GS_TEST_1
};

       void IPU_EraseSub ( void );
static void IPU_DrawSub  ( void );

extern void IPU_Sync        ( void );
extern void IPU_StopSync    ( int  );
extern void IPU_RfshHandler ( void );
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
 "sb        $a0, s_fStopSync\n\t"
 "IPU_RfshHandler:\n\t"
 "lui       $a0, 0xFFFF\n\t"
 "lui       $a1, %hi( s_fRefresh )\n\t"
 "mfc0      $at, $12\n\t"
 "ori       $a0, $a0, 0x7FFF\n\t"
 "and       $at, $at, $a0\n\t"
 "sb        $a0, %lo( s_fRefresh )($a1)\n\t"
 "jr        $ra\n\t"
 "mtc0      $at, $12\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void IPU_DestroyContext ( void ) {

 SMS_SetAlarm ( 0, NULL, 0 );

 DisableDmac ( DMAC_I_GIF );
 RemoveDmacHandler ( DMAC_I_GIF, g_IPUCtx.m_DMAHandlerID_GIF );
 DeleteSema ( s_SyncS );

 IPU_RESET();

 if ( s_pDXSBDFrm ) free ( s_pDXSBDFrm );
 if ( s_pDXSBAFrm ) free ( s_pDXSBAFrm );
 if ( s_pDXSBRFrm ) free ( s_pDXSBRFrm );

}  /* end IPU_DestroyContext */

void IPU_Flush ( void );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "IPU_Flush:\n\t"
 "lbu      $a0, s_QIdx\n\t"
 "lui      $v0, 0x2000\n\t"
 "beqz     $a0, 2f\n\t"
 "la       $a1, s_VIFQueue\n\t"
 "la       $a2, s_DMAVIFDraw\n\t"
 "la       $at, s_DMAVIFDraw\n\t"
 "or       $a2, $a2, $v0\n\t"
 "1:\n\t"
 "ld       $v0, 0($a1)\n\t"
 "addiu    $a1, $a1, 8\n\t"
 "addiu    $a0 , $a0, -8\n\t"
 "lui      $v1, 0x1001\n\t"
 "sd       $v0, 0($a2)\n\t"
 "bnez     $a0, 1b\n\t"
 "addiu    $a2, $a2, 16\n\t"
 "sh       $zero, -14($a2)\n\t"
 "1:\n\t"
 "lw       $v0, -0x7000($v1)\n\t"
 "nop\n\t"
 "nop\n\t"
 "andi     $v0, $v0, 256\n\t"
 "nop\n\t"
 "bne      $v0, $zero, 1b\n\t"
 "addiu    $v0, $zero, 261\n\t"
 "sw       $zero, -0x6FE0($v1)\n\t"
 "sw       $at,   -0x6FD0($v1)\n\t"
 "sw       $v0,   -0x7000($v1)\n\t"
 "sb       $zero, s_QIdx\n\t"
 "2:\n\t"
 "jr       $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void IPU_SetTEX ( void ) {

 unsigned long* lpDMA = UNCACHED_SEG( g_IPUCtx.m_DMAGIFTX );

 lpDMA[ 0 ] = GIF_TAG( 3, 1, 0, 0, 0, 1 );
 lpDMA[ 1 ] = GIFTAG_REGS_AD;
 lpDMA[ 2 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 lpDMA[ 3 ] = GS_TEX1_1;
 lpDMA[ 4 ] = GS_SET_FRAME_2( g_IPUCtx.m_VRAM >> 5, g_IPUCtx.m_TBW, g_IPUCtx.m_TexFmt, 0 );
 lpDMA[ 5 ] = GS_FRAME_2;
 lpDMA[ 6 ] = GS_SET_SCISSOR_2( 0, g_IPUCtx.m_Width, 0, g_IPUCtx.m_Height );
 lpDMA[ 7 ] = GS_SCISSOR_2;
 __asm__ __volatile__( "sync.l\n\t" );
 DMA_Send ( DMAC_GIF, g_IPUCtx.m_DMAGIFTX, 4 );

}  /* end IPU_SetTEX */

int IPU_GIFHandlerDraw      ( int aCh, void* apArg, void* apAddr );
int IPU_GIFHandlerDummyDraw ( int aCh, void* apArg, void* apAddr );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "IPU_GIFHandlerDraw:\n\t"
 "lw    $a1, s_pFrame\n\t"
 "nor   $at, $zero, $zero\n\t"
 "sw    $at, 24($a1)\n\t"
 "IPU_GIFHandlerDummyDraw:\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "bgezal    $zero, IPU_Flush\n\t"
 "lw        $a3, s_SyncS\n\t"
 "jal       iSignalSema\n\t"
 "or        $a0, $zero, $a3\n\t"
 "mfc0      $at, $12\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 "ori       $at, $at, 0x8000\n\t"
 "mtc0      $zero, $9\n\t"
 "mtc0      $at, $12\n\t"
 "sync.p\n\t"
 "jr        $ra\n\t"
 "nor       $v0, $zero, $zero\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

extern void PowerOf2 ( int, int, int*, int* );

static void IPU_VBlankStartHandler ( void* apParam ) {

 if ( g_IPUCtx.m_fDraw ) {

  long lVPTS = g_IPUCtx.m_VideoPTS;

  if ( !s_fStopSync && g_IPUCtx.m_pAudioPTS ) {

   int64_t lAudioPTS = *g_IPUCtx.m_pAudioPTS;
   int64_t lDiff     = lVPTS - lAudioPTS;

   if ( lDiff > 80 || !lAudioPTS )
    return;
   else if ( lDiff > -200 && !s_fRefresh ) return;

  } else if ( !s_fRefresh ) return;

  s_fRefresh       = 0;
  g_IPUCtx.m_fDraw = 0;

  if ( s_pDXSBAFrm && s_pDXSBAFrm -> m_EndPTS   <= lVPTS                 ) IPU_EraseSub ();
  if ( s_pDXSBRFrm && s_pDXSBRFrm -> m_StartPTS <= lVPTS && !s_pDXSBAFrm ) IPU_DrawSub  ();

  DMA_SendChainA ( DMAC_GIF, g_IPUCtx.m_DMAGIFPack );

 }  /* end if */

}  /* end IPU_VBlankStartHandler */

void IPU_EraseSub ( void ) {

 unsigned long* lpGIFPack = ( unsigned long* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );

 lpGIFPack[ 4 ] = DMA_TAG( 11, 0, DMATAG_ID_REFE, 0, s_DMAGIFDraw, 0  );

 s_pDXSBDFrm = s_pDXSBAFrm;
 s_pDXSBAFrm = NULL;

 __asm__ __volatile__( "sync.l\n\t" );

}  /* end IPU_EraseSub */

static void IPU_DrawSub ( void ) {

 int            lTW, lTH;
 SMS_DXSBFrame* lpFrm     = s_pDXSBRFrm;
 unsigned int   lDst      = ( unsigned int )&s_DXSBDPack | 0x20000000;
 unsigned short lRW       = lpFrm -> m_RWidth;
 unsigned int   lTBW      = (  ( lRW + 63 ) & ~63  ) >> 6;
 unsigned long* lpGIFPack = ( unsigned long* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );

 s_pDXSBAFrm = lpFrm;
 s_pDXSBRFrm = NULL;

 PowerOf2 ( lpFrm -> m_Width, lpFrm -> m_Height, &lTW, &lTH );

 SMS_DXSB_DP_BB_TBW ( lDst ) = lTBW;
 SMS_DXSB_DP_TX_W   ( lDst ) = lRW;
 SMS_DXSB_DP_TX_H   ( lDst ) = lpFrm -> m_RHeight;
 SMS_DXSB_DP_QWC_GIF( lDst ) = lpFrm -> m_QWCPixmap;
 SMS_DXSB_DP_QWC_DMA( lDst ) = lpFrm -> m_QWCPixmap;
 SMS_DXSB_DP_PTR_DMA( lDst ) = ( unsigned int )lpFrm -> m_pPixmap;
 SMS_DXSB_DP_TEX0   ( lDst ) = GS_SET_TEX0_1(
  g_IPUCtx.m_VRAM, lTBW, GSPixelFormat_PSMT4HH, lTW, lTH, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL,
  0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_NOUPDATE
 );
 SMS_DXSB_DP_XYZ_LT ( lDst ) = GS_XYZ ( lpFrm -> m_Left  * g_GSCtx.m_Width, lpFrm -> m_Top    * g_GSCtx.m_Height, 0 );
 SMS_DXSB_DP_UV_R   ( lDst ) = ( lpFrm -> m_Width  << 4 ) - 8;
 SMS_DXSB_DP_UV_B   ( lDst ) = ( lpFrm -> m_Height << 4 ) - 8;
 SMS_DXSB_DP_XYZ_RB ( lDst ) = GS_XYZ ( lpFrm -> m_Right * g_GSCtx.m_Width, lpFrm -> m_Bottom * g_GSCtx.m_Height, 0 );

 lpGIFPack[ 4 ] = DMA_TAG( 11, 0, DMATAG_ID_REF,  0, s_DMAGIFDraw, 0 );
 lpGIFPack[ 6 ] = DMA_TAG(  0, 0, DMATAG_ID_NEXT, 0, &s_DXSBDPack, 0 );

 __asm__ __volatile__( "sync.l\n\t" );

}  /* end IPU_DrawSub */

static void IPU_Display ( void* apFB, long aVideoPTS ) {

 unsigned long int* lpGIFPack = ( unsigned long int* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );

 lpGIFPack[ 0 ] = DMA_TAG(  0, 0, DMATAG_ID_CALL, 0, ( unsigned int )(  ( SMS_FrameBuffer* )apFB  ) -> m_pData, 0  );
 s_pFrame       = ( SMS_FrameBuffer* )apFB;

 WaitSema ( s_SyncS );

 g_IPUCtx.m_VideoPTS = aVideoPTS;
 g_IPUCtx.m_fDraw    = 1; 

}  /* end IPU_Display */

static void IPU_ChangeMode ( unsigned int anIdx ) {

 if ( anIdx < 8 ) {

  int lImgTop = g_IPUCtx.m_ImgTop   [ anIdx ];
  int lImgBtm = g_IPUCtx.m_ImgBottom[ anIdx ];
  int lDelta  = g_Config.m_ImgOffs << 4;

  if ( lImgTop + lDelta < 0 )
   lDelta -= lImgTop + lDelta;
  else if ( lImgBtm + lDelta > g_IPUCtx.m_ScrBottom )
   lDelta -= ( lImgBtm + lDelta ) - g_IPUCtx.m_ScrBottom;

  lImgTop += lDelta;
  lImgBtm += lDelta;

  g_IPUCtx.m_ModeIdx = anIdx;

  SMS_EEDIntr ();
   s_DMAGIFDraw[  0 ] = GIF_TAG( 1, 0, 0, 0, 1, 6 );
   s_DMAGIFDraw[  1 ] = ( GS_TEX0_1 <<  0 ) | ( GS_PRIM <<  4 ) |
                        ( GS_UV     <<  8 ) | ( GS_XYZ2 << 12 ) |
                        ( GS_UV     << 16 ) | ( GS_XYZ2 << 20 );
   s_DMAGIFDraw[  2 ] = GS_SET_TEX0(
                         g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, g_IPUCtx.m_TexFmt,
                         g_IPUCtx.m_TW, g_IPUCtx.m_TH, 0, 1, 0, 0, 0, 0, 0
                        );
   s_DMAGIFDraw[  3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0 );
   s_DMAGIFDraw[  4 ] = GS_SET_UV( g_IPUCtx.m_TxtLeft  [ anIdx ], g_IPUCtx.m_TxtTop   [ anIdx ] );
   s_DMAGIFDraw[  5 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], lImgTop, 0 );
   s_DMAGIFDraw[  6 ] = GS_SET_UV( g_IPUCtx.m_TxtRight [ anIdx ], g_IPUCtx.m_TxtBottom[ anIdx ] );
   s_DMAGIFDraw[  7 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], lImgBtm, 0 );
   s_DMAGIFDraw[  8 ] = GIF_TAG( 3, 0, 0, 0, 0, 1 );
   s_DMAGIFDraw[  9 ] = GIFTAG_REGS_AD;
   s_DMAGIFDraw[ 10 ] = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
   s_DMAGIFDraw[ 11 ] = GS_TEX1_1;
   s_DMAGIFDraw[ 12 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
   s_DMAGIFDraw[ 13 ] = GS_PRIM;
   s_DMAGIFDraw[ 14 ] = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 );
   s_DMAGIFDraw[ 15 ] = GS_RGBAQ;
   s_DMAGIFDraw[ 16 ] = GIF_TAG( 2, 1, 0, 0, 1, 2 );
   s_DMAGIFDraw[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
   s_DMAGIFDraw[ 18 ] = GS_SET_XYZ( 0, 0, 0 );
   if ( !g_IPUCtx.m_ImgLeft[ anIdx ] ) {
    s_DMAGIFDraw[ 19 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, lImgTop, 0 );
    s_DMAGIFDraw[ 20 ] = GS_SET_XYZ( 0, lImgBtm, 0 );
    s_DMAGIFDraw[ 21 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
   } else {
    s_DMAGIFDraw[ 19 ] = GS_SET_XYZ( g_IPUCtx.m_ImgLeft [ anIdx ], g_IPUCtx.m_ScrBottom, 0 );
    s_DMAGIFDraw[ 20 ] = GS_SET_XYZ( g_IPUCtx.m_ImgRight[ anIdx ], 0, 0 );
    s_DMAGIFDraw[ 21 ] = GS_SET_XYZ( g_IPUCtx.m_ScrRight, g_IPUCtx.m_ScrBottom, 0 );
   }  /* end else */
   SyncDCache ( s_DMAGIFDraw, &s_DMAGIFDraw[ 22 ] );
  SMS_EEIntr ( 1 );

 }  /* end if */

}  /* end IPU_ChangeMode */

static void _ipu_compute_fields ( unsigned int anIdx, unsigned int aWidth ) {

 float lAR    = ( float )aWidth / ( float )g_IPUCtx.m_Height;
 int   lDelta = ( g_IPUCtx.m_Width - aWidth ) / 2;
 int   lShift = g_XShift;

 if ( g_IPUCtx.m_fWS && anIdx < 4 ) lAR += ( 16.0F / 9.0F - 4.0F / 3.0F );

 g_IPUCtx.m_TxtLeft  [ anIdx ] = ( lDelta << 4 ) + 8;
 g_IPUCtx.m_TxtTop   [ anIdx ] = 8;
 g_IPUCtx.m_TxtRight [ anIdx ] = ( aWidth << 4 ) + g_IPUCtx.m_TxtLeft[ anIdx ] - 6;
 g_IPUCtx.m_TxtBottom[ anIdx ] = ( g_IPUCtx.m_Height << 4 ) - 6;

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
   "pcpyld  $ra, $ra, $ra\n\t"
   "dsll32  %3, %3, 0\n\t"
   "move    $a0, $zero\n\t"
   "move    $a1, %2\n\t"
   "move    $a2, $zero\n\t"
   "jal     GS_XYZ\n\t"
   "or      $a1, $a1, %3\n\t"
   "srl     %0, $v0, 16\n\t"
   "dsrl32  %1, $a1, 16\n\t"
   "pcpyud  $ra, $ra, $ra\n\t"
   ".set reorder\n\t"
   : "=r"( lTop ), "=r"( lBottom )
   :  "r"( lTop ),  "r"( lBottom )
   : "a0", "a1", "a2", "v0", "v1"
  );

  g_IPUCtx.m_ImgLeft  [ anIdx ] = 0;
  g_IPUCtx.m_ImgRight [ anIdx ] = (  ( g_GSCtx.m_Width << 4 ) >> lShift  ) - 6;
  g_IPUCtx.m_ImgTop   [ anIdx ] = lTop;
  g_IPUCtx.m_ImgBottom[ anIdx ] = lBottom - 15;

 } else {

  int lW = ( int )(  ( float )g_GSCtx.m_Height * lAR + 0.5F  );

  g_IPUCtx.m_ImgTop   [ anIdx ] = 0;
  g_IPUCtx.m_ImgBottom[ anIdx ] = g_IPUCtx.m_ScrBottom - 15;
  g_IPUCtx.m_ImgLeft  [ anIdx ] = (   (  ( g_GSCtx.m_Width - lW ) >> 1  ) << 4   ) >> lShift;
  g_IPUCtx.m_ImgRight [ anIdx ] = ( g_IPUCtx.m_ImgLeft[ anIdx ] + (  ( lW << 4 ) >> lShift  ) ) - 15;

 }  /* end else */

}  /* end _ipu_compute_fields */

static void IPU_Pan ( int aDir ) {

 int lDelta = ( 64 << 4 ) - 16;

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

 float lHeight = g_IPUCtx.m_fWS ? ( float )g_IPUCtx.m_Height * ( 3.0F / 4.0F ) : ( float )g_IPUCtx.m_Height;
 float lAR     = (  ( float )g_GSCtx.m_Width  ) / (  ( float )g_GSCtx.m_Height  );
 int   lWP     = ( int )( lHeight * lAR );
 int   lDelta  = (  ( int )g_IPUCtx.m_Width - lWP  ) / 3;

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
 g_IPUCtx.m_TxtRight [ 4 ] = ( g_IPUCtx.m_Width  << 4 ) - 16;
 g_IPUCtx.m_TxtBottom[ 4 ] = ( g_IPUCtx.m_Height << 4 ) - 16;

 g_IPUCtx.m_ImgLeft  [ 4 ] = 0;
 g_IPUCtx.m_ImgTop   [ 4 ] = 0;
 g_IPUCtx.m_ImgRight [ 4 ] = g_GSCtx.m_PWidth  << 4;
 g_IPUCtx.m_ImgBottom[ 4 ] = g_GSCtx.m_PHeight << 4;

 lAR    = 16.0F / 9.0F;
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

void IPU_QueuePacket ( int, void* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "IPU_QueuePacket:\n\t"
 "lbu       $v0, s_QIdx\n\t"
 "la        $v1, s_VIFQueue\n\t"
 "lui       $at, 0x3000\n\t"
 "dsll32    $a1, $a1, 0\n\t"
 "or        $a0, $a0, $at\n\t"
 "addu      $v1, $v1, $v0\n\t"
 "addiu     $v0, $v0, 8\n\t"
 "or        $a1, $a1, $a0\n\t"
 "sb        $v0, s_QIdx\n\t"
 "jr        $ra\n\t"
 "sd        $a1, 0($v1)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void IPU_Suspend ( void ) {

 DisableDmac ( DMAC_I_GIF );

}  /* end IPU_Suspend */

static void IPU_Resume ( void ) {

 DMAC -> m_STAT = 4;

 EnableDmac ( DMAC_I_GIF );

}  /* end IPU_Resume */

static void IPU_Repaint ( void ) {

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

static void IPU_DummyVBlankStartHandler ( void* apParam ) {

 IPUContext* lpCtx = ( IPUContext* )apParam;

 if ( lpCtx -> m_fPG ) {
  if ( lpCtx -> m_fFL ^= 1 ) return;
 } else if (   !(  ( *GS_CSR >> 13 ) & 1  )   ) return;

 if ( lpCtx -> m_fDraw ) {

  lpCtx -> m_fDraw = 0;
  DMA_SendChainA ( DMAC_GIF, lpCtx -> m_pDMAPacket );

 }  /* end if */

}  /* end IPU_DummyVBlankStartHandler */

static void IPU_SetBrightness ( unsigned int aBrightness ) {

 unsigned long* lpPtr = UNCACHED_SEG( &g_IPUCtx.m_DMAGIFBgtn[ 4 ] );

 if ( aBrightness == 128 )
  lpPtr[ 0 ] = g_IPUCtx.m_Alpha = GS_SET_ALPHA( 1, 2, 2, 2, 0x80 );
 else {
  unsigned long lAlpha;
  if ( aBrightness > 128 ) {
   float lVal = aBrightness - 128;
   lAlpha = GS_SET_ALPHA_2( 1, 2, 2, 0, 0x80 );
   aBrightness = ( unsigned int )( lVal * 0.7F );
  } else {
   aBrightness = 128 - aBrightness;
   lAlpha      = GS_SET_ALPHA_2( 1, 0, 2, 2, 0x80 );
  }  /* end else */
  lpPtr[ 0 ] = g_IPUCtx.m_Alpha  = lAlpha;
  lpPtr[ 5 ] = g_IPUCtx.m_BRGBAQ = GS_SET_RGBAQ( aBrightness, aBrightness, aBrightness, 0x00, 0x00 );
 }  /* end else */

}  /* end IPU_SetBrightness */

static void IPU_QueueSubtitle ( void* apSub ) {

 SMS_DXSBFrame* lpFrame = ( SMS_DXSBFrame* )apSub;

 if ( !s_pDXSBRFrm )
  s_pDXSBRFrm = lpFrame;
 else {
  if ( s_pDXSBRFrm -> m_EndPTS <= g_IPUCtx.m_VideoPTS ) {
   free ( s_pDXSBRFrm );
   s_pDXSBRFrm = lpFrame;
  } else free ( lpFrame );
 }  /* end else */

}  /* end IPU_QueueSubtitle */

IPUContext* IPU_InitContext ( int aWidth, int aHeight, long* apAudioPTS, int afWS ) {

 ee_sema_t      lSema;
 unsigned short lCRTMode = GS_Params () -> m_GSCRTMode;

 int ( *GIFHandler ) ( int, void*, void* );

 lSema.init_count = 1;
 s_SyncS = CreateSema ( &lSema );

 s_QIdx     = 0;
 s_fRefresh = 1;

 g_IPUCtx.Destroy       = IPU_DestroyContext;
 g_IPUCtx.QueuePacket   = IPU_QueuePacket;
 g_IPUCtx.Sync          = IPU_Sync;
 g_IPUCtx.Flush         = IPU_Flush;
 g_IPUCtx.SetBrightness = IPU_SetBrightness;
 g_IPUCtx.StopSync      = IPU_StopSync;
 g_IPUCtx.m_pAudioPTS   = apAudioPTS;
 g_IPUCtx.m_Width       = aWidth;
 g_IPUCtx.m_Height      = aHeight;

 switch ( lCRTMode ) {
  case GSVideoMode_NTSC          :
  case GSVideoMode_PAL           :
  case GSVideoMode_DTV_1920x1080I: g_IPUCtx.m_fPG = 0; break;
  default                        : g_IPUCtx.m_fPG = 1; break;
 }  /* end switch */

 s_pDXSBDFrm =
 s_pDXSBAFrm =
 s_pDXSBRFrm = NULL;

 if ( aWidth && aHeight ) {

  unsigned int       lVRAM, lImgSize;
  unsigned long int* lpGIFPack;
  unsigned int       lTBW = ( aWidth + 63 ) >> 6;
  int                lf16 = ( g_Config.m_PlayerFlags & SMS_PF_C16 ) && !( g_Config.m_PlayerFlags & SMS_PF_C32 );
  unsigned int       lVSync;
retry:
  if ( lf16 ) {

   g_IPUCtx.m_PixFmt =
   g_IPUCtx.m_TexFmt = GSPixelFormat_PSMCT16;

  } else {

   g_IPUCtx.m_PixFmt = GSPixelFormat_PSMCT32;
   g_IPUCtx.m_TexFmt = GSPixelFormat_PSMCT24;

  }  /* end else */

  lImgSize = (   ( lTBW << 6 ) * (  ( aHeight + 31 ) & ~31  ) * ( 4 >> lf16 )   ) >> 8;
  lVRAM    = 0x4000 - lImgSize;
   
  if ( !lf16 && g_GSCtx.m_VRAMFontPtr && lVRAM < g_GSCtx.m_VRAMPtr ) {
   lf16 = 1;
   goto retry;
  }  /* end if */

  if (  !lf16 && !g_GSCtx.m_VRAMFontPtr && (
          lCRTMode == GSVideoMode_DTV_1920x1080I ||
          lCRTMode == GSVideoMode_DTV_1280x720P
         )
  ) lVRAM = ( g_GSCtx.m_VRAMPtr2 << 5 ) - lImgSize + 352;

  if ( !g_GSCtx.m_DrawDelay )
   lVSync = SMS_EstimateVSync ( lf16, aWidth, aHeight );
  else lVSync = g_GSCtx.m_DrawDelay;

  g_IPUCtx.m_ScrRight  = g_GSCtx.m_PWidth  << 4;
  g_IPUCtx.m_ScrBottom = g_GSCtx.m_PHeight << 4;
  g_IPUCtx.m_VRAM      = lVRAM;
  g_IPUCtx.m_TBW       = lTBW;
  g_IPUCtx.m_ModeIdx   = 0;
  g_IPUCtx.m_fWS       = afWS;
  PowerOf2 ( aWidth, aHeight, &g_IPUCtx.m_TW, &g_IPUCtx.m_TH );

  lpGIFPack = ( unsigned long int* )(  ( unsigned int )&s_DXSBDPack.m_BitBlt.m_Value | 0x20000000  );
  lpGIFPack[ 0 ] = GS_SET_BITBLTBUF( 0, 0, 0, lVRAM, 0, GSPixelFormat_PSMT4HH ),

  lpGIFPack = ( unsigned long int* )(  ( unsigned int )&g_IPUCtx.m_DMAGIFPack[ 0 ] | 0x20000000  );
  lpGIFPack[ 2 ] = DMA_TAG(  8, 0, DMATAG_ID_REF,  0, ( unsigned int )g_IPUCtx.m_DMAGIFBgtn, 0  );
  lpGIFPack[ 4 ] = DMA_TAG( 11, 0, DMATAG_ID_REFE, 0, s_DMAGIFDraw,                          0  );

  g_IPUCtx.SetTEX     = IPU_SetTEX;
  g_IPUCtx.Reset      = IPU_Reset;
  g_IPUCtx.ChangeMode = IPU_ChangeMode;
  g_IPUCtx.Pan        = IPU_Pan;
  g_IPUCtx.Repaint    = IPU_Repaint;

  g_IPUCtx.Suspend       = IPU_Suspend;
  g_IPUCtx.Resume        = IPU_Resume;
  g_IPUCtx.Display       = IPU_Display;
  g_IPUCtx.QueueSubtitle = IPU_QueueSubtitle;

  g_IPUCtx.m_DMAGIFBgtn[  0 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
  g_IPUCtx.m_DMAGIFBgtn[  1 ] = GIFTAG_REGS_AD;
  g_IPUCtx.m_DMAGIFBgtn[  2 ] = GS_SET_TEXFLUSH( 0 );
  g_IPUCtx.m_DMAGIFBgtn[  3 ] = GS_TEXFLUSH;
  g_IPUCtx.m_DMAGIFBgtn[  4 ] = g_IPUCtx.m_Alpha;
  g_IPUCtx.m_DMAGIFBgtn[  5 ] = GS_ALPHA_2;
  g_IPUCtx.m_DMAGIFBgtn[  6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
  g_IPUCtx.m_DMAGIFBgtn[  7 ] = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
  g_IPUCtx.m_DMAGIFBgtn[  8 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 1, 0 );
  g_IPUCtx.m_DMAGIFBgtn[  9 ] = g_IPUCtx.m_BRGBAQ;
  g_IPUCtx.m_DMAGIFBgtn[ 10 ] = GS_SET_XYZ( 0, 0, 0 );
  g_IPUCtx.m_DMAGIFBgtn[ 11 ] = GS_SET_XYZ( aWidth << 4, aHeight << 4, 0 );
  g_IPUCtx.m_DMAGIFBgtn[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  g_IPUCtx.m_DMAGIFBgtn[ 13 ] = GIFTAG_REGS_AD;
  g_IPUCtx.m_DMAGIFBgtn[ 14 ] = GS_SET_ALPHA_2( 0, 1, 0, 1, 0 );
  g_IPUCtx.m_DMAGIFBgtn[ 15 ] = GS_ALPHA_2;
  SyncDCache ( g_IPUCtx.m_DMAGIFBgtn, &g_IPUCtx.m_DMAGIFBgtn[ 16 ] );

  g_IPUCtx.SetTEX ();
  DMA_Wait ( DMAC_GIF );

  IPU_SetBrightness (  ( unsigned int )( g_Config.m_PlayerBrightness * 10.625F + 0.5F )  );
  IPU_Reset ();

  SMS_SetAlarm ( g_IPUCtx.m_VSync = lVSync, IPU_VBlankStartHandler, NULL );

  GIFHandler = IPU_GIFHandlerDraw;

 } else {

  g_IPUCtx.Display    = IPU_DummyDisplay;
  g_IPUCtx.SetTEX     = IPU_Dummy;
  g_IPUCtx.Reset      = IPU_Dummy;
  g_IPUCtx.Suspend    = IPU_DummySuspend;
  g_IPUCtx.Resume     = IPU_DummyResume;
  g_IPUCtx.ChangeMode = IPU_DummyChangeMode;
  g_IPUCtx.Pan        = IPU_DummyPan;
  g_IPUCtx.Repaint    = IPU_DummyRepaint;

  SMS_SetAlarm ( g_GSCtx.m_DrawDelay2, IPU_DummyVBlankStartHandler, &g_IPUCtx );

  __asm__ __volatile__(
   ".set noat\n\t"
   "nor     $at, $zero, $zero\n\t"
   "mtc0    $a1, $11\n\t"
   ".set at\n\t"
  );

  GIFHandler = IPU_GIFHandlerDummyDraw;

 }  /* end else */

 g_IPUCtx.m_DMAHandlerID_GIF = AddDmacHandler2 ( DMAC_I_GIF, GIFHandler, 0, NULL );
 g_IPUCtx.Resume ();
 SetCPUTimerHandler ( IPU_RfshHandler );

 return &g_IPUCtx;

}  /* end IPU_InitContext */
