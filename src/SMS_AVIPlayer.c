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
#include "SMS_AVI.h"
#include "GS.h"
#include "GUI.h"
#include "IPU.h"
#include "SPU.h"
#include "FileContext.h"
#include "SMS_AudioBuffer.h"
#include "SMS_VideoBuffer.h"
#include "Timer.h"
#include "CDDA.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <libpad.h>

#define SMS_VPACKET_QSIZE    384
#define SMS_APACKET_QSIZE    384
#define SMS_VIDEO_QUEUE_SIZE  10
#define SMS_AUDIO_QUEUE_SIZE  10

#define SMS_FLAGS_STOP  0x00000001
#define SMS_FLAGS_PAUSE 0x00000002

static SMS_AVIPlayer s_Player;

static SMS_AVIPacket**   s_VPacketBuffer;
static SMS_AVIPacket**   s_APacketBuffer;
static SMS_FrameBuffer** s_VideoBuffer;
static uint8_t**         s_AudioBuffer;

static SMS_RB_CREATE( s_VPacketQueue, SMS_AVIPacket*   );
static SMS_RB_CREATE( s_APacketQueue, SMS_AVIPacket*   );
static SMS_RB_CREATE( s_VideoQueue,   SMS_FrameBuffer* );
static SMS_RB_CREATE( s_AudioQueue,   uint8_t*         );

extern void* _gp;

static int              s_SemaRPutVideo;
static int              s_SemaDPutVideo;
static int              s_SemaRPutAudio;
static int              s_SemaDPutAudio;
static int              s_SemaPauseAudio;
static int              s_SemaPauseVideo;
static int              s_MainThreadID;
static int              s_VideoRThreadID;
static int              s_VideoDThreadID;
static int              s_AudioRThreadID;
static int              s_AudioDThreadID;
static uint8_t          s_VideoRStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_VideoDStack[ 0x20000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_AudioRStack[ 0x10000 ] __attribute__(   (  aligned( 16 )  )   );
static uint8_t          s_AudioDStack[ 0x20000 ] __attribute__(   (  aligned( 16 )  )   );
static IPUContext*      s_pIPUCtx;
static SMS_Codec*       s_pVideoCodec;
static int              s_VideoIdx;
static SPUContext*      s_pSPUCtx;
static SMS_Codec*       s_pAudioCodec;
static int              s_AudioIdx;
static SMS_AudioBuffer* s_AudioSamples;
static float            s_AudioTime;
static int              s_nPackets;
static int              s_Flags;

static void _sms_play_v ( void ) {

 int              lSize;
 SMS_FrameBuffer* lpFrame;
 SMS_AVIPacket*   lpPacket = SMS_AVINewPacket ( s_Player.m_pAVICtx );

 s_Player.m_pGUICtx -> Status ( "Buffering AVI file (video only)..." );
 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 384 );

 s_Player.m_pGUICtx -> m_pGSCtx -> m_fDblBuf = GS_OFF;
 s_Player.m_pGUICtx -> m_pGSCtx -> m_fZBuf   = GS_OFF;

 s_Player.m_pGUICtx -> m_pGSCtx -> InitScreen ();
 s_Player.m_pGUICtx -> m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 s_pIPUCtx = IPU_InitContext (
  s_Player.m_pGUICtx -> m_pGSCtx,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Width,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Height
 );

 while ( 1 ) {

  int lButtons = GUI_ReadButtons ();

  if ( lButtons & PAD_SELECT )

   GUI_WaitButton ( PAD_START );

  else if ( lButtons & PAD_TRIANGLE ) break;

  lSize = SMS_AVIReadPacket ( lpPacket );

  if ( lSize < 0 ) break;

  if ( lpPacket -> m_StmIdx != s_VideoIdx ) continue;

  if (  s_pVideoCodec -> Decode (
         &s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
        )
  ) {

   s_pIPUCtx -> Sync ();
   s_pIPUCtx -> Display ( lpFrame );

  }  /* end if */

 }  /* end while */

 lpPacket -> Destroy ( lpPacket );

}  /* end _sms_play_v */

static void _sms_play_a ( void ) {

 static SMS_AudioBuffer s_DummyBuffer;

 int            lSize;
 SMS_AVIPacket* lpPacket;

 lpPacket = SMS_AVINewPacket ( s_Player.m_pAVICtx );

 s_AudioSamples = &s_DummyBuffer;

 s_Player.m_pGUICtx -> Status ( "Buffering AVI file (audio only)..." );
 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 1024 );

 s_Player.m_pGUICtx -> m_pGSCtx -> InitScreen ();
 s_Player.m_pGUICtx -> m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 s_Player.m_pGUICtx -> m_pGSCtx -> DrawText ( 10, 60, 0, "Audio only", 0 );

 s_pSPUCtx = SPU_InitContext (
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_Channels,
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_SampleRate
 );

 if ( s_pSPUCtx )

  while ( 1 ) {

   int lButtons = GUI_ReadButtons ();

   if ( lButtons & PAD_SELECT )

    GUI_WaitButton ( PAD_START );

   else if ( lButtons & PAD_TRIANGLE ) break;

   lSize = SMS_AVIReadPacket ( lpPacket );

   if ( lSize < 0 ) break;

   if ( lpPacket -> m_StmIdx != s_AudioIdx ) continue;

   s_AudioSamples -> m_Len = 0;

   do {

    if (  s_pAudioCodec -> Decode (
           &s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec, ( void** )&s_AudioSamples, lpPacket -> m_pData, lpPacket -> m_Size
          )
    ) {

     s_pSPUCtx -> PlayPCM ( s_AudioSamples -> m_pOut );

     s_AudioSamples -> Release ();

    } else break;

   } while ( s_AudioSamples -> m_Len > 0 );

  }  /* end while */

  lpPacket -> Destroy ( lpPacket );

}  /* end _sms_play_a */

static void _sms_video_renderer ( void* apParam ) {

 SMS_FrameBuffer* lpFrame;
 float            lFrameRate = ( float )s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_RealFrameRate / ( float )s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_RealFrameRateBase;
 float            lDiff;
 int              lnVideoFrames = 0;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VideoQueue )  ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) WaitSema ( s_SemaPauseVideo );

  lpFrame = *SMS_RB_POPSLOT( s_VideoQueue );
  SMS_RB_POPADVANCE( s_VideoQueue );

  if ( s_Flags & SMS_FLAGS_STOP ) {

   SignalSema ( s_SemaRPutVideo );
   break;

  }  /* end if */

  s_pIPUCtx -> Sync ();

  lDiff = ( ++lnVideoFrames * 1000 / lFrameRate ) - s_AudioTime;

  if ( lDiff > 20.0F ) Timer_Wait ( lDiff / 4.0F );

  s_pIPUCtx -> Display ( lpFrame );

  SignalSema ( s_SemaRPutVideo );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _sms_video_renderer */

static void _sms_video_decoder ( void* apParam ) {

 SMS_AVIPacket*   lpPacket;
 SMS_FrameBuffer* lpFrame;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VPacketQueue )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_VPacketQueue );
  SMS_RB_POPADVANCE( s_VPacketQueue );
  --s_nPackets;

  if (  s_pVideoCodec -> Decode (
         &s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
        )
  ) {

   if ( s_Flags & SMS_FLAGS_STOP ) {

    lpPacket -> Destroy ( lpPacket );
    break;

   }  /* end if */

   WaitSema ( s_SemaRPutVideo );

   *SMS_RB_PUSHSLOT( s_VideoQueue ) = lpFrame;
   SMS_RB_PUSHADVANCE( s_VideoQueue );

   WakeupThread ( s_VideoRThreadID );

  }  /* end if */

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( s_SemaDPutVideo );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _sms_video_decoder */

static void _sms_audio_renderer ( void* apParam ) {

 uint8_t* lpSamples;
 uint32_t lnChannels = s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_Channels;
 uint32_t lBPS       = s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_BitsPerSample;
 uint32_t lSPS       = s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_SampleRate;
 uint32_t lPlayed    = 0;

 if ( !lBPS ) lBPS = 16;

 s_AudioTime = 0.0F;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioQueue )  ) {

   s_pSPUCtx -> Mute ( 1 );
   break;

  }  /* end if */

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_pSPUCtx -> Mute ( 1 );
   WaitSema ( s_SemaPauseAudio );
   s_pSPUCtx -> Mute ( 0 );

  }  /* end if */

  lpSamples = *SMS_RB_POPSLOT( s_AudioQueue );
  SMS_RB_POPADVANCE( s_AudioQueue );

  SignalSema ( s_SemaRPutAudio );

  if ( s_Flags & SMS_FLAGS_STOP ) {

   s_AudioSamples -> Release ();
   break;

  }  /* end if */

  lPlayed    += *( int* )lpSamples;
  s_AudioTime = (  lPlayed * (  1000.0F / ( lBPS * lnChannels / 8 )  )   ) / ( float )lSPS;

  s_pSPUCtx -> PlayPCM ( lpSamples );

  s_AudioSamples -> Release ();

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _sms_audio_renderer */

static void _sms_audio_decoder ( void* apParam ) {

 static SMS_AudioBuffer s_DummyBuffer;

 SMS_AVIPacket* lpPacket;

 s_AudioSamples = &s_DummyBuffer;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_APacketQueue )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_APacketQueue );
  SMS_RB_POPADVANCE( s_APacketQueue );
  --s_nPackets;

  s_AudioSamples -> m_Len = 0;

  do {

   if (  s_pAudioCodec -> Decode (
          &s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec, ( void** )&s_AudioSamples, lpPacket -> m_pData, lpPacket -> m_Size
         )
   ) {

    if ( s_Flags & SMS_FLAGS_STOP ) {

     lpPacket -> Destroy ( lpPacket );
     goto end;

    }  /* end if */

    WaitSema ( s_SemaRPutAudio );

    *SMS_RB_PUSHSLOT( s_AudioQueue ) = s_AudioSamples -> m_pOut;
    SMS_RB_PUSHADVANCE( s_AudioQueue );

    WakeupThread ( s_AudioRThreadID );

   } else break;

  } while ( s_AudioSamples -> m_Len > 0 );

  lpPacket -> Destroy ( lpPacket );

  SignalSema ( s_SemaDPutAudio );

 }  /* end while */
end:
 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _sms_audio_decoder */

static void _sms_play_a_v ( void ) {

 int            lSize;
 SMS_AVIPacket* lpPacket;

 s_nPackets = 0;

 s_Player.m_pGUICtx -> Status ( "Buffering AVI file..." );
 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 1024 );

 s_pSPUCtx = SPU_InitContext (
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_Channels,
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_SampleRate
 );

 s_Player.m_pGUICtx -> m_pGSCtx -> m_fDblBuf = GS_OFF;
 s_Player.m_pGUICtx -> m_pGSCtx -> m_fZBuf   = GS_OFF;

 s_Player.m_pGUICtx -> m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );
 s_Player.m_pGUICtx -> m_pGSCtx -> VSync ();
 s_Player.m_pGUICtx -> m_pGSCtx -> InitScreen ();
 s_Player.m_pGUICtx -> m_pGSCtx -> VSync ();
 s_Player.m_pGUICtx -> m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 s_pIPUCtx = IPU_InitContext (
  s_Player.m_pGUICtx -> m_pGSCtx,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Width,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Height
 );

 if ( s_pSPUCtx && s_pIPUCtx )

  while ( 1 ) {

   int lButtons = GUI_ReadButtons ();

   if ( lButtons ) {

    if ( lButtons & PAD_SELECT ) {

     s_Flags |=  SMS_FLAGS_PAUSE;
     GUI_WaitButton ( PAD_START );
     s_Flags &= ~SMS_FLAGS_PAUSE;
     SignalSema ( s_SemaPauseAudio );
     SignalSema ( s_SemaPauseVideo );

    } else if (  lButtons & PAD_TRIANGLE && *( int* )&s_AudioTime  ) {

     int            i;
     SMS_AVIPacket* lpPacket;

     s_Flags |= SMS_FLAGS_STOP;

     for ( i = 0; i < 4; ++i ) SleepThread ();

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

     break;

    }  /* end if */

   }  /* end if */

   g_CDDASpeed = s_nPackets < 128 ? 4 : 3;

   lpPacket = SMS_AVINewPacket ( s_Player.m_pAVICtx );
nextPacket:
   lSize = SMS_AVIReadPacket ( lpPacket );

   if ( lSize < 0 ) {

    lpPacket -> Destroy ( lpPacket );
    break;

   }  /* end if */

   if ( lpPacket -> m_StmIdx == s_VideoIdx ) {

    WaitSema ( s_SemaDPutVideo );

    *SMS_RB_PUSHSLOT( s_VPacketQueue ) = lpPacket;
    SMS_RB_PUSHADVANCE( s_VPacketQueue );
    ++s_nPackets;

    WakeupThread ( s_VideoDThreadID );

   } else if ( lpPacket -> m_StmIdx == s_AudioIdx ) {

    WaitSema ( s_SemaDPutAudio );

    *SMS_RB_PUSHSLOT( s_APacketQueue ) = lpPacket;
    SMS_RB_PUSHADVANCE( s_APacketQueue );
    ++s_nPackets;

    WakeupThread ( s_AudioDThreadID );

   } else goto nextPacket;

  }  /* end while */

 else if ( s_pIPUCtx )

  _sms_play_v ();

 else if ( s_pSPUCtx ) _sms_play_a ();

}  /* end _sms_play_a_v */

