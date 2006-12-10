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
#include "SMS_ContainerAVI.h"
#include "SMS_FourCC.h"
#include "SMS_Integer.h"
#include "SMS_GUI.h"
#include "SMS_Locale.h"
#include "SMS_Config.h"

#include <malloc.h>

#define MYCONT( c ) (  ( _AVIContainer* )c -> m_pCtx  )
#define MYSTRM( s ) (  ( _AVIStream*    )s -> m_pCtx  )

#define AVIIF_INDEX        0x10
#define AVIF_ISINTERLEAVED 0x00000100

typedef struct _AVIdxEntry {

 uint32_t m_Flags;
 uint32_t m_Pos;
 uint32_t m_Len;
 uint32_t m_CumLen;

} _AVIdxEntry;

typedef struct _AVIStream {

 int64_t      m_StartTime;
 int64_t      m_Duration;
 _AVIdxEntry* m_pIdx;
 uint32_t     m_IdxAllocSize;
 uint32_t     m_nIdx;
 int32_t      m_Idx;
 int32_t      m_Rate;
 int32_t      m_Scale;
 int32_t      m_PTSWrapBits;
 int32_t      m_SampleSize;
 uint32_t     m_CumLen;
 uint32_t     m_FrameOffset;
 uint32_t     m_NewFrameOffset;

} _AVIStream;

typedef struct _AVIContainer {

 int64_t  m_StartTime;
 uint32_t m_BitRate;
 uint32_t m_RiffEnd;
 uint32_t m_MoviEnd;
 uint32_t m_MoviList;

} _AVIContainer;

static int _ProbeFile ( FileContext* apFileCtx, uint32_t* apRiffEnd ) {

 int      retVal;
 uint32_t lTag = File_GetUInt ( apFileCtx );

 if (  lTag == SMS_MKTAG( 'R', 'I', 'F', 'F' )  ) {

  *apRiffEnd  = File_GetUInt ( apFileCtx );
  *apRiffEnd += apFileCtx -> m_CurPos;
  lTag        = File_GetUInt ( apFileCtx );

  retVal = lTag == SMS_MKTAG( 'A', 'V', 'I', ' ' ) ||
           lTag == SMS_MKTAG( 'A', 'V', 'I', 'X' );

 } else retVal = 0;

 return retVal;

}  /* end _ProbeFile */

static void _DestroyStream ( SMS_Stream* apStm ) {

 if (  MYSTRM( apStm ) -> m_pIdx  ) free (  MYSTRM( apStm ) -> m_pIdx  );

}  /* end _DestroyStream */

static SMS_Stream* _NewStream ( SMS_Container* apCont ) {

 SMS_Stream* retVal = NULL;

 if ( apCont -> m_nStm < SMS_MAX_STREAMS ) {

  retVal = ( SMS_Stream* )calloc (  1, sizeof( SMS_Stream ) + sizeof ( _AVIStream )  );

  if ( retVal ) {

   retVal -> m_pCtx   = ( char* )retVal + sizeof( SMS_Stream );
   retVal -> m_pCodec = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );

   MYSTRM( retVal ) -> m_Idx       = apCont -> m_nStm;
   MYSTRM( retVal ) -> m_StartTime =
   MYSTRM( retVal ) -> m_Duration  =
   retVal -> m_CurDTS              = SMS_NOPTS_VALUE;

   retVal -> Destroy = _DestroyStream;

   apCont -> m_pStm[ apCont -> m_nStm++ ] = retVal;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _NewStream */

static int _Load_idx1 ( SMS_Container* apCtx, int aSize ) {

 int          i, lnIdx = aSize / 16;
 uint32_t     lIdx, lTag, lFlags, lPos, lLen, lPerc;
 _AVIStream*  lpStm;
 _AVIdxEntry* lpIndices;
 _AVIdxEntry* lpIndex;
 FileContext* lpFileCtx    = apCtx -> m_pFileCtx;
 int          lPrevIdx     = -1;
 int          lnSameIdx    =  0;
 int          lnMaxSameIdx = -1;

 if ( lnIdx <= 0 ) return 0;

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 64 );

 GUI_Progress ( STR_LOADING_INDICES.m_pStr, 0, 0 );

 lPerc = lnIdx / 100 + 1;

 for ( i = 0; i < lnIdx; ++i ) {

  lTag   = File_GetUInt ( lpFileCtx );
  lFlags = File_GetUInt ( lpFileCtx );
  lPos   = File_GetUInt ( lpFileCtx );
  lLen   = File_GetUInt ( lpFileCtx );

  lIdx  = (  ( lTag        & 0xFF  ) - '0'  ) * 10;
  lIdx += (  ( lTag >> 8 ) & 0xFF  ) - '0';

  if (  !( i % lPerc )  ) {

   float lPos = ( float )i / ( float ) lnIdx * 100.0F;

   GUI_Progress (  STR_LOADING_INDICES.m_pStr, ( unsigned int )( lPos + 0.5F ), 0  );

  }  /* end if */

  if ( lIdx >= apCtx -> m_nStm ) continue;

  if ( lIdx == lPrevIdx )

   ++lnSameIdx;

  else {

   if ( lnMaxSameIdx < lnSameIdx ) lnMaxSameIdx = lnSameIdx;

   lnSameIdx = 0;
   lPrevIdx  = lIdx;

  }  /* end else */

  lpStm = MYSTRM( apCtx -> m_pStm[ lIdx ] );

  lpIndices = SMS_Realloc (
   lpStm -> m_pIdx, &lpStm -> m_IdxAllocSize,
   ( lpStm -> m_nIdx + 1 ) * sizeof ( _AVIdxEntry )
  );

  if ( lpIndices ) {

   lpStm -> m_pIdx = lpIndices;

   lpIndex = &lpIndices[ lpStm -> m_nIdx++ ];
   lpIndex -> m_Flags  = lFlags;
   lpIndex -> m_Pos    = lPos;
   lpIndex -> m_Len    = lLen;
   lpIndex -> m_CumLen = lpStm -> m_CumLen;

   lpStm -> m_CumLen += lLen;

  }  /* end if */

 }  /* end for */

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 0 );

 return lnMaxSameIdx > 256 ? -1 : 1;

}  /* end _Load_idx1 */

static int _LoadIndex ( SMS_Container* apCont ) {

 uint32_t     lTag, lSize;
 uint32_t     lPos = apCont -> m_pFileCtx -> m_CurPos;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 int          retVal    = 0;

 lpFileCtx -> Seek (  lpFileCtx, MYCONT( apCont ) -> m_MoviEnd  );

 while ( 1 )

  if ( lpFileCtx -> m_CurPos < lpFileCtx -> m_Size ) {

   lTag  = File_GetUInt ( lpFileCtx );
   lSize = File_GetUInt ( lpFileCtx );

   if (   lTag == SMS_MKTAG( 'i', 'd', 'x', '1' ) && (  retVal = _Load_idx1 ( apCont, lSize )  )   ) {

    apCont -> m_Flags |= SMS_CONT_FLAGS_SEEKABLE;
    break;

   }  /* end if */

   lSize += ( lSize & 1 );
   lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize );

  } else break;

 lpFileCtx -> Seek ( lpFileCtx, lPos );

 return retVal;

}  /* end _LoadIndex */

static int _ReadHeader ( SMS_Container* apCtx ) {

 int                retVal;
 uint32_t           lTag, lSubTag, lSize, lnFrames, lLen, lStmIdx, lFlags = 0;
 int32_t            lFramePeriod = 0, lBitRate;
 int32_t            i, lCount, lScale, lRate;
 SMS_Stream*        lpStm;
 FileContext*       lpFileCtx  = apCtx -> m_pFileCtx;
 enum SMS_CodecType lCodecType = SMS_CodecTypeUnknown;

 retVal  =  1;
 lStmIdx = -1;

 while ( 1 )

  if ( lpFileCtx -> m_CurPos < lpFileCtx -> m_Size ) {

   lTag  = File_GetUInt ( lpFileCtx );
   lSize = File_GetUInt ( lpFileCtx );

   switch ( lTag ) {

    case SMS_MKTAG( 'L', 'I', 'S', 'T' ):

     lSubTag = File_GetUInt ( lpFileCtx );

     if (  lSubTag == SMS_MKTAG( 'm', 'o', 'v', 'i' )  ) {

      MYCONT( apCtx ) -> m_MoviList = lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos - 4 );
      MYCONT( apCtx ) -> m_MoviEnd  = MYCONT( apCtx ) -> m_MoviList + lSize;

      goto endOfHeader;

     }  /* end if */

    break;

    case SMS_MKTAG( 'a', 'v', 'i', 'h' ):  /* Main AVI header */

     lFramePeriod = File_GetUInt ( lpFileCtx );      /* dwMicroSecPerFrame */
     lBitRate     = File_GetUInt ( lpFileCtx ) * 8;  /* dwMaxBytesPerSec   */
                    File_GetUInt ( lpFileCtx );      /* dwReserved1        */
     lFlags       = File_GetUInt ( lpFileCtx );      /* dwFlags            */
                    File_GetUInt ( lpFileCtx );      /* dwTotalFrames      */
                    File_GetUInt ( lpFileCtx );      /* dwInitialFrames    */
     lCount       = File_GetUInt ( lpFileCtx );      /* dwStreams          */

     for ( i = 0; i < lCount; ++i )

      if (  !_NewStream ( apCtx )  ) goto error;

     lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 28 );

    break;

    case SMS_MKTAG( 's', 't', 'r', 'h' ):

     ++lStmIdx;

     lTag    = File_GetUInt ( lpFileCtx );  /* fccType    */
     lSubTag = File_GetUInt ( lpFileCtx );  /* fccHandler */

     switch ( lTag ) {

      case SMS_MKTAG( 'v', 'i', 'd', 's' ):

       lCodecType = SMS_CodecTypeVideo;

       if ( lStmIdx >= apCtx -> m_nStm )

        lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 8 );

       else {

        lpStm = apCtx -> m_pStm[ lStmIdx ];

        lpStm -> m_pCodec -> m_StmTag = lSubTag;
        lpStm -> m_Flags             |= SMS_STRM_FLAGS_VIDEO;

        File_GetUInt   ( lpFileCtx );  /* dwFlags         */
        File_GetUShort ( lpFileCtx );  /* wPriority       */
        File_GetUShort ( lpFileCtx );  /* wLanguage       */
        File_GetUInt   ( lpFileCtx );  /* dwInitialFrames */

        lScale = File_GetUInt ( lpFileCtx );  /* dwScale */
        lRate  = File_GetUInt ( lpFileCtx );  /* dwRate  */

        if ( !lScale || !lRate ) {

         if ( lFramePeriod ) {

          lRate  = SMS_TIME_BASE;
          lScale = lFramePeriod;

         } else {

          lRate  = 25;
          lScale =  1;

         }  /* end else */

        }  /* end if */

        MYSTRM( lpStm ) -> m_Rate  = lRate;
        MYSTRM( lpStm ) -> m_Scale = lScale;

        SMSContainer_SetPTSInfo ( lpStm, lScale, lRate );

        lpStm -> m_pCodec -> m_FrameRate     = lRate;
        lpStm -> m_pCodec -> m_FrameRateBase = lScale;

                   File_GetUInt ( lpFileCtx );  /* dwStart  */
        lnFrames = File_GetUInt ( lpFileCtx );  /* dwLength */

        MYSTRM( lpStm ) -> m_StartTime = SMS_INT64( 0 );
        MYSTRM( lpStm ) -> m_Duration  = SMS_Rescale (
         lnFrames, ( int64_t )lpStm -> m_pCodec -> m_FrameRateBase *
                   ( int64_t )SMS_TIME_BASE,
         lpStm -> m_pCodec -> m_FrameRate
        );
        lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 36 );

       }  /* end else */

      break;

      case SMS_MKTAG( 'a', 'u', 'd', 's' ): {

       lCodecType = SMS_CodecTypeAudio;

       if ( lStmIdx >= apCtx -> m_nStm )

        lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 8 );

       else {

        lpStm = apCtx -> m_pStm[ lStmIdx ];

        lpStm -> m_Flags |= SMS_STRM_FLAGS_AUDIO;

        File_GetUInt   ( lpFileCtx );  /* dwFlags         */
        File_GetUShort ( lpFileCtx );  /* wPriority       */
        File_GetUShort ( lpFileCtx );  /* wLanguage       */
        File_GetUInt   ( lpFileCtx );  /* dwInitialFrames */

        MYSTRM( lpStm ) -> m_Scale = File_GetUInt ( lpFileCtx );  /* dwScale */
        MYSTRM( lpStm ) -> m_Rate  = File_GetUInt ( lpFileCtx );  /* dwRate  */

        SMSContainer_SetPTSInfo (  lpStm, MYSTRM( lpStm ) -> m_Scale, MYSTRM( lpStm ) -> m_Rate  );

               File_GetUInt ( lpFileCtx );  /* dwStart  */
        lLen = File_GetUInt ( lpFileCtx );  /* dwLength */

        File_GetUInt ( lpFileCtx );  /* dwSuggestedBufferSize */
        File_GetUInt ( lpFileCtx );  /* dwQuality             */

        MYSTRM( lpStm ) -> m_SampleSize = File_GetUInt ( lpFileCtx );  /* dwSampleSize */
        MYSTRM( lpStm ) -> m_StartTime  = 0;

        if (  MYSTRM( lpStm ) -> m_Rate ) MYSTRM( lpStm ) -> m_Duration = ( int64_t )lLen * SMS_TIME_BASE / MYSTRM( lpStm ) -> m_Rate;

        lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 48 );

       }  /* end else */

      } break;

      case SMS_MKTAG( 't', 'x', 't', 's' ):

       lCodecType = SMS_CodecTypeUnknown;
       lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - 8 );

      break;

      default: goto error;

     }  /* end switch */

    break;

    case SMS_MKTAG( 's', 't', 'r', 'f' ):

    if ( lStmIdx >= apCtx -> m_nStm )

     lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize );

    else {

     lpStm = apCtx -> m_pStm[ lStmIdx ];

     switch ( lCodecType ) {

      case SMS_CodecTypeVideo:

       File_GetUInt ( lpFileCtx );

       lpStm -> m_pCodec -> m_Width  = File_GetUInt ( lpFileCtx );
       lpStm -> m_pCodec -> m_Height = File_GetUInt ( lpFileCtx );

       File_GetUShort ( lpFileCtx );

       lpStm -> m_pCodec -> m_BitsPerSample = File_GetUShort ( lpFileCtx );

       lSubTag = File_GetUInt ( lpFileCtx );

       File_GetUInt ( lpFileCtx );
       File_GetUInt ( lpFileCtx );
       File_GetUInt ( lpFileCtx );
       File_GetUInt ( lpFileCtx );
       File_GetUInt ( lpFileCtx );

       lLen = lSize - 40;

       lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lLen );

       if ( lLen & 1 ) File_GetByte ( lpFileCtx );

       lpStm -> m_pCodec -> m_Type = SMS_CodecTypeVideo;
       lpStm -> m_pCodec -> m_Tag  = lSubTag;
       lpStm -> m_pCodec -> m_ID   = SMS_CodecGetID ( SMS_CodecTypeVideo, lSubTag );

      break;

      case SMS_CodecTypeAudio: {

       int32_t lID = File_GetUShort ( lpFileCtx );

       lpStm -> m_pCodec -> m_Type          = SMS_CodecTypeAudio;
       lpStm -> m_pCodec -> m_Tag           = lID;
       lpStm -> m_pCodec -> m_Channels      = File_GetUShort ( lpFileCtx );
       lpStm -> m_pCodec -> m_SampleRate    = File_GetUInt ( lpFileCtx );
       lpStm -> m_pCodec -> m_BitRate       = File_GetUInt ( lpFileCtx ) * 8;
       File_GetUShort ( lpFileCtx );
       lpStm -> m_pCodec -> m_BitsPerSample = lSize == 14 ? 8 : File_GetUShort ( lpFileCtx );
       lpStm -> m_pCodec -> m_ID            = SMS_CodecGetID ( SMS_CodecTypeAudio, lID );

       if ( lSize > 16 ) {

        lLen = File_GetUShort ( lpFileCtx );

        if ( lLen > 0 ) lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lLen );

        if ( lSize - lLen - 18 > 0 ) lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize - lLen - 18 );

       }  /* end if */
       
      } break;

      default:

       lpStm -> m_pCodec -> m_Type = SMS_CodecTypeUnknown;
       lpStm -> m_pCodec -> m_ID   = SMS_CodecID_NULL;
       lpStm -> m_pCodec -> m_Tag  = 0;

       lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize );

     }  /* end switch */

    }  /* end else */

    break;

    case SMS_MKTAG( 'I', 'A', 'S', '1' ):
    case SMS_MKTAG( 'I', 'A', 'S', '2' ):
    case SMS_MKTAG( 'I', 'A', 'S', '3' ):
    case SMS_MKTAG( 'I', 'A', 'S', '4' ):
    case SMS_MKTAG( 'I', 'A', 'S', '5' ):
    case SMS_MKTAG( 'I', 'A', 'S', '6' ):
    case SMS_MKTAG( 'I', 'A', 'S', '7' ):
    case SMS_MKTAG( 'I', 'A', 'S', '8' ):
    case SMS_MKTAG( 'I', 'A', 'S', '9' ): {

     char* lpBuf = ( char* )malloc ( lSize += lSize & 1 );

     lpFileCtx -> Read ( lpFileCtx, lpBuf, lSize );

     lSubTag = (  ( char* )&lTag  )[ 3 ] - '1';
     lLen    = 0;

     for ( lSize = 0; lSize < apCtx -> m_nStm; ++lSize )

      if ( apCtx -> m_pStm[ lSize ] -> m_Flags & SMS_STRM_FLAGS_AUDIO ) {

       if ( !lSubTag ) {

        apCtx -> m_pStm[ lSize ] -> m_pName = lpBuf;
        break;

       } else --lSubTag;

      }  /* end if */

    } break;

    default:

     lSize += ( lSize & 1 );
     lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize );

   }  /* end switch */

  } else goto error;
endOfHeader:
 if ( lStmIdx != apCtx -> m_nStm - 1 )
error: retVal = 0;
 if ( retVal && _LoadIndex ( apCtx ) < 0  ) retVal = -1;

 return retVal;

}  /* end _ReadHeader */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 _AVIContainer* lpAVICont  = MYCONT( apCont );
 FileContext*   lpBuf      = apCont -> m_pFileCtx;
 int32_t        lData[ 8 ] = { -1, -1, -1, -1, -1, -1, -1, -1 };
 int32_t        i          = lpBuf -> m_CurPos;
 int32_t        lCount, lSize;
 _AVIStream*    lpAVIStm;

 while ( lpBuf -> m_CurPos < lpBuf -> m_Size ) {

  if ( lpBuf -> m_CurPos >= lpAVICont -> m_MoviEnd ) {

   uint32_t lTag[ 2 ], lSize;

   File_Skip ( lpBuf, lpAVICont -> m_RiffEnd - lpBuf -> m_CurPos );

   if (  !_ProbeFile ( lpBuf, &lpAVICont -> m_RiffEnd )  ) return -1;

   lTag[ 0 ] = File_GetUInt ( lpBuf );
   lSize     = File_GetUInt ( lpBuf );
   lTag[ 1 ] = File_GetUInt ( lpBuf );

   if (  lTag[ 0 ] == SMS_MKTAG( 'L', 'I', 'S', 'T' ) &&
         lTag[ 1 ] == SMS_MKTAG( 'm', 'o', 'v', 'i' )
   )

    lpAVICont -> m_MoviEnd = lpBuf -> m_CurPos + lSize - 4; 

   else return -1;

  }  /* end if */

  lData[ 0 ] = lData[ 1 ];
  lData[ 1 ] = lData[ 2 ];
  lData[ 2 ] = lData[ 3 ];
  lData[ 3 ] = lData[ 4 ];
  lData[ 4 ] = lData[ 5 ];
  lData[ 5 ] = lData[ 6 ];
  lData[ 6 ] = lData[ 7 ];
  lData[ 7 ] = File_GetByte ( lpBuf );

  lSize  = lData[ 4 ] + ( lData[ 5 ] <<  8 ) +
                        ( lData[ 6 ] << 16 ) +
                        ( lData[ 7 ] << 24 );
  lCount = ( lData[ 2 ] - '0' ) * 10 + ( lData[ 3 ] - '0' );

  if ( lData[ 2 ] >= '0' && lData[ 2 ] <= '9' &&
       lData[ 3 ] >= '0' && lData[ 3 ] <= '9' &&
       lData[ 0 ] == 'i' && lData[ 1 ] == 'x' &&
       lCount < ( int32_t )apCont -> m_nStm   &&
       i + lSize <= ( int32_t )lpAVICont -> m_MoviEnd
  ) File_Skip ( lpBuf, lSize );
            
  lCount = ( lData[ 0 ] - '0' ) * 10 + ( lData[ 1 ] - '0' );

  if ( lData[ 0 ] >= '0' && lData[ 0 ] <= '9' &&
       lData[ 1 ] >= '0' && lData[ 1 ] <= '9' &&
       (  ( lData[ 2 ] == 'd' && lData[ 3 ] == 'c' ) || 
	      ( lData[ 2 ] == 'w' && lData[ 3 ] == 'b' ) || 
          ( lData[ 2 ] == 'd' && lData[ 3 ] == 'b' ) ||
          ( lData[ 2 ] == '_' && lData[ 3 ] == '_' )
       )                                        &&
       lCount    <  ( int32_t )apCont -> m_nStm &&
       i + lSize <= ( int32_t )lpAVICont -> m_MoviEnd
  ) {

   SMS_Stream* lpStm = apCont -> m_pStm[ lCount ];

   if ( lpStm -> m_pPktBuf ) {

    SMS_AVPacket* lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lSize );

    if ( !lpPkt ) return 0;

    lpBuf -> Read ( lpBuf, lpPkt -> m_pData, lSize );

    if ( lSize & 1 ) {

     File_GetByte ( lpBuf );
     ++lSize;

    }  /* end if */

    lpAVIStm = MYSTRM( lpStm );

    lpPkt -> m_DTS = lpAVIStm -> m_FrameOffset;

    if ( lpAVIStm -> m_SampleSize ) lpPkt -> m_DTS /= lpAVIStm -> m_SampleSize;

    lpPkt -> m_StmIdx = lCount;

    if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {

     if (  lpAVIStm -> m_FrameOffset < lpAVIStm -> m_nIdx &&
           lpAVIStm -> m_pIdx[ lpAVIStm -> m_FrameOffset ].m_Flags & AVIIF_INDEX
     ) lpPkt -> m_Flags |= SMS_PKT_FLAG_KEY;

    } else lpPkt -> m_Flags |= SMS_PKT_FLAG_KEY;

    if ( lpAVIStm -> m_SampleSize )
     lpAVIStm -> m_FrameOffset += lpPkt -> m_Size;
    else ++lpAVIStm -> m_FrameOffset;

    SMSContainer_CalcPktFields ( lpStm, lpPkt );

    *apIdx = lCount;

    return lSize;

   } else File_Skip ( lpBuf, lSize );

  }  /* end else */

 }  /* end while */

 return -1;

}  /* end _ReadPacket */

