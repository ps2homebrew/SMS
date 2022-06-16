/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001 Fabrice Bellard.
#               2007 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
# 
*/
#include "SMS_ContainerASF.h"
#include "SMS_Locale.h"

#include <malloc.h>
#include <string.h>

#define FRAME_HEADER_SIZE 17

#define ASF_HEADER         0
#define DATA_HEADER        1
#define FILE_HEADER        2
#define STREAM_HEADER      3
#define AUDIO_STREAM       4
#define VIDEO_STREAM       5
#define COMMAND_STREAM     6
#define ESE_STREAM_HEADER  7
#define ES_AUDIO_STREAM    8
#define COMMENT_HEADER     9
#define STREAM_BITRATE    10
#define EX_CTX_HEADER     11

#define MYSTRM( s ) (  ( ASFStream* )s -> m_pCtx  )

#define DO_2BITS( f, bits, var, defval )                 \
 switch ( bits & 3 ) {                                   \
  case 3 : var = File_GetUInt   ( f ); rsize += 4; break;\
  case 2 : var = File_GetUShort ( f ); rsize += 2; break;\
  case 1 : var = File_GetByte   ( f ); rsize += 1; break;\
  default: var = defval; break;                          \
 }

static int SMS_INLINE guid_equal ( const SMS_GUID apLeft, const SMS_GUID apRight ) {
 return !memcmp (  apLeft, apRight, sizeof ( SMS_GUID )  );
}  /* end guid_equal */

typedef struct ASFPkt {
 int64_t  pts;
 int64_t  pos;
 int      size;
 uint8_t* data;
 int      alloc;
 int      flags;
 int      stream_index;
} ASFPkt;

typedef struct ASFStream {
 unsigned char seq;
 int           frag_offset;
 int           ds_span;
 int           ds_packet_size;
 int           ds_chunk_size;
 int64_t       packet_pos;
 ASFPkt        pkt;
} ASFStream;

typedef struct ASFMainHeader {
 SMS_GUID guid;
 uint64_t file_size;
 uint64_t create_time;
 uint64_t play_time;
 uint64_t send_time;
 uint32_t preroll;
 uint32_t ignore;
 uint32_t flags;
 uint32_t min_pktsize;
 uint32_t max_pktsize;
 uint32_t max_bitrate;
} ASFMainHeader;

typedef struct ASFIndex {
 uint32_t packet_number;
 uint16_t packet_count;
} ASFIndex;

typedef struct {
 unsigned int  packet_size;
 short         asfid2avid     [ 128 ];
 uint32_t      stream_bitrates[ 128 ];
 uint64_t      nb_packets;
 int           packet_size_left;
 uint64_t      data_offset;
 uint64_t      data_object_offset;
 uint64_t      data_object_size;
 uint8_t       index_read;
 ASFMainHeader hdr;
 int           packet_flags;
 int           packet_property;
 int           packet_timestamp;
 int           packet_segsizetype;
 int           packet_segments;
 int           packet_seq;
 int           packet_replic_size;
 int           packet_key_frame;
 int           packet_padsize;
 int           packet_frag_offset;
 int           packet_frag_size;
 int64_t       packet_frag_timestamp;
 int           packet_multi_size;
 int           packet_obj_size;
 int           packet_time_delta;
 int           packet_time_start;
 int64_t       packet_pos;
 int           stream_index;
 ASFIndex*     index_ptr;
} ASFContext;

