/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 DivX, Inc.
# Adopted for SMS in 2007 by Eugene Plotnikov
# Licensed (like the original source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_DXSB.h"

#include <malloc.h>
#include <kernel.h>

typedef struct _DXSB_Color {
 unsigned char m_R __attribute__(  ( packed )  );
 unsigned char m_G __attribute__(  ( packed )  );
 unsigned char m_B __attribute__(  ( packed )  );
} _DXSB_Color __attribute__(  ( packed )  );

typedef struct _DXSB_Header {
 unsigned char  m_PTS[ 27 ] __attribute__(  ( packed )  );
 unsigned short m_Width     __attribute__(  ( packed )  );
 unsigned short m_Height    __attribute__(  ( packed )  );
 unsigned short m_Left      __attribute__(  ( packed )  );
 unsigned short m_Top       __attribute__(  ( packed )  );
 unsigned short m_Right     __attribute__(  ( packed )  );
 unsigned short m_Bottom    __attribute__(  ( packed )  );
 unsigned short m_Offset    __attribute__(  ( packed )  );
 _DXSB_Color    m_Clr[ 4 ]  __attribute__(  ( packed )  );
} _DXSB_Header __attribute__(  ( packed )  );

extern void mips_memset ( void*, int, unsigned int         );
extern void dxsb_pack   ( void*, const void*, unsigned int );

extern void _get_pts ( const unsigned char*, s64*  );

static float s_HFactor;
static float s_VFactor;
static int*  s_pSubIdx;
static int   s_Base;
static int   s_Ratio;

void SMS_DXSB_SetRatio ( int aBase, int aRatio ) {

 s_Base  = aBase;
 s_Ratio = aRatio;

}  /* end SMS_DXSB_SetRatio */

void SMS_DXSB_Init ( int aWidth, int aHeight, int* apSubIdx ) {

 s_HFactor = 1.0F / ( float )aWidth;
 s_VFactor = 1.0F / ( float )aHeight;
 s_pSubIdx = apSubIdx;
 s_Base    = 1;
 s_Ratio   = 1;

}  /* end SMS_DXSB_Init */

__asm__(
 ".data\n\t"
 ".align 4\n\t"
 "s_HMS:    .word   0x00000030, 0x00000000, 0x00000030, 0x00000000\n\t"
 "          .word   0x02255100, 0x00000000, 0x000927C0, 0x00000000\n\t"
 "          .word   0x00002710, 0x00000000, 0x00000064, 0x00000000\n\t"
 "          .word   0x0036EE80, 0x00000000, 0x0000EA60, 0x00000000\n\t"
 "          .word   0x000003E8, 0x00000000, 0x0000000A, 0x00000000\n\t"
 ".text\n\t"
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 "_get_pts:\n\t"
 "lui       $a3, %hi( s_HMS )\n\t"
 "addiu     " ASM_REG_T4 ", $zero, 2\n\t"
 "lq        $at, %lo( s_HMS )+ 0($a3)\n\t"
 "lq        $v0, %lo( s_HMS )+16($a3)\n\t"
 "lq        $v1, %lo( s_HMS )+32($a3)\n\t"
 "lq        $a2, %lo( s_HMS )+48($a3)\n\t"
 "lq        $a3, %lo( s_HMS )+64($a3)\n\t"
 "1:\n\t"
 "lbu       " ASM_REG_T0 ",  1($a0)\n\t"
 "lbu       " ASM_REG_T1 ",  4($a0)\n\t"
 "lbu       " ASM_REG_T2 ",  7($a0)\n\t"
 "lbu       " ASM_REG_T3 ", 10($a0)\n\t"
 "pcpyld    " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "pcpyld    " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
 "psubuw    " ASM_REG_T0 ", " ASM_REG_T0 ", $at\n\t"
 "psubuw    " ASM_REG_T2 ", " ASM_REG_T2 ", $at\n\t"
 "pmultuw   $zero, " ASM_REG_T0 ", $v0\n\t"
 "lbu       " ASM_REG_T0 ",  2($a0)\n\t"
 "lbu       " ASM_REG_T1 ",  5($a0)\n\t"
 "lbu       " ASM_REG_T5 ",  8($a0)\n\t"
 "lbu       " ASM_REG_T3 ", 11($a0)\n\t"
 "pmadduw   $zero, " ASM_REG_T2 ", $v1\n\t"
 "pcpyld    " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "pcpyld    " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T5 "\n\t"
 "psubuw    " ASM_REG_T0 ", " ASM_REG_T0 ", $at\n\t"
 "psubuw    " ASM_REG_T2 ", " ASM_REG_T2 ", $at\n\t"
 "pmadduw   $zero, " ASM_REG_T0 ", $a2\n\t"
 "lbu       " ASM_REG_T1 ", 12($a0)\n\t"
 "addiu     " ASM_REG_T4 ", " ASM_REG_T4 ", -1\n\t"
 "pmadduw   " ASM_REG_T0 ", " ASM_REG_T2 ", $a3\n\t"
 "subu      " ASM_REG_T1 ", " ASM_REG_T1 ", $at\n\t"
 "addiu     $a1, $a1, 8\n\t"
 "pcpyud    " ASM_REG_T2 ", " ASM_REG_T0 ", $zero\n\t"
 "addiu     $a0, $a0, 13\n\t"
 "addu      " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
 "addu      " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T1 "\n\t"
 "bgtz      " ASM_REG_T4 ", 1b\n\t"
 "sd        " ASM_REG_T2 ", -8($a1)\n\t"
 "jr        $ra\n\r"
 "dxsb_pack:\n\t"
 "srl       $v1, $a2, 6\n\t"
 "beqz      $v1, 1f\n\t"
 "andi      $a2, $a2, 0x3F\n\t"
 "2:\n\t"
 "ld        " ASM_REG_T0 ",  0($a1)\n\t"
 "ld        " ASM_REG_T1 ",  8($a1)\n\t"
 "addiu     $v1, $v1, -1\n\t"
 "ld        " ASM_REG_T2 ", 16($a1)\n\t"
 "ld        " ASM_REG_T3 ", 24($a1)\n\t"
 "ld        " ASM_REG_T4 ", 32($a1)\n\t"
 "ld        " ASM_REG_T5 ", 40($a1)\n\t"
 "ld        " ASM_REG_T6 ", 48($a1)\n\t"
 "ld        " ASM_REG_T7 ", 56($a1)\n\t"
 "addiu     $a1, $a1, 64\n\t"
 "dsrl      " ASM_REG_T8 ", " ASM_REG_T0 ", 4\n\t"
 "dsrl      " ASM_REG_T9 ", " ASM_REG_T1 ", 4\n\t"
 "or        " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T8 "\n\t"
 "or        " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T9 "\n\t"
 "dsrl      " ASM_REG_T8 ", " ASM_REG_T2 ", 4\n\t"
 "dsrl      " ASM_REG_T9 ", " ASM_REG_T3 ", 4\n\t"
 "or        " ASM_REG_T2 ", " ASM_REG_T2 ", " ASM_REG_T8 "\n\t"
 "or        " ASM_REG_T3 ", " ASM_REG_T3 ", " ASM_REG_T9 "\n\t"
 "dsrl      " ASM_REG_T8 ", " ASM_REG_T4 ", 4\n\t"
 "dsrl      " ASM_REG_T9 ", " ASM_REG_T5 ", 4\n\t"
 "or        " ASM_REG_T4 ", " ASM_REG_T4 ", " ASM_REG_T8 "\n\t"
 "or        " ASM_REG_T5 ", " ASM_REG_T5 ", " ASM_REG_T9 "\n\t"
 "dsrl      " ASM_REG_T8 ", " ASM_REG_T6 ", 4\n\t"
 "dsrl      " ASM_REG_T9 ", " ASM_REG_T7 ", 4\n\t"
 "or        " ASM_REG_T6 ", " ASM_REG_T6 ", " ASM_REG_T8 "\n\t"
 "or        " ASM_REG_T7 ", " ASM_REG_T7 ", " ASM_REG_T9 "\n\t"
 "ppacb     " ASM_REG_T0 ", $zero, " ASM_REG_T0 "\n\t"
 "ppacb     " ASM_REG_T1 ", $zero, " ASM_REG_T1 "\n\t"
 "ppacb     " ASM_REG_T2 ", $zero, " ASM_REG_T2 "\n\t"
 "ppacb     " ASM_REG_T3 ", $zero, " ASM_REG_T3 "\n\t"
 "ppacb     " ASM_REG_T4 ", $zero, " ASM_REG_T4 "\n\t"
 "ppacb     " ASM_REG_T5 ", $zero, " ASM_REG_T5 "\n\t"
 "ppacb     " ASM_REG_T6 ", $zero, " ASM_REG_T6 "\n\t"
 "ppacb     " ASM_REG_T7 ", $zero, " ASM_REG_T7 "\n\t"
 "pextlw    " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "pextlw    " ASM_REG_T2 ", " ASM_REG_T3 ", " ASM_REG_T2 "\n\t"
 "pextlw    " ASM_REG_T4 ", " ASM_REG_T5 ", " ASM_REG_T4 "\n\t"
 "pextlw    " ASM_REG_T6 ", " ASM_REG_T7 ", " ASM_REG_T6 "\n\t"
 "pcpyld    " ASM_REG_T0 ", " ASM_REG_T2 ", " ASM_REG_T0 "\n\t"
 "pcpyld    " ASM_REG_T4 ", " ASM_REG_T6 ", " ASM_REG_T4 "\n\t"
 "sq        " ASM_REG_T0 ",  0($a0)\n\t"
 "sq        " ASM_REG_T4 ", 16($a0)\n\t"
 "bgtz      $v1, 2b\n\t"
 "addiu     $a0, $a0, 32\n\t"
 "1:\n\t"
 "beqz      $a2, 1f\n\t"
 "nop\n\t"
 "2:\n\t"
 "ld        " ASM_REG_T0 ", 0($a1)\n\t"
 "ld        " ASM_REG_T1 ", 8($a1)\n\t"
 "addiu     $a2, $a2, -16\n\t"
 "addiu     $a1, $a1, 16\n\t"
 "dsrl      " ASM_REG_T2 ", " ASM_REG_T0 ", 4\n\t"
 "dsrl      " ASM_REG_T3 ", " ASM_REG_T1 ", 4\n\t"
 "or        " ASM_REG_T0 ", " ASM_REG_T0 ", " ASM_REG_T2 "\n\t"
 "or        " ASM_REG_T1 ", " ASM_REG_T1 ", " ASM_REG_T3 "\n\t"
 "ppacb     " ASM_REG_T0 ", $zero, " ASM_REG_T0 "\n\t"
 "ppacb     " ASM_REG_T1 ", $zero, " ASM_REG_T1 "\n\t"
 "pextlw    " ASM_REG_T0 ", " ASM_REG_T1 ", " ASM_REG_T0 "\n\t"
 "sd        " ASM_REG_T0 ", 0($a0)\n\t"
 "bgtz      $a2, 2b\n\t"
 "addiu     $a0, $a0, 8\n\t"
 "1:\n\t"
 "jr        $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static int inline _nibble ( const unsigned char* apBuf, int anOffset ) {
 return (    apBuf[ anOffset >> 1 ] >> (   (  1 - ( anOffset & 1 )  ) << 2   )    ) & 0xF;
}  /* end _nibble */

static SMS_DXSBFrame* _decode ( void* apData, int aSize ) {

 unsigned int   lW, lH;
 _DXSB_Header*  lpHdr  = ( _DXSB_Header* )apData;
 unsigned int   lRW    = (   (  ( lW = lpHdr -> m_Width  ) + 31  ) & ~31   );
 unsigned int   lRH    = (   (  ( lH = lpHdr -> m_Height ) +  7  ) &  ~7   );
 unsigned int   lSize  = ( lRW >> 1 ) * lRH;
 SMS_DXSBFrame* retVal = ( SMS_DXSBFrame* )memalign (
  64, lSize + sizeof ( SMS_DXSBFrame )
 );

 if ( retVal ) {

  int            i, lIt, lX, lY, lLen;
  unsigned char* lpDst, *lpPixmap, *lpTmp = SMS_SPR_FREE;
  float          lLeft, lTop, lRight, lBottom;

  apData  = (  ( unsigned char* )apData  ) + sizeof ( _DXSB_Header );
  aSize  -= sizeof ( _DXSB_Header );

  _get_pts ( lpHdr -> m_PTS, &retVal -> m_StartPTS );

  lLeft   = lpHdr -> m_Left   * s_HFactor;
  retVal -> m_pPixmap   = (  ( unsigned char* )retVal  ) + sizeof ( SMS_DXSBFrame );
  lRight  = lpHdr -> m_Right  * s_HFactor;
  retVal -> m_Width     = lW;
  lTop    = lpHdr -> m_Top    * s_VFactor;
  retVal -> m_Height    = lH;
  retVal -> m_FrameType = SMS_FT_T_TYPE;
  lBottom = lpHdr -> m_Bottom * s_VFactor;
  retVal -> m_QWCPixmap = lSize >> 4;
  retVal -> m_RWidth    = lRW;
  retVal -> m_RHeight   = lRH;

  retVal -> m_Left   = lLeft;
  retVal -> m_Top    = lTop;
  retVal -> m_Right  = lRight;
  retVal -> m_Bottom = lBottom;

  InvalidDCache ( retVal -> m_pPixmap, retVal -> m_pPixmap + lSize );

  lpDst = lpPixmap = ( unsigned char* )(  ( unsigned int )retVal -> m_pPixmap | 0x30000000  );

  lX      = 0;
  lY      = 0;
  lIt     = 0;
  aSize <<= 1;

  for ( i = 0; i < 2; ++i ) {

   while ( lIt < aSize ) {

    unsigned int lVal  = _nibble ( apData, lIt++ );
    unsigned int lDiff = lW - lX;

    if ( lVal < 4 ) {

     lVal = ( lVal << 4 ) | _nibble ( apData, lIt++ );

     if ( lVal < 16 ) {

      lVal = ( lVal << 4 ) | _nibble ( apData, lIt++ );

      if ( lVal < 64 ) {

       lVal = ( lVal << 4 ) | _nibble ( apData, lIt++ );

       if ( lVal < 4 ) lVal |= lDiff << 2;

      }  /* end if */

     }  /* end if */

    }  /* end if */

    lLen = lVal >> 2;

    if ( lLen > lDiff ) lLen = lDiff;

    mips_memset (  lpTmp + lX, ( lVal & 3 ) + 3, lLen  );

    if (  ( lX += lLen ) >= lW  ) {

     dxsb_pack ( lpDst, lpTmp, lRW );

     if (  ( lY += 2 ) >= lH  ) break;

     lpDst += lRW;
     lX     = 0;
     lIt   += lIt & 1;

    }  /* end if */

   }  /* end for */

   lpDst = lpPixmap + ( lRW >> 1 );
   lY    = 1;
   lX    = 0;
   lIt  += lIt & 1;

  }  /* end for */

 }  /* end if */

 return retVal;

}  /* end _decode */

int SMS_DXSB_Decode ( SMS_AVPacket* apPkt, SMS_RingBuffer* apOutput ) {

 int retVal = 0;

 if ( apPkt -> m_StmIdx == *s_pSubIdx ) {

  SMS_DXSBFrame* lpFrame = _decode ( apPkt -> m_pData, apPkt -> m_Size );

  if ( lpFrame ) {

   SMS_DXSBFrame** lppFrame = ( SMS_DXSBFrame** )SMS_RingBufferAlloc ( apOutput, 4 );

   *lppFrame = lpFrame;

   lpFrame -> m_StartPTS = SMS_Rescale ( lpFrame -> m_StartPTS, s_Base, s_Ratio );
   lpFrame -> m_EndPTS   = SMS_Rescale ( lpFrame -> m_EndPTS,   s_Base, s_Ratio );

   SMS_RingBufferPost ( apOutput );

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end SMS_DXSB_Decode */
