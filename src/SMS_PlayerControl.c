/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 bix64
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_PlayerControl.h"
#include "SMS_Container.h"
#include "SMS_VideoBuffer.h"
#include "GS.h"
#include "GUI.h"
#include "VIF.h"
#include "IPU.h"
#include "SPU.h"
#include "DMA.h"
#include "PAD.h"
#include "Timer.h"
#include "Config.h"
#include "SubtitleContext.h"
#include "StringList.h"

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define QWC_TD_TOTAL 20
#define QWC_TD_PRSTS 28

static uint64_t s_TDTotal[ FONT_GSP_SIZE(  8 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_TDPTS  [ FONT_GSP_SIZE( 12 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );

#define QWC_OSD_AV 12
#define QWC_OSD_SV 12
#define QWC_OSD_DT 16

static uint64_t s_OSDAV[ FONT_GSP_SIZE( 4 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_OSDSV[ FONT_GSP_SIZE( 4 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_OSDDT[ FONT_GSP_SIZE( 6 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_OSDNR[ FONT_GSP_SIZE( 8 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );

static uint64_t s_VCPaint [  66 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_VCErase [  16 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_SCPaint [ 274 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_SCErase [  16 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_IntTime;

static uint64_t s_DummyErase[  6 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_DummyPaint[ 24 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );

static StringList*     s_pLang;
static StringListNode* s_pCurLang;

extern SMS_Player s_Player;

static void _osd_timer_handler_incr ( void ) {

 PlayerControl_MkTime ( s_Player.m_AudioTime, 0 );

}  /* end _osd_timer_handler_incr */

static void _osd_timer_handler_decr ( void ) {

 PlayerControl_MkTime ( s_Player.m_pCont -> m_Duration - s_Player.m_AudioTime, 1 );

}  /* end _osd_timer_handler_decr */

static void TimerHandler ( void ) {

 if ( g_Timer >= s_IntTime ) {

  g_IPUCtx.iQueuePacket ( 8, s_VCErase );
  Timer_iRegisterHandler ( 1, NULL );

 }  /* end if */

}  /* end TimerHandler */

int PlayerControl_Index2Volume ( void ) {

 static unsigned s_lScale[ 25 ] = {
      0,   150,   400,   560,   800,
   1070,  1330,  1530,  1730,  2190,
   2550,  3010,  3520,  4080,  4490,
   5250,  5970,  6990,  8000,  9080,
  10450, 12030, 13560, 14940, 16383
 };

 return s_lScale[ g_Config.m_PlayerVolume = SMS_clip ( g_Config.m_PlayerVolume, 0, 24 ) ];

}  /* end PlayerControl_Index2Volume */

static void _FormatTime ( char* apBuf, uint64_t aTime ) {

 int lS = ( int )( aTime / SMS_TIME_BASE );
 int lM = lS / 60;
 int lH = lM / 60;

 lS %= 60;
 lM %= 60;

 sprintf ( apBuf, "%1d:%02d:%02d", lH, lM, lS );

}  /* end _FormatTime */

void PlayerControl_Init ( void ) {

 GSVertex   lPoints[ 8 ];
 uint64_t   lX      = ( 20 << 4 ) + ( g_GSCtx.m_OffsetX  << 4 );
 uint64_t   lY      = (   (  ( g_GSCtx.m_Height - 288 ) >> 1  ) << 3   ) + ( g_GSCtx.m_OffsetY << 4 );
 uint64_t   lDX     = 30 << 4;
 uint64_t*  lpPaint = _U( s_VCPaint );
 uint64_t*  lpErase = _U( s_VCErase );
 char       lBuff[ 32 ];
 int        i, j = 1;

 lpPaint[ 0 ] =                   lpErase[ 0 ] = 0;
 lpPaint[ 1 ] = VIF_DIRECT( 32 ); lpErase[ 1 ] = VIF_DIRECT( 7 );

 lpPaint[ 2 ] = lpErase[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 lpPaint[ 3 ] = lpErase[ 3 ] = GIF_AD;
 lpPaint[ 4 ] = lpErase[ 4 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 lpPaint[ 5 ] = lpErase[ 5 ] = GS_TEST_1;

 lpPaint[ 6 ] = lpErase[ 6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 7 ] = lpErase[ 7 ] = GS_RGBAQ | ( GS_PRIM << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 8 ] = lpErase[ 8 ] = g_Palette[ g_Config.m_PlayerVBCIdx - 1 ];
 lpPaint[ 9 ] = lpErase[ 9 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, g_GSCtx.m_PrimCtx, 0 );

 lpPaint[ 10 ] = lpErase[ 10 ] = GS_SETREG_XYZ(        lX,                lY, 0  );
 lpPaint[ 11 ] = lpErase[ 11 ] = GS_SETREG_XYZ(  lX + lDX, lY + ( 288 << 3 ), 0  );

 lpPaint[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); lpErase[ 12 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lpPaint[ 13 ] = lpErase[ 13 ] = GIF_AD;
 lpPaint[ 14 ] = lpErase[ 14 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 2 );
 lpPaint[ 15 ] = lpErase[ 15 ] = GS_TEST_1;

 lpPaint[ 16 ] = GIF_TAG( 24, 1, 0, 0, 1, 2  );
 lpPaint[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );

 lY  = ( 52              << 3 ) + ( g_GSCtx.m_OffsetY << 4 );
 lX  =                            ( g_GSCtx.m_OffsetX << 4 );
 lDX = ( g_GSCtx.m_Width << 4 ) + ( g_GSCtx.m_OffsetX << 4 );

 lBuff[ 0 ] = ' ';
 _FormatTime ( &lBuff[ 1 ], s_Player.m_pCont -> m_Duration );

 lpPaint = _U( s_TDTotal );

 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - 144, 52, 0, lBuff, 0, &lpPaint, 0 );

 if ( s_pLang )

  s_pLang -> Destroy ( s_pLang, 0 );

 else s_pLang = StringList_Init ();

 for ( i = 0; i < SMS_MAX_STREAMS; ++i ) {

  SMS_Stream* lpStm = s_Player.m_pCont -> m_pStm[ i ];

  if ( !lpStm ) break;

  if ( lpStm -> m_Flags & SMS_STRM_FLAGS_AUDIO ) {

   if ( lpStm -> m_pName )

    s_pLang -> PushBack ( s_pLang, lpStm -> m_pName );

   else {

    char lBuff[ 32 ]; sprintf ( lBuff, "%d", j );

    s_pLang -> PushBack ( s_pLang, lBuff );

   }  /* end else */

   s_pLang -> m_pTail -> m_pParam = ( void* )i;

  }  /* end if */

  ++j;

 }  /* end for */

 if ( s_pLang -> m_Size == 1 ) {

  i = ( int )s_pLang -> m_pTail -> m_pParam;

  s_pLang -> Pop ( s_pLang );
  s_pLang -> PushBack ( s_pLang, "default" );

  s_pLang -> m_pTail -> m_pParam = ( void* )i;

 }  /* end if */

 s_pCurLang = s_pLang -> m_pHead;

 lpPaint = _U( s_OSDAV );
 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - 224, 52, 0, "V/A:", 4, &lpPaint, 0 );

 lpPaint = _U( s_OSDSV );
 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - 224, 52, 0, "S/V:", 4, &lpPaint, 0 );

 lpPaint = _U( s_OSDDT );
 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - 148, 52, 0, " 00:00", 6, &lpPaint, 0 );

 s_Player.m_OSDPackets[ 0 ] = s_OSDAV;
 s_Player.m_OSDQWC    [ 0 ] = QWC_OSD_AV;
 s_Player.m_OSDPackets[ 1 ] = s_OSDDT;
 s_Player.m_OSDQWC    [ 1 ] = QWC_OSD_DT;

 lPoints[ 0 ].m_X = 0;
 lPoints[ 0 ].m_Y = 0;
 lPoints[ 1 ].m_X = 0;
 lPoints[ 1 ].m_Y = g_GSCtx.m_Height - 96;
 lPoints[ 2 ].m_X = 0;
 lPoints[ 2 ].m_Y = g_GSCtx.m_Height - 64;
 lPoints[ 3 ].m_X = 0;
 lPoints[ 3 ].m_Y = g_GSCtx.m_Height;
 lPoints[ 4 ].m_X = g_GSCtx.m_Width;
 lPoints[ 4 ].m_Y = 0;
 lPoints[ 5 ].m_X = lPoints[ 4 ].m_X;
 lPoints[ 5 ].m_Y = lPoints[ 1 ].m_Y;
 lPoints[ 6 ].m_X = lPoints[ 4 ].m_X;
 lPoints[ 6 ].m_Y = lPoints[ 2 ].m_Y;
 lPoints[ 7 ].m_X = lPoints[ 4 ].m_X;
 lPoints[ 7 ].m_Y = lPoints[ 3 ].m_Y;
 g_GSCtx.Scale ( lPoints, 8 );

 lpPaint = _U( s_DummyErase );

 lpPaint[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 1 ] = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 2 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 3 ] = GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 );
 lpPaint[ 4 ] = GS_SETREG_XYZ( lPoints[ 0 ].m_X, lPoints[ 0 ].m_Y, 0 );
 lpPaint[ 5 ] = GS_SETREG_XYZ( lPoints[ 7 ].m_X, lPoints[ 7 ].m_Y, 0 );

 lpPaint = _U( s_DummyPaint );
 lpPaint[  0 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 lpPaint[  1 ] = GIF_AD;
 lpPaint[  2 ] = GSAlphaBlend_Back2Front;
 lpPaint[  3 ] = GS_ALPHA_1;
 lpPaint[  4 ] = GIF_TAG( 2, 1, 0, 0, 1, 9 );
 lpPaint[  5 ] = ( u64 )GS_PRIM | (  ( u64 )GS_RGBAQ <<  4  ) | (  ( u64 )GS_XYZ2 <<  8  )
                                | (  ( u64 )GS_RGBAQ << 12  ) | (  ( u64 )GS_XYZ2 << 16  )
                                | (  ( u64 )GS_RGBAQ << 20  ) | (  ( u64 )GS_XYZ2 << 24  )
                                | (  ( u64 )GS_RGBAQ << 28  ) | (  ( u64 )GS_XYZ2 << 32  );
 lpPaint[  6 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 0, 0, 1, 0, 0, 0, 0 );
 lpPaint[  7 ] = GS_SETREG_RGBA( 0, 0, 0x80, 0 );
 lpPaint[  8 ] = GS_SETREG_XYZ( lPoints[ 0 ].m_X, lPoints[ 0 ].m_Y, 0 );
 lpPaint[  9 ] = GS_SETREG_RGBA( 0, 0, 0x00, 0x10 );
 lpPaint[ 10 ] = GS_SETREG_XYZ( lPoints[ 1 ].m_X, lPoints[ 1 ].m_Y, 0 );
 lpPaint[ 11 ] = GS_SETREG_RGBA( 0, 0, 0x80, 0 );
 lpPaint[ 12 ] = GS_SETREG_XYZ( lPoints[ 4 ].m_X, lPoints[ 4 ].m_Y, 0 );
 lpPaint[ 13 ] = GS_SETREG_RGBA( 0, 0, 0x00, 0x10 );
 lpPaint[ 14 ] = GS_SETREG_XYZ( lPoints[ 5 ].m_X, lPoints[ 5 ].m_Y, 0 );
 lpPaint[ 15 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_TRISTRIP, 1, 0, 0, 1, 0, 0, 0, 0 );
 lpPaint[ 16 ] = GS_SETREG_RGBA( 0, 0, 0x00, 0x10 );
 lpPaint[ 17 ] = GS_SETREG_XYZ( lPoints[ 2 ].m_X, lPoints[ 2 ].m_Y, 0 );
 lpPaint[ 18 ] = GS_SETREG_RGBA( 0, 0, 0x40, 0x00 );
 lpPaint[ 19 ] = GS_SETREG_XYZ( lPoints[ 3 ].m_X, lPoints[ 3 ].m_Y, 0 );
 lpPaint[ 20 ] = GS_SETREG_RGBA( 0, 0, 0x00, 0x10 );
 lpPaint[ 21 ] = GS_SETREG_XYZ( lPoints[ 6 ].m_X, lPoints[ 6 ].m_Y, 0 );
 lpPaint[ 22 ] = GS_SETREG_RGBA( 0, 0, 0x40, 0x00 );
 lpPaint[ 23 ] = GS_SETREG_XYZ( lPoints[ 7 ].m_X, lPoints[ 7 ].m_Y, 0 );

 s_Player.m_OSDPackets[ 2 ] = s_DummyErase;
 s_Player.m_OSDPackets[ 3 ] = s_DummyPaint;
 s_Player.m_OSDQWC    [ 2 ] =  3;
 s_Player.m_OSDQWC    [ 3 ] = 12;

 if ( s_Player.m_pCont -> m_pPlayList ) {

  s_Player.m_OSDPackets[ 4 ] = ( uint64_t* )malloc (
   PlayerControl_GSPLen ( s_Player.m_pCont -> m_pPlayList, 0 ) * sizeof ( uint64_t )
  );
  FlushCache ( 0 );
  s_Player.m_OSDQWC[ 4 ] = PlayerControl_GSPacket (
   s_Player.m_OSDPLPos, s_Player.m_pCont -> m_pPlayList, s_Player.m_OSDPackets[ 4 ]
  );

 } else s_Player.m_OSDPackets[ 4 ] = NULL;

 {  /* begin block (bix64's scrollbar) */

  uint64_t lDY;
  int      lXPos = ( g_GSCtx.m_Width  -  30 );
  int      lYPos = ( g_GSCtx.m_Height + 400 );

  if ( g_Config.m_ScrollBarNum ==  64 ||
       g_Config.m_ScrollBarNum == 128
  )

   lXPos = ( g_GSCtx.m_Width - 60 );

  else if ( g_Config.m_ScrollBarNum ==  80 ||
            g_Config.m_ScrollBarNum == 112
  ) lXPos = ( g_GSCtx.m_Width - 90 );

  if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Top )

   lYPos  = 10;

  else if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Bottom )

   lYPos  = g_GSCtx.m_Height - 30;

  lX      = ( lXPos << 4 ) + ( g_GSCtx.m_OffsetX << 4 );
  lY      = ( lYPos << 3 ) + ( g_GSCtx.m_OffsetY << 4 );
  lDY     = 20 << 3;
  lpPaint = _U( s_SCPaint );
  lpErase = _U( s_SCErase );

  lpPaint[ 0 ] =                                                                                             lpErase[ 0 ] = 0;
  lpPaint[ 1 ] = VIF_DIRECT(     (    (   (  ( g_Config.m_ScrollBarNum * 2 ) + 18  ) / 2   ) - 1    )     ); lpErase[ 1 ] = VIF_DIRECT( 7 );

  lpPaint[ 2 ] = lpErase[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
  lpPaint[ 3 ] = lpErase[ 3 ] = GIF_AD;
  lpPaint[ 4 ] = lpErase[ 4 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
  lpPaint[ 5 ] = lpErase[ 5 ] = GS_TEST_1;

  lpPaint[ 6 ] = lpErase[ 6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
  lpPaint[ 7 ] = lpErase[ 7 ] = GS_RGBAQ | ( GS_PRIM << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
  lpPaint[ 8 ] = lpErase[ 8 ] = g_Palette[ g_Config.m_PlayerSBCIdx - 1 ];
  lpPaint[ 9 ] = lpErase[ 9 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, g_GSCtx.m_PrimCtx, 0 );

  lpPaint[ 10 ] = lpErase[ 10 ] = GS_SETREG_XYZ(   lX, lY, 0   );
  lpPaint[ 11 ] = lpErase[ 11 ] = GS_SETREG_XYZ(   lX - (  ( g_GSCtx.m_Width - 30 ) << 4  ), lY + lDY, 0   );

  lpPaint[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); lpErase[ 12 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
  lpPaint[ 13 ] = lpErase[ 13 ] = GIF_AD;
  lpPaint[ 14 ] = lpErase[ 14 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 2 );
  lpPaint[ 15 ] = lpErase[ 15 ] = GS_TEST_1;

  lpPaint[ 16 ] = GIF_TAG(  g_Config.m_ScrollBarNum, 1, 0, 0, 1, 2  );
  lpPaint[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );

 }  /* end block (bix64's scrollbar) */

 s_Player.m_OSDPackets[ 6 ] = s_OSDNR;

}  /* end PlayerControl_Init */

void PlayerControl_MkTime ( int64_t aTime, int aDir ) {

 static char s_lBuff0[ 14 ] __attribute__(   (  aligned( 8 )  )   ) = {
  'P', 'l', 'a', 'y', ' '
 };
 static char s_lBuff1[ 14 ] __attribute__(   (  aligned( 8 )  )   ) = {
  ' ', 'R', 'e', 'm', ' '
 };
 static char* s_lpBuf [ 2 ] = { s_lBuff0, s_lBuff1 };
 static int   s_Offset[ 2 ] = {      320,      328 };

 uint64_t* lpPacket = _U( s_TDPTS );

 _FormatTime ( &s_lpBuf[ aDir ][ 5 ], aTime );

 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - s_Offset[ aDir ], 52, 0, s_lpBuf[ aDir ], 12, &lpPacket, 0 );

}  /* end PlayerControl_MkTime */

void PlayerControl_DisplayTime ( int anOp, int64_t aTime, int afDraw ) {

 char      lBuff[ 13 ];
 char*     lpSrc;
 uint64_t* lpPacket = _U( s_TDPTS );
 int       lOffset;

 switch ( anOp ) {

  case  0: lpSrc = "Play "; lOffset = 304; break;
  case  1: lpSrc = "FFwd "; lOffset = 320; break;
  case -1: lpSrc = " Rew "; lOffset = 320; break;
  case  2: lpSrc = "Curs "; lOffset = 320; break;
  default: return;

 }  /* end switch */

 strcpy ( lBuff, lpSrc );

 _FormatTime ( &lBuff[ 5 ], aTime );

 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - lOffset, 52, 0, lBuff, 12, &lpPacket, 0 );

 if ( !afDraw ) {

  s_Player.m_pIPUCtx -> PQueuePacket ( QWC_TD_TOTAL, s_TDTotal );
  s_Player.m_pIPUCtx -> PQueuePacket ( QWC_TD_PRSTS, s_TDPTS   );

 } else {

  uint64_t  lChain[ 4 ] __attribute__(   ( aligned( 16 )  )   );
  uint64_t* lpChain = _U( lChain );

  lpChain[ 0 ] = DMA_TAG(  QWC_TD_PRSTS, 1, DMA_REF,  0, ( u32 )s_TDPTS,   0  );
  lpChain[ 1 ] = 0L;
  lpChain[ 2 ] = DMA_TAG(  QWC_TD_TOTAL, 1, DMA_REFE, 0, ( u32 )s_TDTotal, 0  );
  lpChain[ 3 ] = 0L;
  DMA_Wait ( DMA_CHANNEL_VIF1 );
  DMA_SendChainToVIF1 ( lChain );
  DMA_Wait ( DMA_CHANNEL_VIF1 );

 }  /* end else */

}  /* end PlayerControl_DisplayTime */

void PlayerControl_AdjustVolume ( int aDelta ) {

 int i,    lVol    = g_Config.m_PlayerVolume = SMS_clip ( g_Config.m_PlayerVolume + aDelta, 0, 24 );
 uint64_t* lpPaint = _U( s_VCPaint );
 uint64_t  lV      = lpPaint[ 10 ];
 uint64_t  lX      = ( lV & 0xFFFF ) + ( 12 << 4 );
 uint64_t  lY      = ( lV >> 16 ) & 0xFFFF;
 uint64_t  lDX     = 6 << 4;

 lVol = 24 - lVol;

 for (  i = 18; i < 18 + ( lVol << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SETREG_XYZ( lX, lY, 1LL );
  lY += 6 << 3;
  lpPaint[ i + 1 ] = GS_SETREG_XYZ( lX + lDX, lY, 1LL );
  lY += 6 << 3;

 }  /* end for */

 lX  = lV & 0xFFFF;
 lDX = 30 << 4;

 for ( ; i < 18 + ( 24 << 1 ); i += 2 ) {

  lpPaint[ i + 0 ] = GS_SETREG_XYZ( lX, lY, 1LL );
  lY += 6 << 3;
  lpPaint[ i + 1 ] = GS_SETREG_XYZ( lX + lDX, lY, 1LL );
  lY += 6 << 3;

 }  /* end for */

 g_IPUCtx.QueuePacket ( 33, s_VCPaint );

 s_IntTime = g_Timer + 2000;
 Timer_RegisterHandler ( 1, TimerHandler );

 s_Player.m_pSPUCtx -> SetVolume (  PlayerControl_Index2Volume ()  );

}  /* end PlayerControl_AdjustVolume */

static int PlayerControl_Scroll ( int aDir ) {

 SMS_Container* lpCont    = s_Player.m_pCont;
 SMS_Stream*    lpStm     = lpCont -> m_pStm[ s_Player.m_VideoIdx ];
 FileContext*   lpFileCtx = lpCont -> m_pFileCtx;
 IPUContext*    lpIPUCtx  = s_Player.m_pIPUCtx;
 int64_t        lTime     = s_Player.m_VideoTime;
 SMS_AVPacket*  lpPacket  = lpCont -> NewPacket ( lpCont );
 int            retVal    = 0;
 int64_t        lIncr     = 3000LL * aDir;
 uint64_t       lNextTime = 0LL;
 uint32_t       lFilePos  = 0U;
 void*          lpHandler = Timer_RegisterHandler ( 2, NULL );

 lpIPUCtx -> Sync    ();
 lpIPUCtx -> Repaint ();
 PlayerControl_DisplayTime ( aDir, lTime, 1 );

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos,  0 );
 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 10 );

 while ( 1 ) {

  uint64_t         lDisplayTime = g_Timer;
  int64_t          lPos         = SMS_Rescale (  lTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
  int              lSize        = lpCont -> Seek ( lpCont, s_Player.m_VideoIdx, aDir, lPos );
  SMS_FrameBuffer* lpFrame;

  if ( !lSize ) {

   retVal = 0;
   break;

  } else if ( lTime == 0LL ) {

   retVal = 1;
   break;

  }  /* end if */

  lFilePos = lpFileCtx -> m_CurPos;
  lSize    = lpCont -> ReadPacket ( lpPacket );

  if ( lSize < 0 ) {

   retVal = 0;
   break;

  } else if ( lSize == 0 ) continue;

  lpFrame = NULL;

  s_Player.m_pVideoCodec -> Decode (
   &lpStm -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
  );

  if ( lpFrame ) {

   int64_t lDiff;

   do {

    uint32_t lButtons = GUI_ReadButtons ();

    if ( lButtons && g_Timer > lNextTime ) {

     lNextTime = g_Timer + 200;

     if ( lButtons == PAD_TRIANGLE ) {

      goto end;

     } else if ( lButtons == PAD_RIGHT ) {

      if ( aDir > 0 ) {

       if ( lIncr < 60000LL ) lIncr += 3000LL;

      } else if ( lIncr < -3000LL ) {

       lIncr += 3000LL;

      } else {

       lIncr = 3000LL;
       aDir  = 1;

      }  /* end else */

     } else if ( lButtons == PAD_LEFT ) {

      if ( aDir > 0 ) {

       if ( lIncr > 3000LL )

        lIncr -= 3000LL;

       else {

        lIncr = -3000LL;
        aDir  = -1;

       }  /* end else */

      } else if ( lIncr > -60000LL ) lIncr -= 3000LL;

     } else if ( lButtons == PAD_CROSS ) {

      lpIPUCtx -> Sync ();
      PlayerControl_DisplayTime ( 0, lpPacket -> m_PTS, 0 );
      lpIPUCtx -> Display ( lpFrame );

      retVal = 1;
      goto end;

     }  /* end if */

    }  /* end if */

    lDiff = 300LL - ( g_Timer - lDisplayTime );

   } while ( lDiff > 0 );

   lpIPUCtx -> Sync ();
   PlayerControl_DisplayTime ( aDir, lpPacket -> m_PTS, 0 );

   if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_PF_SUBS )  ) {

    s_Player.m_pSubCtx -> m_Idx = 0;
    s_Player.m_pSubCtx -> Display ( lpPacket -> m_PTS );

   }  /* end if */

   lpIPUCtx -> Display ( lpFrame );

  }  /* end if */

  lTime += lIncr;

  if ( lTime < 0 ) lTime = 0;

 }  /* end while */
end:
 if ( retVal ) {

  lpFileCtx -> Stream ( lpFileCtx, lFilePos, 0                         );
  lpFileCtx -> Stream ( lpFileCtx, lFilePos, lpFileCtx -> m_StreamSize );

 }  /* end if */

 s_Player.m_VideoTime = lpPacket -> m_PTS;
 s_Player.m_AudioTime = lpPacket -> m_PTS;

 lpPacket -> Destroy ( lpPacket );

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> m_Idx = 0;

 Timer_RegisterHandler ( 2, lpHandler );

 return retVal;

}  /* end PlayerControl_Scroll */

int PlayerControl_FastForward ( void ) {

 return PlayerControl_Scroll ( 1 );

}  /* end PlayerControl_FastForward */

int PlayerControl_Rewind ( void ) {

 return PlayerControl_Scroll ( -1 );

}  /* end PlayerControl_Rewind */

StringListNode* PlayerControl_ChangeLang ( void ) {

 if ( !s_pCurLang -> m_pNext )

  s_pCurLang = s_pLang -> m_pHead;

 else s_pCurLang = s_pCurLang -> m_pNext;

 return s_pCurLang;

}  /* end PlayerControl_ChangeLang */

StringListNode* PlayerControl_GetLang ( void ) {

 return s_pCurLang;

}  /* end PlayerControl_GetLang */

void PlayerControl_SwitchSubs ( void ) {

 if ( s_Player.m_Flags & SMS_PF_SUBS )

  s_Player.m_Flags &= ~SMS_PF_SUBS;

 else s_Player.m_Flags |= SMS_PF_SUBS;

}  /* end PlayerControl_SwitchSubs */

static void* s_pTimerOSDHandlers[ 3 ] = {
 NULL, _osd_timer_handler_incr, _osd_timer_handler_decr
};

static void _handle_timer_osd ( int aDir ) {

 unsigned int lOSD = s_Player.m_OSD;

 if ( lOSD > 2 ) lOSD = 0;

 if ( ++lOSD == 3 )

  lOSD = 0;

 else {

  s_Player.m_OSDPackets[ 0 ] = s_TDTotal;
  s_Player.m_OSDPackets[ 1 ] = s_TDPTS;
  s_Player.m_OSDQWC    [ 0 ] = QWC_TD_TOTAL;
  s_Player.m_OSDQWC    [ 1 ] = QWC_TD_PRSTS;

  (   (  void ( * ) ( void )  )s_pTimerOSDHandlers[ lOSD ]   ) ();

  Timer_RegisterHandler ( 2, s_pTimerOSDHandlers[ lOSD ] );

 }   /* end else */

 s_Player.m_OSD = lOSD;

}  /* end _handle_timer_osd */

static void _handle_adjust_osd ( int aDelta, int* apVal, int aLimit ) {

 int       lVal = *apVal;
 int       lS, lMS;
 char      lBuf[ 8 ];
 uint64_t* lpPaint = _U( s_OSDDT );
 char      lSign;

 if ( lVal == aLimit && aDelta > 0 )

  lVal = aLimit - 250;

 else if ( lVal == -aLimit && aDelta < 0 ) lVal = -( aLimit - 250 );

 lVal += aDelta;

 *apVal = lVal;

 if ( lVal < 0 ) {

  lVal  = -lVal;
  lSign = '-';

 } else lSign = ' ';

 lS  = lVal / SMS_TIME_BASE;
 lMS = lVal % 1000;

 sprintf ( lBuf, "%c%02u:%03u\n", lSign, lS, lMS );

 g_GSCtx.TextGSPacket (  g_GSCtx.m_Width - 148, 52, 0, lBuf, 6, &lpPaint, 0  );

 s_Player.m_OSDPackets[ 1 ] = s_OSDDT;
 s_Player.m_OSDQWC    [ 1 ] = QWC_OSD_DT;

}  /* end _handle_adjust_osd */

static void _handle_av_adjust_osd ( int aDir ) {

 s_Player.m_OSDPackets[ 0 ] = s_OSDAV;
 s_Player.m_OSDQWC    [ 0 ] = QWC_OSD_AV;

 _handle_adjust_osd ( aDir, &s_Player.m_AVDelta, 5000 );

}  /* end _handle_av_adjust_osd */

static void _handle_sv_adjust_osd ( int aDir ) {

 s_Player.m_OSDPackets[ 0 ] = s_OSDSV;
 s_Player.m_OSDQWC    [ 0 ] = QWC_OSD_SV;

 _handle_adjust_osd ( aDir, &s_Player.m_SVDelta, 30000 );

}  /* end _handle_sv_adjust_osd */

static void ( *s_OSDHandlers[ 3 ] ) ( int ) = {
 _handle_timer_osd,
 _handle_av_adjust_osd,
 _handle_sv_adjust_osd
};

void PlayerControl_HandleOSD ( int aType, int aData ) {

 s_OSDHandlers[ aType ] ( aData );

}  /* end PlayerControl_HandleOSD */

unsigned int PlayerControl_GSPLen ( struct StringList* apList, unsigned int aLen ) {

 StringListNode* lpNode = apList -> m_pHead;

 aLen += PC_GSP_SIZE(  strlen ( lpNode -> m_pString )  );

 lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  aLen += strlen ( lpNode -> m_pString ) << 2;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return aLen;

}  /* end PlayerControl_GSPLen */

unsigned int PlayerControl_GSPacket ( int anY, struct StringList* apList, uint64_t* apDMA ) {

 unsigned int    j, k    = 6;
 unsigned int    retVal  = 6;
 StringListNode* lpNode  = apList -> m_pHead;
 unsigned int    lCumLen = 0;
 unsigned int    lXIncr  = g_GSCtx.m_OffsetX << 4;
 unsigned int    lYIncr  = g_GSCtx.m_OffsetY << 4;

 apDMA = ( u64* )UNCACHED_SEG( apDMA );
 apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apDMA[ 1 ] = ( GS_TEX0_1 + !g_GSCtx.m_PrimCtx ) | ( GS_PRIM << 4 );
 apDMA[ 2 ] = GS_SETREG_TEX0( g_GSCtx.m_Font.m_Text, 16, GSPSM_4, 10, 8, 1, 1, g_GSCtx.m_Font.m_CLUT[ 1 ], 0, 0, 0, 1 );
 apDMA[ 3 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, !g_GSCtx.m_PrimCtx, 0 );
 apDMA[ 5 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 while ( 1 ) {

  unsigned int lX[ 32 ];
  unsigned int lX1, lX2;
  unsigned int lY1, lY2;
  unsigned int lU1, lU2;
  unsigned int lV1, lV2;
  unsigned int lTX, lTY;
  int          lCurX;

  if ( anY >= -32 ) {

   unsigned int lLen = strlen ( lpNode -> m_pString );

   lX1 = (  g_GSCtx.m_Width - g_GSCtx.TextWidth ( lpNode -> m_pString, lLen )  ) >> 1;

   if ( lX1 > g_GSCtx.m_Width ) lX1 = 0;

   lY1 = ( anY << 3 ) + lYIncr;
   lY2 = (  ( anY + 32 ) << 3  ) + lYIncr;

   for ( j = 0; j < 32; ++j ) lX[ j ] = lX1;

   for ( j = 0; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lX[ l ] - g_Kerns[ lChr ].m_Kern[ l ].m_Left;

     if ( lOffset > lCurX ) lCurX = lOffset;

    }  /* end for */

    lX1  = ( lCurX << 4 ) + lXIncr;
    lX2  = (  ( lCurX + 32 ) << 4  ) + lXIncr;

    for ( l = 0; l < 32; ++l ) lX[ l ] = lCurX + 32 - g_Kerns[ lChr ].m_Kern[ l ].m_Right;

    lTY = 0;

    while ( lChr > 30 ) {

     lChr -= 31;
     lTY  += 32;

    }  /* end while */

    lTX = lChr * 32;

    lU1 = ( lTX << 4 ) + lXIncr;
    lU2 = (  ( lTX + 32 ) << 4  ) + lXIncr;

    lV1 = ( lTY << 4 ) + lXIncr;
    lV2 = (  ( lTY + 32 ) << 4  ) + lXIncr;

    apDMA[ k + 0 ] = GS_SETREG_UV( lU1, lV1 );
    apDMA[ k + 1 ] = GS_SETREG_XYZ( lX1, lY1, 0 );
    apDMA[ k + 2 ] = GS_SETREG_UV( lU2, lV2 );
    apDMA[ k + 3 ] = GS_SETREG_XYZ( lX2, lY2, 0 );

   }  /* end for */

   retVal  += lLen << 2;
   lCumLen += lLen;

  }  /* end if */

  if (  !( lpNode = lpNode -> m_pNext ) || ( anY += 32 ) > ( int )g_GSCtx.m_Height  ) break;

 }  /* end while */

 apDMA[ 4 ] = GIF_TAG( lCumLen, 1, 0, 0, 1, 4 );

 return retVal >> 1;

}  /* end PlayerControl_GSPacket */

void PlayerControl_Destroy ( void ) {

 if ( s_Player.m_OSDPackets[ 3 ] ) free ( s_Player.m_OSDPackets[ 3 ] );

}  /* end PlayerControl_Destroy */

static void PlayerControl_DisplayScrollBar ( int aPos ) {

 int       i;
 uint64_t* lpPaint = _U( s_SCPaint );
 uint64_t  lV1       = lpPaint[ 10 ];
 uint64_t  lX1       = ( lV1 & 0xFFFF );
 uint64_t  lY1       = (  ( lV1 >> 16 ) & 0xFFFF  ) + ( 7 << 3 );
 uint64_t  lDY1      = 3 << 3;
 uint64_t  lIncr;
 uint32_t  lQWC;

 aPos = g_Config.m_ScrollBarNum - aPos;

 if ( g_Config.m_ScrollBarNum == 32 ) {

  lIncr = 9 << 4;
  lQWC  = 41;

 } else if ( g_Config.m_ScrollBarNum == 48 ) {

  lIncr = 6 << 4;
  lQWC  = 57;

 } else if ( g_Config.m_ScrollBarNum == 64 ) {

  lIncr = 4 << 4;
  lQWC  = 73;

 } else if ( g_Config.m_ScrollBarNum == 80 ) {

  lIncr = 3 << 4;
  lQWC  = 89;

 } else if ( g_Config.m_ScrollBarNum == 96 ) {

  lIncr = 3 << 4;
  lQWC  = 105;

 } else if ( g_Config.m_ScrollBarNum == 112 ) {

  lIncr = 2 << 4;
  lQWC  = 121;

 } else {  /* 128 */

  lIncr = 2 << 4;
  lQWC  = 137;

 }  /* end else */

 for (  i = 18; i < 18 + ( aPos << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SETREG_XYZ( lX1, lY1, 1LL );
  lX1 -= lIncr;
  lpPaint[ i + 1 ] = GS_SETREG_XYZ( lX1, lY1 + lDY1, 1LL );
  lX1 -= lIncr;

 }  /* end for */

 lY1  = (  ( lV1 >> 16 ) & 0xFFFF  );
 lDY1 = 18 << 3;

 for (  ; i < 18 + ( g_Config.m_ScrollBarNum << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SETREG_XYZ( lX1, lY1, 1LL );
  lX1 -= lIncr;
  lpPaint[ i + 1 ] = GS_SETREG_XYZ( lX1, lY1 + lDY1, 1LL );
  lX1 -= lIncr;

 }  /* end for */

 g_IPUCtx.QueuePacket ( lQWC, s_SCPaint );

 s_IntTime = g_Timer + 10000;

 Timer_RegisterHandler ( 1, TimerHandler );

}  /* end PlayerControl_DisplayScrollBar */

int PlayerControl_ScrollBar ( void ) {

 SMS_Container* lpCont      = s_Player.m_pCont;
 SMS_Stream*    lpStm       = lpCont -> m_pStm[ s_Player.m_VideoIdx ];
 FileContext*   lpFileCtx   = lpCont -> m_pFileCtx;
 IPUContext*    lpIPUCtx    = s_Player.m_pIPUCtx;
 SMS_AVPacket*  lpPacket    = lpCont -> NewPacket ( lpCont );
 int            retVal      = 0;
 uint64_t       lNextTime   = 0LL;
 uint64_t       lNextTime1  = 0LL;
 uint32_t       lFilePos    = 0U;
 int64_t        lTotalTime  = lpCont -> m_Duration;
 int64_t        lPassTime   = s_Player.m_VideoTime;
 int64_t        lScale      = (  lTotalTime / ( g_Config.m_ScrollBarNum + 1 )  );
 float          lCurPos1    = (  lPassTime / lScale  );
 float          j;
 
 lpIPUCtx -> Suspend ();

 for (  j = 0; j < ( g_Config.m_ScrollBarNum - 1 ); j += 1  )

  if (   (  lCurPos1 >= ( j - 0.5F )  ) &&
         (  lCurPos1 <= ( j + 0.5F )  )
  ) lCurPos1 = j;

 if (  lCurPos1 < 0.5F                                ) lCurPos1 = 0;
 if (  lCurPos1 > ( g_Config.m_ScrollBarNum - 0.5F )  ) lCurPos1 = g_Config.m_ScrollBarNum;

 int     lCurPos     = SMS_clip ( lCurPos1 , 0, g_Config.m_ScrollBarNum );
 int     lResume     = 1;
 int     lTestPause  = 1;
 int     lTestSelect = 0;
 int     lDir        = 0;
 int64_t lTime;
 int64_t lPos;
 int     lSize;
 
 PlayerControl_DisplayScrollBar ( lCurPos );

 if ( g_Config.m_PlayerFlags & SMS_PF_TIME ) PlayerControl_DisplayTime ( 2, lPassTime, 1 );

 while ( 1 ) {

  g_GSCtx.VSync ();

  if ( lResume == 1 ) {
   
   lTime = s_Player.m_VideoTime;

  } else {
		    
   if ( lCurPos == 0 ) {
    
    lTime = 100LL;

   } else { 

    lTime = ( lScale * lCurPos );

   }  /* end else */

  }  /* end else */

  if ( s_IntTime <= g_Timer ) lpIPUCtx -> QueuePacket ( 8, s_SCErase );

  lpIPUCtx -> Flush   ();
  lpIPUCtx -> Repaint ();

  if ( s_IntTime >= g_Timer && g_Config.m_PlayerFlags & SMS_PF_TIME ) PlayerControl_DisplayTime ( 2, lTime, 1 );

  uint32_t lButtons = GUI_ReadButtons ();

  if ( lButtons && g_Timer > lNextTime ) {

   lNextTime = g_Timer + 100;

   if ( lButtons == PAD_START || lButtons == PAD_CROSS ) {

    lpIPUCtx -> Repaint ();
    PlayerControl_DisplayTime ( 0, lTime, 1 );

    retVal = 1;
    goto end;

   } else if ( lButtons == PAD_RIGHT && lCurPos < g_Config.m_ScrollBarNum ) {

    lCurPos = SMS_clip ( ( lCurPos + 1 ) , 0, g_Config.m_ScrollBarNum );
    lResume = 0;

    PlayerControl_DisplayScrollBar ( lCurPos );

   } else if ( lButtons == PAD_LEFT && lCurPos > 0 ) {

    lCurPos = SMS_clip (  ( lCurPos - 1 ), 0, g_Config.m_ScrollBarNum  );
    lResume = 0;

    PlayerControl_DisplayScrollBar ( lCurPos );

   } else if ( lButtons == PAD_SELECT ) {

    if ( g_Timer > lNextTime1 && lTestSelect == 1 ) {
    
     if ( lTestPause == 1 ) {

      g_IPUCtx.QueuePacket ( 8, s_SCErase );
      lTestPause = 0;
      s_IntTime  = g_Timer;

     } else {

      PlayerControl_DisplayScrollBar ( lCurPos );

      lTestPause = 1;

     }  /* end else */

    }  /* end if */

    lNextTime1  = g_Timer + 250;
    lTestSelect = 1;
    
   } else if ( lButtons == PAD_TRIANGLE ) {

    goto stop;

   }  /* end if */

  }  /* end if */

 }  /* end while */
end:
 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 0 );

 lDir = ( lTime - lPassTime );

 if ( lDir >  0 ) lDir =  1;
 if ( lDir <= 0 ) lDir = -1;

 lPos  = SMS_Rescale (  lTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
 lSize = lpCont -> Seek ( lpCont, s_Player.m_VideoIdx, lDir, lPos );

 lFilePos = lpFileCtx -> m_CurPos;
 lSize    = lpCont -> ReadPacket ( lpPacket );
 lpFileCtx -> Stream ( lpFileCtx, lFilePos, lpFileCtx -> m_StreamSize );

 s_Player.m_VideoTime = lpPacket -> m_PTS;
 s_Player.m_AudioTime = lpPacket -> m_PTS;

 lpIPUCtx -> QueuePacket ( 8, s_SCErase );
 lpPacket -> Destroy ( lpPacket );
stop:
 lpIPUCtx -> Resume ();

 return retVal;

}  /* end PlayerControl_ScrollBar */

void PlayerControl_UpdateDuration ( unsigned int aDuration ) {

 char      lBuff[ 9 ];
 uint64_t* lpPaint;

 if ( aDuration ) {

  int  lM = aDuration / 60;
  int  lH = lM        / 60;

  aDuration %= 60;
  lM        %= 60;

  sprintf ( lBuff, " %1d:%02d:%02d", lH, lM, aDuration );

 } else {

  memset ( lBuff, ' ', 8 );
  lBuff[ 8 ] = '\x00';

 }  /* end else */

 lpPaint = _U( s_TDTotal );

 g_GSCtx.TextGSPacket ( g_GSCtx.m_Width - 144, 52, 0, lBuff, 8, &lpPaint, 0 );

}  /* end PlayerControl_UpdateDuration */

void PlayerControl_UpdateItemNr ( void ) {

 char      lBuff[ 32 ];
 uint64_t* lpDMA;

 sprintf ( lBuff, "% 3d /% 3d", s_Player.m_PlayItemNr, s_Player.m_pCont -> m_pPlayList -> m_Size );

 lpDMA = _U( s_OSDNR );

 s_Player.m_OSDQWC[ 6 ] = g_GSCtx.TextGSPacket ( 64, 52, 0, lBuff, 0, &lpDMA, 0 );

}  /* end PlayerControl_UpdateItemNr */