static void _sms_avi_destroy ( void ) {

 if ( s_VideoBuffer ) {

  if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

   WakeupThread ( s_VideoDThreadID );
   SleepThread ();

  }  /* end if */

  DeleteSema ( s_SemaDPutVideo );
  free ( s_VPacketBuffer );

  if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

   WakeupThread ( s_VideoRThreadID );  
   SleepThread ();

  }  /* end if */

  DeleteSema ( s_SemaRPutVideo  );
  DeleteSema ( s_SemaPauseVideo );
  free ( s_VideoBuffer );

 }  /* end if */

 if ( s_AudioBuffer ) {

  if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

   WakeupThread ( s_AudioDThreadID );
   SleepThread ();

  }  /* end if */

  DeleteSema ( s_SemaDPutAudio );
  free ( s_APacketBuffer );

  if (  !( s_Flags & SMS_FLAGS_STOP )  ) {

   WakeupThread ( s_AudioRThreadID );
   SleepThread ();

  }  /* end if */

  DeleteSema ( s_SemaRPutAudio );
  free ( s_AudioBuffer );

  DeleteSema ( s_SemaPauseAudio );

 }  /* end if */

 s_pSPUCtx -> Destroy ();
 s_pSPUCtx = NULL;

 if ( s_pIPUCtx ) {

  s_pIPUCtx -> Sync    ();
  s_pIPUCtx -> Destroy ();
  s_pIPUCtx = NULL;

 }  /* end if */

 CDDA_Synchronize ();
 CDDA_Stop        ();
 CDDA_Synchronize ();

 s_Player.m_pAVICtx -> Destroy ( s_Player.m_pAVICtx );
 s_Player.m_pAVICtx = NULL;

}  /* end _sms_avi_destroy */

