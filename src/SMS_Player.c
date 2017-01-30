/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 - 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
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
#include "SMS_IOP.h"
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
#include "SMS_DXSB.h"
#include "SMS_FileDir.h"
#include "SMS_History.h"
#include "SMS_PgInd.h"
#include "SMS_GUIClock.h"

#ifndef EMBEDDED
# include "SMS_OS.h"
#endif  /* EMBEDDED */

#include <kernel.h>
#include <stdio.h>
#include <limits.h>
#include <libhdd.h>
#include <string.h>
#include "libmpeg_internal.h"

#define SMS_VP_BUFFER_SIZE ( 1024 * 1024 * 2 )
#define SMS_AP_BUFFER_SIZE ( 1024 *  512     )

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

static int  ( *FFwdFunc ) ( void );
static int  ( *RewFunc  ) ( void );

extern void SMS_PlayerMenu ( void );

static void _check_ac3 ( SMS_Stream* apStm ) {
 if ( apStm -> m_pCodec -> m_ID == SMS_CodecID_AC3 ||
      apStm -> m_pCodec -> m_ID == SMS_CodecID_DTS
 ) {
  if (  !( g_Config.m_PlayerFlags & SMS_PF_SPDIF ) || ( g_Config.m_PlayerFlags & SMS_PF_PDW22 )  ) {
   if ( apStm -> m_pCodec -> m_Channels > 2 ) apStm -> m_pCodec -> m_Channels = 2;
   s_Player.m_Flags |= SMS_FLAGS_AC3;
  } else {
   s_Player.m_Flags               |= SMS_FLAGS_SPDIF;
   apStm -> m_pCodec -> m_Channels = 5;
  }  /* end else */
 } else {
  if ( apStm -> m_pCodec -> m_ID == SMS_CodecID_AAC && apStm -> m_pCodec -> m_Channels > 2 ) apStm -> m_pCodec -> m_Channels = 2;
  s_Player.m_Flags &= ~( SMS_FLAGS_SPDIF | SMS_FLAGS_AC3 );
 }  /* end else */
}  /* end _check_ac3 */

static void _sync_dma ( void ) {

 DMA_Wait ( DMAC_TO_IPU   );
 DMA_Wait ( DMAC_FROM_IPU );
 DMA_Wait ( DMAC_GIF  );
 DMA_Wait ( DMAC_VIF1 );

}  /* end _sync_dma */

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

static void _set_colors ( void* apPlayer ) {

 GSContext_SetTextColor ( 0, g_Palette[ g_Config.m_PlayerSCNIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 1, g_Palette[ g_Config.m_PlayerSCBIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 2, g_Palette[ g_Config.m_PlayerSCIIdx - 1 ] | 0x80000000 );
 GSContext_SetTextColor ( 3, g_Palette[ g_Config.m_PlayerSCUIdx - 1 ] | 0x80000000 );

}  /* end _set_colors */

static void _prepare_ipu_context ( void ) {

 int               lWidth     = 0;
 int               lHeight    = 0;
 int               lfWS       = 0;
 SMS_CodecContext* lpCodecCtx = NULL;

 if ( s_Player.m_VideoIdx >= 0 ) {

  lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;
  lWidth     = lpCodecCtx -> m_Width;
  lHeight    = lpCodecCtx -> m_Height;
  lfWS       = lpCodecCtx -> m_fWS;

 }  /* end if */

 GUI_Redraw ( GUIRedrawMethod_RedrawClear );
 SMS_GUIClockStop ();
 GS_VSync ();
 GSContext_Init ( g_Config.m_DisplayMode, GSZTest_Off, GSDoubleBuffer_Off );
 GS_VSync ();

 _set_colors ( NULL );

 s_Player.m_pIPUCtx = IPU_InitContext (
  lWidth, lHeight, s_Player.m_AudioIdx >= 0 ? &s_Player.m_AudioTime : NULL, lfWS
 );

 if ( lpCodecCtx && lpCodecCtx -> HWCtl ) lpCodecCtx -> HWCtl ( lpCodecCtx, SMS_HWC_Init );

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Prepare ();

}  /* end _prepare_ipu_context */

static void _init_spu ( void ) {

 s_Player.m_pSPUCtx -> Destroy ();
 s_Player.m_pSPUCtx = SPU_InitContext (
  s_Player.m_AudioChannels, s_Player.m_AudioSampleRate, SPU_Index2Volume ( g_Config.m_PlayerVolume ),
  s_Player.m_PDW22Base, s_Player.m_PDW22Ratio
 );

}  /* end _init_spu */

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
  s_Player.m_Flags |= SMS_FLAGS_STOP;
  if ( s_Player.m_Flags & SMS_FLAGS_PAUSE ) SignalSema ( s_SemaPauseAudio );
  SMS_RingBufferPost ( s_pAudioBuffer );
  SleepThread ();
  s_Player.m_Flags &= ~SMS_FLAGS_STOP;
 }  /* end if */

 while (  SMS_RingBufferCount ( s_pVideoBuffer )  ) {

  SMS_FrameBuffer* lpFrame = ( SMS_FrameBuffer* )SMS_RingBufferWait ( s_pVideoBuffer );

  if ( lpFrame -> m_FrameType == SMS_FT_T_TYPE ) free ( lpFrame );

  SMS_RingBufferFree ( s_pVideoBuffer, 4 );

 }  /* end while */

 SMS_RingBufferReset ( s_pVideoBuffer );

 for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

  SMS_Stream*     lpStm = s_Player.m_pCont -> m_pStm[ i ];
  SMS_RingBuffer* lpRB  = lpStm -> m_pPktBuf;

  if ( lpRB ) {

   if ( lpStm -> m_Flags & SMS_STRM_FLAGS_AUDIO ) while (  SMS_RingBufferCount ( lpRB )  ) {

    SMS_AVPacket* lpPack = ( SMS_AVPacket* )SMS_RingBufferWait ( lpRB );

    if ( lpPack -> m_Flags & SMS_PKT_FLAG_NWC ) {
     SMS_CodecContext* lpCodecCtx = ( SMS_CodecContext* )( uint32_t )lpPack -> m_DTS;
     SMS_CodecDestroy ( lpCodecCtx );
     if ( lpStm -> m_pCodec == lpCodecCtx ) lpStm -> m_pCodec = NULL;
    }  /* end if */

    SMS_RingBufferFree ( lpRB, lpPack -> m_Size + 64 );

   }  /* end while */

   SMS_RingBufferReset ( lpRB );

  }  /* end if */

 }  /* end for */

 SMS_RingBufferReset ( s_pAudioBuffer );

 if ( afDelete ) for ( i = 0; i < 4; ++i ) DeleteThread ( s_ThreadIDs[ i ] );

}  /* end _terminate_threads */

static unsigned char s_VideoBuffer[ 192 ] __attribute__(   (  aligned( 64 )  )   );

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
  s_Player.m_Flags &= ~SMS_FLAGS_DYNMSK;
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

 SMS_PgIndStop ();
 s_Player.m_pIPUCtx -> Resume ();

 for ( i = 0; i < 4; ++i ) ResumeThread ( s_ThreadIDs[ i ] );

}  /* end _block_callback */

static int _fill_packet_queues ( void ) {

 int          i, lStmIdx, lSize = 0, lLock = 0;
 SMS_Stream** lpStms = s_Player.m_pCont -> m_pStm;

 s_Player.m_pIPUCtx -> Suspend ();
 SMS_PgIndStart ();

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

 if ( !lLock ) {

  SMS_PgIndStop ();
  s_Player.m_pIPUCtx -> Resume ();

  for ( i = 0; i < 4; ++i ) ResumeThread ( s_ThreadIDs[ i ] );

 }  /* end if */

 return lSize;

}  /* end _fill_packet_queues */

static void _sms_dummy_video_renderer ( void* apParam ) {

 static u64 s_lDMA[ 16 ] __attribute__(   (  aligned( 64 )  )   );

 u64*   lpDMA     = _U( s_lDMA );
 short* lpSamples = NULL;
 int    i         = 0;

 SMS_RingBufferWait ( s_pVideoBuffer );

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 2 ], 0, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 2 ], 0  );
 lpDMA[ i++ ] = 0;

 if ( s_Player.m_pCont -> m_pPlayList ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 4 ], 0, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 4 ], 0  );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 3 ], 0, DMATAG_ID_REF,  0, ( u32 )s_Player.m_OSDPackets[ 3 ], 0  );
 lpDMA[ i++ ] = 0;

 if ( g_Config.m_PlayerFlags & SMS_PF_ANIM ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 5 ], 0, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 5 ], 0   );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 6 ], 0, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 6 ], 0   );
 lpDMA[ i++ ] = 0;

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 1 ] - 1, 0, DMATAG_ID_REF, 0, ( u32 )( s_Player.m_OSDPackets[ 1 ] + 2 ), 0   );
 lpDMA[ i++ ] = 0;

 if ( g_Config.m_PlayerFlags & SMS_PF_ASD ) {

  lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 7 ], 0, DMATAG_ID_REF, 0, ( u32 )( s_Player.m_OSDPackets[ 7 ] ), 0   );
  lpDMA[ i++ ] = 0;

 }  /* end if */

 lpDMA[ i++ ] = DMA_TAG(  s_Player.m_OSDQWC[ 0 ] - 1, 0, DMATAG_ID_REFE, 0, ( u32 )( s_Player.m_OSDPackets[ 0 ] + 2 ), 0   );
 lpDMA[ i   ] = 0;

 while ( 1 ) {

  if ( s_Player.m_pAudioSamples && lpSamples == s_Player.m_pAudioSamples )

   lpSamples += 1024;

  else lpSamples = s_Player.m_pAudioSamples;

  SMS_SpectrumCalc ( lpSamples );

  s_Player.m_pIPUCtx -> Sync ();

  if ( s_Player.m_Flags & SMS_FLAGS_PAUSE ) {

   _draw_text ( STR_PAUSE.m_pStr );

   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  if ( s_Player.m_Flags & SMS_FLAGS_VSCROLL ) {

   if ( s_Player.m_OSDPLPos == s_Player.m_OSDPLRes ) {

    s_Player.m_Flags &= ~SMS_FLAGS_VSCROLL;

    if (  !( s_Player.m_Flags & SMS_FLAGS_AASCROLL )  )

     SignalSema ( s_SemaAckPause );

    else s_Player.m_Flags &= ~( SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL );

   } else {

    s_Player.m_OSDPLPos   += s_Player.m_OSDPLInc;
    s_Player.m_OSDQWC[ 4 ] = PlayerControl_GSPacket (
     s_Player.m_OSDPLPos, s_Player.m_pCont -> m_pPlayList, s_Player.m_OSDPackets[ 4 ]
    );
    lpDMA[ 2 ] = DMA_TAG(  s_Player.m_OSDQWC[ 4 ], 1, DMATAG_ID_REF, 0, ( u32 )s_Player.m_OSDPackets[ 4 ], 0  );

   }  /* end else */

  }  /* end else */

  SMS_PlayerBallSim_Update ( s_Player.m_OSDPackets[ 5 ] );

  if (  !( g_Config.m_PlayerFlags & SMS_PF_SPDIF ) || ( g_Config.m_PlayerFlags & SMS_PF_PDW22 )  ) SMS_SpectrumUpdate ();

  if ( s_Player.m_Flags & SMS_FLAGS_DISPCTL ) {
   s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 8 ], s_Player.m_OSDPackets[ 8 ] );
   s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 9 ], s_Player.m_OSDPackets[ 9 ] );
  }  /* end if */

  __asm__ __volatile__( "sync.l\n\t" );

  s_Player.m_pIPUCtx -> Display ( s_lDMA, 0 );

 }  /* end while */

}  /* end _sms_dummy_video_renderer */

static void _sms_video_renderer ( void* apParam ) {

 SMS_FrameBuffer* lpFrame;

 while ( 1 ) {

  lpFrame = *( SMS_FrameBuffer** )SMS_RingBufferWait ( s_pVideoBuffer );

  s_Player.m_pIPUCtx -> Sync ();

  if ( s_Player.m_Flags & SMS_FLAGS_PAUSE ) {

   if (  !( s_Player.m_Flags & SMS_FLAGS_MENU )  )
    _draw_text ( STR_PAUSE.m_pStr );
   else s_Player.m_pIPUCtx -> Sync ();

   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseVideo );

  }  /* end if */

  if ( lpFrame -> m_FrameType != SMS_FT_T_TYPE ) {

   if (  s_Player.m_pSubCtx && ( s_Player.m_Flags & SMS_FLAGS_SUBS )  )

    s_Player.m_pSubCtx -> Display ( lpFrame -> m_StartPTS - s_Player.m_SVDelta );

   if ( s_Player.m_Flags & SMS_FLAGS_DISPCTL ) {
    s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 8 ], s_Player.m_OSDPackets[ 8 ] );
    s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 9 ], s_Player.m_OSDPackets[ 9 ] );
   }  /* end if */

   if ( s_Player.m_OSD ) {
    s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 0 ], s_Player.m_OSDPackets[ 0 ] );
    s_Player.m_pIPUCtx -> QueuePacket ( s_Player.m_OSDQWC[ 1 ], s_Player.m_OSDPackets[ 1 ] );
   }  /* end if */

   s_Player.m_pIPUCtx -> Display ( lpFrame, s_Player.m_VideoTime = lpFrame -> m_StartPTS );

  } else {

   if ( s_Player.m_Flags & SMS_FLAGS_SUBS )
    s_Player.m_pIPUCtx -> QueueSubtitle ( lpFrame );
   else free ( lpFrame );

  }  /* end else */

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

 while ( 1 ) if (   s_Player.m_pVideoCodec -> Decode ( lpCtx, s_pVideoBuffer, lpBuff )   ) {
  RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );
 }  /* end if */

}  /* end _sms_video_decoder */

static void _sms_audio_only_renderer ( void* apParam ) {

 SMS_CodecContext* lpCodecCtx = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec;
 float             lMult      = ( float )( s_Player.m_AudioChannels > 2 ? 2 : s_Player.m_AudioChannels );

 lMult                = (  1000.0F / ( 2 * lMult )  ) / ( float )s_Player.m_AudioSampleRate;
 s_Player.m_AudioTime = 0LL;

 while ( 1 ) {

  int64_t  lPTS;
  int      lLen;
  uint8_t* lpPacket = SMS_RingBufferWait ( s_pAudioBuffer );

  if ( s_Player.m_Flags & SMS_FLAGS_STOP ) break;

  if ( s_Player.m_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );
   s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

  }  /* end if */

  lLen = *( int* )( lpPacket + 64 );
  lPTS = *( int64_t* )lpPacket;

  if ( lPTS == SMS_STPTS_VALUE ) {

   if (   (  ( int64_t* )lpPacket  )[ 1 ]   )
    lpCodecCtx = ( SMS_CodecContext* )( uint32_t )(  ( int64_t* )lpPacket  )[ 1 ];

   PlayerControl_UpdateDuration ( 1, s_Player.m_pCont -> m_Duration );

   s_Player.m_AudioTime = 0LL;

   if ( lpCodecCtx -> m_Channels   != s_Player.m_AudioChannels   ||
        lpCodecCtx -> m_SampleRate != s_Player.m_AudioSampleRate
   ) {

    s_Player.m_AudioChannels   = lpCodecCtx -> m_Channels;
    s_Player.m_AudioSampleRate = lpCodecCtx -> m_SampleRate;

    lMult = ( float )( s_Player.m_AudioChannels > 2 ? 2 : s_Player.m_AudioChannels );
    lMult = (  1000.0F / ( 2 * lMult )  ) / ( float )s_Player.m_AudioSampleRate;

    _init_spu ();

   }  /* end if */

   if ( s_Player.m_pCont -> m_pPlayList ) {

    if ( !s_Player.m_pPlayItem ) {

     s_Player.m_pPlayItem  = s_Player.m_pCont -> m_pPlayList -> m_pHead;
     s_Player.m_PlayItemNr = 1;
     s_Player.m_Flags     &= ~SMS_FLAGS_ASCROLL;

    } else {

     if (  !( s_Player.m_Flags & SMS_FLAGS_ABSCROLL )  ) {

      s_Player.m_pPlayItem = s_Player.m_pPlayItem -> m_pNext;
      ++s_Player.m_PlayItemNr;

     } else {

      if ( s_Player.m_PlayItemNr > 1 ) {

       s_Player.m_pPlayItem = s_Player.m_pPlayItem -> m_pPrev;
       --s_Player.m_PlayItemNr;

      }  /* end if */

      s_Player.m_Flags &= ~SMS_FLAGS_ABSCROLL;

     }  /* end else */

     PlayerControl_UpdateItemNr ();

     if (  !( s_Player.m_Flags & SMS_FLAGS_ASCROLL )  ) {

      s_Player.m_OSDPLInc = -8;
      s_Player.m_OSDPLRes = s_Player.m_OSDPLPos - 32;
      s_Player.m_Flags   |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL );

     } else s_Player.m_Flags &= ~SMS_FLAGS_ASCROLL;

    }  /* end else */

   }  /* end if */

  } else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult + 0.5F );

  s_Player.m_pAudioSamples = ( short* )( lpPacket + 80 );

  s_Player.m_pSPUCtx -> PlayPCM ( lpPacket + 64 );

  SMS_RingBufferFree ( s_pAudioBuffer, lLen + 80 );

 }  /* end while */

 s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );

}  /* end _sms_audio_only_renderer */

static void _audio_callback ( SMS_RingBuffer* apRB ) {

 (  ( int64_t* )apRB -> m_pPtr  )[ 0 ] = (  ( int64_t* )apRB -> m_pUserCBParam  )[ 0 ];
 (  ( int64_t* )apRB -> m_pPtr  )[ 1 ] = (  ( int64_t* )apRB -> m_pUserCBParam  )[ 1 ];

 (  ( int64_t* )apRB -> m_pUserCBParam  )[ 0 ] = SMS_NOPTS_VALUE;
 (  ( int64_t* )apRB -> m_pUserCBParam  )[ 1 ] = 0;

 SMS_RingBufferPost ( apRB );

}  /* end _audio_callback */

static void _sms_audio_only_decoder ( void* apParam ) {

 SMS_AVPacket*     lpPacket;
 int64_t           lPTS[ 2 ]  = { SMS_STPTS_VALUE, 0 };
 SMS_Stream*       lpStm      = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ];
 SMS_RingBuffer*   lpBuff     = lpStm -> m_pPktBuf;
 SMS_CodecContext* lpCodecCtx = lpStm -> m_pCodec;
 int               lSize;

 s_pAudioBuffer -> m_pUserCBParam = &lPTS[ 0 ];
 s_pAudioBuffer -> UserCB         = _audio_callback;

 while ( 1 ) {

  lpPacket = ( SMS_AVPacket* )SMS_RingBufferWait ( lpBuff );

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {
   if ( lPTS[ 0 ] != SMS_STPTS_VALUE )
    lPTS[ 0 ] = lpPacket -> m_PTS;
   if ( lpPacket -> m_PTS == SMS_STPTS_VALUE ) {
    if ( lpPacket -> m_Flags & SMS_PKT_FLAG_NWC ) {
     SMS_CodecDestroy ( s_Player.m_pAudioCodecCtx );
     lpStm = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ];
     s_Player.m_pAudioCodecCtx = lpCodecCtx = ( SMS_CodecContext* )( uint32_t )lpPacket -> m_DTS;
     SMS_CodecOpen ( lpCodecCtx );
     s_Player.m_pAudioCodec    = lpCodecCtx -> m_pCodec;
     lPTS[ 1 ]                 = ( int64_t )( uint32_t )lpCodecCtx;
    }  /* end if */
    _check_ac3 ( lpStm );
    s_Player.m_pAudioCodec -> Init ( lpCodecCtx );
   }  /* end if */

   do {
    lSize = s_Player.m_pAudioCodec -> Decode ( lpCodecCtx, s_pAudioBuffer, lpBuff );
   } while ( lSize );

  }  /* end if */

  SMS_RingBufferFree ( lpBuff, lpPacket -> m_Size + 64 );
   
 }  /* end while */

}  /* end _sms_audio_only_decoder */

static void _sms_audio_renderer ( void* apParam ) {

 uint32_t lBPS  = 0;
 float    lMult = 0.0F;

 if ( s_Player.m_AudioIdx >= 0 ) {

  lBPS  = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec -> m_BitsPerSample;
  if ( !lBPS ) lBPS = 16;
  lMult = (  1000.0F / ( lBPS * s_Player.m_AudioChannels / 8 )  ) / ( float )s_Player.m_AudioSampleRate;

 }  /* end if */

 if ( s_Player.m_pIPUCtx ) s_Player.m_pIPUCtx -> StopSync ( 1 );

 while ( 1 ) {

  int64_t        lPTS;
  int            lLen;
  unsigned char* lpPacket = SMS_RingBufferWait ( s_pAudioBuffer );

  if ( s_Player.m_Flags & SMS_FLAGS_STOP ) break;

  if ( s_Player.m_Flags & SMS_FLAGS_PAUSE ) {

   s_Player.m_pIPUCtx -> StopSync ( 1 );
   s_Player.m_pSPUCtx -> Silence ();
   SignalSema ( s_SemaAckPause );
   WaitSema ( s_SemaPauseAudio );

   if ( s_Player.m_Flags & SMS_FLAGS_STOP ) break;

   s_Player.m_pSPUCtx -> SetVolume (  SPU_Index2Volume ( g_Config.m_PlayerVolume )  );

  }  /* end if */

  lLen = *( int* )( lpPacket + 64 );
  lPTS = (  ( int64_t* )lpPacket  )[ 0 ];

  if ( lPTS != SMS_NOPTS_VALUE )
   s_Player.m_AudioTime = lPTS;
  else s_Player.m_AudioTime += ( int64_t )( uint32_t )( lLen * lMult + 0.5F );

  s_Player.m_AudioTime += s_Player.m_AVDelta;

  if (   (  ( int64_t* )lpPacket  )[ 1 ]   ) _init_spu ();

  s_Player.m_pSPUCtx -> PlayPCM ( lpPacket + 64 );

  s_Player.m_pIPUCtx -> StopSync (
   !SMS_RingBufferCount ( s_pAudioBuffer )
  );

  SMS_RingBufferFree ( s_pAudioBuffer, lLen + 80 );

 }  /* end while */

 if ( s_Player.m_pSPUCtx ) s_Player.m_pSPUCtx -> Silence ();

 WakeupThread ( s_MainThreadID );

}  /* end _sms_audio_renderer */

static void _sms_audio_decoder ( void* apParam ) {

 SMS_AVPacket*   lpPacket;
 int64_t         lPTS[ 2 ] = { SMS_NOPTS_VALUE, 0 };
 int             lSize;
 SMS_RingBuffer* lpBuff;

 s_pAudioBuffer -> m_pUserCBParam = &lPTS[ 0 ];
 s_pAudioBuffer -> UserCB         = _audio_callback;

 if (  s_Player.m_AudioIdx < 0 || !( lpBuff = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pPktBuf )  ) return;

 while ( 1 ) {

  lpPacket = ( SMS_AVPacket* )SMS_RingBufferWait ( lpBuff );

  if ( lpPacket -> m_StmIdx == s_Player.m_AudioIdx ) {

   lPTS[ 0 ] = lpPacket -> m_PTS;

   if ( s_Player.m_AudioIdx != s_Player.m_PrevAudioIdx ) {

    SMS_Stream*       lpStm = s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ];
    SMS_CodecContext* lpCtx = lpStm -> m_pCodec;
    SMS_Codec*        lpCod = lpCtx -> m_pCodec;

    _check_ac3 ( lpStm );
    lpCod -> Init ( lpCtx );

    s_Player.m_pAudioCodec     = lpCod;
    s_Player.m_pAudioCodecCtx  = lpCtx;
    s_Player.m_PrevAudioIdx    = s_Player.m_AudioIdx;
    s_Player.m_AudioChannels   = lpCtx -> m_Channels;
    s_Player.m_AudioSampleRate = lpCtx -> m_SampleRate;

    lPTS[ 1 ] = 1;

   }  /* end if */

   do {
    lSize = s_Player.m_pAudioCodec -> Decode (
     s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec, s_pAudioBuffer, lpBuff
    );
   } while ( lSize );

  }  /* end if */

  if ( lPTS[ 0 ] != SMS_NOPTS_VALUE ) s_Player.m_AudioTime = lPTS[ 0 ];

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

 if (   s_Player.m_pPlayItem && s_Player.m_pPlayItem -> m_pNext && !(  s_Player.m_Flags & ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL )  )   ) {

  _init_queues ( 0 );
  _start_threads ();

  if ( s_Player.m_pPlayItem -> m_pNext ) {

   s_Player.m_pAudioSamples = NULL;
   s_Player.m_OSDPLInc      = -8;
   s_Player.m_OSDPLRes      = s_Player.m_OSDPLPos - 32;

   s_Player.m_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL );
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

 if (   s_Player.m_pPlayItem && s_Player.m_pPlayItem -> m_pPrev && !(  s_Player.m_Flags & ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_ABSCROLL )  )   ) {

  _init_queues ( 0 );
  _start_threads ();

  if ( s_Player.m_pPlayItem -> m_pPrev ) {

   s_Player.m_pAudioSamples = NULL;
   s_Player.m_OSDPLInc      = 8;
   s_Player.m_OSDPLRes      = s_Player.m_OSDPLPos + 32;

   s_Player.m_Flags |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ASCROLL | SMS_FLAGS_ABSCROLL );
   SMS_RingBufferPost ( s_pVideoBuffer );

   retVal = lpCont -> Seek (  lpCont, 0, 0, ( uint32_t )s_Player.m_pPlayItem -> m_pPrev  );

   WaitSema ( s_SemaAckPause );

  }  /* end if */

  SMS_RingBufferPost ( s_pVideoBuffer );

 }  /* end if */

 return retVal;

}  /* end _Rew_A */

static int _sms_play ( void* apPlayer ) {

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

   float lFPS = ( float )lpStms[ s_Player.m_VideoIdx ] -> m_pCodec -> m_FrameRate / 
                ( float )lpStms[ s_Player.m_VideoIdx ] -> m_pCodec -> m_FrameRateBase;

   SMS_GUIClockSuspend ();
   SMS_PgIndStart ();
   GUI_Progress ( STR_LOADING_SUBTITLES.m_pStr, 100, 0 );
   s_Player.m_pSubFileCtx -> Stream ( s_Player.m_pSubFileCtx, 0, 10 );
   s_Player.m_pSubCtx = SubtitleContext_Init (
    s_Player.m_pSubFileCtx, s_Player.m_SubFormat, lFPS, s_Player.m_PDW22Ratio, s_Player.m_PDW22Base
   );
   s_Player.m_pSubFileCtx -> Stream ( s_Player.m_pSubFileCtx, 0, 0 );
   s_Player.m_pSubFileCtx -> Destroy ( s_Player.m_pSubFileCtx );
   SMS_PgIndStop ();
   SMS_GUIClockResume ();
 
   if ( s_Player.m_pSubCtx -> m_ErrorCode ) {

    sprintf (
     lBuff, STR_SUB_ERROR.m_pStr,
     s_Player.m_pSubCtx -> m_ErrorCode == SubtitleError_Format ?
     STR_FORMAT.m_pStr : STR_SEQUENCE.m_pStr, s_Player.m_pSubCtx -> m_ErrorLine
    );
    GUI_Error ( lBuff );

    s_Player.m_pSubCtx = NULL;

   } else {

    int i, lnStm = s_Player.m_pCont -> m_nStm;

    for ( i = 0; i < lnStm; ++i ) if ( lpStms[ i ] -> m_Flags & SMS_STRM_FLAGS_SUBTL ) {

     SMS_RingBufferDestroy ( lpStms[ i ] -> m_pPktBuf );
     lpStms[ i ] -> m_pPktBuf = NULL;

    }  /* end for */

    s_Player.m_Flags &= ~SMS_FLAGS_DXSB;

   }  /* end else */

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
   SPU_Index2Volume ( g_Config.m_PlayerVolume ),
   s_Player.m_PDW22Base, s_Player.m_PDW22Ratio
  );

  ++lnDec;

 }  /* end if */
 
 if ( s_Player.m_pFileCtx ) {

  sprintf ( lBuff, STR_BUFFERING_FILE.m_pStr, s_Player.m_pCont -> m_pName  );

  if ( !lfNoVideo && lfSeekable ) {

   long lPTS = SMS_HistoryLook ( s_Player.m_pCont -> m_pFileCtx -> m_pPath, NULL );

   if ( lPTS != -1 ) {

    char lBuf[ 8 ];

    lPTS = SMS_Rescale ( lPTS, s_Player.m_PDW22Ratio, s_Player.m_PDW22Base );
    PlayerControl_FormatTime ( lBuf, lPTS );
    sprintf ( s_VideoBuffer, STR_RESUME.m_pStr, lBuf );

    if (  GUI_Question ( s_VideoBuffer )  ) {

     SMS_Stream* lpStm = s_Player.m_pCont -> m_pStm[ s_Player.m_VideoIdx ];

     lPTS = SMS_Rescale (  lPTS - s_Player.m_pCont -> m_StartTime, lpStm -> m_TimeBase.m_Den, SMS_TIME_BASE * ( int64_t )lpStm -> m_TimeBase.m_Num  );
     s_Player.m_pCont -> Seek ( s_Player.m_pCont, s_Player.m_VideoIdx, -1, lPTS );

    }  /* end if */

   }  /* end if */

  }  /* end if */

  GUI_Status ( lBuff );
  s_Player.m_pFileCtx -> Stream (
   s_Player.m_pFileCtx, s_Player.m_StartPos = s_Player.m_pFileCtx -> m_CurPos, s_Player.m_pFileCtx -> m_StreamSize >> 3
  );

 }  /* end if */

 _prepare_ipu_context ();

 PlayerControl_Init ();
 FlushCache ( 0 );

 if ( lfNoVideo ) {

  s_Player.m_pIPUCtx -> Suspend ();
  s_Player.m_OSDPackets[ 5 ] = SMS_PlayerBallSim_Init ( &s_Player.m_OSDQWC[ 5 ] );
  s_Player.m_pIPUCtx -> Resume  ();

  SMS_SpectrumInit ();

  PlayerControl_UpdateDuration ( 0, s_Player.m_pCont -> m_Duration );
  PlayerControl_HandleOSD ( 0, 0 );
  PlayerControl_UpdateItemNr ();
  SMS_RingBufferPost ( s_pVideoBuffer );

 } else {

  s_Player.m_OSDPackets[ 5 ] = NULL;
  lSize = _fill_packet_queues ();

 }  /* end else */
repeat:
 s_Player.m_pIPUCtx -> StopSync ( 1 );

 while ( 1 ) {

  uint32_t lBtn = GUI_ReadButtons ();

  if ( lBtn ) {

   lPOffTime = g_Timer + g_Config.m_PowerOff;

   if ( g_Timer <= lNextTime ) goto skip;

   lNextTime = g_Timer + 300;

   if (  ( lBtn == SMS_PAD_SELECT || lBtn == RC_PAUSE ) && *( int* )&s_Player.m_AudioTime  ) {

    if ( g_Config.m_ScrollBarPos == SMScrollBarPos_Inactive || lfNoVideo || !lfSeekable ) {

     s_Player.m_Flags |= SMS_FLAGS_PAUSE;

     s_Player.m_pIPUCtx -> StopSync ( 1 );

     if (  SMS_RingBufferCount ( s_pAudioBuffer )              ) WaitSema ( s_SemaAckPause );
     if (  SMS_RingBufferCount ( s_pVideoBuffer ) || lfNoVideo ) WaitSema ( s_SemaAckPause );

     s_Player.m_pIPUCtx -> StopSync ( 0 );

     if ( g_CMedia & 1 ) CDVD_Stop ();

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
     s_Player.m_Flags &= ~SMS_FLAGS_PAUSE;
     SignalSema ( s_SemaPauseAudio );
     SignalSema ( s_SemaPauseVideo );
     lSize = 0;

     if ( g_CMedia & 1 ) {
      CDDA_Standby     ();
      CDDA_Synchronize ();
     }  /* end if */

    } else if (  *( int* )&s_Player.m_AudioTime  ) {

     int lSts;

     s_Player.m_Flags |= ( SMS_FLAGS_PAUSE | SMS_FLAGS_MENU );

     s_Player.m_pIPUCtx -> StopSync ( 1 );

     if (  SMS_RingBufferCount ( s_pAudioBuffer )  ) WaitSema ( s_SemaAckPause );
     if (  SMS_RingBufferCount ( s_pVideoBuffer )  ) WaitSema ( s_SemaAckPause );

     s_Player.m_pIPUCtx -> StopSync ( 0 );

     while (  GUI_ReadButtons ()  );

     if (   !(  lSts = PlayerControl_ScrollBar ( _init_queues )  )   ) {
      s_Player.m_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
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

    s_Player.m_Flags |= ( SMS_FLAGS_PAUSE | SMS_FLAGS_MENU );

    if (  SMS_RingBufferCount ( s_pAudioBuffer )  ) WaitSema ( s_SemaAckPause );
    if (  SMS_RingBufferCount ( s_pVideoBuffer )  ) WaitSema ( s_SemaAckPause );

    s_Player.m_pIPUCtx -> Suspend ();

    if ( lpCodecCtx -> m_Flags & SMS_CODEC_FLAG_IPU ) {
     SuspendThread ( THREAD_ID_VD );
     _MPEG_Suspend ();
     IPU_FRST      ();
    }  /* end if */

    _sync_dma ();
    SMS_PlayerMenu ();

    if ( lpCodecCtx -> m_Flags & SMS_CODEC_FLAG_IPU ) {
     ResumeThread ( THREAD_ID_VD );
     IPU_FRST     ();
     _MPEG_Resume ();
    }  /* end if */

    s_Player.m_pIPUCtx -> Resume  ();
    s_Player.m_Flags &= ~SMS_FLAGS_MENU;

    lPOffTime = g_Timer + g_Config.m_PowerOff;

    goto resume;

   } else if ( lBtn == SMS_PAD_TRIANGLE ||
               lBtn == RC_RESET         ||
               lBtn == RC_RETURN        ||
               lBtn == RC_STOP
          ) {
exit:
    s_Player.m_Flags |= ( SMS_FLAGS_EXIT | SMS_FLAGS_USER_STOP );
    break;

   } else if (  ( lBtn == SMS_PAD_UP ) && lfVolume && s_Player.m_pSPUCtx ) {

    PlayerControl_AdjustVolume ( 1 );

   } else if (  ( lBtn == SMS_PAD_DOWN ) && lfVolume && s_Player.m_pSPUCtx ) {

    PlayerControl_AdjustVolume ( -1 );

   } else if (   lfSeekable && (  lBtn == SMS_PAD_RIGHT || lBtn == RC_SCAN_RIGHT || ( lfNoVideo && lBtn == RC_NEXT )  )   ) {

    if (  !FFwdFunc ()  ) {

     s_Player.m_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
     goto exit;

    }  /* end if */

    lSize = lfNoVideo ? 0 : _fill_packet_queues ();

   } else if (   lfSeekable && ( lBtn == SMS_PAD_LEFT || lBtn == RC_SCAN_LEFT || ( lfNoVideo && lBtn == RC_PREV )  )   ) {

    if (  !RewFunc ()  ) {

     s_Player.m_Flags |= ( SMS_FLAGS_STOP | SMS_FLAGS_USER_STOP );
     goto exit;

    }  /* end if */

    lSize = lfNoVideo ? 0 : _fill_packet_queues ();
    s_Player.m_EOF = 0;

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

   } else if (  ( lBtn == SMS_PAD_CROSS || lBtn == RC_TIME || lBtn == RC_ENTER ) && !lfNoVideo  ) {

    PlayerControl_HandleOSD ( 0, 0 );

   } else if (  ( lBtn == SMS_PAD_L2 || lBtn == RC_PREV ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, -10 );

    else if ( s_Player.m_OSD == 4 )

     PlayerControl_HandleOSD ( 2, -10 );

    else PlayerControl_AdjustBrightness ( -1 );

   } else if (  ( lBtn == SMS_PAD_R2 || lBtn == RC_NEXT ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD == 3 )

     PlayerControl_HandleOSD ( 1, 10 );

    else if ( s_Player.m_OSD == 4 )

     PlayerControl_HandleOSD ( 2, 10 );

    else PlayerControl_AdjustBrightness ( 1 );

   } else if (  lBtn == ( SMS_PAD_L2 | SMS_PAD_R2 ) && !lfNoVideo  ) {

    if ( s_Player.m_OSD == 3 ) {

     s_Player.m_AVDelta = 0;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if ( s_Player.m_OSD == 4 ) {

     s_Player.m_SVDelta = 0;
     PlayerControl_HandleOSD ( 2, 0 );

    }  /* end if */

   } else if ( lBtn == SMS_PAD_CIRCLE && !lfNoVideo  ) {

    if ( s_Player.m_OSD < 3 ) {

     s_Player.m_OSD = 3;
     PlayerControl_HandleOSD ( 1, 0 );

    } else if (  ++s_Player.m_OSD == ( s_Player.m_pSubCtx ? 5 : 4 )  ) {

     s_Player.m_OSD = 0;

    } else PlayerControl_HandleOSD ( 2, 0 );

   } else if ( lBtn == RC_AUDIO && !lfNoVideo ) {

    PlayerControl_ChangeLangOSD ( THREAD_ID_VR );

   }  /* end if */

  } else if ( g_Config.m_PowerOff > 0 && g_Timer >= lPOffTime ) SMS_IOPowerOff ();
skip:
  g_CDDASpeed = 3;

  if ( lSize >= 0 ) {

   lSize = s_Player.m_pCont -> ReadPacket ( s_Player.m_pCont, &lPktIdx );

   if ( lSize <= 0 ) continue;

  } else {

   int i, lnPackets = 0;

   s_Player.m_EOF = 1;

   for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

    SMS_RingBuffer* lpBuff = s_Player.m_pCont -> m_pStm[ i ] -> m_pPktBuf;

    if ( lpBuff ) lnPackets += SMS_RingBufferCount ( lpBuff );

   }  /* end if */

   lnPackets += SMS_RingBufferCount ( s_pAudioBuffer );
   lnPackets += SMS_RingBufferCount ( s_pVideoBuffer );

   if ( !lnPackets ) break;

   SetAlarm ( g_GSCtx.m_PHeight, _sms_play_alarm, &lSema );
   WaitSema ( lSema );

   continue;

  }  /* end else */

  SMS_RingBufferPost ( lpStms[ lPktIdx ] -> m_pPktBuf );

 }  /* end while */

 if (  !( s_Player.m_Flags & SMS_FLAGS_EXIT ) && lfNoVideo && ( g_Config.m_PlayerFlags & SMS_PF_REP )  ) {

  SMS_Container* lpCont = s_Player.m_pCont;

  if ( lpCont -> m_pPlayList ) {

   if ( lpCont -> m_pPlayList -> m_Size == 1 ) {

    lpCont -> m_pFileCtx -> Seek ( lpCont -> m_pFileCtx, s_Player.m_StartPos );

   } else {

    s_Player.m_PlayItemNr = 1;
    s_Player.m_OSDPLRes   = g_GSCtx.m_Height - 96;
    s_Player.m_OSDPLInc   = 16;
    s_Player.m_Flags     |= ( SMS_FLAGS_VSCROLL | SMS_FLAGS_ABSCROLL | SMS_FLAGS_ASCROLL );
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
 IPU_FRST ();

 if ( s_Player.m_Flags & SMS_FLAGS_EXIT ) _draw_text ( STR_STOPPING.m_pStr );

 while (  GUI_ReadButtons ()  );

 return s_Player.m_Flags & SMS_FLAGS_USER_STOP;

}  /* end _sms_play */

static unsigned char* s_pVPBuff;
static unsigned char* s_pAPBuff;

static void _Destroy ( void* apPlayer ) {

 int  lfSeekable = s_Player.m_pCont -> m_Flags & SMS_CONT_FLAGS_SEEKABLE;
 char lPath[ 512 ];

 strcpy ( lPath, s_Player.m_pCont -> m_pFileCtx -> m_pPath );

 if ( s_Player.m_AudioIdx >= 0 &&
      ( s_Player.m_pCont -> m_pStm[ s_Player.m_AudioIdx ] == NULL ||
        s_Player.m_pAudioCodecCtx != s_Player.m_pCont -> m_pStm[
         s_Player.m_AudioIdx
        ] -> m_pCodec
      )
 ) SMS_CodecDestroy ( s_Player.m_pAudioCodecCtx );

 DeleteSema ( s_SemaPauseAudio );
 DeleteSema ( s_SemaPauseVideo );
 DeleteSema ( s_SemaAckPause   );

 SMS_RingBufferDestroy ( s_pVideoBuffer );
 SMS_RingBufferDestroy ( s_pAudioBuffer );

 if ( s_pAPBuff ) {
  free ( s_pAPBuff );
  s_pAPBuff = NULL;
 }  /* end if */

 if ( s_pVPBuff ) {
  free ( s_pVPBuff );
  s_pVPBuff = NULL;
 }  /* end if */

 s_Player.m_pIPUCtx -> Destroy ();
 s_Player.m_pIPUCtx = NULL;

 if (  CDDA_DiskType () != DiskType_None  ) CDVD_Stop ();

 if ( s_Player.m_pSubCtx ) s_Player.m_pSubCtx -> Destroy ();

 s_Player.m_pCont -> Destroy ( s_Player.m_pCont, 1 );
 s_Player.m_pCont = NULL;

 SMS_TimerReset ( 2, NULL );
 PlayerControl_Destroy ();

 if ( s_Player.m_pSPUCtx ) {

  s_Player.m_pSPUCtx -> Destroy ();
  s_Player.m_pSPUCtx = NULL;

 }  /* end if */

 if (  s_Player.m_AudioIdx >= 0 &&
       s_Player.m_VideoIdx >= 0 &&
       lfSeekable
 ) {

  if ( s_Player.m_Flags & SMS_FLAGS_USER_STOP ) {
   long int lVideoTime = SMS_Rescale ( s_Player.m_VideoTime, s_Player.m_PDW22Base, s_Player.m_PDW22Ratio );
   SMS_HistoryAdd ( lPath, lVideoTime );
   SMS_HistorySave ();
  } else if (  SMS_HistoryRemove ( lPath )  ) SMS_HistorySave ();

 }  /* end if */

 if (  g_Config.m_PowerOff < 0 && !( s_Player.m_Flags & SMS_FLAGS_USER_STOP )  ) SMS_IOPowerOff ();

}  /* end _Destroy */
#ifdef EMBEDDED
static unsigned char s_VDStack[ 0x4000 ] __attribute__(   (  aligned( 16 )  )   );
static unsigned char s_ADStack[ 0x4000 ] __attribute__(   (  aligned( 16 )  )   );
#else
# define s_VDStack &g_OS[     0 ]
# define s_ADStack &g_OS[ 16384 ]
#endif  /* EMBEDDED */

extern void SMS_JPEGPlayerDestroy ( void* );
extern int  SMS_JPEGPlayerPlay    ( void* );

#define MAX_SUB 8

static void _setup_video_pulldown22 ( void ) {

 unsigned short lVideoMode = GS_Params () -> m_GSCRTMode;

 if ( s_Player.m_VideoIdx >= 0                && !( s_Player.m_Flags & SMS_FLAGS_SPDIF                                       ) &&
    ( g_Config.m_PlayerFlags & SMS_PF_PDW22 ) &&  (  lVideoMode == GSVideoMode_PAL || lVideoMode == GSVideoMode_DTV_640x576P )
 ) {

  int            i;
  SMS_Container* lpCont     = s_Player.m_pCont;
  SMS_Stream*    lpVideoStm = lpCont -> m_pStm[ s_Player.m_VideoIdx ];
  uint32_t       lRate      = lpVideoStm -> m_pCodec -> m_FrameRate;
  uint32_t       lBase      = lpVideoStm -> m_pCodec -> m_FrameRateBase;
  float          lFPS       = ( float )lRate / ( float )lBase;
 
  if ( lFPS > 23.8 && lFPS < 25.0F ) {

   s_Player.m_Flags |= SMS_FLAGS_PDW22;

   SMSContainer_SetPTSInfo ( lpVideoStm, 1, 25 );
 
   lpVideoStm -> m_pCodec -> m_FrameRate     = 25;
   lpVideoStm -> m_pCodec -> m_FrameRateBase = 1;

   s_Player.m_PDW22Ratio = lRate;
   s_Player.m_PDW22Base  = lBase * 25;

   SMS_DXSB_SetRatio ( s_Player.m_PDW22Ratio, s_Player.m_PDW22Base );

   lpCont -> m_Duration  = SMS_Rescale (
    lpCont -> m_Duration, s_Player.m_PDW22Ratio, s_Player.m_PDW22Base
   );

   for ( i = 0; i < lpCont -> m_nStm; ++i ) {

    SMS_Stream* lpStm = lpCont -> m_pStm[ i ];

    if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeAudio ) {

     int64_t a = ( int64_t )( lpStm -> m_TimeBase.m_Num ) * lRate;
     int64_t b = ( int64_t )( lpStm -> m_TimeBase.m_Den ) * lBase;
     int     c = 25;
     int     j;

     for ( j = 0; j < 2; ++j ) if ( a % 5 == 0 ) {       
      a = a / 5;
      c = c / 5;
     }  /* end if */
 
     int64_t d = a;
     int64_t e = b;

     while ( e > 0 ) {
      int64_t r = d % e;
      d = e;
      e = r;
     }  /* end while */

     a = a / d;
     b = ( b / d ) * c;

     while (  ( a | b ) > 0x7FFFFFFF  ) {
      a = a >> 1;
      b = b >> 1;
     }  /* end while */

     SMSContainer_SetPTSInfo ( lpStm, a, b );

    }  /* end if */

   }  /* end for */

  }  /* end if */

 }  /* end if */

}  /* end _setup_video_pulldown22 */

SMS_Player* SMS_InitPlayer ( FileContext* apFileCtx, FileContext* apSubFileCtx, unsigned int aSubFormat ) {

 void ( *lpAR ) ( void* );
 void ( *lpAD ) ( void* );
 void ( *lpVR ) ( void* );

 SMS_RingBuffer* lpVideoBuffer = NULL;
 SMS_RingBuffer* lpAudioBuffer = NULL;

 s_pVideoBuffer        = NULL;
 s_pAudioBuffer        = NULL;
 s_Player.m_Flags     &= ~SMS_FLAGS_DYNMSK;
 s_Player.m_AudioTime  = 0L;
 s_Player.m_VideoTime  = 0L;
 s_Player.m_EOF        = 0;
 s_Player.m_pErrorMsg  = &STR_UNSUPPORTED_FILE;
 s_Player.m_PDW22Base  = 1;
 s_Player.m_PDW22Ratio = 1;

 g_pSPRTop = SMS_SPR_FREE;

 GUI_Status ( STR_DETECTING_FFORMAT.m_pStr );

 SMS_PgIndStart ();

 s_Player.m_pCont   = SMS_GetContainer ( apFileCtx, 0x80000000 );
 s_Player.m_pSubCtx = NULL;
 s_Player.m_Flags  &= ~( SMS_FLAGS_SPDIF | SMS_FLAGS_DXSB | SMS_PF_PDW22 );
 s_MainThreadID     = GetThreadId ();

 if ( s_Player.m_pCont != NULL ) {

  if (  !strcmp ( s_Player.m_pCont -> m_pName, g_pJPEG )  ) {

   s_Player.Play    = SMS_JPEGPlayerPlay;
   s_Player.Destroy = SMS_JPEGPlayerDestroy;

  } else {
  
   int         i;
   ee_thread_t lThread;
   int         lSubStm[ MAX_SUB ];
   int         lSubIdx = 0;

   s_Player.m_VideoIdx = 0x80000000;
   s_Player.m_AudioIdx = 0x80000000;

   for ( i = 0; i < s_Player.m_pCont -> m_nStm; ++i ) {

    SMS_Stream* lpStm = s_Player.m_pCont -> m_pStm[ i ];

    if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {

     if ( s_Player.m_VideoIdx < 0 ) {

      if (  SMS_CodecOpen ( lpStm -> m_pCodec )  ) {

       if ( lpStm -> m_pCodec -> m_ID != SMS_CodecID_DXSB ) {

        SMS_CodecContext* lpVideoCtx = lpStm -> m_pCodec;

        if ( lpVideoCtx -> m_Width  > 1024 ||
             lpVideoCtx -> m_Height > 1024
        ) {
         s_Player.m_pErrorMsg = &STR_RESOLUTION_TOO_BIG;
         goto destroy;
        }  /* end if */

        s_Player.m_pVideoCodec = lpVideoCtx -> m_pCodec;
        s_Player.m_pVideoCodec -> Init ( lpVideoCtx );

        s_Player.m_VideoIdx = i;

        s_pVPBuff = SMS_SyncMalloc ( SMS_VP_BUFFER_SIZE );
        lpStm -> m_pPktBuf = lpVideoBuffer = SMS_RingBufferInit (
         ( void* )(
           (  ( unsigned int )s_pVPBuff  ) | (
           lpStm -> m_pCodec -> m_Flags & SMS_CODEC_FLAG_UNCACHED
          )
         ), SMS_VP_BUFFER_SIZE
        );

       }  /* end if */

      } else goto addSub;

     } else if ( lpStm -> m_pCodec -> m_ID == SMS_CodecID_DXSB )
addSub:
      if ( lSubIdx < MAX_SUB ) lSubStm[ lSubIdx++ ] = i;

    } else if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeAudio ) {

     if (  SMS_CodecOpen ( lpStm -> m_pCodec )  ) {

      SMS_CodecContext* lpCtx = lpStm -> m_pCodec;
      SMS_Codec*        lpCod = lpCtx -> m_pCodec;

      _check_ac3 ( lpStm );
      lpCod -> Init ( lpCtx );

      if ( !lpAudioBuffer ) {
       s_Player.m_pAudioCodec     = lpCod;
       s_Player.m_pAudioCodecCtx  = lpCtx;
       s_Player.m_PrevAudioIdx    =
       s_Player.m_AudioIdx        = i;
       s_Player.m_AudioChannels   = lpCtx -> m_Channels;
       s_Player.m_AudioSampleRate = lpCtx -> m_SampleRate;
       s_pAPBuff = SMS_SyncMalloc ( SMS_AP_BUFFER_SIZE );
       lpStm -> m_pPktBuf = lpAudioBuffer = SMS_RingBufferInit ( s_pAPBuff, SMS_AP_BUFFER_SIZE );
      } else {
       lpStm -> m_pPktBuf = lpAudioBuffer;
       SMS_RingBufferAddRef ( lpAudioBuffer );
      }  /* end else */

     }  /* end if */

    }  /* end if */

   }  /* end for */

   if ( s_Player.m_VideoIdx >= 0 ) {

    lpAR = _sms_audio_renderer;
    lpAD = _sms_audio_decoder;
    lpVR = _sms_video_renderer;

    if ( lSubIdx ) {

     SMS_DXSB_Init (
      s_Player.m_pCont -> m_pStm[  lSubStm[ 0 ]  ] -> m_pCodec -> m_Width,
      s_Player.m_pCont -> m_pStm[  lSubStm[ 0 ]  ] -> m_pCodec -> m_Height,
      &s_Player.m_SubIdx
     );

     s_Player.m_SubIdx = lSubStm[ 0 ];
     s_Player.m_Flags |= SMS_FLAGS_DXSB;

     for ( i = 0; i < lSubIdx; ++i ) {
      s_Player.m_pCont -> m_pStm[  lSubStm[ i ]  ] -> m_Flags   |= SMS_STRM_FLAGS_SUBTL;
      s_Player.m_pCont -> m_pStm[  lSubStm[ i ]  ] -> m_pPktBuf  = lpVideoBuffer;
      SMS_RingBufferAddRef ( lpVideoBuffer );
     }  /* end for */

    }  /* end if */

   } else if ( s_Player.m_AudioIdx >= 0 ) {

    lpAR = _sms_audio_only_renderer;
    lpAD = _sms_audio_only_decoder;
    lpVR = _sms_dummy_video_renderer;

    s_Player.m_PlayItemNr = 1;
    PlayerControl_UpdateItemNr ();

   } else {
destroy:
    s_Player.m_pCont -> Destroy ( s_Player.m_pCont, 1 );
    s_Player.m_pCont = NULL;
    goto error;
   }  /* end else */

   _setup_video_pulldown22 ();

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

   lThread.stack_size       = 16384;
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

   lThread.stack_size       = 16384;
   lThread.stack            = s_ADStack;
   lThread.initial_priority = SMS_THREAD_PRIORITY;
   lThread.gp_reg           = &_gp;
   lThread.func             = lpAD;
   THREAD_ID_AD = CreateThread ( &lThread );

   _init_queues ( 1 );
   _start_threads ();

   s_Player.m_Flags        |= SMS_FLAGS_SUBS;
   s_Player.m_OSD           = 0;
   s_Player.m_PanScan       = g_Config.m_PlayerFlags >> 28;
   s_Player.m_AVDelta       = g_Config.m_AVDelta;
   s_Player.m_SVDelta       = g_Config.m_SVDelta;
   s_Player.SetColors       = _set_colors;
   s_Player.m_OSDPLPos      = g_GSCtx.m_Height - 96;
   s_Player.m_pPlayItem     = NULL;
   s_Player.m_pAudioSamples = NULL;

  }  /* end else */

 } else {
error:
  if (  ( int )apFileCtx > 0 && apSubFileCtx  ) apSubFileCtx -> Destroy ( apSubFileCtx );

 }  /* end else */

 SMS_PgIndStop ();

 return s_Player.m_pCont ? &s_Player : NULL;

}  /* end SMS_InitPlayer */