static int _HasTimings ( SMS_Container* apCont ) {

 int         retVal = 0;
 uint32_t    i;
 SMS_Stream* lpStm;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  lpStm = apCont -> m_pStm[ i ];

  if (  MYSTRM( lpStm ) -> m_StartTime != SMS_NOPTS_VALUE &&
        MYSTRM( lpStm ) -> m_Duration  != SMS_NOPTS_VALUE
  ) {

   retVal = 1;
   break;

  }  /* end if */

 }  /* end for */

 return retVal;

}  /* end _HasTimings */

static void _UpdateStmTimings ( SMS_Container* apCont ) {

 int64_t     lStartTime, lEndTime, lTmp;
 uint32_t    i;
 SMS_Stream* lpStm;

 lStartTime = SMS_MAXINT64;
 lEndTime   = SMS_MININT64;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  lpStm = apCont -> m_pStm[ i ];

  if (  MYSTRM( lpStm ) -> m_StartTime != SMS_NOPTS_VALUE  ) {

   if (  MYSTRM( lpStm ) -> m_StartTime < lStartTime  ) lStartTime = MYSTRM( lpStm ) -> m_StartTime;

   if (  MYSTRM( lpStm ) -> m_Duration != SMS_NOPTS_VALUE  ) {

    lTmp = MYSTRM( lpStm ) -> m_StartTime + MYSTRM( lpStm ) -> m_Duration;

    if ( lTmp > lEndTime ) lEndTime = lTmp;

   }  /* end if */

  }  /* end if */

 }  /* end for */

 if ( lStartTime != SMS_MAXINT64 ) {

  MYCONT( apCont ) -> m_StartTime = lStartTime;

  if ( lEndTime != SMS_MAXINT64 ) {

   apCont -> m_Duration = lEndTime - lStartTime;

   if ( apCont -> m_pFileCtx -> m_Size > 0 )

    MYCONT( apCont ) -> m_BitRate = ( uint32_t )(  ( float )apCont -> m_pFileCtx -> m_Size * 8.0F * ( float )SMS_TIME_BASE / ( float )apCont -> m_Duration  );

  }  /* end if */

 }  /* end if */

}  /* end _UpdateStmTimings */

