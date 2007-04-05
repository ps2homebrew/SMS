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

extern void mips_memset ( void*, int, unsigned int );

extern void _get_pts ( const unsigned char*, long* );

static float s_HFactor;
static float s_VFactor;
static int*  s_pSubIdx;

void SMS_DXSB_Init ( int aWidth, int aHeight, int* apSubIdx ) {

 s_HFactor = 1.0F / ( float )aWidth;
 s_VFactor = 1.0F / ( float )aHeight;
 s_pSubIdx = apSubIdx;

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
 "addiu     $t4, $zero, 2\n\t"
 "lq        $at, %lo( s_HMS )+ 0($a3)\n\t"
 "lq        $v0, %lo( s_HMS )+16($a3)\n\t"
 "lq        $v1, %lo( s_HMS )+32($a3)\n\t"
 "lq        $a2, %lo( s_HMS )+48($a3)\n\t"
 "lq        $a3, %lo( s_HMS )+64($a3)\n\t"
 "1:\n\t"
 "lbu       $t0,  1($a0)\n\t"
 "lbu       $t1,  4($a0)\n\t"
 "lbu       $t2,  7($a0)\n\t"
 "lbu       $t3, 10($a0)\n\t"
 "pcpyld    $t0, $t1, $t0\n\t"
 "pcpyld    $t2, $t3, $t2\n\t"
 "psubuw    $t0, $t0, $at\n\t"
 "psubuw    $t2, $t2, $at\n\t"
 "pmultuw   $zero, $t0, $v0\n\t"
 "lbu       $t0,  2($a0)\n\t"
 "lbu       $t1,  5($a0)\n\t"
 "lbu       $t5,  8($a0)\n\t"
 "lbu       $t3, 11($a0)\n\t"
 "pmadduw   $zero, $t2, $v1\n\t"
 "pcpyld    $t0, $t1, $t0\n\t"
 "pcpyld    $t2, $t3, $t5\n\t"
 "psubuw    $t0, $t0, $at\n\t"
 "psubuw    $t2, $t2, $at\n\t"
 "pmadduw   $zero, $t0, $a2\n\t"
 "lbu       $t1, 12($a0)\n\t"
 "addiu     $t4, $t4, -1\n\t"
 "pmadduw   $t0, $t2, $a3\n\t"
 "subu      $t1, $t1, $at\n\t"
 "addiu     $a1, $a1, 8\n\t"
 "pcpyud    $t2, $t0, $zero\n\t"
 "addiu     $a0, $a0, 13\n\t"
 "addu      $t2, $t2, $t0\n\t"
 "addu      $t2, $t2, $t1\n\t"
 "bgtz      $t4, 1b\n\t"
 "sd        $t2, -8($a1)\n\t"
 "jr        $ra\n\r"
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
 unsigned int   lRW    = (   (  ( lW = lpHdr -> m_Width  ) + 7  ) & ~7   );
 unsigned int   lRH    = (   (  ( lH = lpHdr -> m_Height ) + 7  ) & ~7   );
 unsigned int   lSize  = lRW * lRH;
 SMS_DXSBFrame* retVal = ( SMS_DXSBFrame* )memalign (
  64, lSize + sizeof ( SMS_DXSBFrame )
 );

 if ( retVal ) {

  int            i, lIt, lX, lY, lLen;
  unsigned char* lpDst, *lpPixmap;
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

  lpDst = lpPixmap = ( char* )(  ( unsigned int )retVal -> m_pPixmap | 0x30000000  );

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

    mips_memset (  lpDst + lX, ( lVal & 3 ) + 3, lLen  );

    if (  ( lX += lLen ) >= lW  ) {

     if (  ( lY += 2 ) >= lH  ) break;

     lpDst += lRW << 1;
     lX     = 0;
     lIt   += lIt & 1;

    }  /* end if */

   }  /* end for */

   lpDst = lpPixmap + lRW;
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

   SMS_RingBufferPost ( apOutput );

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end SMS_DXSB_Decode */
