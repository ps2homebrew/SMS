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
#include "SMS_AVI.h"
#include "SMS_FourCC.h"
#include "SMS_Integer.h"
#include "FileContext.h"

#include <malloc.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#ifdef _WIN32
# include <errno.h>
#endif  /* _WIN32 */

static void _SMS_AVIDestroyPacket ( SMS_AVIPacket* apPkt ) {

 if ( apPkt != 0 ) {

  free ( apPkt -> m_pData );
  free ( apPkt );

 }  /* end if */

}  /* end _SMS_AVIDestroyPacket */

static void _SMS_AVIDestroyContext ( SMS_AVIContext* apCtx ) {

 uint32_t i;

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  if ( apCtx -> m_pStm[ i ] -> m_pIdx ) free ( apCtx -> m_pStm[ i ] -> m_pIdx );

  if ( apCtx -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

   apCtx -> m_pStm[ i ] -> m_Codec.m_pCodec -> Destroy ( &apCtx -> m_pStm[ i ] -> m_Codec );
   free ( apCtx -> m_pStm[ i ] -> m_Codec.m_pCodec );

   SMS_CodecClose ( &apCtx -> m_pStm[ i ] -> m_Codec );

  }  /* end if */

  free ( apCtx -> m_pStm[ i ] );

 }  /* end for */

 apCtx -> m_pFileCtx -> Destroy ( apCtx -> m_pFileCtx );

 free ( apCtx );

}  /* end _SMS_AVIDestroyContext */

static SMS_AVIStream* _SMS_AVINewStream ( SMS_AVIContext* apCtx, int32_t anID ) {

 SMS_AVIStream* retVal = 0;

 if ( apCtx -> m_nStm < 2 ) {

  retVal = ( SMS_AVIStream* )calloc (  1, sizeof( SMS_AVIStream )  );

  if ( retVal ) {

   retVal -> m_Idx       = apCtx -> m_nStm;
   retVal -> m_ID        = anID;
   retVal -> m_StartTime =
   retVal -> m_Duration  =
   retVal -> m_CurDTS    = SMS_NOPTS_VALUE;

   apCtx -> m_pStm[ apCtx -> m_nStm++ ] = retVal;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _SMS_AVINewStream */

static void _SMS_AVISetPTSInfo ( SMS_AVIStream* apStm, int aPTSWrapBits, int aPTSNum, int aPTSDen ) {

 apStm -> m_PTSWrapBits    = aPTSWrapBits;
 apStm -> m_TimeBase.m_Num = aPTSNum;
 apStm -> m_TimeBase.m_Den = aPTSDen;

}  /* end _SMS_AVISetPTSInfo */

static void _SMS_AVINewPacket ( SMS_AVIPacket* apPkt, int aSize ) {

 void* lpData = SMS_Realloc ( apPkt -> m_pData, &apPkt -> m_AllocSize, aSize + 8 );

 apPkt -> m_PTS      = SMS_NOPTS_VALUE;
 apPkt -> m_DTS      = SMS_NOPTS_VALUE;
 apPkt -> m_StmIdx   = 0;
 apPkt -> m_Flags    = 0;
 apPkt -> m_Duration = 0;
 apPkt -> m_pData    = lpData; 
 apPkt -> m_Size     = aSize;

}  /* end _SMS_AVINewPacket */

static int _SMS_AVIRead_idx1 ( SMS_AVIContext* apCtx, int aSize ) {

 int i,          lnIdx = aSize / 16;
 uint32_t        lIdx, lTag, lFlags, lPos, lLen;
 SMS_AVIStream*  lpStm;
 SMS_AVIdxEntry* lpIndices;
 SMS_AVIdxEntry* lpIndex;

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

  lpStm = apCtx -> m_pStm[ lIdx ];

  lpIndices = SMS_Realloc (
   lpStm -> m_pIdx, &lpStm -> m_IdxAllocSize,
   ( lpStm -> m_nIdx + 1 ) * sizeof ( SMS_AVIdxEntry )
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

}  /* end _SMS_AVIRead_idx1 */

static void _SMS_AVILoadIndex ( SMS_AVIContext* apCtx ) {

 uint32_t lTag, lSize;
 uint32_t lPos = apCtx -> m_pFileCtx -> m_CurPos;

 apCtx -> m_pFileCtx -> Seek (
  apCtx -> m_pFileCtx, apCtx -> m_MoviEnd
 );

 while ( 1 )

  if ( apCtx -> m_pFileCtx -> m_CurPos < apCtx -> m_pFileCtx -> m_Size ) {

   lTag  = File_GetUInt ( apCtx -> m_pFileCtx );
   lSize = File_GetUInt ( apCtx -> m_pFileCtx );

   if (  lTag == SMS_MKTAG( 'i', 'd', 'x', '1' ) &&
         _SMS_AVIRead_idx1 ( apCtx, lSize )
   ) break;

   lSize += ( lSize & 1 );
   apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + lSize );

  } else break;

 apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, lPos );

}  /* end _SMS_AVILoadIndex */

static int _SMS_AVIHasTimings ( SMS_AVIContext* apCtx ) {

 int            retVal = 0;
 uint32_t       i;
 SMS_AVIStream* lpStm;

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  lpStm = apCtx -> m_pStm[ i ];

  if ( lpStm -> m_StartTime != SMS_NOPTS_VALUE &&
       lpStm -> m_Duration  != SMS_NOPTS_VALUE
  ) {

   retVal = 1;
   break;

  }  /* end if */

 }  /* end for */

 return retVal;

}  /* end _SMS_AVIHasTimings */

static void _SMS_AVIEstimateTimingsFromBitRate ( SMS_AVIContext* apCtx ) {

 int64_t        lDuration;
 uint32_t       i, lBitRate;
 SMS_AVIStream* lpStm;

 if ( apCtx -> m_BitRate == 0 ) {

  lBitRate = 0;

  for ( i = 0; i < apCtx -> m_nStm; ++i ) {

   lpStm     = apCtx -> m_pStm[ i ];
   lBitRate += lpStm -> m_Codec.m_BitRate;

  }  /* end for */

  apCtx -> m_BitRate = lBitRate;

 }  /* end if */

 if ( apCtx -> m_Duration == SMS_NOPTS_VALUE && 
      apCtx -> m_BitRate  != 0               && 
      apCtx -> m_FileSize  > 0
 )  {

  lDuration = ( int64_t )(   (  8.0F * ( float )SMS_TIME_BASE * ( float )apCtx -> m_FileSize  ) / ( float )apCtx -> m_BitRate   );

  for ( i = 0; i < apCtx -> m_nStm; ++i ) {

   lpStm = apCtx -> m_pStm[ i ];

   if (  lpStm -> m_StartTime == SMS_NOPTS_VALUE ||
         lpStm -> m_Duration  == SMS_NOPTS_VALUE
   ) {

    lpStm -> m_StartTime = 0;
    lpStm -> m_Duration  = lDuration;

   }  /* end if */

  }  /* end for */

 }  /* end if */

}  /* end _SMS_AVIEstimateTimingsFromBitRate */

static void _SMS_AVIUpdateStmTimings ( SMS_AVIContext* apCtx ) {

 int64_t        lStartTime, lEndTime, lTmp;
 uint32_t       i;
 SMS_AVIStream* lpStm;

 lStartTime = SMS_MAXINT64;
 lEndTime   = SMS_MININT64;

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  lpStm = apCtx -> m_pStm[ i ];

  if ( lpStm -> m_StartTime != SMS_NOPTS_VALUE ) {

   if ( lpStm -> m_StartTime < lStartTime ) lStartTime = lpStm -> m_StartTime;

   if ( lpStm -> m_Duration != SMS_NOPTS_VALUE ) {

    lTmp = lpStm -> m_StartTime + lpStm -> m_Duration;

    if ( lTmp > lEndTime ) lEndTime = lTmp;

   }  /* end if */

  }  /* end if */

 }  /* end for */

 if ( lStartTime != SMS_MAXINT64 ) {

  apCtx -> m_StartTime = lStartTime;

  if ( lEndTime != SMS_MAXINT64 ) {

   apCtx -> m_Duration = lEndTime - lStartTime;

   if ( apCtx -> m_FileSize > 0 )

    apCtx -> m_BitRate = ( uint32_t )(  ( float )apCtx -> m_FileSize * 8.0F * ( float )SMS_TIME_BASE / ( float )apCtx -> m_Duration  );

  }  /* end if */

 }  /* end if */

}  /* end _SMS_AVIUpdateStmTimings */

static int64_t _SMS_AVIRescale ( int64_t anA, int64_t aB, int64_t aC ) {

 SMS_Integer lAi, lCi;
    
 if ( anA < 0 ) return -_SMS_AVIRescale ( -anA, aB, aC );
    
 if ( aB <= INT_MAX && aC <= INT_MAX )

  return anA <= INT_MAX ? ( anA * aB + aC / 2 ) / aC
                        : anA / aC * aB + ( anA % aC * aB + aC / 2 ) / aC;
    
 lAi = SMS_Integer_mul_i (  SMS_Integer_int2i ( anA ), SMS_Integer_int2i ( aB )  );
 lCi = SMS_Integer_int2i ( aC );
 lAi = SMS_Integer_add_i (  lAi, SMS_Integer_shr_i ( lCi, 1 )  );
    
 return SMS_Integer_i2int (  SMS_Integer_div_i ( lAi, lCi )  );

}  /* end _SMS_AVIRescale */

static void _SMS_AVIFillAllStmTimings ( SMS_AVIContext* apCtx ) {

 uint32_t       i;
 SMS_AVIStream* lpStm;

 _SMS_AVIUpdateStmTimings ( apCtx );

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  lpStm = apCtx -> m_pStm[ i ];

  if ( lpStm -> m_StartTime == SMS_NOPTS_VALUE ) {

   lpStm -> m_StartTime = apCtx -> m_StartTime;
   lpStm -> m_Duration  = apCtx -> m_Duration;

  }  /* end if */

 }  /* end for */

}  /* end _SMS_AVIFillAllStmTimings */

static void _SMS_AVIEstimateTimings ( SMS_AVIContext* apCtx ) {

 apCtx -> m_FileSize = apCtx -> m_pFileCtx -> m_Size;

 if (  _SMS_AVIHasTimings ( apCtx )  )

  _SMS_AVIFillAllStmTimings ( apCtx );

 else _SMS_AVIEstimateTimingsFromBitRate ( apCtx );

 _SMS_AVIUpdateStmTimings ( apCtx );

}  /* end _SMS_AVIEstimateTimings */

static void _SMS_AVICalcPktFields ( SMS_AVIStream* apStm, SMS_AVIPacket* apPkt ) {

 int lNum, lDen;

 if ( apStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

  lNum = apStm -> m_Codec.m_FrameRateBase;
  lDen = apStm -> m_Codec.m_FrameRate;

  if ( lDen && lNum ) {

   apPkt -> m_Duration = ( int )_SMS_AVIRescale (
    1, lNum * ( int64_t )apStm -> m_TimeBase.m_Den, lDen * ( int64_t )apStm -> m_TimeBase.m_Num
   );

   if ( apPkt -> m_PTS == SMS_NOPTS_VALUE ) {

    if ( apPkt -> m_DTS == SMS_NOPTS_VALUE ) {

     apPkt -> m_PTS = apStm -> m_CurDTS;
     apPkt -> m_DTS = apStm -> m_CurDTS;

    } else {

     apStm -> m_CurDTS = apPkt -> m_DTS;
     apPkt -> m_PTS    = apPkt -> m_DTS;

    }  /* end else */

   } else {

    apStm -> m_CurDTS = apPkt -> m_PTS;
    apPkt -> m_DTS    = apPkt -> m_PTS;

   }  /* end else */

   apStm -> m_CurDTS += apPkt -> m_Duration;

   if ( apPkt -> m_PTS != SMS_NOPTS_VALUE )

    apPkt -> m_PTS = _SMS_AVIRescale ( apPkt -> m_PTS, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den );

   if ( apPkt -> m_DTS != SMS_NOPTS_VALUE )

    apPkt -> m_DTS = _SMS_AVIRescale ( apPkt -> m_DTS, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den );

   apPkt -> m_Duration = ( int )_SMS_AVIRescale ( apPkt -> m_Duration, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den );

  }  /* end if */

 }  /* end if */

}  /* end _SMS_AVICalcPktFields */

SMS_AVIPacket* SMS_AVINewPacket ( SMS_AVIContext* apCtx ) {

 SMS_AVIPacket* retVal = calloc (  1, sizeof ( SMS_AVIPacket )  );

 retVal -> m_pCtx  = apCtx;
 retVal -> Destroy = _SMS_AVIDestroyPacket;

 return retVal;

}  /* end SMS_AVINewPacket */

SMS_AVIContext* SMS_AVINewContext ( FileContext* apBuf ) {

 SMS_AVIContext* retVal = ( SMS_AVIContext* )calloc(  1, sizeof ( SMS_AVIContext )  );

 if ( retVal ) {

  retVal -> m_pFileCtx = apBuf;
  retVal -> Destroy    = _SMS_AVIDestroyContext;

 }  /* end if */

 return retVal;

}  /* end SMS_AVINewContext */

int SMS_AVIProbeFile ( SMS_AVIContext* apCtx ) {

 int      retVal;
 uint32_t lTag = File_GetUInt ( apCtx -> m_pFileCtx );

 if (  lTag == SMS_MKTAG( 'R', 'I', 'F', 'F' )  ) {

  apCtx -> m_RiffEnd  = File_GetUInt ( apCtx -> m_pFileCtx );
  apCtx -> m_RiffEnd += apCtx -> m_pFileCtx -> m_CurPos;
  lTag                = File_GetUInt ( apCtx -> m_pFileCtx );

  retVal = lTag == SMS_MKTAG( 'A', 'V', 'I', ' ' ) ||
           lTag == SMS_MKTAG( 'A', 'V', 'I', 'X' );

 } else retVal = 0;

 return retVal;

}  /* end SMS_AVIProbeFile */

int SMS_AVIReadHeader ( SMS_AVIContext* apCtx ) {

 int                retVal;
 uint32_t           lTag, lSubTag, lSize, lnFrames, lLen, lStmIdx;
 int32_t            lFramePeriod = 0, lBitRate;
 int32_t            i, lCount, lScale, lRate;
 SMS_AVIStream*     lpStm;
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

      apCtx -> m_MoviList = apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos - 4 );
      apCtx -> m_MoviEnd  = apCtx -> m_MoviList + lSize;

      goto endOfHeader;

     }  /* end if */

    break;

    case SMS_MKTAG( 'a', 'v', 'i', 'h' ):  /* Main AVI header */

     lFramePeriod = File_GetUInt ( apCtx -> m_pFileCtx );      /* dwMicroSecPerFrame */
     lBitRate     = File_GetUInt ( apCtx -> m_pFileCtx ) * 8;  /* dwMaxBytesPerSec   */

     apCtx -> m_pFileCtx -> Seek ( apCtx -> m_pFileCtx, apCtx -> m_pFileCtx -> m_CurPos + 16 );

     lCount = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStreams */

     for ( i = 0; i < lCount; ++i )

      if (  !_SMS_AVINewStream ( apCtx, i )  ) goto error;

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

        lpStm -> m_Rate  = lRate;
        lpStm -> m_Scale = lScale;

        _SMS_AVISetPTSInfo ( lpStm, 64, lScale, lRate );

        lpStm -> m_Codec.m_FrameRate     = lRate;
        lpStm -> m_Codec.m_FrameRateBase = lScale;

                   File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStart  */
        lnFrames = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwLength */

        lpStm -> m_StartTime = SMS_INT64( 0 );
        lpStm -> m_Duration  = _SMS_AVIRescale (
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

        lpStm -> m_Scale = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwScale */
        lpStm -> m_Rate  = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwRate  */

        _SMS_AVISetPTSInfo ( lpStm, 64, lpStm -> m_Scale, lpStm -> m_Rate );

        lpStm -> m_Start = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwStart  */
        lLen             = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwLength */

        File_GetUInt ( apCtx -> m_pFileCtx );  /* dwSuggestedBufferSize */
        File_GetUInt ( apCtx -> m_pFileCtx );  /* dwQuality             */

        lpStm -> m_SampleSize = File_GetUInt ( apCtx -> m_pFileCtx );  /* dwSampleSize */
        lpStm -> m_StartTime  = 0;

        if ( lpStm -> m_Rate ) lpStm -> m_Duration = ( int64_t )lLen * SMS_TIME_BASE / lpStm -> m_Rate;

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
 if ( retVal ) _SMS_AVILoadIndex ( apCtx );

 return retVal;

}  /* end SMS_AVIReadHeader */

void SMS_AVICalcFrameRate ( SMS_AVIContext* apCtx ) {

 uint32_t       i;
 SMS_AVIStream* lpStm;

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  lpStm = apCtx -> m_pStm[ i ];

  if ( lpStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

   lpStm -> m_RealFrameRate     = lpStm -> m_Codec.m_FrameRate;
   lpStm -> m_RealFrameRateBase = lpStm -> m_Codec.m_FrameRateBase;

  }  /* end if */

 }  /* end for */

 _SMS_AVIEstimateTimings ( apCtx );

}  /* end SMS_AVICalcFrameRate */

int SMS_AVIReadPacket ( SMS_AVIPacket* apPkt ) {

 SMS_AVIContext* lpCtx      = apPkt -> m_pCtx;
 FileContext*    lpBuf      = lpCtx -> m_pFileCtx;
 int32_t         lData[ 8 ] = { -1, -1, -1, -1, -1, -1, -1, -1 };
 int32_t         i          = lpBuf -> m_CurPos;
 int32_t         lCount, lSize;
 SMS_AVIStream*  lpStm;

 while ( lpBuf -> m_CurPos < lpBuf -> m_Size ) {

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
       lCount < ( int32_t )lpCtx -> m_nStm    &&
       i + lSize <= ( int32_t )lpCtx -> m_MoviEnd
  ) lpBuf -> Seek ( lpBuf, lpBuf -> m_CurPos + lSize );
            
  lCount = ( lData[ 0 ] - '0' ) * 10 + ( lData[ 1 ] - '0' );

  if ( lData[ 0 ] >= '0' && lData[ 0 ] <= '9' &&
       lData[ 1 ] >= '0' && lData[ 1 ] <= '9' &&
       (  ( lData[ 2 ] == 'd' && lData[ 3 ] == 'c' ) || 
	      ( lData[ 2 ] == 'w' && lData[ 3 ] == 'b' ) || 
          ( lData[ 2 ] == 'd' && lData[ 3 ] == 'b' ) ||
          ( lData[ 2 ] == '_' && lData[ 3 ] == '_' )
       )                                       &&
       lCount    <  ( int32_t )lpCtx -> m_nStm &&
       i + lSize <= ( int32_t )lpCtx -> m_MoviEnd
  ) {
        
   _SMS_AVINewPacket ( apPkt, lSize );

   if ( apPkt -> m_pData == NULL ) return 0;

   lpBuf -> Read ( lpBuf, apPkt -> m_pData, lSize );

   if ( lSize & 1 ) {

    File_GetByte ( lpBuf );
    ++lSize;

   }  /* end if */

   lpStm = lpCtx -> m_pStm[ lCount ];

   apPkt -> m_DTS = lpStm -> m_FrameOffset;

   if ( lpStm -> m_SampleRate ) apPkt -> m_DTS /= lpStm -> m_SampleRate;

   apPkt -> m_StmIdx = lCount;

   if ( lpStm -> m_Codec.m_Type == SMS_CodecTypeVideo ) {

    if ( lpStm -> m_FrameOffset < lpStm -> m_nIdx ) apPkt -> m_Flags |= SMS_PKT_FLAG_KEY;

   } else apPkt -> m_Flags |= SMS_PKT_FLAG_KEY; 

   if ( lpStm -> m_SampleSize )

    lpStm -> m_FrameOffset += apPkt -> m_Size;

   else ++lpStm -> m_FrameOffset;

   _SMS_AVICalcPktFields ( lpStm, apPkt );

   return lSize;

  }  /* end if */

 }  /* end while */

 return -1;

}  /* end SMS_AVIReadPacket */

void SMS_AVIPrintInfo ( SMS_AVIContext* apCtx ) {

 uint32_t i;

 printf ( "Duration: " );

 if ( apCtx -> m_Duration != SMS_NOPTS_VALUE ) {

  int lH, lM, lS, lMS;

  lS   = ( int )( apCtx -> m_Duration / SMS_TIME_BASE );
  lMS  = ( int )( apCtx -> m_Duration % SMS_TIME_BASE );
  lM   = lS / 60;
  lS  %= 60;
  lH   = lM / 60;
  lM  %= 60;
  printf (  "%02d:%02d:%02d.%01d", lH, lM, lS,  ( 10 * lMS ) / SMS_TIME_BASE  );

 } else printf ( "N/A" );

 printf ( ", bitrate: " );

 if ( apCtx -> m_BitRate )

  printf ( "%d kb/s", apCtx -> m_BitRate / 1000 );

 else printf ( "N/A" );

 printf ( "\n" );

 for ( i = 0; i < apCtx -> m_nStm; ++i ) {

  SMS_AVIStream* lpStm = apCtx -> m_pStm [ i ];

  if ( lpStm -> m_Codec.m_Type == SMS_CodecTypeVideo )

   printf ( " Stream %d: video %c%c%c%c", i, (  ( char* )&lpStm -> m_Codec.m_Tag  )[ 0 ],
                                             (  ( char* )&lpStm -> m_Codec.m_Tag  )[ 1 ],
                                             (  ( char* )&lpStm -> m_Codec.m_Tag  )[ 2 ],
                                             (  ( char* )&lpStm -> m_Codec.m_Tag  )[ 3 ]
   );

  else printf ( " Stream %d: audio 0x%0X", i, lpStm -> m_Codec.m_Tag  );

  printf ( "\n" );

 }  /* end if */

}  /* end SMS_AVIPrintInfo */
