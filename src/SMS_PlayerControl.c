#include "SMS_PlayerControl.h"
#include "SMS_Container.h"
#include "SMS_VideoBuffer.h"
#include "GS.h"
#include "GUI.h"
#include "VIF.h"
#include "IPU.h"
#include "SPU.h"
#include "DMA.h"
#include "Timer.h"

#include <libpad.h>
#include <stdio.h>
#include <string.h>

static uint64_t s_VCPaint[                  66 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_VCErase[                  16 ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_TDTotal[ FONT_GSP_SIZE(  9 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_TDPTS  [ FONT_GSP_SIZE( 13 ) ] SMS_DATA_SECTION __attribute__(   (  aligned( 16 )  )   );
static uint64_t s_IntTime;

static void TimerHandler ( void ) {

 if ( g_Timer >= s_IntTime ) {

  g_IPUCtx.iQueuePacket ( 8, s_VCErase );
  Timer_iRegisterHandler ( 1, NULL );

 }  /* end if */

}  /* end TimerHandler */

int PlayerControl_Index2Volume ( SMS_Player* apPlayer ) {

 static unsigned s_lScale[ 25 ] = {
      0,   150,   400,   560,   800,
   1070,  1330,  1530,  1730,  2190,
   2550,  3010,  3520,  4080,  4490,
   5250,  5970,  6990,  8000,  9080,
  10450, 12030, 13560, 14940, 16383
 };

 return s_lScale[ apPlayer -> m_Volume = SMS_clip ( apPlayer -> m_Volume, 0, 24 ) ];

}  /* end PlayerControl_Index2Volume */

static void _FormatTime ( char* apBuf, uint64_t aTime ) {

 int lS   = ( int )( aTime / SMS_TIME_BASE );
 int lM   = lS / 60;
 int lH   = lM / 60;

 lS %= 60;
 lM %= 60;

 sprintf (  apBuf, "%02d:%02d:%02d", lH, lM, lS );

}  /* end _FormatTime */

void PlayerControl_Init ( SMS_Player* apPlayer ) {

 GSContext* lpGSCtx = apPlayer -> m_pGUICtx -> m_pGSCtx;
 uint64_t   lX      = ( 20 << 4 ) + ( lpGSCtx -> m_OffsetX  << 4 );
 uint64_t   lY      = (   (  ( lpGSCtx -> m_Height - 288 ) >> 1  ) << 3   ) + ( lpGSCtx -> m_OffsetY << 4 );
 uint64_t   lDX     = 30 << 4;
 uint64_t*  lpPaint = _U( s_VCPaint );
 uint64_t*  lpErase = _U( s_VCErase );
 char       lBuff[ 32 ];

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

 lY  = ( 52                 << 3 ) + ( lpGSCtx -> m_OffsetY << 4 );
 lX  =                               ( lpGSCtx -> m_OffsetX << 4 );
 lDX = ( lpGSCtx -> m_Width << 4 ) + ( lpGSCtx -> m_OffsetX << 4 );

 lBuff[ 0 ] = ' ';
 _FormatTime ( &lBuff[ 1 ], apPlayer -> m_pCont -> m_Duration );

 lpPaint = _U( s_TDTotal );

 lpGSCtx -> TextGSPacket ( lpGSCtx -> m_Width - 144, 52, 0, lBuff, 0, &lpPaint );

}  /* end PlayerControl_Init */

void PlayerControl_DisplayTime ( SMS_Player* apPlayer, int anOp, uint64_t aTime, int afDraw ) {

 char       lBuff[ 14 ];
 char*      lpSrc;
 uint64_t*  lpPacket = _U( s_TDPTS );
 GSContext* lpGSCtx  = apPlayer -> m_pGUICtx -> m_pGSCtx;

 switch ( anOp ) {

  case  0: lpSrc = "Play "; break;
  case  1: lpSrc = "FFwd "; break;
  case -1: lpSrc = " Rew "; break;
  default: return;

 }  /* end switch */

 strcpy ( lBuff, lpSrc );

 _FormatTime ( &lBuff[ 5 ], aTime );

 lpGSCtx -> TextGSPacket ( lpGSCtx -> m_Width - 336, 52, 0, lBuff, 0, &lpPacket );

 if ( !afDraw ) {

  apPlayer -> m_pIPUCtx -> PQueuePacket ( 22, s_TDTotal );
  apPlayer -> m_pIPUCtx -> PQueuePacket ( 30, s_TDPTS   );

 } else {

  uint64_t  lChain[ 4 ] __attribute__(   ( aligned( 16 )  )   );
  uint64_t* lpChain = _U( lChain );

  lpChain[ 0 ] = DMA_TAG(  30, 1, DMA_REF,  0, ( u32 )s_TDPTS,   0  );
  lpChain[ 1 ] = 0LL;
  lpChain[ 2 ] = DMA_TAG(  22, 1, DMA_REFE, 0, ( u32 )s_TDTotal, 0  );
  lpChain[ 3 ] = 0LL;
  DMA_SendChainToVIF1 ( lChain );

 }  /* end else */

}  /* end PlayerControl_DisplayTime */

void PlayerControl_AdjustVolume ( SMS_Player* apPlayer, int aDelta ) {

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

 apPlayer -> m_pSPUCtx -> SetVolume (
  PlayerControl_Index2Volume ( apPlayer )
 );

}  /* end PlayerControl_AdjustVolume */

static int PlayerControl_Scroll ( SMS_Player* apPlayer, int aDir ) {

 SMS_Container* lpCont    = apPlayer -> m_pCont;
 SMS_Stream*    lpStm     = lpCont -> m_pStm[ apPlayer -> m_VideoIdx ];
 FileContext*   lpFileCtx = lpCont -> m_pFileCtx;
 IPUContext*    lpIPUCtx  = apPlayer -> m_pIPUCtx;
 int64_t        lTime     = apPlayer -> m_VideoTime;
 SMS_AVPacket*  lpPacket  = lpCont -> NewPacket ( lpCont );
 int            retVal    = 0;
 int64_t        lIncr     = 3000LL * aDir;
 uint64_t       lNextTime = 0LL;
 uint32_t       lFilePos  = 0U;

 lpIPUCtx -> Sync ();
 PlayerControl_DisplayTime ( apPlayer, aDir, lTime, 1 );

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos,  0 );
 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 10 );

 while ( 1 ) {

  uint64_t         lDisplayTime = g_Timer;
  int64_t          lPos         = SMS_Rescale (  lTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
  int              lSize        = lpCont -> Seek ( lpCont, apPlayer -> m_VideoIdx, aDir, lPos );
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

  apPlayer -> m_pVideoCodec -> Decode (
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
      PlayerControl_DisplayTime ( apPlayer, 0, lpPacket -> m_PTS, 0 );
      lpIPUCtx -> Display ( lpFrame );

      retVal = 1;
      goto end;

     }  /* end if */

    }  /* end if */

    lDiff = 300LL - ( g_Timer - lDisplayTime );

   } while ( lDiff > 0 );

   lpIPUCtx -> Sync ();
   PlayerControl_DisplayTime ( apPlayer, aDir, lpPacket -> m_PTS, 0 );
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

 apPlayer -> m_VideoTime = lpPacket -> m_PTS;
 apPlayer -> m_AudioTime = lpPacket -> m_PTS;

 lpPacket -> Destroy ( lpPacket );

 return retVal;

}  /* end PlayerControl_Scroll */

int PlayerControl_FastForward ( SMS_Player* apPlayer ) {

 return PlayerControl_Scroll ( apPlayer, 1 );

}  /* end PlayerControl_FastForward */

int PlayerControl_Rewind ( SMS_Player* apPlayer ) {

 return PlayerControl_Scroll ( apPlayer, -1 );

}  /* end PlayerControl_Rewind */
