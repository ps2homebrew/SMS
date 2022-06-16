/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUIClock.h"

#include "SMS_CDDA.h"
#include "SMS_DateTime.h"
#include "SMS_DMA.h"
#include "SMS_GS.h"
#include "SMS_Timer.h"
#include "SMS_EE.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>

extern void* _gp;

GUIClockParam g_Clock;

static unsigned long s_PrevTimer;
static int           s_ThreadID;
static int           s_HandlerID;
static void*         s_pImg;
static unsigned long s_DMA    [  8 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned long s_SendPkt[ 24 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned long s_DrawPkt[ 48 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );

static void _clock_thread  ( void*             );
static int  _vblnk_handler ( int, void*, void* );

void SMS_GUIClockInitialize ( void ) {

 static unsigned char s_lStack[ 4096 ] __attribute__(   (  aligned( 16 ), section( ".bss" )  )   );

 if ( !s_ThreadID ) {

  ee_thread_t lThreadParam;

  lThreadParam.func             = _clock_thread;
  lThreadParam.stack            = s_lStack;
  lThreadParam.stack_size       = sizeof ( s_lStack );
  lThreadParam.gp_reg           = &_gp;
  lThreadParam.initial_priority = 33;
  s_ThreadID = CreateThread ( &lThreadParam );

 }  /* end if */

}  /* end SMS_GUIClockInitialize */

void SMS_GUIClockStart ( const GUIClockParam* apParam ) {

 if ( !s_HandlerID ) {

  s_PrevTimer = g_Timer;
  s_HandlerID = AddIntcHandler2 (  2, _vblnk_handler, 0, ( void* )s_ThreadID  );

  StartThread (  s_ThreadID, ( void* )apParam  );
  WakeupThread ( s_ThreadID );
  EnableIntc ( 2 );

 }  /* end if */

}  /* end SMS_GUIClockStart */

void SMS_GUIClockStop ( void ) {

 if ( s_HandlerID ) {

  SMS_EEDIntr ();
   if (  RemoveIntcHandler ( 2, s_HandlerID ) <= 0  ) DisableIntc ( 2 );
  SMS_EEIntr ( 1 );

  TerminateThread ( s_ThreadID );

  s_HandlerID = 0;

  if ( s_pImg ) {
   free ( s_pImg );
   s_pImg = NULL;
  }  /* end if */

 }  /* end if */

}  /* end SMS_GUIClockStop */

void SMS_GUIClockSuspend ( void ) {

 if ( s_HandlerID ) {
  SMS_EEDIntr ();
   if (  RemoveIntcHandler ( 2, s_HandlerID ) <= 0  ) DisableIntc ( 2 );
  SMS_EEIntr ( 1 );
  SuspendThread ( s_ThreadID );
 }  /* end if */

}  /* end SMS_GUIClockSuspend */

void SMS_GUIClockResume ( void ) {

 if ( s_HandlerID ) {
  DMA_SendChain ( DMAC_GIF, s_DMA );
  s_HandlerID = AddIntcHandler2 (  2, _vblnk_handler, 0, ( void* )s_ThreadID  );
  ResumeThread ( s_ThreadID );
  EnableIntc ( 2 );
 }  /* end if */

}  /* end SMS_GUIClockResume */

void SMS_GUIClockRedraw ( void ) {

 DMA_SendChain ( DMAC_GIF, s_DMA );
 DMA_Wait ( DMAC_GIF );

}  /* end SMS_GUIClockRedraw */

static int _vblnk_handler ( int aCause, void* apArg, void* apAddr ) {

 if ( g_Timer - s_PrevTimer >= 500 ) {
  s_PrevTimer = g_Timer;
  iWakeupThread (  ( int )apArg  );
 }  /* end if */

 return 0;

}  /* end _vblnk_handler */

static void _clock_thread ( void* apParam ) {

 static const char s_lFmtAM[] __attribute__(   (  aligned( 1 ), section( ".rodata" )  )   ) = "%2ld:%02ld AM";
 static const char s_lFmtPM[] __attribute__(   (  aligned( 1 ), section( ".rodata" )  )   ) = "%2ld:%02ld PM";
 static const char s_lFmt24[] __attribute__(   (  aligned( 1 ), section( ".rodata" )  )   ) = "%2ld:%02ld";
 static const char s_lFmUSA[] __attribute__(   (  aligned( 1 ), section( ".rodata" )  )   ) = "00:00 00";
 static const char s_lFmEUR[] __attribute__(   (  aligned( 1 ), section( ".rodata" )  )   ) = "00:00";

 CDDA_RTC             lRTC;
 unsigned long        lTime;
 unsigned long        lTimeBase;
 unsigned long        lMinutes;
 unsigned long        lHour;
 unsigned long        lMins;
 unsigned char        lHalf;
 int                  lTimeFormat;
 const char*          lpFmt;
 const GUIClockParam* lpCParam = ( const GUIClockParam* )apParam;
 float                lAR      = GS_Params () -> m_AspectRatio[ 0 ];
 int                  lPY      = ( int )( lpCParam -> m_Y * lAR );
 int                  lPH      = ( int )( lpCParam -> m_H * lAR );
 GSPixelFormat        lPSM     = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.PSM;
 unsigned int         lFBW     = g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.FBW;
 int                  lQWC     = (   (   lpCParam -> m_W * lPH * (  2 + ( lPSM == GSPixelFormat_PSMCT24 )  )   ) + 15    ) >> 4;
 unsigned long*       lpTex0, *lpDrawPkt;
 GSStoreImage         lStoreParam;
 int                  lX, lDX;
 int                  lDY;
 int                  lLen;

 CDDA_ReadClock ( &lRTC );
 SMS_LocalTime  ( &lRTC );

 GS_InitStoreImage ( &lStoreParam, 0, lX = lpCParam -> m_X, lPY, lpCParam -> m_W, lPH );

 s_pImg = memalign (  64, lDY = (  ( lQWC << 4 ) + 63  ) & ~63  );

 s_SendPkt[  0 ] = DMA_TAG( 6, 0, DMATAG_ID_CNT, 0, 0, 0 );
 s_SendPkt[  1 ] = 0L;
 s_SendPkt[  2 ] = GIF_TAG( 4, 0, 0, 0, 0, 1 );
 s_SendPkt[  3 ] = GIFTAG_REGS_AD;
 s_SendPkt[  4 ] = GS_SET_TRXREG( lpCParam -> m_W, lPH );
 s_SendPkt[  5 ] = GS_TRXREG;
 s_SendPkt[  6 ] = GS_SET_BITBLTBUF( 0, 0, lPSM, 0, lFBW, lPSM );
 s_SendPkt[  7 ] = GS_BITBLTBUF;
 s_SendPkt[  8 ] = GS_SET_TRXPOS( 0, 0, lpCParam -> m_X, lPY, GS_TRXPOS_DIR_LR_UD );
 s_SendPkt[  9 ] = GS_TRXPOS;
 s_SendPkt[ 10 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
 s_SendPkt[ 11 ] = GS_TRXDIR;
 s_SendPkt[ 12 ] = GIF_TAG( lQWC, 0, 0, 0, 2, 0 );
 s_SendPkt[ 13 ] = 0L;
 s_SendPkt[ 14 ] = DMA_TAG(  lQWC, 0, DMATAG_ID_REF, 0, ( unsigned int )s_pImg, 0  );
 s_SendPkt[ 15 ] = 0L;
 s_SendPkt[ 16 ] = DMA_TAG( 2, 0, DMATAG_ID_RET, 0, 0, 0 );
 s_SendPkt[ 17 ] = 0L;
 s_SendPkt[ 18 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 s_SendPkt[ 19 ] = GIFTAG_REGS_AD;
 s_SendPkt[ 20 ] = GS_SET_TEXFLUSH( 0 );
 s_SendPkt[ 21 ] = GS_TEXFLUSH;

 lTime       = ( lRTC.m_Second + lRTC.m_Minute * 60 + lRTC.m_Hour * 3600 ) * 1000;
 lTimeBase   = g_Timer;
 lHalf       = 0;
 lMinutes    = -1;
 lTimeFormat = SMS_TimeFormat ();

 lDY = lpCParam -> m_H - 32;

 if ( lTimeFormat ) {
  lpFmt = s_lFmUSA;
  lQWC  = GS_TXT_PACKET_SIZE( 8 ) >> 1;
  lLen  = 8;
 } else {
  lpFmt = s_lFmEUR;
  lQWC  = GS_TXT_PACKET_SIZE( 5 ) >> 1;
  lLen  = 5;
 }  /* end else */

 lDX = 0;
 lPY = GSFont_WidthEx (  ( char* )lpFmt, lLen, 0  );

 if ( lPY >= lpCParam -> m_W )
  while ( lPY >= lpCParam -> m_W ) lPY = GSFont_WidthEx (  ( char* )lpFmt, lLen, --lDX  );
 else while ( lPY <= lpCParam -> m_W ) lPY = GSFont_WidthEx (  ( char* )lpFmt, lLen, ++lDX  );

 if ( lPY > lpCParam -> m_W ) --lDX;

 lPY = GSFont_WidthEx (  ( char* )lpFmt, lLen, lDX  );
 lX  = lpCParam -> m_X + (  ( lpCParam -> m_W - lPY ) >> 1  );

 s_DMA[ 0 ] = DMA_TAG(    0, 0, DMATAG_ID_CALL, 0, s_SendPkt, 0 );
 s_DMA[ 2 ] = DMA_TAG( lQWC, 0, DMATAG_ID_REFE, 0, s_DrawPkt, 0 );

 FlushCache ( 0 );
 GS_StoreImage ( &lStoreParam, s_pImg );

 lpDrawPkt = UNCACHED_SEG( s_DrawPkt );

 if ( g_GSCtx.m_FontTexFmt != GSPixelFormat_PSMT4HL )
  lpTex0 = lpDrawPkt + 6;
 else lpTex0 = lpDrawPkt + 2;

 while ( 1 ) {

  unsigned long  lNewMinutes;
  char           lBuf[ 12 ];
  GSRegTEX0      lTex0;

  SleepThread ();

  lHalf    ^= 1;
  lTime    += g_Timer - lTimeBase;
  lTimeBase = g_Timer;

  if ( lTime > 86400000 ) lTime = lTime - 86400000;

  lNewMinutes = lTime / 1000 / 60;

  if ( lNewMinutes != lMinutes ) {

   lHour    = lNewMinutes / 60;
   lMins    = lNewMinutes % 60;
   lMinutes = lNewMinutes;

   if ( lTimeFormat == 1 ) {
    if ( lHour > 12 ) {
     lHour -= 12;
     lpFmt  = s_lFmtPM;
    } else lpFmt = s_lFmtAM;
   } else lpFmt = s_lFmt24;

   sprintf ( lBuf, lpFmt, lHour, lMins );

  }  /* end if */

  lBuf[ 2 ] = lHalf ? ':' : ' ';

  GSFont_RenderEx ( lBuf, lLen, lX, lpCParam -> m_Y, lpDrawPkt, lDX, lDY );

  lTex0.m_Value = lpTex0[ 0 ];
  lTex0.CSA     = 0;
  lpTex0[ 0 ]   = lTex0.m_Value;

  DMA_SendChain ( DMAC_GIF, s_DMA );

 }  /* end while */

}  /* end _clock_thread */
