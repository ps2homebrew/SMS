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
#include "SMS_MP3.h"
#include "SMS_List.h"
#include "SMS_Locale.h"

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

static int _ReadPacket ( SMS_AVPacket* apPkt ) {

 FileContext* lpFileCtx = (  ( SMS_Container* )apPkt -> m_pCtx  ) -> m_pFileCtx;

 apPkt -> Alloc ( apPkt, MP3_PACKET_SIZE );

 if ( !apPkt -> m_pData ) return 0;

 apPkt -> m_Size = lpFileCtx -> Read (
  lpFileCtx, apPkt -> m_pData, MP3_PACKET_SIZE
 );

 return apPkt -> m_Size ? apPkt -> m_Size : -1;

}  /* end _ReadPacket */

int SMS_GetContainerMP3 ( SMS_Container* apCont ) {

 int          retVal = 0;
 uint8_t      lBuf[ ID3_HEADER_SIZE ];
 FileContext* lpFileCtx = apCont -> m_pFileCtx;

 if (  lpFileCtx -> Read ( lpFileCtx, lBuf, ID3_HEADER_SIZE ) == ID3_HEADER_SIZE  ) {

  uint64_t lVal;

  if (  _id3_match ( lBuf )  ) {

   lVal = (  ( lBuf[ 6 ] & 0x7F ) << 21  ) |
          (  ( lBuf[ 7 ] & 0x7F ) << 14  ) |
          (  ( lBuf[ 8 ] & 0x7F ) <<  7  ) |
             ( lBuf[ 9 ] & 0x7F );
   File_Skip (  lpFileCtx, ( uint32_t )lVal  );

  } else lpFileCtx -> Seek ( lpFileCtx, 0 );

  if (  lpFileCtx -> Read ( lpFileCtx, &lVal, 4 ) == 4 &&
        MP3_CheckHeader (
         ( uint32_t )(   lVal = SMS_bswap32 (  ( uint32_t )lVal  ) & SMS_INT64( 0x00000000FFFFFFFF )   )
        ) &&
        (   4 - (  ( lVal >> 17 ) & 3  )   )      == 3
  ) {

   int               lMPEG25;
   int               lLSF;
   int               lSampleRate;
   int               lnChannels;
   int               lBitRateIdx;
   SMS_Stream*       lpStm;
   SMS_CodecContext* lpCodecCtx;
   char*             lpSlash;
   char*             lpDot;

   if (  lVal & ( 1 << 20 )  ) {

    lLSF    = (  lVal & ( 1 << 19 )  ) ? 0 : 1;
    lMPEG25 = 0;

   } else {

    lLSF    = 1;
    lMPEG25 = 1;

   }  /* end else */

   lSampleRate = g_mpa_freq_tab[ ( lVal >> 10 ) & 3 ] >> ( lLSF + lMPEG25 );
   lnChannels  = (  ( lVal >> 6 ) & 3  ) == SMS_MPA_MONO ? 1 : 2;
   lBitRateIdx = (  ( uint32_t )lVal >> 12  ) & 0xF;

   apCont -> m_pName    = g_pMP3Str;
   apCont -> ReadPacket = _ReadPacket;

   apCont -> m_pStm[ 0 ] = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
   apCont -> m_nStm              = 1;

   lpStm -> m_pCodec = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

   lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
   lpStm -> m_SampleRate = lSampleRate;

   lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
   lpCodecCtx -> m_Tag           = 0x00000055;
   lpCodecCtx -> m_ID            = SMS_CodecID_MP3;
   lpCodecCtx -> m_Channels      = lnChannels;
   lpCodecCtx -> m_SampleRate    = lSampleRate;
   lpCodecCtx -> m_BitsPerSample = 16;

   if ( lBitRateIdx ) lpCodecCtx -> m_BitRate = g_mpa_bitrate_tab[ lLSF ][ 2 ][ lBitRateIdx ] * 1000;

   apCont -> m_Duration = 0L;

   lpFileCtx -> Seek ( lpFileCtx, 0 );

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

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerMP3 */
