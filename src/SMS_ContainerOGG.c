/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_ContainerOGG.h"
#include "SMS_Locale.h"
#include "SMS_OGG.h"

#include <malloc.h>

uint64_t SMS_OGGVProbe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 uint64_t         retVal = 0;
 char*            lpBuf;
 unsigned int     lnRead;
 ogg_sync_state   lSyncState;
 ogg_stream_state lStrmState;
 ogg_page         lPage;
 ogg_packet       lPacket;
 vorbis_info      lInfo;
 vorbis_comment   lComment;

 ogg_sync_init ( &lSyncState );
 lpBuf  = ogg_sync_buffer ( &lSyncState, 4096 );
 lnRead = apFileCtx -> Read ( apFileCtx, lpBuf, 4096 );
 ogg_sync_wrote ( &lSyncState, lnRead );

 if (  ogg_sync_pageout ( &lSyncState, &lPage ) == 1  ) {

  ogg_stream_init (  &lStrmState, ogg_page_serialno ( &lPage )  );
  vorbis_info_init    ( &lInfo    );
  vorbis_comment_init ( &lComment );

  if (  ogg_stream_pagein         ( &lStrmState, &lPage         ) == 0 &&
        ogg_stream_packetout      ( &lStrmState, &lPacket       ) == 1 &&
        vorbis_synthesis_headerin ( &lInfo, &lComment, &lPacket ) == 0
  ) {
   int i = 0;
   while ( i < 2 ) {
    while ( i < 2 ) {
     int lRes = ogg_sync_pageout ( &lSyncState, &lPage );
     if ( lRes == 0 ) break;
     if ( lRes == 1 ) {
      ogg_stream_pagein ( &lStrmState, &lPage );
      while ( i < 2 ) {
       lRes = ogg_stream_packetout ( &lStrmState, &lPacket );
       if ( lRes == 0 ) break;
       if ( lRes  < 0 ) goto end;
       vorbis_synthesis_headerin ( &lInfo, &lComment, &lPacket );
       ++i;
      }  /* end while */
     }  /* end if */
    }  /* end while */
    lpBuf  = ogg_sync_buffer ( &lSyncState, 4096 );
    lnRead = apFileCtx -> Read ( apFileCtx, lpBuf, 4096 );
    if ( lnRead == 0 && i < 2 ) goto end;
    ogg_sync_wrote ( &lSyncState, lnRead );
   }  /* end while */

   apInfo -> m_SampleRate = lInfo.rate;
   apInfo -> m_nChannels  = lInfo.channels;
   apInfo -> m_BitRate    = lInfo.bitrate_nominal;

   retVal = lInfo.channels < 3 ? 1 : -1;

  }  /* end if */
end:
  ogg_stream_clear ( &lStrmState );
  vorbis_comment_clear ( &lComment );
  vorbis_info_clear    ( &lInfo    );

 }  /* end if */

 ogg_sync_clear ( &lSyncState );

 apFileCtx -> Seek ( apFileCtx, 0 );

 return retVal;

}  /* end SMS_OGGVProbe */

int SMS_GetContainerOGG ( SMS_Container* apCont ) {

 int           retVal    = 0;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_AudioInfo lInfo;

 if (  ( int )lpFileCtx <= 0  ) return retVal;

 if (   (  retVal = SMS_OGGVProbe ( lpFileCtx, &lInfo  )  ) > 0   ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;

  apCont -> m_DefPackSize = 4096;
  apCont -> m_DefPackIdx  = 0;
  apCont -> m_pName       = g_pOGGStr;
  apCont -> ReadPacket    = SMSContainer_DefReadPacket;
  apCont -> m_pStm[ 0 ]   = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
  apCont -> m_nStm        = 1;

  lpStm -> m_pCodec     = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );
  lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
  lpStm -> m_SampleRate = lInfo.m_SampleRate;

  lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
  lpCodecCtx -> m_Tag           = 0x0000674F;
  lpCodecCtx -> m_ID            = SMS_CodecID_OGGV;
  lpCodecCtx -> m_Channels      = lInfo.m_nChannels;
  lpCodecCtx -> m_SampleRate    = lInfo.m_SampleRate;
  lpCodecCtx -> m_BitsPerSample = 16;
  lpCodecCtx -> m_BitRate       = lInfo.m_BitRate;

  retVal = SMSContainer_SetName ( apCont, lpFileCtx );

 }  /* end if */

 lpFileCtx -> Seek ( lpFileCtx, 0 );

 return retVal;

}  /* end SMS_GetContainerOGG */
