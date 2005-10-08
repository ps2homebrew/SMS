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

#include <malloc.h>
#include <limits.h>

#define MYCONT( c ) (  ( _AVIContainer* )c -> m_pCtx  )
#define MYSTRM( s ) (  ( _AVIStream*    )s -> m_pCtx  )

#define AVIIF_INDEX 0x10

typedef struct _AVIdxEntry {

 uint32_t m_Flags;
 uint32_t m_Pos;
 uint32_t m_Len;
 uint32_t m_CumLen;

} _AVIdxEntry;

typedef struct _AVIStream {

 int64_t      m_StartTime;
 int64_t      m_Duration;
 int64_t      m_CurDTS;
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
 SMS_Rational m_TimeBase;

} _AVIStream;

typedef struct _AVIContainer {

 int64_t  m_StartTime;
 int64_t  m_Duration;
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

 if ( apCont -> m_nStm < 32 ) {

  retVal = ( SMS_Stream* )calloc (  1, sizeof( SMS_Stream ) + sizeof ( _AVIStream )  );

  if ( retVal ) {

   retVal -> m_pCtx = ( char* )retVal + sizeof( SMS_Stream );

   MYSTRM( retVal ) -> m_Idx       = apCont -> m_nStm;
   MYSTRM( retVal ) -> m_StartTime =
   MYSTRM( retVal ) -> m_Duration  =
   MYSTRM( retVal ) -> m_CurDTS    = SMS_NOPTS_VALUE;

   retVal -> Destroy = _DestroyStream;

   apCont -> m_pStm[ apCont -> m_nStm++ ] = retVal;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _NewStream */

static void _AllocPacket ( SMS_AVPacket* apPkt, int aSize ) {

 void* lpData = SMS_Realloc ( apPkt -> m_pData, &apPkt -> m_AllocSize, aSize + 8 );

 apPkt -> m_PTS      = SMS_NOPTS_VALUE;
 apPkt -> m_DTS      = SMS_NOPTS_VALUE;
 apPkt -> m_StmIdx   = 0;
 apPkt -> m_Flags    = 0;
 apPkt -> m_Duration = 0;
 apPkt -> m_pData    = lpData; 
 apPkt -> m_Size     = aSize;

}  /* end _AllocPacket */

static void _SetPTSInfo ( _AVIStream* apStm, int aPTSWrapBits, int aPTSNum, int aPTSDen ) {

 apStm -> m_PTSWrapBits    = aPTSWrapBits;
 apStm -> m_TimeBase.m_Num = aPTSNum;
 apStm -> m_TimeBase.m_Den = aPTSDen;

}  /* end _AVISetPTSInfo */

static int64_t _Rescale ( int64_t anA, int64_t aB, int64_t aC ) {

 SMS_Integer lAi, lCi;
    
 if ( anA < 0 ) return -_Rescale ( -anA, aB, aC );
    
 if ( aB <= INT_MAX && aC <= INT_MAX )

  return anA <= INT_MAX ? ( anA * aB + aC / 2 ) / aC
                        : anA / aC * aB + ( anA % aC * aB + aC / 2 ) / aC;
    
 lAi = SMS_Integer_mul_i (  SMS_Integer_int2i ( anA ), SMS_Integer_int2i ( aB )  );
 lCi = SMS_Integer_int2i ( aC );
 lAi = SMS_Integer_add_i (  lAi, SMS_Integer_shr_i ( lCi, 1 )  );
    
 return SMS_Integer_i2int (  SMS_Integer_div_i ( lAi, lCi )  );

}  /* end _Rescale */

static int _Load_idx1 ( SMS_Container* apCtx, int aSize ) {

 int          i, lnIdx = aSize / 16;
 uint32_t     lIdx, lTag, lFlags, lPos, lLen;
 _AVIStream*  lpStm;
 _AVIdxEntry* lpIndices;
 _AVIdxEntry* lpIndex;

 if ( lnIdx <= 0 ) return 0;

 apCtx -> m_pFileCtx -> Stream (
  apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos, 64
 );

 for ( i = 0; i < lnIdx; ++i ) {

  lTag   = File_GetUInt ( apCtx -> m_pFileCtx );
  lFlags = File_GetUInt ( apCtx -> m_pFileCtx );
  lPos   = File_GetUInt ( apCtx -> m_pFileCtx );
  lLen   = File_GetUInt ( apCtx -> m_pFileCtx );

  lIdx  = (  ( lTag        & 0xFF  ) - '0'  ) * 10;
  lIdx += (  ( lTag >> 8 ) & 0xFF  ) - '0';

  if ( lIdx >= apCtx -> m_nStm ) continue;

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

 apCtx -> m_pFileCtx -> Stream (
  apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos, 0
 );

 return 1;

}  /* end _Load_idx1 */

static void _LoadIndex ( SMS_Container* apCont ) {

 uint32_t     lTag, lSize;
 uint32_t     lPos = apCont -> m_pFileCtx -> m_CurPos;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;

 lpFileCtx -> Seek (  lpFileCtx, MYCONT( apCont ) -> m_MoviEnd  );

 while ( 1 )

  if ( lpFileCtx-> m_CurPos < lpFileCtx -> m_Size ) {

   lTag  = File_GetUInt ( lpFileCtx );
   lSize = File_GetUInt ( lpFileCtx );

   if (  lTag == SMS_MKTAG( 'i', 'd', 'x', '1' ) &&
         _Load_idx1 ( apCont, lSize )
   ) break;

   lSize += ( lSize & 1 );
   lpFileCtx -> Seek ( lpFileCtx, lpFileCtx -> m_CurPos + lSize );

  } else break;

 lpFileCtx -> Seek ( lpFileCtx, lPos );

}  /* end _LoadIndex */

static int _ReadHeader ( SMS_Container* apCtx ) {

 int                retVal;
 uint32_t           lTag, lSubTag, lSize, lnFrames, lLen, lStmIdx;
 int32_t            lFramePeriod = 0, lBitRate;
 int32_t            i, lCount, lScale, lRate;
 SMS_Stream*        lpStm;
 enum SMS_CodecType lCodecType = SMS_CodecTypeUnknown;
 int32_t            lAudioIdx = -1;
 int32_t            lVideoIdx = -1;

 retVal  =  1;
 lStmIdx = -1;

 while ( 1 )

  if ( apCtx -> m_pFileCtx -> m_CurPos < apCtx -> m_pFileCtx -> m_Size ) {

   lTag  = File_GetUInt ( apCtx -> m_pFileCtx );
   lSize = File_GetUInt ( apCtx -> m_pFileCtx );

   switch ( lTag ) {

    case SMS_MKTAG( 'L', 'I', 'S', 'T' ):

     lSubTag = File_GetUInt ( apCtx -> m_pFileCtx );

     if (  lSubTag == SMS_MKTAG( 'm', 'o', 'v', 'i' )  ) {

      MYCONT( apCtx ) -> m_MoviList = apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos - 4 );
      MYCONT( apCtx ) -> m_MoviEnd  = MYCONT( apCtx ) -> m_MoviList + lSize;

      goto endOfHeader;

     }  /* end if */

    break;

    case SMS_MKTAG( 'a', 'v', 'i', 'h' ):  /* Main AVI header */

     lFramePeriod = File_GetUInt ( apCtx -> m_pFileCtx );      /* dwMicroSecPerFrame */
     lBitRate     = File_GetUInt ( apCtx -> m_pFileCtx ) * 8;  /* dwMaxBytesPerSec   */

     apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + 16 );

     lCount = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStreams */

     for ( i = 0; i < lCount; ++i )

      if (  !_NewStream ( apCtx )  ) goto error;

     apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 28 );

    break;

    case SMS_MKTAG( 's', 't', 'r', 'h' ):

     ++lStmIdx;

     lTag    = File_GetUInt ( apCtx -> m_pFileCtx );  /* fccType    */
     lSubTag = File_GetUInt ( apCtx -> m_pFileCtx );  /* fccHandler */

     switch ( lTag ) {

      case SMS_MKTAG( 'v', 'i', 'd', 's' ):

       lCodecType = SMS_CodecTypeVideo;

       if ( lStmIdx >= apCtx -> m_nStm )

        apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 8 );

       else {

        lpStm = apCtx -> m_pStm[ lVideoIdx = lStmIdx ];
        lpStm -> m_Codec.m_StmTag = lSubTag;

        File_GetUInt   ( apCtx -> m_pFileCtx );  /* dwFlags         */
        File_GetUShort ( apCtx -> m_pFileCtx );  /* wPriority       */
        File_GetUShort ( apCtx -> m_pFileCtx );  /* wLanguage       */
        File_GetUInt   ( apCtx -> m_pFileCtx );  /* dwInitialFrames */

        lScale = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwScale */
        lRate  = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwRate  */

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

        _SetPTSInfo (  MYSTRM( lpStm ), 64, lScale, lRate  );

        lpStm -> m_Codec.m_FrameRate     = lRate;
        lpStm -> m_Codec.m_FrameRateBase = lScale;

                   File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStart  */
        lnFrames = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwLength */

        MYSTRM( lpStm ) -> m_StartTime = SMS_INT64( 0 );
        MYSTRM( lpStm ) -> m_Duration  = _Rescale (
         lnFrames, ( int64_t )lpStm -> m_Codec.m_FrameRateBase *
                   ( int64_t )SMS_TIME_BASE,
         lpStm -> m_Codec.m_FrameRate
        );
        apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 36 );

       }  /* end else */

      break;

      case SMS_MKTAG( 'a', 'u', 'd', 's' ): {

       lCodecType = SMS_CodecTypeAudio;

       if ( lStmIdx >= apCtx -> m_nStm )

        apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 8 );

       else {

        lpStm = apCtx -> m_pStm[ lAudioIdx = lStmIdx ];

        File_GetUInt   ( apCtx -> m_pFileCtx );  /* dwFlags         */
        File_GetUShort ( apCtx -> m_pFileCtx );  /* wPriority       */
        File_GetUShort ( apCtx -> m_pFileCtx );  /* wLanguage       */
        File_GetUInt   ( apCtx -> m_pFileCtx );  /* dwInitialFrames */

        MYSTRM( lpStm ) -> m_Scale = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwScale */
        MYSTRM( lpStm ) -> m_Rate  = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwRate  */

        _SetPTSInfo (  MYSTRM( lpStm ), 64, MYSTRM( lpStm ) -> m_Scale, MYSTRM( lpStm ) -> m_Rate );

               File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStart  */
        lLen = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwLength */

        File_GetUInt ( apCtx -> m_pFileCtx );  /* dwSuggestedBufferSize */
        File_GetUInt ( apCtx -> m_pFileCtx );  /* dwQuality             */

        MYSTRM( lpStm ) -> m_SampleSize = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwSampleSize */
        MYSTRM( lpStm ) -> m_StartTime  = 0;

        if (  MYSTRM( lpStm ) -> m_Rate ) MYSTRM( lpStm ) -> m_Duration = ( int64_t )lLen * SMS_TIME_BASE / MYSTRM( lpStm ) -> m_Rate;

        apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 48 );

       }  /* end else */

      } break;

      case SMS_MKTAG( 't', 'x', 't', 's' ):

       apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - 8 );

      break;

      default: goto error;

     }  /* end switch */

    break;

    case SMS_MKTAG( 's', 't', 'r', 'f' ):

    if ( lStmIdx >= apCtx -> m_nStm )

     apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize );

    else {

     lpStm = apCtx -> m_pStm[ lStmIdx ];

     switch ( lCodecType ) {

      case SMS_CodecTypeVideo:

       File_GetUInt ( apCtx -> m_pFileCtx );

       lpStm -> m_Codec.m_Width  = File_GetUInt ( apCtx -> m_pFileCtx );
       lpStm -> m_Codec.m_Height = File_GetUInt ( apCtx -> m_pFileCtx );

       File_GetUShort ( apCtx -> m_pFileCtx );

       lpStm -> m_Codec.m_BitsPerSample = File_GetUShort ( apCtx -> m_pFileCtx );

       lSubTag = File_GetUInt ( apCtx -> m_pFileCtx );

       File_GetUInt ( apCtx -> m_pFileCtx );
       File_GetUInt ( apCtx -> m_pFileCtx );
       File_GetUInt ( apCtx -> m_pFileCtx );
       File_GetUInt ( apCtx -> m_pFileCtx );
       File_GetUInt ( apCtx -> m_pFileCtx );

       lLen = lSize - 40;

       apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lLen );

       if ( lLen & 1 ) File_GetByte ( apCtx -> m_pFileCtx );

       lpStm -> m_Codec.m_Type = SMS_CodecTypeVideo;
       lpStm -> m_Codec.m_Tag  = lSubTag;
       lpStm -> m_Codec.m_ID   = SMS_CodecGetID ( SMS_CodecTypeVideo, lSubTag );

      break;

      case SMS_CodecTypeAudio: {

       int32_t lID = File_GetUShort ( apCtx -> m_pFileCtx );

       lpStm -> m_Codec.m_Type          = SMS_CodecTypeAudio;
       lpStm -> m_Codec.m_Tag           = lID;
       lpStm -> m_Codec.m_Channels      = File_GetUShort ( apCtx -> m_pFileCtx );
       lpStm -> m_Codec.m_SampleRate    = File_GetUInt ( apCtx -> m_pFileCtx );
       lpStm -> m_Codec.m_BitRate       = File_GetUInt ( apCtx -> m_pFileCtx ) * 8;
       lpStm -> m_Codec.m_BlockAlign    = File_GetUShort ( apCtx -> m_pFileCtx );
       lpStm -> m_Codec.m_BitsPerSample = lSize == 14 ? 8 : File_GetUShort ( apCtx -> m_pFileCtx );
       lpStm -> m_Codec.m_ID            = SMS_CodecGetID ( SMS_CodecTypeAudio, lID );

       if ( lSize > 16 ) {

        lLen = File_GetUShort ( apCtx -> m_pFileCtx );

        if ( lLen > 0 ) apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lLen );

        if ( lSize - lLen - 18 > 0 ) apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize - lLen - 18 );

       }  /* end if */
       
      } break;

      default:

       lpStm -> m_Codec.m_Type = SMS_CodecTypeUnknown;
       lpStm -> m_Codec.m_ID   = SMS_CodecID_NULL;
       lpStm -> m_Codec.m_Tag  = 0;

       apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize );

     }  /* end switch */

    }  /* end else */

    break;

    default:

     lSize += ( lSize & 1 );
     apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize );

   }  /* end switch */

  } else goto error;
endOfHeader:
 if ( lStmIdx != apCtx -> m_nStm - 1 )
error: retVal = 0;
 if ( retVal ) _LoadIndex ( apCtx );

 return retVal;

}  /* end _ReadHeader */

static void _CalcPktFields ( SMS_Stream* apStm, SMS_AVPacket* apPkt ) {

 int lNum, lDen;

 if ( apStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

  lNum = apStm -> m_Codec.m_FrameRateBase;
  lDen = apStm -> m_Codec.m_FrameRate;

 } else if ( apStm -> m_Codec.m_Type == SMS_CodecTypeAudio ) {

  lNum = apStm -> m_Codec.m_FrameSize <= 1 ? ( apPkt -> m_Size * 8 * apStm -> m_Codec.m_SampleRate ) / apStm -> m_Codec.m_BitRate
                                           : apStm -> m_Codec.m_FrameSize;
  lDen = apStm -> m_Codec.m_SampleRate;

 } else return;

 if ( lDen && lNum )

  apPkt -> m_Duration = ( int )_Rescale (
   1, lNum * ( int64_t )MYSTRM( apStm ) -> m_TimeBase.m_Den, lDen * ( int64_t )MYSTRM( apStm ) -> m_TimeBase.m_Num
  );

 if ( apPkt -> m_PTS == SMS_NOPTS_VALUE ) {

  if ( apPkt -> m_DTS == SMS_NOPTS_VALUE ) {

   apPkt -> m_PTS = MYSTRM( apStm ) -> m_CurDTS;
   apPkt -> m_DTS = MYSTRM( apStm ) -> m_CurDTS;

  } else {

   MYSTRM( apStm ) -> m_CurDTS = apPkt -> m_DTS;
   apPkt -> m_PTS              = apPkt -> m_DTS;

  }  /* end else */

 } else {

  MYSTRM( apStm ) -> m_CurDTS = apPkt -> m_PTS;
  apPkt -> m_DTS              = apPkt -> m_PTS;

 }  /* end else */

 MYSTRM( apStm ) -> m_CurDTS += apPkt -> m_Duration;

 if ( apPkt -> m_PTS != SMS_NOPTS_VALUE )

  apPkt -> m_PTS = _Rescale ( apPkt -> m_PTS, SMS_TIME_BASE * ( int64_t )MYSTRM( apStm ) -> m_TimeBase.m_Num, MYSTRM( apStm ) -> m_TimeBase.m_Den );

 if ( apPkt -> m_DTS != SMS_NOPTS_VALUE )

  apPkt -> m_DTS = _Rescale ( apPkt -> m_DTS, SMS_TIME_BASE * ( int64_t )MYSTRM( apStm ) -> m_TimeBase.m_Num, MYSTRM( apStm ) -> m_TimeBase.m_Den );

 apPkt -> m_Duration = ( int )_Rescale ( apPkt -> m_Duration, SMS_TIME_BASE * ( int64_t )MYSTRM( apStm ) -> m_TimeBase.m_Num, MYSTRM( apStm ) -> m_TimeBase.m_Den );

}  /* end _CalcPktFields */

static int _ReadPacket ( SMS_AVPacket* apPkt ) {

 SMS_Container* lpCont     = apPkt  -> m_pCtx;
 _AVIContainer* lpAVICont  = MYCONT( lpCont );
 FileContext*   lpBuf      = lpCont -> m_pFileCtx;
 int32_t        lData[ 8 ] = { -1, -1, -1, -1, -1, -1, -1, -1 };
 int32_t        i          = lpBuf -> m_CurPos;
 int32_t        lCount, lSize;
 SMS_Stream*    lpStm;
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
       lCount < ( int32_t )lpCont -> m_nStm   &&
       i + lSize <= ( int32_t )lpAVICont -> m_MoviEnd
  ) lpBuf -> Seek ( lpBuf, lpBuf -> m_CurPos + lSize );
            
  lCount = ( lData[ 0 ] - '0' ) * 10 + ( lData[ 1 ] - '0' );

  if ( lData[ 0 ] >= '0' && lData[ 0 ] <= '9' &&
       lData[ 1 ] >= '0' && lData[ 1 ] <= '9' &&
       (  ( lData[ 2 ] == 'd' && lData[ 3 ] == 'c' ) || 
	      ( lData[ 2 ] == 'w' && lData[ 3 ] == 'b' ) || 
          ( lData[ 2 ] == 'd' && lData[ 3 ] == 'b' ) ||
          ( lData[ 2 ] == '_' && lData[ 3 ] == '_' )
       )                                       &&
       lCount    <  ( int32_t )lpCont -> m_nStm &&
       i + lSize <= ( int32_t )lpAVICont -> m_MoviEnd
  ) {
        
   _AllocPacket ( apPkt, lSize );

   if ( apPkt -> m_pData == NULL ) return 0;

   lpBuf -> Read ( lpBuf, apPkt -> m_pData, lSize );

   if ( lSize & 1 ) {

    File_GetByte ( lpBuf );
    ++lSize;

   }  /* end if */

   lpStm    = lpCont -> m_pStm[ lCount ];
   lpAVIStm = MYSTRM( lpStm );

   apPkt -> m_DTS = lpAVIStm -> m_FrameOffset;

   if ( lpStm -> m_SampleRate ) apPkt -> m_DTS /= lpStm -> m_SampleRate;

   apPkt -> m_StmIdx = lCount;

   if ( lpStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

    if (  lpAVIStm -> m_FrameOffset < lpAVIStm -> m_nIdx &&
          lpAVIStm -> m_pIdx[ lpAVIStm -> m_FrameOffset ].m_Flags & AVIIF_INDEX
    ) apPkt -> m_Flags |= SMS_PKT_FLAG_KEY;

   } else apPkt -> m_Flags |= SMS_PKT_FLAG_KEY; 

   if ( lpAVIStm -> m_SampleSize )

    lpAVIStm -> m_FrameOffset += apPkt -> m_Size;

   else ++lpAVIStm -> m_FrameOffset;

   _CalcPktFields ( lpStm, apPkt );

   return lSize;

  }  /* end if */

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

   MYCONT( apCont ) -> m_Duration = lEndTime - lStartTime;

   if ( apCont -> m_pFileCtx -> m_Size > 0 )

    MYCONT( apCont ) -> m_BitRate = ( uint32_t )(  ( float )apCont -> m_pFileCtx -> m_Size * 8.0F * ( float )SMS_TIME_BASE / ( float )MYCONT( apCont ) -> m_Duration  );

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
   MYSTRM( lpStm ) -> m_Duration  = MYCONT( apCont ) -> m_Duration;

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
   lBitRate += lpStm -> m_Codec.m_BitRate;

  }  /* end for */

  MYCONT( apCont ) -> m_BitRate = lBitRate;

 }  /* end if */

 if (  MYCONT( apCont ) -> m_Duration == SMS_NOPTS_VALUE && 
       MYCONT( apCont ) -> m_BitRate  != 0               && 
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

 uint32_t    i;
 SMS_Stream* lpStm;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  lpStm = apCont -> m_pStm[ i ];

  if ( lpStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

   lpStm -> m_RealFrameRate     = lpStm -> m_Codec.m_FrameRate;
   lpStm -> m_RealFrameRateBase = lpStm -> m_Codec.m_FrameRateBase;

  }  /* end if */

 }  /* end for */

 _EstimateTimings ( apCont );

}  /* end _CalcFrameRate */

int SMS_GetContainerAVI ( SMS_Container* apCont ) {

 int      retVal = 0;
 uint32_t lRiffEnd;

 if (  _ProbeFile ( apCont -> m_pFileCtx , &lRiffEnd )  ) {

  apCont -> m_pCtx = ( _AVIContainer* )calloc (  1, sizeof ( _AVIContainer )  );

  if (  _ReadHeader ( apCont )  ) {

   MYCONT( apCont ) -> m_RiffEnd = lRiffEnd;

   apCont -> m_pName    = "AVI";
   apCont -> ReadPacket = _ReadPacket;

   _CalcFrameRate ( apCont );

   retVal = 1;

  } else free ( apCont -> m_pCtx );

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerAVI */
