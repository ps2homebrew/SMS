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
#include "SMS_DMA.h"
#include "SMS_GS.h"
#include "SMS_VSync.h"

#include <kernel.h>
#include <malloc.h>

extern short g_MaxSyncVal[ 8 ] __attribute__(   (  section( ".data" )  )   );

unsigned int SMS_EstimateVSync ( int aPixFmt, int aWidth, int aHeight ) {

 unsigned long* lpDMA, *lpPtr;
 unsigned long  lGIFTag, lDMATag;
 unsigned int   lPixFmt;
 unsigned int   lQWC, lX, lY;
 float          lnMB, lMB2HS;

 switch ( aPixFmt ) {
  case 0:
   lPixFmt = GSPixelFormat_PSMCT32;
   lQWC    = 64;
  break;
  case 1:
   lPixFmt = GSPixelFormat_PSMCT16;
   lQWC    = 32;
  break;
  default:
   lPixFmt = GSPixelFormat_PSMCT24;
   lQWC    = 48;
  break;
 }  /* end switch */

 lGIFTag = GIF_TAG( lQWC, 0, 0, 0, 2, 0 );
 lDMATag = DMA_TAG(  lQWC, 0, DMATAG_ID_REF, 0, 0x00100000, 0  );
 lpDMA   = lpPtr = ( unsigned long* )malloc (  ( 8 + 12 * 32 * 32 ) << 3  );

 lpDMA[ 0 ] = DMA_TAG( 3, 0, DMATAG_ID_CNT, 0, 0, 0 );
 lpDMA[ 1 ] = 0LL;
 lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
 lpDMA[ 3 ] = GIFTAG_REGS_AD;
 lpDMA[ 4 ] = GS_SET_TRXREG( 16, 16 );
 lpDMA[ 5 ] = GS_TRXREG;
 lpDMA[ 6 ] = GS_SET_BITBLTBUF( 0, 0, lPixFmt, 0x3FC0, 1, lPixFmt );
 lpDMA[ 7 ] = GS_BITBLTBUF;
 lpDMA     += 8;

 for ( lY = 0; lY < 32; ++lY )
  for ( lX = 0; lX < 32; ++lX ) {
   lpDMA[ 0 ] = DMA_TAG( 4, 0, DMATAG_ID_CNT, 0, 0, 0 );
   lpDMA[ 1 ] = 0LL;
    lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
    lpDMA[ 3 ] = GIFTAG_REGS_AD;
    lpDMA[ 4 ] = GS_SET_TRXPOS( 0, 0, 0, 0, GS_TRXPOS_DIR_LR_UD );
    lpDMA[ 5 ] = GS_TRXPOS;
    lpDMA[ 6 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
    lpDMA[ 7 ] = GS_TRXDIR;
    lpDMA[ 8 ] = lGIFTag;
    lpDMA[ 9 ] = 0LL;
   lpDMA[ 10 ] = lDMATag;
   lpDMA[ 11 ] = 0L;
   lpDMA += 12;
  }  /* end for */

 __asm__(
  ".set noat\n\t"
  "lui  $at, 0x7000\n\t"
  "ori  %1, %1, 0x8000\n\t"
  "nor  $at, $at, $at\n\t"
  "and  %0, %0, $at\n\t"
  ".set at\n\t"
  : "=r"( lDMATag ), "=r"( lGIFTag )
 );

 lpDMA[ -4 ] = lGIFTag;
 lpDMA[ -2 ] = lDMATag;

 FlushCache ( 0 );

 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "1:\n\t"
  "di\n\t"
  "sync.p\n\t"
  "mfc0 $at, $12\n\t"
  "lui  $v0, 0x0001\n\t"
  "and  $at, $at, $v0\n\t"
  "bne  $at, $zero, 1b\n\t"
  "nop\n\t"
  "lui  $at, 0x1200\n\t"
  "or   $v0, $zero, 0x0004\n\t"
  "sd   $v0, 0x1000($at)\n\t"
  "sync.p\n\t"
  "1:\n\t"
  "ld   $v1, 0x1000($at)\n\t"
  "andi $v1, $v1, 4\n\t"
  "beq  $v1, $zero, 1b\n\t"
  "nop\n\t"
  "sd   $v0, 0x1000($at)\n\t"
  "sync.p\n\t"
  "mfc0 $v0, $9\n\t"
  "1:\n\t"
  "ld   $v1, 0x1000($at)\n\t"
  "andi $v1, $v1, 4\n\t"
  "beq  $v1, $zero, 1b\n\t"
  "nop\n\t"
  "mfc0 $v1, $9\n\t"
  "sub  %0, $v1, $v0\n\t"
  "lui  $at, 0x1001\n\t"
  "ori  $v1, $zero, 0x0105\n\t"
  "sw   $zero, -24544($at)\n\t"
  "sw   %2, -24528($at)\n\t"
  "sw   $v1, -24576($at)\n\t"
  "sync.p\n\t"
  "mfc0 $v0, $9\n\t"
  "1:\n\t"
  "lw   $v1, -24576($at)\n\t"
  "andi $v1, $v1, 0x0100\n\t"
  "bne  $v1, $zero, 1b\n\t"
  "nop\n\t"
  "mfc0 $v1, $9\n\t"
  "subu %1, $v1, $v0\n\t"
  "ei\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( lPixFmt ), "=r"( lQWC ): "r"( lpPtr ): "at", "v0", "v1", "a0"
 );

 lnMB   = ( float )(  ( aWidth + 15 ) >> 4  ) * (  ( aHeight + 15 ) >> 4  );
 lMB2HS = ( float )lPixFmt / (  ( float )lQWC / 1024.0F  ) + 0.5F;
 lnMB  /= lMB2HS;
 lnMB  += lnMB / 8.0F;
 lX     = ( unsigned int )( lnMB + 0.5F );

 free ( lpPtr );

 return g_MaxSyncVal[ GS_VMode2Index (  GS_Params () -> m_GSCRTMode  ) ] - lX;

}  /* end SMS_EstimateVSync */