static void _FillAllStmTimings ( SMS_Container* apCont ) {

 uint32_t    i;
 SMS_Stream* lpStm;

 _UpdateStmTimings ( apCont );

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  lpStm = apCont -> m_pStm[ i ];

  if (  MYSTRM( lpStm ) -> m_StartTime == SMS_NOPTS_VALUE ) {

   MYSTRM( lpStm ) -> m_StartTime = MYCONT( apCont ) -> m_StartTime;
   MYSTRM( lpStm ) -> m_Duration  = apCont -> m_Duration;

  }  /* end if */

 }  /* end for */

}  /* end _FillAllStmTimings */

static void _EstimateTimingsFromBitRate ( SMS_Container* apCont ) {

 int64_t     lDuration;
 uint32_t    i, lBitRate;
 SMS_Stream* lpStm;

 if (  MYCONT( apCont ) -> m_BitRate == 0  ) {

  lBitRate = 0;

  for ( i = 0; i < apCont -> m_nStm; ++i ) {

   lpStm     = apCont -> m_pStm[ i ];
   lBitRate += lpStm -> m_pCodec -> m_BitRate;

  }  /* end for */

  MYCONT( apCont ) -> m_BitRate = lBitRate;

 }  /* end if */

 if (  apCont -> m_Duration == SMS_NOPTS_VALUE && 
       MYCONT( apCont ) -> m_BitRate  != 0     && 
       apCont -> m_pFileCtx -> m_Size  > 0
 )  {

  lDuration = ( int64_t )(   (  8.0F * ( float )SMS_TIME_BASE * ( float )apCont -> m_pFileCtx -> m_Size  ) / ( float )MYCONT( apCont ) -> m_BitRate   );

  for ( i = 0; i < apCont -> m_nStm; ++i ) {

   lpStm = apCont -> m_pStm[ i ];

   if (  MYSTRM( lpStm ) -> m_StartTime == SMS_NOPTS_VALUE ||
         MYSTRM( lpStm ) -> m_Duration  == SMS_NOPTS_VALUE
   ) {

    MYSTRM( lpStm ) -> m_StartTime = 0;
    MYSTRM( lpStm ) -> m_Duration  = lDuration;

   }  /* end if */

  }  /* end for */

 }  /* end if */

}  /* end _EstimateTimingsFromBitRate */

