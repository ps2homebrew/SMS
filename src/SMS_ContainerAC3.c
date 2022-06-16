/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_ContainerAC3.h"
#include "SMS_AC3.h"
#include "SMS_Locale.h"
#include "SMS_Data.h"

#include <malloc.h>

uint64_t SMS_AC3Probe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 int      i;
 uint32_t lPos = apFileCtx -> m_CurPos;

 for ( i = 0; i < 8; ++i ) {

  int      lFlags, lFrameSize;
  uint64_t lHdr = File_GetULong ( apFileCtx );

  if (  FILE_EOF( apFileCtx )  ) break;

  lFrameSize = AC3_SyncInfo (  ( uint8_t* )&lHdr, &lFlags, &apInfo -> m_SampleRate, &apInfo -> m_BitRate  );

  if ( !lFrameSize ) break;

  apInfo -> m_nChannels = g_AC3Channels[ lFlags & 7 ];

  if ( lFlags & AC3_LFE ) ++apInfo -> m_nChannels;

  File_Skip ( apFileCtx, lFrameSize - 8 );

 }  /* end for */

 apFileCtx -> Seek ( apFileCtx, lPos );

 return i == 8;

}  /* end SMS_AC3Probe */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 uint8_t       lBuf[ 8 ];
 int           retVal    = -1;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_Stream*   lpStm     = apCont -> m_pStm[ 0 ];
 SMS_AVPacket* lpPkt;

 *( uint64_t* )&lBuf[ 0 ] = 0;
 apIdx[ 0 ]               = 0;

 if (  lpFileCtx -> Read ( lpFileCtx, &lBuf, 8 ) == 8  ) while (  !FILE_EOF( lpFileCtx )  ) {

  int lFrameSize, lFlags, lSampleRate, lBitRate;

  lFrameSize = AC3_SyncInfo ( lBuf, &lFlags, &lSampleRate, &lBitRate );

  if ( lFrameSize ) {

   lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lFrameSize );

   if ( lpPkt ) {

    *( uint64_t* )lpPkt -> m_pData = *( uint64_t* )&lBuf[ 0 ];
    lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData + 8, lFrameSize - 8 );

    retVal = lFrameSize;

   }  /* end if */

   break;

  } else {

   *( uint64_t* )&lBuf[ 0 ] = *( uint64_t* )&lBuf[ 0 ] >> 8;
   lBuf[ 7 ] = File_GetByte ( lpFileCtx );

  }  /* end else */

 }  /* end while */

 return retVal;

}  /* end _ReadPacket */

int SMS_GetContainerAC3 ( SMS_Container* apCont ) {

 int           retVal    = 0;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_AudioInfo lInfo;

 if (  ( int )lpFileCtx <= 0  ) return retVal;

 if (  SMS_AC3Probe ( lpFileCtx, &lInfo )  ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;

  apCont -> m_pName    = g_pAC3;
  apCont -> ReadPacket = _ReadPacket;

  apCont -> m_pStm[ 0 ] = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
  apCont -> m_nStm              = 1;

  lpStm -> m_pCodec = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

  lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
  lpStm -> m_SampleRate = lInfo.m_SampleRate;

  lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
  lpCodecCtx -> m_Tag           = 0x00002000;
  lpCodecCtx -> m_ID            = SMS_CodecID_AC3;
  lpCodecCtx -> m_Channels      = lInfo.m_nChannels;
  lpCodecCtx -> m_SampleRate    = lInfo.m_SampleRate;
  lpCodecCtx -> m_BitsPerSample = 16;
  lpCodecCtx -> m_BitRate       = lInfo.m_BitRate;

  retVal = SMSContainer_SetName ( apCont, lpFileCtx );

  g_pSynthBuffer = SMS_AUD_SPR;

  lpFileCtx -> Seek ( lpFileCtx, 0 );

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerAC3 */
