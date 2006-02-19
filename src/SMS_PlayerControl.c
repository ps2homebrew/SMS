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
#include "SMS_GS.h"
#include "SMS_VIF.h"
#include "SMS_IPU.h"
#include "SMS_SPU.h"
#include "SMS_DMA.h"
#include "SMS_PAD.h"
#include "SMS_GUI.h"
#include "SMS_Timer.h"
#include "SMS_Config.h"
#include "SMS_SubtitleContext.h"
#include "SMS_List.h"
#include "SMS_Locale.h"

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#define OSD_Y_POS 32

static char s_TimeFmt[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "0:00:00";

static uint64_t s_OSDNR[ GS_TXT_PACKET_SIZE( 8 ) ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );

static uint64_t s_DummyErase[                                                    6 ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_DummyPaint[ ( GS_VGR_PACKET_SIZE() << 1 ) + GS_RRT_PACKET_SIZE() ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );

static uint64_t  s_VCPaint [  66 ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t  s_VCErase [  16 ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t  s_SCPaint [ 274 ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t  s_SCErase [  16 ] SMS_BSS_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t  s_IntTime;
static int       s_VCDY;
static uint64_t* s_pPTS;
static uint64_t* s_pSts  [ 5 ];
static int       s_StsQWC[ 5 ];
static int       s_PTSX[ 2 ];
static uint64_t* s_pAV;
static int       s_AVQWC;
static uint64_t* s_pSV;
static int       s_SVQWC;
static uint64_t* s_pDelta;
static int       s_DeltaX;
static unsigned  s_SBY1;
static unsigned  s_SBY2;

static SMS_List*     s_pLang;
static SMS_ListNode* s_pCurLang;

extern SMS_Player s_Player;

static void _osd_timer_handler_incr ( void ) {

 PlayerControl_MkTime ( s_Player.m_AudioTime );

}  /* end _osd_timer_handler_incr */

static void _osd_timer_handler_decr ( void ) {

 PlayerControl_MkTime ( s_Player.m_pCont -> m_Duration - s_Player.m_AudioTime );

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

static void _create_status ( SMString* apStr, int anIdx ) {

 int lX = GSFont_Width ( apStr -> m_pStr, apStr -> m_Len );

 s_pSts[ anIdx ] = GSContext_NewList (  s_StsQWC[ anIdx ] = GS_TXT_PACKET_SIZE( apStr -> m_Len )  );
 GSFont_Render ( apStr -> m_pStr, apStr -> m_Len, s_PTSX[ 0 ] - lX - 16, OSD_Y_POS, s_pSts[ anIdx ] );
 s_StsQWC[ anIdx ] = ( s_StsQWC[ anIdx ] + 2 ) >> 1;

}  /* end _create_status */

void PlayerControl_Init ( void ) {

 uint64_t* lpPaint = _U( s_VCPaint );
 uint64_t* lpErase = _U( s_VCErase );
/* Initialize volume control display list */
 int lX = 20 << 4;
 int lY = ( g_GSCtx.m_Height - 288 ) >> 1;
 int lW = 30 << 4;
 int lH = 288;

 __asm__ __volatile__(
  ".set noreorder\n\t"
  "li       $v0, 6\n\t"
  "pxor     $a0, $a0, $a0\n\t"
  "pxor     $a1, $a1, $a1\n\t"
  "pxor     $a2, $a2, $a2\n\t"
  "pcpyld   $v0, $v0, $zero\n\t"
  "move     $a1, %2\n\t"
  "dsll32   %3, %3, 0\n\t"
  "or       $a1, $a1, %3\n\t"
  "jal      GS_XYZ\n\t"
  "por      $a1, $a1, $v0\n\t"
  "pcpyud   $v1, $a1, $zero\n\t"
  "srl      %0, $v0, 16\n\t"
  "srl      $v1, $v1, 16\n\t"
  "dsrl32   %1, $a1, 16\n\t"
  "sw       $v1, %4\n\t"
  ".set reorder\n\t"
  : "=r"( lY ), "=r"( lH )
  :  "r"( lY ),  "r"( lH ), "m"( s_VCDY )
  : "a0", "a1", "a2", "v0", "v1"
 );

 lpPaint[  0 ] =                   lpErase[ 0 ] = 0;
 lpPaint[  1 ] = VIF_DIRECT( 32 ); lpErase[ 1 ] = VIF_DIRECT( 7 );

 lpPaint[  2 ] = lpErase[  2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 lpPaint[  3 ] = lpErase[  3 ] = GIFTAG_REGS_AD;
 lpPaint[  4 ] = lpErase[  4 ] = GS_SET_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 lpPaint[  5 ] = lpErase[  5 ] = GS_TEST_1;

 lpPaint[  6 ] = lpErase[  6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[  7 ] = lpErase[  7 ] = GS_RGBAQ | ( GS_PRIM << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[  8 ] = lpErase[  8 ] = g_Palette[ g_Config.m_PlayerVBCIdx - 1 ];
 lpPaint[  9 ] = lpErase[  9 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );

 lpPaint[ 10 ] = lpErase[ 10 ] = GS_SET_XYZ(       lX,      lY, 0  );
 lpPaint[ 11 ] = lpErase[ 11 ] = GS_SET_XYZ(  lX + lW, lY + lH, 0  );

 lpPaint[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); lpErase[ 12 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lpPaint[ 13 ] = lpErase[ 13 ] = GIFTAG_REGS_AD;
 lpPaint[ 14 ] = lpErase[ 14 ] = GS_SET_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 2 );
 lpPaint[ 15 ] = lpErase[ 15 ] = GS_TEST_1;

 lpPaint[ 16 ] = GIF_TAG( 24, 1, 0, 0, 1, 2  );
 lpPaint[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
/* Initialize display lists for OSD timer (status, current PTS, and total time) */
 g_GSCtx.m_TextColor = 0;

 lX     = GSFont_Width ( s_TimeFmt, 7 ) + 16;
 s_pPTS = GSContext_NewList (  GS_TXT_PACKET_SIZE( 7 ) << 1  );
 SyncDCache (   s_pPTS - 2, s_pPTS + (  GS_TXT_PACKET_SIZE( 7 ) << 1  )   );

 s_PTSX[ 1 ] = g_GSCtx.m_Width - lX;
 PlayerControl_UpdateDuration ( 1, s_Player.m_pCont -> m_Duration / SMS_TIME_BASE );

 s_PTSX[ 0 ] = s_PTSX[ 1 ] - lX;
 PlayerControl_UpdateDuration ( 0, 0 );

 _create_status ( &STR_PLAY, 0 );
 _create_status ( &STR_REM,  1 );
 _create_status ( &STR_FFWD, 2 );
 _create_status ( &STR_REW,  3 );
 _create_status ( &STR_CURS, 4 );
/* Initialize display lists for delta time adjustments */
 lX      = GSFont_Width ( s_TimeFmt, 7 ) + 16;
 s_pAV   = GSContext_NewList (  s_AVQWC = GS_TXT_PACKET_SIZE( STR_VA.m_Len )  );
 SyncDCache ( s_pAV - 2, s_pAV + s_AVQWC );
 s_AVQWC = ( s_AVQWC + 2 ) >> 1;

 s_DeltaX = g_GSCtx.m_Width - lX - 16;

 lX = GSFont_Width ( STR_VA.m_pStr, STR_VA.m_Len ) + 16;
 GSFont_Render (
  STR_VA.m_pStr, STR_VA.m_Len, s_DeltaX - lX, OSD_Y_POS, _U( s_pAV )
 );

 s_pSV   = GSContext_NewList (  s_SVQWC = GS_TXT_PACKET_SIZE( STR_SV.m_Len )  );
 SyncDCache ( s_pSV - 2, s_pSV + s_SVQWC );
 s_SVQWC = ( s_SVQWC + 2 ) >> 1;

 lX = GSFont_Width ( STR_SV.m_pStr, STR_SV.m_Len ) + 16;
 GSFont_Render (
  STR_SV.m_pStr, STR_SV.m_Len, s_DeltaX - lX, OSD_Y_POS, _U( s_pSV )
 );

 s_pDelta = GSContext_NewList (  GS_TXT_PACKET_SIZE( 7 )  );
 SyncDCache (  s_pDelta - 2, s_pDelta + (  GS_TXT_PACKET_SIZE( 7 ) >> 1  )  );
/* Initialize song numbers for MP3 player */
 s_Player.m_OSDPackets[ 6 ] = s_OSDNR;
/* Initialize bix64's scrollbar */
 lX = g_GSCtx.m_Width - 30;

 if ( g_Config.m_ScrollBarNum ==  64 ||
      g_Config.m_ScrollBarNum == 128
 )

  lX = g_GSCtx.m_Width - 60;

 else if ( g_Config.m_ScrollBarNum ==  80 ||
           g_Config.m_ScrollBarNum == 112
 ) lX = g_GSCtx.m_Width - 90;

 if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Top )

  lY = 10;

 else if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Bottom )

  lY = g_GSCtx.m_Height - 30;

 __asm__ __volatile__(
  ".set noreorder\n\t"
  "li       $v0, 20\n\t"
  "move     $a0, $zero\n\t"
  "dsll32   $v0, $v0, 0\n\t"
  "move     $a1, %2\n\t"
  "or       $a1, $a1, $v0\n\t"
  "jal      GS_XYZ\n\t"
  "move     $a2, $zero\n\t"
  "dsrl32   $a1, $a1, 0\n\t"
  "sw       $v0, %0\n\t"
  "addu     $v0, $v0, $a1\n\t"
  "sw       $v0, %1\n\t"
  ".set reorder\n\t"
  : "=m"( s_SBY1 ), "=m"( s_SBY2 ) : "r"( lY ) : "a0", "a1", "a2", "v0", "v1"
 );

 lX <<= 4;

 lpPaint = _U( s_SCPaint );
 lpErase = _U( s_SCErase );

 lpPaint[ 0 ] =                                                                                             lpErase[ 0 ] = 0;
 lpPaint[ 1 ] = VIF_DIRECT(     (    (   (  ( g_Config.m_ScrollBarNum * 2 ) + 18  ) / 2   ) - 1    )     ); lpErase[ 1 ] = VIF_DIRECT( 7 );

 lpPaint[ 2 ] = lpErase[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 lpPaint[ 3 ] = lpErase[ 3 ] = GIFTAG_REGS_AD;
 lpPaint[ 4 ] = lpErase[ 4 ] = GS_SET_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 lpPaint[ 5 ] = lpErase[ 5 ] = GS_TEST_1;

 lpPaint[ 6 ] = lpErase[ 6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 7 ] = lpErase[ 7 ] = GS_RGBAQ | ( GS_PRIM << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 8 ] = lpErase[ 8 ] = g_Palette[ g_Config.m_PlayerSBCIdx - 1 ];
 lpPaint[ 9 ] = lpErase[ 9 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );

 lpPaint[ 10 ] = lpErase[ 10 ] = lX | s_SBY1;
 lpPaint[ 11 ] = lpErase[ 11 ] =      s_SBY2;

 lpPaint[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); lpErase[ 12 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lpPaint[ 13 ] = lpErase[ 13 ] = GIFTAG_REGS_AD;
 lpPaint[ 14 ] = lpErase[ 14 ] = GS_SET_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 2 );
 lpPaint[ 15 ] = lpErase[ 15 ] = GS_TEST_1;

 lpPaint[ 16 ] = GIF_TAG(  g_Config.m_ScrollBarNum, 1, 0, 0, 1, 2  );
 lpPaint[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );
/* Initialize language strings */
 if ( s_pLang )

  SMS_ListDestroy ( s_pLang, 0 );

 else s_pLang = SMS_ListInit ();

 lY = 1;

 for ( lX = 0; lX < SMS_MAX_STREAMS; ++lX ) {

  SMS_Stream* lpStm = s_Player.m_pCont -> m_pStm[ lX ];

  if ( !lpStm ) break;

  if ( lpStm -> m_Flags & SMS_STRM_FLAGS_AUDIO ) {

   if ( lpStm -> m_pName )

    SMS_ListPushBack ( s_pLang, lpStm -> m_pName );

   else {

    char lBuff[ 32 ]; sprintf ( lBuff, g_pPercDStr, lY );

    SMS_ListPushBack ( s_pLang, lBuff );

   }  /* end else */

   s_pLang -> m_pTail -> m_Param = lX;

  }  /* end if */

  ++lY;

 }  /* end for */

 if ( s_pLang -> m_Size == 1 ) {

  lX = ( int )s_pLang -> m_pTail -> m_Param;

  SMS_ListPop ( s_pLang );
  SMS_ListPushBack ( s_pLang, STR_DEFAULT1.m_pStr );

  s_pLang -> m_pTail -> m_Param = lX;

 }  /* end if */

 s_pCurLang = s_pLang -> m_pHead;
/* Initialize MP3/M3U playlist (if any) */
 if ( s_Player.m_pCont -> m_pPlayList ) {

  s_Player.m_OSDPackets[ 4 ] = ( uint64_t* )malloc (
   PlayerControl_GSPLen ( s_Player.m_pCont -> m_pPlayList, 0 ) * sizeof ( uint64_t )
  );
  FlushCache ( 0 );
  s_Player.m_OSDQWC[ 4 ] = PlayerControl_GSPacket (
   s_Player.m_OSDPLPos, s_Player.m_pCont -> m_pPlayList, s_Player.m_OSDPackets[ 4 ]
  );

 } else s_Player.m_OSDPackets[ 4 ] = NULL;
/* Initialize MP3/M3U display */
 lpPaint = _U( s_DummyErase );

 lpPaint[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 1 ] = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 3 ] = 0L;
 lpPaint[ 4 ] = 0L;
 lpPaint[ 5 ] = ( g_GSCtx.m_PWidth << 4 ) | ( g_GSCtx.m_PHeight << 20 );

 lpPaint = _U( s_DummyPaint );

 GS_RenderRoundRect (
  ( GSRoundRectPacket* )(  lpPaint + (  GS_VGR_PACKET_SIZE() << 1  ) - 2  ),
  0, g_GSCtx.m_Height - 97, g_GSCtx.m_Width - 1, 35, -8, g_Palette[ g_Config.m_BrowserIBCIdx - 1 ]
 );
 GSContext_RenderVGRect (
  lpPaint, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height - 96,
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x80, 0x00 ),
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x10, 0x00 )
 );
 GSContext_RenderVGRect (
  lpPaint + GS_VGR_PACKET_SIZE(),
  0, g_GSCtx.m_Height - 64, g_GSCtx.m_Width, 64,
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x10, 0x00 ),
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x80, 0x00 )
 );

 s_Player.m_OSDPackets[ 2 ] = s_DummyErase;
 s_Player.m_OSDPackets[ 3 ] = s_DummyPaint;
 s_Player.m_OSDQWC    [ 2 ] = 3;
 s_Player.m_OSDQWC    [ 3 ] = sizeof ( s_DummyPaint ) >> 4;

}  /* end PlayerControl_Init */

void PlayerControl_Destroy ( void ) {

 int i;

 GSContext_DeleteList ( s_pPTS   );
 GSContext_DeleteList ( s_pAV    );
 GSContext_DeleteList ( s_pSV    );
 GSContext_DeleteList ( s_pDelta );

 for ( i = 0; i < 5; ++i ) GSContext_DeleteList ( s_pSts[ i ] );

 if ( s_Player.m_OSDPackets[ 4 ] ) free ( s_Player.m_OSDPackets[ 4 ] );

}  /* end PlayerControl_Destroy */

void PlayerControl_MkTime ( int64_t aTime ) {

 char      lBuf[ 8 ];
 uint64_t* lpPacket = _U( s_pPTS );

 _FormatTime ( lBuf, aTime );

 GSFont_Render ( lBuf, 7, s_PTSX[ 0 ], OSD_Y_POS, lpPacket );

}  /* end PlayerControl_MkTime */

void PlayerControl_DisplayTime ( int anOp, int64_t aTime, int afDraw ) {

 int lIdx;

 switch ( anOp ) {

  case  0: lIdx = 0; break;
  case  1: lIdx = 2; break;
  case -1: lIdx = 3; break;
  case  2: lIdx = 4; break;
  default: return;

 }  /* end switch */

 PlayerControl_MkTime ( aTime );

 if ( !afDraw ) {

  s_Player.m_pIPUCtx -> PQueuePacket ( s_StsQWC[ lIdx ], s_pSts[ lIdx ]    - 2   );
  s_Player.m_pIPUCtx -> PQueuePacket ( GS_TXT_PACKET_SIZE( 7 ) + 1, s_pPTS - 2   );

 } else {

  GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
   GSContext_CallList ( 1, s_pSts[ lIdx ] );
   GSContext_CallList ( 1, s_pPTS         );
  GSContext_Flush ( 1, GSFlushMethod_DeleteLists );

 }  /* end else */

}  /* end PlayerControl_DisplayTime */

void PlayerControl_AdjustVolume ( int aDelta ) {

 int i,    lVol    = g_Config.m_PlayerVolume = SMS_clip ( g_Config.m_PlayerVolume + aDelta, 0, 24 );
 uint64_t* lpPaint = _U( s_VCPaint );
 uint64_t  lV      = lpPaint[ 10 ];
 uint64_t  lX      = ( lV & 0xFFFF ) + ( 12 << 4 );
 uint64_t  lY      = ( lV >> 16 ) & 0xFFFF;
 uint64_t  lDX     = 6 << 4;
 uint64_t  lDY     = s_VCDY;

 lVol = 24 - lVol;

 for (  i = 18; i < 18 + ( lVol << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SET_XYZ( lX,       lY, 1LL );
  lY += lDY;
  lpPaint[ i + 1 ] = GS_SET_XYZ( lX + lDX, lY, 1LL );
  lY += lDY;

 }  /* end for */

 lX  = lV & 0xFFFF;
 lDX = 30 << 4;

 for (  ; i < 18 + ( 24 << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SET_XYZ( lX,       lY, 1LL );
  lY += lDY;
  lpPaint[ i + 1 ] = GS_SET_XYZ( lX + lDX, lY, 1LL );
  lY += lDY;

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
   lpStm -> m_pCodec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
  );

  if ( lpFrame ) {

   int64_t lDiff;

   do {

    uint32_t lButtons = GUI_ReadButtons ();

    if ( lButtons && g_Timer > lNextTime ) {

     lNextTime = g_Timer + 200;

     if ( lButtons == SMS_PAD_TRIANGLE ) {

      goto end;

     } else if ( lButtons == SMS_PAD_RIGHT ) {

      if ( aDir > 0 ) {

       if ( lIncr < 60000LL ) lIncr += 3000LL;

      } else if ( lIncr < -3000LL ) {

       lIncr += 3000LL;

      } else {

       lIncr = 3000LL;
       aDir  = 1;

      }  /* end else */

     } else if ( lButtons == SMS_PAD_LEFT ) {

      if ( aDir > 0 ) {

       if ( lIncr > 3000LL )

        lIncr -= 3000LL;

       else {

        lIncr = -3000LL;
        aDir  = -1;

       }  /* end else */

      } else if ( lIncr > -60000LL ) lIncr -= 3000LL;

     } else if ( lButtons == SMS_PAD_CROSS ) {

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

SMS_ListNode* PlayerControl_ChangeLang ( void ) {

 if ( !s_pCurLang -> m_pNext )

  s_pCurLang = s_pLang -> m_pHead;

 else s_pCurLang = s_pCurLang -> m_pNext;

 return s_pCurLang;

}  /* end PlayerControl_ChangeLang */

SMS_ListNode* PlayerControl_GetLang ( void ) {

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

  int lIdx = lOSD - 1;

  s_Player.m_OSDPackets[ 0 ] = s_pSts  [ lIdx ] - 2;
  s_Player.m_OSDQWC    [ 0 ] = s_StsQWC[ lIdx ];
  s_Player.m_OSDPackets[ 1 ] = s_pPTS - 2;
  s_Player.m_OSDQWC    [ 1 ] = GS_TXT_PACKET_SIZE( 7 ) + 1;

  (   (  void ( * ) ( void )  )s_pTimerOSDHandlers[ lOSD ]   ) ();

  Timer_RegisterHandler ( 2, s_pTimerOSDHandlers[ lOSD ] );

 }   /* end else */

 s_Player.m_OSD = lOSD;

}  /* end _handle_timer_osd */

static void _handle_adjust_osd ( int aDelta, int* apVal, int aLimit ) {

 int       lVal = *apVal;
 int       lS, lMS;
 char      lBuf[ 8 ];
 uint64_t* lpPaint = _U( s_pDelta );
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

 GSFont_Render ( lBuf, 7, s_DeltaX, OSD_Y_POS, lpPaint );

 s_Player.m_OSDPackets[ 1 ] = s_pDelta - 2;
 s_Player.m_OSDQWC    [ 1 ] = (  GS_TXT_PACKET_SIZE( 7 ) >> 1  ) + 1;

}  /* end _handle_adjust_osd */

static void _handle_av_adjust_osd ( int aDir ) {

 s_Player.m_OSDPackets[ 0 ] = s_pAV - 2;
 s_Player.m_OSDQWC    [ 0 ] = s_AVQWC;

 _handle_adjust_osd ( aDir, &s_Player.m_AVDelta, 5000 );

}  /* end _handle_av_adjust_osd */

static void _handle_sv_adjust_osd ( int aDir ) {

 s_Player.m_OSDPackets[ 0 ] = s_pSV - 2;
 s_Player.m_OSDQWC    [ 0 ] = s_SVQWC;

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

unsigned int PlayerControl_GSPLen ( SMS_List* apList, unsigned int aLen ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 aLen += PC_GSP_SIZE(  strlen ( lpNode -> m_pString )  );

 lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  aLen += strlen ( lpNode -> m_pString ) << 2;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 return aLen;

}  /* end PlayerControl_GSPLen */

unsigned int PlayerControl_GSPacket ( int anY, SMS_List* apList, uint64_t* apDMA ) {

 unsigned int  j, k    = 6;
 unsigned int  retVal  = 6;
 SMS_ListNode* lpNode  = apList -> m_pHead;
 unsigned int  lCumLen = 0;
 int           lDispW  = g_GSCtx.m_Width - 4;

 apDMA = ( u64* )UNCACHED_SEG( apDMA );
 apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apDMA[ 1 ] = GS_TEX0_1 | ( GS_PRIM << 4 );
 apDMA[ 2 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, GSPixelFormat_PSMT4, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, g_GSCtx.m_CLUT[ 1 ], GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 0, GS_TEX_CLD_LOAD );
 apDMA[ 3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
 apDMA[ 5 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 while ( 1 ) {

  unsigned int lXV[ 32 ];
  unsigned int lX;
  unsigned int lY1, lY2;
  unsigned int lU, lV;
  int          lCurX;

  if ( anY >= -32 ) {

   unsigned int lLen   = strlen ( lpNode -> m_pString );
   int          lH     = 32;
   int          lDTY   = 0;
   int          lY     = anY;
   int          lDelta = 0;
   int          lDW;
   float        lAR;

   while (  GSFont_WidthEx ( lpNode -> m_pString, lLen, lDelta ) > lDispW && lDelta > -16 ) --lDelta;
   while (  GSFont_WidthEx ( lpNode -> m_pString, lLen, lDelta ) > lDispW                 ) --lLen;

   lAR  = ( 32.0F + lDelta ) / 32.0F;
   lDW  = lDelta << 4;
   lX   = (  g_GSCtx.m_Width - GSFont_WidthEx ( lpNode -> m_pString, lLen, lDelta )  ) >> 1;

   if ( lX > g_GSCtx.m_Width ) lX = 0;

   if ( lY < 0 ) {

    lDTY   = -lY;
    lDTY <<= 4;
    lH    +=  lY;
    lY    = 0;

   }  /* end if */

   __asm__ __volatile__ (
    ".set noreorder\n\t"
    "move     $t9, $ra\n\t"
    "addu     $v0, %2, %3\n\t"
    "move     $a1, %2\n\t"
    "move     $a0, $zero\n\t"
    "dsll32   $v0, $v0, 0\n\t"
    "move     $a2, $zero\n\t"
    "jal      GS_XYZ\n\t"
    "or       $a1, $a1, $v0\n\t"
    "srl      $v0, $v0, 16\n\t"
    "dsrl32   $a1, $a1, 0\n\t"
    "sll      $v0, $v0, 16\n\t"
    "move     %0, $v0\n\t"
    "move     %1, $a1\n\t"
    "move     $ra, $t9\n\t"
    ".set reorder\n\t"
    : "=r"( lY1 ), "=r"( lY2 ) : "r"( lY ), "r"( lH ) : "a0", "a1", "a2", "v0", "v1", "t9"
   );

   for ( j = 0; j < 32; ++j ) lXV[ j ] = lX;

   for ( j = 0; j < lLen; ++j, k += 4 ) {

    unsigned int  l;
    unsigned char lChr = lpNode -> m_pString[ j ] - ' ';

    lCurX = -INT_MAX;

    for ( l = 0; l < 32; ++l ) {

     int lOffset = lXV[ l ] - g_GSCharIndent[ lChr ].m_Left[ l ] * lAR;

     __asm__ __volatile__(
      "pmaxw %0, %1, %2\n\t"
      : "=r"( lCurX ) : "r"( lCurX ), "r"( lOffset )
     );

    }  /* end for */

    lX = lCurX << 4;

    for ( l = 0; l < 32; ++l ) lXV[ l ] = lCurX + ( 31 - g_GSCharIndent[ lChr ].m_Right[ l ] ) * lAR;

    lU = ( lChr & 0x0000000F ) << 9;
    lV = ( lChr & 0xFFFFFFF0 ) << 5;

    apDMA[ k + 0 ] = GS_SET_UV( lU + 8, lV + lDTY + 8 );
    apDMA[ k + 1 ] = lX | lY1;
    apDMA[ k + 2 ] = GS_SET_UV( lU + 504, lV + 504 );
    apDMA[ k + 3 ] = ( lX + 512 + lDW ) | lY2;

   }  /* end for */

   retVal  += lLen << 2;
   lCumLen += lLen;

  }  /* end if */

  if (  !( lpNode = lpNode -> m_pNext ) || ( anY += 32 ) > ( int )g_GSCtx.m_Height  ) break;

 }  /* end while */

 apDMA[ 4 ] = GIF_TAG( lCumLen, 1, 0, 0, 1, 4 );

 return retVal >> 1;

}  /* end PlayerControl_GSPacket */

static void PlayerControl_DisplayScrollBar ( int aPos ) {

 int       i;
 uint64_t* lpPaint = _U( s_SCPaint );
 uint64_t  lX      = lpPaint[ 10 ] & 0xFFFF;
 uint64_t  lY1;
 uint64_t  lY2;
 uint64_t  lIncr;
 uint32_t  lQWC;

 lY2   = s_SBY2 - s_SBY1;
 lIncr = lY2 / 5;

 lY1 = s_SBY1 + (  ( lY2 - lIncr ) >> 1  );
 lY2 = lY1 + lIncr;

 lY1 &= 0xFFFF0000;
 lY2 &= 0xFFFF0000;

 aPos = g_Config.m_ScrollBarNum - aPos;

 if ( g_Config.m_ScrollBarNum == 32 ) {

  lIncr = 148;
  lQWC  = 41;

 } else if ( g_Config.m_ScrollBarNum == 48 ) {

  lIncr = 98;
  lQWC  = 57;

 } else if ( g_Config.m_ScrollBarNum == 64 ) {

  lIncr = 65;
  lQWC  = 73;

 } else if ( g_Config.m_ScrollBarNum == 80 ) {

  lIncr = 47;
  lQWC  = 89;

 } else if ( g_Config.m_ScrollBarNum == 96 ) {

  lIncr = 48;
  lQWC  = 105;

 } else if ( g_Config.m_ScrollBarNum == 112 ) {

  lIncr = 33;
  lQWC  = 121;

 } else {  /* 128 */

  lIncr = 32;
  lQWC  = 137;

 }  /* end else */

 for (  i = 18; i < 18 + ( aPos << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = lX | lY1 | ( 1L << 32 );
  lX -= lIncr;
  lpPaint[ i + 1 ] = lX | lY2 | ( 1L << 32 );
  lX -= lIncr;

 }  /* end for */

 for (  ; i < 18 + ( g_Config.m_ScrollBarNum << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = lX | s_SBY1 | ( 1L << 32 );
  lX -= lIncr;
  lpPaint[ i + 1 ] = lX | s_SBY2 | ( 1L << 32 );
  lX -= lIncr;

 }  /* end for */

 g_IPUCtx.QueuePacket ( lQWC, s_SCPaint );

 s_IntTime = g_Timer + 10000;

 Timer_RegisterHandler ( 1, TimerHandler );

}  /* end PlayerControl_DisplayScrollBar */

int PlayerControl_ScrollBar (  void ( *apInitQueues ) ( int ), int aSemaA, int aSemaV  ) {

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

 int     lCurPos     = SMS_clip ( lCurPos1, 0, g_Config.m_ScrollBarNum );
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

  GS_VSync ();

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

   if ( lButtons == SMS_PAD_START || lButtons == SMS_PAD_CROSS ) {

    lpIPUCtx -> Repaint ();
    PlayerControl_DisplayTime ( 0, lTime, 1 );

    retVal = 1;
    break;

   } else if ( lButtons == SMS_PAD_RIGHT && lCurPos < g_Config.m_ScrollBarNum ) {

    lCurPos = SMS_clip ( ( lCurPos + 1 ) , 0, g_Config.m_ScrollBarNum );
    lResume = 0;

    PlayerControl_DisplayScrollBar ( lCurPos );

   } else if ( lButtons == SMS_PAD_LEFT && lCurPos > 0 ) {

    lCurPos = SMS_clip (  ( lCurPos - 1 ), 0, g_Config.m_ScrollBarNum  );
    lResume = 0;

    PlayerControl_DisplayScrollBar ( lCurPos );

   } else if ( lButtons == SMS_PAD_SELECT ) {

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
    
   } else if ( lButtons == SMS_PAD_TRIANGLE ) {

    lResume = 0;
    break;

   }  /* end if */

  }  /* end if */

 }  /* end while */

 SignalSema ( aSemaA );
 SignalSema ( aSemaV );

 if ( !lResume ) {

  apInitQueues ( 0 );

  if ( retVal ) {

   if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> m_Idx = 0;

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

  }  /* end if */

 }  /* end if */

 lpPacket -> Destroy ( lpPacket );
 lpIPUCtx -> QueuePacket ( 8, s_SCErase );
 lpIPUCtx -> Resume ();

 while (  GUI_ReadButtons ()  );

 return retVal;

}  /* end PlayerControl_ScrollBar */

void PlayerControl_UpdateDuration ( unsigned int anIdx, unsigned int aDuration ) {

 char      lBuff[ 8 ];
 uint64_t* lpPaint;

 if ( !anIdx || aDuration ) {

  int  lM = aDuration / 60;
  int  lH = lM        / 60;

  aDuration %= 60;
  lM        %= 60;

  sprintf ( lBuff, "%1d:%02d:%02d", lH, lM, aDuration );

 } else {

  memset ( lBuff, ' ', 7 );
  lBuff[ 7 ] = '\x00';

 }  /* end else */

 lpPaint = _U( s_pPTS + anIdx * GS_TXT_PACKET_SIZE( 7 ) );

 GSFont_Render ( lBuff, 7, s_PTSX[ anIdx ], OSD_Y_POS, lpPaint );

}  /* end PlayerControl_UpdateDuration */

void PlayerControl_UpdateItemNr ( void ) {

 char      lBuff[ 32 ];
 int       lLen;
 uint64_t* lpDMA;

 sprintf ( lBuff, "% 3d /% 3d", s_Player.m_PlayItemNr, s_Player.m_pCont -> m_pPlayList -> m_Size );

 lpDMA = _U( s_OSDNR );
 lLen  = strlen ( lBuff );

 s_Player.m_OSDQWC[ 6 ] = GS_TXT_PACKET_SIZE( lLen ) >> 1;

 GSFont_Render ( lBuff, lLen, 16, OSD_Y_POS, lpDMA );

}  /* end PlayerControl_UpdateItemNr */
