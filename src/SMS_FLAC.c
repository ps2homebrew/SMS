/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
# Don't remember where the original source code comes from.
# Please adjust the aforementioned copyright statement if needed.
#
*/
#include "SMS_FLAC.h"
#include "SMS_Bitio.h"
#include "SMS_Locale.h"
#include "SMS_RingBuffer.h"

#include <limits.h>
#include <malloc.h>
#include <string.h>

#ifndef min
# define min( a, b ) (   (  ( a ) < ( b )  ) ? ( a ) : ( b )   )
#endif  /* min */

typedef enum FLACDecorrType {
 FLAC_DT_INDEPENDENT,
 FLAC_DT_LEFT_SIDE,
 FLAC_DT_RIGHT_SIDE,
 FLAC_DT_MID_SIDE
} FLACDecorrType;

typedef struct FLAContext {

 FLACData       m_Data;
 uint8_t*       m_pBS;
 int            m_BSize;
 int            m_BSIdx;
 SMS_BitContext m_BitCtx;
 int32_t*       m_pDec[ 2 ];
 int            m_nAlloc;
 int            m_BlockSize;
 int            m_SampleRate;
 int            m_Decor;
 int            m_CurBPS;
 int            m_nAllocFrm;

} FLAContext;

static FLAContext s_Ctx __attribute__(   (  section( ".data" )  )   );

static uint32_t _crc ( uint32_t crc, const uint8_t *buffer, size_t length ) {
 static const unsigned char ctx[ 257 ] __attribute__(   (  section( ".rodata" )  )   ) = {
  0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15, 0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
  0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65, 0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
  0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5, 0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
  0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85, 0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
  0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2, 0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
  0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2, 0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
  0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32, 0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
  0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42, 0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
  0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C, 0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
  0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC, 0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
  0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C, 0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
  0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C, 0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
  0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B, 0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
  0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B, 0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
  0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB, 0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
  0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB, 0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3, 0x01
 };
 const uint8_t* end = buffer + length;
 while ( buffer < end ) crc = ctx[  (  ( uint8_t )crc  ) ^ *buffer++ ] ^ ( crc >> 8 );
 return crc;
}  /* end _crc */

static int64_t _get_utf8 ( SMS_BitContext* apCtx ) {
 int64_t retVal = SMS_GetBits ( apCtx, 8 );
 int     lOnes  = 7 - SMS_log2 (  ( unsigned int )( retVal ^ 255 )  );
 if ( lOnes == 1 ) return -1;
 retVal &= 127 >> lOnes;
 while ( --lOnes > 0 ){
  int lTmp= SMS_GetBits ( apCtx, 8 ) - 128;
  if ( lTmp >> 6 ) return -1;
  retVal= ( retVal << 6 ) + lTmp;
 }  /* end while */
 return retVal;
}  /* end _get_utf8 */

static SMS_INLINE int get_ur_golomb_jpegls ( SMS_BitContext* gb, int k, int limit, int esc_len ) {

 unsigned int buf;
 int          log;

 SMS_OPEN_READER( re, gb );
 SMS_UPDATE_CACHE( re, gb );
 buf = SMS_GET_CACHE( re, gb );
 log = SMS_log2 ( buf );

 if ( log > 20 ) {
  buf >>= log - k;
  buf  += ( 30 - log ) << k;
  SMS_LAST_SKIP_BITS( re, gb, 32 + k - log );
  SMS_CLOSE_READER( re, gb );
  return buf;
 } else {
  int i;
  for ( i = 0; SMS_SHOW_UBITS( re, gb, 1 ) == 0; ++i ) {
   SMS_LAST_SKIP_BITS( re, gb, 1 );
   SMS_UPDATE_CACHE( re, gb );
  }  /* end for */
  SMS_SKIP_BITS( re, gb, 1 );
  if ( i < limit - 1 ) {
   if ( k ) {
    buf = SMS_SHOW_UBITS( re, gb, k );
    SMS_LAST_SKIP_BITS( re, gb, k );
   } else buf = 0;
   SMS_CLOSE_READER( re, gb );
   return buf + ( i << k );
  } else if ( i == limit - 1 ) {
   buf = SMS_SHOW_UBITS( re, gb, esc_len );
   SMS_LAST_SKIP_BITS( re, gb, esc_len );
   SMS_CLOSE_READER( re, gb );
   return buf + 1;
  } else return -1;
 }  /* end else */

}  /* end get_ur_golomb_jpegls */

