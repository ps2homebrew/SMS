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
#include "SMS_PgInd.h"
#include "SMS_GS.h"
#include "SMS_EE.h"
#include "SMS_DMA.h"
#include "SMS_Timer.h"

#include <kernel.h>
#include <stdio.h>

#define IND_SIZE 32

extern void* _gp;
extern int   g_XShift;

static int           s_ThreadID;
static int           s_HandlerID;
static unsigned char s_Stack  [                    4096 ] __attribute__(   (  aligned( 16 ), section( ".bss" )  )   );
static unsigned long s_DrawPkt[                     112 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned long s_SendPkt[                      24 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned int  s_Bitmap [ IND_SIZE * IND_SIZE * 4 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );

static const unsigned int s_Data[ 144 ] __attribute__(   (  aligned( 16 ), section( ".rodata" )  )   ) = {
 0x3F800000, 0x3F333333, 0x3F7C1C5C, 0x3F307A40,
 0x3F708FB2, 0x3F286496, 0x3F5DB3D7, 0x3F1B3116,
 0x3F441B7C, 0x3F094670, 0x3F248DB9, 0x3EE66003,
 0x3EFFFFFD, 0x3EB33331, 0x3EAF1D3F, 0x3E7528F2,
 0x3E31D0CA, 0x3DF8F11A, 0xB439FA42, 0xB4022F2E,
 0xBE31D0E1, 0xBDF8F13A, 0xBEAF1D4A, 0xBE752901,
 0xBF000003, 0xBEB33338, 0xBF248DBE, 0xBEE66009,
 0xBF441B80, 0xBF094673, 0xBF5DB3DA, 0xBF1B3118,
 0xBF708FB4, 0xBF286497, 0xBF7C1C5D, 0xBF307A41,
 0xBF800000, 0xBF333333, 0xBF7C1C5B, 0xBF307A40,
 0xBF708FB0, 0xBF286495, 0xBF5DB3D4, 0xBF1B3114,
 0xBF441B79, 0xBF09466E, 0xBF248DB5, 0xBEE65FFD,
 0xBEFFFFF3, 0xBEB3332A, 0xBEAF1D34, 0xBE7528E3,
 0xBE31D0B3, 0xBDF8F0FA, 0x350B7BB1, 0x34C346C5,
 0x3E31D0F8, 0x3DF8F15A, 0x3EAF1D55, 0x3E752911,
 0x3F000008, 0x3EB3333F, 0x3F248DC2, 0x3EE66010,
 0x3F441B84, 0x3F094676, 0x3F5DB3DD, 0x3F1B311A,
 0x3F708FB6, 0x3F286499, 0x3F7C1C5E, 0x3F307A42,
 0x00000000, 0x00000000, 0x3E31D0D5, 0x3DF8F12A,
 0x3EAF1D45, 0x3E7528FA, 0x3F000001, 0x3EB33334,
 0x3F248DBC, 0x3EE66006, 0x3F441B7E, 0x3F094672,
 0x3F5DB3D8, 0x3F1B3117, 0x3F708FB3, 0x3F286497,
 0x3F7C1C5D, 0x3F307A41, 0x3F800000, 0x3F333333,
 0x3F7C1C5C, 0x3F307A40, 0x3F708FB1, 0x3F286495,
 0x3F5DB3D5, 0x3F1B3115, 0x3F441B7A, 0x3F09466F,
 0x3F248DB7, 0x3EE66000, 0x3EFFFFF8, 0x3EB3332D,
 0x3EAF1D3A, 0x3E7528EA, 0x3E31D0BE, 0x3DF8F10A,
 0xB4B9FA42, 0xB4822F2E, 0xBE31D0EC, 0xBDF8F14A,
 0xBEAF1D50, 0xBE752909, 0xBF000006, 0xBEB3333B,
 0xBF248DC0, 0xBEE6600D, 0xBF441B82, 0xBF094674,
 0xBF5DB3DB, 0xBF1B3119, 0xBF708FB5, 0xBF286498,
 0xBF7C1C5E, 0xBF307A41, 0xBF800000, 0xBF333333,
 0xBF7C1C5B, 0xBF307A3F, 0xBF708FAF, 0xBF286494,
 0xBF5DB3D2, 0xBF1B3113, 0xBF441B77, 0xBF09466D,
 0xBF248DB3, 0xBEE65FFA, 0xBEFFFFEE, 0xBEB33326,
 0xBEAF1D2F, 0xBE7528DB, 0xBE31D0A7, 0xBDF8F0EA
};

static void _pgind_thread ( void* );

int _vblnk_handler    ( int, void*, void* );
int _vblnk_handler_pg ( int, void*, void* );

void SMS_PgIndInitialize ( void ) {

 if ( !s_ThreadID  ) {

  ee_thread_t lThreadParam;

  lThreadParam.func             = _pgind_thread;
  lThreadParam.stack            = s_Stack;
  lThreadParam.stack_size       = sizeof ( s_Stack );
  lThreadParam.gp_reg           = &_gp;
  lThreadParam.initial_priority = 32;
  s_ThreadID = CreateThread ( &lThreadParam );

 }  /* end if */

}  /* end SMS_PgIndInitialize */

void SMS_PgIndStart ( void ) {

 if ( !s_HandlerID ) {

  int lVMode = GS_Params () -> m_GSCRTMode;

  int ( *lpHandler ) ( int, void*, void* ) = _vblnk_handler_pg;

  if ( lVMode == GSVideoMode_NTSC ||
       lVMode == GSVideoMode_PAL  ||
       lVMode == GSVideoMode_DTV_1920x1080I
  ) lpHandler = _vblnk_handler;

  s_HandlerID = AddIntcHandler2 (  2, lpHandler, 0, ( void* )s_ThreadID  );

  StartThread ( s_ThreadID, NULL );
  EnableIntc ( 2 );

 }  /* end if */

}  /* end SMS_PgIndStart */

void SMS_PgIndStop ( void ) {

 if ( s_HandlerID ) {

  unsigned long* lpDMA = UNCACHED_SEG( &s_SendPkt[ 18 ] );

  lpDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );

  SMS_EEDIntr ();
   if (  RemoveIntcHandler ( 2, s_HandlerID ) <= 0  ) DisableIntc ( 2 );
  SMS_EEIntr ( 1 );

  TerminateThread ( s_ThreadID );

  s_HandlerID = 0;

  DMA_SendChain ( DMAC_GIF, s_SendPkt );
  DMA_Wait ( DMAC_GIF );

 }  /* end if */

}  /* end SMS_PgIndStop */

int _vblnk_handler ( int, void*, void* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".text\n\t"
 "_vblnk_handler:\n\t"
 "lui       $at, 0x1200\n\t"
 "ld        $at, 0x1000($at)\n\t"
 "dsrl      $at, $at, 13\n\t"
 "andi      $at, $at, 1\n\t"
 "bnel      $at, $zero, 1f\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "2:\n\t"
 "jr        $ra\n\t"
 "xor       $v0, $v0, $v0\n\t"
 "1:\n\t"
 "jal       iWakeupThread\n\t"
 "or        $a0, $zero, $a1\n\t"
 "beq       $zero, $zero, 2b\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

int _vblnk_handler_pg ( int, void*, void* );
__asm__(
 ".set noreorder\n\t"
 ".set nomacro\n\t"
 ".set noat\n\t"
 ".data\n\t"
 "s_Field:  .byte   0\n\t"
 ".text\n\t"
 "_vblnk_handler_pg:\n\t"
 "lui       $a0, %hi( s_Field )\n\t"
 "lbu       $at, %lo( s_Field )($a0)\n\t"
 "nor       $v0, $zero, $at\n\t"
 "bne       $at, $zero, 1f\n\t"
 "sb        $v0, %lo( s_Field )($a0)\n\t"
 "2:\n\t"
 "jr        $ra\n\t"
 "xor       $v0, $v0, $v0\n\t"
 "1:\n\t"
 "pcpyld    $ra, $ra, $ra\n\t"
 "jal       iWakeupThread\n\t"
 "or        $a0, $zero, $a1\n\t"
 "beq       $zero, $zero, 2b\n\t"
 "pcpyud    $ra, $ra, $ra\n\t"
 ".set at\n\t"
 ".set macro\n\t"
 ".set reorder\n\t"
);

static void _pgind_thread ( void* apArg ) {

 static float s_lAlpha = 128.0F;
 const  float c_Step   = 128.0F / 18.0F;

 int           i, j;
 int           lDrawX  = ( g_GSCtx.m_Width  - IND_SIZE ) >> 1;
 int           lDrawY  = ( g_GSCtx.m_Height - IND_SIZE ) >> 1;
 int           lSendX  = ( lDrawX - IND_SIZE ) >> g_XShift;
 int           lSendY  = lDrawY - IND_SIZE;
 int           lSendW  = ( IND_SIZE << 1 ) >> g_XShift;
 GSPixelFormat lPSM    = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.PSM;
 unsigned int  lFBW    = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.FBW;
 float         lAR     = GS_Params () -> m_AspectRatio[ 0 ];
 int           lPSendY = ( int )( lSendY * lAR );
 int           lPSendH = ( int )(  ( IND_SIZE << 1 ) * lAR  ) + 2;
 int           lQWC    = (   (   lSendW * lPSendH * (  2 + ( lPSM == GSPixelFormat_PSMCT24 )  )   ) + 15    ) >> 4;
 float         lSO[ 3 ];
 GSStoreImage  lStoreParam;
 unsigned long lDMA[ 4 ] __attribute__(   (  aligned( 16 )  )   );

 lSO[ 0 ] = ( float )IND_SIZE;
 lSO[ 1 ] = lDrawX;
 lSO[ 2 ] = lDrawY;

 lDMA[ 0 ] = DMA_TAG(  0, 0, DMATAG_ID_CALL, 0, s_SendPkt, 0 );
 lDMA[ 2 ] = DMA_TAG( 55, 0, DMATAG_ID_REFE, 0, s_DrawPkt, 0 );

 s_SendPkt[  0 ] = DMA_TAG( 6, 0, DMATAG_ID_CNT, 0, 0, 0 );
 s_SendPkt[  1 ] = 0L;
 s_SendPkt[  2 ] = GIF_TAG( 4, 0, 0, 0, 0, 1 );
 s_SendPkt[  3 ] = GIFTAG_REGS_AD;
 s_SendPkt[  4 ] = GS_SET_TRXREG( lSendW, lPSendH );
 s_SendPkt[  5 ] = GS_TRXREG;
 s_SendPkt[  6 ] = GS_SET_BITBLTBUF( 0, 0, lPSM, 0, lFBW, lPSM );
 s_SendPkt[  7 ] = GS_BITBLTBUF;
 s_SendPkt[  8 ] = GS_SET_TRXPOS( 0, 0, lSendX, lPSendY, GS_TRXPOS_DIR_LR_UD );
 s_SendPkt[  9 ] = GS_TRXPOS;
 s_SendPkt[ 10 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
 s_SendPkt[ 11 ] = GS_TRXDIR;
 s_SendPkt[ 12 ] = GIF_TAG( lQWC, 0, 0, 0, 2, 0 );
 s_SendPkt[ 13 ] = 0L;
 s_SendPkt[ 14 ] = DMA_TAG(  lQWC, 0, DMATAG_ID_REF, 0, ( unsigned int )s_Bitmap, 0  );
 s_SendPkt[ 15 ] = 0L;
 s_SendPkt[ 16 ] = DMA_TAG( 2, 0, DMATAG_ID_RET, 0, 0, 0 );
 s_SendPkt[ 17 ] = 0L;
 s_SendPkt[ 18 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 s_SendPkt[ 19 ] = GIFTAG_REGS_AD;
 s_SendPkt[ 20 ] = GS_SET_TEXFLUSH( 0 );
 s_SendPkt[ 21 ] = GS_TEXFLUSH;

 s_DrawPkt[ 0 ] = GIF_TAG( 18, 1, 0, 0, 1, 6 );
 s_DrawPkt[ 1 ] = ( GS_PRIM <<  0 ) | ( GS_RGBAQ <<  4 ) |
                  ( GS_XYZ2 <<  8 ) | ( GS_XYZ2  << 12 ) |
                  ( GS_XYZ2 << 16 ) | ( GS_XYZ2  << 20 );
 GS_XYZv (  &s_DrawPkt[ 2 ], ( float* )s_Data, 72, lSO, 0  );

 for ( i = 73, j = 109; i > 3; i -= 4, j -= 6 ) {
  unsigned long lVal0 = s_DrawPkt[ i - 0 ];
  unsigned long lVal1 = s_DrawPkt[ i - 1 ];
  unsigned long lVal2 = s_DrawPkt[ i - 2 ];
  unsigned long lVal3 = s_DrawPkt[ i - 3 ];
  s_DrawPkt[ j - 5 ] = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 0, 0, 0, 1, 0, 0, 0, 0 );
  s_DrawPkt[ j - 4 ] = GS_SET_RGBAQ( 0xFF, 0x00, 0x00, 0x80, 0x00 );
  s_DrawPkt[ j - 3 ] = lVal0;
  s_DrawPkt[ j - 2 ] = lVal1;
  s_DrawPkt[ j - 1 ] = lVal2;
  s_DrawPkt[ j - 0 ] = lVal3;
 }  /* end for */

 GS_InitStoreImage ( &lStoreParam, 0, lSendX, lPSendY, lSendW, lPSendH );
 FlushCache ( 0 );
 GS_StoreImage ( &lStoreParam, s_Bitmap );

 while ( 1 ) {

  float          lRA   = s_lAlpha;
  unsigned long* lpDMA = UNCACHED_SEG( &s_DrawPkt[ 3 ] );

  SleepThread ();

  for ( i = 0; i < 110; i += 6 ) {
   int lA = ( int )( lRA + 0.5F );
   lpDMA[ i ] = GS_SET_RGBAQ( 0xFF, 0x00, 0x00, lA, 0x00 );
   lRA -= c_Step;
   if ( lRA < 0.0F ) lRA = 128.0F;
  }  /* end for */

  s_lAlpha -= c_Step;

  if ( s_lAlpha <= 0.0F ) s_lAlpha = 128.0F;

  DMA_SendChain ( DMAC_GIF, lDMA );

 }  /* end while */

}  /* end _pgind_thread */