static SMS_GUID s_GUID[] SMS_DATA_SECTION = {
 { 0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C },
 { 0x36, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C },
 { 0xA1, 0xDC, 0xAB, 0x8C, 0x47, 0xA9, 0xCF, 0x11, 0x8E, 0xE4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 },
 { 0x91, 0x07, 0xDC, 0xB7, 0xB7, 0xA9, 0xCF, 0x11, 0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65 },
 { 0x40, 0x9E, 0x69, 0xF8, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B },
 { 0xC0, 0xEF, 0x19, 0xBC, 0x4D, 0x5B, 0xCF, 0x11, 0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B },
 { 0xC0, 0xCF, 0xDA, 0x59, 0xE6, 0x59, 0xD0, 0x11, 0xA3, 0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6 },
 { 0xE2, 0x65, 0xFB, 0x3A, 0xEF, 0x47, 0xF2, 0x40, 0xAC, 0x2C, 0x70, 0xA9, 0x0D, 0x71, 0xD3, 0x43 },
 { 0x9D, 0x8C, 0x17, 0x31, 0xE1, 0x03, 0x28, 0x45, 0xB5, 0x82, 0x3D, 0xF9, 0xDB, 0x22, 0xF5, 0x03 },
 { 0x33, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C },
 { 0xCE, 0x75, 0xF8, 0x7B, 0x8D, 0x46, 0xD1, 0x11, 0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2 },
 { 0x40, 0xA4, 0xD0, 0xD2, 0x07, 0xE3, 0xD2, 0x11, 0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50 }
};

static void SMS_INLINE get_guid ( FileContext* apFileCtx, SMS_GUID aGUID ) {
 apFileCtx -> Read ( apFileCtx, aGUID, sizeof ( SMS_GUID )  );
}  /* end get_guid */

static int asf_read_header ( SMS_Container* apCont ) {

 uint32_t     lBitRate[ 128 ];
 int64_t      lGSize;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 ASFContext*  lpASF;
 SMS_GUID     lGUID;
 SMS_Stream*  lpStm;
 ASFStream*   lpASFStm;
 int          i, lSize;

 memset (  lBitRate, 0, sizeof ( lBitRate )  );

 get_guid ( lpFileCtx, lGUID );

 if (  !guid_equal ( lGUID, s_GUID[ ASF_HEADER ] )  ) goto fail;

 File_Skip ( lpFileCtx, 14 );

 apCont -> m_pCtx = lpASF = ( ASFContext* )calloc (  1, sizeof ( ASFContext )  );

 memset ( lpASF -> asfid2avid, -1, sizeof ( lpASF -> asfid2avid )  );

 while ( 1 ) {

  get_guid ( lpFileCtx, lGUID );

  lGSize = File_GetULong ( lpFileCtx );

  if (  guid_equal ( lGUID, s_GUID[ DATA_HEADER ] )  ) {
   lpASF -> data_object_offset = lpFileCtx -> m_CurPos;
   if (  !( lpASF -> hdr.flags & 0x01 ) && lGSize >= 100  )
    lpASF -> data_object_size = lGSize - 24;
   else lpASF -> data_object_size = ( uint64_t )-1;
   break;
  }  /* end if */

  if ( lGSize < 24 ) goto fail;

  if (  guid_equal ( lGUID, s_GUID[ FILE_HEADER ] )  ) {

   get_guid ( lpFileCtx, lpASF -> hdr.guid );

   lpASF -> hdr.file_size   = File_GetULong ( lpFileCtx );
   lpASF -> hdr.create_time = File_GetULong ( lpFileCtx );
   lpASF -> nb_packets      = File_GetULong ( lpFileCtx );
   lpASF -> hdr.send_time   = File_GetULong ( lpFileCtx );
   lpASF -> hdr.play_time   = File_GetULong ( lpFileCtx );
   lpASF -> hdr.preroll     = File_GetUInt ( lpFileCtx );
   lpASF -> hdr.ignore      = File_GetUInt ( lpFileCtx );
   lpASF -> hdr.flags       = File_GetUInt ( lpFileCtx );
   lpASF -> hdr.min_pktsize = File_GetUInt ( lpFileCtx );
   lpASF -> hdr.max_pktsize = File_GetUInt ( lpFileCtx );
   lpASF -> hdr.max_bitrate = File_GetUInt ( lpFileCtx );
   lpASF -> packet_size     = lpASF -> hdr.max_pktsize;

  } else if (  guid_equal ( lGUID, s_GUID[ STREAM_HEADER ] )  ) {

   enum SMS_CodecType lCodecType;
   int                type_specific_size, sizeX;
   uint64_t           total_size;
   unsigned int       tag1;
   int32_t            pos1, pos2, start_time;
   int                test_for_ext_stream_audio, is_dvr_ms_audio = 0;

   pos1  = lpFileCtx -> m_CurPos;
   lpStm = ( SMS_Stream* )calloc (  1, sizeof ( SMS_Stream ) + sizeof ( ASFStream )  );

   if ( !lpStm ) goto fail;

   lpStm -> m_pCtx     = ( char* )lpStm + sizeof( SMS_Stream );
   lpStm -> m_Duration = SMS_NOPTS_VALUE;
   lpStm -> m_pCodec   = ( SMS_CodecContext* )calloc (  1, sizeof ( SMS_CodecContext )  );
   lpASFStm = MYSTRM( lpStm );

   apCont -> m_pStm[ apCont -> m_nStm++ ] = lpStm;

   SMSContainer_SetPTSInfo ( lpStm, 1, 1000 );

   start_time = lpASF -> hdr.preroll;

   if (  !( lpASF -> hdr.flags & 0x01 )  ) lpStm -> m_Duration = lpASF -> hdr.send_time / ( 10000000 / 1000 ) - start_time;

   get_guid ( lpFileCtx, lGUID );

   test_for_ext_stream_audio = 0;

   if (  guid_equal ( lGUID, s_GUID[ AUDIO_STREAM ] )  )
    lCodecType = SMS_CodecTypeAudio;
   else if (  guid_equal ( lGUID, s_GUID[ VIDEO_STREAM ] )  )
    lCodecType = SMS_CodecTypeVideo;
   else if (  guid_equal ( lGUID, s_GUID[ COMMAND_STREAM ] )  )
    lCodecType = SMS_CodecTypeUnknown;
   else if (  guid_equal ( lGUID, s_GUID[ ESE_STREAM_HEADER ] )  ) {
    test_for_ext_stream_audio = 1;
    lCodecType = SMS_CodecTypeUnknown;
   } else goto fail;

   get_guid ( lpFileCtx, lGUID );

   total_size         = File_GetULong  ( lpFileCtx );
   type_specific_size = File_GetUInt   ( lpFileCtx );
                        File_GetUInt   ( lpFileCtx );
   lpStm -> m_ID      = File_GetUShort ( lpFileCtx ) & 0x7F;
   lpASF -> asfid2avid[ lpStm -> m_ID ] = apCont -> m_nStm - 1;
                        File_GetUInt   ( lpFileCtx );

   if ( test_for_ext_stream_audio ) {
    get_guid ( lpFileCtx, lGUID );
    if (  guid_equal ( lGUID, s_GUID[ ES_AUDIO_STREAM ] )  ) {
     lCodecType      = SMS_CodecTypeAudio;
     is_dvr_ms_audio = 1;
     get_guid ( lpFileCtx, lGUID );
     File_GetUInt ( lpFileCtx );
     File_GetUInt ( lpFileCtx );
     File_GetUInt ( lpFileCtx );
     get_guid ( lpFileCtx, lGUID );
     File_GetUInt ( lpFileCtx );
    }  /* end if */
   }  /* end if */

   lpStm -> m_pCodec -> m_Type = lCodecType;

   if ( lCodecType == SMS_CodecTypeAudio ) {

    SMSContainer_GetWAVHeader ( lpFileCtx, lpStm -> m_pCodec, type_specific_size );

    if ( is_dvr_ms_audio ) {
     lpStm -> m_pCodec -> m_ID  = SMS_CodecID_NULL;
     lpStm -> m_pCodec -> m_Tag = 0;
    }  /* end if */

    pos2 = lpFileCtx -> m_CurPos;

    if (  lGSize >= ( pos2 + 8 - pos1 + 24 )  ) {
     lpASFStm -> ds_span        = File_GetByte   ( lpFileCtx );
     lpASFStm -> ds_packet_size = File_GetUShort ( lpFileCtx );
     lpASFStm -> ds_chunk_size  = File_GetUShort ( lpFileCtx );
     File_GetUShort ( lpFileCtx );
     File_GetByte   ( lpFileCtx );
    }  /* end if */

    if ( lpASFStm -> ds_span > 1 ) {
     if ( lpASFStm -> ds_chunk_size                              == 0 ||
          lpASFStm -> ds_packet_size / lpASFStm -> ds_chunk_size <= 1 ||
          lpASFStm -> ds_packet_size % lpASFStm -> ds_chunk_size != 0
     ) lpASFStm -> ds_span = 0;
    }  /* end if */

    if ( lpStm -> m_pCodec -> m_ID == SMS_CodecID_MP3 )
     lpStm -> m_pCodec -> m_FrameSize = 1152;
    else lpStm -> m_pCodec -> m_FrameSize = 1;

    lpStm -> m_Flags |= SMS_STRM_FLAGS_AUDIO;

   } else if ( lCodecType == SMS_CodecTypeVideo ) {

    File_GetUInt ( lpFileCtx );
    File_GetUInt ( lpFileCtx );
    File_GetByte ( lpFileCtx );
    lSize = File_GetUShort ( lpFileCtx );
    sizeX = File_GetUInt   ( lpFileCtx );
    lpStm -> m_pCodec -> m_Width  = File_GetUInt ( lpFileCtx );
    lpStm -> m_pCodec -> m_Height = File_GetUInt ( lpFileCtx );
    File_GetUShort ( lpFileCtx );
    lpStm -> m_pCodec -> m_BitsPerSample = File_GetUShort ( lpFileCtx );
    tag1 = File_GetUInt ( lpFileCtx );
    File_Skip ( lpFileCtx, 20 );
    lSize = sizeX;

    if ( lSize > 40 ) File_Skip ( lpFileCtx, lSize - 40 );

    lpStm -> m_pCodec -> m_Tag = tag1;
    lpStm -> m_pCodec -> m_ID  = SMS_CodecGetID ( SMS_CodecTypeVideo, tag1 );

   }  /* end if */

   pos2 = lpFileCtx -> m_CurPos;
   File_Skip (   lpFileCtx, ( uint32_t )(  lGSize - ( pos2 - pos1 + 24 )  )   );

  } else if (  guid_equal ( lGUID, s_GUID[ STREAM_BITRATE ] )  ) {

   int j, lnStm = File_GetUShort ( lpFileCtx );

   for ( j = 0; j < lnStm; ++j ) {
    int lFlags   = File_GetUShort ( lpFileCtx );
    int lBitRate = File_GetUInt   ( lpFileCtx );
    int lStmID   = lFlags * 0x7F;
    lpASF -> stream_bitrates[ lStmID ] = lBitRate;
   }  /* end for */

  } else if (  !FILE_EOF( lpFileCtx )  )

   File_Skip (  lpFileCtx, ( uint32_t )( lGSize - 24 )  );

  else goto fail;

 }  /* end while */

 File_Skip ( lpFileCtx, 26 );

 if (  FILE_EOF ( lpFileCtx )  ) goto fail;

 lpASF -> data_offset      = lpFileCtx -> m_CurPos;
 lpASF -> packet_size_left = 0;

 for ( i = 0; i < 128; ++i ) {
  int stream_num = lpASF -> asfid2avid[ i ];
  if ( stream_num >= 0 ) {
   SMS_CodecContext* lpCodec = apCont -> m_pStm[ stream_num ] -> m_pCodec;
   if ( !lpCodec -> m_BitRate ) lpCodec -> m_BitRate = lBitRate[ i ];
  }  /* end if */
 }  /* end for */

 return 1;
fail:
 SMS_DestroyContainer ( apCont, 0 );

 return 0;

}  /* end asf_read_header */

static int asf_get_packet ( ASFContext* apASF, FileContext* apFileCtx ) {

 uint32_t packet_length, padsize;
 int      rsize = 8;
 int      c, d, e, off;

 off = ( int )(  ( apFileCtx -> m_CurPos - apASF -> data_offset ) % apASF -> packet_size + 3  );
 c   = d = e = -1;

 while ( off-- > 0 ) {
  c = d;
  d = e;
  e = File_GetByte ( apFileCtx );
  if ( c == 0x82 && !d && !e ) break;
 }  /* end while */

 if (  ( c & 0x8F ) == 0x82  ) {
  if ( d || e ) return -1;
  c = File_GetByte ( apFileCtx );
  d = File_GetByte ( apFileCtx );
  rsize += 3;
 } else apFileCtx -> Seek ( apFileCtx, apFileCtx -> m_CurPos - 1 );

 apASF -> packet_flags    = c;
 apASF -> packet_property = d;

 DO_2BITS( apFileCtx, apASF -> packet_flags >> 5, packet_length, apASF -> packet_size );
 DO_2BITS( apFileCtx, apASF -> packet_flags >> 1, padsize, 0                          );
 DO_2BITS( apFileCtx, apASF -> packet_flags >> 3, padsize, 0                          );

 if (  packet_length >= ( 1U << 29 )  ) return -1;
 if ( padsize >= packet_length        ) return -1;

 apASF -> packet_timestamp = File_GetUInt ( apFileCtx );

 File_GetUShort ( apFileCtx );

 if ( apASF -> packet_flags & 0x01 ) {
  apASF -> packet_segsizetype = File_GetByte ( apFileCtx ); ++rsize;
  apASF -> packet_segments    = apASF -> packet_segsizetype & 0x3F;
 } else {
  apASF -> packet_segments    = 1;
  apASF -> packet_segsizetype = 0x80;
 }  /* end else */

 apASF -> packet_size_left = packet_length - padsize - rsize;

 if ( packet_length < apASF -> hdr.min_pktsize ) padsize += apASF -> hdr.min_pktsize - packet_length;

 apASF -> packet_padsize = padsize;

 return 0;

}  /* end asf_get_packet */

static int asf_read_frame_header ( ASFContext* apASF, FileContext* apFileCtx ) {

 int     rsize = 1;
 int     num   = File_GetByte ( apFileCtx );
 int64_t ts0, ts1;

 apASF -> packet_segments -= 1;
 apASF -> packet_key_frame = num >> 7;
 apASF -> stream_index     = apASF -> asfid2avid[ num & 0x7F ];

 DO_2BITS( apFileCtx, apASF -> packet_property >> 4, apASF -> packet_seq,         0 );
 DO_2BITS( apFileCtx, apASF -> packet_property >> 2, apASF -> packet_frag_offset, 0 );
 DO_2BITS( apFileCtx, apASF -> packet_property,      apASF -> packet_replic_size, 0 );

 if ( apASF -> packet_replic_size >= 8 ) {
  apASF -> packet_obj_size = File_GetUInt ( apFileCtx );
  if ( apASF -> packet_obj_size >= ( 1 << 24 ) || apASF -> packet_obj_size <= 0 ) return -1;
  apASF -> packet_frag_timestamp = File_GetUInt ( apFileCtx );
  if ( apASF -> packet_replic_size >= 8 + 38 + 4 ) {
   File_Skip ( apFileCtx, 10 );
   ts0 = File_GetULong( apFileCtx );
   ts1 = File_GetULong( apFileCtx );
   File_Skip ( apFileCtx, 12 );
   File_GetUInt ( apFileCtx );
   File_Skip ( apFileCtx, apASF -> packet_replic_size - 8 - 38 - 4 );
   if ( ts0 != -1 )
    apASF -> packet_frag_timestamp= ts0 / 10000;
   else apASF -> packet_frag_timestamp = SMS_NOPTS_VALUE;
  } else File_Skip ( apFileCtx, apASF -> packet_replic_size - 8 );
  rsize += apASF -> packet_replic_size;
 } else if ( apASF -> packet_replic_size == 1 ) {
  apASF -> packet_time_start     = apASF -> packet_frag_offset;
  apASF -> packet_frag_offset    = 0;
  apASF -> packet_frag_timestamp = apASF -> packet_timestamp;
  apASF -> packet_time_delta     = File_GetByte ( apFileCtx );
  ++rsize;
 } else if ( apASF -> packet_replic_size != 0 ) return -1;

 if ( apASF -> packet_flags & 0x01 ) {
  DO_2BITS( apFileCtx, apASF -> packet_segsizetype >> 6, apASF -> packet_frag_size, 0 );
  if ( apASF -> packet_frag_size > apASF -> packet_size_left - rsize ) return -1;
 } else apASF -> packet_frag_size = apASF -> packet_size_left - rsize;

 if ( apASF -> packet_replic_size == 1 ) {
  apASF -> packet_multi_size = apASF -> packet_frag_size;
  if ( apASF -> packet_multi_size > apASF -> packet_size_left ) return -1;
 }  /* end if */

 apASF -> packet_size_left -= rsize;

 return 0;

}  /* end asf_read_frame_header */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 FileContext*  lpFileCtx = apCont -> m_pFileCtx;
 ASFContext*   lpASF     = ( ASFContext* )apCont -> m_pCtx;
 SMS_Stream*   lpStm     = NULL;
 ASFStream*    lpASFStm  = NULL;
 SMS_AVPacket* lpPkt;

 while (  !FILE_EOF( lpFileCtx )  ) {

  if (  lpASF -> packet_size_left < FRAME_HEADER_SIZE || lpASF -> packet_segments < 1 ) {
   int ret = lpASF -> packet_size_left + lpASF -> packet_padsize;
   File_Skip ( lpFileCtx, ret );
   lpASF -> packet_pos = lpFileCtx -> m_CurPos;
   if ( lpASF -> data_object_size != ( uint64_t )-1 && (
         lpASF -> packet_pos - lpASF -> data_object_offset >= lpASF -> data_object_size
        )
   ) break;
   ret = asf_get_packet ( lpASF, lpFileCtx );
   lpASF -> packet_time_start = 0;
   continue;
  }  /* end if */

  if ( lpASF -> packet_time_start == 0 ) {
   if (  asf_read_frame_header ( lpASF, lpFileCtx ) < 0 ) {
    lpASF -> packet_segments = 0;
    continue;
   }  /* end if */
   if ( lpASF -> stream_index < 0 ) {
    lpASF -> packet_time_start = 0;
    File_Skip ( lpFileCtx, lpASF -> packet_frag_size );
    lpASF -> packet_size_left -= lpASF -> packet_frag_size;
    continue;
   }  /* end if */
   lpStm    = apCont -> m_pStm[ lpASF -> stream_index ];
   lpASFStm = ( ASFStream* )lpStm -> m_pCtx;
  }  /* end if */

  if ( lpASF -> packet_replic_size == 1 ) {
   lpASF -> packet_frag_timestamp = lpASF -> packet_time_start;
   lpASF -> packet_time_start    += lpASF -> packet_time_delta;
   lpASF -> packet_obj_size       = lpASF -> packet_frag_size = File_GetByte ( lpFileCtx );
   lpASF -> packet_size_left     -= 1;
   lpASF -> packet_multi_size    -= 1;
   if ( lpASF -> packet_multi_size < lpASF -> packet_obj_size ) {
    lpASF -> packet_time_start = 0;
    File_Skip ( lpFileCtx, lpASF -> packet_multi_size );
    lpASF -> packet_size_left -= lpASF -> packet_multi_size;
    continue;
   }  /* end if */
   lpASF -> packet_multi_size -= lpASF -> packet_obj_size;
  }  /* end if */

  if ( lpASFStm -> frag_offset + lpASF -> packet_frag_size <= lpASFStm -> pkt.size &&
       lpASFStm -> frag_offset + lpASF -> packet_frag_size >  lpASF -> packet_obj_size
  ) lpASF -> packet_obj_size = lpASFStm -> pkt.size;

  if ( lpASFStm -> pkt.size                                != lpASF -> packet_obj_size ||
       lpASFStm -> frag_offset + lpASF -> packet_frag_size >  lpASFStm -> pkt.size
  ) {
   if ( lpASFStm -> pkt.data ) {
    lpASFStm -> frag_offset = 0;
    lpASFStm -> pkt.size    = 0;
   }  /* end if */
   lpASFStm -> pkt.size = lpASF -> packet_obj_size;
   if (  lpASFStm -> pkt.data == NULL || ( lpASFStm -> pkt.alloc < lpASFStm -> pkt.size )  )
    lpASFStm -> pkt.data = ( uint8_t* )realloc ( lpASFStm -> pkt.data, lpASFStm -> pkt.alloc = lpASFStm -> pkt.size );
   lpASFStm -> seq              = lpASF -> packet_seq;
   lpASFStm -> pkt.pts          = lpASF -> packet_frag_timestamp;
   lpASFStm -> pkt.stream_index = lpASF -> stream_index;
   lpASFStm -> pkt.pos          =
   lpASFStm -> packet_pos       = lpASF -> packet_pos;
   if (  lpStm -> m_pCodec -> m_Type == SMS_CodecTypeAudio )
    lpASF -> packet_key_frame = 1;
   if ( lpASF -> packet_key_frame ) lpASFStm -> pkt.flags |= SMS_PKT_FLAG_KEY;
  }  /* end if */

  lpASF -> packet_size_left -= lpASF -> packet_frag_size;

  if ( lpASF -> packet_size_left < 0 ) continue;

  if ( lpASF -> packet_frag_offset >= lpASFStm -> pkt.size ||
       lpASF -> packet_frag_size   >  lpASFStm -> pkt.size - lpASF -> packet_frag_offset
  ) continue;

  if (  !lpFileCtx -> Read (
          lpFileCtx, lpASFStm -> pkt.data + lpASF -> packet_frag_offset, lpASF -> packet_frag_size
         )
  ) break;

  lpASFStm -> frag_offset += lpASF -> packet_frag_size;

  if ( lpASFStm -> frag_offset == lpASFStm -> pkt.size ) {

   if (  lpStm -> m_pCodec -> m_ID == SMS_CodecID_MPEG2 && lpASFStm -> pkt.size > 100 ) {
    int i;
    for ( i = 0; i < lpASFStm -> pkt.size && !lpASFStm -> pkt.data[ i ]; ++i );
    if ( i == lpASFStm -> pkt.size ) {
     lpASFStm -> frag_offset = 0;
     lpASFStm -> pkt.size    = 0;
     continue;
    }  /* end if */
   }  /* end if */

   if ( !lpStm -> m_pPktBuf ) continue;

   if ( lpASFStm -> ds_span > 1 && lpASFStm -> pkt.size == lpASFStm -> ds_packet_size * lpASFStm -> ds_span ) {

    uint8_t* newdata;
    int      offset;

    lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lpASFStm -> pkt.size );

    if ( !lpPkt ) break;

    newdata = lpPkt -> m_pData;
    offset  = 0;

    while ( offset < lpASFStm -> pkt.size ) {
     int off = offset / lpASFStm -> ds_chunk_size;
     int row = off    / lpASFStm -> ds_span;
     int col = off    % lpASFStm -> ds_span;
     int idx = row + col * lpASFStm -> ds_packet_size / lpASFStm -> ds_chunk_size;
     memcpy (
      newdata + offset, lpASFStm -> pkt.data + idx * lpASFStm -> ds_chunk_size,
      lpASFStm -> ds_chunk_size
     );
     offset += lpASFStm -> ds_chunk_size;
    }  /* end while */

   } else {

    lpPkt = apCont -> AllocPacket ( lpStm -> m_pPktBuf, lpASFStm -> pkt.size );

    if ( !lpPkt ) break;

    memcpy ( lpPkt -> m_pData, lpASFStm -> pkt.data, lpASFStm -> pkt.size );

   }  /* end else */

   lpASFStm -> frag_offset = 0;
   lpASFStm -> pkt.size    = 0;

   lpPkt -> m_PTS    = lpASFStm -> pkt.pts;
   lpPkt -> m_StmIdx = *apIdx = lpASF -> stream_index;
   lpPkt -> m_Flags  = lpASFStm -> pkt.flags;

   SMSContainer_CalcPktFields ( lpStm, lpPkt );

   return lpPkt -> m_Size;

  }  /* end if */

 }  /* end while */

 if ( lpASFStm ) lpASFStm -> pkt.size = 0;

 return -1;

}  /* end _ReadPacket */

static void _Destroy ( SMS_Container* apCont, int afAll ) {

 uint32_t i;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {
  ASFStream* lpStm = MYSTRM( apCont -> m_pStm[ i ] );
  if ( lpStm && lpStm -> pkt.data ) free ( lpStm -> pkt.data );
 }  /* end for */

 SMS_DestroyContainer ( apCont, afAll );

}  /* end _Destroy */

uint64_t SMS_WMAProbe ( FileContext* apFileCtx, SMS_AudioInfo* apInfo ) {

 SMS_GUID lGUID;
 int64_t  lGSize;
 uint64_t retVal = 0;

 get_guid ( apFileCtx, lGUID );

 if (  guid_equal ( lGUID, s_GUID[ ASF_HEADER ] )  ) {

  File_Skip ( apFileCtx, 14 );

  while (  !FILE_EOF( apFileCtx )  ) {

   get_guid ( apFileCtx, lGUID );

   lGSize = File_GetULong ( apFileCtx );

   if (  lGSize < 24 || guid_equal ( lGUID, s_GUID[ DATA_HEADER ] )  ) break;

   if (  guid_equal ( lGUID, s_GUID[ STREAM_HEADER ] )  ) {

    get_guid ( apFileCtx, lGUID );

    if (  guid_equal ( lGUID, s_GUID[ AUDIO_STREAM ] )  ) {

     File_Skip ( apFileCtx, 40 );

     apInfo -> m_nChannels  = File_GetUShort ( apFileCtx );
     apInfo -> m_SampleRate = File_GetUInt   ( apFileCtx );
     apInfo -> m_BitRate    = File_GetUInt   ( apFileCtx ) * 8;

     retVal = 1;
     break;

    } else File_Skip (  apFileCtx, ( uint32_t )( lGSize - 40 )  );

   } else File_Skip (  apFileCtx, ( uint32_t )( lGSize - 24 )  );

  }  /* end while */

 }  /* end if */

 apFileCtx -> Seek ( apFileCtx, 0 );

 return retVal;
 
}  /* end SMS_WMAProbe */

int SMS_GetContainerASF ( SMS_Container* apCont ) {

 int i, retVal = 0;

 if (  ( int )apCont -> m_pFileCtx <= 0  ) return retVal;

 if (   (  retVal = asf_read_header ( apCont )  )   ) {

  int64_t lDuration = 0;

  apCont -> m_pName    = g_pASFStr;
  apCont -> ReadPacket = _ReadPacket;
  apCont -> Destroy    = _Destroy;

  for ( i = 0; i < apCont -> m_nStm; ++i )
   if ( apCont -> m_pStm[ i ] -> m_Duration != SMS_NOPTS_VALUE &&
        apCont -> m_pStm[ i ] -> m_Duration  > lDuration
   ) lDuration = apCont -> m_pStm[ i ] -> m_Duration;

  retVal = SMSContainer_SetName ( apCont, apCont -> m_pFileCtx );

  if ( lDuration ) apCont -> m_Duration = lDuration / SMS_TIME_BASE;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerASF */

