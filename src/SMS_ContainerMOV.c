/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
#               2008 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
# 
*/
#include "SMS.h"
#include "SMS_ContainerMOV.h"
#include "SMS_FourCC.h"
#include "SMS_Locale.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

typedef struct MOVIndex {
 uint32_t m_Flags;
 uint32_t m_DTS;
 uint32_t m_Pos;
 uint32_t m_Size;
} MOVIndex;

typedef struct {
 int m_Count;
 int m_Duration;
} MOVStts;

typedef struct {
 int m_First;
 int m_Count;
 int m_ID;
} MOVStsc;

typedef struct {
 unsigned int m_Type;
 int64_t      m_Offset;
 int64_t      m_Size;
} MOVAtom;

typedef struct {
 int64_t m_Offset;
 int64_t m_Size;
} MOVMDat;

typedef struct MOVStream {
 int          m_nIdx;
 MOVIndex*    m_pIdx;
 uint32_t     m_IdxAllocSize;
 int          m_NextChunk;
 unsigned int m_nChunks;
 int64_t*     m_pChunkOffsets;
 unsigned int m_nStts;
 MOVStts*     m_pStts;
 unsigned int m_nCtts;
 MOVStts*     m_pCtts;
 unsigned int m_nSample2Chunk;
 MOVStsc*     m_pSample2Chunk;
 unsigned int m_Sample2CTimeIdx;
 int          m_Sample2CTimeSample;
 unsigned int m_SampleSize;
 unsigned int m_nSamples;
 int*         m_pSampleSize;
 unsigned int m_nKeyFrames;
 int*         m_pKeyFrames;
 int          m_TimeScale;
 int          m_TimeRate;
 unsigned int m_CurSample;
 unsigned int m_BytesPerFrame;
 unsigned int m_SamplesPerFrame;
} MOVStream;

struct MOVContext;

typedef int ( *mov_parse_func ) ( struct MOVContext*, FileContext*, MOVAtom* );

typedef struct MOVParseEntry {
 uint32_t       m_Type;
 mov_parse_func m_Func;
} MOVParseEntry;

typedef struct MOVContext {
 SMS_Container*       m_pBase;
 int                  m_TimeScale;
 int64_t              m_Duration;
 int64_t              m_MDATOffset;
 int                  m_nMDAT;
 MOVMDat*             m_pMDAT;
 const MOVParseEntry* m_pParseTbl;
 char                 m_fMOOV;
 char                 m_fMDAT;
 char                 m_fISOM;
} MOVContext;

static uint32_t _read_int_be ( FileContext* apFileCtx ) {
 uint32_t retVal = File_GetUInt ( apFileCtx );
 __asm__ __volatile__(
  "dsll32   %0, %0, 0\n\t"
  "dsrl32   %1, %0, 0\n\t"
  : "=r"( retVal ) : "r"( retVal )
 );
 return SMS_bswap32 ( retVal );
}  /* end _read_int_be */

static uint64_t _read_long_be ( FileContext* apFileCtx ) {
 uint64_t lHigh  = SMS_bswap32 (  File_GetUInt ( apFileCtx )  );
 uint64_t lLow   = SMS_bswap32 (  File_GetUInt ( apFileCtx )  );
 return ( lHigh << 32 ) | lLow;
}  /* end _read_long_be */

static void _free ( void* apPtr ) {
 if ( apPtr ) free ( apPtr );
}  /* end _free */

static int _mov_read_dflt ( MOVContext*, FileContext*, MOVAtom* );

static void _DestroyStream ( SMS_Stream* apStm ) {
 MOVStream* lpStm = ( MOVStream* )apStm -> m_pCtx;
 if ( lpStm -> m_pIdx  ) free ( lpStm -> m_pIdx  );
 if ( lpStm -> m_pCtts ) free ( lpStm -> m_pCtts );
}  /* end _DestroyStream */

static int _mov_read_ftyp ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 uint32_t lType = File_GetUInt ( apFileCtx );
 if (  lType != SMS_MKTAG( 'q', 't', ' ', ' ' )  ) apCtx -> m_fISOM = 1;
 File_GetUInt ( apFileCtx );
 File_Skip (  apFileCtx, ( uint32_t )( apAtom -> m_Size - 8 )  );
 return 0;
}  /* end _mov_read_ftyp */

static int _mov_read_moov ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom) {
 _mov_read_dflt ( apCtx, apFileCtx, apAtom );
 apCtx -> m_fMOOV = 1;
 if ( apCtx -> m_fMDAT ) return 1;
 return 0;
}  /* end _mov_read_moov */

static int _mov_read_mvhd ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 int lVer = File_GetByte ( apFileCtx );
 File_Skip (  apFileCtx, lVer == 1 ? 19 : 11 );
 apCtx -> m_TimeScale = _read_int_be ( apFileCtx );
 apCtx -> m_Duration  = lVer == 1 ? _read_long_be ( apFileCtx ) : _read_int_be ( apFileCtx );
 File_Skip ( apFileCtx, 80 );
 return 0;
}  /* end _mov_read_mvhd */

static int _mov_read_trak ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm;
 MOVStream*  lpMyStm;
 lpStm             = ( SMS_Stream*       )calloc (  1, sizeof ( SMS_Stream ) + sizeof ( MOVStream )  );
 lpStm -> m_pCodec = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );
 lpMyStm           = ( MOVStream* )(  ( char* )lpStm + sizeof( SMS_Stream )  );
 lpStm -> m_pCtx             = lpMyStm;
 lpStm -> m_pCodec -> m_Type = SMS_CodecTypeUnknown;
 lpStm -> Destroy            = _DestroyStream;
 apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm++ ] = lpStm;
 return _mov_read_dflt ( apCtx, apFileCtx, apAtom );
}  /* end _mov_read_trak */

static int _mov_read_tkhd ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 int         lVer  = File_GetByte ( apFileCtx );
 File_Skip (  apFileCtx, lVer == 1 ? 19 : 11 );
 lpStm -> m_ID = _read_int_be ( apFileCtx );
 File_Skip (  apFileCtx, lVer == 1 ? 72 : 68 );
 return 0;
}  /* end _mov_read_tkhd */

static int _mov_read_mdhd ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm   = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 MOVStream*  lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
 int         lVer    = File_GetByte ( apFileCtx );
 int         lLang;
 if ( lVer > 1 ) return 1;
 File_Skip ( apFileCtx, lVer == 1 ? 19 : 11 );
 lpMyStm -> m_TimeScale = _read_int_be ( apFileCtx );
 lpStm -> m_Duration    = lVer == 1 ? _read_long_be ( apFileCtx ) : _read_int_be ( apFileCtx );
 lLang                  = File_GetShortBE ( apFileCtx );
 File_Skip ( apFileCtx, 2 );
 return 0;
}  /* end _mov_read_mdhd */

static int _mov_read_hdlr ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 uint32_t    lType;
 uint32_t    lSubType;
 File_Skip ( apFileCtx, 4 );
 lType    = File_GetUInt ( apFileCtx );
 lSubType = File_GetUInt ( apFileCtx );
 if ( !lType ) apCtx -> m_fISOM = 1;
 if (  lSubType == SMS_MKTAG( 'v', 'i', 'd', 'e' )  )
  lpStm ->m_pCodec -> m_Type = SMS_CodecTypeVideo;
 else if (  lSubType == SMS_MKTAG( 's', 'o', 'u', 'n' )  )
  lpStm ->m_pCodec -> m_Type = SMS_CodecTypeAudio;
 else if (  lSubType == SMS_MKTAG( 'm', '1', 'a', ' ' )  ) {
  lpStm ->m_pCodec -> m_Type = SMS_CodecTypeAudio;
  lpStm ->m_pCodec -> m_ID   = SMS_CodecID_MP2;
 }  /* end if */
 File_Skip ( apFileCtx, 12 );
 if ( apAtom -> m_Size <= 24 ) return 0;
 File_Skip (   apFileCtx, ( uint32_t )(  apAtom -> m_Size - ( apFileCtx -> m_CurPos - apAtom -> m_Offset )  )   );
 return 0;
}  /* end _mov_read_hdlr */

static int _mov_read_hdlr_m4a ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  uint32_t    lType;
  uint32_t    lSubType;
  File_Skip ( apFileCtx, 4 );
  lType    = File_GetUInt ( apFileCtx );
  lSubType = File_GetUInt ( apFileCtx );
  if ( !lType ) apCtx -> m_fISOM = 1;
  if (  lSubType == SMS_MKTAG( 's', 'o', 'u', 'n' )  )
   lpStm -> m_pCodec -> m_Type = SMS_CodecTypeAudio;
  else if ( lSubType == SMS_MKTAG( 'v', 'i', 'd', 'e' ) ||
            lSubType == SMS_MKTAG( 'm', '1', 'a', ' ' )
  ) {
   free ( lpStm -> m_pCodec );
   free ( lpStm );
   apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ] = NULL;
  }  /* end else */
  File_Skip ( apFileCtx, 12 );
  if ( apAtom -> m_Size <= 24 ) return 0;
  File_Skip (   apFileCtx, ( uint32_t )(  apAtom -> m_Size - ( apFileCtx -> m_CurPos - apAtom -> m_Offset )  )   );
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_hdlr_m4a */

static int _mov_read_stsd ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  SMS_CodecContext* lpCodecCtx = lpStm -> m_pCodec;
  MOVStream*        lpMyStm    = ( MOVStream* )lpStm -> m_pCtx;
  int               lnEntries;
  uint32_t          lFmt;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  while ( lnEntries-- ) {
   MOVAtom     lAtom     = { 0, 0, 0 };
   uint32_t    lStartPos = apFileCtx -> m_CurPos;
   int         lSize     = _read_int_be ( apFileCtx );
   lFmt = File_GetUInt ( apFileCtx );
   File_Skip ( apFileCtx, 8 );
   if ( lpCodecCtx -> m_Tag ) {
    File_Skip (  apFileCtx, lSize - ( apFileCtx -> m_CurPos - lStartPos )  );
    continue;
   }  /* end if */
   lpCodecCtx -> m_Tag = lFmt;
   lpCodecCtx -> m_ID  = SMS_CodecGetID ( lpStm -> m_pCodec -> m_Type, lFmt );
   if ( lpCodecCtx -> m_Type == SMS_CodecTypeVideo ) {
    File_Skip ( apFileCtx, 16 );
    lpCodecCtx -> m_Width  = File_GetShortBE ( apFileCtx );
    lpCodecCtx -> m_Height = File_GetShortBE ( apFileCtx );
    File_Skip ( apFileCtx, 46 );
    lpCodecCtx -> m_BitsPerSample = File_GetShortBE ( apFileCtx );
   } else if ( lpCodecCtx -> m_Type == SMS_CodecTypeAudio ) {
    uint16_t lVer = File_GetShortBE ( apFileCtx );
    File_Skip ( apFileCtx, 6 );
    lpCodecCtx -> m_Channels      = File_GetShortBE ( apFileCtx );
    lpCodecCtx -> m_BitsPerSample = File_GetShortBE ( apFileCtx );
    File_Skip ( apFileCtx, 4 );
    lpCodecCtx -> m_SampleRate    = _read_int_be ( apFileCtx ) >> 16;
    if ( !apCtx -> m_fISOM ) {
     if ( lVer == 1 ) {
      lpMyStm -> m_SamplesPerFrame = _read_int_be ( apFileCtx );
      File_Skip ( apFileCtx, 4 );
      lpMyStm -> m_BytesPerFrame   = _read_int_be ( apFileCtx );
      File_Skip ( apFileCtx, 4 );
     } else if ( lVer == 2 ) {
      double lVal;
      File_Skip ( apFileCtx, 4 );
      *( uint64_t* )&lVal = _read_long_be ( apFileCtx );
      lpCodecCtx -> m_SampleRate = ( int32_t )lVal;
      lpCodecCtx -> m_Channels   = _read_int_be ( apFileCtx );
      File_Skip ( apFileCtx, 20 );
     }  /* end if */
    }  /* end if */
   } else File_Skip ( apFileCtx, apFileCtx -> m_CurPos - lStartPos );
   lAtom.m_Size = lSize - ( apFileCtx -> m_CurPos - lStartPos );
   if ( lAtom.m_Size > 8 )
    _mov_read_dflt ( apCtx, apFileCtx, &lAtom );
   else if ( lAtom.m_Size > 0 ) File_Skip (  apFileCtx, ( uint32_t )lAtom.m_Size  );
  }  /* end while */
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_stsd */

