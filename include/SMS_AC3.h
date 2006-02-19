/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on ffmpeg project (no copyright notes in the original source code)
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_AC3_H
# define __SMS_AC3_H

# ifndef __SMS_Bitio_H
#  include "SMS_Bitio.h"
# endif  /* __SMS_Bitio_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# ifndef __SMS_AudioBuffer_H
#  include "SMS_AudioBuffer.h"
# endif  /* __SMS_AudioBuffer_H */

# define AC3_CHANNEL       0
# define AC3_MONO          1
# define AC3_STEREO        2
# define AC3_3F            3
# define AC3_2F1R          4
# define AC3_3F1R          5
# define AC3_2F2R          6
# define AC3_3F2R          7
# define AC3_CHANNEL1      8
# define AC3_CHANNEL2      9
# define AC3_DOLBY        10
# define AC3_CHANNEL_MASK 15
# define AC3_LFE          16
# define AC3_ADJUST_LEVEL 32

# define LEVEL_3DB     0.7071067811865476
# define LEVEL_PLUS3DB 1.4142135623730951
# define LEVEL_45DB    0.5946035575013605
# define LEVEL_6DB     0.5
# define LEVEL_PLUS6DB 2.0

# define DELTA_BIT_REUSE    0
# define DELTA_BIT_NEW      1
# define DELTA_BIT_NONE     2
# define DELTA_BIT_RESERVED 3

# define EXP_REUSE 0
# define EXP_D15   1
# define EXP_D25   2
# define EXP_D45   3

# define HEADER_SIZE 7

# define SAMPLE( x ) ( sample_t )(  ( x ) * ( 1 << 30 )  )
# define LEVEL( x )  ( level_t  )(  ( x ) * ( 1 << 26 )  )
# define CONVERT( acmod, output ) (   (  ( output ) << 3 ) + ( acmod )   )

typedef int32_t sample_t;
typedef int32_t level_t;
typedef int16_t quantizer_t;

typedef struct SMS_BA {

 uint8_t m_BAI;
 uint8_t m_DeltBAE;
 int8_t  m_DeltBA[ 50 ];

} SMS_BA;

typedef struct SMS_ExpBAP {

 uint8_t m_Exp[ 256 ];
 int8_t  m_BAP[ 256 ];

} SMS_ExpBAP;

typedef struct SMS_Codec_AC3Context {

 uint8_t          m_InBuf[ 4096 ];
 SMS_ExpBAP       m_FBWExpBAP[ 5 ];
 SMS_ExpBAP       m_CplExpBAP;
 SMS_ExpBAP       m_LFEExpBAP;
 level_t          m_CplCo[ 5 ][ 18 ];
 SMS_BitContext   m_BitCtx;
 SMS_BA           m_BA[ 5 ];
 SMS_BA           m_CplBA;
 SMS_BA           m_LFEBA;
 uint8_t          m_EndMant[ 5 ];
 sample_t         m_Bias;
 level_t          m_CLev;
 level_t          m_SLev;
 level_t          m_Level;
 level_t          m_DynRng;
 uint32_t         m_CplBndStrc;
 uint32_t         m_BitsLeft;
 uint32_t         m_CurrentWord;
 int              m_FrameSize;
 int              m_Flags;
 int              m_nChannels;
 int              m_Downmixed;
 int              m_Output;
 int              m_DynRnge;
 SMS_AudioBuffer* m_pOutBuffer;
 sample_t*        m_pSamples;
 uint32_t*        m_pBufStart;
 uint8_t*         m_pInBuf;
 uint16_t         m_BAI;
 uint16_t         m_LFSRState;
 uint8_t          m_FSCod;
 uint8_t          m_HalfRate;
 uint8_t          m_ACMmod;
 uint8_t          m_LFEOn;
 uint8_t          m_ChInCpl;
 uint8_t          m_PhsFlgInU;
 uint8_t          m_CplStrtMant;
 uint8_t          m_CplEndMant;
 uint8_t          m_CplStrtBnd;
 uint8_t          m_nCplBnd;
 uint8_t          m_RematFlg;
 uint8_t          m_CSNROffst;
 uint8_t          m_CplFLeak;
 uint8_t          m_CplSLeak;

} SMS_Codec_AC3Context;

static SMS_INLINE int32_t MUL ( sample_t anA, sample_t aB ) {
 int32_t lA = anA;
 int32_t lB = aB;
 int32_t lC = ( lA  & 0xFFFF ) * ( lB >>     16 ) +
              ( lA >>     16 ) * ( lB  & 0xFFFF );
 return ( lC >> 14 ) + (   (  ( lA >> 16 ) * ( lB >> 16 )  ) << 2   );
}  /* end MUL */

static int32_t __inline MUL_L ( int32_t a, int32_t b ) {
 int32_t lA = a;
 int32_t lB = b;
 int32_t lC = ( lA  & 0xFFFF ) * ( lB >> 16     ) +
              ( lA >>     16 ) * ( lB  & 0xFFFF );
 return ( lC >> 10 ) + (   (  ( lA >> 16 ) * ( lB >> 16 )  ) << 6   );
}  /* end MUL_L */

#define COEFF( c, _t, _l, e ) {                                                \
 quantizer_t t     = ( _t );                                                   \
 level_t     l     = ( _l );                                                   \
 int         shift = ( e ) - 5;                                                \
 sample_t    tmp   = t * ( l >> 16 ) + (   (  t * ( l & 0xFFFF )  ) >> 16   ); \
 ( c ) = shift >= 0 ? tmp >> shift : tmp << -shift;                            \
}

#define COMPUTE_MASK() {                                                  \
 if ( lPSD  > lDbKnee                 ) lMask -= ( lPSD - lDbKnee ) >> 2; \
 if ( lMask > lpHtH[ i >> lHalfRate ] ) lMask  = lpHtH[ i >> lHalfRate ]; \
 lMask -= lSnrOffset + 128 * lpDeltBA[ i ];                               \
 lMask  = lMask > 0 ? 0 : (  ( -lMask ) >> 5  );                          \
 lMask -= lFloor;                                                         \
}

#define UPDATE_LEAK() {                                      \
 aFastLeak += lFDecay;                                       \
 if ( aFastLeak > lPSD + lFGain ) aFastLeak = lPSD + lFGain; \
 aSlowLeak += lSDecay;                                       \
 if ( aSlowLeak > lPSD + lSGain ) aSlowLeak = lPSD + lSGain; \
}

# define MUL_C( a, b ) MUL_L(  a, LEVEL( b )  )
# define DIV( a, b )   (    (   (  ( int64_t )LEVEL( a )  ) << 26   ) / ( b )    )

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_AC3_Open ( SMS_CodecContext* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_AC3_H */
