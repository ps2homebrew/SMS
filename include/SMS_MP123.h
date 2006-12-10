/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
# Copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
#           2005-2006 Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licensed (like the original mpg123 and ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#ifndef __SMS_MP123_H
# define __SMS_MP123_H

# ifndef __SMS_Bitio_H
#  include "SMS_Bitio.h"
# endif  /* __SMS_Bitio_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

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

extern const uint16_t g_mpa_freq_tab   [ 3 ];
extern const uint16_t g_mpa_bitrate_tab[ 2 ][ 3 ][ 15 ];

typedef struct SMS_MP3GranInfo {

 int          m_SCFSI;
 unsigned int m_Part23Len;
 unsigned int m_BigValues;
 unsigned int m_ScaleFaCompress;
 unsigned int m_BlockType;
 unsigned int m_fMixedBlock;
 unsigned int m_TabSelect   [ 3 ];
 unsigned int m_SubBlockGain[ 3 ];
 unsigned int m_MaxBand     [ 3 ];
 unsigned int m_MaxBandL;
 unsigned int m_MaxB;
 unsigned int m_Reg1Start;
 unsigned int m_Reg2Start;
 unsigned int m_fPre;
 unsigned int m_ScaleFacScale;
 unsigned int m_Count1TblSel;
 float*       m_pFullGain[ 3 ];
 float*       m_pPow2Gain;

} SMS_MP3GranInfo;

typedef struct SMS_MP2AllocTable {

 short m_Bits;
 short m_D;

} SMS_MP2AllocTable;

typedef struct SMS_MP3SideInfo {

 unsigned int m_MainDataBegin;
 unsigned int m_PrivBits;
 struct {
  SMS_MP3GranInfo m_GI[ 2 ];
 } m_Ch[ 2 ];

} SMS_MP3SideInfo;

typedef struct SMS_Codec_MPAContext {

 uint8_t            m_InBuf[ 2 ][ SMS_MPA_MAX_CODED_FRAME_SIZE + SMS_BACKSTEP_SIZE ];
 SMS_BitContext     m_BitCtx;
 unsigned int       m_FreeFmtNextHdr;
 int                m_InBufIdx;
 int                m_FrameSize;
 int                m_LSF;
 int                m_Layer;
 int                m_SampleRateIdx;
 int                m_ErrorProtection;
 int                m_SampleRate;
 int                m_Mode;
 int                m_ModeExt;
 int                m_nChannels;
 int                m_BitRate;
 int                m_BitRateIdx;
 int                m_FreeFmtFrameSize;
 int                m_OldFrameSize;
 SMS_MP2AllocTable* m_pAlloc;
 int                m_2SBLinit;
 int                m_JSBound;
 unsigned char*     m_pInBufPtr;
 unsigned char*     m_pInBuf;
 unsigned char*     m_pPos;
 int                m_Len;

} SMS_Codec_MPAContext;

extern SMS_Codec_MPAContext g_MPACtx;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_MP123_Open ( SMS_CodecContext* );
int  MP123_CheckHeader    ( uint32_t );
int  MP123_DecodeHeader   ( uint32_t );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MP123_H */
