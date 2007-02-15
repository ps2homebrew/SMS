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
#include "SMS_Sounds.h"
#include "SMS_Data.h"
#include "SMS_Spectrum.h"
#include "SMS_RC.h"
#include "SMS_RingBuffer.h"

#include <kernel.h>
#include <stdio.h>
#include <limits.h>
#include <libhdd.h>
#include <string.h>
#include "libmpeg_internal.h"

#define SMS_VP_BUFFER_SIZE ( 1024 * 1024 * 3 )
#define SMS_AP_BUFFER_SIZE ( 1024 *  512     )

#define SMS_FLAGS_STOP      0x00000001
#define SMS_FLAGS_PAUSE     0x00000002
#define SMS_FLAGS_MENU      0x00000004
#define SMS_FLAGS_EXIT      0x00000008
#define SMS_FLAGS_VSCROLL   0x00000010
#define SMS_FLAGS_ASCROLL   0x00000020
#define SMS_FLAGS_AASCROLL  0x00000040
#define SMS_FLAGS_ABSCROLL  0x00000080
#define SMS_FLAGS_SPDIF     0x00000100
#define SMS_FLAGS_USER_STOP 0x00000200

#define THREAD_ID_VR s_ThreadIDs[ 0 ]
#define THREAD_ID_VD s_ThreadIDs[ 1 ]
#define THREAD_ID_AR s_ThreadIDs[ 2 ]
#define THREAD_ID_AD s_ThreadIDs[ 3 ]

SMS_Player s_Player;

static SMS_RingBuffer* s_pVideoBuffer;
static SMS_RingBuffer* s_pAudioBuffer;

extern void* _gp;

static int s_ThreadIDs[ 4 ];
static int s_SemaPauseAudio;
static int s_SemaPauseVideo;
static int s_SemaAckPause;
static int s_MainThreadID;
static int s_Flags;

static int  ( *FFwdFunc ) ( void );
static int  ( *RewFunc  ) ( void );

extern void SMS_PlayerMenu ( void );

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
 int lfNoCSC = 0;

 if ( s_Player.m_VideoIdx >= 0 ) {

  SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;

  lWidth  = lpCodecCtx -> m_Width;
  lHeight = lpCodecCtx -> m_Height;

  if ( lpCodecCtx -> m_Flags & SMS_CODEC_FLAG_NOCSC ) lfNoCSC = 1;

 }  /* end if */

 GUI_Redraw ( GUIRedrawMethod_RedrawClear );
 GS_VSync ();
 GSContext_Init ( g_Config.m_DisplayMode, GSZTest_On, GSDoubleBuffer_Off );
 GS_VSync ();

 _set_colors ();

 s_Player.m_pIPUCtx = IPU_InitContext (
  lWidth, lHeight, s_Player.m_AudioIdx >= 0 ? &s_Player.m_AudioTime : NULL, lfNoCSC
 );

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Prepare ();

}  /* end _prepare_ipu_context */

static void _clear_packet_queues ( void ) {

 int i;

 for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

  SMS_RingBuffer* lpBuff = s_Player.m_pCont -> m_pStm[ i ] -> m_pPktBuf;

  if ( lpBuff ) SMS_RingBufferReset ( lpBuff );

 }  /* end for */

}  /* end _clear_packet_queues */

static void _terminate_threads ( int afDelete ) {

 int         i;
 ee_thread_t lThread;

 ChangeThreadPriority ( s_MainThreadID, SMS_THREAD_PRIORITY - 3 );
 s_Player.m_pIPUCtx -> StopSync ( 1 );
 s_Player.m_pIPUCtx -> Sync ();
 s_Player.m_pIPUCtx -> StopSync ( 0 );
 TerminateThread ( THREAD_ID_VR );
 TerminateThread ( THREAD_ID_VD );
 TerminateThread ( THREAD_ID_AD );
 ReferThreadStatus ( THREAD_ID_AR, &lThread );
 ChangeThreadPriority ( s_MainThreadID, SMS_THREAD_PRIORITY );

 if ( lThread.status != 0x10 ) {
  s_Flags |=  SMS_FLAGS_STOP;
  if ( s_Flags & SMS_FLAGS_PAUSE ) SignalSema ( s_SemaPauseAudio );
  SMS_RingBufferPost ( s_pAudioBuffer );
  SleepThread ();
  s_Flags &= ~SMS_FLAGS_STOP;
 }  /* end if */

 if ( !afDelete ) {

  for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

   SMS_RingBuffer* lpRB = s_Player.m_pCont -> m_pStm[ i ] -> m_pPktBuf;

   if ( lpRB ) SMS_RingBufferReset ( lpRB );

  }  /* end for */

  SMS_RingBufferReset ( s_pVideoBuffer );
  SMS_RingBufferReset ( s_pAudioBuffer );

 } else for ( i = 0; i < 4; ++i ) DeleteThread ( s_ThreadIDs[ i ] );

}  /* end _terminate_threads */

static unsigned char s_VideoBuffer[ 320 ] __attribute__(   (  aligned( 64 )  )   );

static void _init_queues ( int afCreate ) {

 ee_sema_t lSema;

 if ( afCreate ) {

  s_pVideoBuffer  = SMS_RingBufferInit (  s_VideoBuffer, sizeof ( s_VideoBuffer )  );
  s_pAudioBuffer  = SMS_RingBufferInit (  UNCACHED_SEG( SMS_AUDIO_BUFFER ), SMS_AUDIO_BUFFER_SIZE  );

  lSema.init_count = 0;
  s_SemaPauseAudio = CreateSema ( &lSema );
  s_SemaPauseVideo = CreateSema ( &lSema );
  s_SemaAckPause   = CreateSema ( &lSema );

 } else {

  _terminate_threads ( 0 );
  s_Flags = 0;
  _clear_packet_queues ();

 }  /* end else */

}  /* end _init_queues */

static void _start_threads ( void ) {

 int i;

 for ( i = 0; i < 4; ++i ) StartThread ( s_ThreadIDs[ i ], s_Player.m_pCont );

}  /* end _start_threads */

static void _block_callback ( SMS_RingBuffer* apRB ) {

 int i;

 *( int* )apRB -> m_pBlockCBParam = 1;

 for ( i = 0; i < 4; ++i ) ResumeThread ( s_ThreadIDs[ i ] );

}  /* end _block_callback */

static int _fill_packet_queues ( void ) {

 int          i, lStmIdx, lSize = 0, lLock = 0;
 SMS_Stream** lpStms = s_Player.m_pCont -> m_pStm;

 for ( i = 0; i < 4; ++i ) SuspendThread ( s_ThreadIDs[ i ] );

 for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

  SMS_RingBuffer* lpRB = lpStms[ i ] -> m_pPktBuf;

  if ( lpRB ) {
   lpRB -> BlockCB         = _block_callback;
   lpRB -> m_pBlockCBParam = &lLock;
  }  /* end if */

 }  /* end for */

 while ( !lLock ) {

  if (   (  lSize = s_Player.m_pCont -> ReadPacket ( s_Player.m_pCont, &lStmIdx )  ) <= 0   ) break;

  SMS_RingBufferPost ( lpStms[ lStmIdx ] -> m_pPktBuf );

 }  /* end while */

 for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

  SMS_RingBuffer* lpRB = lpStms[ i ] -> m_pPktBuf;

  if ( lpRB ) {
   lpRB -> BlockCB         = NULL;
   lpRB -> m_pBlockCBParam = NULL;
  }  /* end if */

 }  /* end for */

 if ( !lLock ) for ( i = 0; i < 4; ++i ) ResumeThread ( s_ThreadIDs[ i ] );

 return lSize;

}  /* end _fill_packet_queues */

static void _sms_dummy_video_renderer ( void* apParam ) {

 static u64 s_lDMA[ 16 ] __attribute__(   (  aligned( 16 )  )   );

 u64*   lpDMA     = _U( s_lDMA );
 short* lpSamples = NULL;
 int    i         = 0;

 SMS_RingBufferWait ( s_pVideoBuffer );

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

 if ( g_Config.m_PlayerFlags & SMS_PF_ASD ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 7 ], 1, DMATAG_ID_REF, 0, ( u32 )( s_Player.m_OSDPackets[ 7 ] ), 0   );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 0 ] - 1, 1, DMATAG_ID_REFE, 0, ( u32 )( s_Player.m_OSDPackets[ 0 ] + 2 ), 0   );
 lpDMA[ i   ] = 0;

 while ( 1 ) {

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

  if ( s_Player.m_pAudioSamples && lpSamples == s_Player.m_pAudioSamples )

   lpSamples += 1024;

  else lpSamples = s_Player.m_pAudioSamples;

  SMS_SpectrumUpdate ( lpSamples );

  __asm__ __volatile__( "sync\n\t" );

  s_Player.m_pIPUCtx -> Display ( s_lDMA, 0 );

 }  /* end while */

}  /* end _sms_dummy_video_renderer */

static void _sms_video_renderer ( void* apParam ) {

 SMS_FrameBuffer* lpFrame;

 while ( 1 ) {

  lpFrame = *( SMS_FrameBuffer** )SMS_RingBufferWait ( s_pVideoBuffer );

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   if (  !( s_Flags & SMS_FLAGS_MENU )  )
    _draw_text ( STR_PAUSE.m_pStr );
   else s_Player.m_pIPUCtx -> Sync ();

   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  s_Player.m_pIPUCtx -> Sync ();

  if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_PF_SUBS )  ) s_Player.m_pSubCtx -> Display ( lpFrame -> m_SPTS - s_Player.m_SVDelta );

  if ( s_Player.m_OSD ) {

   s_Player.m_pIPUCtx -> PQueuePacket ( s_Player.m_OSDQWC[ 1 ], s_Player.m_OSDPackets[ 1 ] );
   s_Player.m_pIPUCtx -> PQueuePacket ( s_Player.m_OSDQWC[ 0 ], s_Player.m_OSDPackets[ 0 ] );

  }  /* end if */

  s_Player.m_pIPUCtx -> Display ( lpFrame, s_Player.m_VideoTime = lpFrame -> m_PTS );

  SMS_RingBufferFree ( s_pVideoBuffer, 4 );

 }  /* end while */

}  /* end _sms_video_renderer */

static void _sms_video_decoder ( void* apParam ) {

 SMS_CodecContext* lpCtx;
 SMS_RingBuffer*   lpBuff;

 if ( s_Player.m_VideoIdx < 0 ) return;

 lpCtx  = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;
 lpBuff = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pPktBuf;

 if ( !lpBuff ) return;

 while ( 1 ) {
  s_Player.m_pVideoCodec -> Decode ( lpCtx, s_pVideoBuffer, lpBuff );
  RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );
 }  /* end while */

}  /* end _sms_video_decoder */

static void _sms_audio_only_renderer ( void* apParam ) {

 SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec;
 uint32_t          lBPS       = lpCodecCtx -> m_BitsPerSample;
 float             lMult;

 if ( !lBPS ) lBPS = 16;

 lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

 s_Player.m_AudioTime = 0LL;

 while ( 1 ) {

  int64_t  lPTS;
  int      lLen;
  uint8_t* lpPacket = SMS_RingBufferWait ( s_pAudioBuffer );

  if ( s_Flags & SMS_FLAGS_STOP ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );
   s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

  }  /* end if */

  lLen = *( int* )( lpPacket + 64 );
  lPTS = *( int64_t* )lpPacket;

  if ( lPTS == SMS_STPTS_VALUE ) {

   PlayerControl_UpdateDuration ( 1, s_Player.m_pCont -> m_Duration );

   s_Player.m_AudioTime = 0LL;

   if ( lpCodecCtx -> m_Channels   != s_Player.m_AudioChannels   ||
        lpCodecCtx -> m_SampleRate != s_Player.m_AudioSampleRate
   ) {

    s_Player.m_AudioChannels   = lpCodecCtx -> m_Channels;
    s_Player.m_AudioSampleRate = lpCodecCtx -> m_SampleRate;

    lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

    s_Player.m_pSPUCtx -> Destroy ();
    s_Player.m_pSPUCtx = SPU_InitContext (
     s_Player.m_AudioChannels, s_Player.m_AudioSampleRate, SPU_Index2Volume ( g_Config.m_PlayerVolume )
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

      if ( s_Player.m_PlayItemNr > 1 ) {

       s_Player.m_pPlayItem = s_Player.m_pPlayItem -> m_pPrev;
       --s_Player.m_PlayItemNr;

      }  /* end if */

      s_Flags &= ~SMS_FLAGS_ABSCROLL;

     }  /* end else */

     PlayerControl_UpdateItemNr ();

     if (  !( s_Flags & SMS_FLAGS_ASCROLL )  ) {

      s_Player.m_OSDPLInc = -4;
      s_Player.m_OSDPLRes = s_Player.m_OSDPLPos - 32;
      s_Flags            |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL );

     } else s_Flags &= ~SMS_FLAGS_ASCROLL;

    }  /* end else */

   }  /* end if */

  } else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult );

  s_Player.m_pAudioSamples = ( short* )( lpPacket + 68 );

  s_Player.m_pSPUCtx -> PlayPCM ( lpPacket + 64 );

  SMS_RingBufferFree ( s_pAudioBuffer, lLen + 68 );

 }  /* end while */

 s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );

}  /* end _sms_audio_only_renderer */

static void _audio_callback ( SMS_RingBuffer* apRB ) {

 *( int64_t* )apRB -> m_pPtr         = *( int64_t* )apRB -> m_pUserCBParam;
 *( int64_t* )apRB -> m_pUserCBParam = SMS_NOPTS_VALUE;

 SMS_RingBufferPost ( apRB );

}  /* end _audio_callback */

static void _sms_audio_only_decoder ( void* apParam ) {

 SMS_AVPacket*     lpPacket;
 int64_t           lPTS       = SMS_STPTS_VALUE;
 SMS_Stream*       lpStm      = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ];
 SMS_RingBuffer*   lpBuff     = lpStm -> m_pPktBuf;
 SMS_CodecContext* lpCodecCtx = lpStm -> m_pCodec;
 int               lSize;

 s_pAudioBuffer -> m_pUserCBParam = &lPTS;
 s_pAudioBuffer -> UserCB         = _audio_callback;

 while ( 1 ) {

  lpPacket = ( SMS_AVPacket* )SMS_RingBufferWait ( lpBuff );

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

   if ( lPTS != SMS_STPTS_VALUE ) lPTS = lpPacket -> m_PTS;

   do {
    lSize = s_Player.m_pAudioCodec -> Decode ( lpCodecCtx, s_pAudioBuffer, lpBuff );
   } while ( lSize );

  }  /* end if */

  SMS_RingBufferFree ( lpBuff, lpPacket -> m_Size + 64 );
   
 }  /* end while */

}  /* end _sms_audio_only_decoder */

static void _sms_audio_renderer ( void* apParam ) {

 uint32_t lBPS     = 0;
 int64_t  lEndTime = 0;
 float    lMult    = 0.0F;

 if ( s_Player.m_AudioIdx >= 0 ) {

  lBPS     = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec -> m_BitsPerSample;
  lEndTime = s_Player.m_pCont -> m_Duration - 200LL;

  if ( !lBPS ) lBPS = 16;

  lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

 }  /* end if */

 while ( 1 ) {

  int64_t        lPTS;
  int            lLen;
  unsigned char* lpPacket = SMS_RingBufferWait ( s_pAudioBuffer );

  if ( s_Flags & SMS_FLAGS_STOP ) break;

  if ( s_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );

   if ( s_Flags & SMS_FLAGS_STOP ) break;

   s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

  }  /* end if */

  lLen = *( int* )( lpPacket + 64 );
  lPTS = *( int64_t* )lpPacket;

  if ( lPTS != SMS_NOPTS_VALUE )
   s_Player.m_AudioTime = lPTS;
  else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult );

  s_Player.m_AudioTime += s_Player.m_AVDelta;
  s_Player.m_pSPUCtx -> PlayPCM ( lpPacket + 64 );

  if ( s_Player.m_AudioTime >= lEndTime ) {

   s_Player.m_pIPUCtx -> StopSync ( 1 );
   s_Player.m_pSPUCtx -> SetVolume ( 0 );
   lEndTime = 0x7FFFFFFFFFFFFFFFL;

  }  /* end if */

  SMS_RingBufferFree ( s_pAudioBuffer, lLen + 68 );

 }  /* end while */

 if ( s_Player.m_pSPUCtx ) s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );

}  /* end _sms_audio_renderer */

static void _sms_audio_decoder ( void* apParam ) {

 SMS_AVPacket*   lpPacket;
 int64_t         lPTS;
 int             lSize;
 SMS_RingBuffer* lpBuff;

 s_pAudioBuffer -> m_pUserCBParam = &lPTS;
 s_pAudioBuffer -> UserCB         = _audio_callback;

 if (  s_Player.m_AudioIdx < 0 || !( lpBuff = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pPktBuf )  ) return;

 while ( 1 ) {

  lpPacket = ( SMS_AVPacket* )SMS_RingBufferWait ( lpBuff );

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

   lPTS = lpPacket -> m_PTS;

   do {
    lSize = s_Player.m_pAudioCodec -> Decode (
     s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec, s_pAudioBuffer, lpBuff
    );
   } while ( lSize );

  }  /* end if */

  if ( lPTS != SMS_NOPTS_VALUE ) s_Player.m_AudioTime = lPTS;

  SMS_RingBufferFree ( lpBuff, lpPacket -> m_Size + 64 );

 }  /* end while */

}  /* end _sms_audio_decoder */

static void _sms_play_alarm ( int anID, unsigned short aTime, void* apArg ) {

 iSignalSema (  *( int* )apArg  );

}  /* end _sms_play_alarm */

static int _FFwd_AV ( void ) {

 int retVal;

 _init_queues ( 0 );
 retVal = PlayerControl_FastForward ();
 if ( s_Player.m_AudioIdx >= 0 ) s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec );
 _start_threads ();

 return retVal;

}  /* end _FFwd_AV */

static int _Rew_AV ( void ) {

 int retVal;

 _init_queues ( 0 );
 retVal = PlayerControl_Rewind ();
 if ( s_Player.m_AudioIdx >= 0 ) s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec );
 _start_threads ();

 return retVal;

}  /* end _Rew_AV */

static int _FFwd_A ( void ) {

 int            retVal = 1;
 SMS_Container* lpCont = s_Player.m_pCont;

 if (   s_Player.m_pPlayItem && s_Player.m_pPlayItem -> m_pNext && !(  s_Flags & ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL )  )   ) {

  _init_queues ( 0 );
  s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec );
  _start_threads ();

  if ( s_Player.m_pPlayItem -> m_pNext ) {

   s_Player.m_pAudioSamples = NULL;
   s_Player.m_OSDPLInc      = -4;
   s_Player.m_OSDPLRes      = s_Player.m_OSDPLPos - 32;

   s_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL );
   SMS_RingBufferPost ( s_pVideoBuffer );

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
  s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec );
  _start_threads ();

  if ( s_Player.m_pPlayItem -> m_pPrev ) {

   s_Player.m_pAudioSamples = NULL;
   s_Player.m_OSDPLInc      = 4;
   s_Player.m_OSDPLRes      = s_Player.m_OSDPLPos + 32;

   s_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_ABSCROLL );
   SMS_RingBufferPost ( s_pVideoBuffer );

   retVal = lpCont -> Seek (  lpCont, 0, 0, ( uint32_t )s_Player.m_pPlayItem -> m_pPrev  );

   WaitSema ( s_SemaAckPause );

  }  /* end if */

  SMS_RingBufferPost ( s_pVideoBuffer );

 }  /* end if */

 return retVal;

}  /* end _Rew_A */

