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
#include "SMS_Sounds.h"
#include "SMS_RC.h"
#include "SMS_MPEG.h"
#include "SMS_MPEG12.h"
#include "SMS_FileDir.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "SMS_PgInd.h"

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define OSD_Y_POS 32

static char s_TimeFmt[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "0:00:00";

static int       s_VCDY;
static uint64_t  s_VCXY;
static uint64_t  s_SBXY;
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
static uint64_t* s_pLangOSD;

static SMS_List*     s_pLang;
static SMS_ListNode* s_pCurLang;
static SMS_List*     s_pSubLang;
static SMS_ListNode* s_pCurSubLang;
static GUIObject*    s_pInfo;

extern SMS_Player s_Player;

extern GUIObject* SMS_GUInfo       ( SMS_Container* );
extern void       SMS_GUInfoDelete ( GUIObject**    );
extern void       IPU_EraseSub     ( void           );

static float s_Speaker[ 48 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   ) = {
 0.2380952F, 0.1904760F, 0.0000000F, 0.0476192F,
 0.0000000F, 0.0476192F, 0.2190476F, 0.2380952F,
 0.4571440F, 0.4285720F, 0.4761920F, 0.4285720F,
 0.4761920F, 0.4285720F, 0.4571440F, 0.2380952F,
 0.2000000F, 0.2380952F, 0.1904760F, 0.2380952F,
 0.1904760F, 0.0000000F, 0.0000000F, 0.0000000F,
 0.2380952F, 0.2857144F, 0.2380952F, 0.2857144F,
 0.7619040F, 0.7142840F, 0.7619040F, 0.7142840F,
 1.0000000F, 0.9047600F, 1.0000000F, 0.4761920F,
 0.0000000F, 0.0952380F, 0.0000000F, 0.2857144F,
 0.2571428F, 0.2857144F, 0.2857144F, 0.7619040F,
 0.7619040F, 0.0000000F, 0.0000000F, 0.0000000F
};

static float s_Sun[ 80 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   ) = {
 0.700000F, 0.650000F, 0.687938F, 0.640954F,
 0.653208F, 0.614906F, 0.599999F, 0.574999F,
 0.534729F, 0.526047F, 0.465270F, 0.473952F,
 0.399999F, 0.425000F, 0.346791F, 0.385093F,
 0.312061F, 0.359046F, 0.300000F, 0.350000F,
 0.312062F, 0.359046F, 0.346791F, 0.385094F,
 0.400000F, 0.425000F, 0.465271F, 0.473953F,
 0.534730F, 0.526047F, 0.600000F, 0.575000F,
 0.653209F, 0.614907F, 0.687939F, 0.640954F,
 0.700000F, 0.650000F, 0.700000F, 0.650000F,
 0.500000F, 0.500000F, 0.431595F, 0.448696F,
 0.371442F, 0.403581F, 0.326794F, 0.370096F,
 0.303038F, 0.352279F, 0.303039F, 0.352279F,
 0.326795F, 0.370096F, 0.371443F, 0.403582F,
 0.431597F, 0.448697F, 0.500000F, 0.500000F,
 0.568404F, 0.551303F, 0.628558F, 0.596418F,
 0.673205F, 0.629904F, 0.696962F, 0.647721F,
 0.696962F, 0.647721F, 0.673205F, 0.629904F,
 0.628557F, 0.596418F, 0.568404F, 0.551303F,
 0.500000F, 0.500000F, 0.500000F, 0.500000F
};

static float s_Rays[ 8 ][ 8 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   ) = {
 { 1.000000F, 1.000000F, 0.750000F, 0.750000F, 0.475000F, 0.525000F, 0.475000F, 0.525000F },
 { 0.871231F, 0.835876F, 0.694454F, 0.659099F, 0.835875F, 0.871231F, 0.659099F, 0.694454F },
 { 0.525001F, 0.475001F, 0.525000F, 0.475000F, 1.000000F, 1.000000F, 0.750000F, 0.750000F },
 { 0.164125F, 0.128770F, 0.340901F, 0.305546F, 0.871232F, 0.835876F, 0.694455F, 0.659099F },
 { 0.000000F, 0.000000F, 0.250000F, 0.250000F, 0.525000F, 0.475000F, 0.525000F, 0.475001F },
 { 0.128768F, 0.164123F, 0.305545F, 0.340900F, 0.164126F, 0.128770F, 0.340902F, 0.305546F },
 { 0.475000F, 0.525000F, 0.475000F, 0.525000F, 0.000000F, 0.000000F, 0.250000F, 0.250000F },
 { 0.835874F, 0.871230F, 0.659098F, 0.694454F, 0.128767F, 0.164123F, 0.305545F, 0.340900F }
};

static void _osd_timer_handler_incr ( void* apArg ) {

 PlayerControl_MkTime ( s_Player.m_AudioTime );

 SMS_iTimerSet ( 2, 1000, _osd_timer_handler_incr, NULL );

}  /* end _osd_timer_handler_incr */

static void _osd_timer_handler_decr ( void* apArg ) {

 PlayerControl_MkTime ( s_Player.m_pCont -> m_Duration - s_Player.m_AudioTime );

 SMS_iTimerSet ( 2, 1000, _osd_timer_handler_decr, NULL );

}  /* end _osd_timer_handler_decr */

static void* s_pTimerOSDHandlers[ 3 ] = {
 NULL, _osd_timer_handler_incr
};

static void TimerHandler ( void* apArg ) {

 s_Player.m_Flags &= ~SMS_FLAGS_DISPCTL;

}  /* end TimerHandler */

void PlayerControl_FormatTime ( char* apBuf, uint64_t aTime ) {

 int lS = ( int )( aTime / SMS_TIME_BASE );
 int lM = lS / 60;
 int lH = lM / 60;

 lS %= 60;
 lM %= 60;

 sprintf ( apBuf, "%1d:%02d:%02d", lH, lM, lS );

}  /* end PlayerControl_FormatTime */

static void _create_status ( SMString* apStr, int anIdx ) {

 int lX = GSFont_Width ( apStr -> m_pStr, apStr -> m_Len );

 s_pSts[ anIdx ] = GSContext_NewList (  s_StsQWC[ anIdx ] = GS_TXT_PACKET_SIZE( apStr -> m_Len )  );
 GSFont_Render ( apStr -> m_pStr, apStr -> m_Len, s_PTSX[ 0 ] - lX - 16, OSD_Y_POS, s_pSts[ anIdx ] );
 s_StsQWC[ anIdx ] = ( s_StsQWC[ anIdx ] + 2 ) >> 1;

}  /* end _create_status */

static void _sb_init ( void ) {

 int       lX = g_GSCtx.m_Width - 30;
 int       lY = g_Config.m_ScrollBarPos == SMScrollBarPos_Top ? 10 : g_GSCtx.m_Height - 30;
 uint64_t* lpPaint;

 __asm__ __volatile__(
  ".set noreorder\n\t"
  "addu     " ASM_REG_T9 ", $zero, $ra\n\t"
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
  "addu     $ra, $zero, " ASM_REG_T9 "\n\t"
  ".set reorder\n\t"
  : "=m"( s_SBY1 ), "=m"( s_SBY2 ) : "r"( lY ) : "a0", "a1", "a2", "v0", "v1", ASM_REG_T9
 );

 lX <<= 4;

 lpPaint = _U( g_SCPaint );

 lpPaint[ 0 ] = 0;
 lpPaint[ 1 ] = VIF_DIRECT(     (    (   (  ( g_Config.m_ScrollBarNum * 2 ) + 8  ) / 2   ) - 1    )     );
 lpPaint[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 lpPaint[ 3 ] = GS_RGBAQ | ( GS_PRIM << 4 );
 lpPaint[ 4 ] = g_Palette[ g_Config.m_PlayerSBCIdx - 1 ];
 lpPaint[ 5 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 6 ] = GIF_TAG(  g_Config.m_ScrollBarNum, 1, 0, 0, 1, 2  );
 lpPaint[ 7 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );

 s_SBXY = lX | s_SBY1;

}  /* end _sb_init */

static void _set_default ( SMS_List* apList ) {

 if ( apList -> m_Size == 1 ) {

  int lX = ( int )apList -> m_pTail -> m_Param;

  SMS_ListPop ( apList );
  SMS_ListPushBack ( apList, STR_DEFAULT1.m_pStr );

  apList -> m_pTail -> m_Param = lX;

 }  /* end if */

}  /* end _set_default */

void PlayerControl_Init ( void ) {

 uint64_t* lpErase;
 uint64_t* lpPaint = _U( g_VCPaint );
 int64_t   lDuration;
/* Initialize volume control display list */
 int   lX = 20 << 4;
 int   lY = ( g_GSCtx.m_Height - 288 ) >> 1;
 int   lH = 288;
 float lScaleOffset[ 3 ] = { 52.0F, 60.0F };

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

 lpPaint[ 0 ] = 0;
 lpPaint[ 1 ] = VIF_DIRECT( 27 );
 lpPaint[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 lpPaint[ 3 ] = GS_RGBAQ | ( GS_PRIM << 4 );
 lpPaint[ 4 ] = g_Palette[ g_Config.m_PlayerVBCIdx - 1 ];
 lpPaint[ 5 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 6 ] = GIF_TAG( 24, 1, 0, 0, 1, 2 );
 lpPaint[ 7 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );

 s_VCXY = GS_SET_XYZ( lX, lY, 0 );
 lY     = (  ( g_GSCtx.m_Height - 288 ) >> 1  ) + 226;

 lScaleOffset[ 2 ] = ( float )lY;

 lpPaint = _U( g_Speaker );
 lpErase = _U( g_Sun     );

 lpPaint[ 0 ] = lpErase[ 0 ] = 0L;
 lpPaint[ 1 ] = VIF_DIRECT( 14 ); lpErase[ 1 ] = VIF_DIRECT( 48 );
 lpPaint[ 2 ] = lpErase[ 2 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 lpPaint[ 3 ] = lpErase[ 3 ] = GS_RGBAQ | ( GS_PRIM << 4 );
 lpPaint[ 4 ] = lpErase[ 4 ] = g_Palette[ g_Config.m_PlayerVBCIdx - 1 ];
 lpPaint[ 5 ] = lpErase[ 5 ] = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 6 ] = GIF_TAG( 21, 1, 0, 0, 1, 1 ); lpErase[ 6 ] = GIF_TAG( 40, 0, 0, 0, 1, 1 );
 lpPaint[ 7 ] = lpErase[ 7 ] = GS_XYZ2;
 GS_XYZv ( &lpPaint[ 8 ], s_Speaker, 24, lScaleOffset, 0 );
 GS_XYZv ( &lpErase[ 8 ], s_Sun,     40, lScaleOffset, 0 );

 lpErase[ 48 ] = GIF_TAG( 8, 1, 0, 0, 1, 6 );
 lpErase[ 49 ] = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 ) | ( GS_XYZ2 << 16 ) | ( GS_XYZ2 << 20 );
 lpErase += 50;

 for ( lX = 0; lX < 8; ++lX, lpErase += 6 ) {

  lpErase[ 0 ] = GS_SET_PRIM( GS_PRIM_PRIM_TRISTRIP, 0, 0, 0, 0, 0, 0, 0, 0 );
  lpErase[ 1 ] = g_Palette[ g_Config.m_PlayerVBCIdx - 1 ];
  GS_XYZv ( &lpErase[ 2 ], s_Rays[ lX ], 4, lScaleOffset, 0 );

 }  /* end for */
/* Initialize display lists for OSD timer (status, current PTS, and total time) */
 g_GSCtx.m_TextColor = 0;

 lX     = GSFont_Width ( s_TimeFmt, 7 ) + 16;
 s_pPTS = GSContext_NewList (  GS_TXT_PACKET_SIZE( 7 ) << 1  );
 SyncDCache (   s_pPTS - 2, s_pPTS + (  GS_TXT_PACKET_SIZE( 7 ) << 1  )   );

 if ( s_Player.m_pCont -> m_Duration == 0x7FFFFFFFFFFFFFFFLL ) {
  lDuration                = 0;
  s_pTimerOSDHandlers[ 2 ] = NULL;
 } else {
  lDuration                = s_Player.m_pCont -> m_Duration / SMS_TIME_BASE;
  s_pTimerOSDHandlers[ 2 ] = _osd_timer_handler_decr;
 }  /* end else */

 s_PTSX[ 1 ] = g_GSCtx.m_Width - lX;
 PlayerControl_UpdateDuration ( 1, lDuration );

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
 s_Player.m_OSDPackets[ 6 ] = g_OSDNR;
/* Initialize bix64's scrollbar */
 _sb_init ();
/* Initialize language strings */
 if ( s_pLang )
  SMS_ListDestroy ( s_pLang, 0 );
 else s_pLang = SMS_ListInit ();

 if ( s_pSubLang )
  SMS_ListDestroy ( s_pSubLang, 0 );
 else s_pSubLang = SMS_ListInit ();

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

  } else if ( lpStm -> m_Flags & SMS_STRM_FLAGS_SUBTL ) {

   if ( lpStm -> m_pName )
    SMS_ListPushBack ( s_pSubLang, lpStm -> m_pName );
   else {
    char lBuff[ 32 ]; sprintf ( lBuff, g_pPercDStr, lY );
    SMS_ListPushBack ( s_pSubLang, lBuff );
   }  /* end else */

   s_pSubLang -> m_pTail -> m_Param = lX;

  }  /* end if */

  ++lY;

 }  /* end for */

 _set_default ( s_pLang    );
 _set_default ( s_pSubLang );

 s_pCurLang    = s_pLang    -> m_pHead;
 s_pCurSubLang = s_pSubLang -> m_pHead;
/* Initialize MP3/M3U playlist (if any) */
 if ( s_Player.m_pCont -> m_pPlayList ) {

  s_Player.m_OSDPackets[ 4 ] = ( uint64_t* )SMS_SyncMalloc (
   PlayerControl_GSPLen ( s_Player.m_pCont -> m_pPlayList, 0 ) * sizeof ( uint64_t )
  );
  s_Player.m_OSDQWC[ 4 ] = PlayerControl_GSPacket (
   s_Player.m_OSDPLPos, s_Player.m_pCont -> m_pPlayList, s_Player.m_OSDPackets[ 4 ]
  );

 } else s_Player.m_OSDPackets[ 4 ] = NULL;
/* Initialize MP3/M3U display */
 lpPaint = _U( g_DErase );

 lpPaint[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 1 ] = GS_PRIM | ( GS_RGBAQ << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0 );
 lpPaint[ 3 ] = 0L;
 lpPaint[ 4 ] = 0L;
 lpPaint[ 5 ] = ( g_GSCtx.m_PWidth << 4 ) | ( g_GSCtx.m_PHeight << 20 );

 lpPaint = _U( g_DPaint );

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
  0, g_GSCtx.m_Height - 64, g_GSCtx.m_Width, 66,
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x10, 0x00 ),
  GS_SET_RGBAQ( 0x00, 0x00, 0x80, 0x80, 0x00 )
 );

 s_Player.m_OSDPackets[ 2 ] = g_DErase;
 s_Player.m_OSDPackets[ 3 ] = g_DPaint;
 s_Player.m_OSDQWC    [ 2 ] =  3;
 s_Player.m_OSDQWC    [ 3 ] = 27;

}  /* end PlayerControl_Init */

void PlayerControl_Destroy ( void ) {

 int i;

 GSContext_DeleteList ( s_pPTS   );
 GSContext_DeleteList ( s_pAV    );
 GSContext_DeleteList ( s_pSV    );
 GSContext_DeleteList ( s_pDelta );

 for ( i = 0; i < 5; ++i ) GSContext_DeleteList ( s_pSts[ i ] );

 if ( s_Player.m_OSDPackets[ 4 ] ) free ( s_Player.m_OSDPackets[ 4 ] );

 SMS_GUInfoDelete ( &s_pInfo );

 GSContext_DeleteList ( s_pLangOSD );
 s_pLangOSD = NULL;

}  /* end PlayerControl_Destroy */

void PlayerControl_MkTime ( int64_t aTime ) {

 char      lBuf[ 16 ];
 uint64_t* lpPacket = _U( s_pPTS );

 PlayerControl_FormatTime ( lBuf, aTime );

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

  s_Player.m_pIPUCtx -> QueuePacket ( s_StsQWC[ lIdx ], s_pSts[ lIdx ]    - 2   );
  s_Player.m_pIPUCtx -> QueuePacket ( GS_TXT_PACKET_SIZE( 7 ) + 1, s_pPTS - 2   );

 } else {

  GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
   GSContext_CallList ( 1, s_pSts[ lIdx ] );
   GSContext_CallList ( 1, s_pPTS         );
  GSContext_Flush ( 1, GSFlushMethod_DeleteLists );

 }  /* end else */

}  /* end PlayerControl_DisplayTime */

