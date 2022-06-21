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
#include "SMS_ContainerFLAC.h"
#include "SMS_FLAC.h"
#include "SMS_FourCC.h"
#include "SMS_Bitio.h"
#include "SMS_Locale.h"

#include <malloc.h>

static uint64_t _do_probe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo, int* apBlockSz ) {

 uint64_t     retVal = 0;
 unsigned int lPos   = apFileCtx -> m_CurPos;

 if (  SMSContainer_SkipID3 ( apFileCtx )  ) {

  lPos = apFileCtx -> m_CurPos;

  if (  File_GetUInt ( apFileCtx ) == SMS_MKTAG( 'f', 'L', 'a', 'C' )  ) {

   uint32_t lBuf;

   apInfo -> m_SampleRate = 0;

   while (  !FILE_EOF( apFileCtx )  ) {

    int lfLast;
    int lType;
    int lSize;

    lBuf = SMS_bswap32 (  File_GetUInt ( apFileCtx )  );
    lfLast = lBuf >> 31; lBuf <<= 1;
    lType  = lBuf >> 25; lBuf <<= 7;
    lSize  = lBuf >>  8;

    if ( lType == 0 ) {

     unsigned char lBuf[ 34 ];

     if ( lSize != 34 ) break;

     if (  apFileCtx -> Read ( apFileCtx, lBuf, 34 ) == 34  ) {

      uint64_t       lnSamples, lnSamplesHI, lnSamplesLO;
      SMS_BitContext lCtx; SMS_InitGetBits ( &lCtx, lBuf, 272 );

      if ( apBlockSz ) {

       apBlockSz[ 0 ] = SMS_GetBits ( &lCtx, 16 );
       apBlockSz[ 1 ] = SMS_GetBits ( &lCtx, 16 );
       apBlockSz[ 2 ] = SMS_GetBits ( &lCtx, 24 );
       apBlockSz[ 3 ] = SMS_GetBits ( &lCtx, 24 );

      } else SMS_SkipBits ( &lCtx, 80 );

      apInfo -> m_SampleRate = SMS_GetBits ( &lCtx, 20 );
      apInfo -> m_nChannels  = SMS_GetBits ( &lCtx,  3 ) + 1;
      apInfo -> m_BitRate    = 0;

      if ( apBlockSz )
       apBlockSz[ 4 ] = SMS_GetBits ( &lCtx, 5 ) + 1;
      else SMS_SkipBits ( &lCtx, 5 );

      lnSamplesHI = SMS_GetBits ( &lCtx,  4 );
      lnSamplesLO = SMS_GetBits ( &lCtx, 32 );
      lnSamples   = ( lnSamplesHI << 32 ) | lnSamplesLO;

      if ( apBlockSz ) apBlockSz[ 5 ] = ( int )( lnSamples / apInfo -> m_SampleRate );

     } else break;

    } else File_Skip ( apFileCtx, lSize );

    if ( lfLast ) {
     if ( apInfo -> m_SampleRate ) retVal = 1;
     break;
    }  /* end if */

   }  /* end while */

  }  /* end if */

 }  /* end if */

 apFileCtx -> Seek ( apFileCtx, lPos );

 return retVal;

}  /* end _do_probe */

uint64_t SMS_FLACProbe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 return _do_probe ( apFileCtx, apInfo, NULL );

}  /* end SMS_FLACProbe */

int SMS_GetContainerFLAC ( SMS_Container* apCont ) {

 int          retVal = 0;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 FLACData     lData;

 if (  ( int )lpFileCtx <= 0  ) return retVal;

 if (  _do_probe ( lpFileCtx, &lData.m_Info, &lData.m_MinBlockSz ) && lData.m_Info.m_nChannels < 3 ) {

  SMS_Stream*       lpStm;
  SMS_CodecContext* lpCodecCtx;

  apCont -> m_pName       = g_pFLAC;
  apCont -> m_DefPackSize = 1024;
  apCont -> ReadPacket    = SMSContainer_DefReadPacket;

  apCont -> m_pStm[ 0 ] = lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );
  apCont -> m_nStm              = 1;

  lpStm -> m_pCodec = lpCodecCtx = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

  lpStm -> m_Flags     |= SMS_STRM_FLAGS_AUDIO;
  lpStm -> m_SampleRate = lData.m_Info.m_SampleRate;

  lpCodecCtx -> m_Type          = SMS_CodecTypeAudio;
  lpCodecCtx -> m_Tag           = SMS_MKTAG( 'f', 'L', 'a', 'C' );
  lpCodecCtx -> m_ID            = SMS_CodecID_FLAC;
  lpCodecCtx -> m_Channels      = lData.m_Info.m_nChannels;
  lpCodecCtx -> m_SampleRate    = lData.m_Info.m_SampleRate;
  lpCodecCtx -> m_BitsPerSample = 16;
  lpCodecCtx -> m_BitRate       = lData.m_Info.m_BitRate;
  lpCodecCtx -> m_pUserData     = malloc (  sizeof ( lData )  );
  lpCodecCtx -> m_UserDataLen   = sizeof ( lData );

  *( FLACData* )lpCodecCtx -> m_pUserData = lData;

  retVal = SMSContainer_SetName ( apCont, lpFileCtx );

  lpStm -> m_Duration = apCont -> m_Duration = lData.m_Duration;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerFLAC */
