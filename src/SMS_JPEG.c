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
#include "SMS.h"
#include "SMS_JPEG.h"
#include "SMS_DMA.h"
#include "SMS_Rescale.h"
#include "SMS_DSP.h"
#include "SMS_FileMapping.h"
#include "SMS_GS.h"
#include "SMS_Config.h"
#include "SMS_VSync.h"
#include "SMS_Locale.h"
#include "SMS_Player.h"
#include "SMS_ContainerJPG.h"
#include "SMS_Timer.h"
#include "SMS_FileDir.h"
#include "SMS_Sounds.h"
#include "SMS_GUI.h"
#include "SMS_RC.h"
#include "SMS_PAD.h"
#include "SMS_GUICons.h"
#include "SMS_VIF.h"
#include "SMS_PgInd.h"

#include <kernel.h>
#include <malloc.h>
#include <string.h>

#define STS_VU0IDCT 0x00000001
#define STS_VU0VU1  0x00000002
#define STS_VU1R    0x00000004
#define STS_VU1CSC  0x00000008
#define STS_DMA9    0x00000010

#define MK_SOI 0xFFD8
#define MK_SOF 0xFFC0
#define MK_DHT 0xFFC4
#define MK_DQT 0xFFDB
#define MK_DRI 0xFFDD
#define MK_SOS 0xFFDA
#define MK_EOI 0xFFD9
#define MK_APP 0xFFE0
#define MK_COM 0xFFFE
#define MK_MSK 0xFFF0

#define MK_RST( x )	(   (  ( x ) & 0xFFF8  ) == 0xFFD0   )

#define HI_NIBBLE( c ) (  ( c ) >> 4  )
#define LO_NIBBLE( c ) (  ( c ) & 15  )
#define HID( c, i )    (  ( c << 1 ) + i  )
#define MAX_SIZE( c )  (  ( c ) ? 384 : 64  )
#define MAX_CELLS( c ) (  MAX_SIZE( c ) - 32  )
#define GET_SIZE( c )  (  _get_short_be ( c )  )
#define SKIP_SEG( c )  (  _skip (  c, GET_SIZE( c ) - 2  )  )
#define DATA_END( c )  (  ( c ) -> m_pData >= ( c ) -> m_pDataEnd  )

#define HEOB 0x00
#define HZRL 0xF0

#define BLK_RB_SLOT( c, slot )   (  ( slot ) == ( c ) -> m_pBlkEnd ? ( short* )0x70000000 : ( slot )  )
#define BLK_RB_EMPTY( c )        (  ( c ) -> m_pBlkIn == ( c ) -> m_pBlkOut  )
#define BLK_RB_FULL( c )         (   BLK_RB_SLOT(  c, ( c ) -> m_pBlkIn + 80  ) == ( c ) -> m_pBlkOut   )
#define BLK_RB_PUSHSLOT( c )     (  ( c ) -> m_pBlkIn   )
#define BLK_RB_POPSLOT( c )      (  ( c ) -> m_pBlkOut  )
#define BLK_RB_PUSHADVANCE( c )  (   ( c ) -> m_pBlkIn  = BLK_RB_SLOT(  ( c ), ( c ) -> m_pBlkIn  + 80  )   )
#define BLK_RB_POPADVANCE( c )   (   ( c ) -> m_pBlkOut = BLK_RB_SLOT(  ( c ), ( c ) -> m_pBlkOut + 80  )   )

extern SMS_LZMAData g_JPEGData[ 5 ] __attribute__(   (  section( ".rodata" )  )   );

static void         _reset_pred   ( SMS_JPEGContext*               );
static int          _ceil_div     ( int, int                       );
static int          _floor_div    ( int, int                       );
static void         _init_MCU     ( SMS_JPEGContext*               );
       void         _pack_8       ( void*, int, int                );
       void         _pack_16      ( void*, int, int                );
static void         _init_clr_fmt ( _jpeg_clr_fmt                  );
static unsigned int _next_MK      ( SMS_JPEGContext*               );
static int          _load_huff    ( SMS_JPEGContext*               );
static int          _load_qtbls   ( SMS_JPEGContext*               );
static void         _process_MCU  ( SMS_JPEGContext*, unsigned int );

static unsigned int inline _get_short_be ( SMS_JPEGContext* apCtx ) {
 unsigned int retVal;
 retVal = apCtx -> m_pData[ 0 ];
 retVal = ( retVal << 8 ) | apCtx -> m_pData[ 1 ];
 apCtx -> m_pData += 2;
 return retVal;
}  /* end _get_short_be */

static void inline _skip ( SMS_JPEGContext* apCtx, unsigned int anBytes ) {
 apCtx -> m_pData += anBytes;
}  /* end _skip */

static unsigned int inline _get_byte ( SMS_JPEGContext* apCtx ) {
 unsigned int retVal = apCtx -> m_pData[ 0 ];
 apCtx -> m_pData   += 1;
 return retVal;
}  /* end _get_byte */

static void inline VU0Kick ( void ) {
 __asm__ __volatile__(
 ".set noat\n\t"
 "cfc2.i    $at, $vi26\n\t"
 "ctc2      $at, $vi27\n\t"
 "vnop\n\t"
 "vnop\n\t"
 "vcallmsr  $vi27\n\t"
 ".set at\n\t"
 ::: "at"
 );
}  /* end VU0Kick */

static int inline VU0Busy ( void ) {
 int retVal;
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui  $at, 0x1001\n\t"
  "lw   %0, -32768($at)\n\t"
  "cfc2 $at, $vi29\n\t"
  "andi %0, %0, 0x0100\n\t"
  "andi $at, $at, 0x0081\n\t"
  "or   %0, $at, %0\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( retVal ) :: "at"
 );
 return retVal;
}  /* end VU0Busy */

static int inline VU1Busy ( void ) {
 int retVal;
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "sync.p\n\t"
  "cfc2 %0, $vi29\n\t"
  "andi %0, %0, 0x0100\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end VU1Busy */

static int inline VU01Busy ( void ) {
 int retVal;
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "sync.p\n\t"
  "lui  $at, 0x1001\n\t"
  "lw   %0, -32768($at)\n\t"
  "cfc2 $at, $vi29\n\t"
  "andi %0, %0, 0x0100\n\t"
  "andi $at, $at, 0x0181\n\t"
  "or   %0, $at, %0\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( retVal ) :: "at"
 );
 return retVal;
}  /* end VU01Busy */

static void inline VU1Kick ( void ) {
 unsigned short lTPC;
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "cfc2     $at, $vi15\n\t"
  "or       %0, $zero, 0x043A\n\t"
  "ctc2     %0, $vi15\n\t"
  "vilwr.x  $vi15, ($vi15)\n\t"
  "cfc2     %0, $vi15\n\t"
  "ctc2     $at, $vi15\n\t"
  "ctc2     %0, $vi31\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  : "=r"( lTPC ) :: "at"
 );
}  /* end VU1Kick */

static void inline DMA9Kick ( void* apDst, void* apSrc, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui  $at, 0x1001\n\t"
  "ori  $t9, $zero, 0x0101\n\t"
  "sw   %1, -11248($at)\n\t"
  "sw   %2, -11232($at)\n\t"
  "sw   %0, -11136($at)\n\t"
  "sw   $t9, -11264($at)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( apDst ), "r"( apSrc ), "r"( aQWC ) : "at", "t9"
 );
}  /* end DMA9Kick */ 

static unsigned int inline DMA9Busy ( void ) {
 unsigned int retVal;
 __asm__ __volatile__(
  "lui  %0, 0x1001\n\t"
  "lw   %0, -11264(%0)\n\t"
  "andi %0, %0, 0x0100\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end DMA9Busy */

static void inline VIF0KickSPR ( void* apTag ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui  $t9, 0x8000\n\t"
  "lui  $at, 0x1001\n\t"
  "or   %0, %0, $t9\n\t"
  "ori  $t9, $zero, 0x0145\n\t"
  "sw   $zero, -32736($at)\n\t"
  "sw   %0, -32720($at)\n\t"
  "sw   $t9, -32768($at)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( apTag ) : "at", "t9"
 );
}  /* end VIF0KickSPR */

static void inline VIF0Kick ( void* apTag ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui  $at, 0x1001\n\t"
  "ori  $t9, $zero, 0x0145\n\t"
  "sw   $zero, -32736($at)\n\t"
  "sw   %0, -32720($at)\n\t"
  "sw   $t9, -32768($at)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( apTag ) : "at", "t9"
 );
}  /* end VIF0Kick */

static unsigned int inline VIF0Busy ( void ) {
 unsigned int retVal;
 __asm__ __volatile__(
  "lui  %0, 0x1001\n\t"
  "lw   %0, -32768(%0)\n\t"
  "andi %0, %0, 0x0100\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end VIF0Busy */

static void inline VIF1Kick ( void* apTag ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set nomacro\n\t"
  ".set noat\n\t"
  "lui  $at, 0x1001\n\t"
  "ori  $t9, $zero, 0x0145\n\t"
  "sw   $zero, -28640($at)\n\t"
  "sw   %0, -28624($at)\n\t"
  "sw   $t9, -28672($at)\n\t"
  ".set at\n\t"
  ".set macro\n\t"
  ".set reorder\n\t"
  :: "r"( apTag ) : "at", "t9"
 );
}  /* end VIF1Kick */

static unsigned int inline VIF1Busy ( void ) {
 unsigned int retVal;
 __asm__ __volatile__(
  "lui  %0, 0x1001\n\t"
  "lw   %0, -28672(%0)\n\t"
  "andi %0, %0, 0x0100\n\t"
  : "=r"( retVal )
 );
 return retVal;
}  /* end VIF1Busy */

static void inline _init_clr_fmt ( _jpeg_clr_fmt aFmt ) {

 SMS_InitVU ( DMAC_VIF1, &g_JPEGData[ aFmt + 1 ] );

 while (  VIF1Busy ()  );

}  /* end _init_clr_fmt */

SMS_JPEGContext* SMS_JPEGInit (  void ( *aProgress ) ( void*, unsigned int ), void* apPrgParam ) {

 const unsigned long lDMATag = 0x0000000070000009UL;  /* DMAEnd ( 9 );                                  */
 const unsigned long lVIFCmd = 0x6D10001001000404UL;  /* STCYCL ( 4, 4 ); UNPACK ( v4_16, 0x10, 0x10 ); */
 const unsigned long lVIFEnd = 0x0000000017000000UL;  /* MSCNT ( 0 )                                    */
 const unsigned long lVIFNop = 0x0000000000000000UL;  /* NOP (); NOP ();                                */

 SMS_JPEGContext* retVal;
 unsigned long*   lpBuf = ( unsigned long* )0x70000000;
 unsigned long*   lpEnd = ( unsigned long* )0x70001E00;

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui  $at, 0x1000\n\t"
  "lw   $v0, 0x3820($at)\n\t"
  "lw   $v1, 0x3C20($at)\n\t"
  "ori  $v0, $v0, 0x0002\n\t"
  "ori  $v1, $v1, 0x0002\n\t"
  "sw   $v0, 0x3820($at)\n\t"
  "sw   $v1, 0x3C20($at)\n\t"
  "sync.l\n\t"
  ".set at\n\t"
  ::: "at", "v0", "v1"
 );

 SMS_InitVU ( DMAC_VIF0, &g_JPEGData[ 0 ] );

 retVal = ( SMS_JPEGContext* )calloc (  1, sizeof ( SMS_JPEGContext )  );

 retVal -> m_pHTbl[ 0 ] = &retVal -> m_DCTbl[ 0 ][ 0 ];
 retVal -> m_pHTbl[ 1 ] = &retVal -> m_DCTbl[ 1 ][ 0 ];
 retVal -> m_pHTbl[ 2 ] = &retVal -> m_ACTbl[ 0 ][ 0 ];
 retVal -> m_pHTbl[ 3 ] = &retVal -> m_ACTbl[ 1 ][ 0 ];
 retVal -> m_Format     = JCF_Invalid;

 do {

  lpBuf[  0 ] = lDMATag;
  lpBuf[  1 ] = lVIFCmd;
  lpBuf[ 18 ] = lVIFEnd;
  lpBuf[ 19 ] = lVIFNop;

  lpBuf += 20;

 } while ( lpBuf != lpEnd );

 retVal -> m_pBlkEnd   = ( short* )lpEnd;
 retVal -> m_pPrgParam = apPrgParam;
 retVal -> progress    = aProgress;

 while (  VIF0Busy ()  );

 return retVal;

}  /* end SMS_JPEGInit */

void SMS_JPEGDestroy ( SMS_JPEGContext* apCtx ) {

 if ( !apCtx ) return;

 SMS_RescaleDestroy ( apCtx -> m_pRC );

 if ( apCtx -> m_pBitmap ) free ( apCtx -> m_pBitmap );

 free ( apCtx );

}  /* end SMS_JPEGDestroy */

int SMS_JPEGLoad ( SMS_JPEGContext* apCtx, void* apData, unsigned int aDataSize ) {

 int retVal = 0;

 apCtx -> m_MR       = 0;
 apCtx -> m_MC       = 0;
 apCtx -> m_pBlkIn   =
 apCtx -> m_pBlkOut  = ( short* )0x70000000;
 apCtx -> m_pData    = ( unsigned char* )apData;
 apCtx -> m_pDataEnd = ( unsigned char* )apData + aDataSize;
 apCtx -> m_nBits    = 0;
 apCtx -> m_PrgPerc  = 0;

 if (  _next_MK ( apCtx ) == MK_SOI  ) {

  unsigned int i, lMark;
  unsigned int lRstInt = 0, lLeft;

  lRstInt = 0;

  do switch (  lMark = _next_MK ( apCtx )  ) {

   case MK_SOF: {

    _jpeg_comp_desc* lpDesc = &apCtx -> m_CompDesc[ 0 ], *lpEnd;
    unsigned int     lnComp;
    _jpeg_clr_fmt    lFmt;
 
    if (  apCtx -> m_pData + GET_SIZE( apCtx ) >= apCtx -> m_pDataEnd  ) goto end;

    _get_byte ( apCtx );

    apCtx -> m_Height = GET_SIZE( apCtx );
    apCtx -> m_Width  = GET_SIZE( apCtx );
    apCtx -> m_nComp  = lnComp = _get_byte ( apCtx );

    if ( lnComp != 3 ) goto end;

    lpEnd = lpDesc + lnComp;

    do {

     lpDesc -> m_CompID = _get_byte ( apCtx );
     lMark              = _get_byte ( apCtx );
     lpDesc -> m_HSF    = HI_NIBBLE( lMark );
     lpDesc -> m_VSF    = LO_NIBBLE( lMark );
     lpDesc -> m_QT     = _get_byte ( apCtx );

    } while ( ++lpDesc != lpEnd );

    _init_MCU ( apCtx );

    if ( apCtx -> m_MCUW == 8 && apCtx -> m_MCUH == 8 ) {
     lFmt                = JCF_8x8;
     apCtx -> m_nBytesPM = 1024;
     apCtx -> m_MCUIncr  = 8;
     apCtx -> m_VU1Mem   = 0x1100C000 + ( 212 * 4 );
     apCtx -> pack       = _pack_8;
    } else if ( apCtx -> m_MCUW == 16 && apCtx -> m_MCUH ==  8 ) {
     lFmt                = JCF_16x8;
     apCtx -> m_nBytesPM = 2048;
     apCtx -> m_MCUIncr  = 16;
     apCtx -> m_VU1Mem   = 0x1100C000 + ( 404 * 4 );
     apCtx -> pack       = _pack_16;
    } else if ( apCtx -> m_MCUW == 8  && apCtx -> m_MCUH == 16 ) {
     lFmt                = JCF_8x16;
     apCtx -> m_nBytesPM = 2048;
     apCtx -> m_MCUIncr  = 8;
     apCtx -> m_VU1Mem   = 0x1100C000 + ( 404 * 4 );
     apCtx -> pack       = _pack_8;
    } else if ( apCtx -> m_MCUW == 16 && apCtx -> m_MCUH == 16 ) {
     lFmt                = JCF_16x16;
     apCtx -> m_nBytesPM = 4096;
     apCtx -> m_MCUIncr  = 16;
     apCtx -> m_VU1Mem   = 0x1100C000 + ( 788 * 4 );
     apCtx -> pack       = _pack_16;
    } else goto end;

    if ( apCtx -> m_Format != lFmt ) _init_clr_fmt ( apCtx -> m_Format = lFmt );

    apCtx -> m_pRC = SMS_RescaleInit (
     apCtx -> m_pRC, apCtx -> m_Width, apCtx -> m_Height,
     apCtx -> m_MCUH, apCtx -> m_MCUW
    );

    apCtx -> m_pDst = ( unsigned int* )apCtx -> m_pRC -> m_pStripePtr;

   } break;

   case MK_DHT: {

    if (  !_load_huff ( apCtx )  ) goto end;

   } break;

   case MK_DQT: {

    if (  !_load_qtbls ( apCtx )  ) goto end;

   } break;

   case MK_DRI: {

    GET_SIZE( apCtx );
    lRstInt = GET_SIZE( apCtx );

   } break;

   case MK_SOS: {

    int lnComp = apCtx -> m_nComp;

    if (  apCtx -> m_pData + GET_SIZE( apCtx ) >= apCtx -> m_pDataEnd  ) goto end;

    if (  _get_byte ( apCtx ) != ( int )apCtx -> m_nComp  ) goto end;

    for (  i = 0; i < ( unsigned )lnComp; ++i  ) {
     char lByte;
     if (  _get_byte ( apCtx ) != apCtx -> m_CompDesc[ i ].m_CompID  ) goto end;
	 lByte = _get_byte ( apCtx );
	 apCtx -> m_CompDesc[ i ].m_DCHT = HI_NIBBLE( lByte );
	 apCtx -> m_CompDesc[ i ].m_ACHT = LO_NIBBLE( lByte );
    }  /* end for */

    if (  apCtx -> m_pData + GET_SIZE( apCtx ) >= apCtx -> m_pDataEnd  ) goto end;

    _get_byte ( apCtx );

    apCtx -> m_nBits = 0;

    _reset_pred ( apCtx );

    if ( lRstInt ) {
     unsigned int lnRst = _ceil_div ( apCtx -> m_MW * apCtx -> m_MH, lRstInt ) - 1;
     lLeft = apCtx -> m_MW * apCtx -> m_MH - lnRst * lRstInt;
     for ( i = 0; i < lnRst; ++i ) {
      _process_MCU ( apCtx, lRstInt );
      lMark = _next_MK ( apCtx );
      if (  !MK_RST( lMark )  ) goto end;
      apCtx -> m_nBits = 0;
      _reset_pred ( apCtx );
     }  /* end for */
    } else lLeft = apCtx -> m_MW * apCtx -> m_MH;

    _process_MCU ( apCtx, lLeft );

   } break;

   case MK_EOI: {

    SMS_RescaleContext* lpRC    = apCtx -> m_pRC;
    unsigned int        lnAlloc = lpRC -> m_NewWidth * lpRC -> m_NewHeight * 3;

    apCtx -> m_nBitmapQWC   = (  ( lnAlloc + 15 ) & ~15  ) >> 4;
    apCtx -> m_nBitmapBytes = lnAlloc;

    if ( apCtx -> m_nAlloc < lnAlloc ) apCtx -> m_pBitmap = ( unsigned char* )realloc64 (   apCtx -> m_pBitmap, (  ( apCtx -> m_nAlloc = lnAlloc ) + 64  ) & ~63   );

    lpRC -> ProcessBuffer (  lpRC, UNCACHED_SEG( apCtx -> m_pBitmap )  );

    retVal = 1;
    goto end;

   } break;

   case MK_COM: {

    SKIP_SEG( apCtx );

   } break;

   case 0x80000000: {

   } goto end;

   default: {
    if (  ( lMark & MK_MSK ) == MK_APP  )
     SKIP_SEG( apCtx );
    else if (  MK_RST( lMark )  )
     _reset_pred ( apCtx );
    else goto end;
   } break;

  } while ( 1 );

 }  /* end if */
end:
 return retVal;

}  /* end SMS_JPEGLoad */

static void _reset_pred ( SMS_JPEGContext* apCtx ) {

 apCtx -> m_CompDesc[ 0 ].m_Pred =
 apCtx -> m_CompDesc[ 1 ].m_Pred =
 apCtx -> m_CompDesc[ 2 ].m_Pred = 0;

}  /* end _reset_pred */

static int _ceil_div ( int aN, int aD ) {

 int i = aN / aD;

 return ( aN > aD * i ) ? i + 1 : i;

}  /* end _ceil_div */

static int _floor_div ( int aN, int aD ) {

 int i = aN / aD;

 return ( aN < aD * i ) ? i - 1 : i;

}  /* end _floor_div */

static void _init_MCU ( SMS_JPEGContext* apCtx ) {

 int i, j, k, lN, lHMax = 0, lVMax = 0;
 int lnComp = apCtx -> m_nComp;

 apCtx -> m_nMCU = 0;

 for ( i = 0, k = 0; i < lnComp; ++i ) {

  if ( apCtx -> m_CompDesc[ i ].m_HSF > lHMax ) lHMax = apCtx -> m_CompDesc[ i ].m_HSF;
  if ( apCtx -> m_CompDesc[ i ].m_VSF > lVMax ) lVMax = apCtx -> m_CompDesc[ i ].m_VSF;

  apCtx -> m_nMCU += ( lN = apCtx -> m_CompDesc[ i ].m_HSF * apCtx -> m_CompDesc[ i ].m_VSF );

  for ( j = 0; j < lN; ++j ) apCtx -> m_MCUIdx[ k++ ] = i;

 }  /* end for */

 apCtx -> m_MCUW = lHMax << 3;
 apCtx -> m_MCUH = lVMax << 3;

 for ( i = 0; i < lnComp; ++i ) {
  apCtx -> m_CompDesc[ i ].m_HDIV = lHMax / apCtx -> m_CompDesc[ i ].m_HSF > 1;
  apCtx -> m_CompDesc[ i ].m_VDIV = lVMax / apCtx -> m_CompDesc[ i ].m_VSF > 1;
 }  /* end for */

 apCtx -> m_MW = _ceil_div ( apCtx -> m_Width,  apCtx -> m_MCUW );
 apCtx -> m_MH = _ceil_div ( apCtx -> m_Height, apCtx -> m_MCUH );
 apCtx -> m_RW = apCtx -> m_MCUW * _floor_div ( apCtx -> m_Width,  apCtx -> m_MCUW );
 apCtx -> m_RH = apCtx -> m_MCUH * _floor_div ( apCtx -> m_Height, apCtx -> m_MCUH );

}  /* end _init_MCU */

void _pack_8  ( void*, int, int );
void _pack_16 ( void*, int, int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_pack_8:\n\t"
 "lui   $a3, 0x7000\n\t"
 "addu  $a2, $a3, $a2\n\t"
 "sll   $a1, $a1, 2\n\t"
 "1:\n\t"
 "lq    $t0, 0x1E00($a3)\n\t"
 "lq    $t1, 0x1E10($a3)\n\t"
 "lq    $t2, 0x1E20($a3)\n\t"
 "lq    $t3, 0x1E30($a3)\n\t"
 "lq    $t4, 0x1E40($a3)\n\t"
 "ppach $t0, $t1, $t0\n\t"
 "lq    $t5, 0x1E50($a3)\n\t"
 "ppach $t2, $t3, $t2\n\t"
 "lq    $t6, 0x1E60($a3)\n\t"
 "lq    $t7, 0x1E70($a3)\n\t"
 "addiu $a3, $a3, 128\n\t"
 "ppach $t4, $t5, $t4\n\t"
 "ppach $t6, $t7, $t6\n\t"
 "ppacb $t0, $t2, $t0\n\t"
 "ppacb $t4, $t6, $t4\n\t"
 "sq    $t0,  0($a0)\n\t"
 "sq    $t4, 16($a0)\n\t"
 "bne   $a3, $a2, 1b\n\t"
 "addu  $a0, $a0, $a1\n\t"
 "jr    $ra\n\t"
 "_pack_16:\n\t"
 "lui   $a3, 0x7000\n\t"
 "sll   $a1, $a1, 2\n\t"
 "addu  $a2, $a3, $a2\n\t"
 "addiu $a1, $a1, -64\n\t"
 "1:\n\t"
 "addiu $at, $a3, 256\n\t"
 "2:\n\t"
 "lq    $t0, 0x1E00($a3)\n\t"
 "lq    $t1, 0x1E10($a3)\n\t"
 "lq    $t2, 0x1E20($a3)\n\t"
 "lq    $t3, 0x1E30($a3)\n\t"
 "lq    $t4, 0x1E40($a3)\n\t"
 "ppach $t0, $t1, $t0\n\t"
 "lq    $t5, 0x1E50($a3)\n\t"
 "ppach $t2, $t3, $t2\n\t"
 "lq    $t6, 0x1E60($a3)\n\t"
 "lq    $t7, 0x1E70($a3)\n\t"
 "addiu $a3, $a3, 128\n\t"
 "ppach $t4, $t5, $t4\n\t"
 "ppach $t6, $t7, $t6\n\t"
 "ppacb $t0, $t2, $t0\n\t"
 "ppacb $t4, $t6, $t4\n\t"
 "sq    $t0,  0($a0)\n\t"
 "sq    $t4, 16($a0)\n\t"
 "bne   $a3, $at, 2b\n\t"
 "addiu $a0, $a0, 32\n\t"
 "bne   $a3, $a2, 1b\n\t"
 "addu  $a0, $a0, $a1\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static unsigned int _next_MK ( SMS_JPEGContext* apCtx ) {

 unsigned char* lpData    = apCtx -> m_pData;
 unsigned char* lpDataEnd = apCtx -> m_pDataEnd;
 unsigned int   lChr0     = *lpData++;
 unsigned int   retVal;

 while ( 1 ) {

  unsigned int lChr1 = *lpData;

  if ( lChr0 == 0xFF && lChr1 ) {
   lpData += 1;
   retVal  = 0xFF00 | lChr1;
   break;
  }  /* end if */

  if ( lpData >= lpDataEnd ) {
   retVal = 0x80000000;
   break;
  }  /* end if */

  lChr0   = lChr1;
  lpData += 1;

 }  /* end while */

 apCtx -> m_pData = lpData;

 return retVal;

}  /* end _next_MK */

static int _load_qtbls ( SMS_JPEGContext* apCtx ) {

 unsigned char* lpData = apCtx -> m_pData;
 int            lLen;

 lLen    = (   (  ( unsigned int )lpData[ 0 ] << 8  ) |
               (  ( unsigned int )lpData[ 1 ]       )
           ) - 2;
 lpData += 2;

 while ( lLen >= 65 ) {

  unsigned char lVal = *lpData++;

  if (  HI_NIBBLE( lVal ) > 0  ) return 0;

  memcpy ( apCtx -> m_QTable[ LO_NIBBLE( lVal ) ], lpData, 64 );

  lpData += 64;
  lLen   -= 65;

 }  /* end for */

 apCtx -> m_pData = lpData;

 return lpData < apCtx -> m_pDataEnd;

}  /* end _load_qtbls */

static int _load_huff ( SMS_JPEGContext* apCtx ) {

 int            i, j, lVal, lSize, lCls, lID, lMaxSize;
 int            lnLeaves, lnNodes, lnCells, lMaxDepth;
 int            lfDone, lNextCell, lNextLevel;
 unsigned int*  lpPtr;
 unsigned char* lpData = apCtx -> m_pData;

 lSize   = (   (  ( unsigned int )lpData[ 0 ] << 8  ) |
               (  ( unsigned int )lpData[ 1 ]       )
           ) - 2;
 lpData += 2;

 while ( lSize > 0 ) {

  lVal = *lpData++;
  lCls = HI_NIBBLE( lVal );
  lID  = LO_NIBBLE( lVal );

  if ( lID > 1 ) return 0;

  lID      = HID( lCls, lID ); --lSize;
  lnCells  = lnNodes   = 1;
  lnLeaves = lMaxDepth = 0;
  lMaxSize = MAX_SIZE( lCls );

  memset (
   lpPtr = apCtx -> m_pHTbl[ lID ], 0, ( lVal = MAX_CELLS( lCls ) ) << 2
  );

  for ( i = 0; i < 16; ++i ) {

   lnLeaves = lpPtr[ lMaxSize - ( i << 1 ) - 1 ] = *lpData++;
   lnCells  = lnNodes << 1;
   lnNodes  = lpPtr[ lMaxSize - ( i << 1 ) - 2 ] = lnCells - lnLeaves;
   if ( lnLeaves ) lMaxDepth = i;

  }  /* end for */

  lSize     -= 16;
  lpPtr[ 0 ] = 0x101;
  lNextCell  = 2;
  i          = 0;
  lfDone     = 0;

  while ( i <= lMaxDepth ) {

   lnLeaves = lpPtr[ lMaxSize - ( i << 1 ) - 1 ];

   for ( j = 0; j < lnLeaves; ++j, ++lpData )
	if ( !lfDone ) {
     lpPtr[ lNextCell++ ] = *lpData | 0x200;
     if ( lNextCell >= lVal ) lfDone = !0;
	}  /* end if */

   lSize -= lnLeaves;

   if (  lfDone || ( i == lMaxDepth )  ) {
    ++i;
    continue;
   }  /* end if */

   lnNodes    = lpPtr[ lMaxSize - ( i << 1 ) - 2 ];
   lNextLevel = lNextCell + lnNodes;

   for ( j = 0; j < lnNodes; ++j ) {

    if ( lNextCell >= lVal ) {
     lfDone = ~0;
     break;
    }  /* end if */

    lpPtr[ lNextCell++ ] = ( lNextLevel >> 1 ) | (
     (   !(  ( lNextLevel | 1 ) >= lVal  )   ) << 8
    );
    lNextLevel += 2;

   }  /* end for */

   ++i;

  }  /* end while */

 }  /* end while */

 apCtx -> m_pData = lpData;

 return lpData < apCtx -> m_pDataEnd;

}  /* end _load_huff */

static void inline _unpack ( SMS_JPEGContext* apCtx, short* apTbl, _jpeg_comp_desc* apDesc ) {

 unsigned int   i, lRun;
 unsigned int   lVal;
 unsigned int   lSym;
 unsigned int*  lpDC    = apCtx -> m_pHTbl[ HID( 0, apDesc -> m_DCHT ) ];
 unsigned int*  lpAC    = apCtx -> m_pHTbl[ HID( 1, apDesc -> m_ACHT ) ];
 unsigned char* lpQT    = apCtx -> m_QTable[ ( unsigned char )apDesc -> m_QT ];
 unsigned int   lnBits  = apCtx -> m_nBits;
 unsigned int   lBitBuf = apCtx -> m_BitBuf;
 unsigned char* lpData  = apCtx -> m_pData;

 __asm__ __volatile__(
  "sq   $zero,   0(%0)\n\t"
  "sq   $zero,  16(%0)\n\t"
  "sq   $zero,  32(%0)\n\t"
  "sq   $zero,  48(%0)\n\t"
  "sq   $zero,  64(%0)\n\t"
  "sq   $zero,  80(%0)\n\t"
  "sq   $zero,  96(%0)\n\t"
  "sq   $zero, 112(%0)\n\t"
  :: "r"( apTbl )
 );

 lSym = 0;

 while (   ( lpDC[ lSym ] & 0x300 ) == 0x100   ) {
  unsigned int lBit;
  while ( lnBits <= 16 ) {
   unsigned char lChr = *lpData;
   lpData += 1 + ( lChr == 0xFF );
   lBitBuf = ( lBitBuf << 8 ) | lChr;
   lnBits += 8;
  }  /* end while */
  lBit    = (  lBitBuf >> ( lnBits - 1 )  ) & 1;
  lnBits -= 1;
  lSym    = (  ( lpDC[ lSym ] & 0xFF ) << 1  ) | lBit;
 }  /* end while */

 lSym = (  ( lpDC[ lSym ] & 0x300 ) == 0x200  ) ? ( lpDC[ lSym ] & 0xFF ) : 0;

 while ( lnBits <= 16 ) {
  unsigned char lChr = *lpData;
  lpData += 1 + ( lChr == 0xFF );
  lBitBuf = ( lBitBuf << 8 ) | lChr;
  lnBits += 8;
 }  /* end while */

 lVal    = (  lBitBuf >> ( lnBits - lSym )  ) & (  ( 1 << lSym ) - 1  );
 lnBits -= lSym;

 if ( lSym ) {
  unsigned int lTmp = 1 << ( lSym - 1 );
  if ( lVal < lTmp ) lVal += 1 + (  ( -1 ) << lSym  );
 } else lVal = 0;

 lVal += apDesc -> m_Pred;

 apDesc -> m_Pred = lVal;
 apTbl[ 0 ] = lVal * lpQT[ 0 ];

 for ( i = 1; i < 64; ++i ) {

  lSym = 0;

  while (   ( lpAC[ lSym ] & 0x300 ) == 0x100   ) {
   unsigned int lBit;
   while ( lnBits <= 16 ) {
    unsigned char lChr = *lpData;
    lpData += 1 + ( lChr == 0xFF );
    lBitBuf = ( lBitBuf << 8 ) | lChr;
    lnBits += 8;
   }  /* end while */
   lBit    = (  lBitBuf >> ( lnBits - 1 )  ) & 1;
   lnBits -= 1;
   lSym    = (  ( lpAC[ lSym ] & 0xFF ) << 1  ) | lBit;
  }  /* end while */

  lSym = (  ( lpAC[ lSym ] & 0x300 ) == 0x200  ) ? ( lpAC[ lSym ] & 0xFF ) : 0;

  if ( lSym == HEOB ) break;

  if ( lSym == HZRL ) {
   i += 15;
   continue;
  }  /* end if */

  lRun  = ( lSym >> 4 ) & 0x0F;
  lSym &= 0x0F;
  i    += lRun;

  while ( lnBits <= 16 ) {
   unsigned char lChr = *lpData;
   lpData += 1 + ( lChr == 0xFF );
   lBitBuf = ( lBitBuf << 8 ) | lChr;
   lnBits += 8;
  }  /* end while */

  lVal    = (  lBitBuf >> ( lnBits - lSym )  ) & (  ( 1 << lSym ) - 1  );
  lnBits -= lSym;

  if ( lSym ) {
   unsigned int lTmp = 1 << ( lSym - 1 );
   if ( lVal < lTmp ) lVal += 1 + (  ( -1 ) << lSym  );
  } else lVal = 0;

  apTbl[  g_SMS_DSP_zigzag_direct[ i ]  ] = lVal * lpQT[ i ];

 }  /* end for */

 apCtx -> m_pData  = lpData;
 apCtx -> m_BitBuf = lBitBuf;
 apCtx -> m_nBits  = lnBits;

}  /* end _unpack */

static void _process_MCU ( SMS_JPEGContext* apCtx, unsigned int anMCU ) {

 unsigned int     lnMCU    = apCtx -> m_nMCU;
 unsigned int     lnMCUOut = anMCU;
 _jpeg_comp_desc* lpDesc   = &apCtx -> m_CompDesc[ 0 ];
 unsigned int*    lpIdx    = &apCtx -> m_MCUIdx[     0 ];
 unsigned int*    lpEnd    = &apCtx -> m_MCUIdx[ lnMCU ];
 unsigned int     lState   = 0U;
 short*           lpADMA   = ( short* )-1;

 anMCU *= lnMCU;

 while ( lnMCUOut ) {

  if (  ( lState & STS_VU1CSC ) && !VU1Busy ()  ) {
   DMA9Kick (  ( void* )0x00001E00, ( void* )apCtx -> m_VU1Mem, apCtx -> m_nBytesPM >> 4 );
   lState &= ~STS_VU1CSC;
   lState |=  STS_DMA9;
  }  /* end if */

  if (  ( lState & STS_VU1R ) && !VU1Busy () && !lnMCU && !( lState & STS_DMA9 )  ) {
   VU1Kick ();
   lnMCU   = apCtx -> m_nMCU;
   lState &= ~STS_VU1R;
   lState |=  STS_VU1CSC;
  }  /* end if */

  if (   ( lState & STS_VU0VU1 ) && !(  lState & ( STS_VU1R | STS_VU1CSC | STS_DMA9 )  ) && !VU01Busy ()  ) {
   VU1Kick ();
   lnMCU  -= 1;
   lState &= ~STS_VU0VU1;
   lState |=  STS_VU1R;
  }  /* end if */

  if (  ( lState & STS_VU1R ) && !VU1Busy () && lnMCU ) lState &= ~STS_VU1R;

  if (   ( lState & STS_VU0IDCT ) && !( lState & ( STS_VU0VU1 | STS_VU1CSC | STS_VU1R )  ) && !VU01Busy ()  ) {
   VU0Kick ();
   lState |=  STS_VU0VU1;
   lState &= ~STS_VU0IDCT;
   lpADMA  = ( short* )-1;
  }  /* end if */

  if (   !BLK_RB_EMPTY( apCtx ) && !(  lState & ( STS_VU0IDCT | STS_VU0VU1 )  )   ) {

   lpADMA = BLK_RB_POPSLOT( apCtx ); BLK_RB_POPADVANCE( apCtx );
   VIF0KickSPR ( lpADMA );
   lState |= STS_VU0IDCT;

  }  /* end if */

  if (  !BLK_RB_FULL( apCtx ) && anMCU ) {

   short* lpDMA = BLK_RB_PUSHSLOT( apCtx );

   if ( lpDMA != lpADMA ) {
    BLK_RB_PUSHADVANCE( apCtx );
    anMCU -= 1;
    _unpack ( apCtx, lpDMA + 8, lpDesc + lpIdx[ 0 ] );
    if ( ++lpIdx == lpEnd ) lpIdx = &apCtx -> m_MCUIdx[ 0 ];
   }  /* end if */
  }  /* end if */

  if (  ( lState & STS_DMA9 ) && !DMA9Busy ()  ) {

   lState &= ~STS_DMA9;
   apCtx -> pack ( apCtx -> m_pDst, apCtx -> m_pRC -> m_Stride, apCtx -> m_nBytesPM );
   apCtx -> m_pDst   += apCtx -> m_MCUIncr;
   apCtx -> m_MC     += 1;
   lnMCUOut          -= 1;

  }  /* end if */

  if ( apCtx -> m_MC == apCtx -> m_MW ) {

   unsigned int lPerc = ( unsigned int )(   (  ( float )apCtx -> m_MR / ( float )apCtx -> m_MH  ) * 100.0F + 0.5F   );

   if ( apCtx -> progress && lPerc != apCtx -> m_PrgPerc ) apCtx -> progress ( apCtx -> m_pPrgParam, apCtx -> m_PrgPerc = lPerc );

   apCtx -> m_pRC -> ProcessStripe ( apCtx -> m_pRC );

   apCtx -> m_pDst  = apCtx -> m_pRC -> m_pStripePtr;
   apCtx -> m_MC    = 0;
   apCtx -> m_MR   += 1;

   DMA_Wait ( DMAC_GIF );

  }  /* end if */

 }  /* end while */

 apCtx -> m_pData -= 4;

}  /* end _process_MCU */

static void _progress ( void* apParam, unsigned int aPerc ) {

 SMS_JPEGViewer* lpViewer = ( SMS_JPEGViewer* )apParam;
 int             lW       = ( int )(  lpViewer -> m_PixPP * ( float )aPerc + 0.5F  );
 int             lShift   = 4 - g_XShift;

 lpViewer -> m_pStsPgX[  0 ] =
 lpViewer -> m_pStsPgX[ 12 ] = ( unsigned short )(  ( lpViewer -> m_StsX + lW + 12 ) << lShift  );

 DMA_Send ( DMAC_GIF, lpViewer -> m_pStsPgFg, 5 );

}  /* end _progress */

static void _display_def_picture ( SMS_JPEGViewer* apViewer ) {

 GSContext_CallList ( 0, apViewer -> m_pDefPic );
 GS_VSync ();
 GSContext_Flush ( 0, GSFlushMethod_DeleteLists );

}  /* end _display_def_picture */

SMS_JPEGViewer* SMS_JPEGViewerInit ( void ) {

 SMS_JPEGViewer* retVal = ( SMS_JPEGViewer* )memalign (  64, sizeof ( SMS_JPEGViewer )  );
 int             lX, lY, lW;
 unsigned long   lPrim;

 memset (  retVal, 0, sizeof ( SMS_JPEGViewer )  );
 SyncDCache ( retVal, retVal + 1 );

 retVal -> m_pCtx        = SMS_JPEGInit ( _progress, retVal );
 retVal -> m_ClrDepthOrg = g_Config.m_ColorDepth;
 g_Config.m_ColorDepth   = 0;

 if ( retVal -> m_ClrDepthOrg || GS_Params () -> m_GSCRTMode == GSVideoMode_DTV_1920x1080I ) {
  GS_VSync ();
  GSContext_Init ( g_Config.m_DisplayMode, GSZTest_Off, GSDoubleBuffer_Off );
  GS_VSync ();
 }  /* end if */

 SMS_FileMappingInit ();

 retVal -> m_StsW = lW   = ( unsigned int )( g_GSCtx.m_Width * 0.9F + 0.5F );
 retVal -> m_StsH        = 64;
 retVal -> m_StsX = lX   = ( g_GSCtx.m_Width  - retVal -> m_StsW ) >> 1;
 retVal -> m_StsY = lY   = g_GSCtx.m_Height - retVal -> m_StsH - ( retVal -> m_StsH >> 1 );
 retVal -> m_pDefPic     = GSContext_NewList (  GS_VGR_PACKET_SIZE()  );
 retVal -> m_pStsOutline = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );
 retVal -> m_pStsBkgnd   = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );
 retVal -> m_pStsPgBg    = GSContext_NewList (  GS_VGR_PACKET_SIZE()  );
 retVal -> m_pStsPgFg    = ( unsigned long* )memalign (  64, GS_VGR_PACKET_SIZE() << 3  );

 lX += 12;
 lY += 48;
 lW -= 24;

 GSContext_RenderVGRect (
  retVal -> m_pDefPic, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height,
  GS_SET_RGBAQ( 0x00, 0x00, 0x40, 0x80, 0x00 ),
  GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x80, 0x00 )
 );
 GSContext_RenderVGRect (
  retVal -> m_pStsPgBg, lX, lY, lW, 4,
  GS_SET_RGBAQ( 0x20, 0x20, 0x80, 0x20, 0x00 ),
  GS_SET_RGBAQ( 0x20, 0x20, 0x80, 0x20, 0x00 )
 );
 GSContext_RenderVGRect (
  retVal -> m_pStsPgFg, lX, lY, lW, 4,
  GS_SET_RGBAQ( 0x20, 0x20, 0xFF, 0x00, 0x00 ),
  GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 )
 );
 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( retVal -> m_pStsOutline - 2 ),
  retVal -> m_StsX, retVal -> m_StsY,
  retVal -> m_StsW, retVal -> m_StsH,
  -12, 0x80000080
 );
 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( retVal -> m_pStsBkgnd - 2 ),
  retVal -> m_StsX, retVal -> m_StsY,
  retVal -> m_StsW, retVal -> m_StsH,
   12, 0x50201000
 );

 lPrim = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 0, 0, 0, 0, 0, 0, 0 );

 retVal -> m_pDefPic [ 2 ] = lPrim;
 retVal -> m_pStsPgFg[ 2 ] = lPrim;

 _display_def_picture ( retVal );
 
 SyncDCache (  retVal -> m_pStsPgFg, retVal -> m_pStsPgFg + GS_VGR_PACKET_SIZE()  );

 retVal -> m_PixPP   = ( float )lW / 100.0F;
 retVal -> m_pStsPgX = ( unsigned short* )UNCACHED_SEG( retVal -> m_pStsPgFg + 5 );

 return retVal;

}  /* end SMS_JPEGViewerInit */

void SMS_JPEGViewerDestroy ( SMS_JPEGViewer* apViewer ) {

 if ( apViewer ) {

  g_Config.m_ColorDepth = apViewer -> m_ClrDepthOrg;

  free ( apViewer -> m_pStsPgFg );

  GSContext_DeleteList ( apViewer -> m_pStsText    );
  GSContext_DeleteList ( apViewer -> m_pStsPgBg    );
  GSContext_DeleteList ( apViewer -> m_pStsBkgnd   );
  GSContext_DeleteList ( apViewer -> m_pStsOutline );
  GSContext_DeleteList ( apViewer -> m_pDefPic     );

  SMS_FileMappingDestroy ();

  SMS_JPEGDestroy ( apViewer -> m_pCtx );

  free ( apViewer );

 }  /* end if */

}  /* end SMS_JPEGViewerDestroy */

void _transpose ( void*, int, int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_transpose:\n\t"
 "mult    $v1, $a2, $a1\n\t"
 "ori     $t1, $zero, 1\n\t"
 "or      $t2, $zero, $a0\n\t"
 "xor     $t4, $t4, $t4\n\t"
 "addiu   $t5, $v1, -1\n\t"
 "slt     $a0, $t1, $t5\n\t"
 "beqz    $a0, 1f\n\t"
 "addiu   $t6, $v1, -2\n\t"
 "blez    $t6, 1f\n\t"
 "addiu   $t3, $t2, 3\n\t"
 "div     $zero, $t1, $a1\n\t"
 "6:\n\t"
 "mfhi    $a0\n\t"
 "madd    $a0, $a2\n\t"
 "mflo    $a3\n\t"
 "slt     $v1, $t1, $a3\n\t"
 "beqz    $v1, 3f\n\t"
 "or      $t0, $zero, $t1\n\t"
 "div     $zero, $a3, $a1\n\t"
 "4:\n\t"
 "or      $t0, $zero, $a3\n\t"
 "mfhi    $a3\n\t"
 "madd    $a3, $a2\n\t"
 "mflo    $a3\n\t"
 "slt     $t7, $t1, $a3\n\t"
 "bnezl   $t7, 4b\n\t"
 "div     $zero, $a3, $a1\n\t"
 "3:\n\t"
 "beql    $a3, $t1, 5f\n\t"
 "slt     $at, $t1, $t0\n\t"
 "7:\n\t"
 "addiu   $t1, $t1, 1\n\t"
 "slt     $a3, $t1, $t5\n\t"
 "beqz    $a3, 1f\n\t"
 "addiu   $t3, $t3, 3\n\t"
 "slt     $t9, $t4, $t6\n\t"
 "bnezl   $t9, 6b\n\t"
 "div     $zero, $t1, $a1\n\t"
 "1:\n\t"
 "jr      $ra\n\t"
 "nop\n\t"
 "5:\n\t"
 "beqzl   $at, 7b\n\t"
 "addiu   $t4, $t4, 1\n\t"
 "lwl     $a0, 3($t3)\n\t"
 "lwr     $a0, 0($t3)\n\t"
 "mtc1    $a0, $f00\n\t"
 "8:\n\t"
 "div     $zero, $t0, $a2\n\t"
 "sll     $at, $t0, 1\n\t"
 "sll     $v0, $a3, 1\n\t"
 "addu    $t7, $at, $t0\n\t"
 "addu    $at, $v0, $a3\n\t"
 "addu    $a0, $t7, $t2\n\t"
 "addu    $t9, $at, $t2\n\t"
 "or      $a3, $zero, $t0\n\t"
 "lb      $t0, 0($a0)\n\t"
 "lb      $t7, 1($a0)\n\t"
 "mfhi    $t8\n\t"
 "lb      $at, 2($a0)\n\t"
 "madd    $t8, $a1\n\t"
 "sb      $t0, 0($t9)\n\t"
 "sb      $t7, 1($t9)\n\t"
 "sb      $at, 2($t9)\n\t"
 "mflo    $t0\n\t"
 "slt     $at, $t1, $t0\n\t"
 "bnez    $at, 8b\n\t"
 "addiu   $t4, $t4, 1\n\t"
 "sll     $v1, $a3, 1\n\t"
 "addu    $t0, $v1, $a3\n\t"
 "mfc1    $v1, $f00\n\t"
 "addu    $t8, $t0, $t2\n\t"
 "sb      $v1, 0($t8)\n\t"
 "srl     $v1, $v1, 8\n\t"
 "sb      $v1, 1($t8)\n\t"
 "srl     $v1, $v1, 8\n\t"
 "sb      $v1, 2($t8)\n\t"
 "beq     $zero, $zero, 7b\n\t"
 "addiu   $t4, $t4, 1\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

void _flip_v ( void*, int, int );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_flip_v:\n\t"
 "sll	$at, $a1, 1\n\t"
 "addiu $a2, $a2, -1\n\t"
 "addu  $a1, $at, $a1\n\t"
 "mult  $a2, $a2, $a1\n\t"
 "srl   $v0, $a1, 3\n\t"
 "andi  $v1, $a1, 7\n\t"
 "sll   $a1, $a1, 1\n\t"
 "addu  $a2, $a0, $a2\n\t"
 "sltu  $at, $a0, $a2\n\t"
 "beq   $at, $zero, 1f\n\t"
 "or    $a3, $zero, $v0\n\t"
 "5:\n\t"
 "beq   $a3, $zero, 2f\n\t"
 "or    $t0, $zero, $v1\n\t"
 "3:\n\t"
 "ldl   $t1, 7($a0)\n\t"
 "addiu $a3, $a3, -1\n\t"
 "ldr   $t1, 0($a0)\n\t"
 "addiu $a0, $a0, 8\n\t"
 "ldl   $t2, 7($a2)\n\t"
 "addiu $a2, $a2, 8\n\t"
 "ldr   $t2, -8($a2)\n\t"
 "sdl   $t1, -1($a2)\n\t"
 "sdr   $t1, -8($a2)\n\t"
 "sdl   $t2, -1($a0)\n\t"
 "bne   $a3, $zero, 3b\n\t"
 "sdr   $t2, -8($a0)\n\t"
 "2:\n\t"
 "beq   $t0, $zero, 4f\n\t"
 "3:\n\t"
 "lbu   $t1, 0($a0)\n\t"
 "addiu $t0, $t0, -1\n\t"
 "lbu   $t2, 0($a2)\n\t"
 "addiu $a2, $a2, 1\n\t"
 "addiu $a0, $a0, 1\n\t"
 "sb    $t1, -1($a2)\n\t"
 "bne   $t0, $zero, 3b\n\t"
 "sb    $t2, -1($a0)\n\t"
 "4:\n\t"
 "subu  $a2, $a2, $a1\n\t"
 "sltu  $at, $a0, $a2\n\t"
 "bne   $at, $zero, 5b\n\t"
 "or    $a3, $zero, $v0\n\t"
 "1:\n\t"
 "jr    $ra\n\t"
 "nop\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void _calc_best_fit ( unsigned int aMaxW, unsigned int aMaxH, unsigned int anImgW, unsigned int anImgH, unsigned int* apCoord ) {

 unsigned int lImgW, lImgH;
 float        lRatio = ( float )anImgW / ( float )anImgH;

 lImgW  = aMaxW;
 lImgH  = ( unsigned int )(  ( float )aMaxW / lRatio  );

 if ( lImgH > aMaxH ) {
  lImgH = aMaxH;
  lImgW = ( unsigned int )(  ( float )aMaxH * lRatio  );
 }  /* end if */

 apCoord[ 0 ] = ( aMaxW - lImgW ) >> 1;
 apCoord[ 1 ] = ( aMaxH - lImgH ) >> 1;
 apCoord[ 2 ] = apCoord[ 0 ] + lImgW;
 apCoord[ 3 ] = apCoord[ 1 ] + lImgH;

}  /* end _calc_best_fit */

extern void PowerOf2 ( int, int, int*, int* );

static void _recalc_layout ( SMS_JPEGViewer* apViewer, SMS_JPEGContext* apCtx ) {

 unsigned int   lWidth   = apViewer -> m_Width;
 unsigned int   lHeight  = apViewer -> m_Height;
 unsigned int   i, lTBW  = ( lWidth + 63 ) >> 6;
 unsigned int   lImgSize = (   ( lTBW << 6 ) * (  ( lHeight + 31 ) & ~31  ) * 4   ) >> 8;
 unsigned int   lVRAM    = 0x4000 - lImgSize;
 unsigned int   lnParts;
 unsigned int   lnRem;
 unsigned int   lScrW, lScrH;
 unsigned int   lTW, lTH;
 unsigned long* lpDMA;
 unsigned char* lpData;
 unsigned long* lpXYZ;
 unsigned int   lDispCoord[ 2 ][ 4 ];
 unsigned int   lBorder;

/* format image upload packet */
 PowerOf2 ( lWidth, lHeight, &lTW, &lTH );

 lnParts = apCtx -> m_nBitmapQWC / 0x7FFF;
 lnRem   = apCtx -> m_nBitmapQWC % 0x7FFF;

 lpDMA = UNCACHED_SEG( apViewer -> m_BitBltPack );

 lpDMA[  0 ] = DMA_TAG( 5, 0, DMATAG_ID_CNT, 0, 0, 0 );
 lpDMA[  1 ] = 0UL;
 lpDMA[  2 ] = GIF_TAG( 4, 0, 0, 0, GIFTAG_FLG_PACKED, 1 );
 lpDMA[  3 ] = GIFTAG_REGS_AD;
 lpDMA[  4 ] = GS_SET_TRXREG( lWidth, lHeight );
 lpDMA[  5 ] = GS_TRXREG;
 lpDMA[  6 ] = GS_SET_BITBLTBUF( 0, 0, GSPixelFormat_PSMCT24, lVRAM, lTBW, GSPixelFormat_PSMCT24 );
 lpDMA[  7 ] = GS_BITBLTBUF;
 lpDMA[  8 ] = GS_SET_TRXPOS( 0, 0, 0, 0, GS_TRXPOS_DIR_LR_UD );
 lpDMA[  9 ] = GS_TRXPOS;
 lpDMA[ 10 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
 lpDMA[ 11 ] = GS_TRXDIR;
 lpDMA      += 12;
 lpData      = apCtx -> m_pBitmap;

 for ( i = 0; i < lnParts; ++i ) {

  *lpDMA++ = DMA_TAG( 1, 0, DMATAG_ID_CNT, 0, 0, 0 );
  *lpDMA++ = 0LL;
  *lpDMA++ = GIF_TAG( 0x7FFF, 0, 0, 0, GIFTAG_FLG_IMAGE, 0 );
  *lpDMA++ = 0UL;
  *lpDMA++ = DMA_TAG(  0x7FFF, 0, DMATAG_ID_REF, 0, ( unsigned int )lpData, 0  );
  *lpDMA++ = 0UL;

  lpData += 0x7FFF << 4;

 }  /* end for */

 if ( lnRem ) {

  *lpDMA++ = DMA_TAG( 1, 0, DMATAG_ID_CNT, 0, 0, 0 );
  *lpDMA++ = 0UL;
  *lpDMA++ = GIF_TAG( lnRem, 0, 0, 0, GIFTAG_FLG_IMAGE, 0 );
  *lpDMA++ = 0UL;
  *lpDMA++ = DMA_TAG(  lnRem, 0, DMATAG_ID_REF, 0, ( unsigned int )lpData, 0  );
  *lpDMA++ = 0UL;

 }  /* end if */

 *lpDMA++ = DMA_TAG( 4, 0, DMATAG_ID_RET, 0, 0, 0 );
 *lpDMA++ = 0LL;
 *lpDMA++ = GIF_TAG( 3, 1, 0, 0, GIFTAG_FLG_PACKED, 1 );
 *lpDMA++ = GIFTAG_REGS_AD;
 *lpDMA++ = GS_SET_TEX1( 0, 0, 1, 1, 0, 0, 0 );
 *lpDMA++ = GS_TEX1_1;
 *lpDMA++ = GS_SET_TEXFLUSH( 0 );
 *lpDMA++ = GS_TEXFLUSH;
 *lpDMA++ = GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x00, 0x00 );
 *lpDMA++ = GS_RGBAQ;

 apViewer -> m_TextCoord[ 0 ] = 8;
 apViewer -> m_TextCoord[ 1 ] = 8;
 apViewer -> m_TextCoord[ 2 ] = (  ( lWidth  - 1 ) << 4  ) - 8;
 apViewer -> m_TextCoord[ 3 ] = (  ( lHeight - 1 ) << 4  ) - 8;
/* calculate best fit for normal display */
 _calc_best_fit ( lScrW = g_GSCtx.m_Width, lScrH = g_GSCtx.m_Height, lWidth,  lHeight, lDispCoord[ 1 ] );

 if ( lWidth < lScrW && lHeight < lScrH ) {

  lDispCoord[ 0 ][ 0 ] = ( lScrW - lWidth  ) >> 1;
  lDispCoord[ 0 ][ 1 ] = ( lScrH - lHeight ) >> 1;
  lDispCoord[ 0 ][ 2 ] = lDispCoord[ 0 ][ 0 ] + lWidth;
  lDispCoord[ 0 ][ 3 ] = lDispCoord[ 0 ][ 1 ] + lHeight;
  lpData               = ( void* )1;
 
 } else {

  lDispCoord[ 0 ][ 0 ] = lDispCoord[ 1 ][ 0 ];
  lDispCoord[ 0 ][ 1 ] = lDispCoord[ 1 ][ 1 ];
  lDispCoord[ 0 ][ 2 ] = lDispCoord[ 1 ][ 2 ];
  lDispCoord[ 0 ][ 3 ] = lDispCoord[ 1 ][ 3 ];
  lpData               = ( void* )0;

 }  /* end else */

 lpXYZ = UNCACHED_SEG( apViewer -> m_XYZ );

 lpXYZ[ 0 ] = GS_XYZ ( lDispCoord[ 0 ][ 0 ], lDispCoord[ 0 ][ 1 ], 0 );
 lpXYZ[ 1 ] = GS_XYZ ( lDispCoord[ 0 ][ 2 ], lDispCoord[ 0 ][ 3 ], 0 );
 lpXYZ[ 2 ] = GS_XYZ ( lDispCoord[ 1 ][ 0 ], lDispCoord[ 1 ][ 1 ], 0 );
 lpXYZ[ 3 ] = GS_XYZ ( lDispCoord[ 1 ][ 2 ], lDispCoord[ 1 ][ 3 ], 0 );

 lpXYZ += ( apViewer -> m_Mode << 1 );

 lpDMA = UNCACHED_SEG( apViewer -> m_ImagePack );

 lpDMA[  0 ] = GIF_TAG( 1, 1, 0, 0, 1, 6 );
 lpDMA[  1 ] = ( GS_TEX0_1 <<  0 ) | ( GS_PRIM <<  4 ) |
               ( GS_UV     <<  8 ) | ( GS_XYZ2 << 12 ) |
               ( GS_UV     << 16 ) | ( GS_XYZ2 << 20 );
 lpDMA[  2 ] = GS_SET_TEX0( lVRAM, lTBW, GSPixelFormat_PSMCT24, lTW, lTH, 0, 1, 0, 0, 0, 0, 0 );
 lpDMA[  3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0 );
 lpDMA[  4 ] = GS_SET_UV( apViewer -> m_TextCoord[ 0 ], apViewer -> m_TextCoord[ 1 ] );
 lpDMA[  5 ] = lpXYZ[ 0 ];
 lpDMA[  6 ] = GS_SET_UV( apViewer -> m_TextCoord[ 2 ], apViewer -> m_TextCoord[ 3 ] );
 lpDMA[  7 ] = lpXYZ[ 1 ];

 lpDMA = UNCACHED_SEG( apViewer -> m_BordPackZ );

 lpDMA[  0 ] = DMA_TAG( 6, 0, DMATAG_ID_RET, 0, 0, 0 );
 lpDMA[  1 ] = 0UL;
 lpDMA[  2 ] = GIF_TAG( 5, 1, 0, 0, GIFTAG_FLG_PACKED, 1 );
 lpDMA[  3 ] = GIFTAG_REGS_AD;
 lpDMA[  4 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpDMA[  5 ] = GS_PRIM;
 lpDMA[  6 ] = GS_XYZ ( 0, 0, 0 );
 lpDMA[  7 ] = GS_XYZ2;
 lpDMA[  9 ] = GS_XYZ2;
 lpDMA[ 11 ] = GS_XYZ2;
 lpDMA[ 12 ] = GS_XYZ ( lScrW, lScrH, 0 );
 lpDMA[ 13 ] = GS_XYZ2;

 if ( lDispCoord[ 1 ][ 0 ] ) {
  lpDMA[  8 ] = GS_XYZ ( lDispCoord[ 1 ][ 0 ], lScrH, 0 );
  lpDMA[ 10 ] = GS_XYZ ( lDispCoord[ 1 ][ 2 ],     0, 0 );
 } else {
  lpDMA[  8 ] = GS_XYZ( lScrW, lDispCoord[ 1 ][ 1 ], 0 );
  lpDMA[ 10 ] = GS_XYZ(     0, lDispCoord[ 1 ][ 3 ], 0 );
 }  /* end else */

 lpDMA = UNCACHED_SEG( apViewer -> m_BordPack );

 if ( lpData ) {

  lpDMA[  0 ] = DMA_TAG( 10, 0, DMATAG_ID_RET, 0, 0, 0 );
  lpDMA[  1 ] = 0UL;
  lpDMA[  2 ] = GIF_TAG( 9, 1, 0, 0, GIFTAG_FLG_PACKED, 1 );
  lpDMA[  3 ] = GIFTAG_REGS_AD;
  lpDMA[  4 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
  lpDMA[  5 ] = GS_PRIM;
  lpDMA[  6 ] = GS_XYZ ( 0, 0, 0 );
  lpDMA[  7 ] = GS_XYZ2;
  lpDMA[  8 ] = GS_XYZ ( lScrW, lDispCoord[ 0 ][ 1 ], 0 );
  lpDMA[  9 ] = GS_XYZ2;
  lpDMA[ 10 ] = GS_XYZ ( 0, lDispCoord[ 0 ][ 1 ], 0 );
  lpDMA[ 11 ] = GS_XYZ2;
  lpDMA[ 12 ] = GS_XYZ ( lDispCoord[ 0 ][ 0 ], lDispCoord[ 0 ][ 3 ], 0 );
  lpDMA[ 13 ] = GS_XYZ2;
  lpDMA[ 14 ] = GS_XYZ ( lDispCoord[ 0 ][ 2 ], lDispCoord[ 0 ][ 1 ], 0 );
  lpDMA[ 15 ] = GS_XYZ2;
  lpDMA[ 16 ] = GS_XYZ ( lScrW, lDispCoord[ 0 ][ 3 ], 0 );
  lpDMA[ 17 ] = GS_XYZ2;
  lpDMA[ 18 ] = GS_XYZ ( 0, lDispCoord[ 0 ][ 3 ], 0 );
  lpDMA[ 19 ] = GS_XYZ2;
  lpDMA[ 20 ] = GS_XYZ ( lScrW, lScrH, 0 );
  lpDMA[ 21 ] = GS_XYZ2;

 } else memcpy (  lpDMA, UNCACHED_SEG( apViewer -> m_BordPackZ ), sizeof ( apViewer -> m_BordPackZ )  );

 lpDMA = UNCACHED_SEG( apViewer -> m_DrawPack );

 lBorder    = ( unsigned int )( apViewer -> m_Mode ? apViewer -> m_BordPackZ : apViewer -> m_BordPack );
 lpDMA[ 0 ] = DMA_TAG(  0, 0, DMATAG_ID_CALL, 0, ( unsigned int )apViewer -> m_BitBltPack, 0  );
 lpDMA[ 1 ] = 0UL;
 lpDMA[ 2 ] = DMA_TAG(  4, 0, DMATAG_ID_REF,  0, ( unsigned int )apViewer -> m_ImagePack,  0  );
 lpDMA[ 3 ] = 0UL;
 lpDMA[ 4 ] = DMA_TAG(  0, 0, DMATAG_ID_CALL, 0, lBorder, 0  );
 lpDMA[ 5 ] = 0UL;
 lpDMA[ 6 ] = DMA_TAG( 0, 0, DMATAG_ID_END, 0, 0, 0 );
 lpDMA[ 7 ] = 0UL;

}  /* end _recalc_layout */

static void _display_picture ( SMS_JPEGViewer* apViewer ) {

 GS_VSync2 ( g_GSCtx.m_DrawDelay );
 DMA_SendChainA ( DMAC_GIF, apViewer -> m_DrawPack );
 DMA_Wait ( DMAC_GIF );

}  /* end _display_picture */

static void _display_text ( SMS_JPEGViewer* apViewer, const char* apFileName ) {

 char         lBuf[ 1024 ];
 char*        lpPtr;
 int          lLen;
 int          lDW     = 0;
 unsigned int lW      = apViewer -> m_StsW - 24;
 int          lfTrunc = 0;

 strcpy ( lBuf, STR_LOADING.m_pStr );

 lpPtr = strchr ( lBuf, '%' );

 if ( lpPtr ) *lpPtr = '\x00';

 strcat ( lBuf, g_pQuote );

 lpPtr = &lBuf[ strlen ( lBuf ) ];

 while ( 1 ) {

  strcpy ( lpPtr, apFileName );
  strcat ( lpPtr, g_pQuote   );
  strcat ( lpPtr, g_pPeriod  );

  lLen = strlen ( lBuf );

  if (  GSFont_WidthEx ( lBuf, lLen, lDW ) < lW  ) break;

  if ( lDW <= -12 ) {
   lDW = -12;
   ++apFileName;
   if ( !lfTrunc ) {
    strcpy ( lpPtr, g_pPeriod );
    lpPtr  += 3;
    lfTrunc = 1;
   }  /* end if */
  } else --lDW;

 }  /* end while */

 GSContext_DeleteList ( apViewer -> m_pStsText );
 apViewer -> m_pStsText = GSContext_NewList (  GS_TXT_PACKET_SIZE( lLen )  );

 GSFont_RenderEx ( lBuf, lLen, apViewer -> m_StsX + 12, apViewer -> m_StsY + 12, apViewer -> m_pStsText, lDW, 0 );

 GSContext_CallList ( 0, apViewer -> m_pStsBkgnd   );
 GSContext_CallList ( 0, apViewer -> m_pStsOutline );
 GSContext_CallList ( 0, apViewer -> m_pStsText    );
 GSContext_CallList ( 0, apViewer -> m_pStsPgBg    );

 GS_VSync ();
 GSContext_Flush ( 0, GSFlushMethod_DeleteLists );

}  /* end _display_text */

static void _error ( SMS_JPEGViewer* apViewer ) {

 unsigned long* lpDMA;
 unsigned int   lX, lY, lW, lH, lTX, lTY, lTW;
 char*          lpStr = STR_UNSUPPORTED_IMAGE.m_pStr;
 unsigned int   lLen  = STR_UNSUPPORTED_IMAGE.m_Len;
          int   lDW   = 0;
 unsigned long  lIcon[ 32 ] __attribute__(   (  aligned( 16 )  )   );

 lW = ( unsigned int )( g_GSCtx.m_Width  * 0.9F + 0.5F );
 lH = ( unsigned int )( g_GSCtx.m_Height * 0.1F + 0.5F );

 while (   (  lTW = GSFont_WidthEx ( lpStr, lLen, lDW )  ) > lW && lDW >= -16   ) --lDW;
 while (   (  lTW = GSFont_WidthEx ( lpStr, lLen, lDW )  ) > lW                 ) --lLen;

 lW  = lTW + 64;
 lX  = ( g_GSCtx.m_Width  - lW ) >> 1;
 lY  = ( g_GSCtx.m_Height - lH ) >> 1;
 lTX = lX + (  ( lW - lTW ) >> 1  ) + 16;
 lTY = lY + (  ( lH - 32  ) >> 1  );

 lpDMA = GSContext_NewPacket (  0, GS_RRT_PACKET_SIZE(), GSPaintMethod_Init  );
 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( lpDMA - 2 ),
  lX, lY, lW, lH, -12, 0x80000080
 );
 lpDMA = GSContext_NewPacket (  0, GS_RRT_PACKET_SIZE(), GSPaintMethod_Continue  );
 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( lpDMA - 2 ),
  lX, lY, lW, lH, 12, 0x60000080
 );

 GUI_DrawIcon ( GUICON_ERROR, lX + 8, lTY, GUIcon_Misc, lIcon );
 SyncDCache ( &lIcon[ 0 ], &lIcon[ 32 ] );
 GSContext_CallList2 ( 0, lIcon );

 lpDMA = GSContext_NewPacket (  0, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Continue  );
 GSFont_RenderEx ( lpStr, lLen, lTX, lTY, lpDMA, lDW, 0 );

 GS_VSync ();
 GSContext_Flush ( 0, GSFlushMethod_DeleteLists );

}  /* end _error */

void SMS_JPEGPlayerDestroy ( void* apPlayer ) {

 SMS_Player* lpPlayer = ( SMS_Player* )apPlayer;

 lpPlayer -> m_pCont -> Destroy ( lpPlayer -> m_pCont, 1 );
 lpPlayer -> m_pCont = NULL;

}  /* end SMS_JPEGPlayerDestroy */

typedef void ( *Transpose ) ( void*, int, int );

static void _rotate ( SMS_JPEGViewer* apViewer, SMS_JPEGContext* apCtx, Transpose aFirst, Transpose aSecond, unsigned int aDim0, unsigned int aDim1 ) {

 unsigned int lTmp = apViewer -> m_Width;

 SMS_PgIndStart ();
  aFirst  ( apCtx -> m_pBitmap, lTmp, apViewer -> m_Height );
  aSecond ( apCtx -> m_pBitmap, aDim0, aDim1 );
 SMS_PgIndStop  ();

 FlushCache ( 0 );

 apViewer -> m_Width  = apViewer -> m_Height;
 apViewer -> m_Height = lTmp;

 _recalc_layout ( apViewer, apCtx );
 _display_picture ( apViewer );

}   /* end _rotate */

void SMS_JPEGViewerPerform ( SMS_JPEGViewer* apViewer, SMS_List* apList ) {

 static unsigned s_Btn[] __attribute__(   (  section( ".data" )  )   ) = {
  SMS_PAD_LEFT, SMS_PAD_RIGHT, SMS_PAD_TRIANGLE, SMS_PAD_SQUARE, SMS_PAD_CROSS,
  SMS_PAD_CIRCLE, SMS_PAD_R1, RC_ANGLE, RC_SHUFFLE,
  RC_PLAY, RC_NEXT, RC_PREV, RC_SCAN_LEFT, RC_SCAN_RIGHT, RC_SLOW_LEFT,
  RC_SLOW_RIGHT, RC_DISPLAY, RC_RETURN, RC_ENTER, RC_STOP
 };

 SMS_JPEGContext* lpCtx = apViewer -> m_pCtx;
 char             lFileName[ 1024 ];
 char*            lpEOS;
 FileContext*     lpFileCtx;
 int              lfPic = 0;

 SMS_ListSort ( apList );

 apViewer -> m_pItems   = apList;
 apViewer -> m_pCurrent = apList -> m_pHead;

 if (   strchr (  _STR( apViewer -> m_pCurrent ), ':'  )   )
  lpEOS = &lFileName[ 0 ];
 else {
  int lLen = strlen ( g_CWD );
  strcpy ( lFileName, g_CWD );
  lpEOS = &lFileName[ lLen ];
  if (  g_CWD[ lLen - 1 ] != '/' ) {
   strcat (  lFileName, g_SlashStr );
   lpEOS += 1;
  }  /* end if */
 }  /* end else */

 while ( apViewer -> m_pCurrent ) {

  int lfError = 0;
  int lBtn;

  lpEOS[ 0 ] = '\x00'; strcat (  lFileName, _STR( apViewer -> m_pCurrent )  );

  lpFileCtx = STIO_InitFileContext ( lFileName, NULL );

  if ( lpFileCtx ) {

   void* lpData;

   _display_text ( apViewer, lFileName );

   lpFileCtx -> Stream ( lpFileCtx, 0, 32 );

   lpData = SMS_FileMappingMap ( lpFileCtx );

   if (  SMS_JPEGLoad ( apViewer -> m_pCtx, lpData, lpFileCtx -> m_Size )  ) {

    if (  apViewer -> m_Width  != lpCtx -> m_pRC -> m_NewWidth ||
          apViewer -> m_Height != lpCtx -> m_pRC -> m_NewHeight
    ) {
     apViewer -> m_Width  = lpCtx -> m_pRC -> m_NewWidth;
     apViewer -> m_Height = lpCtx -> m_pRC -> m_NewHeight;
     _recalc_layout ( apViewer, lpCtx );
    }  /* end if */

    lfPic = 1;

   } else lfError = 1;

   SMS_FileMappingUnMap ( lpData );

   lpFileCtx -> Destroy ( lpFileCtx );

  } else lfError = 1;

  if ( lfPic   ) _display_picture ( apViewer );
  if ( lfError ) {
   _display_text ( apViewer, lFileName );
   _error ( apViewer );
  }  /* end if */

  while (  GUI_ReadButtons ()  );
user_input:
  lBtn = GUI_WaitButtons (  sizeof ( s_Btn ) / sizeof ( s_Btn[ 0 ] ), s_Btn, 200  );

  switch ( lBtn ) {

   case SMS_PAD_RIGHT:
   case SMS_PAD_CROSS:
   case RC_PLAY:
   case RC_NEXT:
   case RC_SCAN_RIGHT:
   case RC_SLOW_RIGHT:
   case RC_ENTER:
    if ( apViewer -> m_pItems -> m_Size == 1 ) goto user_input;
    apViewer -> m_pCurrent = apViewer -> m_pCurrent -> m_pNext;
    if ( !apViewer -> m_pCurrent ) apViewer -> m_pCurrent = apViewer -> m_pItems -> m_pHead;
   break;

   case SMS_PAD_TRIANGLE:
   case RC_RETURN:
   case RC_STOP:
   return;

   case SMS_PAD_SQUARE:
   case RC_DISPLAY: {
    unsigned long* lpXYZ = UNCACHED_SEG( apViewer -> m_XYZ       );
    unsigned long* lpPkt = UNCACHED_SEG( apViewer -> m_ImagePack );
    unsigned int   lBorder;
    apViewer -> m_Mode ^= 1;
    lpXYZ     += ( apViewer -> m_Mode << 1 );
    lBorder    = ( unsigned int )( apViewer -> m_Mode ? apViewer -> m_BordPackZ : apViewer -> m_BordPack );
    lpPkt[ 5 ] = lpXYZ[ 0 ];
    lpPkt[ 7 ] = lpXYZ[ 1 ];
    lpPkt = UNCACHED_SEG( apViewer -> m_DrawPack );
    lpPkt[ 4 ] = DMA_TAG( 0, 0, DMATAG_ID_CALL, 0, lBorder, 0 );
    if ( lfPic )
     _display_picture ( apViewer );
    else _display_def_picture ( apViewer );
   } goto user_input;

   case SMS_PAD_LEFT:
   case RC_PREV:
   case RC_SCAN_LEFT:
   case RC_SLOW_LEFT:
    if ( apViewer -> m_pItems -> m_Size == 1 ) goto user_input;
    if ( !apViewer -> m_pCurrent -> m_pPrev )
     apViewer -> m_pCurrent = apViewer -> m_pItems -> m_pTail;
    else apViewer -> m_pCurrent = apViewer -> m_pCurrent -> m_pPrev;
   break;

   case SMS_PAD_CIRCLE:
   case RC_ANGLE      : if ( lfPic ) {
    _rotate ( apViewer, lpCtx, _transpose, _flip_v, apViewer -> m_Height, apViewer -> m_Width );
   } goto user_input;

   case SMS_PAD_R1:
   case RC_SHUFFLE: if ( lfPic ) {
    _rotate ( apViewer, lpCtx, _flip_v, _transpose, apViewer -> m_Width, apViewer -> m_Height );
   } goto user_input;

  }  /* end switch */

  if ( lfError ) {
   if ( lfPic )
    _display_picture ( apViewer );
   else _display_def_picture ( apViewer );
  }  /* end if */

 }  /* end while */

}  /* end SMS_JPEGViewerPerform */

int SMS_JPEGPlayerPlay ( void* apPlayer ) {

 SMS_Player*        lpPlayer = ( SMS_Player*        )apPlayer;
 SMS_ContainerJPEG* lpCont   = ( SMS_ContainerJPEG* )lpPlayer -> m_pCont -> m_pCtx;
 SMS_List*          lpList   = ( SMS_List*          )(    (   (  ( unsigned int )lpCont -> m_pFileList  ) << 1   ) >> 1    );

 SMS_JPEGViewerPerform ( lpCont -> m_pViewer, lpList );

 return 0;

}  /* end SMS_JPEGPlayerPlay */
