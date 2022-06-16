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
#include "SMS_Rescale.h"

#include <string.h>
#include <malloc.h>

static void _precalc_x ( SMS_RescaleContext* apCtx ) {

 float        lScale = 1.0F / apCtx -> m_Scale;
 int          lXMax  = apCtx -> m_Width - 1;
 _res_param*  lpIt   = apCtx -> m_XParam;
 _res_param*  lpEnd  = apCtx -> m_XParam + apCtx -> m_NewWidth;
 float        lX     = 0.0F;

 while ( lpIt != lpEnd ) {

  float lfX  = lX * lScale;
  int   liX  = ( int )lfX;
  int   liX1 = liX + 1;
  float lDX  = lfX - liX;

  lX += 1.0F;

  if ( liX1 > lXMax ) liX1 = lXMax;

  lpIt -> m_iDelta[ 0 ] = liX  * 4;
  lpIt -> m_iDelta[ 1 ] = liX1 * 4;
  lpIt -> m_fDelta[ 0 ] = lDX;
  lpIt -> m_fDelta[ 1 ] = 1.0F - lDX;

  lpIt += 1;

 }  /* end while */

}  /* end _precalc_x */

static void _precalc_y ( SMS_RescaleContext* apCtx ) {

 float        lScale = 1.0F / apCtx -> m_Scale;
 int          lYMax  = apCtx -> m_Height - 1;
 _res_param*  lpIt   = apCtx -> m_YParam;
 _res_param*  lpEnd  = apCtx -> m_YParam + apCtx -> m_NewHeight;
 float        lY     = 0.0F;

 while ( lpIt != lpEnd ) {

  float lfY  = lY * lScale;
  int   liY  = ( int )lfY;
  int   liY1 = liY + 1;
  float lDY  = lfY - liY;

  if ( liY1 > lYMax ) liY1 = lYMax;

  lY += 1.0F;

  lpIt -> m_iDelta[ 0 ] = liY;
  lpIt -> m_iDelta[ 1 ] = liY1;
  lpIt -> m_fDelta[ 0 ] = lDY;
  lpIt -> m_fDelta[ 1 ] = 1.0F - lDY;

  lpIt += 1;

 }  /* end while */

}  /* end _precalc_y */

void _rescale_stripe ( struct SMS_RescaleContext* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_rescale_stripe:\n\t"
 "lw    $a2, 12($a0)\n\t"
 "lhu   $a3, 22($a0)\n\t"
 "xor   $at, $at, $at\n\t"
 "sll   $a3, $a3, 2\n\t"
 "lhu   $t1, 20($a0)\n\t"
 "lw    $t0, 40($a0)\n\t"
 "mult  $t1, $t1, $a3\n\t"
 "lw    $a1, 44($a0)\n\t"
 "lw    $v0, 52($a0)\n\t"
 "or    $t2, $zero, $a1\n\t"
 "addu  $t1, $t1, $t0\n\t"
 "lw    $v1, 56($a0)\n\t"
 "addiu $t5, $a0, 68\n\t"
 "4:\n\r"
 "beql  $t0, $t1, 1f\n\t"
 "mult  $at, $at, $a2\n\t"
 "lw    $t3, 0($v1)\n\t"
 "lw    $t4, 4($v1)\n\t"
 "beq   $t3, $v0, 5f\n\t"
 "sll   $t3, $a2, 4\n\t"
 "bne   $t4, $v0, 2f\n\t"
 "5:\n\t"
 "addu  $t6, $zero, $t5\n\t"
 "addu  $t7, $t3, $t5\n\t"
 "addiu $at, $at, 1\n\t"
 "beql  $t4, $v0, 3f\n\t"
 "addiu $v1, $v1, 16\n\t"
 "3:\n\t"
 "lw    $t3,  0($t6)\n\t"
 "lw    $t4,  4($t6)\n\t"
 "lw    $t8,  8($t6)\n\t"
 "lw    $t9, 12($t6)\n\t"
 "addu  $t3, $t3, $t0\n\t"
 "addu  $t4, $t4, $t0\n\t"
 "lw    $t3, 0($t3)\n\t"
 "lw    $t4, 0($t4)\n\t"
 "pextlw    $t8, $t8, $t8\n\t"
 "pextlw    $t9, $t9, $t9\n\t"
 "pextlw    $t8, $t8, $t8\n\t"
 "pextlw    $t9, $t9, $t9\n\t"
 "qmtc2 $t8, $vf3\n\t"
 "qmtc2 $t9, $vf4\n\t"
 "pextlb    $t3, $zero, $t3\n\t"
 "pextlb    $t4, $zero, $t4\n\t"
 "pextlh    $t3, $zero, $t3\n\t"
 "pextlh    $t4, $zero, $t4\n\t"
 "qmtc2 $t3, $vf1\n\t"
 "qmtc2 $t4, $vf2\n\t"
 "vitof0.xyzw   $vf1, $vf1\n\t"
 "vitof0.xyzw   $vf2, $vf2\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf1, $vf4\n\t"
 "vmadd.xyzw    $vf1, $vf2, $vf3\n\t"
 "addiu $t6, $t6, 16\n\t"
 "vftoi0.xyzw   $vf1, $vf1\n\t"
 "addiu $a1, $a1, 4\n\t"
 "qmfc2 $t3, $vf1\n\t"
 "ppach $t3, $t3, $t3\n\t"
 "ppacb $t3, $t3, $t3\n\t"
 "bne   $t6, $t7, 3b\n\t"
 "sw    $t3, -4($a1)\n\t"
 "2:\n\t"
 "addu  $t0, $t0, $a3\n\t"
 "beq   $zero, $zero, 4b\n\t"
 "addiu $v0, $v0, 1\n\t"
 "1:\n\t"
 "sw    $v0, 52($a0)\n\t"
 "sw    $v1, 56($a0)\n\t"
 "sll   $at, $at, 2\n\t"
 "addu  $t2, $t2, $at\n\t"
 "jr    $ra\n\t"
 "sw    $t2, 44($a0)\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _rescale_buffer ( struct SMS_RescaleContext*, void* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_rescale_buffer:\n\t"
 "lw        $at, 12($a0)\n\t"
 "lw        $a2, 16($a0)\n\t"
 "lw        $v0, 48($a0)\n\t"
 "addiu     $v1, $a0, 16452\n\t"
 "sll       $at, $at, 2\n\t"
 "sll       $a2, $a2, 4\n\t"
 "addu      $a3, $v0, $at\n\t"
 "addu      $a2, $a2, $v1\n\t"
 "1:\n\t"
 "lw        $t3,  4($v1)\n\t"
 "lw        $t0,  8($v1)\n\t"
 "lw        $t1, 12($v1)\n\t"
 "addu      $t2, $v0, $at\n\t"
 "addiu     $v1, $v1, 16\n\t"
 "lw        $t4,  0($v1)\n\t"
 "pextlw    $t0, $t0, $t0\n\t"
 "pextlw    $t1, $t1, $t1\n\t"
 "pextlw    $t0, $t0, $t0\n\t"
 "pextlw    $t1, $t1, $t1\n\t"
 "qmtc2     $t0, $vf1\n\t"
 "qmtc2     $t1, $vf2\n\t"
 "subu      $t4, $t4, $t3\n\t"
 "2:\n\t"
 "lw        $t0, 0($v0)\n\t"
 "lw        $t1, 0($a3)\n\t"
 "pextlb    $t0, $zero, $t0\n\t"
 "pextlb    $t1, $zero, $t1\n\t"
 "pextlh    $t0, $zero, $t0\n\t"
 "pextlh    $t1, $zero, $t1\n\t"
 "qmtc2     $t0, $vf3\n\t"
 "qmtc2     $t1, $vf4\n\t"
 "vitof0.xyzw   $vf3, $vf3\n\t"
 "vitof0.xyzw   $vf4, $vf4\n\t"
 "vmula.xyzw    " ASM_REG_ACC ", $vf3, $vf2\n\t"
 "vmadd.xyzw    $vf3, $vf4, $vf1\n\t"
 "addiu     $v0, $v0, 4\n\t"
 "addiu     $a3, $a3, 4\n\t"
 "vftoi0.xyzw   $vf3, $vf3\n\t"
 "addiu     $a1, $a1, 3\n\t"
 "qmfc2     $t0, $vf3\n\t"
 "ppach     $t0, $t0, $t0\n\t"
 "ppacb     $t0, $t0, $t0\n\t"
 "swl       $t0,  0($a1)\n\t"
 "bne       $v0, $t2, 2b\n\t"
 "swr       $t0, -3($a1)\n\t"
 "movn      $t4, $at, $t4\n\t"
 "addu      $v0, $v0, $t4\n\t"
 "bne       $v1, $a2, 1b\n\t"
 "addu      $a3, $a3, $t4\n\t"
 "jr        $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void _copy_stripe ( SMS_RescaleContext* apCtx ) {

 int lnBytes = apCtx -> m_StripeH * apCtx -> m_Stride * 4;

 apCtx -> m_pStripePtr = ( unsigned int* )apCtx -> m_pStripePtr + ( lnBytes >> 2 );

}  /* end _copy_stripe */

