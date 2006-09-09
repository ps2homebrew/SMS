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
#include "SMS.h"
#include "SMS_IPU.h"
#include "SMS_DMA.h"
#include "SMS_VIF.h"
#include "SMS_GS.h"

#include <kernel.h>
#include <malloc.h>

static void _destroy ( IPULoadImage* apLoadImg ) {

 free ( apLoadImg -> m_pDMA );

}  /* end _destroy */

void IPU_InitLoadImage ( IPULoadImage* apLoadImg, int aWidth, int aHeight ) {

 unsigned int   lDMASize  = sizeof ( unsigned long ) * (  8 + 12 * ( aWidth >> 4 )  );
 unsigned int   lDataSize = aWidth << 6;
 unsigned long* lpDMA     = ( unsigned long* )malloc ( lDataSize += lDMASize );
 unsigned char* lpData    = (  ( unsigned char* )lpDMA  ) + lDMASize;

 lpDMA[ 0 ] = DMA_TAG( 3, 0, DMATAG_ID_CNT, 0, 0, 0 );
 lpDMA[ 1 ] = VIF_DIRECT( 3 );
 lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
 lpDMA[ 3 ] = GIFTAG_REGS_AD;
 lpDMA[ 4 ] = GS_SET_BITBLTBUF( 0, 0, 0, g_GSCtx.m_VRAMTexPtr, g_GSCtx.m_TBW, GSPixelFormat_PSMCT32 );
 lpDMA[ 5 ] = GS_BITBLTBUF;
 lpDMA[ 6 ] = GS_SET_TRXREG( 16, 16 );
 lpDMA[ 7 ] = GS_TRXREG;

 apLoadImg -> m_pDMA   = lpDMA; lpDMA += 8;
 apLoadImg -> m_pData  = lpData;
 apLoadImg -> m_Width  = aWidth;
 apLoadImg -> m_Height = aHeight;
 apLoadImg -> m_QWC    = aWidth << 2;
 apLoadImg -> Destroy  = _destroy;

 for ( aHeight = 0; aHeight < aWidth; aHeight += 16, lpData += 1024, lpDMA += 12 ) {

  lpDMA[  0 ] = DMA_TAG( 4, 0, DMATAG_ID_CNT, 0, 0, 0 );
  lpDMA[  1 ] = VIF_DIRECT( 4 );
  lpDMA[  2 ] = GIF_TAG( 2, 0, 0, 0, GIFTAG_FLG_PACKED, 1 );
  lpDMA[  3 ] = GIFTAG_REGS_AD;
  lpDMA[  5 ] = GS_TRXPOS;
  lpDMA[  6 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
  lpDMA[  7 ] = GS_TRXDIR;
  lpDMA[  8 ] = GIF_TAG( 64, 0, 0, 0, GIFTAG_FLG_IMAGE, 1 );
  lpDMA[  9 ] = 0;
  lpDMA[ 10 ] = DMA_TAG( 64, 0, DMATAG_ID_REF, 0, lpData, 0 );
  lpDMA[ 11 ] = VIF_DIRECT( 64 );

 }  /* end for */

 (  ( GIFTag* )( lpDMA - 4 )  ) -> EOP = 1;
 (  ( DMATag* )( lpDMA - 2 )  ) -> ID  = DMATAG_ID_REFE;

 SyncDCache ( apLoadImg -> m_pDMA, apLoadImg -> m_pDMA + lDataSize );

}  /* end IPU_InitLoadImage */

static void _darken_image ( unsigned char* apBuf, unsigned int aQWC ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "lui      $t0, 0x00FF\n\t"
  "lui      $t1, 0xFF00\n\t"
  "ori      $t0, $t0, 0xFFFF\n\t"
  "pextlw   $t1, $t1\n\t"
  "pextlw   $t0, $t0\n\t"
  "pcpyld   $t1, $t1\n\t"
  "pcpyld   $t0, $t0\n\t"
  "srl      $a1, $a1, 1\n\t"
  "1:\n\t"
  "lq       $v1,  0($a0)\n\t"
  "lq       $t6, 16($a0)\n\t"
  "pand     $t2, $v1, $t0\n\t"
  "pand     $t3, $v1, $t1\n\t"
  "pand     $t4, $t6, $t0\n\t"
  "pand     $t5, $t6, $t1\n\t"
  "psrlw    $t3, $t3, 1\n\t"
  "psrlw    $t5, $t5, 1\n\t"
  "pand     $t3, $t3, $t1\n\t"
  "pand     $t5, $t5, $t1\n\t"
  "por      $v1, $t2, $t3\n\t"
  "por      $t6, $t4, $t5\n\t"
  "subu     $a1, $a1, 1\n\t"
  "sq       $v1,  0($a0)\n\t"
  "sq       $t6, 16($a0)\n\t"
  "bgtz     $a1, 1b\n\t"
  "addiu    $a0, $a0, 32\n\t"
  ".set reorder\n\t"
 );

}  /* end _darken_image */

void IPU_LoadImage ( IPULoadImage* apLoadImg, void* apData, int aSize, int aX, int anY, int afDarken, int aTH0, int aTH1 ) {

 unsigned long* lpBegin = UNCACHED_SEG( apLoadImg -> m_pDMA + 12 );
 unsigned long* lpEnd   = UNCACHED_SEG( apLoadImg -> m_pData     );
 unsigned int   lH      = apLoadImg -> m_Height + anY;
 unsigned int   i, lCode;

 SyncDCache (  apData, (  ( char* )apData  ) + aSize  );

 IPU_RESET();
 IPU -> m_CTRL |= 1 << 23;
 IPU_CMD_SETTH( aTH0, aTH1 );

 DMA_Send (  DMAC_TO_IPU, apData, ( aSize + 15 ) >> 4  );

 lCode = IPU_FDEC ( 0 );

 if ( lCode == 0x000001B2 ) {

  lCode = IPU_FDEC ( 32 );

  if (  ( lCode >> 16    ) == apLoadImg -> m_Width &&
        ( lCode & 0xFFFF ) == apLoadImg -> m_Height
  ) {

   unsigned int lSlice = 0x00000101;
   unsigned int lQSC;

   if (  IPU_FDEC( 32 ) == lSlice  ) {

    unsigned long* lpPos;

    lQSC = IPU_FDEC( 32 ) >> 27;
    ++lSlice;

    for ( i = anY; i < lH; i += 16 ) {

     unsigned int j = aX;

     if (  !IPU_IDEC ( 7, lQSC, 0, 0, 0, 0 )  ) break;

     DMA_RecvA ( DMAC_FROM_IPU, apLoadImg -> m_pData, apLoadImg -> m_QWC );

     for ( lpPos = lpBegin; lpPos < lpEnd; lpPos += 12, j += 16 ) *lpPos = GS_SET_TRXPOS( 0, 0, j, i, 0 );

     DMA_Wait ( DMAC_FROM_IPU );

     if ( afDarken ) _darken_image (  UNCACHED_SEG( apLoadImg -> m_pData ), apLoadImg -> m_QWC  );

     DMA_SendChainT ( DMAC_VIF1, apLoadImg -> m_pDMA );

     if (  IPU_FDEC ( 0 ) != lSlice++  ) break;

     IPU_FDEC ( 32 );
     DMA_Wait ( DMAC_VIF1 );

    }  /* end for */

   }  /* end if */

  }  /* end if */

 }  /* end if */

 DMA_Stop ( DMAC_FROM_IPU );
 DMA_Stop ( DMAC_TO_IPU   );
 IPU_RESET();

}  /* end IPU_LoadImage */

unsigned int IPU_ImageInfo ( void* apData, unsigned int* apHeight ) {

 unsigned int retVal = 0;

 if (  SMS_unaligned32 ( apData ) == 0xB2010000  ) {

  uint32_t lDim = SMS_bswap32 (    SMS_unaligned32 (   (  ( unsigned char* )apData ) + 4   )    );

  *apHeight = lDim & 0xFFFF;
  retVal    = lDim >> 16;

 }  /* end if */

 return retVal;

}  /* end IPU_ImageInfo */
