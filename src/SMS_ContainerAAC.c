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
#include "SMS_ContainerAAC.h"
#include "SMS_Bitio.h"
#include "SMS_FourCC.h"

#include <malloc.h>

static const int s_SampleRates[ 16 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
 16000, 12000, 11025,  8000,  7350
};

static const uint8_t s_Channels[ 8 ] __attribute__(   (  section( ".rodata" )  )   ) = {
 0, 1, 2, 3, 4, 5, 6, 8
};

static uint64_t _aac_do_probe ( uint8_t* apBuf, SMS_AudioInfo* apInfo, int* apSize ) {

 uint64_t       retVal = 0;
 SMS_BitContext lBitCtx;
 int            lSR, lCh, lSize;

 SMS_InitGetBits ( &lBitCtx, apBuf, 64 );

 {  /* begin block */

  SMS_OPEN_READER( re, &lBitCtx );
  SMS_UPDATE_CACHE( re, &lBitCtx );

  if (  SMS_SHOW_UBITS( re, &lBitCtx, 12 ) == 0x0FFF  ) {

   SMS_SKIP_BITS( re, &lBitCtx, 18 );
   lSR = SMS_SHOW_UBITS( re, &lBitCtx, 4 );

   if (  ( lSR = s_SampleRates[ lSR ] )  ) {

    SMS_SKIP_BITS( re, &lBitCtx, 5 );
    lCh = SMS_SHOW_UBITS( re, &lBitCtx, 3 );

    if (  ( lCh = s_Channels[ lCh ] )  ) {

     SMS_SKIP_BITS( re, &lBitCtx, 7 );
     SMS_UPDATE_CACHE( re, &lBitCtx );
     lSize = SMS_SHOW_UBITS( re, &lBitCtx, 13 );

     if ( lSize > 7 ) {

      apInfo -> m_SampleRate = lSR;
      apInfo -> m_nChannels  = lCh;
      apInfo -> m_BitRate    =   0;

      apSize[ 0 ] = lSize - 8;
      retVal      = *( uint64_t* )apBuf;

     }  /* end if */

    }  /* end if */

   }  /* end if */

  }  /* end if */

 }  /* end block */

 return retVal;

}  /* end _aac_do_probe */

uint64_t SMS_AACProbe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 uint8_t  lBuf[ 8 ];
 uint32_t lPos   = apFileCtx -> m_CurPos;
 uint64_t retVal = 0;

 if (  apFileCtx -> Read ( apFileCtx, lBuf, 8 ) == 8  ) while (  !FILE_EOF( apFileCtx ) && apFileCtx -> m_CurPos - lPos < 16384  ) {

  if (   (  *( uint16_t* )&lBuf[ 0 ] & 0xF0FF  ) == 0xF0FF   ) {

   int lSize;

   if (  _aac_do_probe ( lBuf, apInfo, &lSize )  ) {

    SMS_AudioInfo lInfo;
    int           i, lnMatches = 0, lSize2;

    File_Skip ( apFileCtx, lSize );

    for ( i = 0; i < 4; ++i )
     if (  apFileCtx -> Read ( apFileCtx, lBuf, 8 ) == 8 &&
           _aac_do_probe ( lBuf, &lInfo, &lSize2 )       &&
           lInfo.m_SampleRate == apInfo -> m_SampleRate  &&
           lInfo.m_nChannels  == apInfo -> m_nChannels   
     ) {
      ++lnMatches;
      File_Skip ( apFileCtx, lSize2 );
     } else break;

    if ( lnMatches == 4 ) {
     lPos   = apFileCtx -> m_CurPos - lSize - 16;
     retVal = *( uint64_t* )&lBuf[ 0 ];
     break;
    }  /* end if */

   }  /* end if */

  }  /* end if */

  *( uint64_t* )&lBuf[ 0 ] = *( uint64_t* )&lBuf[ 0 ] >> 8;
  lBuf[ 7 ] = File_GetByte ( apFileCtx );

 }  /* end while */

 apFileCtx -> Seek ( apFileCtx, lPos );

 return retVal;

}  /* end SMS_AACProbe */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 uint8_t       lBuf[ 8 ];
 int           retVal    = -1;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_Stream*   lpStm     = apCont -> m_pStm[ 0 ];
 SMS_AVPacket* lpPkt;

 *( uint64_t* )&lBuf[ 0 ] = 0;
 apIdx[ 0 ]               = 0;

 if (  lpFileCtx -> Read ( lpFileCtx, &lBuf, 8 ) == 8  ) while (  !FILE_EOF( lpFileCtx )  ) {

  if (   (  *( uint16_t* )&lBuf[ 0 ] & 0xF0FF  ) == 0xF0FF  ) {

   uint64_t lHdr  = (    (   ( uint64_t )SMS_bswap32 (  *( uint32_t* )&lBuf[ 0 ]  )   ) << 32    ) |
                    (    (   ( uint64_t )SMS_bswap32 (  *( uint32_t* )&lBuf[ 4 ]  )   )          );
   uint32_t lSize = ( uint32_t )(  ( lHdr >> 21 ) & 0x1FFF  );
   uint32_t lnRB  = ( uint32_t )(  ( lHdr >>  8 ) & 0x0003  );

   lnRB  = lSize * 8 * lpStm -> m_SampleRate / (  ( lnRB + 1 ) * 1024  );
   lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lSize );

   if ( lpPkt ) {

    *( uint64_t* )lpPkt -> m_pData = *( uint64_t* )&lBuf[ 0 ];
    lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData + 8, lSize - 8 );

    lpPkt -> m_PTS                 = ( uint32_t )(  *( float* )&lpStm -> m_CurDTS + 0.5F  );
    *( float* )&lpStm -> m_CurDTS += lSize * 8000.0F / ( float )lnRB;
    retVal = lSize;

   }  /* end if */

   break;

  } else {

   *( uint64_t* )&lBuf[ 0 ] = *( uint64_t* )&lBuf[ 0 ] >> 8;
   lBuf[ 7 ] = File_GetByte ( lpFileCtx );

  }  /* end else */

 }  /* end while */

 return retVal;

}  /* end _ReadPacket */

int SMS_GetContainerAAC ( SMS_Container* apCont ) {

 int           retVal = 0;
 SMS_AudioInfo lInfo;
 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 uint64_t      lHdr;

 if (  ( int )lpFileCtx <= 0  ) return retVal;

 if (   (  lHdr = SMS_AACProbe ( lpFileCtx, &lInfo )  )   ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;

  apCont -> m_pName    = "AAC";
  apCont -> ReadPacket = _ReadPacket;

  apCont -> m_pStm[ 0 ] = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
  apCont -> m_nStm              = 1;

  lpStm -> m_pCodec = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

  lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
  lpStm -> m_SampleRate = lInfo.m_SampleRate;

  lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
  lpCodecCtx -> m_Tag           = SMS_MKTAG( 'a', 'a', 'c', ' ' );
  lpCodecCtx -> m_ID            = SMS_CodecID_AAC;
  lpCodecCtx -> m_Channels      = lInfo.m_nChannels;
  lpCodecCtx -> m_SampleRate    = lInfo.m_SampleRate;
  lpCodecCtx -> m_BitsPerSample = 16;
  lpCodecCtx -> m_BitRate       = lInfo.m_BitRate;
  lpCodecCtx -> m_pUserData     = malloc ( 16 );
  lpCodecCtx -> m_UserDataLen   = -16;

  *( uint64_t* )lpCodecCtx -> m_pUserData = lHdr;

  retVal = SMSContainer_SetName ( apCont, lpFileCtx );

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerAAC */
