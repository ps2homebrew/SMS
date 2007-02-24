/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
#               2007 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_ContainerMPEG_PS.h"
#include "SMS_Bitio.h"
#include "SMS_MP123.h"
#include "SMS_AC3.h"

#include <malloc.h>
#include <string.h>

#define MAX_SYNC_SIZE 100000

#define PACK_START_CODE    (  ( unsigned int )0x000001BA  )
#define SYSHDR_START_CODE  (  ( unsigned int )0x000001BB  )
#define PROGRAM_STREAM_MAP (  ( unsigned int )0x000001BC  )
#define PRIVATE_STREAM_1   (  ( unsigned int )0x000001BD  )
#define PADDING_STREAM     (  ( unsigned int )0x000001BE  )
#define PRIVATE_STREAM_2   (  ( unsigned int )0x000001BF  )

#define STREAM_TYPE_VIDEO_MPEG1 0x01
#define STREAM_TYPE_VIDEO_MPEG2 0x02
#define STREAM_TYPE_AUDIO_MPEG1 0x03
#define STREAM_TYPE_AUDIO_MPEG2 0x04
#define STREAM_TYPE_AUDIO_AC3   0x81

#define MYCONT( p ) (  ( _MPEGPSContainer* )( p -> m_pCtx )  )

typedef struct _MPEGPSContainer {

 unsigned char m_Type[ 256 ];
 unsigned char m_Buff[  15 ]; 
 int           m_BufLen;

} _MPEGPSContainer;

static int64_t _get_pts ( FileContext* apFileCtx, int aChr ) {

 int64_t retVal;
 int     lVal;

 if ( aChr < 0 ) aChr = File_GetByte ( apFileCtx );

 retVal  = ( int64_t )(  ( aChr >> 1 ) & 0x07  ) << 30;
 lVal    = File_GetShortBE ( apFileCtx );
 retVal |= ( int64_t )( lVal >> 1 ) << 15;
 lVal    = File_GetShortBE ( apFileCtx );
 retVal |= ( int64_t )( lVal >> 1 );

 return retVal;

}  /* end _get_pts */

static int _next_start_code ( FileContext* apFileCtx ) {

 unsigned int lState, lByte;
          int lVal,   lN;

 lState = 0xFF;
 lN     = MAX_SYNC_SIZE;

 while ( lN > 0 ) {

  if (  FILE_EOF( apFileCtx )  ) break;

  lByte = File_GetByte ( apFileCtx );
  --lN;

  if ( lState == 1 ) {

   lState = (  ( lState << 8 ) | lByte ) & 0x00FFFFFF;
   lVal   = lState;

   goto found;

  }  /* end if */

  lState = (  ( lState << 8 ) | lByte ) & 0x00FFFFFF;

 }  /* end while */

 lVal = -1;
found:
 return lVal;

}  /* end _next_start_code */

static long _psm_parse ( _MPEGPSContainer* apCont, FileContext* apFileCtx ) {

 int lPLen, lPILen, lMLen;

 lPLen = File_GetShortBE ( apFileCtx );
 File_GetByte ( apFileCtx );
 File_GetByte ( apFileCtx );
 lPILen = File_GetShortBE ( apFileCtx );

 File_Skip ( apFileCtx, lPILen );

 lMLen = File_GetShortBE ( apFileCtx );

 while ( lMLen >= 4 ){

  unsigned char lType = File_GetByte    ( apFileCtx );
  unsigned char lID   = File_GetByte    ( apFileCtx );
  uint16_t      lILen = File_GetShortBE ( apFileCtx );

  apCont -> m_Type[ lID ] = lType;

  File_Skip ( apFileCtx, lILen );
  lMLen -= 4 + lILen;

 }  /* end while */

 File_GetUInt ( apFileCtx );

 return 2 + lPLen;

}  /* end _psm_parse */

