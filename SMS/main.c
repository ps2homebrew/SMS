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
#include "GS.h"
#include "IPU.h"
#include "SMS_AVI.h"
#include "FileContext.h"
#include "SMS_DSP.h"
#include "SMS_Bitio.h"
#include "SMS_MPEG.h"
#include <stdio.h>

#ifndef _WIN32
# include "DMA.h"
# include <kernel.h>
# include <sifrpc.h>
# include <loadfile.h>
# include <audsrv.h>
#else
# include <crtdbg.h>
#endif  /* _WIN32 */

static void VideoTest ( FileContext* );
static void AudioTest ( FileContext* );

#define FILENAME "Time machine.avi"
#define FILENAME "Tom and Jerry blast off to Mar"

int main ( void ) {
#ifndef _WIN32
 CDDAContext* lpCDDACtx;
 FileContext* lpFileCtx = NULL;
 GSContext*   lpGSCtx   = GS_InitContext ( GSDisplayMode_PAL_I );

 lpGSCtx -> InitScreen ();
 lpGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );

 lpGSCtx -> m_Font.m_BkMode = GSBkMode_Opaque;
 lpGSCtx -> DrawText ( 10, 50, 0, "Checking CD..." );

 DMA_Init ( D_CTRL_RELE_ON, D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8 );
 DMA_ChannelInit ( DMA_CHANNEL_GIF      );
 DMA_ChannelInit ( DMA_CHANNEL_TO_IPU   );
 DMA_ChannelInit ( DMA_CHANNEL_FROM_IPU );
 DMA_ChannelInit ( DMA_CHANNEL_TO_SPR   );

 SifInitRpc ( 0 );
 lpCDDACtx = CDDA_InitContext ( 'Z' );

 if ( lpCDDACtx != NULL ) {

  lpGSCtx -> DrawText ( 10, 76, 0, "Initializing file context..." );
  lpFileCtx = CDDA_InitFileContext ( lpCDDACtx, FILENAME );

 } else {

  lpGSCtx -> SetTextColor (  GS_SETREG_RGBA( 0xFF, 0, 0, 0x80 )  );
  lpGSCtx -> DrawText ( 10, 76, 0, "CD was not recognized. Execution terminated." );

  SleepThread ();

 }  /* end else */
#else
 FileContext* lpFileCtx = STIO_InitFileContext ( "D:\\Test.dat" );
# ifdef _DEBUG
 _CrtSetDbgFlag (
  _CRTDBG_ALLOC_MEM_DF    |
  _CRTDBG_LEAK_CHECK_DF   |
  _CRTDBG_CHECK_ALWAYS_DF
 );
# endif  /* _DEBUG */
#endif  /* _WIN32 */
 SMS_DSP_Init   ();
 SMS_Timer_Init ();

 if ( lpFileCtx != NULL ) {
#if 0
  VideoTest ( lpFileCtx );
#elif 0
  AudioTest ( lpFileCtx );
#elif 0 && !defined( _WIN32 )
  PlayerTest ( lpFileCtx );
#elif 1 && !defined( _WIN32 )
  GSContext*     lpGSCtx  = GS_InitContext ( GSDisplayMode_PAL_I );
  SMS_AVIPlayer* lpPlayer = SMS_AVIInitPlayer ( lpFileCtx, lpGSCtx );

  if ( lpPlayer != NULL ) {

   lpPlayer -> Play    ();
   lpPlayer -> Destroy ();

  } else {

   lpGSCtx -> SetTextColor (  GS_SETREG_RGBA( 0xFF, 0, 0, 0x80 )  );
   lpGSCtx -> DrawText ( 10, 128, 0, "Failed to open AVI file. Execution terminated." );

   SleepThread ();

  }  /* end if */
#endif
 } else {

  lpGSCtx -> SetTextColor (  GS_SETREG_RGBA( 0xFF, 0, 0, 0x80 )  );
  lpGSCtx -> DrawText ( 10, 102, 0, "Could not open file. Execution terminated." );

 }  /* end else */
#ifndef _WIN32
 if ( lpCDDACtx != NULL ) CDDA_DestroyContext ( lpCDDACtx );

 printf ( "Done\n" );

 SleepThread ();
#endif  /* _WIN32 */
 return 0;

}  /* end main */

static void VideoTest ( FileContext* apCtx ) {

 GSContext*      lpGSCtx  = GS_InitContext ( GSDisplayMode_PAL_I );
 IPUContext*     lpIPUCtx;
 SMS_AVIContext* lpAVICtx = SMS_AVINewContext ( apCtx );

 if ( lpAVICtx != NULL ) {

  if (  SMS_AVIProbeFile ( lpAVICtx ) && SMS_AVIReadHeader ( lpAVICtx )  ) {

   int            lSize;
   uint64_t       lTime;
   uint32_t       i, lnFrames = 0, lVideoIdx = 0xFFFFFFFF;
   SMS_AVIPacket* lpPacket = SMS_AVINewPacket ( lpAVICtx );
   SMS_Frame*     lpFrame;
   SMS_Codec*     lpCodec  = NULL;
   uint32_t       lWidth   = 0;
   uint32_t       lHeight  = 0;

   SMS_AVICalcFrameRate ( lpAVICtx );
   SMS_AVIPrintInfo     ( lpAVICtx );

   for ( i = 0; i < lpAVICtx -> m_nStm; ++i )

    if ( lpAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

     lWidth    = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_Width;
     lHeight   = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_Height;
     lVideoIdx = i;

     SMS_CodecOpen ( &lpAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( lpAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

      lpCodec = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      lpCodec -> Init ( &lpAVICtx -> m_pStm[ i ] -> m_Codec );

     }  /* end if */

     break;

    }  /* end for */

    if ( lpCodec ) {
#ifndef _WIN32
     lpGSCtx -> m_fDblBuf = GS_OFF;
     lpGSCtx -> m_fZBuf   = GS_OFF;
#endif  /* _WIN32 */
     lpGSCtx -> InitScreen ();
     lpGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0xFF, 0x00 )  );

     lpIPUCtx = IPU_InitContext ( lpGSCtx, lWidth, lHeight );
#ifndef _WIN32
     apCtx -> Stream ( apCtx, apCtx -> m_CurPos, 512 );
#endif  /* _WIN32 */
     lTime = SMS_Time ();

     while ( 1 ) {

      lSize = SMS_AVIReadPacket ( lpPacket );

      if ( lSize < 0 ) break;

      if ( lpPacket -> m_StmIdx != lVideoIdx ) continue;

      if (  lpCodec -> Decode (
             &lpAVICtx -> m_pStm[ lVideoIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
            )
      ) {

       lpIPUCtx -> Sync ();
       lpIPUCtx -> Display ( lpFrame -> m_pData, &lpFrame -> m_Locked );

       ++lnFrames;

       if ( lnFrames == 4096 ) break;

      }  /* end if */

     }  /* end while */

     printf (   "Frames: %u\tTime: %u\n", lnFrames, ( uint32_t )(  SMS_Time () - lTime  )   );

     lpIPUCtx -> Sync    ();
     lpIPUCtx -> Destroy ();
     lpPacket -> Destroy ( lpPacket );

    }  /* end if */

  }  /* end if */

  lpAVICtx  -> Destroy ( lpAVICtx );

 }  /* end if */

 lpGSCtx  -> Destroy ();

}  /* end VideoTest */

static void AudioTest ( FileContext* apCtx ) {

 GSContext*      lpGSCtx  = GS_InitContext ( GSDisplayMode_PAL_I );
 SMS_AVIContext* lpAVICtx = SMS_AVINewContext ( apCtx );

 if ( lpAVICtx != NULL ) {

  if (  SMS_AVIProbeFile ( lpAVICtx ) && SMS_AVIReadHeader ( lpAVICtx )  ) {

   int             lSize;
   uint64_t        lTime;
   uint32_t        i, lnFrames = 0, lAudioIdx = 0xFFFFFFFF;
   SMS_AVIPacket*  lpPacket = SMS_AVINewPacket ( lpAVICtx );
   SMS_AudioFrame* lpFrame  = NULL;
   SMS_Codec*      lpCodec  = NULL;

   SMS_AVICalcFrameRate ( lpAVICtx );
   SMS_AVIPrintInfo     ( lpAVICtx );
#ifndef _WIN32
   SifLoadModule ( "rom0:LIBSD",      0, NULL );
   SifLoadModule ( "host:audsrv.irx", 0, NULL );
   audsrv_init ();
   audsrv_set_volume ( MAX_VOLUME );
#endif  /* _WIN32 */
   for ( i = 0; i < lpAVICtx -> m_nStm; ++i )

    if ( lpAVICtx -> m_pStm[ i ] -> m_Codec.m_Type == SMS_CodecTypeAudio ) {

     lAudioIdx = i;

     SMS_CodecOpen ( &lpAVICtx -> m_pStm[ i ] -> m_Codec );

     if ( lpAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {
#ifndef _WIN32
      audsrv_fmt_t lAudFmt;

      lAudFmt.bits     = 16;
      lAudFmt.freq     = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_SampleRate;
      lAudFmt.channels = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_Channels;

      lSize = audsrv_set_format ( &lAudFmt );
      printf ( "audsrv_set_format: %d\n", lSize );
#endif  /* _WIN32 */
      lpCodec = lpAVICtx -> m_pStm[ i ] -> m_Codec.m_pCodec;
      lpCodec -> Init ( &lpAVICtx -> m_pStm[ i ] -> m_Codec );

     }  /* end if */

     break;

    }  /* end for */

    if ( lpCodec ) {

     lpGSCtx -> InitScreen ();
     lpGSCtx -> ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0xFF, 0x00 )  );
#ifndef _WIN32
     apCtx -> Stream ( apCtx, apCtx -> m_CurPos, 512 );
#endif  /* _WIN32 */
     lTime = SMS_Time ();

     while ( 1 ) {

      if ( lpFrame == NULL || lpFrame -> m_Len == 0 ) {

       lSize = SMS_AVIReadPacket ( lpPacket );

       if ( lSize < 0 ) break;

      }  /* end if */

      if ( lpPacket -> m_StmIdx != lAudioIdx ) continue;

      if (   (  lSize = lpCodec -> Decode (
                 &lpAVICtx -> m_pStm[ lAudioIdx ] -> m_Codec, ( void** )&lpFrame, lpPacket -> m_pData, lpPacket -> m_Size
                )
             )
      ) {
#ifndef _WIN32
       audsrv_wait_audio ( lSize );
       audsrv_play_audio ( lpFrame -> m_pData, lSize );
#endif  /* _WIN32 */
      }  /* end if */

     }  /* end while */
#ifndef _WIN32
     audsrv_set_volume ( MIN_VOLUME );
     audsrv_quit ();
#endif  /* _WIN32 */
     printf (   "Frames: %u\tTime: %u\n", lnFrames, ( uint32_t )(  SMS_Time () - lTime  )   );

     lpPacket -> Destroy ( lpPacket );

    }  /* end if */

  }  /* end if */

  lpAVICtx  -> Destroy ( lpAVICtx );

 }  /* end if */

 lpGSCtx  -> Destroy ();

}  /* end AudioTest */

