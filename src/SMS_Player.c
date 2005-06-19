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
#include "SMS.h"
#include "SMS_AVI.h"

#include <kernel.h>
#include <stdio.h>

#define SMS_PACKET_QUEUE_SIZE 4
#define SMS_FRAME_QUEUE_SIZE  4

extern void* _gp;

static SMS_AVIPlayer s_Player;

static SMS_AVIPacket* s_Packets     [ SMS_PACKET_QUEUE_SIZE << 1 ];
static SMS_AVIPacket* s_AudioPackets[ SMS_PACKET_QUEUE_SIZE      ];
static SMS_AVIPacket* s_VideoPackets[ SMS_PACKET_QUEUE_SIZE      ];
static SMS_Frame*     s_VideoFrames [ SMS_FRAME_QUEUE_SIZE       ];

static SMS_RB_CREATE( s_PacketBuffer,     SMS_AVIPacket* );
static SMS_RB_CREATE( s_AudioBuffer,      SMS_AVIPacket* );
static SMS_RB_CREATE( s_VideoBuffer,      SMS_AVIPacket* );
static SMS_RB_CREATE( s_VideoFrameBuffer, SMS_Frame*     );

static int s_MainThreadID;
static int s_ActiveThreadCount;

static unsigned int s_VideoIndex;
static unsigned int s_AudioIndex;

static int s_SemaGetPacket;

static int           s_SemaPutVideo;
static int           s_ThreadIDProcessVideo;
static unsigned char s_StackProcessVideo[ 128 * 1024 ] __attribute__(   (  aligned( 16 )  )   );

static int           s_SemaPutAudio;
static int           s_ThreadIDProcessAudio;
static unsigned char s_StackProcessAudio[ 128 * 1024 ] __attribute__(   (  aligned( 16 )  )   );

static SMS_Codec* s_pVideoCodec;
static SMS_Codec* s_pAudioCodec;

static int s_nPackets;
static int s_nAudioPackets;
static int s_nVideoPackets;

static void _process_video_thread ( void ) {

 SMS_AVIPacket* lpPacket;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VideoBuffer )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_VideoBuffer );
  SMS_RB_POPADVANCE( s_VideoBuffer );
/// Process video start
  ++s_nVideoPackets;
/// Process video end
  *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_PacketBuffer );
  SignalSema ( s_SemaPutVideo  );
  SignalSema ( s_SemaGetPacket );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _process_video_thread */

static void _process_audio_thread ( void ) {

 SMS_AVIPacket* lpPacket;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioBuffer )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_AudioBuffer );
  SMS_RB_POPADVANCE( s_AudioBuffer );
/// Process audio start
  ++s_nAudioPackets;
/// Process audio end
  *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_PacketBuffer );
  SignalSema ( s_SemaPutAudio  );
  SignalSema ( s_SemaGetPacket );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _process_audio_thread */

static int _SMS_PlayerInit ( void ) {

 int         i;
 int         lQSize;
 ee_sema_t   lSema;
 ee_thread_t lThread;

 s_nPackets      =
 s_nAudioPackets =
 s_nVideoPackets = 0;

 lSema.init_count =
 lSema.max_count  = SMS_PACKET_QUEUE_SIZE << 1;
 s_SemaGetPacket  = CreateSema ( &lSema );

 s_AudioIndex = s_VideoIndex = 0xFFFFFFFF;

 for ( i = 0; i < s_Player.m_pCtx -> m_nStm; ++i )

  switch ( s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec.m_Type ) {

   case SMS_CodecTypeVideo: {

    s_VideoIndex = i;

    SMS_CodecOpen ( &s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec );

    if ( s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

     s_pVideoCodec = s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
     s_pVideoCodec -> Init ( &s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec );

    } else s_VideoIndex = 0xFFFFFFFF;

   } break;

   case SMS_CodecTypeAudio: {

    s_AudioIndex = i;

    SMS_CodecOpen ( &s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec );

    if ( s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

     s_pAudioCodec = s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
     s_pAudioCodec -> Init ( &s_Player.m_pCtx -> m_pStm[ i ] -> m_Codec );

    } else s_AudioIndex = 0xFFFFFFFF;

   } break;

   default: break;

  }  /* end switch */

 if ( s_VideoIndex != 0xFFFFFFFF ) {

  lThread.stack_size       = 1024 * 128;
  lThread.gp_reg           = &_gp;
  lThread.func             = _process_video_thread;
  lThread.stack            = s_StackProcessVideo;
  lThread.initial_priority = 3;
  s_ThreadIDProcessVideo   = CreateThread ( &lThread );

  lQSize = SMS_PACKET_QUEUE_SIZE;

  if ( s_AudioIndex == 0xFFFFFFFF ) lQSize += lQSize;

  SMS_RB_INIT( s_VideoBuffer,      s_VideoPackets, lQSize               );
  SMS_RB_INIT( s_VideoFrameBuffer, s_VideoFrames,  SMS_FRAME_QUEUE_SIZE );

  lSema.init_count =
  lSema.max_count  = lQSize;
  s_SemaPutVideo   = CreateSema ( &lSema );

 }  /* end if */

 if ( s_AudioIndex != 0xFFFFFFFF ) {

  lThread.stack_size       = 1024 * 128;
  lThread.gp_reg           = &_gp;
  lThread.func             = _process_audio_thread;
  lThread.stack            = s_StackProcessAudio;
  lThread.initial_priority = 2;
  s_ThreadIDProcessAudio   = CreateThread ( &lThread );

  lQSize = SMS_PACKET_QUEUE_SIZE;

  if ( s_VideoIndex == 0xFFFFFFFF ) lQSize += lQSize;

  SMS_RB_INIT( s_AudioBuffer, s_AudioPackets, lQSize );

  lSema.init_count =
  lSema.max_count  = lQSize;
  s_SemaPutAudio   = CreateSema ( &lSema );

 }  /* end if */

 if ( s_AudioIndex != 0xFFFFFFFF || s_VideoIndex != 0xFFFFFFFF ) {

  SMS_RB_INIT( s_PacketBuffer, s_Packets, SMS_PACKET_QUEUE_SIZE << 1 );

  do {

   *SMS_RB_PUSHSLOT( s_PacketBuffer ) = SMS_AVINewPacket ( s_Player.m_pCtx );
   SMS_RB_PUSHADVANCE( s_PacketBuffer );

  } while (  !SMS_RB_FULL( s_PacketBuffer )  );

  lQSize = 1;

 } else lQSize = 0;

 return lQSize;

}  /* end _SMS_PlayerInit */

static void _SMS_PlayerDestroy ( void ) {

 DeleteSema ( s_SemaGetPacket );

 if ( s_VideoIndex != 0xFFFFFFFF ) DeleteSema ( s_SemaPutVideo );
 if ( s_AudioIndex != 0xFFFFFFFF ) DeleteSema ( s_SemaPutAudio );

 while (  !SMS_RB_EMPTY( s_PacketBuffer )  ) {

  (  *SMS_RB_POPSLOT( s_PacketBuffer )  ) -> Destroy (  *SMS_RB_POPSLOT( s_PacketBuffer )  );
  SMS_RB_POPADVANCE( s_PacketBuffer );

 }  /* end while */

 while (  !SMS_RB_EMPTY( s_AudioBuffer )  ) {

  (  *SMS_RB_POPSLOT( s_AudioBuffer )  ) -> Destroy (  *SMS_RB_POPSLOT( s_AudioBuffer )  );
  SMS_RB_POPADVANCE( s_AudioBuffer );

 }  /* end while */

 while (  !SMS_RB_EMPTY( s_VideoBuffer )  ) {

  (  *SMS_RB_POPSLOT( s_VideoBuffer )  ) -> Destroy (  *SMS_RB_POPSLOT( s_VideoBuffer )  );
  SMS_RB_POPADVANCE( s_VideoBuffer );

 }  /* end while */

 printf ( "Total packets: %d\n", s_nPackets      );
 printf ( "Video packets: %d\n", s_nVideoPackets );
 printf ( "Audio packets: %d\n", s_nAudioPackets );

}  /* end _SMS_PlayerDestroy */

static void _SMS_PlayerPlay ( void ) {

 int            lSize;
 SMS_AVIPacket* lpPacket;

 s_MainThreadID      = GetThreadId ();
 s_ActiveThreadCount = 1;

 if ( s_AudioIndex != 0xFFFFFFFF ) {

  ++s_ActiveThreadCount;
  StartThread ( s_ThreadIDProcessAudio, NULL );

 }  /* end if */

 if ( s_VideoIndex != 0xFFFFFFFF ) {

  ++s_ActiveThreadCount;
  StartThread ( s_ThreadIDProcessVideo, NULL );

 }  /* end if */

 while ( 1 ) {

  WaitSema ( s_SemaGetPacket );

  lpPacket = *SMS_RB_POPSLOT( s_PacketBuffer );
  SMS_RB_POPADVANCE( s_PacketBuffer );
  lSize    = SMS_AVIReadPacket ( lpPacket );

  if ( lSize < 0 ) break;

  if ( lpPacket -> m_StmIdx == s_VideoIndex ) {

   WaitSema ( s_SemaPutVideo );

   *SMS_RB_PUSHSLOT( s_VideoBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_VideoBuffer );
   WakeupThread ( s_ThreadIDProcessVideo );

  } else {

   WaitSema ( s_SemaPutAudio );

   *SMS_RB_PUSHSLOT( s_AudioBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_AudioBuffer );
   WakeupThread ( s_ThreadIDProcessAudio );

  }  /* end else */

  if ( ++s_nPackets == 128 ) break;

 }  /* end while */

 WakeupThread ( s_ThreadIDProcessAudio );
 WakeupThread ( s_ThreadIDProcessVideo );

 while ( --s_ActiveThreadCount ) SleepThread ();

}  /* end _SMS_PlayerPlay */

SMS_AVIPlayer* SMS_AVICreatePlayer ( SMS_AVIContext* apCtx ) {

 s_Player.m_pCtx     = apCtx;
 s_Player.Initialize = _SMS_PlayerInit;
 s_Player.Destroy    = _SMS_PlayerDestroy;
 s_Player.Play       = _SMS_PlayerPlay;

 return &s_Player;

}  /* end SMS_CreatePlayer */