static void _paint_slider ( int aPos ) {

 int       i;
 uint64_t* lpPaint = _U( g_VCPaint );
 int       lShift  = g_XShift;
 uint64_t  lV      = s_VCXY;
 uint64_t  lX      = ( lV & 0xFFFF ) + ( 12 << 4 );
 uint64_t  lY      = ( lV >> 16 ) & 0xFFFF;
 uint64_t  lDX     = 6 << 4;
 uint64_t  lDY     = s_VCDY;

 aPos  = 24 - aPos;
 lX  >>= lShift;
 lDX >>= lShift;

 for (  i = 8; i < 8 + ( aPos << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SET_XYZ( lX,       lY, 0 );
  lY += lDY;
  lpPaint[ i + 1 ] = GS_SET_XYZ( lX + lDX, lY, 0 );
  lY += lDY;

 }  /* end for */

 lX    = lV & 0xFFFF;
 lDX   = 30 << 4;
 lX  >>= lShift;
 lDX >>= lShift;

 for (  ; i < 8 + ( 24 << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = GS_SET_XYZ( lX,       lY, 0 );
  lY += lDY;
  lpPaint[ i + 1 ] = GS_SET_XYZ( lX + lDX, lY, 0 );
  lY += lDY;

 }  /* end for */

 s_Player.m_OSDPackets[ 9 ] = g_VCPaint;
 s_Player.m_OSDQWC    [ 9 ] = 28;

 s_Player.m_Flags |= SMS_FLAGS_DISPCTL;
 SMS_TimerSet ( 1, 2000, TimerHandler, NULL );

}  /* end _paint_slider */

void PlayerControl_AdjustVolume ( int aDelta ) {

 g_Config.m_PlayerVolume = SMS_clip ( g_Config.m_PlayerVolume + aDelta, 0, 24 );

 s_Player.m_OSDPackets[ 8 ] = g_Speaker;
 s_Player.m_OSDQWC    [ 8 ] = 15;

 _paint_slider ( g_Config.m_PlayerVolume );

 s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

}  /* end PlayerControl_AdjustVolume */

void PlayerControl_AdjustBrightness ( int aDelta ) {

 g_Config.m_PlayerBrightness = SMS_clip ( g_Config.m_PlayerBrightness + aDelta, 0, 24 );

 s_Player.m_OSDPackets[ 8 ] = g_Sun;
 s_Player.m_OSDQWC    [ 8 ] = 49;

 _paint_slider ( g_Config.m_PlayerBrightness );

 g_IPUCtx.SetBrightness (  ( unsigned int )( g_Config.m_PlayerBrightness * 10.625F + 0.5F )  );

}  /* end PlayerControl_AdjustBrightness */

static unsigned char s_DispBuff[ 64 ] __attribute__(   (  aligned( 64 ), section ( ".bss" )  )   );

static void SeekCB ( SMS_RingBuffer* apRB ) {

 SMS_Container*  lpCont = ( SMS_Container* )apRB -> m_pWaitCBParam;
 SMS_RingBuffer* lpVB   = lpCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pPktBuf;
 SMS_Stream**    lppStm = lpCont -> m_pStm;

 while ( 1 ) {

  int lPktIdx, lSize = lpCont -> ReadPacket ( lpCont, &lPktIdx );

  if ( lSize <= 0 ) {
   lpVB -> m_pOut = NULL;
   SMS_RingBufferPost ( lpVB );
   return;
  } else if ( lPktIdx == s_Player.m_VideoIdx ) {
   SMS_RingBufferPost ( lpVB );
   return;
  } else {
   SMS_AVPacket* lpPkt;
   SMS_RingBufferPost ( lppStm[ lPktIdx ] -> m_pPktBuf );
   lpPkt = ( SMS_AVPacket* )SMS_RingBufferWait ( lppStm[ lPktIdx ] -> m_pPktBuf );
   SMS_RingBufferFree ( lppStm[ lPktIdx ] -> m_pPktBuf, lpPkt -> m_Size + 64 );
  }  /* end else */

 }  /* end while */

}  /* end SeekCB */

static int PlayerControl_Scroll ( int aDir ) {

 SMS_Container*    lpCont     = s_Player.m_pCont;
 SMS_Stream**      lpStms     = lpCont -> m_pStm;
 SMS_Stream*       lpStm      = lpStms[ s_Player.m_VideoIdx ];
 FileContext*      lpFileCtx  = lpCont -> m_pFileCtx;
 IPUContext*       lpIPUCtx   = s_Player.m_pIPUCtx;
 int64_t           lTime      = s_Player.m_VideoTime;
 int               retVal     = 0;
 int64_t           lIncr      = 3000LL * aDir;
 uint64_t          lNextTime  = 0LL;
 uint32_t          lFilePos   = 0U;
 void*             lpHandler  = SMS_TimerReset ( 2, NULL );
 SMS_FrameBuffer*  lpFrame    = NULL;
 SMS_RingBuffer*   lpBuff     = lpStms[ s_Player.m_VideoIdx ] -> m_pPktBuf;
 SMS_RingBuffer*   lpDispBuff = SMS_RingBufferInit (  s_DispBuff, sizeof ( s_DispBuff )  );
 SMS_CodecContext* lpVideoCtx = lpStm -> m_pCodec;

 lpVideoCtx -> HWCtl ( lpVideoCtx, SMS_HWC_Reset );

 s_Player.m_pIPUCtx -> StopSync ( 1 );
 lpIPUCtx -> Suspend ();
 GS_VSync ();
 IPU_EraseSub        ();
 lpIPUCtx -> Repaint ();
 lpIPUCtx -> Resume  ();
 lpIPUCtx -> Sync    ();
 PlayerControl_DisplayTime ( aDir, lTime, 1 );

 if ( lpStm -> m_pCodec -> m_ID == SMS_CodecID_MPEG1 ||
      lpStm -> m_pCodec -> m_ID == SMS_CodecID_MPEG2
 ) lIncr <<= 3;

 lTime += lIncr;

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos,  0 );
 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 10 );

 lpBuff -> WaitCB         = SeekCB;
 lpBuff -> m_pWaitCBParam = lpCont;

 SMS_Codec_MPEG12_Reset (
  SMS_MPEG12_RESET_QUEUE | SMS_MPEG12_RESET_DECODER
 );

 while ( 1 ) {

  uint64_t lDisplayTime = g_Timer;
  int64_t  lPos         = SMS_Rescale (  lTime - lpCont -> m_StartTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
  int      lSize        = lpCont -> Seek ( lpCont, s_Player.m_VideoIdx, aDir, lPos );

  if ( !lSize ) {

   retVal = 0;
   break;

  } else if ( lTime == 0LL ) {

   s_Player.m_VideoTime = 0;
   s_Player.m_AudioTime = 0;
   lpFrame              = NULL;

   retVal = 1;
   break;

  }  /* end if */

  lFilePos = lpFileCtx -> m_CurPos;

  if (  s_Player.m_pVideoCodec -> Decode (
         lpStm -> m_pCodec, lpDispBuff, lpBuff
        )
  ) {

   int64_t lDiff;

   lpFrame = *( SMS_FrameBuffer** )SMS_RingBufferWait ( lpDispBuff );
   lpFrame -> m_FrameType = -1;

   SMS_RingBufferFree ( lpDispBuff, 4 );

   s_Player.m_VideoTime = lpFrame -> m_StartPTS;
   s_Player.m_AudioTime = lpFrame -> m_StartPTS;

   do {

    uint32_t lButtons = GUI_ReadButtons ();

    if ( lButtons && g_Timer > lNextTime ) {

     lNextTime = g_Timer + 300;

     if ( lButtons == SMS_PAD_TRIANGLE || lButtons == RC_RESET || lButtons == RC_RETURN || lButtons == RC_STOP ) {

      goto end;

     } else if ( lButtons == SMS_PAD_RIGHT || lButtons == RC_SCAN_RIGHT ) {

      if ( aDir > 0 ) {

       if ( lIncr < 60000LL ) lIncr += 3000LL;

      } else if ( lIncr < -3000LL ) {

       lIncr += 3000LL;

      } else {

       lIncr = 3000LL;
       aDir  = 1;

      }  /* end else */

     } else if ( lButtons == SMS_PAD_LEFT || lButtons == RC_SCAN_LEFT ) {

      if ( aDir > 0 ) {

       if ( lIncr > 3000LL )

        lIncr -= 3000LL;

       else {

        lIncr = -3000LL;
        aDir  = -1;

       }  /* end else */

      } else if ( lIncr > -60000LL ) lIncr -= 3000LL;

     } else if ( lButtons == SMS_PAD_CROSS || lButtons == RC_ENTER || lButtons == RC_PLAY ) {

      PlayerControl_DisplayTime ( 0, lpFrame -> m_StartPTS, 0 );
      lpIPUCtx -> Display ( lpFrame, -1 );
      lpIPUCtx -> Sync ();

      retVal = 1;
      goto end;

     }  /* end if */

    }  /* end if */

    lDiff = 300LL - ( g_Timer - lDisplayTime );

   } while ( lDiff > 0 );

   PlayerControl_DisplayTime ( aDir, lpFrame -> m_StartPTS, 0 );

   if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_FLAGS_SUBS )  ) {

    s_Player.m_pSubCtx -> Reset ();
    s_Player.m_pSubCtx -> Display ( lpFrame -> m_StartPTS );

   }  /* end if */

   lpIPUCtx -> Display ( lpFrame, -1 );
   lpIPUCtx -> Sync ();

  } else if (  FILE_EOF( lpCont -> m_pFileCtx )  ) {

   retVal = 0;
   break;

  }  /* end if */

  lTime += lIncr;

  if ( lTime < 0 ) lTime = 0;

  SMS_Codec_MPEG12_Reset ( SMS_MPEG12_RESET_DECODER );

 }  /* end while */
end:
 SMS_Codec_MPEG12_Reset (
  SMS_MPEG12_RESET_QUEUE   |
  SMS_MPEG12_RESET_DECODER |
  SMS_MPEG12_RESET_RECOVER
 );
 SMS_RingBufferReset ( lpBuff );

 lpBuff -> WaitCB = NULL;

 s_Player.m_VideoTime = 0;
 s_Player.m_AudioTime = 0;

 if ( retVal ) {

  lpFileCtx -> Stream ( lpFileCtx, lFilePos, 0                         );
  lpFileCtx -> Stream ( lpFileCtx, lFilePos, lpFileCtx -> m_StreamSize );

 }  /* end if */

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Reset ();

 SMS_TimerSet ( 2, 1000, lpHandler, NULL );

 s_Player.m_pIPUCtx -> StopSync ( 0 );

 SMS_RingBufferDestroy ( lpDispBuff );

 return retVal;

}  /* end PlayerControl_Scroll */

int PlayerControl_FastForward ( void ) {

 return PlayerControl_Scroll ( 1 );

}  /* end PlayerControl_FastForward */

int PlayerControl_Rewind ( void ) {

 return PlayerControl_Scroll ( -1 );

}  /* end PlayerControl_Rewind */

static SMS_ListNode* _do_change_lang ( SMS_List* apList, SMS_ListNode* apListNode ) {

 if ( !apListNode -> m_pNext )

  apListNode = apList -> m_pHead;

 else apListNode = apListNode -> m_pNext;

 return apListNode;

}  /* end _do_change_lang */

SMS_ListNode* PlayerControl_ChangeLang ( void ) {

 return s_pCurLang = _do_change_lang ( s_pLang, s_pCurLang );

}  /* end PlayerControl_ChangeLang */

SMS_ListNode* PlayerControl_ChangeSubLang ( void ) {

 return s_pCurSubLang = _do_change_lang ( s_pSubLang, s_pCurSubLang );

}  /* end PlayerControl_ChangeSubLang */

SMS_ListNode* PlayerControl_GetLang ( void ) {

 return s_pCurLang;

}  /* end PlayerControl_GetLang */

SMS_ListNode* PlayerControl_GetSubLang ( void ) {

 return s_pCurSubLang;

}  /* end PlayerControl_GetSubLang */

void PlayerControl_SwitchSubs ( void ) {

 if ( s_Player.m_Flags & SMS_FLAGS_SUBS )

  s_Player.m_Flags &= ~SMS_FLAGS_SUBS;

 else s_Player.m_Flags |= SMS_FLAGS_SUBS;

}  /* end PlayerControl_SwitchSubs */

void PlayerControl_UpdateInfo ( void ) {

 SMS_GUInfoDelete ( &s_pInfo );
 s_pInfo = SMS_GUInfo ( s_Player.m_pCont );

 s_Player.m_OSDPackets[ 0 ] = s_pInfo -> m_pGSPacket - 2;
 s_Player.m_OSDQWC    [ 0 ] = (  ( s_pInfo -> m_pGSPacket[ -1 ] >> 32 ) & 0xFFFF  ) + 1;
 s_Player.m_OSDPackets[ 1 ] = NULL;
 s_Player.m_OSDQWC    [ 1 ] = 0;

}  /* end PlayerControl_UpdateInfo */

