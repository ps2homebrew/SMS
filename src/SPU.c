/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 by Gil Megidish
#               2005 Adopted for SMS by Eugene Plotnikov
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SPU.h"
#include "SIF.h"

#include <kernel.h>
#include <loadfile.h>

#define AUDSRV_IRX 0x870884D

#define MIN( a, b ) (  ( a ) <= ( b )  ) ? ( a ) : ( b )

#define MIN_VOLUME 0x0000
#define MAX_VOLUME 0x3FFF

#define AUDSRV_CMD_INIT       0x0000
#define AUDSRV_CMD_QUIT       0x0001
#define AUDSRV_CMD_SET_FORMAT 0x0003
#define AUDSRV_CMD_PLAY_AUDIO 0x0004
#define AUDSRV_CMD_WAIT_AUDIO 0x0005
#define AUDSRV_CMD_SET_VOLUME 0x0007

static SPUContext         s_SPUCtx;
static SifRpcClientData_t s_ClientData  __attribute__(   (  aligned( 64 )  )   );
static unsigned int       s_Buffer[ 3 ] __attribute__(   (  aligned( 64 )  )   );

typedef struct audsrv_fmt_t {

 int m_Freq;
 int m_BPS;
 int m_nChan;

} audsrv_fmt_t;

static int _audsrv_init ( void ) {

 int retVal = 0;

 if (  SIF_BindRPC ( &s_ClientData, AUDSRV_IRX )  ) {

  s_Buffer[ 0 ] = 1;

  if (  SifCallRpc (
         &s_ClientData, AUDSRV_CMD_INIT, 0, NULL, 0, s_Buffer, 4, 0, 0
        ) >= 0 && s_Buffer[ 0 ] == 0
  ) retVal = 1;

 }  /* end if */

 return retVal;

}  /* end _audsrv_init */

static int _audsrv_set_format ( audsrv_fmt_t* apFmt ) {

 int retVal = 0;

 s_Buffer[ 0 ] = apFmt -> m_Freq;
 s_Buffer[ 1 ] = apFmt -> m_BPS;
 s_Buffer[ 2 ] = apFmt -> m_nChan;

 if (  SifCallRpc (
        &s_ClientData, AUDSRV_CMD_SET_FORMAT, 0, s_Buffer, 12, s_Buffer, 4, 0, 0
       ) >= 0 && s_Buffer[ 0 ] == 0
 ) retVal = 1;

 return retVal;

}  /* end _audsrv_set_format */

static int _audsrv_set_volume ( int aVol ) {

 int retVal = 0;

 s_Buffer[ 0 ] = aVol;

 if (  SifCallRpc (
        &s_ClientData, AUDSRV_CMD_SET_VOLUME, 0, s_Buffer, 4, s_Buffer, 4, 0, 0
       ) >= 0 && s_Buffer[ 0 ] == 0
 ) retVal = 1;

 return retVal;

}  /* end _audsrv_set_volume */

static int _audsrv_wait_audio ( int aCount ) {

 int retVal = 0;

 s_Buffer[ 0 ] = aCount;

 if (  SifCallRpc (
        &s_ClientData, AUDSRV_CMD_WAIT_AUDIO, 0, s_Buffer, 4, s_Buffer, 4, 0, 0
       ) >= 0 && s_Buffer[ 0 ] == 0
 ) retVal = 1;

 return retVal;

}  /* end _audsrv_wait_audio */

static int _audsrv_play_audio ( char* apData ) {

 int retVal = 0;
 int lLen   = *( int* )apData;

 if (  SifCallRpc (
        &s_ClientData, AUDSRV_CMD_PLAY_AUDIO, SIF_RPC_M_NOWBDC, apData, lLen + 4, apData, 4, 0, 0
       ) >= 0 && *( int* )apData == 0
 ) retVal = 1;

 *( int* )apData = lLen;

 return retVal;

}  /* end _audsrv_play_audio */

static void _spu_destroy ( void ) {

 _audsrv_set_volume ( MIN_VOLUME );

}  /* end _spu_destroy */

static void _spu_destroy_dummy ( void ) {

}  /* end _spu_destroy_dummy */

static void _spu_mute ( int afMute ) {

 _audsrv_set_volume ( afMute ? MIN_VOLUME : MAX_VOLUME );

}  /* end _spu_mute */

static void _spu_mute_dummy ( int afMute ) {

}  /* end _spu_mute_dummy */

static void _spu_play_pcm ( char* apBuf ) {

 _audsrv_wait_audio (  *( int* )apBuf  );
 _audsrv_play_audio ( apBuf );

}  /* end _spu_play_pcm */

static void _spu_play_pcm_init ( char* apBuf ) {

 _spu_mute ( 0 );
 _spu_play_pcm ( apBuf );

 s_SPUCtx.PlayPCM = _spu_play_pcm;

}  /* end _spu_play_pcm_init */

static void _spu_play_pcm_dummy ( char* apBuf ) {

}  /* end _spu_play_pcm_dummy */

SPUContext* SPU_InitContext ( int anChannels, int aFreq ) {

 audsrv_fmt_t lFmt;

 s_SPUCtx.PlayPCM = _spu_play_pcm_dummy;
 s_SPUCtx.Mute    = _spu_mute_dummy;
 s_SPUCtx.Destroy = _spu_destroy_dummy;

 if (  _audsrv_init ()  ) {

  lFmt.m_Freq  =      aFreq;
  lFmt.m_BPS   =         16;
  lFmt.m_nChan = anChannels;

  if (  _audsrv_set_format ( &lFmt )  ) {

   s_SPUCtx.Destroy = _spu_destroy;
   s_SPUCtx.PlayPCM = _spu_play_pcm_init;
   s_SPUCtx.Mute    = _spu_mute;

  }  /* end if */

 }  /* end if */

 return &s_SPUCtx;

}  /* end SPU_InitContext */