static void _EstimateTimings ( SMS_Container* apCont ) {

 if (  _HasTimings ( apCont )  )

  _FillAllStmTimings ( apCont );

 else _EstimateTimingsFromBitRate ( apCont );

 _UpdateStmTimings ( apCont );

}  /* end _EstimateTimings */

static void _CalcFrameRate ( SMS_Container* apCont ) {

 uint32_t          i;
 SMS_Stream*       lpStm;
 SMS_CodecContext* lpCodecCtx;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  lpStm      = apCont -> m_pStm[ i ];
  lpCodecCtx = lpStm -> m_pCodec;

  if ( lpCodecCtx -> m_Type == SMS_CodecTypeVideo ) {

   lpStm -> m_RealFrameRate     = lpCodecCtx -> m_FrameRate;
   lpStm -> m_RealFrameRateBase = lpCodecCtx -> m_FrameRateBase;

  }  /* end if */

 }  /* end for */

 _EstimateTimings ( apCont );

}  /* end _CalcFrameRate */

static int _LocateFrame ( _AVIdxEntry* apEnt, int anEnt, int aPos ) {

 int lA, lB, lM, lPos;

 lA = 0;
 lB = anEnt - 1;

 while ( lA <= lB ) {

  lM   = ( lA + lB ) >> 1;
  lPos = apEnt[ lM ].m_Pos;

  if ( lPos == aPos )

   goto found;

  else if ( lPos > aPos )

   lB = lM - 1;

  else lA = lM + 1;

 }  /* end while */

 lM = lA;

 if ( lM > 0 ) --lM;
found:
 return lM;

}  /* end _LocateFrame */

