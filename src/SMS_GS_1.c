/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2007      Petr Otoupal (HDTV support)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_VIF.h"
#include "SMS_INTC.h"
#include "SMS_Config.h"
#include "SMS_Locale.h"

#include <kernel.h>
#include <malloc.h>

extern int g_XShift;

GSContext g_GSCtx = {
 .m_Width    = 640U,
 .m_Height   = 480U,
 .m_PWidth   = 640U,
 .m_OffsetX  =    0,
 .m_OffsetY  =    0,
 .m_BkColor  =    0UL,
 .m_CodePage = GSCodePage_WinLatin1
};

static GSLoadImage  s_CLUTLoadImage __attribute__(  (  aligned( 64 )  )   );
static unsigned int s_CLUT[ 16 ]    __attribute__(  (  aligned( 64 )  )   );

void GSContext_Init ( GSVideoMode aMode, GSZTest aZTest, GSDoubleBuffer aDblBuf ) {

 unsigned int lSize;
 int          lPixSize;
 int          lColorDepth;
 int          lf16  = g_Config.m_ColorDepth;
 GSParams*    lpPar = GS_Params ();

 lpPar -> m_PARNTSC = g_Config.m_PAR[ 0 ];
 lpPar -> m_PARPAL  = g_Config.m_PAR[ 1 ];

 if ( aMode == GSVideoMode_Default ) aMode = g_pBXDATASYS[ 6 ] == 'E' ? GSVideoMode_PAL : GSVideoMode_NTSC;

 lSize = GS_VMode2Index ( aMode );

 g_GSCtx.m_PWidth     = g_Config.m_DispWH[ lSize ][ 0 ];
 g_GSCtx.m_Width      = g_Config.m_DispWH[ lSize ][ 0 ];
 g_GSCtx.m_PHeight    = g_Config.m_DispWH[ lSize ][ 1 ];
 g_GSCtx.m_Height     = g_Config.m_DispWH[ lSize ][ 1 ];
 g_GSCtx.m_DrawDelay  = g_Config.m_SyncPar[ lSize ][ aDblBuf ];
 g_GSCtx.m_DrawDelay2 = g_Config.m_SyncPar[ lSize ][ 2 ];

 if ( aMode == GSVideoMode_NTSC || aMode == GSVideoMode_PAL )
  g_GSCtx.m_Height = 480;
 else if ( aMode == GSVideoMode_DTV_1920x1080I ) {
  lf16             = aDblBuf;
  g_GSCtx.m_PWidth = g_GSCtx.m_PWidth >> !aDblBuf;
 }  /* end if */

 if ( lf16 ) {
  lColorDepth = GSPixelFormat_PSMCT16;
  lPixSize    = 2;
 } else {
  lColorDepth = GSPixelFormat_PSMCT24;
  lPixSize    = 3;
 }  /* end if */

 g_GSCtx.m_OffsetY -= g_GSCtx.m_OffsetY & 1;

 GS_Reset ( GSInterlaceMode_On, aMode, GSFieldMode_Field );
 GS_InitDC ( &g_GSCtx.m_DispCtx, lColorDepth, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, g_GSCtx.m_OffsetX, g_GSCtx.m_OffsetY );

 GIF_MODE() = 0;

 lSize = g_GSCtx.m_PWidth;

 while ( 1 ) {
  int lVal = lSize * lPixSize;
  if (  !( lVal & 15 ) && !( lVal % lPixSize )  ) break;
  ++lSize;
 }  /* end while */

 g_GSCtx.m_LWidth  = lSize;
 g_GSCtx.m_PixSize = lPixSize;

 lSize = GS_InitGC ( 0, &g_GSCtx.m_DrawCtx[ 0 ], lColorDepth, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, aZTest );
         GS_InitGC ( 1, &g_GSCtx.m_DrawCtx[ 1 ], lColorDepth, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, aZTest );
 GS_InitClear ( &g_GSCtx.m_ClearPkt, 0, 0, g_GSCtx.m_Width << 1, g_GSCtx.m_Height << 1, g_GSCtx.m_BkColor, aZTest );

 SyncDCache (   &g_GSCtx, (  ( unsigned char* )&g_GSCtx  ) + sizeof ( GSContext )   );

 g_GSCtx.m_VRAMPtr = g_GSCtx.m_VRAMPtr2 = g_GSCtx.m_DrawCtx[ 0 ].m_ZBUFVal.ZBP;

 if ( g_GSCtx.m_pDBuf ) {
  free ( g_GSCtx.m_pDBuf );
  g_GSCtx.m_pDBuf = NULL;
 }  /* end if */

 GS_SetDC (  &g_GSCtx.m_DispCtx, ( aMode <= GSVideoMode_PAL ) || ( aMode == GSVideoMode_DTV_1920x1080I )  );
 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 0 ] );
 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 1 ] );
 GS_Clear ( &g_GSCtx.m_ClearPkt     );

 if ( aZTest == GSZTest_On )

  g_GSCtx.m_VRAMPtr += lSize;

 else if ( aDblBuf && !g_GSCtx.m_pDBuf ) {

  g_GSCtx.m_pDBuf = ( unsigned char* )memalign (
   64, lSize = (  ( g_GSCtx.m_LWidth * g_GSCtx.m_PHeight * lPixSize + 63 ) & ~63  )
  );
  InvalidDCache ( g_GSCtx.m_pDBuf, g_GSCtx.m_pDBuf + lSize );

 }  /* end else */

 g_GSCtx.m_VRAMPtr <<= 5;

 GS_InitLoadImage (  UNCACHED_SEG( &s_CLUTLoadImage ), 0, 1, GSPixelFormat_PSMCT32, 0, 0, 8, 2  );

 for ( lSize = 0; lSize < 4; ++lSize ) GSContext_SetTextColor ( lSize, 0x0000000080FFFFFFL );

 if ( lColorDepth == GSPixelFormat_PSMCT16 )
  g_GSCtx.m_VRAMFontPtr = g_GSCtx.m_VRAMPtr;
 else g_GSCtx.m_VRAMFontPtr = 0;

 GSFont_Init ();

 if ( g_GSCtx.m_pDisplayList[ 0 ] ) free ( g_GSCtx.m_pDisplayList[ 0 ] );
 if ( g_GSCtx.m_pDisplayList[ 1 ] ) free ( g_GSCtx.m_pDisplayList[ 1 ] );

 g_GSCtx.m_pDisplayList[ 0 ] = NULL;
 g_GSCtx.m_pDisplayList[ 1 ] = NULL;
 g_GSCtx.m_nAlloc      [ 0 ] = 0;
 g_GSCtx.m_nAlloc      [ 1 ] = 0;
 g_GSCtx.m_PutIndex    [ 0 ] = 0;
 g_GSCtx.m_PutIndex    [ 1 ] = 0;

}  /* end GSContext_Init */

unsigned long* GSContext_NewPacket ( int aCtx, int aDWC, GSPaintMethod aMethod ) {

 int            lPutIdx = g_GSCtx.m_PutIndex[ aCtx ];
 unsigned int   lQWC    = ( aDWC + 1 ) >> 1;
 unsigned long* lpList  = g_GSCtx.m_pDisplayList[ aCtx ];

 DMA_Wait ( DMAC_VIF1 );

 if ( aDWC > g_GSCtx.m_nAlloc[ aCtx ] - lPutIdx - 8 )

  g_GSCtx.m_nAlloc[ aCtx ] = aDWC + g_GSCtx.m_nAlloc[ aCtx ] + 512;

 g_GSCtx.m_pDisplayList[ aCtx ] = lpList = ( unsigned long* )realloc64 (
  lpList, (  g_GSCtx.m_nAlloc[ aCtx ] * sizeof ( unsigned long ) + 63  ) & ~63
 );

 if ( aMethod & 0x02 ) {

  lPutIdx = 0;

  if ( aMethod & 0x01 ) {

   lpList[ 0 ] = DMA_TAG( 8, 1, DMATAG_ID_REF, 0, &g_GSCtx.m_ClearPkt, 0 );
   lpList[ 1 ] = 0;
   lPutIdx += 2;

  }  /* end if */

 }  /* end if */

 if ( aDWC ) {

  lpList[ lPutIdx ] = DMA_TAG( lQWC + 1, 1, DMATAG_ID_CNT, 0, 0, 0 );
  g_GSCtx.m_pLastTag[ aCtx ] = &lpList[ lPutIdx++ ];
  lpList[ lPutIdx++ ] = 0;
  lpList[ lPutIdx++ ] = 0;
  lpList[ lPutIdx++ ] = VIF_DIRECT( lQWC );

 }  /* end if */

 g_GSCtx.m_PutIndex[ aCtx ] = lPutIdx + aDWC;

 return lpList + lPutIdx;

}  /* end GSContext_NewPacket */

void GSContext_Flush ( int aCtx, GSFlushMethod aMethod ) {

 unsigned long* lpList = g_GSCtx.m_pDisplayList[ aCtx ];

 if ( aMethod != GSFlushMethod_DeleteListsOnly ) {

  DMATag* lpTag   = ( DMATag* )g_GSCtx.m_pLastTag[ aCtx ];
  int     lPutIdx = g_GSCtx.m_PutIndex[ aCtx ];

  if ( lpTag -> ID == DMATAG_ID_CNT )
   lpTag -> ID = DMATAG_ID_END;
  else if ( lpTag -> ID == DMATAG_ID_REF )
   lpTag -> ID = DMATAG_ID_REFE;
  else {
   lpList[ lPutIdx++ ] = DMA_TAG( 0, 0, DMATAG_ID_END, 0, 0, 0 );
   lpList[ lPutIdx   ] = 0;
  }  /* end else */

  SyncDCache ( lpList, lpList + lPutIdx );

  DMA_SendChainT ( DMAC_VIF1, lpList );

  g_GSCtx.m_PutIndex[ aCtx ] = 0;

 }  /* end if */

 if ( aMethod > GSFlushMethod_KeepLists ) {

  DMA_Wait ( DMAC_VIF1 );
  free ( lpList );

  g_GSCtx.m_pDisplayList[ aCtx ] = NULL;
  g_GSCtx.m_nAlloc[ aCtx ]       = 0;

 }  /* end if */

}  /* end GSContext_Flush */

unsigned long* GSContext_NewList ( unsigned int aDWC ) {

 unsigned int   lQWC   = ( aDWC + 1 ) >> 1;
 unsigned long* retVal = ( unsigned long* )memalign (
  64, (  ( aDWC + 2 ) * sizeof ( unsigned long ) + 63  ) & ~63
 );

 retVal[ 0 ] = 0;
 retVal[ 1 ] = VIF_DIRECT( lQWC );

 return retVal + 2;

}  /* end GSContext_NewList */

void GSContext_DeleteList ( unsigned long* apList ) {

 if ( apList ) free ( apList - 2 );

}  /* end GSContext_DeleteList */

void GSContext_CallList ( int aCtx, unsigned long* apList ) {

 unsigned int   lQWC    = ( apList[ -1 ] >> 32 ) & 0xFFFF;
 unsigned int   lPutIdx = g_GSCtx.m_PutIndex[ aCtx ];
 unsigned long* lpList  = g_GSCtx.m_pDisplayList[ aCtx ];

 if ( 2 > g_GSCtx.m_nAlloc[ aCtx ] - g_GSCtx.m_PutIndex[ aCtx ] )

  g_GSCtx.m_pDisplayList[ aCtx ] = lpList = ( unsigned long* )realloc (
   lpList, ( g_GSCtx.m_nAlloc[ aCtx ] = 2 + g_GSCtx.m_nAlloc[ aCtx ] + 512 ) * sizeof ( unsigned long )
  );

 g_GSCtx.m_pLastTag[ aCtx ] = &lpList[ lPutIdx ];
 lpList[ lPutIdx++ ] = DMA_TAG(  lQWC + 1, 1, DMATAG_ID_REF, 0, ( unsigned int )( apList - 2 ), 0  );
 lpList[ lPutIdx++ ] = 0;
 g_GSCtx.m_PutIndex[ aCtx ] = lPutIdx;

 SyncDCache (  apList - 2, apList + ( lQWC << 1 )  );

}  /* end GSContext_CallList */

void GSContext_CallList2 ( int aCtx, unsigned long* apList ) {

 unsigned int   lPutIdx = g_GSCtx.m_PutIndex    [ aCtx ];
 unsigned long* lpList  = g_GSCtx.m_pDisplayList[ aCtx ];

 if ( 2 > g_GSCtx.m_nAlloc[ aCtx ] - g_GSCtx.m_PutIndex[ aCtx ] )

  g_GSCtx.m_pDisplayList[ aCtx ] = lpList = ( unsigned long* )realloc (
   lpList, ( g_GSCtx.m_nAlloc[ aCtx ] = 2 + g_GSCtx.m_nAlloc[ aCtx ] + 512 ) * sizeof ( unsigned long )
  );

 g_GSCtx.m_pLastTag[ aCtx ] = &lpList[ lPutIdx ];
 lpList[ lPutIdx++ ] = DMA_TAG(  0, 0, DMATAG_ID_CALL, 0, ( unsigned int )apList, 0  );
 lpList[ lPutIdx++ ] = 0;
 g_GSCtx.m_PutIndex[ aCtx ] = lPutIdx;

}  /* end GSContext_CallList2 */

void GSContext_SetTextColor ( unsigned int aColorIdx, unsigned long aColor ) {

 unsigned long lDMA[ 8 ] __attribute__(   (  aligned( 64 )  )  );
 unsigned int* lpCLUT    = UNCACHED_SEG(  s_CLUT          );
 GSLoadImage*  lpLoadImg = UNCACHED_SEG( &s_CLUTLoadImage );

 lpCLUT[ 0 ] = 0;
 lpCLUT[ 1 ] = GS_SET_RGBAQ( 0, 0, 0, aColor >> 25, 0 );
 lpCLUT[ 2 ] = aColor;
 lpCLUT[ 3 ] = 0;
 lpCLUT[ 4 ] = aColor;
 lpCLUT[ 5 ] = GS_SET_RGBAQ( 0, 0, 0, aColor >> 25, 0 );

 lpLoadImg -> m_BitBltBufReg.DBP = 0x3FC0;

 GS_LoadImage ( &s_CLUTLoadImage, s_CLUT );

 lDMA[ 0 ] = GIF_TAG( 2, 1, 0, 0, 0, 1 );
 lDMA[ 1 ] = GIFTAG_REGS_AD;
 lDMA[ 2 ] = 0;
 lDMA[ 3 ] = GS_TEXFLUSH;
 lDMA[ 4 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, g_GSCtx.m_FontTexFmt, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0x3FC0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, aColorIdx, GS_TEX_CLD_LOAD );
 lDMA[ 5 ] = GS_TEX0_1;
 SyncDCache ( lDMA, lDMA + 6 );
 DMA_Send ( DMAC_GIF, lDMA, 3 );
 DMA_Wait ( DMAC_GIF );

}  /* end GSContext_SetTextColor */

void GSContext_RenderTexSprite ( GSTexSpritePacket* apPkt, int aX, int anY, int aWidth, int aHeight, int aTX, int aTY, int aTW, int aTH ) {

 unsigned long lXYZ0;
 unsigned long lXYZ1;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set noat\n\t"
  "pcpyld   $ra, $ra, $ra\n\t"
  "addu     $v0, %3, %5\n\t"
  "move     $a0, %2\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a1, %3\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "lw       $at, g_XShift\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "srav     %2, %2, $at\n\t"
  "srav     %4, %4, $at\n\t"
  "addu     $v1, %2, %4\n\t"
  "move     %0, $v0\n\t"
  "sll      $v1, $v1, 4\n\t"
  "or       %1, $v1, $a1\n\t"
  "pcpyud   $ra, $ra, $ra\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  : "=r"( lXYZ0 ), "=r"( lXYZ1 ) : "r"( aX ), "r"( anY ), "r"( aWidth ), "r"( aHeight ) : "a0", "a1", "a2", "v0", "v1"
 );

 apPkt -> m_VIFCodes[ 0 ]       = 0;
 apPkt -> m_VIFCodes[ 1 ]       = VIF_DIRECT( 7 );
 apPkt -> m_Tag0.m_HiLo.m_Lo    = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 apPkt -> m_Tag0.m_HiLo.m_Hi    = GIFTAG_REGS_AD;
 apPkt -> m_TEXFLUSHVal.m_Value = 0;
 apPkt -> m_TEXFLUSHTag         = GS_TEXFLUSH;
 apPkt -> m_Tag1.m_HiLo.m_Lo    = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apPkt -> m_Tag1.m_HiLo.m_Hi    = GS_TEX0_1 | ( GS_PRIM << 4 );
 apPkt -> m_TEX0Val.m_Value     = GS_SET_TEX0( g_GSCtx.m_VRAMTexPtr, g_GSCtx.m_TBW, GSPixelFormat_PSMCT32, g_GSCtx.m_TW, g_GSCtx.m_TH, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0, 0, 0, 0, 0 );
 apPkt -> m_PRIMVal.m_Value     = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_OFF, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 apPkt -> m_Tag2.m_HiLo.m_Lo    = GIF_TAG( 2, 1, 0, 0, 1, 2 );
 apPkt -> m_Tag2.m_HiLo.m_Hi    = GS_UV | ( GS_XYZ2 << 4 );
 apPkt -> m_UV0Val.m_Value      = GS_SET_UV( ++aTX << 4, ++aTY << 4 );
 apPkt -> m_XYZ0Val.m_Value     = lXYZ0;
 apPkt -> m_UV1Val.m_Value      = GS_SET_UV(  ( aTX + --aTW ) << 4, ( aTY + --aTH ) << 4  );
 apPkt -> m_XYZ1Val.m_Value     = lXYZ1;

}  /* end GSContext_RenderTexSprite */

void GSContext_RenderVGRect ( unsigned long* apDMA, int aX, int anY, int aWidth, int aHeight, unsigned long aClr0, unsigned long aClr1 ) {

 GSRegXYZ lXYZ0;
 GSRegXYZ lXYZ1;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  ".set noat\n\t"
  "pcpyld   $ra, $ra, $ra\n\t"
  "addu     $v0, %3, %5\n\t"
  "move     $a0, %2\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a1, %3\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "lw       $at, g_XShift\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "srav     %2, %2, $at\n\t"
  "srav     %4, %4, $at\n\t"
  "addu     $v1, %2, %4\n\t"
  "move     %0, $v0\n\t"
  "sll      $v1, $v1, 4\n\t"
  "or       %1, $v1, $a1\n\t"
  "pcpyud   $ra, $ra, $ra\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  : "=r"( lXYZ0.m_Value ), "=r"( lXYZ1.m_Value ) : "r"( aX ), "r"( anY ), "r"( aWidth ), "r"( aHeight ) : "a0", "a1", "a2", "v0", "v1"
 );

 apDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 1, 7 );
 apDMA[ 1 ] = ( u64 )GS_PRIM | (  ( u64 )GS_RGBAQ <<  4  ) | (  ( u64 )GS_XYZ2  <<  8  )
                             | (  ( u64 )GS_XYZ2  << 12  ) | (  ( u64 )GS_RGBAQ << 16  )
                             | (  ( u64 )GS_XYZ2  << 20  ) | (  ( u64 )GS_XYZ2  << 24  )
                             | (  ( u64 )GS_NOP   << 28  );
 apDMA[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 0, 0, 1, 0, 0, 0, 0 );
 apDMA[ 3 ] = aClr0;
 apDMA[ 4 ] = lXYZ0.m_Value;
 apDMA[ 5 ] = GS_SET_XYZ( lXYZ1.X, lXYZ0.Y, 0 );
 apDMA[ 6 ] = aClr1;
 apDMA[ 7 ] = GS_SET_XYZ( lXYZ0.X, lXYZ1.Y, 0 );
 apDMA[ 8 ] = lXYZ1.m_Value;
 apDMA[ 9 ] = 0;

}  /* end GSContext_RenderVGRect */

void GS_InitStoreImage ( GSStoreImage* apPkt, unsigned int aSrc, int aX, int anY, int aWidth, int aHeight ) {

 GSPixelFormat lPSM = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.PSM;
 unsigned int  lFBW = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.FBW;

 apPkt -> m_VIFCodes[ 0 ]        = 0x0000000006008000L;
 apPkt -> m_VIFCodes[ 1 ]        = 0x5000000613000000L;
 apPkt -> m_Tag.m_HiLo.m_Lo      = GIF_TAG( 5, 1, 0, 0, 0, 1 );
 apPkt -> m_Tag.m_HiLo.m_Hi      = GIFTAG_REGS_AD;
 apPkt -> m_BITBLTBUFVal.m_Value = GS_SET_BITBLTBUF ( aSrc << 5, lFBW, lPSM, 0, 0, 0 );
 apPkt -> m_BITBLTBUFTag         = GS_BITBLTBUF;
 apPkt -> m_TRXREGVal.m_Value    = GS_SET_TRXREG( aWidth, aHeight );
 apPkt -> m_TRXREGTag            = GS_TRXREG;
 apPkt -> m_TRXPOSVal.m_Value    = GS_SET_TRXPOS( aX, anY, 0, 0, GS_TRXPOS_DIR_LR_UD );
 apPkt -> m_TRXPOSTag            = GS_TRXPOS;
 apPkt -> m_FINISHVal.m_Value    = GS_SET_FINISH( 0 );
 apPkt -> m_FINISHTag            = GS_FINISH;
 apPkt -> m_TRXDIRVal.m_Value    = GS_SET_TRXDIR( GS_TRXDIR_LOCAL_TO_HOST );
 apPkt -> m_TRXDIRTag            = GS_TRXDIR;

}  /* end GS_InitStoreImage */

void GS_StoreImage ( GSStoreImage* apPkt, void* apDst ) {

 int  lW, lH, lHH, lSize;
 int  lRemB, lRemQ, lDMAQWC, lRSize;
 int  lnDMA, lDMARem;
 u128 lTmp;

 volatile unsigned int*  lpVIFStat  = ( volatile unsigned int*  )0x10003C00;
 volatile unsigned long* lpGSBusDir = ( volatile unsigned long* )0x12001040;
 volatile u128*          lpVIFIFO   = ( volatile u128*          )0x10005000;

 lW = apPkt -> m_TRXREGVal.RRW;
 lH = apPkt -> m_TRXREGVal.RRH;

 switch ( apPkt -> m_BITBLTBUFVal.SPSM ) {

  case GSPixelFormat_PSMCT32:

   lSize   = ( lW * lH ) << 2;
   lRemB   = lSize & 15;
   lRemQ   = ( lSize >> 4 ) &  7;
   lDMAQWC = ( lSize >> 4 ) & ~7;

   if ( !lRemB ) {
    lRSize = 0;
    lHH    = lH;
   } else {
    lHH      = ( lH + 3 ) & ~3;
    lRSize   = (  ( lW * lHH ) >> 2  ) - lDMAQWC - lRemQ - 1;
   }  /* end else */

  break;

  case GSPixelFormat_PSMCT24:

   lSize   = lW * lH * 3;
   lRemB   = lSize & 15;
   lRemQ   = ( lSize >> 4 ) &  7;
   lDMAQWC = ( lSize >> 4 ) & ~7;

   if ( !lRemB ) {
    lRSize = 0;
    lHH    = lH;
   } else {
    lHH    = ( lH + 15 ) & ~15;
    lRSize = (  ( lW * lHH * 3 ) >> 4  ) - lDMAQWC - lRemQ - 1;
   }  /* end else */

  break;

  case GSPixelFormat_PSMCT16:

   lSize   = ( lW * lH ) << 1;
   lRemB   = lSize & 15;
   lRemQ   = ( lSize >> 4 ) &  7;
   lDMAQWC = ( lSize >> 4 ) & ~7;

   if ( !lRemB ){
    lRSize = 0;
    lHH    = lH;
   } else {
    lHH    = ( lH + 7 ) & ~7;
    lRSize = (  ( lW * lHH ) >> 3  ) - lDMAQWC - lRemQ - 1;
   }  /* end else */

  break;

  default: return;

 }  /* end switch */

 if ( lRemB ) apPkt -> m_TRXREGVal.m_Value = GS_SET_TRXREG( lW, lHH );

 lnDMA   = lDMAQWC / 0xFFF8;
 lDMARem = lDMAQWC % 0xFFF8;

 *GS_CSR = 2;

 SyncDCache ( apPkt, apPkt + 1 );

 DMA_Send ( DMAC_VIF1, apPkt, 7 );

 while (  !( *GS_CSR & 2 )  );

 lpVIFStat [ 0 ] = 0x00800000;
 lpGSBusDir[ 0 ] = 0x00000001;

 while ( lnDMA-- ) {
  DMA_RecvA ( DMAC_VIF1, apDst, 0xFFF8 );
  apDst = (  ( u128* )apDst  ) + 0xFFF8;
  DMA_Wait ( DMAC_VIF1 );
 }  /* end while */

 if ( lDMARem ) {
  DMA_RecvA ( DMAC_VIF1, apDst, lDMARem );
  apDst = (  ( u128* )apDst  ) + lDMARem;
  DMA_Wait ( DMAC_VIF1 );
 }  /* end if */

 for ( lW = 0; lW < lRemQ; ++lW ) {
  while (   !( lpVIFStat[ 0 ] & 0x1F000000 )  );
  (  ( u128* )apDst  )[ lW ] = lpVIFIFO[ 0 ];
 }  /* end for */

 apDst = (  ( u128* )apDst  ) + lRemQ;

 if ( lRemB ) {
  while (   !( lpVIFStat[ 0 ] & 0x1F000000 )  );
  lTmp = lpVIFIFO[ 0 ];
  for ( lW = 0; lW < lRemB; ++lW )
   (  ( unsigned char* )apDst  )[ lW ] = (  ( unsigned char* )&lTmp  )[ lW ];
  while ( lRSize-- ) {
   while (   !( lpVIFStat[ 0 ] & 0x1F000000 )  );
   lpVIFIFO[ 0 ];
  }  /* end for */
 }  /* end if */

 lpVIFStat [ 0 ] = 0;
 lpGSBusDir[ 0 ] = 0;
 *GS_CSR         = 2;
 lpVIFIFO  [ 0 ] = 0x06000000;

}  /* end GS_StoreImage */
