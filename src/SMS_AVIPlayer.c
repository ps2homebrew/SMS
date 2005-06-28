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
#include "IPU.h"
#include "SPU.h"
#include "FileContext.h"
#include "SMS_AudioBuffer.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>

#define SMS_PACKET_QUEUE_LENGTH 5

static SMS_AVIPlayer s_Player;

static SMS_AVIPacket** s_Packets;
static SMS_AVIPacket** s_VideoPackets;
static SMS_AVIPacket** s_AudioPackets;

static SMS_RB_CREATE( s_PacketBuffer, SMS_AVIPacket* );
static SMS_RB_CREATE( s_VideoBuffer,  SMS_AVIPacket* );
static SMS_RB_CREATE( s_AudioBuffer,  SMS_AVIPacket* );

extern void* _gp;

static int           s_SemaPutVideo;
static int           s_SemaPutAudio;
static int           s_SemaGetPacket;
static int           s_MainThreadID;
static int           s_VideoThreadID;
static int           s_AudioThreadID;
static unsigned char s_VideoStack[ 0x20000 ] __attribute__(   (  aligned( 16 )  )   );
static unsigned char s_AudioStack[ 0x20000 ] __attribute__(   (  aligned( 16 )  )   );
static IPUContext*   s_pIPUCtx;
static SPUContext*   s_pSPUCtx;
static SMS_Codec*    s_pVideoCodec;
static int           s_VideoIdx;
static SMS_Codec*    s_pAudioCodec;
static int           s_AudioIdx;

static void _video_decode_thread ( SMS_AVIContext* apCtx  ) {

 SMS_Frame*     lpFrame;
 SMS_AVIPacket* lpPacket;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_VideoBuffer )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_VideoBuffer );
  SMS_RB_POPADVANCE( s_VideoBuffer );

  if (  s_pVideoCodec -> Decode (
         &apCtx -> m_pStm[ s_VideoIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
        )
  ) {

   s_pIPUCtx -> Display ( lpFrame -> m_pData, &lpFrame -> m_Locked );

  }  /* end if */

  *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_PacketBuffer );

  SignalSema ( s_SemaGetPacket );
  SignalSema ( s_SemaPutVideo  );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _video_decode_thread */

static void _audio_decode_thread ( SMS_AVIContext* apCtx  ) {

 static SMS_AudioBuffer s_Buffer;

 int              lSize;
 SMS_AudioBuffer* lpBuffer = &s_Buffer;
 SMS_AVIPacket*   lpPacket;

 while ( 1 ) {

  SleepThread ();

  if (  SMS_RB_EMPTY( s_AudioBuffer )  ) break;

  lpPacket = *SMS_RB_POPSLOT( s_AudioBuffer );
  SMS_RB_POPADVANCE( s_AudioBuffer );

  lpBuffer -> m_Len = 0;

  do {

   if (   (  lSize = s_pAudioCodec -> Decode (
              &apCtx -> m_pStm[ s_AudioIdx ] -> m_Codec, ( void** )&lpBuffer, lpPacket -> m_pData, lpPacket -> m_Size
             )
          )
   ) {

    s_pSPUCtx -> PlayPCM ( lpBuffer -> m_pOut );
    lpBuffer -> Release ();

   } else break;

  } while ( lpBuffer -> m_Len > 0 );

  *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_PacketBuffer );
  SignalSema ( s_SemaGetPacket );
  SignalSema ( s_SemaPutAudio  );

 }  /* end while */

 WakeupThread ( s_MainThreadID );
 ExitDeleteThread ();

}  /* end _audio_decode_thread */

static void _sms_avi_destroy ( void ) {

 if ( s_VideoPackets ) {

  WakeupThread ( s_VideoThreadID );  
  SleepThread ();
  DeleteSema ( s_SemaPutVideo );
  free ( s_VideoPackets );

 }  /* end if */

 if ( s_AudioPackets ) {

  WakeupThread ( s_AudioThreadID );
  SleepThread ();
  DeleteSema ( s_SemaPutAudio );
  free ( s_AudioPackets );

 }  /* end if */

 if ( s_Packets ) {

  DeleteSema ( s_SemaGetPacket );

  while (  !SMS_RB_EMPTY( s_PacketBuffer )  ) {

   (  *SMS_RB_POPSLOT( s_PacketBuffer )  ) -> Destroy (  *SMS_RB_POPSLOT( s_PacketBuffer )  );
   SMS_RB_POPADVANCE( s_PacketBuffer );

  }  /* end while */

  free ( s_Packets );

 }  /* end if */

 s_pSPUCtx -> Destroy ();
 s_pSPUCtx = NULL;

 if ( s_pIPUCtx ) {

  s_pIPUCtx -> Sync    ();
  s_pIPUCtx -> Destroy ();
  s_pIPUCtx = NULL;

 }  /* end if */

 s_Player.m_pAVICtx -> Destroy ( s_Player.m_pAVICtx );
 s_Player.m_pAVICtx = NULL;

}  /* end _sms_avi_destroy */