static int _Seek ( SMS_Container* apCont, int anIdx, int aDir, uint32_t aPos ) {

 int         i;
 _AVIStream* lpStm = MYSTRM( apCont -> m_pStm[ anIdx ] );
 uint32_t    lEnd  = aDir > 0 ? lpStm -> m_nIdx : 0;
 uint32_t    lPos;

 if ( aPos >= lpStm -> m_nIdx ) return 0;

 while (  aPos != lEnd && !( lpStm -> m_pIdx[ aPos ].m_Flags & AVIIF_INDEX )  ) aPos += aDir;

 if ( aPos == lpStm -> m_nIdx ) return 0;

 lpStm -> m_NewFrameOffset = aPos;
 lPos                      = lpStm -> m_pIdx[ aPos ].m_Pos;

 for (  i = 0; i < ( int )apCont -> m_nStm; ++i  ) {

  uint32_t j;

  if ( i != anIdx ) {

   lpStm = MYSTRM( apCont -> m_pStm[ i ] );

   if ( lpStm -> m_nIdx <= 0 ) return 0;

   j = _LocateFrame ( lpStm -> m_pIdx, lpStm -> m_nIdx, lPos );

   if (  ( j + 1 ) < lpStm -> m_nIdx ) ++j;

   if ( lpStm -> m_SampleSize == 0 )

    lpStm -> m_NewFrameOffset = j;

   else lpStm -> m_NewFrameOffset = lpStm -> m_pIdx[ j ].m_CumLen;

  }  /* end if */

 }  /* end for */

 for (  i = 0; i < ( int )apCont -> m_nStm; ++i  ) {

  lpStm = MYSTRM( apCont -> m_pStm[ i ] );
  lpStm -> m_FrameOffset = lpStm -> m_NewFrameOffset;

 }  /* end for */

 lPos += MYCONT( apCont ) -> m_MoviList;
 apCont -> m_pFileCtx -> Seek ( apCont -> m_pFileCtx, lPos );

 return 1;

}  /* end _Seek */

int SMS_GetContainerAVI ( SMS_Container* apCont ) {

 int      retVal = 0;
 uint32_t lRiffEnd;

 if (  ( int )apCont -> m_pFileCtx < 0  ) return retVal;

 if (  _ProbeFile ( apCont -> m_pFileCtx , &lRiffEnd )  ) {

  apCont -> m_pCtx = ( _AVIContainer* )calloc (  1, sizeof ( _AVIContainer )  );

  if (   (  retVal = _ReadHeader ( apCont )  )   ) {

   MYCONT( apCont ) -> m_RiffEnd = lRiffEnd;

   apCont -> m_pName    = g_pAVIStr;
   apCont -> ReadPacket = _ReadPacket;
   apCont -> Seek       = _Seek;

   _CalcFrameRate ( apCont );

   g_Config.m_PlayerFlags &= ~SMS_PF_AUDHP;

  } else free ( apCont -> m_pCtx );

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerAVI */
