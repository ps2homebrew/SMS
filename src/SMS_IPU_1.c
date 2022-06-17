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

 unsigned int   lDMASize  = sizeof ( u64           ) * (  8 + 12 * ( aWidth >> 4 )  );
 unsigned int   lDataSize = aWidth << 6;
 u64*           lpDMA     = ( u64*           )malloc ( lDataSize += lDMASize );
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
 apLoadImg -> m_fPal   = 0;
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

void _darken_image ( unsigned char* apBuf, unsigned int aQWC ) {

 __asm__ __volatile__ (
  ".set noreorder\n\t"
  "lui      " ASM_REG_T0 ", 0x00FF\n\t"
  "lui      " ASM_REG_T1 ", 0xFF00\n\t"
  "ori      " ASM_REG_T0 ", " ASM_REG_T0 ", 0xFFFF\n\t"
  "pextlw   " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
  "pextlw   " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
  "pcpyld   " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T1 "\n\t"
  "pcpyld   " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T0 "\n\t"
  "srl      $a1, $a1, 1\n\t"
  "1:\n\t"
  "lq       $v1,  0($a0)\n\t"
  "lq       " ASM_REG_T6 ", 16($a0)\n\t"
  "pand     " ASM_REG_T2 ", $v1, " ASM_REG_T0 "\n\t"
  "pand     " ASM_REG_T3 ", $v1, " ASM_REG_T1 "\n\t"
  "pand     " ASM_REG_T4 ", " ASM_REG_T6 ", " ASM_REG_T0 "\n\t"
  "pand     " ASM_REG_T5 ", " ASM_REG_T6 ", " ASM_REG_T1 "\n\t"
  "psrlw    " ASM_REG_T3 ", " ASM_REG_T3 ", 1\n\t"
  "psrlw    " ASM_REG_T5 ", " ASM_REG_T5 ", 1\n\t"
  "pand     " ASM_REG_T3 ", " ASM_REG_T3 ", " ASM_REG_T1 "\n\t"
  "pand     " ASM_REG_T5 ", " ASM_REG_T5 ", " ASM_REG_T1 "\n\t"
  "por      $v1, " ASM_REG_T2 ", " ASM_REG_T3 "\n\t"
  "por      " ASM_REG_T6 ", " ASM_REG_T4 ", " ASM_REG_T5 "\n\t"
  "subu     $a1, $a1, 1\n\t"
  "sq       $v1,  0($a0)\n\t"
  "sq       " ASM_REG_T6 ", 16($a0)\n\t"
  "bgtz     $a1, 1b\n\t"
  "addiu    $a0, $a0, 32\n\t"
  ".set reorder\n\t"
 );

}  /* end _darken_image */

void IPU_LoadImage ( IPULoadImage* apLoadImg, void* apData, int aSize, int aX, int anY, int afDarken, int aTH0, int aTH1 ) {

 u64*           lpBegin = UNCACHED_SEG( apLoadImg -> m_pDMA + 12 );
 u64*           lpEnd   = UNCACHED_SEG( apLoadImg -> m_pData     );
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

   lCode = IPU_FDEC( 32 );

   if ( lCode == 0x000001B2 ) {

    apLoadImg -> m_fPal = 1;

    for ( i = 0; i < 16; ++i ) apLoadImg -> m_Pal[ i ] = IPU_FDEC( 32 ) | 0x60000000;

    IPU_FDEC( 32 );

   }  /* end if */

   if (  IPU_FDEC( 0 ) == lSlice  ) {

    u64*           lpPos;

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

unsigned short IPU_ImageInfo ( void* apData, unsigned short* apHeight ) {

 unsigned short retVal = 0;

 if (  SMS_unaligned32 ( apData ) == 0xB2010000  ) {

  uint32_t lDim = SMS_bswap32 (    SMS_unaligned32 (   (  ( unsigned char* )apData ) + 4   )    );

  *apHeight = lDim & 0xFFFF;
  retVal    = lDim >> 16;

 }  /* end if */

 return retVal;

}  /* end IPU_ImageInfo */

void IPU_UnpackImage ( void* apDst, void* apSrc, int aSize, int aWidth, int aHeight, int afDarken, int aTH0, int aTH1 ) {

 int lCode, lMBW, lMBH, lMBS;

 DMAC -> m_SQWC = (  ( aWidth - 16 ) >> 2  ) | 0x00040000;

 lMBW = aWidth  >> 4;
 lMBH = aHeight >> 4;
 lMBS = lMBW << 10;

 IPU_RESET();
 IPU -> m_CTRL |= 1 << 23;
 IPU_CMD_SETTH( aTH0, aTH1 );

 DMA_Send (  DMAC_TO_IPU, apSrc, ( aSize + 15 ) >> 4  );

 lCode = IPU_FDEC ( 0 );

 if ( lCode == 0x000001B2 ) {

  lCode = IPU_FDEC ( 32 );

  if (  ( lCode >> 16 ) == aWidth && ( lCode & 0xFFFF ) == aHeight  ) {

   unsigned int lSlice = 0x00000101;
   unsigned int lQSC;

   if (  IPU_FDEC( 32 ) == lSlice  ) {

    char* lpRunDst = apDst;

    lQSC = IPU_FDEC ( 32 ) >> 27;
    ++lSlice;

    while ( lMBH-- ) {

     char* lpDst = ( char* )lpRunDst;

     if (  !IPU_IDEC ( 7, lQSC, 0, 0, 0, 0 )  ) break;

     __asm__ __volatile__(
      ".set noreorder\n\t"
      ".set noat\n\t"
      "move    $v0, %2\n\t"
      "lui     $at, 0x1001\n\t"
      "2:\n\t"
      "addiu   $v0, $v0, -1\n\t"
      "1:\n\t"
      "lw      $ra, -12288($at)\n\t"
      "andi    $ra, 0x100\n\t"
      "bgtz    $ra, 1b\n\t"
      "nop\n\t"
      "lui     $ra, 0x8000\n\t"
      "ori     $ra, 0x3C00\n\t"
      "sw      $ra, -20464($at)\n\t"
      "addiu   $ra, $zero, 64\n\t"
      "sw      $ra, -20448($at)\n\t"
      "addiu   $ra, $zero, 0x0100\n\t"
      "sw      $ra, -20480($at)\n\t"
      "addiu   $ra, $zero, 64\n\t"
      "sw      %0,  -12272($at)\n\t"
      "sw      $ra, -12256($at)\n\t"
      "addiu   $ra, $zero, 0x3C00\n\t"
      "sw      $ra, -12160($at)\n\t"
      "1:\n\t"
      "lw      $ra, -20480($at)\n\t"
      "andi    $ra, 0x0100\n\t"
      "bgtz    $ra, 1b\n\t"
      "nop\n\t"
      "beq     %3, $zero, 1f\n\t"
      "lui     $a0, 0x7000\n\t"
      "ori     $a0, 0x3C00\n\t"
      "jal     _darken_image\n\t"
      "addiu   $a1, $zero, 64\n\t"
      "1:\n\t"
      "addiu   $ra, $zero, 0x108\n\t"
      "sw      $ra, -12288($at)\n\t"
      "bgtz    $v0, 2b\n\t"
      "addiu   %0, %0, 64\n\t"
      ".set reorder\n\t"
      ".set at\n\t"
      : "=r"( lpDst )
      : "0"( lpDst ), "r"( lMBW ), "r"( afDarken )
      : "at", "v0", "v1", "a0", "a1", ASM_REG_T0, ASM_REG_T1, ASM_REG_T2, ASM_REG_T3, ASM_REG_T4, ASM_REG_T5, ASM_REG_T6
     );

     if (  IPU_FDEC ( 0 ) != lSlice++  ) break;

     IPU_FDEC ( 32 );

     lpRunDst = ( char* )lpRunDst + lMBS;

    }  /* end while */

   }  /* end if */

  }  /* end if */

 }  /* end if */

 DMA_Stop ( DMAC_FROM_IPU );
 DMA_Stop ( DMAC_TO_IPU   );
 IPU_RESET();

}  /* end IPU_UnpackImage */