static void _handle_timer_osd ( int aDir ) {

 unsigned int lOSD = s_Player.m_OSD;

 if ( ++lOSD == 3 ) {

  PlayerControl_UpdateInfo ();

 } else if ( lOSD >= 4 || !s_pTimerOSDHandlers[ lOSD ] ) {

  lOSD = 0;

 } else {

  int lIdx = lOSD - 1;

  s_Player.m_OSDPackets[ 0 ] = s_pSts  [ lIdx ] - 2;
  s_Player.m_OSDQWC    [ 0 ] = s_StsQWC[ lIdx ];
  s_Player.m_OSDPackets[ 1 ] = s_pPTS - 2;
  s_Player.m_OSDQWC    [ 1 ] = GS_TXT_PACKET_SIZE( 7 ) + 1;

  (   (  void ( * ) ( void )  )s_pTimerOSDHandlers[ lOSD ]   ) ();

  SMS_TimerSet ( 2, 1000, s_pTimerOSDHandlers[ lOSD ], NULL );

 }   /* end else */

 s_Player.m_OSD = lOSD;

}  /* end _handle_timer_osd */

static void _handle_adjust_osd ( int aDelta, int* apVal, int aLimit ) {

 int       lVal = *apVal;
 int       lS, lMS;
 char      lBuf[ 14 ];
 uint64_t* lpPaint = _U( s_pDelta );
 char      lSign;

 if ( lVal == aLimit && aDelta > 0 )

  lVal = aLimit - 10;

 else if ( lVal == -aLimit && aDelta < 0 ) lVal = -( aLimit - 10 );

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

 g_Config.m_AVDelta = s_Player.m_AVDelta;

}  /* end _handle_av_adjust_osd */

static void _handle_sv_adjust_osd ( int aDir ) {

 s_Player.m_OSDPackets[ 0 ] = s_pSV - 2;
 s_Player.m_OSDQWC    [ 0 ] = s_SVQWC;

 _handle_adjust_osd ( aDir, &s_Player.m_SVDelta, 30000 );

 g_Config.m_SVDelta = s_Player.m_SVDelta;

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

 aLen += PC_GSP_SIZE(   strlen (  _STR( lpNode )  )   );

 lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  aLen += strlen (  _STR( lpNode )  ) << 2;
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
 int           lPixFmt = g_GSCtx.m_VRAMFontPtr ? GSPixelFormat_PSMT4 : GSPixelFormat_PSMT4HL;
 int           lShift  = g_XShift;
 int           lW;

 apDMA = ( u64* )UNCACHED_SEG( apDMA );
 apDMA[ 0 ] = GIF_TAG( 1, 0, 0, 0, 1, 2 );
 apDMA[ 1 ] = GS_TEX0_1 | ( GS_PRIM << 4 );
 apDMA[ 2 ] = GS_SET_TEX0( g_GSCtx.m_VRAMFontPtr, 8, lPixFmt, 9, 9, GS_TEX_TCC_RGBA, GS_TEX_TFX_DECAL, 0, GSPixelFormat_PSMCT32, GS_TEX_CSM_CSM1, 1, GS_TEX_CLD_NOUPDATE );
 apDMA[ 3 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
 apDMA[ 5 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 lW = ( 512 >> lShift ) + 4;

 while ( 1 ) {

  unsigned int lX;
  unsigned int lY1, lY2;
  unsigned int lU, lV;
  int          lCurX;

  if ( anY >= -32 ) {

   unsigned int lLen   = strlen (  _STR( lpNode )  );
   int          lH     = 32;
   int          lDTY   = 0;
   int          lY     = anY;
   int          lDelta = 0;
   int          lDW;
   float        lAR;

   while (   GSFont_WidthEx (  _STR( lpNode ), lLen, lDelta  ) > lDispW && lDelta > -16   ) --lDelta;
   while (   GSFont_WidthEx (  _STR( lpNode ), lLen, lDelta  ) > lDispW                   ) --lLen;

   lAR      = ( 32.0F + lDelta ) / 32.0F;
   lDW      = lDelta << 4;
   lCurX    = (   g_GSCtx.m_Width - GSFont_WidthEx (  _STR( lpNode ), lLen, lDelta  )   ) >> 1;
   lDelta >>= lShift;

   if ( lCurX > g_GSCtx.m_Width ) lCurX = 0;

   if ( lY < 0 ) {

    lDTY   = -lY;
    lDTY <<= 4;
    lH    +=  lY;
    lY    = 0;

   }  /* end if */

   __asm__ __volatile__ (
    ".set noreorder\n\t"
    "pcpyld   $ra, $ra, $ra\n\t"
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
    "pcpyud   $ra, $ra, $ra\n\t"
    ".set reorder\n\t"
    : "=r"( lY1 ), "=r"( lY2 ) : "r"( lY ), "r"( lH ) : "a0", "a1", "a2", "v0", "v1"
   );

   for ( j = 0; j < lLen; ++j, k += 4 ) {

    unsigned char lChr = _STR( lpNode )[ j ] - ' ';

    lX     = lCurX << 4;
    lCurX += ( int )(  ( float )g_GSCharWidth[ lChr ] * lAR + 0.5F  );
    lX   >>= lShift;

    lU = ( lChr & 0x0000000F ) << 9;
    lV = ( lChr & 0xFFFFFFF0 ) << 5;

    apDMA[ k + 0 ] = GS_SET_UV( lU + 8, lV + lDTY + 8 );
    apDMA[ k + 1 ] = lX | lY1;
    apDMA[ k + 2 ] = GS_SET_UV( lU + 506, lV + 506 );
    apDMA[ k + 3 ] = ( lX + lW + lDW ) | lY2;

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
 uint64_t* lpPaint = _U( g_SCPaint );
 uint64_t  lX      = s_SBXY & 0xFFFF;
 uint64_t  lY1;
 uint64_t  lY2;
 uint64_t  lIncr;
 uint32_t  lQWC;
 int       lShift  = g_XShift;

 lY2   = s_SBY2 - s_SBY1;
 lIncr = lY2 / 5;

 lY1 = s_SBY1 + (  ( lY2 - lIncr ) >> 1  );
 lY2 = lY1 + lIncr;

 lY1 &= 0xFFFF0000;
 lY2 &= 0xFFFF0000;

 aPos = g_Config.m_ScrollBarNum - aPos;

 if ( g_Config.m_ScrollBarNum == 32 ) {
  lIncr = 148;
  lQWC  = 36;
 } else if ( g_Config.m_ScrollBarNum == 48 ) {
  lIncr = 98;
  lQWC  = 52;
 } else if ( g_Config.m_ScrollBarNum == 64 ) {
  lIncr = 74;
  lQWC  = 68;
 } else if ( g_Config.m_ScrollBarNum == 80 ) {
  lIncr = 59;
  lQWC  = 84;
 } else if ( g_Config.m_ScrollBarNum == 96 ) {
  lIncr = 49;
  lQWC  = 100;
 } else if ( g_Config.m_ScrollBarNum == 112 ) {
  lIncr = 42;
  lQWC  = 116;
 } else {  /* 128 */
  lIncr = 37;
  lQWC  = 132;
 }  /* end else */

 for (  i = 8; i < 8 + ( aPos << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = ( lX >> lShift ) | lY1;
  lX -= lIncr;
  lpPaint[ i + 1 ] = ( lX >> lShift ) | lY2;
  lX -= lIncr;

 }  /* end for */

 for (  ; i < 8 + ( g_Config.m_ScrollBarNum << 1 ); i += 2  ) {

  lpPaint[ i + 0 ] = ( lX >> lShift ) | s_SBY1;
  lX -= lIncr;
  lpPaint[ i + 1 ] = ( lX >> lShift ) | s_SBY2;
  lX -= lIncr;

 }  /* end for */

 g_IPUCtx.QueuePacket ( lQWC, g_SCPaint );

}  /* end PlayerControl_DisplayScrollBar */

static void _sc_repaint ( int64_t aTime, int anOp, int aPos ) {

 GS_VSync2 ( g_GSCtx.m_DrawDelay );
 s_Player.m_pIPUCtx -> Repaint ();

 if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_FLAGS_SUBS )  )
  s_Player.m_pSubCtx -> Display ( s_Player.m_VideoTime - s_Player.m_SVDelta );

 PlayerControl_DisplayTime ( anOp, aTime, 1 );
 PlayerControl_DisplayScrollBar ( aPos );

 s_Player.m_pIPUCtx -> Flush ();

}  /* end _sc_repaint */

int PlayerControl_ScrollBar (  void ( *InitQueues ) ( int )  ) {

 SMS_Container* lpCont     = s_Player.m_pCont;
 SMS_Stream*    lpStm      = lpCont -> m_pStm[ s_Player.m_VideoIdx ];
 FileContext*   lpFileCtx  = lpCont -> m_pFileCtx;
 IPUContext*    lpIPUCtx   = s_Player.m_pIPUCtx;
 int            retVal     = 0;
 uint64_t       lNextTime  = 0UL;
 int64_t        lTotalTime = lpCont -> m_Duration - lpCont -> m_StartTime;
 int64_t        lPassTime  = s_Player.m_VideoTime - lpCont -> m_StartTime;
 int            lResume    = 1;
 int64_t        lScale;
 float          lCurPos1;
 float          j;

 lpIPUCtx -> Suspend ();
begin:
 lScale   = (  lTotalTime / ( g_Config.m_ScrollBarNum + 1 )  );
 lCurPos1 = (  lPassTime / lScale  );

 for (  j = 0; j < ( g_Config.m_ScrollBarNum - 1 ); j += 1  )

  if (   (  lCurPos1 >= ( j - 0.5F )  ) &&
         (  lCurPos1 <= ( j + 0.5F )  )
  ) lCurPos1 = j;

 if (  lCurPos1 < 0.5F                                ) lCurPos1 = 0;
 if (  lCurPos1 > ( g_Config.m_ScrollBarNum - 0.5F )  ) lCurPos1 = g_Config.m_ScrollBarNum;

 int     lCurPos = SMS_clip ( lCurPos1, 0, g_Config.m_ScrollBarNum );
 int     lDir    = 0;
 int64_t lTime;
 int64_t lPos;

 lScale = (  lTotalTime / ( g_Config.m_ScrollBarNum + 1 )  );

 _sc_repaint ( lPassTime + lpCont -> m_StartTime, 2, lCurPos );

 if ( g_CMedia & 1 ) CDVD_Stop ();

 while ( 1 ) {

  uint32_t lButtons;

  if ( lResume == 1 ) {

   lTime = s_Player.m_VideoTime - lpCont -> m_StartTime;

  } else {
		    
   if ( lCurPos == 0 ) {
    
    lTime = 100LL;

   } else { 

    lTime = ( lScale * lCurPos );

   }  /* end else */

  }  /* end else */

  lPassTime = lTime;
  lButtons  = GUI_ReadButtons ();

  if ( lButtons && g_Timer > lNextTime ) {

   lNextTime = g_Timer + 200;

   if ( lButtons == SMS_PAD_START || lButtons == SMS_PAD_CROSS || lButtons == RC_ENTER || lButtons == RC_PLAY ) {

    _sc_repaint ( lTime + lpCont -> m_StartTime, 0, lCurPos );

    retVal = 1;
    break;

   } else if (  ( lButtons == SMS_PAD_RIGHT || lButtons == RC_SCAN_RIGHT ) && lCurPos < g_Config.m_ScrollBarNum  ) {

    lCurPos = SMS_clip (  ( lCurPos + 1 ), 0, g_Config.m_ScrollBarNum  );
    lResume = 0;
    _sc_repaint ( lTime + lpCont -> m_StartTime, 2, lCurPos );

   } else if (  ( lButtons == SMS_PAD_LEFT || lButtons == RC_SCAN_LEFT ) && lCurPos > 0  ) {

    lCurPos = SMS_clip (  ( lCurPos - 1 ), 0, g_Config.m_ScrollBarNum  );
    lResume = 0;
    _sc_repaint ( lTime + lpCont -> m_StartTime, 2, lCurPos );

   } else if ( lButtons == SMS_PAD_TRIANGLE || lButtons == RC_RESET || lButtons == RC_RETURN || lButtons == RC_STOP ) {

    lResume = 0;
    break;

   } else if ( lButtons == SMS_PAD_L1 || lButtons == RC_PREV ) {

    g_Config.m_ScrollBarNum -= 16;
 
    if ( g_Config.m_ScrollBarNum == 16 ) g_Config.m_ScrollBarNum = 128;
redo:
    _sb_init ();
    goto begin;

   } else if ( lButtons == SMS_PAD_R1 || lButtons == RC_NEXT ) {

    g_Config.m_ScrollBarNum += 16;
 
    if ( g_Config.m_ScrollBarNum == 144 ) g_Config.m_ScrollBarNum = 32;

    goto redo;

   }  /* end if */

  }  /* end if */

 }  /* end while */

 lpIPUCtx -> Resume ();

 if ( !lResume ) {

  SMS_CodecContext* lpVideoCtx = lpStm -> m_pCodec;

  SMS_Codec_MPEG12_Reset (
   SMS_MPEG12_RESET_QUEUE   |
   SMS_MPEG12_RESET_DECODER |
   SMS_MPEG12_RESET_TIME    |
   SMS_MPEG12_RESET_RECOVER
  );

  if ( retVal ) {

   if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Reset ();

   lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 0 );

   lDir = ( lTime - lPassTime );

   if ( lDir >  0 ) lDir =  1;
   if ( lDir <= 0 ) lDir = -1;

   lPos = SMS_Rescale (  lTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
 
   if (  lpCont -> Seek ( lpCont, s_Player.m_VideoIdx, lDir, lPos )  ) {

    lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, lpFileCtx -> m_StreamSize );

    s_Player.m_VideoTime = 0;
    s_Player.m_AudioTime = 0;

    retVal = -1;

   }  /* end if */

  }  /* end if */

  InitQueues ( 0 );
  lpVideoCtx -> HWCtl ( lpVideoCtx, SMS_HWC_Reset );

 }  /* end if */

 if ( g_CMedia & 1 ) SMS_TimerWait ( 1000 );

 while (  GUI_ReadButtons ()  );

 if ( g_CMedia & 1 ) {
  CDDA_Standby     ();
  CDDA_Synchronize ();
 }  /* end if */

 return retVal;

}  /* end PlayerControl_ScrollBar */

void PlayerControl_UpdateDuration ( unsigned int anIdx, int64_t aDuration ) {

 char      lBuff[ 18 ];
 uint64_t* lpPaint;

 if ( !anIdx || aDuration ) {

  int lM = aDuration / 60;
  int lH = lM        / 60;

  aDuration %= 60;
  lM        %= 60;

  sprintf ( lBuff, "%1d:%02d:%02ld", lH, lM, aDuration );

 } else {

  memset ( lBuff, ' ', 7 );
  lBuff[ 7 ] = '\x00';

 }  /* end else */

 lpPaint = _U(  s_pPTS + anIdx * GS_TXT_PACKET_SIZE( 7 )  );

 GSFont_Render ( lBuff, 7, s_PTSX[ anIdx ], OSD_Y_POS, lpPaint );

}  /* end PlayerControl_UpdateDuration */

void PlayerControl_UpdateItemNr ( void ) {

 char  lBuff[ 32 ];
 char* lpPtr = lBuff;

 sprintf ( lpPtr, "%d", s_Player.m_PlayItemNr );
 lpPtr += strlen ( lpPtr );
 sprintf ( lpPtr, "/%d", s_Player.m_pCont -> m_pPlayList ? s_Player.m_pCont -> m_pPlayList -> m_Size : 1 );
 lpPtr += strlen ( lpPtr );
 memset ( lpPtr, ' ', 16 );

 s_Player.m_OSDQWC[ 6 ] = GS_TXT_PACKET_SIZE( 9 ) >> 1;

 GSFont_Render (  lBuff, 9, 16, OSD_Y_POS, _U( g_OSDNR )  );

}  /* end PlayerControl_UpdateItemNr */

void PlayerControl_ChangeLangOSD ( int aTID ) {

 SMS_ListNode* lpNode = PlayerControl_ChangeLang ();
 int           lLen, lDWC, lW;
 char          lBuf[ 128 ];

 sprintf (  lBuf, g_pFmt3, STR_AUDIO.m_pStr, g_ColonSStr, _STR( s_pCurLang )  );
 lLen = strlen ( lBuf );
 lDWC = GS_TXT_PACKET_SIZE( lLen );
 lW   = GSFont_Width ( lBuf, lLen );

 s_Player.m_PrevAudioIdx = s_Player.m_AudioIdx;
 s_Player.m_AudioIdx     = ( unsigned int )lpNode -> m_Param;

 SuspendThread ( aTID );
  DMA_Wait ( DMAC_VIF1 );
  PlayerControl_UpdateInfo ();
  GSContext_DeleteList ( s_pLangOSD );
  s_pLangOSD = GSContext_NewList ( lDWC );
  GSFont_Render ( lBuf, lLen, g_GSCtx.m_Width - lW - 16, OSD_Y_POS, s_pLangOSD );
  s_pLangOSD[ -1 ] = VIF_DIRECT( lDWC >> 1 );
  SyncDCache ( &s_pLangOSD[ -2 ], &s_pLangOSD[ -2 ] + lDWC );
  s_Player.m_OSDPackets[ 8 ] = &s_pLangOSD[ -2 ];
  s_Player.m_OSDQWC    [ 8 ] = ( lDWC >> 1 ) + 1;
  s_Player.m_Flags |= SMS_FLAGS_DISPCTL;
  SMS_TimerSet ( 1, 2000, TimerHandler, NULL );
 ResumeThread ( aTID );

}  /* end PlayerControl_ChangeLangOSD */