static int _mp4_read_descr_len ( FileContext* apFileCtx) {
 int lLen   = 0;
 int lCount = 4;
 while ( lCount-- ) {
  int lChr = File_GetByte ( apFileCtx );
  lLen = ( lLen << 7 ) | ( lChr & 0x7F );
  if (  !( lChr & 0x80 )  ) break;
 }  /* end while */
 return lLen;
}  /* end _mp4_read_descr_len */

static int _mp4_read_descr ( FileContext* apFileCtx, int* apTag ) {
 apTag[ 0 ] = File_GetByte ( apFileCtx );
 return _mp4_read_descr_len ( apFileCtx );
}  /* end _mp4_read_descr */

#define MP4ESDescrTag          0x03
#define MP4DecConfigDescrTag   0x04
#define MP4DecSpecificDescrTag 0x05

const SMS_CodecTag s_MP4ObjType[] = {
 { SMS_CodecID_MPEG4,  32 },
 { SMS_CodecID_AAC,    64 },
 { SMS_CodecID_MPEG2,  96 },
 { SMS_CodecID_MPEG2,  97 },
 { SMS_CodecID_AAC,   102 },
 { SMS_CodecID_AAC,   103 },
 { SMS_CodecID_MP3,   105 },
 { SMS_CodecID_MPEG1, 106 },
 { SMS_CodecID_MP3,   107 },
 { SMS_CodecID_OGGV,  221 },
 { SMS_CodecID_AC3,   226 }
};

static SMS_CodecID _mp4_get_id ( int aTag ) {
 int i;
 SMS_CodecID retVal = SMS_CodecID_NULL;
 for ( i = 0; i < sizeof ( s_MP4ObjType ) / sizeof ( s_MP4ObjType[ 0 ] ); ++i )
  if (  s_MP4ObjType[ i ].m_Tag == ( unsigned int )aTag  ) {
   retVal = s_MP4ObjType[ i ].m_ID;
   break;
  }  /* end if */
 return retVal;
}  /* end _mp4_get_id */

static int _mov_read_esds ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 int         lTag, lLen;
 File_Skip ( apFileCtx, 4 );
 lLen = _mp4_read_descr ( apFileCtx, &lTag );
 File_Skip ( apFileCtx, lTag == MP4ESDescrTag ? 3 : 2 );
 lLen = _mp4_read_descr ( apFileCtx, &lTag );
 if ( lTag == MP4DecConfigDescrTag ) {
  int               lObjTypeID = File_GetByte ( apFileCtx );
  SMS_CodecContext* lpCodecCtx = lpStm -> m_pCodec;
  File_Skip ( apFileCtx, 8 );
  lpCodecCtx -> m_BitRate = _read_int_be ( apFileCtx );
  lpCodecCtx -> m_ID      = _mp4_get_id ( lObjTypeID );
  lLen = _mp4_read_descr ( apFileCtx, &lTag );
  if ( lTag == MP4DecSpecificDescrTag ) {
   lpCodecCtx -> m_pUserData   = ( uint8_t* )malloc (  ( lLen + 15 ) & ~15  );
   apFileCtx -> Read ( apFileCtx, lpCodecCtx -> m_pUserData, lLen );
   lpCodecCtx -> m_UserDataLen = lLen;
  }  /* end if */
 }  /* end if */
 return 0;
}  /* end _mov_read_esds */

static int32_t _gcd ( int32_t anA, int32_t aB ) {
 return aB ? _gcd ( aB, anA % aB ) : anA;
}  /* end _gcd */

static int _mov_read_stts ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  MOVStream*   lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
  unsigned int i, lnEntries;
  int64_t      lDuration = 0;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  if ( lnEntries >= UINT_MAX / sizeof ( MOVStts )  ) return -1;
  lpMyStm -> m_nStts    = lnEntries;
  lpMyStm -> m_pStts    = ( MOVStts* )malloc ( lnEntries * sizeof ( MOVStts )  );
  lpMyStm -> m_TimeRate = 0;
  for ( i = 0; i < lnEntries; ++i ) {
   int lSampleCount    = _read_int_be ( apFileCtx );
   int lSampleDuration = _read_int_be ( apFileCtx );
   lpMyStm -> m_pStts[ i ].m_Count    = lSampleCount;
   lpMyStm -> m_pStts[ i ].m_Duration = lSampleDuration;
   lpMyStm -> m_TimeRate = _gcd ( lpMyStm -> m_TimeRate, lSampleDuration );
   lDuration  += ( int64_t )lSampleDuration * lSampleCount;
  }  /* end for */
  if ( lDuration ) lpStm -> m_Duration = lDuration;
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_stts */

static int _mov_read_stsc ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm   = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  MOVStream*   lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
  unsigned int i, lnEntries;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  if (  lnEntries >= UINT_MAX / sizeof ( MOVStsc )  ) return -1;
  lpMyStm -> m_nSample2Chunk = lnEntries;
  lpMyStm -> m_pSample2Chunk = ( MOVStsc* )malloc (  lnEntries * sizeof ( MOVStsc )  );
  for ( i = 0; i < lnEntries; ++i ) {
   lpMyStm -> m_pSample2Chunk[ i ].m_First = _read_int_be ( apFileCtx );
   lpMyStm -> m_pSample2Chunk[ i ].m_Count = _read_int_be ( apFileCtx );
   lpMyStm -> m_pSample2Chunk[ i ].m_ID    = _read_int_be ( apFileCtx );
  }  /* end for */
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_stsc */

static int _mov_read_stsz ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  MOVStream* lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
  unsigned int i, lnEntries, lSampleSize;
  File_Skip ( apFileCtx, 4 );
  lSampleSize = _read_int_be ( apFileCtx );
  if ( !lpMyStm -> m_SampleSize ) lpMyStm -> m_SampleSize = lSampleSize;
  lnEntries = _read_int_be ( apFileCtx );
  if (  lnEntries >= UINT_MAX / sizeof ( int )  ) return -1;
  lpMyStm -> m_nSamples = lnEntries;
  if ( lSampleSize ) return 0;
  lpMyStm -> m_pSampleSize = ( unsigned int* )malloc (  lnEntries * sizeof ( int )  );
  for ( i = 0; i < lnEntries; ++i ) lpMyStm -> m_pSampleSize[ i ] = _read_int_be ( apFileCtx );
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end mov_read_stsz */

static int _mov_read_stco ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm   = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  MOVStream* lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
  unsigned int i, lnEntries;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  if (  lnEntries >= UINT_MAX / sizeof ( int64_t )  ) return -1;
  lpMyStm -> m_nChunks       = lnEntries;
  lpMyStm -> m_pChunkOffsets = ( int64_t* )malloc (  lnEntries * sizeof ( int64_t )  );
  if (  apAtom -> m_Type == SMS_MKTAG( 's', 't', 'c', 'o' )  )
   for ( i = 0; i < lnEntries; ++i ) lpMyStm -> m_pChunkOffsets[ i ] = _read_int_be ( apFileCtx );
  else if (  apAtom -> m_Type == SMS_MKTAG( 'c', 'o', '6', '4' )  )
   for ( i = 0; i < lnEntries; ++i ) lpMyStm -> m_pChunkOffsets[ i ] = _read_long_be( apFileCtx );
  else return -1;
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_stco */

static int _mov_read_wave ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 if (  ( uint64_t )apAtom -> m_Size > ( 1 << 30 )  )
  return -1;
 else if ( apAtom -> m_Size > 8 )
  _mov_read_dflt ( apCtx, apFileCtx, apAtom );
 else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_wave */

static int _mov_read_ctts ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_Stream* lpStm = apCtx -> m_pBase -> m_pStm[ apCtx -> m_pBase -> m_nStm - 1 ];
 if ( lpStm ) {
  MOVStream* lpMyStm = ( MOVStream* )lpStm -> m_pCtx;
  unsigned int i, lnEntries;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  if (  lnEntries >= UINT_MAX / sizeof ( MOVStts )  ) return -1;
  lpMyStm -> m_nCtts = lnEntries;
  lpMyStm -> m_pCtts = ( MOVStts* )malloc (  lnEntries * sizeof ( MOVStts )  );
  for ( i = 0; i < lnEntries; ++i ) {
   int lCount    = _read_int_be ( apFileCtx );
   int lDuration = _read_int_be ( apFileCtx );
   if ( lDuration < 0 ) {
    lpMyStm -> m_nCtts = 0;
    File_Skip (  apFileCtx, 8 * ( lnEntries - i - 1 )  );
    break;
   }  /* end if */
   lpMyStm -> m_pCtts[ i ].m_Count    = lCount;
   lpMyStm -> m_pCtts[ i ].m_Duration = lDuration;
   lpMyStm -> m_TimeRate = _gcd ( lpMyStm -> m_TimeRate, lDuration );
  }  /* end for */
 } else File_Skip (  apFileCtx, ( uint32_t )apAtom -> m_Size  );
 return 0;
}  /* end _mov_read_ctts */

static int _mov_read_mdat ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 if ( apAtom -> m_Size == 0 ) return 0;
 apCtx -> m_pMDAT = ( MOVMDat* )realloc ( apCtx -> m_pMDAT, ( apCtx -> m_nMDAT + 1 ) * sizeof ( *apCtx -> m_pMDAT )  );
 apCtx -> m_pMDAT[ apCtx ->m_nMDAT ].m_Offset = apAtom -> m_Offset;
 apCtx -> m_pMDAT[ apCtx ->m_nMDAT ].m_Size   = apAtom -> m_Size;
 ++apCtx -> m_nMDAT;
 apCtx -> m_fMDAT      = 1;
 apCtx -> m_MDATOffset = apAtom -> m_Offset;
 if ( apCtx -> m_fMOOV ) return 1;
 File_Skip (  apFileCtx, ( uint32_t )( apAtom -> m_Size )  );
 return 0;
}  /* end _mov_read_mdat */

static int _mov_read_hdlr_m4ap ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_AudioInfo* lpInfo = ( SMS_AudioInfo* )apCtx -> m_pMDAT;
 if ( !lpInfo -> m_SampleRate ) {
  uint32_t lType;
  uint32_t lSubType;
  File_Skip ( apFileCtx, 4 );
  lType    = File_GetUInt ( apFileCtx );
  lSubType = File_GetUInt ( apFileCtx );
  if ( !lType ) apCtx -> m_fISOM = 1;
  if (  lSubType == SMS_MKTAG( 's', 'o', 'u', 'n' )  ) lpInfo -> m_nChannels = 0x80000000;
  File_Skip ( apFileCtx, 12 );
  if ( apAtom -> m_Size <= 24 ) return 0;
  File_Skip (   apFileCtx, ( uint32_t )(  apAtom -> m_Size - ( apFileCtx -> m_CurPos - apAtom -> m_Offset )  )   );
 } else File_Skip (  apFileCtx, ( uint32_t )( apAtom -> m_Size )  );
 return 0;
}  /* end _mov_read_hdlr_m4ap */

static int _mov_read_stsd_m4ap ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_AudioInfo* lpInfo = ( SMS_AudioInfo* )apCtx -> m_pMDAT;
 if ( lpInfo -> m_nChannels < 0 ) {
  int      lnEntries;
  uint32_t lFmt;
  File_Skip ( apFileCtx, 4 );
  lnEntries = _read_int_be ( apFileCtx );
  while ( lnEntries-- ) {
   MOVAtom     lAtom     = { 0, 0, 0 };
   uint32_t    lStartPos = apFileCtx -> m_CurPos;
   int         lSize     = _read_int_be ( apFileCtx );
   uint16_t    lVer;
   lFmt = File_GetUInt ( apFileCtx );
   File_Skip ( apFileCtx, 8 );
   if ( lpInfo -> m_nChannels > 0 ) {
    File_Skip (  apFileCtx, lSize - ( apFileCtx -> m_CurPos - lStartPos )  );
    continue;
   }  /* end if */
   lVer = File_GetShortBE ( apFileCtx );
   File_Skip ( apFileCtx, 6 );
   lpInfo -> m_nChannels  = File_GetShortBE ( apFileCtx );
   File_Skip ( apFileCtx, 6 );
   lpInfo -> m_SampleRate = _read_int_be ( apFileCtx ) >> 16;
   if ( !apCtx -> m_fISOM ) {
    if ( lVer == 1 )
     File_Skip ( apFileCtx, 16 );
    else if ( lVer == 2 ) {
     double lVal;
     File_Skip ( apFileCtx, 4 );
     *( uint64_t* )&lVal = _read_long_be ( apFileCtx );
     lpInfo -> m_SampleRate = ( int32_t )lVal;
     lpInfo -> m_nChannels  = _read_int_be ( apFileCtx );
     File_Skip ( apFileCtx, 20 );
    }  /* end if */
   }  /* end if */
   lAtom.m_Size = lSize - ( apFileCtx -> m_CurPos - lStartPos );
   if ( lAtom.m_Size > 8 )
    _mov_read_dflt ( apCtx, apFileCtx, &lAtom );
   else if ( lAtom.m_Size > 0 ) File_Skip (  apFileCtx, ( uint32_t )lAtom.m_Size  );
  }  /* end while */
 } else File_Skip (  apFileCtx, ( uint32_t )( apAtom -> m_Size )  );
 return 0;
}  /* end _mov_read_stsd_m4ap */

static int _mov_read_esds_m4ap ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {
 SMS_AudioInfo* lpInfo = ( SMS_AudioInfo* )apCtx -> m_pMDAT;
 if ( lpInfo -> m_nChannels > 0 ) {
  int         lTag, lLen;
  File_Skip ( apFileCtx, 4 );
  lLen = _mp4_read_descr ( apFileCtx, &lTag );
  File_Skip ( apFileCtx, lTag == MP4ESDescrTag ? 3 : 2 );
  lLen = _mp4_read_descr ( apFileCtx, &lTag );
  if ( lTag == MP4DecConfigDescrTag ) {
   File_Skip ( apFileCtx, 9 );
   lpInfo -> m_BitRate = _read_int_be ( apFileCtx );
   lLen = _mp4_read_descr ( apFileCtx, &lTag );
   if ( lTag == MP4DecSpecificDescrTag ) File_Skip ( apFileCtx, lLen );
  }  /* end if */
 }  /* end if */
 return 0;
}  /* end _mov_read_esds_m4ap */

static MOVParseEntry s_MOVParseTbl[] = {
 { SMS_MKTAG( 'h', 'd', 'l', 'r' ), _mov_read_hdlr },
 { SMS_MKTAG( 'f', 't', 'y', 'p' ), _mov_read_ftyp },
 { SMS_MKTAG( 'm', 'o', 'o', 'v' ), _mov_read_moov },
 { SMS_MKTAG( 'm', 'v', 'h', 'd' ), _mov_read_mvhd },
 { SMS_MKTAG( 't', 'r', 'a', 'k' ), _mov_read_trak },
 { SMS_MKTAG( 't', 'k', 'h', 'd' ), _mov_read_tkhd },
 { SMS_MKTAG( 'e', 'd', 't', 's' ), _mov_read_dflt },
 { SMS_MKTAG( 'e', 'l', 's', 't' ), _mov_read_dflt },
 { SMS_MKTAG( 'm', 'd', 'i', 'a' ), _mov_read_dflt },
 { SMS_MKTAG( 'm', 'd', 'h', 'd' ), _mov_read_mdhd },
 { SMS_MKTAG( 'm', 'i', 'n', 'f' ), _mov_read_dflt },
 { SMS_MKTAG( 's', 't', 'b', 'l' ), _mov_read_dflt },
 { SMS_MKTAG( 's', 't', 's', 'd' ), _mov_read_stsd },
 { SMS_MKTAG( 'e', 's', 'd', 's' ), _mov_read_esds },
 { SMS_MKTAG( 's', 't', 't', 's' ), _mov_read_stts },
 { SMS_MKTAG( 'w', 'a', 'v', 'e' ), _mov_read_wave },
 { SMS_MKTAG( 's', 't', 's', 'c' ), _mov_read_stsc },
 { SMS_MKTAG( 'c', 't', 't', 's' ), _mov_read_ctts },
 { SMS_MKTAG( 's', 't', 's', 'z' ), _mov_read_stsz },
 { SMS_MKTAG( 's', 't', 'c', 'o' ), _mov_read_stco },
 { SMS_MKTAG( 'm', 'd', 'a', 't' ), _mov_read_mdat },
 {                               0,           NULL }
};

static MOVParseEntry s_M4AProbeTbl[] = {
 { SMS_MKTAG( 'm', 'o', 'o', 'v' ), _mov_read_dflt      },
 { SMS_MKTAG( 't', 'r', 'a', 'k' ), _mov_read_dflt      },
 { SMS_MKTAG( 'm', 'd', 'i', 'a' ), _mov_read_dflt      },
 { SMS_MKTAG( 'm', 'i', 'n', 'f' ), _mov_read_dflt      },
 { SMS_MKTAG( 's', 't', 'b', 'l' ), _mov_read_dflt      },
 { SMS_MKTAG( 'w', 'a', 'v', 'e' ), _mov_read_dflt      },
 { SMS_MKTAG( 'h', 'd', 'l', 'r' ), _mov_read_hdlr_m4ap },
 { SMS_MKTAG( 's', 't', 's', 'd' ), _mov_read_stsd_m4ap },
 { SMS_MKTAG( 'e', 's', 'd', 's' ), _mov_read_esds_m4ap },
 {                               0,           NULL      }
};

static int _mov_read_dflt ( MOVContext* apCtx, FileContext* apFileCtx, MOVAtom* apAtom ) {

 int64_t              lTotalSize = 0;
 MOVAtom              lAtom;
 int                  retVal = 0;
 const MOVParseEntry* lpIt;

 lAtom.m_Offset = apAtom -> m_Offset;

 if ( apAtom -> m_Size < 0 ) apAtom -> m_Size = SMS_MAXINT64;

 while (   (  ( lTotalSize + 8 ) < apAtom -> m_Size ) && !FILE_EOF( apFileCtx ) && !retVal   ) {
  lAtom.m_Size = apAtom -> m_Size;
  lAtom.m_Type = 0;
  if ( apAtom -> m_Size >= 8 ) {
   lAtom.m_Size = _read_int_be ( apFileCtx );
   lAtom.m_Type = File_GetUInt ( apFileCtx );
  }  /* end if */
  lTotalSize     += 8;
  lAtom.m_Offset += 8;
  if ( lAtom.m_Size == 1 ) {
   lAtom.m_Size    = _read_long_be ( apFileCtx );
   lAtom.m_Offset += 8;
   lTotalSize     += 8;
  }  /* end if */
  if ( lAtom.m_Size == 0 ) {
   lAtom.m_Size = apAtom -> m_Size - lTotalSize;
   if ( lAtom.m_Size <= 8 ) break;
  }  /* end if */
  lAtom.m_Size -= 8;
  if ( lAtom.m_Size < 0 || lAtom.m_Size > apAtom -> m_Size - lTotalSize ) break;
  for ( lpIt = apCtx -> m_pParseTbl; lpIt -> m_Type && lpIt -> m_Type != lAtom.m_Type; ++lpIt );
  if ( !lpIt -> m_Type )
   File_Skip (  apFileCtx, ( uint32_t )lAtom.m_Size  );
  else {
   uint32_t lStartPos = apFileCtx -> m_CurPos;
   int64_t  lLeft;
   retVal = lpIt -> m_Func ( apCtx, apFileCtx, &lAtom );
   lLeft  = lAtom.m_Size - apFileCtx -> m_CurPos + lStartPos;
   if ( lLeft > 0 ) File_Skip (  apFileCtx, ( uint32_t )lLeft  );
  }  /* end else */
  lAtom.m_Offset += lAtom.m_Size;
  lTotalSize     += lAtom.m_Size;
 }  /* end while */

 if ( !retVal && lTotalSize < lAtom.m_Size && lAtom.m_Size < 0x7FFFF ) File_Skip (  apFileCtx, ( uint32_t )( lAtom.m_Size - lTotalSize )  );

 return !retVal;

}  /* end _mov_read_dflt */

static void _add_idx_entry ( MOVStream* apMyStm, int afKF, uint32_t aDTS, uint32_t anOffset, uint32_t aSize ) {
 MOVIndex* lpIdx, *lpIndices;
 lpIndices = ( MOVIndex* )SMS_Realloc (
  apMyStm -> m_pIdx, &apMyStm -> m_IdxAllocSize,
  ( apMyStm -> m_nIdx + 1 ) * sizeof ( MOVIndex )
 );
 apMyStm -> m_pIdx = lpIndices;
 lpIdx = &lpIndices[ apMyStm -> m_nIdx++ ];
 lpIdx -> m_Flags  = afKF;
 lpIdx -> m_DTS    = aDTS;
 lpIdx -> m_Pos    = anOffset;
 lpIdx -> m_Size   = aSize;
}  /* end _add_idx_entry */

static void _mov_build_index ( SMS_Container* apCont, MOVStream* apMyStm, SMS_Stream* apStm ) {

 int64_t      lCurOffset;
 int64_t      lCurDTS  = 0;
 unsigned int lSTTSIdx = 0;
 unsigned int lSTSCIdx = 0;
 unsigned int lSTSSIdx = 0;
 MOVContext*  lpCtx    = ( MOVContext* )apCont -> m_pCtx;
 unsigned int i, j, k;

 if ( apMyStm -> m_pSampleSize || apStm -> m_pCodec -> m_Type == SMS_CodecTypeVideo ) {
  unsigned int lCurSample = 0;
  unsigned int lSTTSample = 0;
  unsigned int lKeyFrame, lSampleSize;
  for ( i = 0; i < apMyStm -> m_nChunks; ++i ) {
   lCurOffset = apMyStm -> m_pChunkOffsets[ i ];
   if ( lSTSCIdx + 1  < apMyStm -> m_nSample2Chunk &&
        i        + 1 == ( unsigned int )apMyStm -> m_pSample2Chunk[ lSTSCIdx + 1 ].m_First
   ) ++lSTSCIdx;
   for (  j = 0; j < ( unsigned int )apMyStm -> m_pSample2Chunk[ lSTSCIdx ].m_Count; ++j ) {
    if ( lCurSample >= apMyStm -> m_nSamples ) return;
    lKeyFrame = !apMyStm -> m_nKeyFrames || lCurSample + 1 == ( unsigned int )apMyStm -> m_pKeyFrames[ lSTSSIdx ];
    if ( lKeyFrame ) {
     if ( lSTSSIdx + 1 < apMyStm -> m_nKeyFrames ) ++lSTSSIdx;
    }  /* end if */
    lSampleSize = apMyStm -> m_SampleSize > 0 ? apMyStm -> m_SampleSize : apMyStm -> m_pSampleSize[ lCurSample ];
    _add_idx_entry (  apMyStm, lKeyFrame, ( uint32_t )lCurDTS, ( uint32_t )lCurOffset, lSampleSize  );
    lCurOffset += lSampleSize;
    lCurDTS    += apMyStm -> m_pStts[ lSTTSIdx ].m_Duration / apMyStm -> m_TimeRate;
    lSTTSample += 1;
    lCurSample += 1;;
    if ( lSTSSIdx + 1  < apMyStm -> m_nStts &&
         lSTTSample   == ( unsigned int )apMyStm -> m_pStts[ lSTTSIdx ].m_Count
    ) {
     lSTTSample = 0;
     lSTSSIdx  += 1;;
    }  /* end if */
   }  /* end for */
  }  /* end for */
 } else {
  unsigned int lnChunkSamples, lChunkSize, lChunkDuration;
  for ( i = 0; i < apMyStm -> m_nChunks; ++i ) {
   lCurOffset = apMyStm -> m_pChunkOffsets[ i ];
   if ( lSTSCIdx + 1  < apMyStm -> m_nSample2Chunk &&
        i        + 1 == ( unsigned int )apMyStm -> m_pSample2Chunk[ lSTSCIdx + 1 ].m_First
   ) ++lSTSCIdx;
   lnChunkSamples = apMyStm -> m_pSample2Chunk[ lSTSCIdx ].m_Count;
   if (  apMyStm -> m_SampleSize > 1 )
    lChunkSize = lnChunkSamples * apMyStm -> m_SampleSize;
   else if (  apMyStm -> m_SamplesPerFrame > 0 && ( lnChunkSamples * apMyStm -> m_BytesPerFrame % apMyStm -> m_SamplesPerFrame == 0 )  )
    lChunkSize = lnChunkSamples * apMyStm -> m_BytesPerFrame / apMyStm -> m_SamplesPerFrame;
   else {
    lChunkSize = INT_MAX;
    for (  j = 0; j < ( unsigned int )apCont -> m_nStm; ++j  ) {
     MOVStream* lpMyStm = apCont -> m_pStm[ j ] -> m_pCtx;
     for ( k = lpMyStm -> m_NextChunk; k < lpMyStm -> m_nChunks; ++k) {
     if ( lpMyStm -> m_pChunkOffsets[ k ] > lCurOffset &&
          lpMyStm -> m_pChunkOffsets[ k ] - lCurOffset < lChunkSize
     ) {
      lChunkSize = ( unsigned int )( lpMyStm -> m_pChunkOffsets[ k ] - lCurOffset );
      lpMyStm -> m_NextChunk = k;
      break;
     }  /* end if */
    }  /* end for */
   }  /* end for */
   if ( lChunkSize == INT_MAX )
    for (  j = 0; j < ( unsigned int )lpCtx -> m_nMDAT; ++j ) {
     if ( lpCtx -> m_pMDAT[ j ].m_Offset <= lCurOffset &&
          lpCtx -> m_pMDAT[ j ].m_Offset + lpCtx -> m_pMDAT[ j ].m_Size > lCurOffset
     ) lChunkSize = ( unsigned int )( lpCtx -> m_pMDAT[ j ].m_Offset + lpCtx -> m_pMDAT[ j ].m_Size - lCurOffset );
    }  /* end for */
    for (  j = 0; j < ( unsigned int )apCont -> m_nStm; ++j  ) {
     MOVStream* lpMyStm = ( MOVStream* )apCont -> m_pStm[ j ] -> m_pCtx;
     lpMyStm -> m_NextChunk = 0;
    }  /* end for */
   }  /* end else */
   _add_idx_entry (  apMyStm, 1, ( uint32_t )lCurDTS, ( uint32_t )lCurOffset, lChunkSize  );
   lChunkDuration = 0;
   while ( lnChunkSamples > 0 ) {
    if (  lnChunkSamples < ( unsigned int )apMyStm -> m_pStts[ lSTTSIdx ].m_Count ) {
     lChunkDuration += apMyStm -> m_pStts[ lSTTSIdx ].m_Duration * lnChunkSamples;
     apMyStm -> m_pStts[ lSTTSIdx ].m_Count -= lnChunkSamples;
     break;
    } else {
     lChunkDuration += apMyStm -> m_pStts[ lSTTSIdx ].m_Duration * lnChunkSamples;
     lnChunkSamples -= apMyStm -> m_pStts[ lSTTSIdx ].m_Count;
     if ( lSTTSIdx + 1 < apMyStm -> m_nStts ) ++lSTTSIdx;
    }  /* end else */
   }  /* end while */
   lCurDTS += lChunkDuration / apMyStm -> m_TimeRate;
  }  /* end for */
 }  /* end else */

}  /* end _mov_build_index */

static int _mov_read_header ( SMS_Container* apCont ) {

 MOVContext*  lpMOV     = ( MOVContext* )apCont -> m_pCtx;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 MOVAtom      lAtom     = { 0, 0, lpFileCtx -> m_Size };
 int          i, retVal;

 retVal = _mov_read_dflt ( lpMOV, lpFileCtx, &lAtom );

 if (  retVal < 0 || ( !lpMOV -> m_fMOOV && !lpMOV -> m_fMDAT )  ) return 0;

 if ( lpFileCtx -> m_CurPos != lpMOV -> m_MDATOffset ) lpFileCtx -> Seek (  lpFileCtx, ( uint32_t )( lpMOV -> m_MDATOffset )  );

 for (  i = 0; i < ( int )apCont -> m_nStm; ++i  ) {

  SMS_Stream* lpStm   = apCont -> m_pStm[ i ];
  MOVStream*  lpMyStm;

  if ( !lpStm ) {
   memmove (
    &apCont -> m_pStm[ i ], &apCont -> m_pStm[ i + 1 ],
    ( &apCont -> m_pStm[ SMS_MAX_STREAMS ] - &apCont -> m_pStm[ i + 1 ] ) * 4
   );
   --apCont -> m_nStm;
   --i;
   continue;
  }  /* end if */

  lpMyStm = ( MOVStream* )lpStm -> m_pCtx;

  if ( !lpMyStm -> m_nStts        || !lpMyStm -> m_nChunks ||
       !lpMyStm -> m_nSample2Chunk|| (
         !lpMyStm -> m_SampleSize && !lpMyStm -> m_nSamples
        )
  ) {
   lpMyStm -> m_nSamples = 0;
   continue;
  }  /* end if */

  if ( !lpMyStm -> m_TimeRate  ) lpMyStm -> m_TimeRate  = 1;
  if ( !lpMyStm -> m_TimeScale ) lpMyStm -> m_TimeScale = lpMOV -> m_TimeScale;

  SMSContainer_SetPTSInfo ( lpStm, lpMyStm -> m_TimeRate, lpMyStm -> m_TimeScale );

  if ( lpStm -> m_pCodec -> m_Type == SMS_CodecTypeAudio &&
       lpMyStm -> m_nStts == 1
  ) lpStm -> m_pCodec -> m_FrameSize = lpMyStm -> m_pStts[ 0 ].m_Duration;

  if ( lpStm -> m_Duration != SMS_NOPTS_VALUE ) lpStm -> m_Duration /= lpMyStm -> m_TimeRate;

  _mov_build_index ( apCont, lpMyStm, lpStm );

  _free ( lpMyStm -> m_pChunkOffsets );
  _free ( lpMyStm -> m_pSample2Chunk );
  _free ( lpMyStm -> m_pSampleSize   );
  _free ( lpMyStm -> m_pKeyFrames    );
  _free ( lpMyStm -> m_pStts         );

 }  /* end for */

 _free ( lpMOV -> m_pMDAT );

 return 1;

}  /* end _mov_read_header */

static uint32_t _mov_probe ( FileContext* apFileCtx ) {

 unsigned int lSize;
 unsigned int lTag   = 0;
 unsigned int lType  = 0;

 lSize = File_GetUInt ( apFileCtx );
 lType = File_GetUInt ( apFileCtx );
 lTag  = File_GetUInt ( apFileCtx );

 lSize = lType == SMS_MKTAG( 'f', 't', 'y', 'p' ) ? lTag : 0;

 apFileCtx -> Seek ( apFileCtx, 0 );

 return lSize;

}  /* end _mov_probe */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 SMS_Stream*   lpStm     = NULL;
 MOVIndex*     lpSample  = NULL;
 MOVStream*    lpMyStm   = NULL;
 unsigned int  i, lIdx   = 0;
 SMS_AVPacket* lpPkt;

 do {

  int64_t lBestDTS = SMS_MAXINT64;

  for ( i = 0; i < apCont -> m_nStm; ++i ) {
   SMS_Stream* lpCurStm   = apCont -> m_pStm[ i ];
   MOVStream*  lpMyCurStm = ( MOVStream* )lpCurStm -> m_pCtx;
   if ( lpMyCurStm -> m_CurSample < lpMyCurStm -> m_nSamples ) {
    MOVIndex* lpCurSample = &lpMyCurStm -> m_pIdx[ lpMyCurStm -> m_CurSample ];
    int64_t   lDTS = SMS_Rescale ( lpCurSample -> m_DTS * lpMyCurStm -> m_TimeRate, SMS_TIME_BASE, lpMyCurStm -> m_TimeScale );
    if ( lDTS < lBestDTS ) {
     lpSample = lpCurSample;
     lBestDTS = lDTS;
     lpStm    = lpCurStm;
     lpMyStm  = lpMyCurStm;
     lIdx     = i;
    }  /* end if */
   }   /* end if */
  }  /* end for */

  if ( !lpSample ) return -1;

  ++lpMyStm -> m_CurSample;

  if ( lpSample -> m_Pos > lpFileCtx -> m_Size ) return -1;

 } while ( !lpStm -> m_pPktBuf );

 lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lpSample -> m_Size );

 if ( !lpPkt ) return -1;

 if ( lpFileCtx -> m_CurPos != lpSample -> m_Pos ) {
  if ( lpFileCtx -> m_CurPos < lpSample -> m_Pos )
   File_Skip ( lpFileCtx, lpSample -> m_Pos - lpFileCtx -> m_CurPos );
  else lpFileCtx -> Seek ( lpFileCtx, lpSample -> m_Pos );
 }  /* end if */

 lpFileCtx -> Read ( lpFileCtx, lpPkt -> m_pData, lpPkt -> m_Size );

 lpPkt -> m_StmIdx = lIdx;
 lpPkt -> m_DTS    = lpSample -> m_DTS;

 if ( lpMyStm -> m_pCtts ) {
  lpPkt -> m_PTS = lpPkt ->m_DTS + lpMyStm -> m_pCtts[ lpMyStm -> m_Sample2CTimeIdx ].m_Duration / lpMyStm -> m_TimeRate;
  ++lpMyStm -> m_Sample2CTimeSample;
  if ( lpMyStm -> m_Sample2CTimeIdx < lpMyStm -> m_nCtts &&
       lpMyStm -> m_pCtts[ lpMyStm -> m_Sample2CTimeIdx ].m_Count == lpMyStm -> m_Sample2CTimeSample
  ) {
   ++lpMyStm -> m_Sample2CTimeIdx;
     lpMyStm -> m_Sample2CTimeSample = 0;
  }  /* end if */
 } else lpPkt -> m_PTS = lpPkt -> m_DTS;

 lpPkt -> m_Flags |= lpSample -> m_Flags ? SMS_PKT_FLAG_KEY : 0;

 apIdx[ 0 ] = lpPkt -> m_StmIdx;

 SMSContainer_CalcPktFields ( lpStm, lpPkt );

 return lpPkt -> m_Size;

}  /* end _ReadPacket */

static int _GetContainerMOV ( SMS_Container* apCont, const MOVParseEntry* apParseTbl ) {

 int          retVal    = 0;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 uint32_t     lType     = _mov_probe (  lpFileCtx );

 if ( lType ) {

  MOVContext* lpMyCtx = ( MOVContext* )calloc (  1, sizeof ( MOVContext )  );

  apCont  -> m_pCtx      = lpMyCtx;
  lpMyCtx -> m_pBase     = apCont;
  lpMyCtx -> m_pParseTbl = apParseTbl;

  if (  _mov_read_header ( apCont )  ) {

   apCont -> m_pName    = g_pMOV;
   apCont -> ReadPacket = _ReadPacket;

   retVal = 1;

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _GetContainerMOV */

uint64_t SMS_M4AProbe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 uint64_t retVal = 0;
 uint32_t lType  = _mov_probe ( apFileCtx );

 if ( lType ) {

  SMS_Container lCont;
  MOVContext    lMyCont; memset (  &lMyCont, 0, sizeof ( lMyCont )  );

  lCont.m_pCtx        = &lMyCont;
  lCont.m_pFileCtx    = apFileCtx;
  lMyCont.m_pMDAT     = ( MOVMDat* )apInfo;
  lMyCont.m_pParseTbl = s_M4AProbeTbl;

  apInfo -> m_SampleRate = 0;
  apInfo -> m_nChannels  = 0;
  apInfo -> m_BitRate    = 0;

  retVal = _mov_read_header ( &lCont ) >= 0 && apInfo -> m_SampleRate && apInfo -> m_nChannels && apInfo -> m_BitRate;

 }  /* end if */

 apFileCtx -> Seek ( apFileCtx, 0 );

 return retVal;

}  /* end SMS_M4AProbe */

int SMS_GetContainerMOV ( SMS_Container* apCont ) {

 int retVal = 0;

 if (  ( int )apCont -> m_pFileCtx <= 0  ) return retVal;

 s_MOVParseTbl[ 0 ].m_Func = _mov_read_hdlr;

 if (   (  retVal = _GetContainerMOV ( apCont, s_MOVParseTbl )  )   )
  retVal = SMSContainer_SetName ( apCont, apCont -> m_pFileCtx );

 return retVal;

}  /* end SMS_GetContainerMOV */

int SMS_GetContainerM4A ( SMS_Container* apCont ) {

 int retVal = 0;

 if (  ( int )apCont -> m_pFileCtx <= 0  ) return retVal;

 s_MOVParseTbl[ 0 ].m_Func = _mov_read_hdlr_m4a;

 if (   (  retVal = _GetContainerMOV ( apCont, s_MOVParseTbl )  )   )
  retVal = SMSContainer_SetName ( apCont, apCont -> m_pFileCtx );

 return retVal;

}  /* end SMS_GetContainerM4A */