static void _sms_avi_play_a_v ( void ) {

 int            lSize;
 SMS_AVIPacket* lpPacket;
 uint64_t       lTime;

 s_pSPUCtx = SPU_InitContext (
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_Channels,
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_SampleRate
 );

 s_Player.m_pGSCtx -> m_fDblBuf = GS_OFF;
 s_Player.m_pGSCtx -> m_fZBuf   = GS_OFF;

 s_Player.m_pGSCtx -> InitScreen ();
 s_Player.m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0xFF, 0x00 )  );

 s_Player.m_pGSCtx -> DrawText ( 10, 60, 0, "Buffering AVI file (this can take a while)..." );

 s_pIPUCtx = IPU_InitContext (
  s_Player.m_pGSCtx,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Width,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Height
 );

 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 2048 );
 s_Player.m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 lTime = SMS_Time ();

 s_pSPUCtx -> Mute ( 0 );

 while ( 1 ) {

  WaitSema ( s_SemaGetPacket );

  lpPacket = *SMS_RB_POPSLOT( s_PacketBuffer );
  SMS_RB_POPADVANCE( s_PacketBuffer );
nextPacket:
  lSize = SMS_AVIReadPacket ( lpPacket );

  if ( lSize < 0 ) {

   *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_PacketBuffer );

   break;

  }  // end if

  if ( lpPacket -> m_StmIdx == s_VideoIdx ) {

   WaitSema ( s_SemaPutVideo );

   *SMS_RB_PUSHSLOT( s_VideoBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_VideoBuffer );

   WakeupThread ( s_VideoThreadID );

  } else if ( lpPacket -> m_StmIdx == s_AudioIdx ) {

   WaitSema ( s_SemaPutAudio );

   *SMS_RB_PUSHSLOT( s_AudioBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_AudioBuffer );

   WakeupThread ( s_AudioThreadID );

  } else goto nextPacket;

 }  /* end while */

}  /* end _sms_avi_play_a_v */

static void _sms_avi_play_a ( void ) {

 int            lSize;
 SMS_AVIPacket* lpPacket;

 s_pSPUCtx = SPU_InitContext (
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_Channels,
  s_Player.m_pAVICtx -> m_pStm[ s_AudioIdx ] -> m_Codec.m_SampleRate
 );

 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 1024 );

 while ( 1 ) {

  WaitSema ( s_SemaGetPacket );

  lpPacket = *SMS_RB_POPSLOT( s_PacketBuffer );
  SMS_RB_POPADVANCE( s_PacketBuffer );
nextPacket:
  lSize = SMS_AVIReadPacket ( lpPacket );

  if ( lSize < 0 ) {

   *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_PacketBuffer );

   break;

  } else if ( lpPacket -> m_StmIdx != s_AudioIdx ) goto nextPacket;

  WaitSema ( s_SemaPutAudio );

  *SMS_RB_PUSHSLOT( s_AudioBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_AudioBuffer );

  WakeupThread ( s_AudioThreadID );

 }  /* end while */

}  /* end _sms_avi_play_a */

static void _sms_avi_play_v ( void ) {

 int            lSize;
 SMS_AVIPacket* lpPacket;
 
 s_Player.m_pGSCtx -> m_fDblBuf = GS_OFF;
 s_Player.m_pGSCtx -> m_fZBuf   = GS_OFF;

 s_Player.m_pGSCtx -> InitScreen ();
 s_Player.m_pGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0xFF, 0x00 )  );

 s_pIPUCtx = IPU_InitContext (
  s_Player.m_pGSCtx,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Width,
  s_Player.m_pAVICtx -> m_pStm[ s_VideoIdx ] -> m_Codec.m_Height
 );

 s_Player.m_pFileCtx -> Stream ( s_Player.m_pFileCtx, s_Player.m_pFileCtx -> m_CurPos, 1024 );

 while ( 1 ) {

  WaitSema ( s_SemaGetPacket );

  lpPacket = *SMS_RB_POPSLOT( s_PacketBuffer );
  SMS_RB_POPADVANCE( s_PacketBuffer );
nextPacket:
  lSize = SMS_AVIReadPacket ( lpPacket );

  if ( lSize < 0 ) {

   *SMS_RB_PUSHSLOT( s_PacketBuffer ) = lpPacket;
   SMS_RB_PUSHADVANCE( s_PacketBuffer );

   break;

  } else if ( lpPacket -> m_StmIdx != s_VideoIdx ) goto nextPacket;

  WaitSema ( s_SemaPutVideo );

  *SMS_RB_PUSHSLOT( s_VideoBuffer ) = lpPacket;
  SMS_RB_PUSHADVANCE( s_VideoBuffer );

  WakeupThread ( s_VideoThreadID );

 }  /* end while */

}  /* end _sms_avi_play_v */

SMS_AVIPlayer* SMS_AVIInitPlayer ( FileContext* apFileCtx, GSContext* apGSCtx ) {

 int lPktQLen    = 0;
 int lVidPktQLen = 0;
 int lAudPktQLen = 0;

 s_Packets      =
 s_VideoPackets =
 s_AudioPackets = NULL;

 apGSCtx -> DrawText ( 10, 102, 0, "Probing AVI file..." );

 s_Player.m_pAVICtx = SMS_AVINewContext ( apFileCtx );
 s_MainThreadID     = GetThreadId ();

 if ( s_Player.m_pAVICtx != NULL ) {

  if (  SMS_AVIProbeFile  ( s_Player.m_pAVICtx ) &&
        SMS_AVIReadHeader ( s_Player.m_pAVICtx )
  ) {

   int         i;
   ee_sema_t   lSema;
   ee_thread_t lThread;

   s_VideoIdx = 0xFFFFFFFF;
   s_AudioIdx = 0xFFFFFFFF;

   SMS_AVICalcFrameRate ( s_Player.m_pAVICtx );
   SMS_AVIPrintInfo     ( s_Player.m_pAVICtx );

   for ( i = 0; i < s_Player.m_pAVICtx -> m_nStm; ++i )

    if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

     s_VideoIdx = i;

     SMS_CodecOpen ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

      s_pVideoCodec = s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      s_pVideoCodec -> Init ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     }  /* end if */

    } else if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeAudio ) {

     s_AudioIdx = i;

     SMS_CodecOpen ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

      s_pAudioCodec = s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      s_pAudioCodec -> Init ( &s_Player.m_pAVICtx -> m_pStm[ i ] -> m_Codec );

     }  /* end if */

    }  /* end if */

   s_Player.m_pGSCtx   = apGSCtx;
   s_Player.m_pFileCtx = apFileCtx;
   s_Player.Destroy    = _sms_avi_destroy;

   if ( s_AudioIdx != 0xFFFFFFFF && s_VideoIdx != 0xFFFFFFFF ) {

    lVidPktQLen = SMS_PACKET_QUEUE_LENGTH;
    lAudPktQLen = lVidPktQLen;

    s_Player.Play = _sms_avi_play_a_v;

   } else if ( s_VideoIdx != 0xFFFFFFFF ) {

    lVidPktQLen   = SMS_PACKET_QUEUE_LENGTH;
    s_Player.Play = _sms_avi_play_v;

   } else if ( s_AudioIdx != 0xFFFFFFFF ) {

    lAudPktQLen   = SMS_PACKET_QUEUE_LENGTH;
    s_Player.Play = _sms_avi_play_a;

   } else s_Player.Play = NULL;

   lPktQLen = lVidPktQLen + lAudPktQLen;

   if ( lVidPktQLen != 0 ) {

    s_VideoPackets = ( SMS_AVIPacket** )calloc (  lVidPktQLen, sizeof ( SMS_AVIPacket* )  );

    SMS_RB_INIT( s_VideoBuffer, s_VideoPackets, lVidPktQLen );

    lSema.init_count =
    lSema.max_count  = lVidPktQLen;
    s_SemaPutVideo   = CreateSema ( &lSema );

    lThread.stack_size       = 0x20000;
    lThread.gp_reg           = &_gp;
    lThread.func             = _video_decode_thread;
    lThread.stack            = s_VideoStack;
    lThread.initial_priority = 2;
    StartThread (  s_VideoThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

   }  /* end if */

   if ( lAudPktQLen ) {

    s_AudioPackets = ( SMS_AVIPacket** )calloc (  lAudPktQLen, sizeof ( SMS_AVIPacket* )  );

    SMS_RB_INIT( s_AudioBuffer, s_AudioPackets, lAudPktQLen );

    lSema.init_count =
    lSema.max_count  = lAudPktQLen;
    s_SemaPutAudio   = CreateSema ( &lSema );

    lThread.stack_size       = 0x20000;
    lThread.gp_reg           = &_gp;
    lThread.func             = _audio_decode_thread;
    lThread.stack            = s_AudioStack;
    lThread.initial_priority = 2;
    StartThread (  s_AudioThreadID = CreateThread ( &lThread ), s_Player.m_pAVICtx  );

   }  /* end if */

   if ( lPktQLen ) {

    s_Packets = ( SMS_AVIPacket** )calloc (  lPktQLen, sizeof ( SMS_AVIPacket* )  );

    SMS_RB_INIT( s_PacketBuffer, s_Packets, lPktQLen );

    do {

     *SMS_RB_PUSHSLOT( s_PacketBuffer ) = SMS_AVINewPacket ( s_Player.m_pAVICtx );
     SMS_RB_PUSHADVANCE( s_PacketBuffer );

    } while (  !SMS_RB_FULL( s_PacketBuffer )  );

    lSema.init_count =
    lSema.max_count  = lPktQLen;
    s_SemaGetPacket  = CreateSema ( &lSema );

   }  /* end if */

  } else {

   s_Player.m_pAVICtx -> Destroy ( s_Player.m_pAVICtx );
   s_Player.m_pAVICtx = NULL;

  }  /* end else */

 }  /* end if */

 return s_Player.m_pAVICtx ? &s_Player : NULL;

}  /* end SMS_AVIInitPlayer */
