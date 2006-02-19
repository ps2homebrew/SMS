/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 - 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_Player.h"
#include "SMS_PlayerControl.h"
#include "SMS_PlayerMenu.h"
#include "SMS_GS.h"
#include "SMS_GUI.h"
#include "SMS_IPU.h"
#include "SMS_SPU.h"
#include "SMS_FileContext.h"
#include "SMS_AudioBuffer.h"
#include "SMS_VideoBuffer.h"
#include "SMS_Timer.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"
#include "SMS_PAD.h"
#include "SMS_Config.h"
#include "SMS_GUIMenu.h"
#include "SMS_SubtitleContext.h"
#include "SMS_List.h"
#include "SMS_PlayerBallSim.h"
#include "SMS_DMA.h"
#include "SMS_Locale.h"

#include <kernel.h>
#include <stdio.h>
#include <limits.h>
#include <libhdd.h>
#include <string.h>

typedef struct SMS_AudioPacket {

 uint8_t* m_pBuf;
 int64_t  m_PTS;

} SMS_AudioPacket;

#define SMS_VPACKET_QSIZE    384
#define SMS_APACKET_QSIZE    384
#define SMS_VIDEO_QUEUE_SIZE  10
#define SMS_AUDIO_QUEUE_SIZE  10

#define SMS_FLAGS_STOP     0x00000001
#define SMS_FLAGS_PAUSE    0x00000002
#define SMS_FLAGS_MENU     0x00000004
#define SMS_FLAGS_EXIT     0x00000008
#define SMS_FLAGS_VSCROLL  0x00000010
#define SMS_FLAGS_ASCROLL  0x00000020
#define SMS_FLAGS_AASCROLL 0x00000040
#define SMS_FLAGS_ABSCROLL 0x00000080

#define SEMA_R_PUT_VIDEO s_Semas[ 0 ]
#define SEMA_D_PUT_VIDEO s_Semas[ 1 ]
#define SEMA_R_PUT_AUDIO s_Semas[ 2 ]
#define SEMA_D_PUT_AUDIO s_Semas[ 3 ]

#define THREAD_ID_VR s_ThreadIDs[ 0 ]
#define THREAD_ID_VD s_ThreadIDs[ 1 ]
#define THREAD_ID_AR s_ThreadIDs[ 2 ]
#define THREAD_ID_AD s_ThreadIDs[ 3 ]

SMS_Player s_Player;

static SMS_AVPacket**    s_VPacketBuffer;
static SMS_AVPacket**    s_APacketBuffer;
static SMS_FrameBuffer** s_VideoBuffer;
static SMS_AudioPacket*  s_AudioBuffer;

static SMS_RB_CREATE( s_VPacketQueue, SMS_AVPacket*    );
static SMS_RB_CREATE( s_APacketQueue, SMS_AVPacket*    );
static SMS_RB_CREATE( s_VideoQueue,   SMS_FrameBuffer* );
static SMS_RB_CREATE( s_AudioQueue,   SMS_AudioPacket  );

extern void* _gp;

static int               s_Semas    [ 4 ];
static int               s_ThreadIDs[ 4 ];
static int               s_SemaPauseAudio;
static int               s_SemaPauseVideo;
static int               s_SemaAckPause;
static int               s_MainThreadID;
static uint8_t           s_VideoRStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t           s_VideoDStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t           s_AudioRStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t           s_AudioDStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );
static SMS_FrameBuffer*  s_pFrame;
static SMS_AudioBuffer*  s_AudioSamples;
static int               s_nPackets;
static int               s_Flags;
static SMS_CodecContext* s_pAudioCodec;

static void ( *ExitThreadFunc ) ( void );
static int  ( *FFwdFunc       ) ( void );
static int  ( *RewFunc        ) ( void );

extern void SMS_PlayerMenu ( void );

#ifdef LOCK_QUEUES
static int s_SemaPALock;
static int s_SemaPVLock;
static int s_SemaVLock;
static int s_SemaALock;

# define LOCK( s ) WaitSema ( s )
# define UNLOCK( s ) SignalSema ( s )
#else
# define LOCK( s )
# define UNLOCK( s )
#endif  /* LOCK_QUEUES */

static void _draw_text ( char* apStr ) {

 int            lLen   = strlen ( apStr );
 int            lWidth = GSFont_Width ( apStr, lLen );
 int            lX     = ( g_GSCtx.m_Width  - lWidth ) / 2;
 int            lY     = ( g_GSCtx.m_Height -     32 ) / 2;
 unsigned long* lpDMA;

 if ( s_Player.m_pIPUCtx ) s_Player.m_pIPUCtx -> Sync ();

 lpDMA = GSContext_NewPacket (  1, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Init  );
 GSFont_Render ( apStr, lLen, lX, lY, lpDMA );
 GSContext_Flush ( 1, GSFlushMethod_DeleteLists );

}  /* end _draw_text */

static void _set_colors ( void ) {

 GSContext_SetTextColor ( 0, g_Palette[ g_Config.m_PlayerSCNIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 1, g_Palette[ g_Config.m_PlayerSCBIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 2, g_Palette[ g_Config.m_PlayerSCIIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 3, g_Palette[ g_Config.m_PlayerSCUIdx - 1 ] | 0x80000000 );

}  /* end _set_colors */

static void _prepare_ipu_context ( void ) {

 int lWidth  = 0;
 int lHeight = 0;

 if ( s_Player.m_VideoIdx != 0xFFFFFFFF ) {

  SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;

  lWidth  = lpCodecCtx -> m_Width;
  lHeight = lpCodecCtx -> m_Height;

 }  /* end if */

 GUI_Redraw ( GUIRedrawMethod_RedrawClear );
 GS_VSync ();
 GSContext_Init ( g_Config.m_DisplayMode, GSZTest_On, GSDoubleBuffer_Off );
 GS_VSync ();

 s_Player.m_pIPUCtx = IPU_InitContext ( lWidth, lHeight );

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Prepare ();

 _set_colors ();

}  /* end _prepare_ipu_context */

static void _clear_packet_queues ( void ) {

 SMS_AVPacket* lpPacket;

 while (  !SMS_RB_EMPTY( s_VPacketQueue )  ) {

  lpPacket = *SMS_RB_POPSLOT( s_VPacketQueue );
  lpPacket -> Destroy ( lpPacket );
  SMS_RB_POPADVANCE( s_VPacketQueue );

 }  /* end while */

 while (  !SMS_RB_EMPTY( s_APacketQueue )  ) {

  lpPacket = *SMS_RB_POPSLOT( s_APacketQueue );
  lpPacket -> Destroy ( lpPacket );
  SMS_RB_POPADVANCE( s_APacketQueue );

 }  /* end while */

}  /* end _clear_packet_queues */

static void _terminate_threads ( int afDelete ) {

 int i;

 ExitThreadFunc = afDelete ? ExitDeleteThread : ExitThread;

 s_Flags |= SMS_FLAGS_STOP;

 for ( i = 0; i < 4; ++i ) {

  SignalSema ( s_Semas[ i ] );
  WakeupThread ( s_ThreadIDs[ i ] );

 }  /* end for */

 for ( i = 0; i < 4; ++i ) SleepThread ();

 if ( !afDelete ) for ( i = 0; i < 4; ++i ) CancelWakeupThread ( s_ThreadIDs[ i ] );

}  /* end _terminate_threads */

static void _init_queues ( int afCreate ) {

 ee_sema_t lSema;
 int       i;

 if ( afCreate ) {

  s_VPacketBuffer = ( SMS_AVPacket**    )calloc (  SMS_VPACKET_QSIZE,    sizeof ( SMS_AVPacket*    )  );
  s_APacketBuffer = ( SMS_AVPacket**    )calloc (  SMS_APACKET_QSIZE,    sizeof ( SMS_AVPacket*    )  );
  s_VideoBuffer   = ( SMS_FrameBuffer** )calloc (  SMS_VIDEO_QUEUE_SIZE, sizeof ( SMS_FrameBuffer* )  );
  s_AudioBuffer   = ( SMS_AudioPacket*  )calloc (  SMS_AUDIO_QUEUE_SIZE, sizeof ( SMS_AudioPacket  )  );

  lSema.init_count = 0;
  lSema.max_count  = 1;
  s_SemaPauseAudio = CreateSema ( &lSema );
  s_SemaPauseVideo = CreateSema ( &lSema );
  lSema.max_count  = 2;
  s_SemaAckPause   = CreateSema ( &lSema );

 } else {

  _terminate_threads ( 0 );

  s_Flags = 0;

  _clear_packet_queues ();

  if ( s_AudioSamples -> Reset ) s_AudioSamples -> Reset ();

  for ( i = 0; i < 4; ++i ) DeleteSema ( s_Semas[ i ] );
#ifdef LOCK_QUEUES
  DeleteSema ( s_SemaPALock );
  DeleteSema ( s_SemaPVLock );
  DeleteSema ( s_SemaVLock  );
  DeleteSema ( s_SemaALock  );
#endif  /* end LOCK_QUEUES */
 }  /* end else */

 SMS_RB_INIT( s_VPacketQueue, s_VPacketBuffer, SMS_VPACKET_QSIZE    );
 SMS_RB_INIT( s_APacketQueue, s_APacketBuffer, SMS_APACKET_QSIZE    );
 SMS_RB_INIT( s_VideoQueue,   s_VideoBuffer,   SMS_VIDEO_QUEUE_SIZE );
 SMS_RB_INIT( s_AudioQueue,   s_AudioBuffer,   SMS_AUDIO_QUEUE_SIZE );

 lSema.init_count =
 lSema.max_count  = SMS_VPACKET_QSIZE - 1;
 SEMA_D_PUT_VIDEO = CreateSema ( &lSema );

 lSema.init_count =
 lSema.max_count  = SMS_APACKET_QSIZE - 1;
 SEMA_D_PUT_AUDIO = CreateSema ( &lSema );

 lSema.init_count = 
 lSema.max_count  = SMS_VIDEO_QUEUE_SIZE - 1;
 SEMA_R_PUT_VIDEO = CreateSema ( &lSema );

 lSema.init_count =
 lSema.max_count  = SMS_AUDIO_QUEUE_SIZE - 1;
 SEMA_R_PUT_AUDIO = CreateSema ( &lSema );
#ifdef LOCK_QUEUES
 lSema.max_count  = 1;
 lSema.init_count = 1;
 s_SemaPALock = CreateSema ( &lSema );
 s_SemaPVLock = CreateSema ( &lSema );
 s_SemaVLock  = CreateSema ( &lSema );
 s_SemaALock  = CreateSema ( &lSema );
#endif  /* end LOCK_QUEUES */
 for ( i = 0; i < 4; ++i ) StartThread ( s_ThreadIDs[ i ], s_Player.m_pCont );

}  /* end _init_queues */

static void _sms_dummy_video_renderer ( void* apParam ) {

 static u64 s_lDMA[ 14 ] __attribute__(   (  aligned( 16 )  )   );

 u64* lpDMA  = _U( s_lDMA );
 int  i      = 0;

 SleepThread ();

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 2 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 2 ], 0  );
 lpDMA[ i++ ] = 0;

 if ( s_Player.m_pCont -> m_pPlayList ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 4 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 4 ], 0  );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 3 ], 1, DMATAG_ID_REF,  0, ( u32 )s_Player.m_OSDPackets[ 3 ], 0  );
 lpDMA[ i++ ] = 0;

 if ( g_Config.m_PlayerFlags & SMS_PF_ANIM ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 5 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 5 ], 0   );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 6 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 6 ], 0   );
 lpDMA[ i++ ] = 0;

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 1 ] - 1, 1, DMATAG_ID_REF, 0, ( u32 )( s_Player.m_OSDPackets[ 1 ] + 2 ), 0   );
 lpDMA[ i++ ] = 0;

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 0 ] - 1, 1, DMATAG_ID_REFE, 0, ( u32 )( s_Player.m_OSDPackets[ 0 ] + 2 ), 0   );
 lpDMA[ i   ] = 0;

 while ( 1 ) {

  if (  s_Flags & ( SMS_FLAGS_STOP | SMS_FLAGS_EXIT )  ) break;

  s_Player.m_pIPUCtx -> Sync ();

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   _draw_text ( STR_PAUSE.m_pStr );

   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  if ( s_Flags & SMS_FLAGS_VSCROLL ) {

   if ( s_Player.m_OSDPLPos == s_Player.m_OSDPLRes ) {

    s_Flags &= ~SMS_FLAGS_VSCROLL;

    if (  !( s_Flags & SMS_FLAGS_AASCROLL )  )

     SignalSema ( s_SemaAckPause );

    else s_Flags &= ~( SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL );

   } else {

    s_Player.m_OSDPLPos   += s_Player.m_OSDPLInc;
    s_Player.m_OSDQWC[ 4 ] = PlayerControl_GSPacket (
     s_Player.m_OSDPLPos, s_Player.m_pCont -> m_pPlayList, s_Player.m_OSDPackets[ 4 ]
    );
    lpDMA[ 2 ] = DMA_TAG(  s_Player.m_OSDQWC[ 4 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 4 ], 0  );

   }  /* end else */

  }  /* end else */

  SMS_PlayerBallSim_Update ( s_Player.m_OSDPackets[ 5 ] );

  __asm__ __volatile__( "sync\n\t" );

  s_Player.m_pIPUCtx -> Display ( s_lDMA );

  Timer_Wait ( 64 );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_dummy_video_renderer */

static void _sms_video_renderer ( void* apParam ) {

 int lfAudio = s_Player.m_AudioIdx != 0xFFFFFFFF;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VideoQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   if (  !( s_Flags & SMS_FLAGS_MENU )  ) _draw_text ( STR_PAUSE.m_pStr );

   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  LOCK( s_SemaVLock );
   s_pFrame = *SMS_RB_POPSLOT( s_VideoQueue );
   SMS_RB_POPADVANCE( s_VideoQueue );
  UNLOCK( s_SemaVLock );

  if ( lfAudio ) {

   int64_t lDiff = ( s_Player.m_VideoTime = s_pFrame -> m_PTS ) - s_Player.m_AudioTime + s_Player.m_AVDelta;

   if ( s_Flags & SMS_FLAGS_STOP ) {

    SignalSema ( SEMA_R_PUT_VIDEO );
    break;

   }  /* end if */

   if ( lDiff > 20 ) Timer_Wait ( lDiff >> 2 );

  }  /* end if */

  s_Player.m_pIPUCtx -> Sync ();

  if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_PF_SUBS )  ) s_Player.m_pSubCtx -> Display ( s_pFrame -> m_SPTS - s_Player.m_SVDelta );

  if ( s_Player.m_OSD ) {

   s_Player.m_pIPUCtx -> PQueuePacket ( s_Player.m_OSDQWC[ 0 ], s_Player.m_OSDPackets[ 0 ] );
   s_Player.m_pIPUCtx -> PQueuePacket ( s_Player.m_OSDQWC[ 1 ], s_Player.m_OSDPackets[ 1 ] );

  }  /* end if */

  s_Player.m_pIPUCtx -> Display ( s_pFrame );

  SignalSema ( SEMA_R_PUT_VIDEO );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_video_renderer */

static void _sms_video_decoder ( void* apParam ) {

 SMS_AVPacket*     lpPacket;
 SMS_FrameBuffer*  lpFrame;
 int64_t           lLastPPTS = 0L;
 SMS_CodecContext* lpCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VPacketQueue )  ) break;

  LOCK( s_SemaPVLock );
   lpPacket = *SMS_RB_POPSLOT( s_VPacketQueue );
   SMS_RB_POPADVANCE( s_VPacketQueue );
  UNLOCK( s_SemaPVLock );

  --s_nPackets;

  if (  s_Player.m_pVideoCodec -> Decode (
         lpCtx, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
        )
  ) {

   if ( s_Flags & SMS_FLAGS_STOP ) {

    lpPacket -> Destroy ( lpPacket );
    break;

   }  /* end if */

   WaitSema ( SEMA_R_PUT_VIDEO );

   if ( lpCtx -> m_HasBFrames && lpFrame -> m_FrameType != SMS_FT_B_TYPE ) {

    lpFrame -> m_PTS = lLastPPTS;
    lLastPPTS        = lpPacket -> m_PTS;

   } else lpFrame -> m_PTS = lpPacket -> m_PTS;

   lpFrame -> m_SPTS = lpPacket -> m_PTS;

   LOCK( s_SemaVLock );
    *SMS_RB_PUSHSLOT( s_VideoQueue ) = lpFrame;
    SMS_RB_PUSHADVANCE( s_VideoQueue );
   UNLOCK( s_SemaVLock );

   WakeupThread ( THREAD_ID_VR );

  }  /* end if */

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( SEMA_D_PUT_VIDEO );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_video_decoder */

