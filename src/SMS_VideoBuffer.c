/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_VideoBuffer.h"
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_IPU.h"

#include <kernel.h>
#include <malloc.h>
#include <string.h>

int _csc_handler ( int, void*, void* );

SMS_VideoBuffer* SMS_VideoBufferInit ( int aWidth, int aHeight ) {

 unsigned short   i, lMBW, lMBH;
 unsigned int     lFrameSize, lFrameStride;
 unsigned int     lnBlk, lStride, lQWC, lDMASize;
 unsigned int     l16Msk = aHeight >> 30;
 unsigned int     lShift = l16Msk & 1;
 SMS_VideoBuffer* retVal;

 if ( aHeight < 0 ) aHeight = -aHeight;

 lMBW = (  ( aWidth  + 15 ) >> 4  ) + 2;
 lMBH = (  ( aHeight + 15 ) >> 4  ) + 2;

 lFrameSize   = lMBW * lMBH * sizeof ( SMS_MacroBlock );
 lFrameStride = lMBW + 1;
 l16Msk     <<= 30;
 lMBH        -= 2;
 lMBW        -= 2;
 l16Msk     >>= 4;
 lnBlk        = lMBW * lMBH;
 lStride      = lMBW * 384;
 lQWC         = lStride >> 4;
 lStride     += 768;
 lDMASize     = lMBH * 16;
 lMBH       <<= 1;

 retVal = ( SMS_VideoBuffer* )calloc (  1, sizeof ( SMS_VideoBuffer )  );

 retVal -> m_CSCParam.m_CSCmd     = 0x70000000 | l16Msk;
 retVal -> m_CSCParam.m_nBlk[ 0 ] = lnBlk / 1023;
 retVal -> m_CSCParam.m_nRem[ 0 ] = lnBlk % 1023;
 retVal -> m_CSCParam.m_QWCB      = 1023                             * ( 64 >> lShift );
 retVal -> m_CSCParam.m_QWCR      = retVal -> m_CSCParam.m_nRem[ 0 ] * ( 64 >> lShift );

 for ( i = 0; i < 3; ++i ) {

  SMS_MacroBlock* lpBase   = ( SMS_MacroBlock* )memalign ( 128, lFrameSize + lDMASize );
  u64*            lpDMA    = ( u64*           )(   (  ( char* )lpBase  ) + lFrameSize   );
  unsigned char*  lpSrc    = ( unsigned char* )( lpBase + lFrameStride );
  u64*            lpDMAEnd = lpDMA + lMBH;

  memset ( lpBase, 0, lFrameSize );

  retVal -> m_VFrm[ i ].m_pBase   = lpBase;
  retVal -> m_VFrm[ i ].m_pData   = ( SMS_MacroBlock* )lpSrc;
  retVal -> m_VFrm[ i ].m_pCSCDma = lpDMA;
  retVal -> m_VFrm[ i ].m_pNext   = &retVal -> m_VFrm[ i + 1 ];

  while ( lpDMA != lpDMAEnd ) {

   lpDMA[ 0 ] = DMA_TAG( lQWC, 0, DMATAG_ID_REF, 0, lpSrc, 0 );
   lpDMA[ 1 ] = 0L;
   lpSrc += lStride;
   lpDMA += 2;

  }  /* end while */

  lpDMA[ -2 ] = DMA_TAG( lQWC, 0, DMATAG_ID_REFE, 0, lpSrc - lStride, 0 );

 }  /* end for */

 retVal -> m_VFrm[ 2 ].m_pNext = NULL;
 retVal -> m_pFree             = &retVal -> m_VFrm[ 0 ];

 retVal -> m_CSCParam.m_HandlerID = AddDmacHandler2 (
  3, _csc_handler, 0, &retVal -> m_CSCParam
 );

 return retVal;

}  /* end SMS_VideoBufferInit */

void SMS_VideoBufferDestroy ( SMS_VideoBuffer* apBuf ) {

 int i;

 for ( i = 0; i < 3; ++i ) free ( apBuf -> m_VFrm[ i ].m_pBase );

 DisableDmac ( 3 );
 RemoveDmacHandler ( 3, apBuf -> m_CSCParam.m_HandlerID );

 free ( apBuf );

}  /* end SMS_VideoBufferDestroy */

void SMS_CSCResume ( void );
__asm__(
 ".set noreorder\n\t"
 ".set noat\n\t"
 ".globl SMS_CSCResume\n\t"
 ".text\n\t"
 "SMS_CSCResume:\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "jal       IPU_FRST\n\t"
 "nop\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 "j         EnableDmac\n\t"
 "or        $a0, $zero, 3\n\t"
 ".set at\n\t"
 ".set reorder\n\t"
);

void SMS_CSCSuspend ( void );
__asm__(
 ".set noreorder\n\t"
 ".set noat\n\t"
 ".globl SMS_CSCSuspend\n\t"
 ".text\n\t"
 "SMS_CSCSuspend:\n\t"
 "j     DisableDmac\n\t"
 "or    $a0, $zero, 3\n\t"
 ".set at\n\t"
 ".set reorder\n\t"
);

void SMS_CSCSync ( void );
__asm__(
 ".set noreorder\n\t"
 ".set noat\n\t"
 ".globl SMS_CSCSync\n\t"
 ".text\n\t"
 "SMS_CSCSync:\n\t"
 "_sms_cscsyn:\n\t"
 "lui   $at, 0x1001\n\t"
 "1:\n\t"
 "lw    $v0, -19456($at)\n\t"   // sync D4
 "lui   $v1, 0x1000\n\t"
 "andi  $v0, $v0, 0x0100\n\t"
 "nop\n\t"
 "nop\n\t"
 "bne   $v0, $zero, 1b\n\t"
 "nop\n\t"
 "1:\n\t"
 "ld    $v0, 0x2000($v1)\n\t"   // sync IPU
 "bltz  $v0, 1b\n\t"
 "nop\n\t"
 "1:\n\t"
 "lw    $v0, -20480($at)\n\t"   // sync D3
 "andi  $v0, $v0, 0x0100\n\t"
 "bne   $v0, $zero, 1b\n\t"
 "nop\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set reorder\n\t"
);

void SMS_CSC ( SMS_CSCParam*, u64*          , void* );
__asm__(                        // a0 = CSC parameters
 ".set noreorder\n\t"           // a1 = DMA packet
 ".set nomacro\n\t"             // a2 = RGB data
 ".set noat\n\t"
 ".globl SMS_CSC\n\t"
 ".text\n\t"
 "SMS_CSC:\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "bgezal    $zero, _sms_cscsyn\n\t"
 "lh        $a3,  0($a0)\n\t"       // a3 = number of slices
 "lh        $t1,  4($a0)\n\t"       // t1 = remainder
 "pcpyud    $ra, $ra, $ra\n\t"
 "sh        $a3,  2($a0)\n\t"       // s_nBlk[ 1 ] = a3
 "sh        $t1,  6($a0)\n\t"       // s_nRem[ 1 ] = t1
 "beql      $a3, $zero, 1f\n\t"     // if ( number of slices == 0 ) goto 1f
 "lh        $v0, 10($a0)\n\t"       // v0 = lQWC
 "addiu     $a3, $a3, -1\n\t"
 "lh        $v0,  8($a0)\n\t"       // v0 = lQWC
 "sh        $a3,  2($a0)\n\t"       // s_nBlk[ 1 ] -= 1
 "beq       $zero, $zero, 2f\n\t"
 "addiu     $v1, $zero, 1023\n\t"   // v1 = number of macroblocks (1023)
 "1:\n\t"
 "addu      $v1, $zero, $t1\n\t"    // v1 = number of macroblocks (remainder)
 "sh        $zero, 6($a0)\n\t"      // no more data
 "2:\n\t"
 "lw        $t0, 12($a0)\n\t"
 "addiu     $a3, $zero, 0x0100\n\t"
 "addiu     $t1, $zero, 0x0105\n\t"
 "sw        $a2, -20464($at)\n\t"   // D3_MADR = a2
 "sw        $v0, -20448($at)\n\t"   // D3_QWC  = v0
 "sw        $a3, -20480($at)\n\t"   // start D3
 "sw        $zero, -19424($at)\n\t" // D4_QWC  = 0
 "sw        $a1, -19408($at)\n\t"   // D4_TADR = a1
 "sw        $t1, -19456($at)\n\t"   // start D4
 "or        $t0, $t0, $v1\n\t"
 "lui       $at, 0x1000\n\t"
 "jr        $ra\n\t"
 "sd        $t0, 0x2000($at)\n\t"   // start IPU CSC
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

int _csc_handler ( int, void*, void* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_csc_handler:\n\t"
 "lh    $a2,  2($a1)\n\t"       // a2 = number of slices
 "lui   $at, 0x1001\n\t"
 "beql  $a2, $zero, 1f\n\t"
 "lh    $a2,  6($a1)\n\t"       // a2 = remainder
 "addiu $a2, $a2, -1\n\t"
 "sh    $a2,  2($a1)\n\t"       // s_nBlk[ 1 ] -= 1
 "lh    $v0,  8($a1)\n\t"       // v0 = QWCB
 "beq   $zero, $zero, 2f\n\t"
 "addiu $v1, $zero, 1023\n\t"   // v1 = number of macroblocks (1023)
 "1:\n\t"
 "beq   $a2, $zero, 3f\n\t"
 "addu  $v1, $zero, $a2\n\t"    // v1 = number of macroblocks (remainder)
 "lh    $v0, 10($a1)\n\t"       // v0 = QWCR
 "sh    $zero, 6($a1)\n\t"      // no more data
 "2:\n\t"
 "lw    $t0,  12($a1)\n"        // t0 = CSC command
 "ori   $a2, $zero, 0x0100\n\t"
 "sw    $v0, -20448($at)\n\t"   // D3_QWC = v0
 "sw    $a2, -20480($at)\n\t"   // start D3
 "or    $t0, $t0, $v1\n\t"
 "lui   $at, 0x1000\n\t"
 "sd    $t0, 0x2000($at)\n\t"   // start IPU CSC
 "3:\n\t"
 "jr    $ra\n\t"
 "xor   $v0, $v0, $v0\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void SMS_FrameBufferInit ( SMS_FrameBuffer* apBuf, int anBuf, int aWidth, int aHeight, int af16 ) {

 unsigned int   i, lSize;
 unsigned int   lMBW, lMBH;
 unsigned int   lQWC, lPixFmt, lIncr;
 u64            lBitBltBuf;
 u64            lGIFTag;
 u64*           lpDMA;

 aWidth  = ( aWidth  + 15 ) & ~15;
 aHeight = ( aHeight + 15 ) & ~15;

 lMBW = aWidth  >> 4;
 lMBH = aHeight >> 4;

 lSize = aWidth * aHeight;

 if ( af16 ) {
  lQWC    = 32;
  lPixFmt = GSPixelFormat_PSMCT16;
  lIncr   = 512;
  lSize <<= 1;
 } else {
  lQWC    = 64;
  lPixFmt = GSPixelFormat_PSMCT32;
  lIncr   = 1024;
  lSize <<= 2;
 }  /* end if */

 lBitBltBuf = GS_SET_BITBLTBUF( 0, 0, lPixFmt, g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, lPixFmt );
 lGIFTag    = GIF_TAG( lQWC, 0, 0, 0, 2, 0 );

 for ( i = 0; i < anBuf; ++i ) {

  int            lX, lY;
  unsigned char* lpPic = ( unsigned char* )memalign (   128, lSize + (  ( 10 + 12 * lMBW * lMBH ) << 3  )   );

  apBuf[ i ].m_pBase     = ( SMS_MacroBlock* )lpPic;
  apBuf[ i ].m_pData     = ( SMS_MacroBlock* )(  lpDMA = ( u64*           )( lpPic + lSize )  );
  apBuf[ i ].m_FrameType = -1;

  lpDMA[ 0 ] = DMA_TAG( 3, 0, DMATAG_ID_CNT, 0, 0, 0 );
  lpDMA[ 1 ] = 0LL;
  lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
  lpDMA[ 3 ] = GIFTAG_REGS_AD;
  lpDMA[ 4 ] = GS_SET_TRXREG( 16, 16 );
  lpDMA[ 5 ] = GS_TRXREG;
  lpDMA[ 6 ] = lBitBltBuf;
  lpDMA[ 7 ] = GS_BITBLTBUF;
  lpDMA     += 8;

  for ( lY = 0; lY < aHeight; lY += 16 )
   for ( lX = 0; lX < aWidth; lX += 16 ) {
    lpDMA[ 0 ] = DMA_TAG( 4, 0, DMATAG_ID_CNT, 0, 0, 0 );
    lpDMA[ 1 ] = 0LL;
     lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
     lpDMA[ 3 ] = GIFTAG_REGS_AD;
     lpDMA[ 4 ] = GS_SET_TRXPOS( 0, 0, lX, lY, GS_TRXPOS_DIR_LR_UD );
     lpDMA[ 5 ] = GS_TRXPOS;
     lpDMA[ 6 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
     lpDMA[ 7 ] = GS_TRXDIR;
     lpDMA[ 8 ] = lGIFTag;
     lpDMA[ 9 ] = 0LL;
    lpDMA[ 10 ] = DMA_TAG(  lQWC, 0, DMATAG_ID_REF, 0, ( unsigned int )lpPic, 0  );
    lpDMA[ 11 ] = 0L;
    lpDMA +=    12;
    lpPic += lIncr;
   }  /* end for */

  lpDMA[ 0 ] = DMA_TAG( 0, 0, DMATAG_ID_RET, 0, 0, 0 );
  lpDMA[ 1 ] = 0L;

 }  /* end for */

}  /* end SMS_FrameBufferInit */

void SMS_FrameBufferDestroy ( SMS_FrameBuffer* apBuf, int anBuf ) {

 int i;

 for ( i = 0; i < anBuf; ++i )
  if ( apBuf[ i ].m_pBase ) {
   free ( apBuf[ i ].m_pBase );
   apBuf[ i ].m_pBase = NULL;
  }  /* end for */

}  /* end SMS_FrameBufferDestroy */

