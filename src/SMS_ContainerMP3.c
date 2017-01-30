/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
# 
*/
#include "SMS_ContainerMP3.h"
#include "SMS_MP123.h"
#include "SMS_List.h"
#include "SMS_Locale.h"
#include "SMS_Config.h"

#include <malloc.h>
#include <string.h>

#define MP3_PACKET_SIZE 1024

uint64_t SMS_MP3Probe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 SMS_ALIGN( uint8_t lBuf[ 4 ], 4 );
 unsigned int lMP3Pos = 16384;
 uint64_t     lVal    = 0;
 SMS_MPAInfo  lInfo;

 lInfo.m_FreeFmtFrameSize = 0;

 if (  SMSContainer_SkipID3 ( apFileCtx )  ) {

  apFileCtx -> Read ( apFileCtx, lBuf, 4 );

  while (  lMP3Pos && !FILE_EOF( apFileCtx )  ) {

   lVal = SMS_bswap32 (  *( uint32_t* )lBuf  );

   if (   MP123_CheckHeader  (  ( uint32_t )lVal          ) &&
         !MP123_DecodeHeader (  ( uint32_t )lVal, &lInfo  )
   ) {

    unsigned int i, lPos;

    apInfo -> m_SampleRate = lInfo.m_SampleRate;
    apInfo -> m_nChannels  = lInfo.m_nChannels;
    apInfo -> m_BitRate    = lInfo.m_BitRate;

    lPos = apFileCtx -> m_CurPos - 4;

    for (  i = 0; i < ( int )g_Config.m_MP3AutoPar; ++i  ) {

     apFileCtx -> Seek ( apFileCtx, apFileCtx -> m_CurPos + lInfo.m_FrameSize - 4 );
     apFileCtx -> Read ( apFileCtx, lBuf, 4 );

     if (  FILE_EOF( apFileCtx )  ) break;

     lVal = SMS_bswap32 (  *( uint32_t* )lBuf  );

     if (   !MP123_CheckHeader  (  ( uint32_t )lVal          ) ||
             MP123_DecodeHeader (  ( uint32_t )lVal, &lInfo  )
     ) {

      lVal = 0;
      goto end;

     }  /* end if */

    }  /* end for */

    apFileCtx -> Seek ( apFileCtx, lPos );

    break;

   }  /* end if */

   lVal = 0L;

   --lMP3Pos;

   lBuf[ 0 ] = lBuf[ 1 ];
   lBuf[ 1 ] = lBuf[ 2 ];
   lBuf[ 2 ] = lBuf[ 3 ];
   lBuf[ 3 ] = File_GetByte ( apFileCtx );

  }  /* end while */

 }  /* end if */
end:
 return lVal;

}  /* end SMS_MP3Probe */

int SMS_GetContainerMP3 ( SMS_Container* apCont ) {

 int            retVal = 0;
 FileContext*   lpFileCtx = apCont -> m_pFileCtx;
 uint64_t       lVal;
 SMS_AudioInfo  lInfo;

 if (  ( int )lpFileCtx <= 0  ) return retVal;

 lVal = SMS_MP3Probe ( lpFileCtx, &lInfo );

 if ( lVal ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;

  apCont -> m_DefPackSize = MP3_PACKET_SIZE;
  apCont -> m_DefPackIdx  = 0;
  apCont -> m_pName       = g_pMP3Str;
  apCont -> ReadPacket    = SMSContainer_DefReadPacket;

  apCont -> m_pStm[ 0 ] = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
  apCont -> m_nStm              = 1;

  lpStm -> m_pCodec = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

  lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
  lpStm -> m_SampleRate = lInfo.m_SampleRate;

  lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
  lpCodecCtx -> m_Tag           = 0x00000055;
  lpCodecCtx -> m_ID            = SMS_CodecID_MP3;
  lpCodecCtx -> m_Channels      = lInfo.m_nChannels;
  lpCodecCtx -> m_SampleRate    = lInfo.m_SampleRate;
  lpCodecCtx -> m_BitsPerSample = 16;
  lpCodecCtx -> m_BitRate       = lInfo.m_BitRate;

  retVal = SMSContainer_SetName ( apCont, lpFileCtx );

  g_pSynthBuffer = SMS_AUD_SPR;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerMP3 */
