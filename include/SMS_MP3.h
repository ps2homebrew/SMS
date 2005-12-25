/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
#               2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MP3_H
# define __SMS_MP3_H

# ifndef __SMS_Bitio_H
#  include "SMS_Bitio.h"
# endif  /* __SMS_Bitio_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# ifndef __SMS_AudioBuffer_H
#  include "SMS_AudioBuffer.h"
# endif  /* __SMS_AudioBuffer_H */

//# define SMS_USE_HIGHPRECISION

#ifdef SMS_USE_HIGHPRECISION
# define FRAC_BITS  23
# define WFRAC_BITS 16
#else
# define FRAC_BITS  15
# define WFRAC_BITS 14
#endif  /* SMS_USE_HIGHPRECISION */

# define SMS_HEADER_SIZE                 4
# define SMS_MPA_MAX_CODED_FRAME_SIZE 1792
# define SMS_BACKSTEP_SIZE             512
# define SMS_SBLIMIT                    32

# define SMS_MODE_EXT_I_STEREO  1
# define SMS_MODE_EXT_MS_STEREO 2

# define SMS_MPA_STEREO  0
# define SMS_MPA_JSTEREO 1
# define SMS_MPA_DUAL    2
# define SMS_MPA_MONO    3

# define SMS_SAME_HEADER_MASK (  0xFFE00000 | ( 3 << 17 ) | (0xF << 12 ) | ( 3 << 10 ) | ( 3 << 19 )  )

#if FRAC_BITS <= 15
typedef int16_t SMS_MPA_INT;
#else
typedef int32_t SMS_MPA_INT;
#endif  /* FRAC_BITS <= 15 */

extern const uint16_t g_mpa_freq_tab   [ 3 ];
extern const uint16_t g_mpa_bitrate_tab[ 2 ][ 3 ][ 15 ];

typedef struct SMS_GranuleDef {

 int32_t m_SBHybrid    [ 576 ];
 uint8_t m_ScaleFactors[  40 ];
 int     m_RegionSize  [   3 ];
 int     m_TableSelect [   3 ];
 int     m_SubblockGain[   3 ];
 int     m_Part23Len;
 int     m_BigVals;
 int     m_GlobGain;
 int     m_ScaleFacCompress;
 int     m_PreFlag;
 int     m_ShortStart;
 int     m_LongEnd;
 uint8_t m_SCFSI;
 uint8_t m_BlockType;
 uint8_t m_SwitchPoint;
 uint8_t m_ScaleFacScale;
 uint8_t m_Count1TblSelect;

} SMS_GranuleDef;

typedef struct SMS_Codec_MP3Context {

 int32_t          m_SBSamples[ 2 ][ 36 ][ SMS_SBLIMIT ];
 int32_t          m_MDCTBuf  [ 2 ][ SMS_SBLIMIT * 18  ];
 uint8_t          m_InBuf[ 2 ][ SMS_MPA_MAX_CODED_FRAME_SIZE + SMS_BACKSTEP_SIZE ];
 SMS_MPA_INT      m_SynthBuf[ 2 ][ 1024 ];
 SMS_BitContext   m_BitCtx;
 int              m_SynthBuffOffset[ 2 ];
 SMS_AudioBuffer* m_pOutBuffer;
 uint32_t         m_FreeFmtNextHdr;
 int              m_InBufIdx;
 int              m_FreeFmtFrameSize;
 int              m_nChannels;
 int              m_BitRate;
 int              m_SampleRateIdx;
 int              m_ErrorProtection;
 int              m_SampleRate;
 int              m_Mode;
 int              m_ModeExt;
 int              m_Layer;
 int              m_FrameSize;
 int              m_OldFrameSize;
 int              m_LSF;
 uint8_t*         m_pInBufPtr;
 uint8_t*         m_pInBuf;

 void ( *ComputeAntiAlias ) ( SMS_GranuleDef* );

} SMS_Codec_MP3Context;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_MP3_Open ( SMS_CodecContext* );
int  MP3_CheckHeader    ( uint32_t );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MP3_H */
