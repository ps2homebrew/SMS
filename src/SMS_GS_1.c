/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_VIF.h"
#include "SMS_Config.h"

#include <kernel.h>
#include <malloc.h>

GSContext g_GSCtx = {
 .m_Width    = 640U,
 .m_Height   = 480U,
 .m_PWidth   = 640U,
 .m_OffsetX  =    0,
 .m_OffsetY  =    0,
 .m_BkColor  =    0UL,
 .m_CodePage = GSCodePage_WinLatin1
};

static GSLoadImage  s_CLUTLoadImage;
static unsigned int s_CLUT[ 16 ] __attribute__(  (  aligned( 16 )  )   );

void GSContext_Init ( GSVideoMode aMode, GSZTest aZTest, GSDoubleBuffer aDblBuf ) {

 unsigned int lSize;
 GSParams*    lpPar = GS_Params ();

 lpPar -> m_PARNTSC = g_Config.m_PAR[ 0 ];
 lpPar -> m_PARPAL  = g_Config.m_PAR[ 1 ];

 if ( aMode == GSVideoMode_Default ) aMode = *( volatile char* )0x1FC7FF52 == 'E' ? GSVideoMode_PAL : GSVideoMode_NTSC;

 switch ( aMode ) {

  case GSVideoMode_NTSC:
   g_GSCtx.m_PWidth  = 640;
   g_GSCtx.m_Width   = 640;
   g_GSCtx.m_PHeight = g_Config.m_DispH[ 0 ];
   g_GSCtx.m_Height  = 480;
  break;

  default             :
  case GSVideoMode_PAL:
   g_GSCtx.m_PWidth  = 640;
   g_GSCtx.m_Width   = 640;
   g_GSCtx.m_PHeight = g_Config.m_DispH[ 1 ];
   g_GSCtx.m_Height  = 480;
  break;

  case GSVideoMode_DTV_720x480P:
   g_GSCtx.m_PWidth  = 640;
   g_GSCtx.m_Width   = 640;
   g_GSCtx.m_PHeight = 512;
   g_GSCtx.m_Height  = 512;
  break;

  case GSVideoMode_VESA_60Hz:
  case GSVideoMode_VESA_75Hz:
   g_GSCtx.m_PWidth  = 640;
   g_GSCtx.m_Width   = 640;
   g_GSCtx.m_PHeight = 480;
   g_GSCtx.m_Height  = 480;
  break;

 }  /* end switch */

 GS_Reset ( GSInterlaceMode_On, aMode, GSFieldMode_Field );
 GS_InitDC ( &g_GSCtx.m_DispCtx, GSPixelFormat_PSMCT32, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, g_GSCtx.m_OffsetX, g_GSCtx.m_OffsetY );
 lSize = GS_InitGC ( 0, &g_GSCtx.m_DrawCtx[ 0 ], GSPixelFormat_PSMCT32, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, aZTest );
         GS_InitGC ( 1, &g_GSCtx.m_DrawCtx[ 1 ], GSPixelFormat_PSMCT32, g_GSCtx.m_PWidth, g_GSCtx.m_PHeight, aZTest );
 GS_InitClear ( &g_GSCtx.m_ClearPkt, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height << 1, g_GSCtx.m_BkColor, aZTest );

 SyncDCache (   &g_GSCtx, (  ( unsigned char* )&g_GSCtx  ) + sizeof ( GSContext )   );

 g_GSCtx.m_VRAMPtr = g_GSCtx.m_VRAMPtr2 = g_GSCtx.m_DrawCtx[ 0 ].m_ZBUFVal.ZBP;

 if ( aZTest == GSZTest_On )

  g_GSCtx.m_VRAMPtr += lSize;

 else if ( aDblBuf ) g_GSCtx.m_VRAMPtr <<= 1;

 g_GSCtx.m_VRAMPtr <<= 5;

 GS_SetDC ( &g_GSCtx.m_DispCtx, 1   );
 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 0 ] );
 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 1 ] );
 GS_Clear ( &g_GSCtx.m_ClearPkt     );

 GS_InitLoadImage (  UNCACHED_SEG( &s_CLUTLoadImage ), 0, 1, GSPixelFormat_PSMCT32, 0, 0, 8, 2  );

 for ( lSize = 0; lSize < 4; ++lSize ) {

  g_GSCtx.m_CLUT[ lSize ] = g_GSCtx.m_VRAMPtr++;
  GSContext_SetTextColor ( lSize, 0x0000000080FFFFFFL );

 }  /* end for */

 g_GSCtx.m_VRAMFontPtr = g_GSCtx.m_VRAMPtr;

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

 int          lPutIdx = g_GSCtx.m_PutIndex[ aCtx ];
 unsigned int lQWC    = ( aDWC + 1 ) >> 1;

 DMA_Wait ( DMAC_VIF1 );

 if ( aDWC > g_GSCtx.m_nAlloc[ aCtx ] - lPutIdx - 6 )

  g_GSCtx.m_pDisplayList[ aCtx ] = ( unsigned long* )realloc (
   g_GSCtx.m_pDisplayList[ aCtx ], ( g_GSCtx.m_nAlloc[ aCtx ] = aDWC + g_GSCtx.m_nAlloc[ aCtx ] + 512 ) * sizeof ( unsigned long )
  );

 if ( aMethod & 0x02 ) {

  lPutIdx = 0;

  if ( aMethod & 0x01 ) {

   g_GSCtx.m_pDisplayList[ aCtx ][ 0 ] = DMA_TAG( 8, 1, DMATAG_ID_REF, 0, &g_GSCtx.m_ClearPkt, 0 );
   g_GSCtx.m_pDisplayList[ aCtx ][ 1 ] = 0;
   lPutIdx += 2;

  }  /* end if */

 }  /* end if */

 if ( aDWC ) {

  g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx   ] = DMA_TAG( lQWC + 1, 1, DMATAG_ID_CNT, 0, 0, 0 );
  g_GSCtx.m_pLastTag    [ aCtx ]              = &g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ];
  g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ] = 0;
  g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ] = 0;
  g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ] = VIF_DIRECT( lQWC );

 }  /* end if */

 g_GSCtx.m_PutIndex[ aCtx ] = lPutIdx + aDWC;

 return g_GSCtx.m_pDisplayList[ aCtx ] + lPutIdx;

}  /* end GSContext_NewPacket */

void GSContext_Flush ( int aCtx, GSFlushMethod aMethod ) {

 DMATag* lpTag = ( DMATag* )g_GSCtx.m_pLastTag[ aCtx ];

 lpTag -> ID = lpTag -> ID == DMATAG_ID_CNT ? DMATAG_ID_END : DMATAG_ID_REFE;

 SyncDCache ( g_GSCtx.m_pDisplayList[ aCtx ], g_GSCtx.m_pDisplayList[ aCtx ] + g_GSCtx.m_PutIndex[ aCtx ] );
 DMA_SendChain ( DMAC_VIF1, g_GSCtx.m_pDisplayList[ aCtx ] );

 g_GSCtx.m_PutIndex[ aCtx ] = 0;

 if ( aMethod == GSFlushMethod_DeleteLists ) {

  DMA_Wait ( DMAC_VIF1 );
  free ( g_GSCtx.m_pDisplayList[ aCtx ] );

  g_GSCtx.m_pDisplayList[ aCtx ] = NULL;
  g_GSCtx.m_nAlloc[ aCtx ]       = 0;

 }  /* end if */

}  /* end GSContext_Flush */

unsigned long* GSContext_NewList ( unsigned int aDWC ) {

 unsigned int   lQWC   = ( aDWC + 1 ) >> 1;
 unsigned long* retVal = ( unsigned long* )malloc (  ( aDWC + 2 ) * sizeof ( unsigned long )  );

 retVal[ 0 ] = 0;
 retVal[ 1 ] = VIF_DIRECT( lQWC );

 return retVal + 2;

}  /* end GSContext_NewList */

void GSContext_DeleteList ( unsigned long* apList ) {

 if ( apList ) free ( apList - 2 );

}  /* end GSContext_DeleteList */

void GSContext_CallList ( int aCtx, unsigned long* apList ) {

 unsigned int lQWC    = ( apList[ -1 ] >> 32 ) & 0xFFFF;
 unsigned int lPutIdx = g_GSCtx.m_PutIndex[ aCtx ];

 if ( 2 > g_GSCtx.m_nAlloc[ aCtx ] - g_GSCtx.m_PutIndex[ aCtx ] )

  g_GSCtx.m_pDisplayList[ aCtx ] = ( unsigned long* )realloc (
   g_GSCtx.m_pDisplayList[ aCtx ], ( g_GSCtx.m_nAlloc[ aCtx ] = 2 + g_GSCtx.m_nAlloc[ aCtx ] + 512 ) * sizeof ( unsigned long )
  );

 g_GSCtx.m_pLastTag    [ aCtx ]              = &g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx ];
 g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ] = DMA_TAG(  lQWC + 1, 1, DMATAG_ID_REF, 0, ( unsigned int )( apList - 2 ), 0  );
 g_GSCtx.m_pDisplayList[ aCtx ][ lPutIdx++ ] = 0;
 g_GSCtx.m_PutIndex    [ aCtx ]              = lPutIdx;

 SyncDCache (  apList - 2, apList + ( lQWC << 1 )  );

}  /* end GSContext_CallList */