static SMS_INLINE int get_sr_golomb_flac ( SMS_BitContext* gb, int k, int limit, int esc_len ) {
 int v = get_ur_golomb_jpegls ( gb, k, limit, esc_len );
 return ( v >> 1 ) ^ -( v & 1 );
}  /* end get_sr_golomb_flac */

static int decode_residuals ( FLAContext* apCtx, SMS_BitContext* apBitCtx, int channel, int pred_order ) {

 int i, tmp, partition, method_type, rice_order;
 int sample = 0, samples;

 method_type = SMS_GetBits ( apBitCtx, 2 );

 if ( method_type > 1 ) return 0;

 rice_order = SMS_GetBits ( apBitCtx, 4 );

 samples = apCtx -> m_BlockSize >> rice_order;

 if ( pred_order > samples ) return 0;

 sample = i = pred_order;

 for (  partition = 0; partition < ( 1 << rice_order ); ++partition  ) {

  tmp = SMS_GetBits ( apBitCtx, method_type == 0 ? 4 : 5 );

  if (  tmp == ( method_type == 0 ? 15 : 31 )  ) {
   tmp = SMS_GetBits ( apBitCtx, 5 );
   for ( ; i < samples; ++i, ++sample ) apCtx -> m_pDec[ channel ][ sample ] = SMS_GetSBits ( apBitCtx, tmp );
  } else for ( ; i < samples; ++i, ++sample ) apCtx -> m_pDec[ channel ][ sample ] = get_sr_golomb_flac ( apBitCtx, tmp, INT_MAX, 0 );

  i = 0;

 }  /* end for */

 return 1;

}  /* end decode_residuals */

static int decode_subframe_fixed ( FLAContext* apCtx, SMS_BitContext* apBitCtx, int channel, int pred_order ) {

 const int blocksize = apCtx -> m_BlockSize;
 int32_t*  decoded   = apCtx -> m_pDec[ channel ];
 int       a, b, c, d, i;

 a = apCtx -> m_CurBPS;

 for ( i = 0; i < pred_order; ++i ) decoded[ i ] = SMS_GetSBits ( apBitCtx, a );

 if (  !decode_residuals ( apCtx, apBitCtx, channel, pred_order )  ) return 0;

 a = decoded[ pred_order - 1 ];
 b = a - decoded[ pred_order - 2 ];
 c = b - decoded[ pred_order - 2 ] + decoded[ pred_order - 3 ];
 d = c - decoded[ pred_order - 2 ] + 2 * decoded[ pred_order - 3 ] - decoded[ pred_order - 4 ];

 switch ( pred_order ) {
  case 0: break;
  case 1: for ( i = pred_order; i < blocksize; ++i ) decoded[ i ] = a += decoded[ i ]; break;
  case 2: for ( i = pred_order; i < blocksize; ++i ) decoded[ i ] = a += b += decoded[ i ]; break;
  case 3: for ( i = pred_order; i < blocksize; ++i ) decoded[ i ] = a += b += c += decoded[ i ]; break;
  case 4: for ( i = pred_order; i < blocksize; ++i ) decoded[ i ] = a += b += c += d += decoded[ i ]; break;
  default: return 0;
 }  /* end switch */

 return 1;

}  /* end decode_subframe_fixed */

static int decode_subframe_lpc ( FLAContext* apCtx, SMS_BitContext* apBitCtx, int channel, int pred_order ) {

 int      i, j, coeff_prec, qlevel;
 int      coeffs[ pred_order ];
 int32_t* decoded = apCtx -> m_pDec[ channel ];

 j = apCtx -> m_CurBPS;

 for ( i = 0; i < pred_order; ++i ) decoded[ i ] = SMS_GetSBits ( apBitCtx, j );

 coeff_prec = SMS_GetBits ( apBitCtx, 4 ) + 1;

 if ( coeff_prec == 16 ) return 0;

 qlevel = SMS_GetSBits ( apBitCtx, 5 );

 if ( qlevel < 0 ) return 0;

 for ( i = 0; i < pred_order; ++i ) coeffs[ i ] = SMS_GetSBits ( apBitCtx, coeff_prec );

 if (  !decode_residuals ( apCtx, apBitCtx, channel, pred_order )  ) return 0;

 if ( apCtx -> m_Data.m_BPS > 16 ) {
  int64_t sum;
  for ( i = pred_order; i < apCtx -> m_BlockSize; ++i ) {
   sum = 0;
   for ( j = 0; j < pred_order; ++j ) sum += ( int64_t )coeffs[ j ] * decoded[ i - j - 1 ];
   decoded[ i ] += ( int )( sum >> qlevel );
  }  /* end for */
 } else {
  for ( i = pred_order; i < apCtx -> m_BlockSize - 1; i += 2 ) {
   int c;
   int d = decoded[ i - pred_order ];
   int s0 = 0, s1 = 0;
   for ( j = pred_order-1; j > 0; --j ) {
    c   = coeffs[ j ];
    s0 += c * d;
    d   = decoded[ i - j ];
    s1 += c * d;
   }  /* end for */
   c   = coeffs[ 0 ];
   s0 += c * d;
   d   = decoded[ i ] += s0 >> qlevel;
   s1 += c * d;
   decoded[ i + 1 ] += s1 >> qlevel;
  }  /* end for */
  if ( i < apCtx -> m_BlockSize ) {
   int sum = 0;
   for ( j = 0; j < pred_order; ++j ) sum += coeffs[ j ] * decoded[ i - j - 1 ];
   decoded[ i ] += sum >> qlevel;
  }  /* end if */
 }  /* end else */

 return 1;

}  /* end decode_subframe_lpc */

static int _decode_subframe ( FLAContext* apCtx, SMS_BitContext* apBitCtx, int channel ) {

 int type, wasted = 0;
 int i, lBlockSize, tmp;

 apCtx -> m_CurBPS = apCtx ->m_Data.m_BPS;

 if ( channel == 0 ) {
  if ( apCtx -> m_Decor == FLAC_DT_RIGHT_SIDE ) ++apCtx -> m_CurBPS;
 } else {
  if ( apCtx -> m_Decor == FLAC_DT_LEFT_SIDE ||
       apCtx -> m_Decor == FLAC_DT_MID_SIDE
  ) ++apCtx -> m_CurBPS;
 }  /* end else */

 if (  SMS_GetBit ( apBitCtx )  ) return 0;

 type       = SMS_GetBits ( apBitCtx, 6 );
 lBlockSize = apCtx -> m_BlockSize;

 if ( SMS_GetBit ( apBitCtx )  ) {
  wasted = 1;
  while (  !SMS_GetBit ( apBitCtx )  ) ++wasted;
  apCtx -> m_CurBPS -= wasted;
 }  /* end if */

 if ( type == 0 ) {
  tmp = SMS_GetSBits ( apBitCtx, apCtx -> m_CurBPS );
  for ( i = 0; i < lBlockSize; ++i ) apCtx -> m_pDec[ channel ][ i ] = tmp;
 } else if ( type == 1 ) {
  for ( i = 0; i < lBlockSize; ++i ) apCtx -> m_pDec[ channel ][ i ] = SMS_GetSBits ( apBitCtx, apCtx -> m_CurBPS );
 } else if ( type >= 8 && type <= 12 ) {
  if (  !decode_subframe_fixed ( apCtx, apBitCtx, channel, type & ~0x8 )  ) return 0;
 } else if ( type >= 32 ) {
  if (  !decode_subframe_lpc ( apCtx, apBitCtx, channel, ( type & ~0x20 ) + 1 )  ) return 0;
 } else return 0;

 if ( wasted ) for ( i = 0; i < lBlockSize; ++i ) apCtx -> m_pDec[ channel ][ i ] <<= wasted;

 return 1;

}  /* end _decode_subframe */

static int _decode_frame ( FLAContext* apCtx ) {

 static const uint8_t sample_size_table[] __attribute__(   (  section( ".rodata" )  )   ) = {
  0, 8, 12, 0, 16, 20, 24, 0
 };

 static const uint16_t blocksize_table[] __attribute__(   (  section( ".rodata" )  )   ) = {
    0, 192,  576, 1152, 2304, 4608,     0,     0,
  256, 512, 1024, 2048, 4096, 8192, 16384, 32768
 };

 static const int sample_rate_table[] __attribute__(   (  section( ".rodata" )  )   ) = {
      0,     0,     0,     0, 8000, 16000, 22050, 24000,
  32000, 44100, 48000, 96000,    0,     0,     0,     0
 };

 int             i, blocksize_code, sample_rate_code, sample_size_code, assignment;
 int             decorrelation, bps, blocksize, samplerate;
 SMS_BitContext* lpCtx  = &apCtx -> m_BitCtx;

 blocksize_code   = SMS_GetBits ( lpCtx, 4 );
 sample_rate_code = SMS_GetBits ( lpCtx, 4 );
 assignment       = SMS_GetBits ( lpCtx, 4 );

 if ( assignment < 8 && apCtx -> m_Data.m_Info.m_nChannels == assignment + 1 )
  decorrelation = FLAC_DT_INDEPENDENT;
 else if ( assignment >= 8 && assignment < 11 && apCtx -> m_Data.m_Info.m_nChannels == 2 )
  decorrelation = FLAC_DT_LEFT_SIDE + assignment - 8;
 else return 0;

 sample_size_code = SMS_GetBits ( lpCtx, 3 );

 if ( sample_size_code == 0 )
  bps = apCtx -> m_Data.m_BPS;
 else if ( sample_size_code != 3 && sample_size_code != 7 )
  bps = sample_size_table[ sample_size_code ];
 else return 0;

 if (  SMS_GetBit ( lpCtx )     ) return 0;
 if (  _get_utf8  ( lpCtx ) < 0 ) return 0;

 if ( blocksize_code == 0 )
  blocksize = apCtx -> m_Data.m_MinBlockSz;
 else if ( blocksize_code == 6 )
  blocksize = SMS_GetBits ( lpCtx, 8 ) + 1;
 else if ( blocksize_code == 7 )
  blocksize = SMS_GetBits ( lpCtx, 16 ) + 1;
 else blocksize = blocksize_table[ blocksize_code ];

 if ( blocksize > apCtx -> m_Data.m_MaxBlockSz ) return 0;

 if ( sample_rate_code == 0 )
  samplerate = apCtx -> m_Data.m_Info.m_SampleRate;
 else if ( sample_rate_code > 3 && sample_rate_code < 12 )
  samplerate = sample_rate_table[ sample_rate_code ];
 else if ( sample_rate_code == 12 )
  samplerate = SMS_GetBits ( lpCtx, 8 ) * 1000;
 else if ( sample_rate_code == 13 )
  samplerate = SMS_GetBits ( lpCtx, 16 );
 else if ( sample_rate_code == 14 )
  samplerate = SMS_GetBits ( lpCtx, 16 ) * 10;
 else return 0;

 SMS_SkipBits ( lpCtx, 8 );

 if (  _crc ( 0, lpCtx -> m_pBuf, lpCtx -> m_Idx / 8 )  ) return 0;

 apCtx -> m_BlockSize  = blocksize;
 apCtx -> m_SampleRate = samplerate;
 apCtx -> m_Data.m_BPS = bps;
 apCtx -> m_Decor      = decorrelation;

 decorrelation = apCtx -> m_Data.m_Info.m_nChannels;

 for ( i = 0; i < decorrelation; ++i ) if (  !_decode_subframe ( apCtx, lpCtx, i )  ) return 0;

 SMS_AlignBits ( lpCtx );
 SMS_SkipBits ( lpCtx, 16 );

 return 1;

}  /* end _decode_frame */

static int32_t _flac_init ( SMS_CodecContext* apCtx ) {

 FLAContext* lpCtx = &s_Ctx;

 lpCtx -> m_Data  = *( FLACData* )apCtx -> m_pUserData;
 lpCtx -> m_BSize = 0;
 lpCtx -> m_BSIdx = 0;

 if ( lpCtx -> m_nAlloc < lpCtx -> m_Data.m_MaxBlockSz ) {

  lpCtx -> m_pDec[ 0 ] = ( int32_t* )realloc (  lpCtx -> m_pDec[ 0 ], lpCtx -> m_Data.m_MaxBlockSz * sizeof ( int32_t ) * 2  );
  lpCtx -> m_pDec[ 1 ] = lpCtx -> m_pDec[ 0 ] + lpCtx -> m_Data.m_MaxBlockSz;
  lpCtx -> m_nAlloc    = lpCtx -> m_Data.m_MaxBlockSz;

 }  /* end if */

 if ( lpCtx -> m_nAllocFrm < lpCtx -> m_Data.m_MaxFrameSz ) {

  lpCtx -> m_pBS       = ( uint8_t* )realloc ( lpCtx -> m_pBS, lpCtx -> m_Data.m_MaxFrameSz );
  lpCtx -> m_nAllocFrm = lpCtx -> m_Data.m_MaxFrameSz;

 }  /* end if */

 return 1;

}  /* end _flac_init */

static void _flac_destroy ( SMS_CodecContext* apCtx ) {

 FLAContext* lpCtx = &s_Ctx;

 free ( lpCtx -> m_pBS );
 lpCtx -> m_pBS       = NULL;
 lpCtx -> m_nAllocFrm = 0;
 free ( lpCtx -> m_pDec[ 0 ] );
 lpCtx -> m_pDec[ 0 ] = NULL;
 lpCtx -> m_nAlloc    = 0;

}  /* end _flac_destroy */

static void _pcm_synth_1 ( short* apSamples, int32_t** appInp, int aCount, int aShift ) {
 int      i;
 int32_t* lpInp = appInp[ 0 ];
 for ( i = 0; i < aCount; ++i ) *apSamples++ = ( lpInp[ i ] << aShift ) >> 8;
}  /* end _pcm_synth_1 */

static void _pcm_synth_2 ( short* apSamples, int32_t** appInp, int aCount, int aShift ) {
 int      i;
 int32_t* lpInpL = appInp[ 0 ];
 int32_t* lpInpR = appInp[ 1 ];
 for ( i = 0; i < aCount; ++i, apSamples += 2 ) {
  apSamples[ 0 ] = ( lpInpL[ i ] << aShift ) >> 8;
  apSamples[ 1 ] = ( lpInpR[ i ] << aShift ) >> 8;
 }  /* end for */
}  /* end _pcm_synth_1 */

static void _pcm_synth_ind ( FLAContext* apCtx, short* apOut, int32_t** appInp, int aCount, int aShift ) {
 static void ( *_synth_indep[ 2 ] ) ( short*, int32_t**, int, int ) __attribute__(   (  section( ".data" )  )   ) = {
  _pcm_synth_1, _pcm_synth_2
 };
 _synth_indep[ apCtx -> m_Data.m_Info.m_nChannels - 1 ] (
  apOut, appInp, aCount, aShift
 );
}  /* end _pcm_synth_ind */

static void _pcm_synth_left ( FLAContext* apCtx, short* apOut, int32_t** appInp, int aCount, int aShift ) {
 int i;
 for ( i = 0; i < aCount; ++i ) {
  int a    = appInp[ 0 ][ i ];
  int b    = appInp[ 1 ][ i ];
  *apOut++ = (  ( a     ) << aShift  ) >> 8;
  *apOut++ = (  ( a - b ) << aShift  ) >> 8;
 }  /* end for */
}  /* end _pcm_synth_left */

static void _pcm_synth_right ( FLAContext* apCtx, short* apOut, int32_t** appInp, int aCount, int aShift ) {
 int i;
 for ( i = 0; i < aCount; ++i ) {
  int a    = appInp[ 0 ][ i ];
  int b    = appInp[ 1 ][ i ];
  *apOut++ = (  ( a + b ) << aShift  ) >> 8;
  *apOut++ = (  (     b ) << aShift  ) >> 8;
 }  /* end for */
}  /* end _pcm_synth_right */

static void _pcm_synth_mid ( FLAContext* apCtx, short* apOut, int32_t** appInp, int aCount, int aShift ) {
 int i;
 for ( i = 0; i < aCount; ++i ) {
  int a    = appInp[ 0 ][ i ];
  int b    = appInp[ 1 ][ i ];
  b       += a -= b >> 1;
  *apOut++ = ( b << aShift ) >> 8;
  *apOut++ = ( a << aShift ) >> 8;
 }  /* end for */
}  /* end _pcm_synth_mid */

static void ( *SynthSamples[ 4 ] ) ( FLAContext*, short*, int32_t**, int, int ) __attribute__(   (  section( ".data" )  )   ) = {
 _pcm_synth_ind, _pcm_synth_left, _pcm_synth_right, _pcm_synth_mid
};

static int32_t _flac_decode ( SMS_CodecContext* apCtx, SMS_RingBuffer* apOutput, SMS_RingBuffer* apInput ) {

 SMS_AVPacket* lpPkt  = ( SMS_AVPacket* )apInput -> m_pOut;
 uint8_t*      lpData = lpPkt -> m_pData;
 int32_t       lSize  = lpPkt -> m_Size;
 FLAContext*   lpCtx  = &s_Ctx;

 while ( lSize > 0 ) {

  int i, lISize = min ( lSize, lpCtx -> m_Data.m_MaxFrameSz - lpCtx -> m_BSize );

  if ( lpCtx -> m_BSIdx ) {
   memmove ( lpCtx -> m_pBS, lpCtx -> m_pBS + lpCtx -> m_BSIdx, lpCtx -> m_BSize );
   lpCtx -> m_BSIdx = 0;
  }  /* end if */

  memcpy ( &lpCtx -> m_pBS[ lpCtx -> m_BSize ], lpData, lISize );
  lpCtx -> m_BSize += lISize;

  if ( lpCtx -> m_BSize == lpCtx -> m_Data.m_MaxFrameSz ) {

   SMS_InitGetBits ( &lpCtx -> m_BitCtx, lpCtx -> m_pBS, lpCtx -> m_BSize << 3 );

   if (   (  *( short* )lpCtx -> m_BitCtx.m_pBuf & 0xFEFF  ) != 0xF8FF   ) {

    SMS_BitContext* lpBitCtx = &lpCtx -> m_BitCtx;

    while (  lpBitCtx -> m_Idx < lpBitCtx -> m_szInBits &&
             (  SMS_ShowBits ( lpBitCtx, 16 ) & 0xFFFE  ) != 0xFFF8
    ) SMS_SkipBits ( lpBitCtx, 8 );

   } else {

    SMS_SkipBits ( &lpCtx -> m_BitCtx, 16 );

    if (  _decode_frame ( lpCtx )  ) {

     int      lShift     = 24 - lpCtx -> m_Data.m_BPS;
     int32_t* lpInp[ 2 ] = { lpCtx -> m_pDec[ 0 ], lpCtx -> m_pDec[ 1 ] };
     int      lnParts, lRem, lBlockLen;
     short*   lpSamples;

     lBlockLen = 4096 >> lpCtx -> m_Data.m_Info.m_nChannels;
     lnParts   = lpCtx -> m_BlockSize / lBlockLen;
     lRem      = lpCtx -> m_BlockSize % lBlockLen;

     for ( i = 0; i < lnParts; ++i ) {

      lpSamples  = ( short* )SMS_RingBufferAlloc ( apOutput, 4096 + 80 );
      lpSamples += 32;
      *( int* )lpSamples = 4096;
      lpSamples += 8;

      SynthSamples[ lpCtx -> m_Decor ] (
       lpCtx, lpSamples, lpInp, lBlockLen, lShift
      );
      apOutput -> UserCB ( apOutput );
      lpInp[ 0 ] += lBlockLen;
      lpInp[ 1 ] += lBlockLen;

     }  /* end for */

     if ( lRem ) {

      lnParts    = lRem << lpCtx -> m_Data.m_Info.m_nChannels;
      lpSamples  = ( short* )SMS_RingBufferAlloc ( apOutput, lnParts + 80 );
      lpSamples += 32;
      *( int* )lpSamples = lnParts;
      lpSamples += 8;

      SynthSamples[ lpCtx -> m_Decor ] (
       lpCtx, lpSamples, lpInp, lRem, lShift
      );
      apOutput -> UserCB ( apOutput );

     }  /* end if */

    } else {

     lpCtx -> m_BSize = 0;
     lpCtx -> m_BSIdx = 0;

     goto next;

    }  /* end else */

   }  /* end else */

   i = ( lpCtx -> m_BitCtx.m_Idx + 7 ) / 8;

   lpCtx -> m_BSIdx += i;
   lpCtx -> m_BSize -= i;

  }  /* end else */
next:
  lSize  -= lISize;
  lpData += lISize;

 }  /* end while */

 return 0;

}  /* end _flac_decode */

void SMS_Codec_FLAC_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = g_pFLAC;
 apCtx -> m_pCodec -> Init    = _flac_init;
 apCtx -> m_pCodec -> Decode  = _flac_decode;
 apCtx -> m_pCodec -> Destroy = _flac_destroy;

}  /* end SMS_Codec_FLAC_Open */
