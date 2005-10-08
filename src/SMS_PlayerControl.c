#include "SMS_PlayerControl.h"
#include "GS.h"
#include "GUI.h"
#include "VIF.h"
#include "IPU.h"
#include "SPU.h"
#include "Timer.h"

static uint64_t s_VCPaint[ 66 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_VCErase[ 16 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_IntTime;

static void TimerHandler ( void ) {

 if ( g_Timer >= s_IntTime ) {

  g_IPUCtx.iQueuePacket ( 8, s_VCErase );
  Timer_iRegisterHandler ( 1, NULL );

 }  /* end if */

}  /* end TimerHandler */

int Index2Volume ( SMS_Player* apPlayer ) {

 static unsigned s_lScale[ 25 ] = {
      0,   150,   400,   560,   800,
   1070,  1330,  1530,  1730,  2190,
   2550,  3010,  3520,  4080,  4490,
   5250,  5970,  6990,  8000,  9080,
  10450, 12030, 13560, 14940, 16383
 };

 return s_lScale[ apPlayer -> m_Volume = SMS_clip ( apPlayer -> m_Volume, 0, 24 ) ];

}  /* end Index2Volume */

void InitPlayerControl ( SMS_Player* apPlayer ) {

 GSContext* lpGSCtx = apPlayer -> m_pGUICtx -> m_pGSCtx;
 uint64_t   lX      = ( 20 << 4 ) + ( lpGSCtx -> m_OffsetX  << 4 );
 uint64_t   lY      = (   (  ( lpGSCtx -> m_Height - 288 ) >> 1  ) << 3   ) + ( lpGSCtx -> m_OffsetY << 4 );
 uint64_t   lDX     = 30 << 4;
 uint64_t*  lpPaint = _U( s_VCPaint );
 uint64_t*  lpErase = _U( s_VCErase );

 lpPaint[ 0 ] =                   lpErase[ 0 ] = 0;
 lpPaint[ 1 ] = VIF_DIRECT( 32 ); lpErase[ 1 ] = VIF_DIRECT( 7 );

 lpPaint[ 2 ] = lpErase[ 2 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 );
 lpPaint[ 3 ] = lpErase[ 3 ] = GIF_AD;
 lpPaint[ 4 ] = lpErase[ 4 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 lpPaint[ 5 ] = lpErase[ 5 ] = GS_TEST_1;

 lpPaint[ 6 ] = lpErase[ 6 ] = GIF_TAG( 1, 0, 0, 0, 1, 4 );
 lpPaint[ 7 ] = lpErase[ 7 ] = GS_RGBAQ | ( GS_PRIM << 4 ) | ( GS_XYZ2 << 8 ) | ( GS_XYZ2 << 12 );
 lpPaint[ 8 ] = lpErase[ 8 ] = GS_SETREG_RGBA( 0x00, 0xFF, 0x00, 0x00 );
 lpPaint[ 9 ] = lpErase[ 9 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, lpGSCtx -> m_PrimCtx, 0 );

 lpPaint[ 10 ] = lpErase[ 10 ] = GS_SETREG_XYZ(        lX,                lY, 0  );
 lpPaint[ 11 ] = lpErase[ 11 ] = GS_SETREG_XYZ(  lX + lDX, lY + ( 288 << 3 ), 0  );

 lpPaint[ 12 ] = GIF_TAG( 1, 0, 0, 0, 0, 1 ); lpErase[ 12 ] = GIF_TAG( 1, 1, 0, 0, 0, 1 );
 lpPaint[ 13 ] = lpErase[ 13 ] = GIF_AD;
 lpPaint[ 14 ] = lpErase[ 14 ] = GS_SETREG_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 2 );
 lpPaint[ 15 ] = lpErase[ 15 ] = GS_TEST_1;

 lpPaint[ 16 ] = GIF_TAG( 24, 1, 0, 0, 1, 2  );
 lpPaint[ 17 ] = GS_XYZ2 | ( GS_XYZ2 << 4 );

}  /* end InitPlayerControl */

void AdjustVolume ( SMS_Player* apPlayer, int aDelta ) {

 int i,    lVol    = apPlayer -> m_Volume = SMS_clip ( apPlayer -> m_Volume + aDelta, 0, 24 );
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

 apPlayer -> m_pSPUCtx -> SetVolume (  Index2Volume ( apPlayer )  );

}  /* end AdjustVolume */