static void _sms_audio_only_renderer ( void* apParam ) {

 SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ 0 ] -> m_pCodec;
 uint32_t          lBPS       = lpCodecCtx -> m_BitsPerSample;
 float             lMult;

 if ( !lBPS ) lBPS = 16;

 lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

 s_Player.m_AudioTime = 0LL;

 while ( 1 ) {

  int              lLen;
  SMS_AudioPacket* lpFrame;

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );
   s_Player.m_pSPUCtx -> SetVolume (  PlayerControl_Index2Volume ()  );

  }  /* end if */

  LOCK( s_SemaALock );
   lpFrame = SMS_RB_POPSLOT( s_AudioQueue );
   SMS_RB_POPADVANCE( s_AudioQueue );
  UNLOCK( s_SemaALock );

  SignalSema ( SEMA_R_PUT_AUDIO );

  if ( s_Flags & SMS_FLAGS_STOP ) {

   s_AudioSamples -> Release ();
   break;

  }  /* end if */

  lLen = *( int* )lpFrame -> m_pBuf;

  if ( lpFrame -> m_PTS == SMS_STPTS_VALUE ) {

   PlayerControl_UpdateDuration ( 1, s_Player.m_pCont -> m_Duration );

   s_Player.m_AudioTime = 0;

   lpCodecCtx = s_pAudioCodec;

   if ( lpCodecCtx -> m_Channels   != s_Player.m_AudioChannels   ||
        lpCodecCtx -> m_SampleRate != s_Player.m_AudioSampleRate
   ) {

    s_Player.m_AudioChannels   = lpCodecCtx -> m_Channels;
    s_Player.m_AudioSampleRate = lpCodecCtx -> m_SampleRate;

    lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

    s_Player.m_pSPUCtx -> Destroy ();
    s_Player.m_pSPUCtx = SPU_InitContext (
     s_Player.m_AudioChannels, s_Player.m_AudioSampleRate, PlayerControl_Index2Volume ()
    );

   }  /* end if */

   if ( s_Player.m_pCont -> m_pPlayList ) {

    if ( !s_Player.m_pPlayItem ) {

     s_Player.m_pPlayItem  = s_Player.m_pCont -> m_pPlayList -> m_pHead;
     s_Player.m_PlayItemNr = 1;
     s_Flags              &= ~SMS_FLAGS_ASCROLL;

    } else {

     if (  !( s_Flags & SMS_FLAGS_ABSCROLL )  ) {

      s_Player.m_pPlayItem = s_Player.m_pPlayItem -> m_pNext;
      ++s_Player.m_PlayItemNr;

     } else {

      s_Player.m_pPlayItem = s_Player.m_pPlayItem -> m_pPrev;
      s_Flags             &= ~SMS_FLAGS_ABSCROLL;
      --s_Player.m_PlayItemNr;

     }  /* end else */

     PlayerControl_UpdateItemNr ();

     if (  !( s_Flags & SMS_FLAGS_ASCROLL )  ) {

      s_Player.m_OSDPLInc = -2;
      s_Player.m_OSDPLRes = s_Player.m_OSDPLPos - 32;
      s_Flags            |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL );

     } else s_Flags &= ~SMS_FLAGS_ASCROLL;

    }  /* end else */

   }  /* end if */

  } else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult );

  s_Player.m_pSPUCtx -> PlayPCM ( lpFrame -> m_pBuf );

  s_AudioSamples -> Release ();

 }  /* end while */

 s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_only_renderer */

static void _sms_audio_only_decoder ( void* apParam ) {

 static SMS_AudioBuffer s_DummyBuffer;

 SMS_AVPacket* lpPacket;
 int64_t       lPTS = 0;

 s_pAudioCodec  = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec;
 s_AudioSamples = &s_DummyBuffer;

 while ( 1 ) {

  SMS_AudioPacket* lpFrame;

  SleepThread ();

  if (  SMS_RB_EMPTY( s_APacketQueue )  ) break;

  LOCK( s_SemaPALock );
   lpPacket = *SMS_RB_POPSLOT( s_APacketQueue );
   SMS_RB_POPADVANCE( s_APacketQueue );
  UNLOCK( s_SemaPALock );

  --s_nPackets;

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

   s_AudioSamples -> m_Len = 0;

   if ( lPTS != SMS_STPTS_VALUE )

    lPTS = lpPacket -> m_PTS;

   else if ( s_pAudioCodec != s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec ) {

    free ( s_pAudioCodec );
    s_pAudioCodec = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec;

   }  /* end else */

   do {

    if (  s_Player.m_pAudioCodec -> Decode (
           s_pAudioCodec, ( void** )&s_AudioSamples, lpPacket -> m_pData, lpPacket -> m_Size
          )
    ) {

     if ( s_Flags & SMS_FLAGS_STOP ) {

      lpPacket -> Destroy ( lpPacket );
      goto end;

     }  /* end if */

     WaitSema ( SEMA_R_PUT_AUDIO );

     LOCK( s_SemaALock );
      lpFrame = SMS_RB_PUSHSLOT( s_AudioQueue );
      lpFrame -> m_pBuf = s_AudioSamples -> m_pOut;
      lpFrame -> m_PTS  = lPTS;
      SMS_RB_PUSHADVANCE( s_AudioQueue );
     UNLOCK( s_SemaALock );

     WakeupThread ( THREAD_ID_AR );

     lPTS = SMS_NOPTS_VALUE;

    } else break;

   } while ( s_AudioSamples -> m_Len > 0 );

  }  /* end if */

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( SEMA_D_PUT_AUDIO );

 }  /* end while */
end:
 if ( s_pAudioCodec != s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec ) free ( s_pAudioCodec );

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_only_decoder */

static void _sms_audio_renderer ( void* apParam ) {

 uint32_t lBPS     = 0;
 int64_t  lEndTime = 0;
 float    lMult    = 0.0F;

 s_Player.m_AudioTime = 0LL;

 if ( s_Player.m_AudioIdx != 0xFFFFFFFF ) {

  lBPS     = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec -> m_BitsPerSample;
  lEndTime = s_Player.m_pCont -> m_Duration - 200LL;

  if ( !lBPS ) lBPS = 16;

  lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

 }  /* end if */

 while ( 1 ) {

  int              lLen;
  SMS_AudioPacket* lpFrame;

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );
   s_Player.m_pSPUCtx -> SetVolume (  PlayerControl_Index2Volume ()  );

  }  /* end if */

  LOCK( s_SemaALock );
   lpFrame = SMS_RB_POPSLOT( s_AudioQueue );
   SMS_RB_POPADVANCE( s_AudioQueue );
  UNLOCK( s_SemaALock );

  SignalSema ( SEMA_R_PUT_AUDIO );

  if ( s_Flags & SMS_FLAGS_STOP ) {

   s_AudioSamples -> Release ();
   break;

  }  /* end if */

  lLen = *( int* )lpFrame -> m_pBuf;

  if ( lpFrame -> m_PTS != SMS_NOPTS_VALUE )

   s_Player.m_AudioTime = lpFrame -> m_PTS;

  else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult );

  s_Player.m_pSPUCtx -> PlayPCM ( lpFrame -> m_pBuf );

  if ( s_Player.m_AudioTime >= lEndTime ) {

   s_Player.m_pSPUCtx -> SetVolume ( 0 );
   lEndTime = 0x7FFFFFFFFFFFFFFFL;

  }  /* end if */

  s_AudioSamples -> Release ();

 }  /* end while */

 if ( s_Player.m_pSPUCtx ) s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_renderer */

static void _sms_audio_decoder ( void* apParam ) {

 static SMS_AudioBuffer s_DummyBuffer;

 SMS_AVPacket* lpPacket;
 int64_t       lPTS;

 s_AudioSamples = &s_DummyBuffer;

 while ( 1 ) {

  SMS_AudioPacket* lpFrame;

  SleepThread ();

  if (  SMS_RB_EMPTY( s_APacketQueue )  ) break;

  LOCK( s_SemaPALock );
   lpPacket = *SMS_RB_POPSLOT( s_APacketQueue );
   SMS_RB_POPADVANCE( s_APacketQueue );
  UNLOCK( s_SemaPALock );

  --s_nPackets;

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

   s_AudioSamples -> m_Len = 0;
   lPTS                    = lpPacket -> m_PTS;

   do {

    if (  s_Player.m_pAudioCodec -> Decode (
           s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec, ( void** )&s_AudioSamples, lpPacket -> m_pData, lpPacket -> m_Size
          )
    ) {

     if ( s_Flags & SMS_FLAGS_STOP ) {

      lpPacket -> Destroy ( lpPacket );
      goto end;

     }  /* end if */

     WaitSema ( SEMA_R_PUT_AUDIO );

     LOCK( s_SemaALock );
      lpFrame = SMS_RB_PUSHSLOT( s_AudioQueue );
      lpFrame -> m_pBuf = s_AudioSamples -> m_pOut;
      lpFrame -> m_PTS  = lPTS;
      SMS_RB_PUSHADVANCE( s_AudioQueue );
     UNLOCK( s_SemaALock );

     WakeupThread ( THREAD_ID_AR );

    } else break;

    lPTS = SMS_NOPTS_VALUE;

   } while ( s_AudioSamples -> m_Len > 0 );

  }  /* end if */

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( SEMA_D_PUT_AUDIO );

 }  /* end while */
end:
 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_decoder */

static void _sms_play_alarm ( int anID, unsigned short aTime, void* apArg ) {

 iSignalSema (  *( int* )apArg  );

}  /* end _sms_play_alarm */

static int _FFwd_AV ( void ) {

 _init_queues ( 0 );

 return PlayerControl_FastForward ();

}  /* end _FFwd_AV */

static int _Rew_AV ( void ) {

 _init_queues ( 0 );

 return PlayerControl_Rewind ();

}  /* end _Rew_AV */

static int _FFwd_A ( void ) {

 int            retVal = 1;
 SMS_Container* lpCont = s_Player.m_pCont;

 if (   s_Player.m_pPlayItem && s_Player.m_pPlayItem -> m_pNext && !(  s_Flags & ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL )  )   ) {

  _init_queues ( 0 );

  if ( s_Player.m_pPlayItem -> m_pNext ) {

   s_Player.m_OSDPLInc = -2;
   s_Player.m_OSDPLRes = s_Player.m_OSDPLPos - 32;
   s_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL );
   WakeupThread ( THREAD_ID_VR );

   retVal = lpCont -> Seek (  lpCont, 0, 0, ( uint32_t )s_Player.m_pPlayItem -> m_pNext  );

   WaitSema ( s_SemaAckPause );

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _FFwd_A */

static int _Rew_A ( void ) {

 int            retVal = 1;
 SMS_Container* lpCont = s_Player.m_pCont;

 if (   s_Player.m_pPlayItem && s_Player.m_pPlayItem -> m_pPrev && !(  s_Flags & ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_ABSCROLL )  )   ) {

  _init_queues ( 0 );

  if ( s_Player.m_pPlayItem -> m_pPrev ) {

   s_Player.m_OSDPLInc = 2;
   s_Player.m_OSDPLRes = s_Player.m_OSDPLPos + 32;
   s_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_ABSCROLL );
   WakeupThread ( THREAD_ID_VR );

   retVal = lpCont -> Seek (  lpCont, 0, 0, ( uint32_t )s_Player.m_pPlayItem -> m_pPrev  );

   WaitSema ( s_SemaAckPause );

  }  /* end if */

  WakeupThread ( THREAD_ID_VR );

 }  /* end if */

 return retVal;

}  /* end _Rew_A */

static void _sms_play ( void ) {

 int           lSize = 0;
 SMS_AVPacket* lpPacket;
 char          lBuff[ 128 ];
 SMS_Stream**  lpStms = s_Player.m_pCont -> m_pStm;
 ee_sema_t     lSemaParam;
 int           lSema;
 int           lfNoVideo = s_Player.m_VideoIdx == 0xFFFFFFFF;
 uint32_t      lnDec     = 1L;
 uint64_t      lNextTime = 0;
 uint64_t      lPOffTime = g_Timer + g_Config.m_PowerOff;

 lSemaParam.init_count = 0;
 lSemaParam.max_count  = 1;
 lSema = CreateSema ( &lSemaParam );

 if ( !lfNoVideo ) {

  if ( s_Player.m_pSubFileCtx ) {

   float lFPS = ( float )lpStms[ s_Player.m_VideoIdx ] -> m_RealFrameRate / 
                ( float )lpStms[ s_Player.m_VideoIdx ] -> m_RealFrameRateBase;

   GUI_Progress ( STR_LOADING_SUBTITLES.m_pStr, 100 );
   s_Player.m_pSubFileCtx -> Stream ( s_Player.m_pSubFileCtx, 0, 10 );
   s_Player.m_pSubCtx = SubtitleContext_Init (
    s_Player.m_pSubFileCtx, s_Player.m_SubFormat, lFPS
   );
   s_Player.m_pSubFileCtx -> Stream ( s_Player.m_pSubFileCtx, 0, 0 );
   s_Player.m_pSubFileCtx -> Destroy ( s_Player.m_pSubFileCtx );
 
   if ( s_Player.m_pSubCtx -> m_ErrorCode ) {

    sprintf (
     lBuff, STR_SUB_ERROR.m_pStr,
     s_Player.m_pSubCtx -> m_ErrorCode == SubtitleError_Format ?
     STR_FORMAT.m_pStr : STR_SEQUENCE.m_pStr, s_Player.m_pSubCtx -> m_ErrorLine
    );
    GUI_Error ( lBuff );
    s_Player.m_pSubCtx = NULL;

   }  /* end if */

  }  /* end if */

  FFwdFunc = _FFwd_AV;
  RewFunc  = _Rew_AV;

 } else {

  FFwdFunc = _FFwd_A;
  RewFunc  = _Rew_A;

 }  /* end else */

 if ( s_Player.m_AudioIdx != 0xFFFFFFFF ) {

  s_Player.m_pSPUCtx = SPU_InitContext (
   s_Player.m_AudioChannels, s_Player.m_AudioSampleRate,
   PlayerControl_Index2Volume ()
  );
  ++lnDec;

 }  /* end if */
 
 s_nPackets = 0;

 if ( s_Player.m_pFileCtx ) {

  sprintf ( lBuff, STR_BUFFERING_FILE.m_pStr, s_Player.m_pCont -> m_pName  );

  GUI_Status ( lBuff );
  s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, s_Player.m_pFileCtx -> m_StreamSize );

 }  /* end if */

 _prepare_ipu_context ();

 PlayerControl_Init ();

 if ( lfNoVideo ) {

  s_Player.m_pIPUCtx -> Suspend ();
  s_Player.m_OSDPackets[ 5 ] = SMS_PlayerBallSim_Init ( &s_Player.m_OSDQWC[ 5 ] );
  s_Player.m_pIPUCtx -> Resume  ();

  PlayerControl_HandleOSD ( 0, 0 );
  PlayerControl_UpdateDuration ( 0, s_Player.m_pCont -> m_Duration );
  PlayerControl_UpdateItemNr ();
  WakeupThread ( THREAD_ID_VR );

 } else s_Player.m_OSDPackets[ 5 ] = NULL;

 while ( 1 ) {

  uint32_t lButtons = GUI_ReadButtons ();

  if ( lButtons ) {

   lPOffTime = g_Timer + g_Config.m_PowerOff;

   if ( g_Timer <= lNextTime ) goto skip;

   lNextTime = g_Timer + 200;

   if ( lButtons & SMS_PAD_SELECT ) {

    unsigned long int lBtn;

    if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Inactive || lfNoVideo ) {

     s_Flags |= SMS_FLAGS_PAUSE;

     for ( lBtn = 0; lBtn < lnDec; ++lBtn ) WaitSema ( s_SemaAckPause );

     while ( 1 ) {

      lBtn = GUI_WaitButtons ( SMS_PAD_START | SMS_PAD_SELECT, 200 );

      if ( lBtn == SMS_PAD_START ) {

       lPOffTime = g_Timer + g_Config.m_PowerOff;
       break;

      }  /* end if */

      if ( lBtn == SMS_PAD_SELECT ) {

       s_Player.m_pIPUCtx -> Sync    ();
       s_Player.m_pIPUCtx -> Repaint ();

      }  /* end if */

     }  /* end while */
resume:
     Timer_Wait ( 500 );
     s_Flags &= ~SMS_FLAGS_PAUSE;
     SignalSema ( s_SemaPauseAudio );
     SignalSema ( s_SemaPauseVideo );
     lSize = 0;

    } else {

     s_Flags |= SMS_FLAGS_PAUSE | SMS_FLAGS_MENU;

     for ( lBtn = 0; lBtn < lnDec; ++lBtn ) WaitSema ( s_SemaAckPause );

     s_Flags &= ~( SMS_FLAGS_PAUSE | SMS_FLAGS_MENU );

     if (  !PlayerControl_ScrollBar (
             _init_queues, s_SemaPauseAudio, s_SemaPauseVideo
            )
     ) {

      s_Flags |= SMS_FLAGS_STOP;
      break;

     }  /* end if */

     lSize = 0;

    }  /* end else */

   } else if (  ( lButtons & SMS_PAD_START ) && !lfNoVideo  ) {

    s_Flags |= SMS_FLAGS_PAUSE | SMS_FLAGS_MENU;

    for ( lButtons = 0; lButtons < lnDec; ++lButtons ) WaitSema ( s_SemaAckPause );

    s_Player.m_pIPUCtx -> Suspend ();
    SMS_PlayerMenu ();
    s_Player.m_pIPUCtx -> Resume  ();
    s_Flags &= ~SMS_FLAGS_MENU;

    lPOffTime = g_Timer + g_Config.m_PowerOff;

    goto resume;

   } else if (  lButtons & SMS_PAD_TRIANGLE && *( int* )&s_Player.m_AudioTime  ) {

    _terminate_threads ( 1 );
    _clear_packet_queues ();
    _draw_text ( STR_STOPPING.m_pStr );

    break;

   } else if ( lButtons == SMS_PAD_UP ) {

    PlayerControl_AdjustVolume ( 1 );

   } else if ( lButtons == SMS_PAD_DOWN ) {

    PlayerControl_AdjustVolume ( -1 );

   } else if (  lButtons == SMS_PAD_RIGHT && ( s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE )  ) {

    if (  !FFwdFunc ()  ) {

     s_Flags |= SMS_FLAGS_STOP;
     break;

    }  /* end if */

    lSize = 0;

   } else if (  lButtons == SMS_PAD_LEFT && ( s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE )  ) {

    if (  !RewFunc ()  ) {

     s_Flags |= SMS_FLAGS_STOP;
     break;

    }  /* end if */

    lSize = 0;

   } else if ( lButtons == SMS_PAD_SQUARE ) {

    if ( ++s_Player.m_PanScan == 5 ) s_Player.m_PanScan = 0;

    g_Config.m_PlayerFlags &= 0x0FFFFFFF;
    g_Config.m_PlayerFlags |= ( s_Player.m_PanScan << 28 );

    s_Player.m_pIPUCtx -> ChangeMode ( s_Player.m_PanScan );

   } else if ( lButtons == SMS_PAD_L1 ) {

    s_Player.m_pIPUCtx -> Pan ( 1 );

   } else if ( lButtons == SMS_PAD_R1 ) {

    s_Player.m_pIPUCtx -> Pan ( -1 );

   } else if (  lButtons == ( SMS_PAD_R1 | SMS_PAD_L1 )  ) {

    s_Player.m_pIPUCtx -> Reset ();

   } else if ( lButtons == SMS_PAD_CROSS && !lfNoVideo ) {

    PlayerControl_HandleOSD ( 0, 0 );

   } else if ( lButtons == SMS_PAD_L2 && !lfNoVideo ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, -250 );

    else if ( s_Player.m_OSD == 4 ) PlayerControl_HandleOSD ( 2, -250 );

   } else if ( lButtons == SMS_PAD_R2 && !lfNoVideo ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, 250 );

    else if ( s_Player.m_OSD == 4 && !lfNoVideo ) PlayerControl_HandleOSD ( 2, 250 );

   } else if (  lButtons == ( SMS_PAD_L2 | SMS_PAD_R2 ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD == 3 ) {

     s_Player.m_AVDelta = 0;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if ( s_Player.m_OSD == 4 ) {

     s_Player.m_SVDelta = 0;
     PlayerControl_HandleOSD ( 2, 0 );

    }  /* end if */

   } else if ( lButtons == SMS_PAD_CIRCLE && !lfNoVideo ) {

    if ( s_Player.m_OSD < 3 ) {

     s_Player.m_OSD = 3;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if (  ++s_Player.m_OSD == ( s_Player.m_pSubCtx ? 5 : 4 )  ) {

     s_Player.m_OSD = 0;

    } else PlayerControl_HandleOSD ( 2, 0 );

   }  /* end if */

  } else if ( g_Config.m_PowerOff > 0 && g_Timer >= lPOffTime ) hddPowerOff ();
skip:
  g_CDDASpeed = s_nPackets < 128 ? 4 : 3;

  lpPacket = s_Player.m_pCont -> NewPacket ( s_Player.m_pCont );
nextPacket:
  if ( lSize >= 0 ) {

   lSize = s_Player.m_pCont -> ReadPacket ( lpPacket );

   if ( lSize <= 0 ) {

    lpPacket -> Destroy ( lpPacket );
    continue;

   }  /* end if */

  } else {

   ee_sema_t lVSemaParam;

   ReferSemaStatus ( SEMA_D_PUT_VIDEO, &lVSemaParam );
   ReferSemaStatus ( SEMA_D_PUT_AUDIO, &lSemaParam  );

   lpPacket -> Destroy ( lpPacket );

   if ( lVSemaParam.count == SMS_VPACKET_QSIZE - 1 &&
        lSemaParam.count  == SMS_APACKET_QSIZE - 1
   ) break;

   SetAlarm ( 1000, _sms_play_alarm, &lSema );
   WaitSema ( lSema );

   continue;

  }  /* end else */

  if ( lpPacket -> m_StmIdx == s_Player.m_VideoIdx ) {

   WaitSema ( SEMA_D_PUT_VIDEO );

   LOCK( s_SemaPVLock );
    *SMS_RB_PUSHSLOT( s_VPacketQueue ) = lpPacket;
    SMS_RB_PUSHADVANCE( s_VPacketQueue );
   UNLOCK( s_SemaPVLock );

   ++s_nPackets;

   WakeupThread ( THREAD_ID_VD );

  } else if ( lpStms[ lpPacket -> m_StmIdx ] -> m_Flags & SMS_STRM_FLAGS_AUDIO ) {

   WaitSema ( SEMA_D_PUT_AUDIO );

   LOCK( s_SemaPALock );
    *SMS_RB_PUSHSLOT( s_APacketQueue ) = lpPacket;
    SMS_RB_PUSHADVANCE( s_APacketQueue );
   UNLOCK( s_SemaPALock );

   ++s_nPackets;

   WakeupThread ( THREAD_ID_AD );

  } else goto nextPacket;

 }  /* end while */

 s_Flags |= SMS_FLAGS_EXIT;

 DeleteSema ( lSema );

}  /* end _sms_play */

static void _Destroy ( void ) {

 DiskType lType;

 if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

  WakeupThread ( THREAD_ID_VD );
  SleepThread ();

 }  /* end if */

 DeleteSema ( SEMA_D_PUT_VIDEO );
 free ( s_VPacketBuffer );

 if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

  WakeupThread ( THREAD_ID_VR );  
  SleepThread ();

 }  /* end if */

 DeleteSema ( SEMA_R_PUT_VIDEO );
 DeleteSema ( s_SemaPauseVideo );
 DeleteSema ( s_SemaAckPause   );
 free ( s_VideoBuffer );
#ifdef LOCK_QUEUES
 DeleteSema ( s_SemaPVLock );
 DeleteSema ( s_SemaVLock  );
#endif  /* LOCK_QUEUES */
 if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

  WakeupThread ( THREAD_ID_AD );
  SleepThread ();

 }  /* end if */

 DeleteSema ( SEMA_D_PUT_AUDIO );
 free ( s_APacketBuffer );

 if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

  WakeupThread ( THREAD_ID_AR );
  SleepThread ();

 }  /* end if */

 DeleteSema ( SEMA_R_PUT_AUDIO );
 DeleteSema ( s_SemaPauseAudio );
 free ( s_AudioBuffer );
#ifdef LOCK_QUEUES
 DeleteSema ( s_SemaPALock );
 DeleteSema ( s_SemaALock  );
#endif  /* LOCK_QUEUES */

 if ( s_Player.m_pSPUCtx ) {

  s_Player.m_pSPUCtx -> Destroy ();
  s_Player.m_pSPUCtx = NULL;

 }  /* end if */

 s_Player.m_pIPUCtx -> Sync    ();
 s_Player.m_pIPUCtx -> Destroy ();
 s_Player.m_pIPUCtx = NULL;

 lType = CDDA_DiskType ();

 if ( lType != DiskType_None ) CDVD_Stop ();

 if ( s_Player.m_pSubCtx ) {

  s_Player.m_pSubCtx -> Destroy ();
  s_Player.m_pSubCtx = NULL;

 }  /* end if */

 s_Player.m_pCont -> Destroy ( s_Player.m_pCont );
 s_Player.m_pCont = NULL;

 Timer_RegisterHandler ( 2, NULL );
 PlayerControl_Destroy ();

 if (  g_Config.m_PowerOff < 0 && !( s_Flags & SMS_FLAGS_STOP )  ) hddPowerOff ();

 SMS_PlayerBallSim_Destroy ( s_Player.m_OSDPackets[ 5 ] );

}  /* end _Destroy */

SMS_Player* SMS_InitPlayer ( FileContext* apFileCtx, FileContext* apSubFileCtx, unsigned int aSubFormat ) {

 void ( *lpAR ) ( void* );
 void ( *lpAD ) ( void* );
 void ( *lpVR ) ( void* );

 s_VPacketBuffer = NULL;
 s_APacketBuffer = NULL;
 s_VideoBuffer   = NULL;
 s_AudioBuffer   = NULL;
 s_Flags         =    0;

 GUI_Status ( STR_DETECTING_FFORMAT.m_pStr );

 s_Player.m_pCont = SMS_GetContainer ( apFileCtx );
 s_MainThreadID   = GetThreadId ();

 if ( s_Player.m_pCont != NULL ) {

  int         i;
  ee_thread_t lThread;
  ee_thread_t lCurrentThread;

  s_Player.m_VideoIdx = 0xFFFFFFFF;
  s_Player.m_AudioIdx = 0xFFFFFFFF;

  for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i )

   if ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_Type == SMS_CodecTypeVideo && s_Player.m_VideoIdx == 0xFFFFFFFF ) {

    SMS_CodecOpen ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec );

    if ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec ) {

     s_Player.m_pVideoCodec = s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec;
     s_Player.m_pVideoCodec -> Init ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec );

     s_Player.m_VideoIdx = i;

    }  /* end if */

   } else if ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_Type == SMS_CodecTypeAudio && s_Player.m_AudioIdx == 0xFFFFFFFF ) {

    SMS_CodecOpen ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec );

    if ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec ) {

     s_Player.m_pAudioCodec = s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec;

     if ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_ID == SMS_CodecID_AC3 ) s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec -> m_Channels = 2;

     s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ i ] -> m_pCodec );

     s_Player.m_AudioIdx        = i;
     s_Player.m_AudioChannels   = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec -> m_Channels;
     s_Player.m_AudioSampleRate = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec -> m_SampleRate;

    }  /* end if */

   }  /* end if */

  if ( s_Player.m_VideoIdx != 0xFFFFFFFF ) {

   lpAR = _sms_audio_renderer;
   lpAD = _sms_audio_decoder;
   lpVR = _sms_video_renderer;

  } else {

   lpAR = _sms_audio_only_renderer;
   lpAD = _sms_audio_only_decoder;
   lpVR = _sms_dummy_video_renderer;

   s_Player.m_PlayItemNr = 1;
   PlayerControl_UpdateItemNr ();

  }  /* end else */

  s_Player.m_pFileCtx    = s_Player.m_pCont -> m_pFileCtx;
  s_Player.m_pSubFileCtx = apSubFileCtx;
  s_Player.m_SubFormat   = aSubFormat;
  s_Player.Destroy       = _Destroy;

  ReferThreadStatus ( s_MainThreadID, &lCurrentThread );

  s_Player.Play = _sms_play;

  lThread.stack_size       = sizeof ( s_VideoRStack );
  lThread.stack            = s_VideoRStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpVR;
  THREAD_ID_VR = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_VideoDStack );
  lThread.stack            = s_VideoDStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = _sms_video_decoder;
  THREAD_ID_VD = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_AudioRStack );
  lThread.stack            = s_AudioRStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpAR;
  THREAD_ID_AR = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_AudioDStack );
  lThread.stack            = s_AudioDStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpAD;
  THREAD_ID_AD = CreateThread ( &lThread );

  _init_queues ( 1 );

  ExitThreadFunc = ExitDeleteThread;

  s_Player.m_Flags    |= SMS_PF_SUBS;
  s_Player.m_OSD       = 0;
  s_Player.m_PanScan   = g_Config.m_PlayerFlags >> 28;
  s_Player.m_AVDelta   = 0;
  s_Player.m_SVDelta   = 0;
  s_Player.SetColors   = _set_colors;
  s_Player.m_OSDPLPos  = g_GSCtx.m_Height - 96;
  s_Player.m_pPlayItem = NULL;

 } else {

  if ( apSubFileCtx ) apSubFileCtx -> Destroy ( apSubFileCtx );

 }  /* end else */

 return s_Player.m_pCont ? &s_Player : NULL;

}  /* end SMS_InitPlayer */
