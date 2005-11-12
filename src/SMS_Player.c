/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_Player.h"
#include "SMS_PlayerControl.h"
#include "GS.h"
#include "GUI.h"
#include "IPU.h"
#include "SPU.h"
#include "FileContext.h"
#include "SMS_AudioBuffer.h"
#include "SMS_VideoBuffer.h"
#include "Timer.h"
#include "CDDA.h"
#include "DMA.h"
#include "Config.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <libpad.h>
#include <limits.h>

typedef struct SMS_AudioPacket {

 uint8_t* m_pBuf;
 float    m_PTS;

} SMS_AudioPacket;

#define SMS_VPACKET_QSIZE    384
#define SMS_APACKET_QSIZE    384
#define SMS_VIDEO_QUEUE_SIZE  10
#define SMS_AUDIO_QUEUE_SIZE  10

#define SMS_FLAGS_STOP  0x00000001
#define SMS_FLAGS_PAUSE 0x00000002

#define SEMA_R_PUT_VIDEO s_Semas[ 0 ]
#define SEMA_D_PUT_VIDEO s_Semas[ 1 ]
#define SEMA_R_PUT_AUDIO s_Semas[ 2 ]
#define SEMA_D_PUT_AUDIO s_Semas[ 3 ]

#define THREAD_ID_VR s_ThreadIDs[ 0 ]
#define THREAD_ID_VD s_ThreadIDs[ 1 ]
#define THREAD_ID_AR s_ThreadIDs[ 2 ]
#define THREAD_ID_AD s_ThreadIDs[ 3 ]

static SMS_Player s_Player;

static SMS_AVPacket**    s_VPacketBuffer;
static SMS_AVPacket**    s_APacketBuffer;
static SMS_FrameBuffer** s_VideoBuffer;
static SMS_AudioPacket*  s_AudioBuffer;

static SMS_RB_CREATE( s_VPacketQueue, SMS_AVPacket*    );
static SMS_RB_CREATE( s_APacketQueue, SMS_AVPacket*    );
static SMS_RB_CREATE( s_VideoQueue,   SMS_FrameBuffer* );
static SMS_RB_CREATE( s_AudioQueue,   SMS_AudioPacket  );

extern void* _gp;

static int              s_Semas    [ 4 ];
static int              s_ThreadIDs[ 4 ];
static int              s_SemaPauseAudio;
static int              s_SemaPauseVideo;
static int              s_MainThreadID;
static uint8_t          s_VideoRStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_VideoDStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_AudioRStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_AudioDStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static SMS_FrameBuffer* s_pFrame;
static SMS_AudioBuffer* s_AudioSamples;
static int              s_nPackets;
static int              s_Flags;

static void ( *ExitThreadFunc ) ( void );

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

 int lWidth = s_Player.m_pGUICtx -> m_pGSCtx -> TextWidth ( apStr, 0 );
 int lX     = ( s_Player.m_pGUICtx -> m_pGSCtx -> m_Width  - lWidth ) / 2;
 int lY     = ( s_Player.m_pGUICtx -> m_pGSCtx -> m_Height -     26 ) / 2;

 if ( s_Player.m_pIPUCtx ) s_Player.m_pIPUCtx -> Sync ();

 s_Player.m_pGUICtx -> m_pGSCtx -> DrawText ( lX, lY, 0, apStr, 0, 0 );

}  /* end _draw_text */

static void _prepare_ipu_context ( void ) {

 GSContext* lpGSCtx = s_Player.m_pGUICtx -> m_pGSCtx;

 lpGSCtx -> m_fDblBuf = GS_OFF;
 lpGSCtx -> m_fZBuf   = GS_ON;

 lpGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );
 lpGSCtx -> VSync ();
 lpGSCtx -> InitScreen ( g_Config.m_DisplayCharset );
 lpGSCtx -> VSync ();
 lpGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 s_Player.m_pIPUCtx = IPU_InitContext (
  lpGSCtx,
  s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_Codec.m_Width,
  s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_Codec.m_Height
 );

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

 } else {

  _terminate_threads ( 0 );

  s_Flags = 0;

  _clear_packet_queues ();

  s_AudioSamples -> Reset ();

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

static void _sms_video_renderer ( void* apParam ) {

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VideoQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   _draw_text ( "Pause" );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  LOCK( s_SemaVLock );
   s_pFrame = *SMS_RB_POPSLOT( s_VideoQueue );
   SMS_RB_POPADVANCE( s_VideoQueue );
  UNLOCK( s_SemaVLock );

  if ( s_Player.m_AudioIdx != 0xFFFFFFFF ) {

   float lDiff = ( s_Player.m_VideoTime = s_pFrame -> m_PTS ) - s_Player.m_AudioTime;

   if ( s_Flags & SMS_FLAGS_STOP ) {

    SignalSema ( SEMA_R_PUT_VIDEO );
    break;

   }  /* end if */

   if ( lDiff > 20.0F ) Timer_Wait ( lDiff / 4.0F );

  }  /* end if */

  s_Player.m_pIPUCtx -> Sync ();
  s_Player.m_pIPUCtx -> Display ( s_pFrame );

  SignalSema ( SEMA_R_PUT_VIDEO );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_video_renderer */

static void _sms_video_decoder ( void* apParam ) {

 SMS_AVPacket*     lpPacket;
 SMS_FrameBuffer*  lpFrame;
 uint64_t          lLastPPTS = 0LL;
 SMS_CodecContext* lpCtx = &s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_Codec;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VPacketQueue )  ) break;

  LOCK( s_SemaPVLock );
   lpPacket = *SMS_RB_POPSLOT( s_VPacketQueue );
   SMS_RB_POPADVANCE( s_VPacketQueue );
  UNLOCK( s_SemaPVLock );

  --s_nPackets;

  if (  s_Player.m_pVideoCodec -> Decode (
         &s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
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

static void _sms_audio_renderer ( void* apParam ) {

 uint32_t lnChannels = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec.m_Channels;
 uint32_t lBPS       = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec.m_BitsPerSample;
 uint32_t lSPS       = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec.m_SampleRate;
 float    lEndTime   = s_Player.m_pCont -> m_Duration - 200.0F;

 if ( !lBPS ) lBPS = 16;

 s_Player.m_AudioTime = 0.0F;

 while ( 1 ) {

  int              lLen;
  SMS_AudioPacket* lpFrame;

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   WaitSema ( s_SemaPauseAudio );
   s_Player.m_pSPUCtx -> SetVolume (
    PlayerControl_Index2Volume ( &s_Player )
   );

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

  else s_Player.m_AudioTime += (  lLen * (  1000.0F / ( lBPS * lnChannels / 8 )  )   ) / ( float )lSPS;

  s_Player.m_pSPUCtx -> PlayPCM ( lpFrame -> m_pBuf );

  if ( s_Player.m_AudioTime >= lEndTime ) {

   s_Player.m_pSPUCtx -> SetVolume ( 0 );
   lEndTime = 1E30F;

  }  /* end if */

  s_AudioSamples -> Release ();

 }  /* end while */

 s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_renderer */

static void _sms_audio_decoder ( void* apParam ) {

 static SMS_AudioBuffer s_DummyBuffer;

 SMS_AVPacket* lpPacket;
 uint64_t      lPTS;

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

  s_AudioSamples -> m_Len = 0;
  lPTS                    = lpPacket -> m_PTS;

  do {

   if (  s_Player.m_pAudioCodec -> Decode (
          &s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec, ( void** )&s_AudioSamples, lpPacket -> m_pData, lpPacket -> m_Size
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

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( SEMA_D_PUT_AUDIO );

 }  /* end while */
end:
 WakeupThread ( s_MainThreadID );
 ExitThreadFunc ();

}  /* end _sms_audio_decoder */

static void _sms_play ( void ) {

 int           lSize;
 SMS_AVPacket* lpPacket;
 char          lBuff[ 128 ];

 sprintf ( lBuff, "Buffering %s file...", s_Player.m_pCont -> m_pName  );

 s_nPackets = 0;

 s_Player.m_pGUICtx -> Status ( lBuff );
 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, s_Player.m_pFileCtx -> m_StreamSize );

 s_Player.m_pSPUCtx = SPU_InitContext (
  s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec.m_Channels,
  s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_Codec.m_SampleRate,
  PlayerControl_Index2Volume ( &s_Player )
 );

 _prepare_ipu_context ();

 if ( s_Player.m_pSPUCtx && s_Player.m_pIPUCtx ) {

  uint64_t lNextTime = 0;

  PlayerControl_Init ( &s_Player );

  while ( 1 ) {

   uint32_t lButtons = GUI_ReadButtons ();

   if ( lButtons ) {

    if ( g_Timer <= lNextTime ) goto skip;

    lNextTime = g_Timer + 200;

    if ( lButtons & PAD_SELECT ) {

     unsigned long int lBtn;

     s_Flags |= SMS_FLAGS_PAUSE;

     while ( 1 ) {

      lBtn = GUI_WaitButton ( PAD_START | PAD_SELECT, 200 );

      if ( lBtn == PAD_START ) break;

      if ( lBtn == PAD_SELECT ) {

       s_Player.m_pIPUCtx -> Sync ();
       s_Player.m_pIPUCtx -> Display ( s_pFrame );

      }  /* end if */

     }  /* end while */

     s_Flags &= ~SMS_FLAGS_PAUSE;
     SignalSema ( s_SemaPauseAudio );
     SignalSema ( s_SemaPauseVideo );

    } else if (  lButtons & PAD_TRIANGLE && *( int* )&s_Player.m_AudioTime  ) {

     _terminate_threads ( 1 );
     _clear_packet_queues ();
     _draw_text ( "Stopping" );

     break;

    } else if ( lButtons == PAD_UP ) {

     PlayerControl_AdjustVolume ( &s_Player, 1 );

    } else if ( lButtons == PAD_DOWN ) {

     PlayerControl_AdjustVolume ( &s_Player, -1 );

    } else if (  lButtons == PAD_RIGHT && ( s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE ) && s_Player.m_VideoIdx != 0xFFFFFFFF  ) {

     _init_queues ( 0 );

     if (  !PlayerControl_FastForward ( &s_Player )  ) {

      s_Flags |= SMS_FLAGS_STOP;
      break;

     }  /* end if */

    } else if (  lButtons == PAD_LEFT && ( s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE ) && s_Player.m_VideoIdx != 0xFFFFFFFF  ) {

     _init_queues ( 0 );

     if (  !PlayerControl_Rewind ( &s_Player )  ) {

      s_Flags |= SMS_FLAGS_STOP;
      break;

     }  /* end if */

    }  /* end if */

   }  /* end if */
skip:
   g_CDDASpeed = s_nPackets < 128 ? 4 : 3;

   lpPacket = s_Player.m_pCont -> NewPacket ( s_Player.m_pCont );
nextPacket:
   lSize = s_Player.m_pCont -> ReadPacket ( lpPacket );

   if ( lSize < 0 ) {

    lpPacket -> Destroy ( lpPacket );
    break;

   } else if ( lSize == 0 ) continue;

   if ( lpPacket -> m_StmIdx == s_Player.m_VideoIdx ) {

    WaitSema ( SEMA_D_PUT_VIDEO );

    LOCK( s_SemaPVLock );
     *SMS_RB_PUSHSLOT( s_VPacketQueue ) = lpPacket;
     SMS_RB_PUSHADVANCE( s_VPacketQueue );
    UNLOCK( s_SemaPVLock );

    ++s_nPackets;

    WakeupThread ( THREAD_ID_VD );

   } else if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

    WaitSema ( SEMA_D_PUT_AUDIO );

    LOCK( s_SemaPALock );
     *SMS_RB_PUSHSLOT( s_APacketQueue ) = lpPacket;
     SMS_RB_PUSHADVANCE( s_APacketQueue );
    UNLOCK( s_SemaPALock );

    ++s_nPackets;

    WakeupThread ( THREAD_ID_AD );

   } else goto nextPacket;

  }  /* end while */

 }  /* end if */

}  /* end _sms_play */

static void _Destroy ( void ) {

 DiskType lType;

 if ( s_VideoBuffer ) {

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
  free ( s_VideoBuffer );
#ifdef LOCK_QUEUES
  DeleteSema ( s_SemaPVLock );
  DeleteSema ( s_SemaVLock  );
#endif  /* LOCK_QUEUES */
 }  /* end if */

 if ( s_AudioBuffer ) {

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
 }  /* end if */

 if ( s_Player.m_pSPUCtx ) {

  s_Player.m_pSPUCtx -> Destroy ();
  s_Player.m_pSPUCtx = NULL;

 }  /* end if */

 if ( s_Player.m_pIPUCtx ) {

  s_Player.m_pIPUCtx -> Sync    ();
  s_Player.m_pIPUCtx -> Destroy ();
  s_Player.m_pIPUCtx = NULL;

 }  /* end if */

 lType = CDDA_DiskType ();

 if (  lType == DiskType_CD  ||
       lType == DiskType_DVD ||
       lType == DiskType_CDDA
 ) {

  CDDA_Synchronize ();
  CDDA_Stop        ();
  CDDA_Synchronize ();

 }  /* end if */

 s_Player.m_pCont -> Destroy ( s_Player.m_pCont );
 s_Player.m_pCont = NULL;

}  /* end _Destroy */

SMS_Player* SMS_InitPlayer ( FileContext* apFileCtx, GUIContext* apGUICtx ) {

 s_VPacketBuffer = NULL;
 s_APacketBuffer = NULL;
 s_VideoBuffer   = NULL;
 s_AudioBuffer   = NULL;
 s_Flags         =    0;

 apGUICtx -> Status ( "Detecting file format..." );

 s_Player.m_pCont = SMS_GetContainer ( apFileCtx, apGUICtx );
 s_MainThreadID   = GetThreadId ();

 if ( s_Player.m_pCont != NULL ) {

  int         i;
  ee_thread_t lThread;
  ee_thread_t lCurrentThread;

  s_Player.m_VideoIdx = 0xFFFFFFFF;
  s_Player.m_AudioIdx = 0xFFFFFFFF;

  for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i )

   if ( s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeVideo && s_Player.m_VideoIdx == 0xFFFFFFFF ) {

    SMS_CodecOpen ( &s_Player.m_pCont -> m_pStm[ i ] -> m_Codec );

    if ( s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

     s_Player.m_pVideoCodec = s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_pCodec;
     s_Player.m_pVideoCodec -> Init ( &s_Player.m_pCont -> m_pStm[ i ] -> m_Codec );

     s_Player.m_VideoIdx = i;

    }  /* end if */

   } else if ( s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeAudio && s_Player.m_AudioIdx == 0xFFFFFFFF ) {

    SMS_CodecOpen ( &s_Player.m_pCont -> m_pStm[ i ] -> m_Codec );

    if ( s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

     s_Player.m_pAudioCodec = s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_pCodec;

     if ( s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_ID == SMS_CodecID_AC3 ) s_Player.m_pCont -> m_pStm[ i ] -> m_Codec.m_Channels = 2;

     s_Player.m_pAudioCodec -> Init ( &s_Player.m_pCont -> m_pStm[ i ] -> m_Codec );

     s_Player.m_AudioIdx = i;

    }  /* end if */

   }  /* end if */

  s_Player.m_pGUICtx  = apGUICtx;
  s_Player.m_pFileCtx = apFileCtx;
  s_Player.Destroy    = _Destroy;

  ReferThreadStatus ( s_MainThreadID, &lCurrentThread );

  s_Player.Play = _sms_play;

  lThread.stack_size       = sizeof ( s_VideoRStack );
  lThread.stack            = s_VideoRStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = _sms_video_renderer;
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
  lThread.func             = _sms_audio_renderer;
  THREAD_ID_AR = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_AudioDStack );
  lThread.stack            = s_AudioDStack;
  lThread.initial_priority = lCurrentThread.current_priority;
  lThread.gp_reg           = &_gp;
  lThread.func             = _sms_audio_decoder;
  THREAD_ID_AD = CreateThread ( &lThread );

  _init_queues ( 1 );

  ExitThreadFunc = ExitDeleteThread;

 } else apFileCtx -> Destroy ( apFileCtx );

 return s_Player.m_pCont ? &s_Player : NULL;

}  /* end SMS_AVIInitPlayer */
