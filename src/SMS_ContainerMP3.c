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

#define ID3_HEADER_SIZE   10
#define MP3_PACKET_SIZE 1024

static int SMS_INLINE _id3_match ( const uint8_t* apBuf ) {

 return apBuf[ 0 ] == 'I'  && apBuf[ 1 ] == 'D'  && apBuf[ 2 ] == '3' &&
        apBuf[ 3 ] != 0xFF && apBuf[ 4 ] != 0xFF &&
        ( apBuf[ 6 ] & 0x80 ) == 0 && ( apBuf[ 7 ] & 0x80 ) == 0 &&
        ( apBuf[ 8 ] & 0x80 ) == 0 && ( apBuf[ 9 ] & 0x80 ) == 0;

}  /* end _id3_match */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 int           lSize;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_AVPacket* lpPkt     = apCont -> AllocPacket (
  apCont -> m_pStm[ *apIdx = 0 ] -> m_pPktBuf, MP3_PACKET_SIZE
 );

 lSize = lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData, MP3_PACKET_SIZE );

 if ( !lSize )
  SMS_RingBufferUnalloc ( apCont -> m_pStm[ 0 ] -> m_pPktBuf, MP3_PACKET_SIZE + 64 );
 else if ( lSize < MP3_PACKET_SIZE )
  memset ( lpPkt -> m_pData + lSize, 0, MP3_PACKET_SIZE - lSize );

 return lSize ? lSize : -1;

}  /* end _ReadPacket */

uint64_t SMS_MP3Probe ( FileContext* apFileCtx, SMS_MP3Info* apInfo ) {

 SMS_ALIGN( uint8_t lBuf[ ID3_HEADER_SIZE ], 4 );
 unsigned int lMP3Pos = 16384;
 uint64_t     lVal    = 0;

 if (  apFileCtx -> Read ( apFileCtx, lBuf, ID3_HEADER_SIZE ) == ID3_HEADER_SIZE  ) {

  if (  _id3_match ( lBuf )  ) {

   lVal = (  ( lBuf[ 6 ] & 0x7F ) << 21  ) |
          (  ( lBuf[ 7 ] & 0x7F ) << 14  ) |
          (  ( lBuf[ 8 ] & 0x7F ) <<  7  ) |
             ( lBuf[ 9 ] & 0x7F );
   File_Skip (  apFileCtx, ( uint32_t )lVal  );

  } else apFileCtx -> Seek ( apFileCtx, 0 );

  apFileCtx -> Read ( apFileCtx, lBuf, 4 );

  while (  lMP3Pos && !FILE_EOF( apFileCtx )  ) {

   lVal  = SMS_bswap32 (  *( uint32_t* )lBuf  );
   lVal &= SMS_INT64( 0x00000000FFFFFFFF );

   if (   MP123_CheckHeader (  ( uint32_t )lVal  )   ) break;

   lVal = 0L;

   --lMP3Pos;

   lBuf[ 0 ] = lBuf[ 1 ];
   lBuf[ 1 ] = lBuf[ 2 ];
   lBuf[ 2 ] = lBuf[ 3 ];
   lBuf[ 3 ] = File_GetByte ( apFileCtx );

  }  /* end while */

  if (  !lMP3Pos || FILE_EOF( apFileCtx )  ) lVal = 0;

 }  /* end if */

 if ( lVal ) {

  int          lMPEG25;
  int          lLSF;
  int          lBitRateIdx;
  unsigned int lMP3Pos = apFileCtx -> m_CurPos - 4;

  if (  lVal & ( 1 << 20 )  ) {

   lLSF    = (  lVal & ( 1 << 19 )  ) ? 0 : 1;
   lMPEG25 = 0;

  } else {

   lLSF    = 1;
   lMPEG25 = 1;

  }  /* end else */

  lBitRateIdx = (  ( uint32_t )lVal >> 12  ) & 0xF;

  apInfo -> m_SampleRate = g_mpa_freq_tab[ ( lVal >> 10 ) & 3 ] >> ( lLSF + lMPEG25 );
  apInfo -> m_nChannels  = (  ( lVal >> 6 ) & 3  ) == SMS_MPA_MONO ? 1 : 2;
  apInfo -> m_BitRate    = lBitRateIdx ? g_mpa_bitrate_tab[ lLSF ][ 2 ][ lBitRateIdx ] * 1000 : 0;

  apFileCtx -> Seek ( apFileCtx, lMP3Pos );

 }  /* end if */

 return lVal;

}  /* end SMS_MP3Probe */

int SMS_GetContainerMP3 ( SMS_Container* apCont ) {

 int          retVal = 0;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 uint64_t     lVal;
 SMS_MP3Info  lInfo;

 if (  ( int )lpFileCtx < 0  ) return retVal;

 lVal = SMS_MP3Probe ( lpFileCtx, &lInfo );

 if ( lVal ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;
  char*             lpSlash;
  char*             lpDot;

  apCont -> m_pName    = g_pMP3Str;
  apCont -> ReadPacket = _ReadPacket;

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

  apCont -> m_Duration = 0L;

  apCont -> m_pPlayList = SMS_ListInit ();
  lpSlash = lpFileCtx -> m_pPath + strlen ( lpFileCtx -> m_pPath ) - 1;
  lpDot   = NULL;

  while ( *lpSlash != '\\' && *lpSlash != '/' ) {

   if ( !lpDot && *lpSlash == '.' ) lpDot = lpSlash;

   --lpSlash;

  }  /* end while */

  if ( lpDot ) *lpDot = '\x00';
   SMS_ListPushBack ( apCont -> m_pPlayList, lpSlash + 1 );
  if ( lpDot ) *lpDot = '.';

  g_Config.m_PlayerFlags |= SMS_PF_AUDHP;

  retVal = 1;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerMP3 */