static int _read_header (
            SMS_Container* apCont, int* apStartCode, int64_t* apPTS, int64_t* apDTS
           ) {

 FileContext*      lpFileCtx = apCont -> m_pFileCtx;
 _MPEGPSContainer* lpMyCont  = MYCONT( apCont );
 int               lLen, lStartCode, lChr, lFlags, lHLen;
 int64_t           lPTS, lDTS, lLastPos;

 lLastPos = -1;
redo:
 lStartCode = _next_start_code ( lpFileCtx );

 if ( lStartCode < 0 ) return 0;

 if ( lStartCode == PACK_START_CODE || lStartCode == SYSHDR_START_CODE ) goto redo;

 if ( lStartCode == PADDING_STREAM || lStartCode == PRIVATE_STREAM_2 ) {

  lLen = File_GetShortBE ( lpFileCtx );
  File_Skip ( lpFileCtx, lLen );

  goto redo;

 }  /* end if */

 if ( lStartCode == PROGRAM_STREAM_MAP ) {

  _psm_parse ( lpMyCont, lpFileCtx );
  goto redo;

 }  /* end if */

 if (   !(  ( lStartCode >= 0x000001C0 && lStartCode <= 0x000001DF  ) ||
            ( lStartCode >= 0x000001E0 && lStartCode <= 0x000001EF  ) ||
            ( lStartCode == 0x000001BD                              )
         )
 ) goto redo;

 lLen = File_GetShortBE ( lpFileCtx );
 lPTS = SMS_NOPTS_VALUE;
 lDTS = SMS_NOPTS_VALUE;

 while ( 1 ) {

  if ( lLen < 1 ) goto redo;

  lChr = File_GetByte ( lpFileCtx );
  --lLen;

  if ( lChr != 0xFF ) break;

 }  /* end while */

 if (  ( lChr & 0xC0 ) == 0x40  ) {

  if ( lLen < 2 ) goto redo;

  File_GetByte ( lpFileCtx );

  lChr  = File_GetByte ( lpFileCtx );
  lLen -= 2;

 }  /* end if */

 if (  ( lChr & 0xF0 ) == 0x20  ) {

  if ( lLen < 4 ) goto redo;

  lDTS  = lPTS = _get_pts ( lpFileCtx, lChr );
  lLen -= 4;

 } else if (  ( lChr & 0xF0 ) == 0x30  ) {

  if ( lLen < 9 ) goto redo;

  lPTS  = _get_pts ( lpFileCtx, lChr );
  lDTS  = _get_pts ( lpFileCtx,   -1 );
  lLen -= 9;

 } else if (  ( lChr & 0xC0 ) == 0x80  ) {

  if ( lChr & 0x30 ) goto redo;

  lFlags = File_GetByte ( lpFileCtx );
  lHLen  = File_GetByte ( lpFileCtx );
  lLen  -= 2;

  if ( lHLen > lLen ) goto redo;

  if (  ( lFlags & 0xC0 ) == 0x80  ) {

   lDTS = lPTS = _get_pts ( lpFileCtx, -1 );

   if ( lHLen < 5 ) goto redo;

   lHLen -= 5;
   lLen  -= 5;

  } else if (  ( lFlags & 0xC0 ) == 0xC0  ) {

   lPTS = _get_pts ( lpFileCtx, -1 );
   lDTS = _get_pts ( lpFileCtx, -1 );

   if ( lHLen < 10 ) goto redo;

   lHLen -= 10;
   lLen  -= 10;

  }  /* end if */

  lLen -= lHLen;

  while ( lHLen > 0 ) {

   File_GetByte ( lpFileCtx );
   --lHLen;

  }  /* end while */

 } else if ( lChr != 0xF ) goto redo;

 if ( lStartCode == PRIVATE_STREAM_1 && !lpMyCont ->m_Type[ lStartCode & 0xFF ] ) {

  if ( lLen < 1 ) goto redo;

  lStartCode = File_GetByte ( lpFileCtx );
  --lLen;

  if ( lStartCode >= 0x80 && lStartCode <= 0xBF ) {

   if ( lLen < 3 ) goto redo;

   File_GetByte ( lpFileCtx );
   File_GetByte ( lpFileCtx );
   File_GetByte ( lpFileCtx );
   lLen -= 3;

  }  /* end if */

 }  /* end if */

 *apStartCode = lStartCode;
 *apPTS       = lPTS;
 *apDTS       = lDTS;

 return lLen;

}  /* end _read_header */

static int _fill_video_parameters ( FileContext* apFileCtx, SMS_Stream* apStm, int aLen ) {

 int          retVal = 0;
 unsigned int lPos   = apFileCtx -> m_CurPos;

 if (  _next_start_code ( apFileCtx ) == 0x000001B3  ) {

  char           lBuffer[ aLen ];
  SMS_BitContext lBitCtx;

  apFileCtx -> Read ( apFileCtx, lBuffer, aLen );
  SMS_InitGetBits ( &lBitCtx, lBuffer, aLen );

  apStm -> m_pCodec -> m_Width  = SMS_GetBits ( &lBitCtx, 12 );
  apStm -> m_pCodec -> m_Height = SMS_GetBits ( &lBitCtx, 12 );

  retVal = 1;

 }  /* end if */

 apFileCtx -> Seek ( apFileCtx, lPos );

 return retVal;

}  /* end _fill_video_parameters */

static int _fill_audio_parameters ( FileContext* apFileCtx, SMS_Stream* apStm, int aLen ) {

 int         retVal = 0;
 int         lPos   = apFileCtx -> m_Pos;
 char        lBuf[ aLen ];
 char*       lpPtr  = lBuf;
 SMS_MPAInfo lInfo;

 lInfo.m_FreeFmtFrameSize = 0;

 apFileCtx -> Read ( apFileCtx, lBuf, aLen );

 aLen -= 4;

 while ( aLen && !retVal ) {

  switch ( apStm -> m_pCodec -> m_ID ) {

   case SMS_CodecID_MP2:
   case SMS_CodecID_MP3: {

    uint64_t lHeader = SMS_bswap32 (  SMS_unaligned32 ( lpPtr )  );
    lHeader &= SMS_INT64( 0x00000000FFFFFFFF );

    if (   MP123_CheckHeader  (  ( uint32_t )lHeader          ) &&
          !MP123_DecodeHeader (  ( uint32_t )lHeader, &lInfo  )
    ) {

     apStm -> m_SampleRate             =
     apStm -> m_pCodec -> m_SampleRate = lInfo.m_SampleRate;
     apStm -> m_pCodec -> m_BitRate    = lInfo.m_BitRate;
     apStm -> m_pCodec -> m_Channels   = lInfo.m_nChannels;

     retVal = 1;
     goto end;

    }  /* end if */

   } break;

   case SMS_CodecID_AC3: {

    int lFlags;

    if (  AC3_SyncInfo ( lpPtr, &lFlags, &apStm -> m_pCodec -> m_SampleRate, &apStm -> m_pCodec -> m_BitRate )  ) {

     apStm -> m_SampleRate           = apStm -> m_pCodec -> m_SampleRate;
     apStm -> m_pCodec -> m_Channels = g_AC3Channels[ lFlags & 7 ];

     if ( lFlags & AC3_LFE ) ++apStm -> m_pCodec -> m_Channels;

     retVal = 1;
     goto end;

    }  /* end if */

   } break;

   default: break;

  }  /* end switch */

  ++lpPtr;
  --aLen;

 }  /* end while */
end:
 apFileCtx -> Seek ( apFileCtx, lPos );

 if ( retVal ) apStm -> m_Flags |= SMS_STRM_FLAGS_AUDIO;

 return retVal;

}  /* end _fill_audio_parameters */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 _MPEGPSContainer* lpMyCont  = MYCONT( apCont );
 FileContext*      lpFileCtx = apCont -> m_pFileCtx;
 SMS_AVPacket*     lpPkt     = NULL;
 SMS_Stream*       lpStm;
 SMS_Stream**      lIt, **lppStm = apCont -> m_pStm;
 int               lLen, lStartCode;
 int64_t           lPTS, lDTS;

 while ( 1 ) {

  lLen = _read_header ( apCont, &lStartCode, &lPTS, &lDTS );

  if (  FILE_EOF( lpFileCtx )  ) {

   if ( lpMyCont -> m_BufLen ) {

    for (  lIt = lppStm; ( lpStm = *lIt ); ++lIt  )

     if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {

      lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lpMyCont -> m_BufLen );
      memcpy ( lpPkt -> m_pData, lpMyCont -> m_Buff, lLen = lpMyCont -> m_BufLen );
      *apIdx = lIt - lppStm;
      break;

     }  /* end if */

    lpMyCont -> m_BufLen = 0;

    if ( lpPkt ) goto end;

   }  /* end if */

   lLen = 0x80000000;
   break;

  } else for (  lIt = lppStm; ( lpStm = *lIt ); ++lIt  ) if (  ( int )lpStm -> m_ID == lStartCode  ) break;

  if ( lpStm && lpStm -> m_pPktBuf ) {

   *apIdx = lIt - lppStm;

   if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {

    int lTotalLen = lLen + lpMyCont -> m_BufLen;
    int lPackLen  = lTotalLen & ~15;
    int lnRead    = lPackLen - lpMyCont -> m_BufLen;
    int lnRem     = lLen - lnRead;

    if ( lPackLen ) {

     lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lPackLen );
     memcpy (  lpPkt -> m_pData, lpMyCont -> m_Buff, lpMyCont -> m_BufLen );
     lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData + lpMyCont -> m_BufLen, lnRead );
     lpFileCtx -> Read ( lpFileCtx, lpMyCont -> m_Buff, lpMyCont -> m_BufLen = lnRem );

    } else {

     lpFileCtx -> Read ( lpFileCtx, lpMyCont -> m_Buff + lpMyCont -> m_BufLen, lLen );
     lpMyCont -> m_BufLen += lLen;
     continue;

    }  /* end else */

   } else {

    lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lLen );
    lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData, lLen );

   }  /* end else */
end:
   lpPkt -> m_PTS    = lPTS;
   lpPkt -> m_DTS    = lDTS;
   lpPkt -> m_StmIdx = *apIdx;

   if ( lPTS > 0 ) SMSContainer_CalcPktFields ( lpStm, lpPkt );

   break;

  } else File_Skip ( lpFileCtx, lLen );

 }  /* end while */

 return lLen;

}  /* end _ReadPacket */

static void _get_stm_pts ( SMS_Container* apCont, int anIdx, int afOverride, uint64_t* apPTS ) {

 uint64_t     lPTS, lDTS;
 SMS_Stream** lIt, **lppStm = apCont -> m_pStm;
 int          lStartCode;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;

 while ( 1 ) {

  int lLen = _read_header ( apCont, &lStartCode, &lPTS, &lDTS );

  if ( !lLen                 ) break;
  if (  ( int64_t )lPTS <= 0 ) goto next;

  for (  lIt = lppStm; *lIt; ++lIt  ) if (  ( int )( *lIt ) -> m_ID == lStartCode  ) break;

  if (   *lIt && (  afOverride || ( int64_t )apPTS[ anIdx ] < 0  )   ) {

   apPTS[ anIdx ] = lPTS;
   break;

  } else
next:
   File_Skip ( lpFileCtx, lLen );

 }  /* end while */

}  /* end _get_stm_pts */

static int _Seek ( SMS_Container* apCont, int anIdx, int aDir, uint32_t aPos ) {

 uint64_t          lPTS, lDTS;
 _MPEGPSContainer* lpMyCont  = MYCONT( apCont );
 FileContext*      lpFileCtx = apCont -> m_pFileCtx;
 SMS_Stream**      lIt, **lppStm = apCont -> m_pStm;
 SMS_Stream*       lpStm;
 int64_t           lBasePos;
 int               lStartCode;
 int               retVal;

 aPos    /= 90;
 lBasePos = ( int64_t )( unsigned int )(
  ( float )lpFileCtx -> m_Size * (  ( float )aPos / ( float )apCont -> m_Duration  )
 );

 lpMyCont  -> m_BufLen = 0;
 lpFileCtx -> Seek (  lpFileCtx, ( unsigned int )lBasePos  );

 while ( 1 ) {

  unsigned int lCurPos = lpFileCtx -> m_CurPos;

  if (   !(  retVal = _read_header ( apCont, &lStartCode, &lPTS, &lDTS )  )   ) break;

  for (  lIt = lppStm; ( lpStm = *lIt ); ++lIt  ) if (  ( int )lpStm -> m_ID == lStartCode  ) break;

  if ( lpStm && lpStm -> m_pPktBuf && lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {

   int lCode = _next_start_code ( lpFileCtx );

   if ( lCode == 0x000001B8 || lCode == 0x000001B3 ) {

    lpFileCtx -> Seek ( lpFileCtx, lCurPos );
    break;

   } else continue;

  }  /* end if */

  File_Skip ( lpFileCtx, retVal );

 }  /* end while */

 return retVal;

}  /* end _Seek */

int SMS_GetContainerMPEG_PS ( SMS_Container* apCont ) {

 uint64_t          lPTS, lDTS;
 int               i, lfVideo, lfAudio, lfVideoParam, retVal = 0;
 FileContext*      lpFileCtx;
 _MPEGPSContainer* lpMyCont;

 if (  ( int )apCont -> m_pFileCtx < 0  ) return retVal;

 i            = 0;
 lfVideo      = 0;
 lfAudio      = 0;
 lfVideoParam = 0;
 lpFileCtx    = apCont -> m_pFileCtx;
 lpMyCont     = ( _MPEGPSContainer* )calloc (  1, sizeof ( _MPEGPSContainer )  );

 apCont -> m_pCtx = lpMyCont;

 while (  i++ < 1024 && !FILE_EOF( lpFileCtx )  ) {

  int                lStartCode, lType, lLen;
  unsigned int       j, lID = SMS_CodecID_NULL;
  enum SMS_CodecType lCodecType = SMS_CodecTypeUnknown;

  lLen = _read_header ( apCont, &lStartCode, &lPTS, &lDTS );

  if ( !lLen ) break;

  for ( j = 0; j < apCont -> m_nStm; ++j )

   if (  ( int )apCont -> m_pStm[ j ] -> m_ID == lStartCode  ) goto next;

  lType = lpMyCont -> m_Type[ lStartCode & 0xFF ];

  if ( lType > 0 ) {

   switch ( lType ) {

    case STREAM_TYPE_VIDEO_MPEG1: if ( !lfVideo ) {
     lfVideo = 1;
     lCodecType = SMS_CodecTypeVideo;
     lID        = SMS_CodecID_MPEG1;
    } break;

    case STREAM_TYPE_VIDEO_MPEG2: if ( !lfVideo ) {
     lfVideo = 1;
     lCodecType = SMS_CodecTypeVideo;
     lID        = SMS_CodecID_MPEG2;
    } break;

    case STREAM_TYPE_AUDIO_MPEG1:
    case STREAM_TYPE_AUDIO_MPEG2:
     lCodecType = SMS_CodecTypeAudio;
     lID        = SMS_CodecID_MP3;
    break;

    case STREAM_TYPE_AUDIO_AC3:
     lCodecType = SMS_CodecTypeAudio;
     lID        = SMS_CodecID_AC3;
    break;

   }  /* end switch */

  } else if ( lStartCode >= 0x1E0 && lStartCode <= 0x1EF && !lfVideo ) {

   lCodecType = SMS_CodecTypeVideo;
   lID        = SMS_CodecID_MPEG2;

  } else if ( lStartCode >= 0x1C0 && lStartCode <= 0x1DF ) {

   lCodecType = SMS_CodecTypeAudio;
   lID        = SMS_CodecID_MP3;

  } else if ( lStartCode >= 0x080 && lStartCode <= 0x087 ) {

   lCodecType = SMS_CodecTypeAudio;
   lID        = SMS_CodecID_AC3;

  }  /* end if */

  if ( lCodecType != SMS_CodecTypeUnknown ) {

   SMS_Stream* lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream )  );

   if ( lpStm ) {

    lpStm -> m_pCodec = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );
    lpStm -> m_pCodec -> m_Type = lCodecType;
    lpStm -> m_pCodec -> m_ID   = lID;
    lpStm -> m_ID               = lStartCode;

    apCont -> m_pStm[ apCont -> m_nStm++ ] = lpStm;
    retVal = 1;

    SMSContainer_SetPTSInfo ( lpStm, 1, 90000 );

    if ( lCodecType == SMS_CodecTypeVideo ) {

     if ( !lfVideoParam ) lfVideoParam = _fill_video_parameters ( lpFileCtx, lpStm, lLen );

    } else lfAudio = _fill_audio_parameters ( lpFileCtx, lpStm, lLen );

   }  /* end if */

  }  /* end if */
next:
  File_Skip ( lpFileCtx, lLen );

  if (  apCont -> m_nStm < SMS_MAX_STREAMS && ( !lfAudio || !lfVideoParam )  )
   continue;
  else break;

 }  /* end while */

 if ( !retVal )
error:
  free ( lpMyCont );

 else {

  if ( !lfVideoParam || !lfAudio ) {

   apCont -> m_pCtx = NULL;

   SMS_DestroyContainer ( apCont, 0 );

   retVal = 0;
   goto error;

  }  /* end if */

  apCont -> m_pName    = "MPEG";
  apCont -> ReadPacket = _ReadPacket;
  apCont -> Seek       = _Seek;
  apCont -> m_Duration = 0x7FFFFFFFFFFFFFFFLL;
  apCont -> m_Flags   |= SMS_CONT_FLAGS_SEEKABLE;

  lpFileCtx -> Seek ( lpFileCtx, 0 );

  {  /* begin block */

   int      lnStm  = apCont -> m_nStm, lStmIdx = 0;
   uint64_t lSSPTS[ lnStm ];
   uint64_t lSEPTS[ lnStm ];

   for ( i = 0; i < lnStm; ++i ) lSSPTS[ i ] = lSEPTS[ i ] = SMS_NOPTS_VALUE;

   while ( lStmIdx < lnStm ) {
    _get_stm_pts ( apCont, lStmIdx, 0, lSSPTS );
    ++lStmIdx;
   }  /* end while */

   lStmIdx = 0;
   lpFileCtx -> Seek (
    lpFileCtx, lpFileCtx -> m_Size > 0x0000FFFF ?
               lpFileCtx -> m_Size - 0x0000FFFF : 0
   );

   while (  !FILE_EOF( lpFileCtx )  ) {
    _get_stm_pts ( apCont, lStmIdx, 1, lSEPTS );
    if ( ++lStmIdx == lnStm ) lStmIdx = 0;
   }  /* end while */

   lPTS = SMS_INT64( 0x7FFFFFFFFFFFFFFF );
   lDTS = SMS_INT64( 0x8000000000000000 );

   for ( i = 0; i < lnStm; ++i ) {
    if (  ( int64_t )lSSPTS[ i ] < ( int64_t )lPTS  ) lPTS = lSSPTS[ i ];
    if (  ( int64_t )lSEPTS[ i ] > ( int64_t )lDTS  ) lDTS = lSEPTS[ i ];
   }  /* end for */

   apCont -> m_Duration = ( int )SMS_Rescale ( lDTS - lPTS, SMS_TIME_BASE, 90000 );

  }  /* end block */

  lpFileCtx -> Seek ( lpFileCtx, 0 );

 }  /* end else */

 return retVal;

}  /* end SMS_GetContainerMPEG_PS */