static void _sms_play ( void ) {

 int           lSize = 0;
 char          lBuff[ 128 ];
 ee_sema_t     lSemaParam;
 int           lSema;
 int           lPktIdx;
 SMS_Stream**  lpStms     = s_Player.m_pCont -> m_pStm;
 int           lfNoVideo  = s_Player.m_VideoIdx < 0;
 uint32_t      lnDec      = 1L;
 uint64_t      lNextTime  = 0;
 uint64_t      lPOffTime  = g_Timer + g_Config.m_PowerOff;
 int           lfVolume   = !( s_Player.m_Flags & SMS_FLAGS_SPDIF );
 int           lfSeekable = s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE;

 lSemaParam.init_count = 0;
 lSema = CreateSema ( &lSemaParam );

 if ( !lfNoVideo ) {

  if ( s_Player.m_pSubFileCtx ) {

   float lFPS = ( float )lpStms[ s_Player.m_VideoIdx ] -> m_RealFrameRate / 
                ( float )lpStms[ s_Player.m_VideoIdx ] -> m_RealFrameRateBase;

   GUI_Progress ( STR_LOADING_SUBTITLES.m_pStr, 100, 0 );
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

 if ( s_Player.m_AudioIdx >= 0 ) {

  s_Player.m_pSPUCtx = SPU_InitContext (
   s_Player.m_AudioChannels, s_Player.m_AudioSampleRate,
   SPU_Index2Volume ( g_Config.m_PlayerVolume )
  );

  ++lnDec;

 }  /* end if */
 
 if ( s_Player.m_pFileCtx ) {

  sprintf ( lBuff, STR_BUFFERING_FILE.m_pStr, s_Player.m_pCont -> m_pName  );

  GUI_Status ( lBuff );
  s_Player.m_pFileCtx -> Stream (
   s_Player.m_pFileCtx, s_Player.m_StartPos = s_Player.m_pFileCtx -> m_CurPos, s_Player.m_pFileCtx -> m_StreamSize >> 3
  );

 }  /* end if */

 _prepare_ipu_context ();

 PlayerControl_Init ();

 if ( lfNoVideo ) {

  s_Player.m_pIPUCtx -> Suspend ();
  s_Player.m_OSDPackets[ 5 ] = SMS_PlayerBallSim_Init ( &s_Player.m_OSDQWC[ 5 ] );
  s_Player.m_pIPUCtx -> Resume  ();

  SMS_SpectrumInit ();

  PlayerControl_HandleOSD ( 0, 0 );
  PlayerControl_UpdateDuration ( 0, s_Player.m_pCont -> m_Duration );
  PlayerControl_UpdateItemNr ();
  SMS_RingBufferPost ( s_pVideoBuffer );

 } else {

  s_Player.m_OSDPackets[ 5 ] = NULL;
  lSize = _fill_packet_queues ();

 }  /* end else */
repeat:
 while ( 1 ) {

  uint32_t lBtn = GUI_ReadButtons ();

  if ( lBtn ) {

   lPOffTime = g_Timer + g_Config.m_PowerOff;

   if ( g_Timer <= lNextTime ) goto skip;

   lNextTime = g_Timer + 300;

   if (  ( lBtn == SMS_PAD_SELECT || lBtn == RC_PAUSE ) && *( int* )&s_Player.m_AudioTime  ) {

    if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Inactive || lfNoVideo || !lfSeekable ) {

     s_Flags |= SMS_FLAGS_PAUSE;

     s_Player.m_pIPUCtx -> StopSync ( 1 );

     if (  SMS_RingBufferCount ( s_pAudioBuffer )              ) WaitSema ( s_SemaAckPause );
     if (  SMS_RingBufferCount ( s_pVideoBuffer ) || lfNoVideo ) WaitSema ( s_SemaAckPause );

     s_Player.m_pIPUCtx -> StopSync ( 0 );

     while (  GUI_ReadButtons ()  );

     while ( 1 ) {

      static unsigned s_ResBtn[] __attribute__(   (  section( ".data" )  )   ) = {
       SMS_PAD_START, SMS_PAD_SELECT, RC_PAUSE, RC_PLAY
      };

      lBtn = GUI_WaitButtons ( 4, s_ResBtn, 200 );

      if ( lBtn == SMS_PAD_START || lBtn == RC_PLAY ) {

       lPOffTime = g_Timer + g_Config.m_PowerOff;
       break;

      }  /* end if */

      if ( lBtn == SMS_PAD_SELECT || lBtn == RC_PAUSE ) {

       s_Player.m_pIPUCtx -> Sync    ();
       s_Player.m_pIPUCtx -> Suspend ();
       s_Player.m_pIPUCtx -> Repaint ();
       s_Player.m_pIPUCtx -> Resume  ();

      }  /* end if */

     }  /* end while */
resume:
     s_Flags &= ~SMS_FLAGS_PAUSE;
     SignalSema ( s_SemaPauseAudio );
     SignalSema ( s_SemaPauseVideo );
     lSize = 0;

    } else if (  *( int* )&s_Player.m_AudioTime ) {

     int lSts;

     s_Flags |= SMS_FLAGS_PAUSE | SMS_FLAGS_MENU;

     s_Player.m_pIPUCtx -> StopSync ( 1 );

     if (  SMS_RingBufferCount ( s_pAudioBuffer )  ) WaitSema ( s_SemaAckPause );
     if (  SMS_RingBufferCount ( s_pVideoBuffer )  ) WaitSema ( s_SemaAckPause );

     s_Player.m_pIPUCtx -> StopSync ( 0 );

     while (  GUI_ReadButtons ()  );

     if (   !(  lSts = PlayerControl_ScrollBar ( _init_queues )  )   ) {
      s_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
      goto exit;
     }  /* end if */

     if ( lSts < 0 ) {

      if ( s_Player.m_AudioIdx >= 0 ) s_Player.m_pAudioCodec -> Init ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec );

      _start_threads ();

      lSize = _fill_packet_queues ();

     } else goto resume;

    }  /* end else */

   } else if (   ( lBtn == SMS_PAD_START || lBtn == RC_MENU ) && !lfNoVideo && *( int* )&s_Player.m_AudioTime  ) {

    SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;

    s_Flags |= SMS_FLAGS_PAUSE | SMS_FLAGS_MENU;

    s_Player.m_pIPUCtx -> StopSync ( 1 );

    if (  SMS_RingBufferCount ( s_pAudioBuffer )  ) WaitSema ( s_SemaAckPause );
    if (  SMS_RingBufferCount ( s_pVideoBuffer )  ) WaitSema ( s_SemaAckPause );

    s_Player.m_pIPUCtx -> StopSync ( 0 );

    s_Player.m_pIPUCtx -> Suspend ();

    if ( lpCodecCtx -> m_Flags & SMS_CODEC_FLAG_IPU ) {
     SuspendThread ( THREAD_ID_VD );
     _MPEG_Suspend ();
     IPU_FRST    ();
    }  /* end if */

    SMS_PlayerMenu ();

    if ( lpCodecCtx -> m_Flags & SMS_CODEC_FLAG_IPU ) {
     ResumeThread ( THREAD_ID_VD );
     IPU_FRST     ();
     _MPEG_Resume ();
    }  /* end if */

    s_Player.m_pIPUCtx -> Resume  ();
    s_Flags &= ~SMS_FLAGS_MENU;

    lPOffTime = g_Timer + g_Config.m_PowerOff;

    goto resume;

   } else if ( lBtn == SMS_PAD_TRIANGLE ||
               lBtn == RC_RESET         ||
               lBtn == RC_RETURN        ||
               lBtn == RC_STOP
          ) {
exit:
    s_Flags |= ( SMS_FLAGS_EXIT | SMS_FLAGS_USER_STOP );
    break;

   } else if (  ( lBtn == SMS_PAD_UP || lBtn == RC_TOPX ) && lfVolume && s_Player.m_pSPUCtx ) {

    PlayerControl_AdjustVolume ( 1 );

   } else if (  ( lBtn == SMS_PAD_DOWN || lBtn == RC_BOTTOMX ) && lfVolume && s_Player.m_pSPUCtx ) {

    PlayerControl_AdjustVolume ( -1 );

   } else if (  ( lBtn == SMS_PAD_RIGHT || lBtn == RC_SCAN_RIGHT || lBtn == RC_RIGHTX ) && lfSeekable  ) {

    if (  !FFwdFunc ()  ) {

     s_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
     goto exit;

    }  /* end if */

    lSize = lfNoVideo ? 0 : _fill_packet_queues ();

   } else if (  ( lBtn == SMS_PAD_LEFT || lBtn == RC_SCAN_LEFT || lBtn == RC_LEFTX ) && lfSeekable  ) {

    if (  !RewFunc ()  ) {

     s_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
     goto exit;

    }  /* end if */

    lSize = lfNoVideo ? 0 : _fill_packet_queues ();

   } else if ( lBtn == SMS_PAD_SQUARE || lBtn == RC_DISPLAY ) {

    if ( ++s_Player.m_PanScan == 8 ) s_Player.m_PanScan = 0;

    g_Config.m_PlayerFlags &= 0x0FFFFFFF;
    g_Config.m_PlayerFlags |= ( s_Player.m_PanScan << 28 );

    s_Player.m_pIPUCtx -> ChangeMode ( s_Player.m_PanScan );

   } else if ( lBtn == SMS_PAD_L1 ) {

    s_Player.m_pIPUCtx -> Pan ( 1 );

   } else if ( lBtn == SMS_PAD_R1 ) {

    s_Player.m_pIPUCtx -> Pan ( -1 );

   } else if (  lBtn == ( SMS_PAD_R1 | SMS_PAD_L1 )  ) {

    s_Player.m_pIPUCtx -> Reset ();

   } else if (  ( lBtn == SMS_PAD_CROSS || lBtn == RC_TIME ) && !lfNoVideo  ) {

    PlayerControl_HandleOSD ( 0, 0 );

   } else if ( lBtn == SMS_PAD_L2 && !lfNoVideo ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, -25 );

    else if ( s_Player.m_OSD == 4 )

     PlayerControl_HandleOSD ( 2, -25 );

    else PlayerControl_AdjustBrightness ( -1 );

   } else if ( lBtn == SMS_PAD_R2 && !lfNoVideo ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, 25 );

    else if ( s_Player.m_OSD == 4 )

     PlayerControl_HandleOSD ( 2, 25 );

    else PlayerControl_AdjustBrightness ( 1 );

   } else if (  lBtn == ( SMS_PAD_L2 | SMS_PAD_R2 ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD == 3 ) {

     s_Player.m_AVDelta = 0;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if ( s_Player.m_OSD == 4 ) {

     s_Player.m_SVDelta = 0;
     PlayerControl_HandleOSD ( 2, 0 );

    }  /* end if */

   } else if (  ( lBtn == SMS_PAD_CIRCLE || lBtn == RC_AUDIO ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD < 3 ) {

     s_Player.m_OSD = 3;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if (  ++s_Player.m_OSD == ( s_Player.m_pSubCtx ? 5 : 4 )  ) {

     s_Player.m_OSD = 0;

    } else PlayerControl_HandleOSD ( 2, 0 );

   }  /* end if */

  } else if ( g_Config.m_PowerOff > 0 && g_Timer >= lPOffTime ) hddPowerOff ();
skip:
  g_CDDASpeed = 3;

  if ( lSize >= 0 ) {

   lSize = s_Player.m_pCont -> ReadPacket ( s_Player.m_pCont, &lPktIdx );

   if ( lSize <= 0 ) continue;

  } else {

   int i, lnPackets = 0;

   for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

    SMS_RingBuffer* lpBuff = s_Player.m_pCont -> m_pStm[ i ] -> m_pPktBuf;

    if ( lpBuff ) lnPackets += SMS_RingBufferCount ( lpBuff );

   }  /* end if */

   lnPackets += SMS_RingBufferCount ( s_pVideoBuffer );
   lnPackets += SMS_RingBufferCount ( s_pAudioBuffer );

   if ( !lnPackets ) break;

   SetAlarm ( 1000, _sms_play_alarm, &lSema );
   WaitSema ( lSema );

   continue;

  }  /* end else */

  SMS_RingBufferPost ( lpStms[ lPktIdx ] -> m_pPktBuf );

 }  /* end while */

 if (  !( s_Flags & SMS_FLAGS_EXIT ) && lfNoVideo && ( g_Config.m_PlayerFlags & SMS_PF_REP )  ) {

  SMS_Container* lpCont = s_Player.m_pCont;

  if ( lpCont -> m_pPlayList ) {

   if ( lpCont -> m_pPlayList -> m_Size == 1 ) {

    lpCont -> m_pFileCtx -> Seek ( lpCont -> m_pFileCtx, s_Player.m_StartPos );

   } else {

    s_Player.m_PlayItemNr = 1;
    s_Player.m_OSDPLRes   = g_GSCtx.m_Height - 96;
    s_Player.m_OSDPLInc   = 16;
    s_Flags              |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ABSCROLL | SMS_FLAGS_ASCROLL );
    s_Player.m_pPlayItem  = s_Player.m_pCont -> m_pPlayList -> m_pHead;

    lpCont -> Seek (  lpCont, 0, 0, ( uint32_t )lpCont -> m_pPlayList -> m_pHead  );

    WaitSema ( s_SemaAckPause );

   }  /* end else */

   s_Player.m_pAudioSamples = NULL;
   s_Player.m_AudioTime     = 0;
   lSize                    = 0;

   s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

   goto repeat;

  }  /* end if */

 }  /* end if */

 DeleteSema ( lSema );
 _terminate_threads ( 1 );

 if ( s_Flags & SMS_FLAGS_EXIT ) _draw_text ( STR_STOPPING.m_pStr );

 while (  GUI_ReadButtons ()  );

}  /* end _sms_play */

static void _Destroy ( void ) {

 DiskType lType;

 DeleteSema ( s_SemaPauseAudio );
 DeleteSema ( s_SemaPauseVideo );
 DeleteSema ( s_SemaAckPause   );

 SMS_RingBufferDestroy ( s_pVideoBuffer );
 SMS_RingBufferDestroy ( s_pAudioBuffer );

 if ( s_Player.m_pSPUCtx ) {

  s_Player.m_pSPUCtx -> Destroy ();
  s_Player.m_pSPUCtx = NULL;

 }  /* end if */

 s_Player.m_pIPUCtx -> Destroy ();
 s_Player.m_pIPUCtx = NULL;

 lType = CDDA_DiskType ();

 if ( lType != DiskType_None ) CDVD_Stop ();

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Destroy ();

 s_Player.m_pCont -> Destroy ( s_Player.m_pCont, 1 );
 s_Player.m_pCont = NULL;

 SMS_TimerReset ( 2, NULL );
 PlayerControl_Destroy ();

 if (  g_Config.m_PowerOff < 0 && !( s_Flags & SMS_FLAGS_USER_STOP )  ) hddPowerOff ();

}  /* end _Destroy */

static unsigned char s_VDStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );
static unsigned char s_ADStack[ 0x8000 ] __attribute__(   (  aligned( 16 )  )   );

static unsigned char s_VPBuff[ SMS_VP_BUFFER_SIZE ] __attribute__(   (  aligned( 64 )  )   );
static unsigned char s_APBuff[ SMS_VP_BUFFER_SIZE ] __attribute__(   (  aligned( 64 )  )   );

SMS_Player* SMS_InitPlayer ( FileContext* apFileCtx, FileContext* apSubFileCtx, unsigned int aSubFormat ) {

 void ( *lpAR ) ( void* );
 void ( *lpAD ) ( void* );
 void ( *lpVR ) ( void* );

 SMS_RingBuffer* lpAudioBuffer = NULL;

 s_pVideoBuffer       = NULL;
 s_pAudioBuffer       = NULL;
 s_Flags              =   0;
 s_Player.m_AudioTime = 0LL;
 s_Player.m_VideoTime = 0LL;

 g_pSPRTop = SMS_SPR_FREE;

 GUI_Status ( STR_DETECTING_FFORMAT.m_pStr );

 s_Player.m_pCont   = SMS_GetContainer ( apFileCtx, 0x80000000 );
 s_Player.m_pSubCtx = NULL;
 s_Player.m_Flags  &= ~SMS_FLAGS_SPDIF;
 s_MainThreadID     = GetThreadId ();

 if ( s_Player.m_pCont != NULL ) {

  int         i;
  ee_thread_t lThread;

  s_Player.m_VideoIdx = 0x80000000;
  s_Player.m_AudioIdx = 0x80000000;

  for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

   SMS_Stream* lpStm = s_Player.m_pCont -> m_pStm[ i ];

   if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo && s_Player.m_VideoIdx < 0 ) {

    SMS_CodecOpen ( lpStm -> m_pCodec );

    if ( lpStm -> m_pCodec -> m_pCodec ) {

     s_Player.m_pVideoCodec = lpStm -> m_pCodec -> m_pCodec;
     s_Player.m_pVideoCodec -> Init ( lpStm -> m_pCodec );

     s_Player.m_VideoIdx = i;

     lpStm -> m_pPktBuf = SMS_RingBufferInit (
      ( void* )(
        (  ( unsigned int )&s_VPBuff[ 0 ]  ) | (
        lpStm -> m_pCodec -> m_Flags & SMS_CODEC_FLAG_UNCACHED
       )
      ), sizeof ( s_VPBuff )
     );

    }  /* end if */

   } else if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeAudio ) {

    if ( !lpAudioBuffer ) {

     SMS_CodecOpen ( lpStm -> m_pCodec );

     if ( lpStm -> m_pCodec -> m_pCodec ) {

      s_Player.m_pAudioCodec = lpStm -> m_pCodec -> m_pCodec;

      if ( lpStm -> m_pCodec -> m_ID == SMS_CodecID_AC3  ) {

       if (  !( g_Config.m_PlayerFlags & SMS_PF_SPDIF )  ) {

        if ( lpStm -> m_pCodec -> m_Channels > 2 ) lpStm -> m_pCodec -> m_Channels = 2;

       } else {

        s_Player.m_Flags               |= SMS_FLAGS_SPDIF;
        lpStm -> m_pCodec -> m_Channels = 5;

       }  /* end else */

      }  /* end if */

      s_Player.m_pAudioCodec -> Init ( lpStm -> m_pCodec );

      s_Player.m_AudioIdx        = i;
      s_Player.m_AudioChannels   = lpStm -> m_pCodec -> m_Channels;
      s_Player.m_AudioSampleRate = lpStm -> m_pCodec -> m_SampleRate;

      lpStm -> m_pPktBuf = lpAudioBuffer = SMS_RingBufferInit (  s_APBuff, sizeof ( s_APBuff )  );

     }  /* end if */

    } else {

     lpStm -> m_pPktBuf = lpAudioBuffer;
     SMS_RingBufferAddRef ( lpAudioBuffer );

    }  /* end else */

   }  /* end if */

  }  /* end for */

  FlushCache ( 0 );

  if ( s_Player.m_VideoIdx >= 0 ) {

   lpAR = _sms_audio_renderer;
   lpAD = _sms_audio_decoder;
   lpVR = _sms_video_renderer;

  } else if ( s_Player.m_AudioIdx >= 0 ) {

   lpAR = _sms_audio_only_renderer;
   lpAD = _sms_audio_only_decoder;
   lpVR = _sms_dummy_video_renderer;

   s_Player.m_PlayItemNr = 1;
   PlayerControl_UpdateItemNr ();

  } else {

   s_Player.m_pCont -> Destroy ( s_Player.m_pCont, 1 );
   s_Player.m_pCont = NULL;
   goto error;

  }  /* end else */

  s_Player.m_pFileCtx    = s_Player.m_pCont -> m_pFileCtx;
  s_Player.m_pSubFileCtx = apSubFileCtx;
  s_Player.m_SubFormat   = aSubFormat;
  s_Player.Destroy       = _Destroy;
  s_Player.Play          = _sms_play;

  lThread.stack_size       = 16384;
  lThread.stack            = g_VRStack;
  lThread.initial_priority = SMS_THREAD_PRIORITY - 1;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpVR;
  THREAD_ID_VR = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_VDStack );
  lThread.stack            = s_VDStack;
  lThread.initial_priority = SMS_THREAD_PRIORITY;
  lThread.gp_reg           = &_gp;
  lThread.func             = _sms_video_decoder;
  THREAD_ID_VD = CreateThread ( &lThread );

  lThread.stack_size       = 16384;
  lThread.stack            = g_ARStack;
  lThread.initial_priority = SMS_THREAD_PRIORITY;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpAR;
  THREAD_ID_AR = CreateThread ( &lThread );

  lThread.stack_size       = sizeof ( s_ADStack );
  lThread.stack            = s_ADStack;
  lThread.initial_priority = SMS_THREAD_PRIORITY;
  lThread.gp_reg           = &_gp;
  lThread.func             = lpAD;
  THREAD_ID_AD = CreateThread ( &lThread );

  _init_queues ( 1 );
  _start_threads ();

  s_Player.m_Flags        |= SMS_PF_SUBS;
  s_Player.m_OSD           = 0;
  s_Player.m_PanScan       = g_Config.m_PlayerFlags >> 28;
  s_Player.m_AVDelta       = 0;
  s_Player.m_SVDelta       = 0;
  s_Player.SetColors       = _set_colors;
  s_Player.m_OSDPLPos      = g_GSCtx.m_Height - 96;
  s_Player.m_pPlayItem     = NULL;
  s_Player.m_pAudioSamples = NULL;

 } else {
error:
  if (  ( int )apFileCtx > 0 && apSubFileCtx  ) apSubFileCtx -> Destroy ( apSubFileCtx );

 }  /* end else */

 return s_Player.m_pCont ? &s_Player : NULL;

}  /* end SMS_InitPlayer */