void GSContext_SetTextColor ( unsigned int aColorIdx, unsigned long aColor ) {

 unsigned int* lpCLUT    = UNCACHED_SEG(  s_CLUT          );
 GSLoadImage*  lpLoadImg = UNCACHED_SEG( &s_CLUTLoadImage );

 lpCLUT[ 0 ] = 0;
 lpCLUT[ 1 ] = GS_SET_RGBAQ( 0, 0, 0, aColor >> 24, 0 );
 lpCLUT[ 2 ] = aColor;

 lpLoadImg -> m_BitBltBufReg.DBP = g_GSCtx.m_CLUT[ aColorIdx ];
 GS_LoadImage ( &s_CLUTLoadImage, s_CLUT );
 DMA_Wait ( DMAC_GIF );

}  /* end GSContext_SetTextColor */

void GSContext_InitBitBlt ( GSBitBltPacket* apPkt, unsigned int aDst, int aDstX, int aDstY, int aWidth, int aHeight, unsigned int aSrc, int aSrcX, int aSrcY ) {

 GSPixelFormat lPSM;
 unsigned int  lFBW;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "pcpyld   %3, $zero, %3\n\t"
  "move     $t9, $ra\n\t"
  "dsll32   %3, %3, 0\n\t"
  "pxor     $v0, $v0, $v0\n\t"
  "dsrl32   %3, %3, 0\n\t"
  "dsll32   $v0, %4, 0\n\t"
  "pcpyld   $v1, %5, $zero\n\t"
  "por      $a1, $v0, %3\n\t"
  "move     $a2, $zero\n\t"
  "move     $a0, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "por      $a1, $a1, $v1\n\t"
  "srl      %0, $a1, 20\n\t"
  "pcpyud   $v1, $a1, $zero\n\t"
  "dsrl32   %1, $a1, 20\n\t"
  "srl      %2, $v1, 20\n\t"
  "move     $ra, $t9\n\t"
  ".set reorder\n\t"
  :"=r"( aDstY ), "=r"( aHeight ), "=r"( aSrcY ) : "r"( aDstY ), "r"( aHeight ), "r"( aSrcY ) : "a0", "a1", "a2", "t9"
 );

 lPSM = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.PSM;
 lFBW = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.FBW;

 apPkt -> m_VIFCodes[ 0 ]        = 0;
 apPkt -> m_VIFCodes[ 1 ]        = VIF_DIRECT( 6 );
 apPkt -> m_Tag.m_Lo             = GIF_TAG( 5, 1, 0, 0, 0, 1 );
 apPkt -> m_Tag.m_Hi             = GIFTAG_REGS_AD;
 apPkt -> m_BITBLTBUFVal.m_Value = GS_SET_BITBLTBUF ( aSrc << 5, lFBW, lPSM, aDst << 5, lFBW, lPSM );
 apPkt -> m_BITBLTBUFTag         = GS_BITBLTBUF;
 apPkt -> m_TRXREGVal.m_Value    = GS_SET_TRXREG( aWidth, aHeight );
 apPkt -> m_TRXREGTag            = GS_TRXREG;
 apPkt -> m_TRXPOSVal.m_Value    = GS_SET_TRXPOS( aSrcX, aSrcY, aDstX, aDstY, GS_TRXPOS_DIR_LR_UD );
 apPkt -> m_TRXPOSTag            = GS_TRXPOS;
 apPkt -> m_TRXDIRVal.m_Value    = GS_SET_TRXDIR( GS_TRXDIR_LOCAL_TO_LOCAL );
 apPkt -> m_TRXDIRTag            = GS_TRXDIR;
 apPkt -> m_FINISHVal.m_Value    = GS_SET_FINISH( 0 );
 apPkt -> m_FINISHTag            = GS_FINISH;

}  /* end GS_InitBitBlt */

void GSContext_BitBlt ( GSBitBltPacket* apPkt ) {

 *GS_CSR = *GS_CSR | 2;

 SyncDCache ( apPkt, apPkt + 1 );

 DMA_Send ( DMAC_VIF1, apPkt, 7 );

 while (  !( *GS_CSR & 2 )  );

}  /* end GSContext_BitBlt */

void GSContext_RenderTexSprite ( GSTexSpritePacket* apPkt, int aX, int anY, int aWidth, int aHeight, int aTX, int aTY, int aTW, int aTH ) {

 unsigned long lXYZ0;
 unsigned long lXYZ1;

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "move     $t9, $ra\n\t"
  "addu     $v0, %3, %5\n\t"
  "move     $a0, %2\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a1, %3\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "addu     $v1, %2, %4\n\t"
  "move     %0, $v0\n\t"
  "sll      $v1, $v1, 4\n\t"
  "or       %1, $v1, $a1\n\t"
  "move     $ra, $t9\n\t"
  ".set reorder\n\t"
  : "=r"( lXYZ0 ), "=r"( lXYZ1 ) : "r"( aX ), "r"( anY ), "r"( aWidth ), "r"( aHeight ) : "a0", "a1", "a2", "v0", "v1", "t9"
 );

 apPkt -> m_VIFCodes[ 0 ]       = 0;
 apPkt -> m_VIFCodes[ 1 ]       = VIF_DIRECT( 7 );
 apPkt -> m_Tag0.m_Lo           = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 apPkt -> m_Tag0.m_Hi           = GIFTAG_REGS_AD;
 apPkt -> m_TEXFLUSHVal.m_Value = 0;
 apPkt -> m_TEXFLUSHTag         = GS_TEXFLUSH;
 apPkt -> m_Tag1.m_Lo           = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apPkt -> m_Tag1.m_Hi           = GS_TEX0_1 | ( GS_PRIM << 4 );
 apPkt -> m_TEX0Val.m_Value     = GS_SET_TEX0( g_GSCtx.m_VRAMTexPtr, g_GSCtx.m_TBW, GSPixelFormat_PSMCT32, g_GSCtx.m_TW, g_GSCtx.m_TH, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0, 0, 0, 0, 1 );
 apPkt -> m_PRIMVal.m_Value     = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, GS_PRIM_IIP_FLAT, GS_PRIM_TME_ON, GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_OFF, GS_PRIM_FST_UV, GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED );
 apPkt -> m_Tag2.m_Lo           = GIF_TAG( 2, 1, 0, 0, 1, 2 );
 apPkt -> m_Tag2.m_Hi           = GS_UV | ( GS_XYZ2 << 4 );
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
  "move     $t9, $ra\n\t"
  "addu     $v0, %3, %5\n\t"
  "move     $a0, %2\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a1, %3\n\t"
  "move     $a2, $zero\n\t"
  "jal      GS_XYZ\n\t"
  "or       $a1, $a1, $v0\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "addu     $v1, %2, %4\n\t"
  "move     %0, $v0\n\t"
  "sll      $v1, $v1, 4\n\t"
  "or       %1, $v1, $a1\n\t"
  "move     $ra, $t9\n\t"
  ".set reorder\n\t"
  : "=r"( lXYZ0.m_Value ), "=r"( lXYZ1.m_Value ) : "r"( aX ), "r"( anY ), "r"( aWidth ), "r"( aHeight ) : "a0", "a1", "a2", "v0", "v1", "t9"
 );

 apDMA[  0 ] = GIF_TAG( 1, 1, 0, 0, 1, 9 );
 apDMA[  1 ] = ( u64 )GS_PRIM | (  ( u64 )GS_RGBAQ <<  4  ) | (  ( u64 )GS_XYZ2 <<  8  )
                              | (  ( u64 )GS_RGBAQ << 12  ) | (  ( u64 )GS_XYZ2 << 16  )
                              | (  ( u64 )GS_RGBAQ << 20  ) | (  ( u64 )GS_XYZ2 << 24  )
                              | (  ( u64 )GS_RGBAQ << 28  ) | (  ( u64 )GS_XYZ2 << 32  );
 apDMA[  2 ] = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 0, 0, 1, 0, 0, 0, 0 );
 apDMA[  3 ] = aClr0;
 apDMA[  4 ] = lXYZ0.m_Value;
 apDMA[  5 ] = aClr1;
 apDMA[  6 ] = GS_SET_XYZ( lXYZ0.X, lXYZ1.Y, 0 );
 apDMA[  7 ] = aClr0;
 apDMA[  8 ] = GS_SET_XYZ( lXYZ1.X, lXYZ0.Y, 0 );
 apDMA[  9 ] = aClr1;
 apDMA[ 10 ] = lXYZ1.m_Value;
 apDMA[ 11 ] = 0;

}  /* end GSContext_RenderVGRect */