static void _copy_buffer ( struct SMS_RescaleContext* apCtx, void* apDst ) {

 unsigned char* lpDst  = ( unsigned char* )apDst;
 unsigned char* lpSrc  = ( unsigned char* )apCtx -> m_pTemp;
 unsigned int   lDIncr = apCtx -> m_NewWidth;
 unsigned int   lSIncr = apCtx -> m_Stride * 4;
 unsigned int   lH     = apCtx -> m_NewHeight;

 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "1:\n\t"
  "or       $at, $zero, %2\n\t"
  "or       $t8, $zero, %1\n\t"
  "2:\n\t"
  "lw       $t9, 0($t8)\n\t"
  "addiu    $t8, $t8, 4\n\t"
  "addiu    %0, %0, 3\n\t"
  "addiu    $at, $at, -1\n\t"
  "swl      $t9,  0(%0)\n\t"
  "bgtz     $at, 2b\n\t"
  "swr      $t9, -3(%0)\n\t"
  "addiu    %4, %4, -1\n\t"
  "bgtz     %4, 1b\n\t"
  "addu     %1, %1, %3\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( lpDst ), "r"( lpSrc ), "r"( lDIncr ), "r"( lSIncr ), "r"( lH ) : "at", "t8", "t9"
 );

}  /* end _copy_buffer */

SMS_RescaleContext* SMS_RescaleInit ( SMS_RescaleContext* apCtx, int aWidth, int aHeight, int aStripeH, int aStripeW ) {

 float lScale;
 int   lAllocSize, lStripeAllocSize;
 int   lMax  = aWidth > aHeight ? aWidth : aHeight;
 int   lMaxT = 1024;
 int   lWidth, lHeight;
 int   lPrevW, lPrevH;
 int   lStride, lStripeH;

 if ( !apCtx ) apCtx = ( SMS_RescaleContext* )calloc (  1, sizeof ( SMS_RescaleContext )  );

 lPrevW = apCtx -> m_Width;
 lPrevH = apCtx -> m_Height;

 lStripeH = aStripeH - 1;
 aStripeW = aStripeW - 1;

 apCtx -> m_Width    = aWidth;
 apCtx -> m_Height   = aHeight;
 apCtx -> m_Stride   = lStride = ( aWidth + aStripeW ) & ~aStripeW;
 apCtx -> m_nStripes = (  ( aHeight + lStripeH ) & ~lStripeH  ) / aStripeH;
 apCtx -> m_StripeH  = aStripeH;

 lStripeAllocSize = lStride * aStripeH * 4;

 while ( 1 ) {

  if ( lMax > lMaxT ) {

   lScale     = ( float )lMaxT / lMax;
   lWidth     = ( int )( aWidth  * lScale + 0.5F );
   lHeight    = ( int )( aHeight * lScale + 0.5F );
   lAllocSize = lWidth * lHeight * 8;

  } else {

   lScale     = 1.0F;
   lWidth     = aWidth;
   lHeight    = aHeight;
   lAllocSize = lStride * ( aStripeH * apCtx -> m_nStripes ) * 4;

  }  /* end else */

  if ( 0x100000 - lWidth * lHeight >= 0xFC00 ) break;

  lMaxT -= 1;

 }  /* end while */

 apCtx -> m_NewWidth  = lWidth;
 apCtx -> m_NewHeight = lHeight;

 if (  lAllocSize > ( int )apCtx -> m_nAlloc  ) {

  apCtx -> m_nAlloc = lAllocSize;
  apCtx -> m_pTemp  = realloc ( apCtx -> m_pTemp, lAllocSize );

 }  /* end if */

 if (  lStripeAllocSize > ( int )apCtx -> m_nStripeAlloc  ) {

  apCtx -> m_nStripeAlloc = lStripeAllocSize;
  apCtx -> m_pStripe      = realloc ( apCtx -> m_pStripe, lStripeAllocSize );

 }  /* end if */

 apCtx -> m_Scale = lScale;

 if ( lScale != 1.0F ) {

  apCtx -> m_Y     = 0;
  apCtx -> m_pCurY = &apCtx -> m_YParam[ 0 ];

  if ( aWidth != lPrevW || aHeight != lPrevH ) {
   _precalc_y ( apCtx );
   _precalc_x ( apCtx );
  }  /* end if */

  apCtx -> ProcessStripe = _rescale_stripe;
  apCtx -> ProcessBuffer = _rescale_buffer;

  apCtx -> m_pStripePtr = apCtx -> m_pStripe;

 } else {

  apCtx -> ProcessStripe = _copy_stripe;
  apCtx -> ProcessBuffer = _copy_buffer;

  apCtx -> m_pStripePtr = apCtx -> m_pTemp;

 }  /* end else */

 apCtx -> m_pTempPtr = apCtx -> m_pTemp;

 return apCtx;

}  /* end SMS_RescaleInit */

void SMS_RescaleDestroy ( SMS_RescaleContext* apCtx ) {

 if ( !apCtx ) return;

 free ( apCtx -> m_pStripe );
 free ( apCtx -> m_pTemp   );
 free ( apCtx );

}  /* end SMS_RescaleDestroy */