SMS_AVIPlayer* SMS_AVIInitPlayer ( FileContext* apFileCtx, GUIContext* apGUICtx ) {

 s_VPacketBuffer = NULL;
 s_APacketBuffer = NULL;
 s_VideoBuffer   = NULL;
 s_AudioBuffer   = NULL;
 s_Flags         =    0;

 apGUICtx -> Status ( "Checking file format..." );

 s_Player.m_pAVICtx = SMS_AVINewContext ( apFileCtx );
 s_MainThreadID     = GetThreadId ();

 if ( s_Player.m_pAVICtx != NULL ) {

  if (  SMS_AVIProbeFile  ( s_Player.m_pAVICtx ) &&
        SMS_AVIReadHeader ( s_Player.m_pAVICtx )
  ) {

   int i;

   s_VideoIdx = 0xFFFFFFFF;
   s_AudioIdx = 0xFFFFFFFF;

   SMS_AVICalcFrameRate ( s_Player.m_pAVICtx );
   SMS_AVIPrintInfo     ( s_Player.m_pAVICtx );

   for ( i = 0; i < s_Player.m_pAVICtx -> m_nStm; ++i )

    if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

     SMS_CodecOpen ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

      s_pVideoCodec = s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      s_pVideoCodec -> Init ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

      s_VideoIdx = i;

     }  /* end if */

    } else if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeAudio ) {

     SMS_CodecOpen ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

      s_pAudioCodec = s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      s_pAudioCodec -> Init ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

      s_AudioIdx = i;

     }  /* end if */

    }  /* end if */

   s_Player.m_pGUICtx  = apGUICtx;
   s_Player.m_pFileCtx = apFileCtx;
   s_Player.Destroy    = _sms_avi_destroy;

   if ( s_AudioIdx != 0xFFFFFFFF && s_VideoIdx != 0xFFFFFFFF ) {

    ee_sema_t   lSema;
    ee_thread_t lThread;
    ee_thread_t lCurrentThread;

    ReferThreadStatus ( s_MainThreadID, &lCurrentThread );

    s_Player.Play = _sms_play_a_v;

    lThread.stack_size       = sizeof ( s_VideoRStack );
    lThread.stack            = s_VideoRStack;
    lThread.initial_priority = lCurrentThread.current_priority + 1;
    lThread.gp_reg           = &_gp;
    lThread.func             = _sms_video_renderer;
    StartThread (  s_VideoRThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

    lThread.stack_size       = sizeof ( s_VideoDStack );
    lThread.stack            = s_VideoDStack;
    lThread.initial_priority = lCurrentThread.current_priority;
    lThread.gp_reg           = &_gp;
    lThread.func             = _sms_video_decoder;
    StartThread (  s_VideoDThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

    lThread.stack_size       = sizeof ( s_AudioRStack );
    lThread.stack            = s_AudioRStack;
    lThread.initial_priority = lCurrentThread.current_priority;
    lThread.gp_reg           = &_gp;
    lThread.func             = _sms_audio_renderer;
    StartThread (  s_AudioRThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

    lThread.stack_size       = sizeof ( s_AudioDStack );
    lThread.stack            = s_AudioDStack;
    lThread.initial_priority = lCurrentThread.current_priority;
    lThread.gp_reg           = &_gp;
    lThread.func             = _sms_audio_decoder;
    StartThread (  s_AudioDThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

    s_VPacketBuffer = ( SMS_AVIPacket**   )calloc (  SMS_VPACKET_QSIZE,    sizeof ( SMS_AVIPacket*   )  );
    s_APacketBuffer = ( SMS_AVIPacket**   )calloc (  SMS_APACKET_QSIZE,    sizeof ( SMS_AVIPacket*   )  );
    s_VideoBuffer   = ( SMS_FrameBuffer** )calloc (  SMS_VIDEO_QUEUE_SIZE, sizeof ( SMS_FrameBuffer* )  );
    s_AudioBuffer   = ( uint8_t**         )calloc (  SMS_AUDIO_QUEUE_SIZE, sizeof ( uint8_t*         )  );

    SMS_RB_INIT( s_VPacketQueue, s_VPacketBuffer, SMS_VPACKET_QSIZE    );
    SMS_RB_INIT( s_APacketQueue, s_APacketBuffer, SMS_APACKET_QSIZE    );
    SMS_RB_INIT( s_VideoQueue,   s_VideoBuffer,   SMS_VIDEO_QUEUE_SIZE );
    SMS_RB_INIT( s_AudioQueue,   s_AudioBuffer,   SMS_AUDIO_QUEUE_SIZE );

    lSema.init_count =
    lSema.max_count  = SMS_VPACKET_QSIZE - 1;
    s_SemaDPutVideo = CreateSema ( &lSema );

    lSema.init_count =
    lSema.max_count  = SMS_APACKET_QSIZE - 1;
    s_SemaDPutAudio = CreateSema ( &lSema );

    lSema.init_count = 
    lSema.max_count  = SMS_VIDEO_QUEUE_SIZE - 1;
    s_SemaRPutVideo = CreateSema ( &lSema );

    lSema.init_count =
    lSema.max_count  = SMS_AUDIO_QUEUE_SIZE - 1;
    s_SemaRPutAudio = CreateSema ( &lSema );

    lSema.init_count = 0;
    lSema.max_count  = 1;
    s_SemaPauseAudio = CreateSema ( &lSema );
    s_SemaPauseVideo = CreateSema ( &lSema );

   } else if ( s_VideoIdx != 0xFFFFFFFF ) {

    s_Player.Play = _sms_play_v;

   } else if ( s_AudioIdx != 0xFFFFFFFF ) {

    s_Player.Play = _sms_play_a;

   } else s_Player.Play = NULL;

  } else {

   s_Player.m_pAVICtx -> Destroy ( s_Player.m_pAVICtx );
   s_Player.m_pAVICtx = NULL;

  }  /* end else */

 } else apFileCtx -> Destroy ( apFileCtx );

 return s_Player.m_pAVICtx ? &s_Player : NULL;

}  /* end SMS_AVIInitPlayer */
