/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
# Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_MPEG4.h"
#include "SMS_VLC.h"
#include "SMS_H263.h"
#include "SMS_FourCC.h"
#include "SMS_VideoBuffer.h"

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
# include <ctype.h>
#endif  /* _WIN32 */

#define MYCTX()   (  ( SMS_Codec_MPEG4Context* )apCtx -> m_pCodec -> m_pCtx  )
#define BASECTX() g_MPEGCtx

#define STATIC_SPRITE          1
#define GMC_SPRITE             2
#define INTRA_MCBPC_VLC_BITS   6
#define INTER_MCBPC_VLC_BITS   7
#define CBPY_VLC_BITS          6
#define DC_VLC_BITS            9
#define SPRITE_TRAJ_VLC_BITS   6
#define MB_TYPE_B_VLC_BITS     4
#define H263_MBTYPE_B_VLC_BITS 6
#define CBPC_B_VLC_BITS        3
#define BITSTREAM_BUFFER_SIZE  1024 * 256
#define USER_DATA_STARTCODE    0x1B2
#define GOP_STARTCODE          0x1B3
#define VOP_STARTCODE          0x1B6
#define MOTION_MARKER          0x1F001
#define DC_MARKER              0x6B001
#define TEX_VLC_BITS           9

static int s_Init;

static SMS_VLC s_IntraMCBPC_vlc;
static SMS_VLC s_InterMCBPC_vlc;
static SMS_VLC s_cbpy_vlc;
static SMS_VLC s_dc_lum;
static SMS_VLC s_dc_chrom;
static SMS_VLC s_sprite_trajectory;
static SMS_VLC s_mb_type_b_vlc;
static SMS_VLC s_h263_mbtype_b_vlc;
static SMS_VLC s_cbpc_b_vlc;

const uint8_t s_IntraMCBPC_bits[  9 ] = { 1, 3, 3, 3, 4, 6, 6, 6, 9 };
const uint8_t s_IntraMCBPC_code[  9 ] = { 1, 1, 2, 3, 1, 1, 2, 3, 1 };
const uint8_t s_InterMCBPC_bits[ 28 ] = { 
  1,  4,  4,  6,
  5,  8,  8,  7,
  3,  7,  7,  9,
  6,  9,  9,  9,
  3,  7,  7,  8,
  9,  0,  0,  0,
 11, 13, 13, 13
};
const uint8_t s_InterMCBPC_code[ 28 ] = { 
 1,  3,  2,  5,
 3,  4,  3,  3, 
 3,  7,  6,  5,
 4,  4,  3,  2,
 2,  5,  4,  5,
 1,  0,  0,  0,
 2, 12, 14, 15
};
const uint8_t s_cbpy_tab[ 16 ][ 2 ] = {
 { 3, 4 }, { 5, 5 }, { 4, 5 }, {  9, 4 }, { 3, 5 }, { 7, 4 }, { 2, 6 }, { 11, 4 },
 { 2, 5 }, { 3, 6 }, { 5, 4 }, { 10, 4 }, { 4, 4 }, { 8, 4 }, { 6, 4 }, {  3, 2 }
};
const uint8_t s_mvtab[ 33 ][ 2 ] = {
 {  1,  1 }, {  1,  2 }, {  1,  3 }, {  1,  4 }, {  3,  6 }, {  5,  7 }, {  4,  7 }, {  3,  7 },
 { 11,  9 }, { 10,  9 }, {  9,  9 }, { 17, 10 }, { 16, 10 }, { 15, 10 }, { 14, 10 }, { 13, 10 },
 { 12, 10 }, { 11, 10 }, { 10, 10 }, {  9, 10 }, {  8, 10 }, {  7, 10 }, {  6, 10 }, {  5, 10 },
 {  4, 10 }, { 7,  11 }, {  6, 11 }, {  5, 11 }, {  4, 11 }, {  3, 11 }, {  2, 11 }, {  3, 12 },
 {  2, 12 }
};
const uint16_t s_inter_vlc[ 103 ][ 2 ] = {
 { 0x02,  2 }, { 0x0F,  4 }, { 0x15,  6 }, { 0x17,  7 },
 { 0x1F,  8 }, { 0x25,  9 }, { 0x24,  9 }, { 0x21, 10 },
 { 0x20, 10 }, { 0x07, 11 }, { 0x06, 11 }, { 0x20, 11 },
 { 0x06,  3 }, { 0x14,  6 }, { 0x1E,  8 }, { 0x0F, 10 },
 { 0x21, 11 }, { 0x50, 12 }, { 0x0E,  4 }, { 0x1D,  8 },
 { 0x0E, 10 }, { 0x51, 12 }, { 0x0D,  5 }, { 0x23,  9 },
 { 0x0D, 10 }, { 0x0C,  5 }, { 0x22,  9 }, { 0x52, 12 },
 { 0x0B,  5 }, { 0x0C, 10 }, { 0x53, 12 }, { 0x13,  6 },
 { 0x0B, 10 }, { 0x54, 12 }, { 0x12,  6 }, { 0x0A, 10 },
 { 0x11,  6 }, { 0x09, 10 }, { 0x10,  6 }, { 0x08, 10 },
 { 0x16,  7 }, { 0x55, 12 }, { 0x15,  7 }, { 0x14,  7 },
 { 0x1C,  8 }, { 0x1B,  8 }, { 0x21,  9 }, { 0x20,  9 },
 { 0x1F,  9 }, { 0x1E,  9 }, { 0x1D,  9 }, { 0x1C,  9 },
 { 0x1B,  9 }, { 0x1A,  9 }, { 0x22, 11 }, { 0x23, 11 },
 { 0x56, 12 }, { 0x57, 12 }, { 0x07,  4 }, { 0x19,  9 },
 { 0x05, 11 }, { 0x0F,  6 }, { 0x04, 11 }, { 0x0E,  6 },
 { 0x0D,  6 }, { 0x0C,  6 }, { 0x13,  7 }, { 0x12,  7 },
 { 0x11,  7 }, { 0x10,  7 }, { 0x1A,  8 }, { 0x19,  8 },
 { 0x18,  8 }, { 0x17,  8 }, { 0x16,  8 }, { 0x15,  8 },
 { 0x14,  8 }, { 0x13,  8 }, { 0x18,  9 }, { 0x17,  9 },
 { 0x16,  9 }, { 0x15,  9 }, { 0x14,  9 }, { 0x13,  9 },
 { 0x12,  9 }, { 0x11,  9 }, { 0x07, 10 }, { 0x06, 10 },
 { 0x05, 10 }, { 0x04, 10 }, { 0x24, 11 }, { 0x25, 11 },
 { 0x26, 11 }, { 0x27, 11 }, { 0x58, 12 }, { 0x59, 12 },
 { 0x5A, 12 }, { 0x5B, 12 }, { 0x5C, 12 }, { 0x5D, 12 },
 { 0x5E, 12 }, { 0x5F, 12 }, { 0x03,  7 }
};
const int8_t s_inter_run[ 102 ] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  2,  2,  2,  2,  3,  3,
  3,  4,  4,  4,  5,  5,  5,  6,
  6,  6,  7,  7,  8,  8,  9,  9,
 10, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24,
 25, 26,  0,  0,  0,  1,  1,  2,
  3,  4,  5,  6,  7,  8,  9, 10,
 11, 12, 13, 14, 15, 16, 17, 18,
 19, 20, 21, 22, 23, 24, 25, 26,
 27, 28, 29, 30, 31, 32, 33, 34,
 35, 36, 37, 38, 39, 40
};
const int8_t s_inter_level[ 102 ] = {
  1,  2,  3,  4,  5,  6,  7,  8,
  9, 10, 11, 12,  1,  2,  3,  4,
  5,  6,  1,  2,  3,  4,  1,  2,
  3,  1,  2,  3,  1,  2,  3,  1,
  2,  3,  1,  2,  1,  2,  1,  2,
  1,  2,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  2,  3,  1,  2,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1
};
static SMS_RLTable s_rl_inter = {
 102, 58, s_inter_vlc, s_inter_run, s_inter_level
};
const uint16_t s_intra_vlc[ 103 ][ 2 ] = {
 { 0x02,  2 }, { 0x06,  3 }, { 0x0F,  4 }, { 0x0D,  5 },
 { 0x0C,  5 }, { 0x15,  6 }, { 0x13,  6 }, { 0x12,  6 },
 { 0x17,  7 }, { 0x1F,  8 }, { 0x1E,  8 }, { 0x1D,  8 },
 { 0x25,  9 }, { 0x24,  9 }, { 0x23,  9 }, { 0x21,  9 },
 { 0x21, 10 }, { 0x20, 10 }, { 0x0F, 10 }, { 0x0E, 10 },
 { 0x07, 11 }, { 0x06, 11 }, { 0x20, 11 }, { 0x21, 11 },
 { 0x50, 12 }, { 0x51, 12 }, { 0x52, 12 }, { 0x0E,  4 },
 { 0x14,  6 }, { 0x16,  7 }, { 0x1C,  8 }, { 0x20,  9 },
 { 0x1F,  9 }, { 0x0D, 10 }, { 0x22, 11 }, { 0x53, 12 },
 { 0x55, 12 }, { 0x0B,  5 }, { 0x15,  7 }, { 0x1E,  9 },
 { 0x0C, 10 }, { 0x56, 12 }, { 0x11,  6 }, { 0x1B,  8 },
 { 0x1D,  9 }, { 0x0B, 10 }, { 0x10,  6 }, { 0x22,  9 },
 { 0x0A, 10 }, { 0x0D,  6 }, { 0x1C,  9 }, { 0x08, 10 },
 { 0x12,  7 }, { 0x1B,  9 }, { 0x54, 12 }, { 0x14,  7 },
 { 0x1A,  9 }, { 0x57, 12 }, { 0x19,  8 }, { 0x09, 10 },
 { 0x18,  8 }, { 0x23, 11 }, { 0x17,  8 }, { 0x19,  9 },
 { 0x18,  9 }, { 0x07, 10 }, { 0x58, 12 }, { 0x07,  4 },
 { 0x0C,  6 }, { 0x16,  8 }, { 0x17,  9 }, { 0x06, 10 },
 { 0x05, 11 }, { 0x04, 11 }, { 0x59, 12 }, { 0x0F,  6 },
 { 0x16,  9 }, { 0x05, 10 }, { 0x0E,  6 }, { 0x04, 10 },
 { 0x11,  7 }, { 0x24, 11 }, { 0x10,  7 }, { 0x25, 11 },
 { 0x13,  7 }, { 0x5A, 12 }, { 0x15,  8 }, { 0x5B, 12 },
 { 0x14,  8 }, { 0x13,  8 }, { 0x1A,  8 }, { 0x15,  9 },
 { 0x14,  9 }, { 0x13,  9 }, { 0x12,  9 }, { 0x11,  9 },
 { 0x26, 11 }, { 0x27, 11 }, { 0x5C, 12 }, { 0x5D, 12 },
 { 0x5E, 12 }, { 0x5F, 12 }, { 0x03,  7 }
};
const int8_t s_intra_run[ 102 ] = {
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  2,  2,  2,
  2,  2,  3,  3,  3,  3,  4,  4,
  4,  5,  5,  5,  6,  6,  6,  7,
  7,  7,  8,  8,  9,  9, 10, 11,
 12, 13, 14,  0,  0,  0,  0,  0,
  0,  0,  0,  1,  1,  1,  2,  2,
  3,  3,  4,  4,  5,  5,  6,  6,
  7,  8,  9, 10, 11, 12, 13, 14,
 15, 16, 17, 18, 19, 20
};
const int8_t s_intra_level[ 102 ] = {
  1,  2,  3,  4,  5,  6,  7,  8,
  9, 10, 11, 12, 13, 14, 15, 16,
 17, 18, 19, 20, 21, 22, 23, 24,
 25, 26, 27,  1,  2,  3,  4,  5,
  6,  7,  8,  9, 10,  1,  2,  3,
  4,  5,  1,  2,  3,  4,  1,  2,
  3,  1,  2,  3,  1,  2,  3,  1,
  2,  3,  1,  2,  1,  2,  1,  1,
  1,  1,  1,  1,  2,  3,  4,  5,
  6,  7,  8,  1,  2,  3,  1,  2,
  1,  2,  1,  2,  1,  2,  1,  2,
  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1
};
static SMS_RLTable s_rl_intra = {
 102, 67, s_intra_vlc, s_intra_run, s_intra_level
};
static const uint16_t s_inter_rvlc[ 170 ][ 2 ] = {
 { 0x0006,  3 }, { 0x0001,  4 }, { 0x0004,  5 }, { 0x001C,  7 },
 { 0x003C,  8 }, { 0x003D,  8 }, { 0x007C,  9 }, { 0x00FC, 10 },
 { 0x00FD, 10 }, { 0x01FC, 11 }, { 0x01FD, 11 }, { 0x03FC, 12 },
 { 0x07FC, 13 }, { 0x07FD, 13 }, { 0x0BFC, 13 }, { 0x0BFD, 13 },
 { 0x0FFC, 14 }, { 0x0FFD, 14 }, { 0x1FFC, 15 }, { 0x0007,  3 },
 { 0x000C,  6 }, { 0x005C,  8 }, { 0x007D,  9 }, { 0x017C, 10 },
 { 0x02FC, 11 }, { 0x03FD, 12 }, { 0x0DFC, 13 }, { 0x17FC, 14 },
 { 0x17FD, 14 }, { 0x000A,  4 }, { 0x001D,  7 }, { 0x00BC,  9 },
 { 0x02FD, 11 }, { 0x05FC, 12 }, { 0x1BFC, 14 }, { 0x1BFD, 14 },
 { 0x0005,  5 }, { 0x005D,  8 }, { 0x017D, 10 }, { 0x05FD, 12 },
 { 0x0DFD, 13 }, { 0x1DFC, 14 }, { 0x1FFD, 15 }, { 0x0008,  5 },
 { 0x006C,  8 }, { 0x037C, 11 }, { 0x0EFC, 13 }, { 0x2FFC, 15 },
 { 0x0009,  5 }, { 0x00BD,  9 }, { 0x037D, 11 }, { 0x0EFD, 13 },
 { 0x000D,  6 }, { 0x01BC, 10 }, { 0x06FC, 12 }, { 0x1DFD, 14 },
 { 0x0014,  6 }, { 0x01BD, 10 }, { 0x06FD, 12 }, { 0x2FFD, 15 },
 { 0x0015,  6 }, { 0x01DC, 10 }, { 0x0F7C, 13 }, { 0x002C,  7 },
 { 0x01DD, 10 }, { 0x1EFC, 14 }, { 0x002D,  7 }, { 0x03BC, 11 },
 { 0x0034,  7 }, { 0x077C, 12 }, { 0x006D,  8 }, { 0x0F7D, 13 },
 { 0x0074,  8 }, { 0x1EFD, 14 }, { 0x0075,  8 }, { 0x1F7C, 14 },
 { 0x00DC,  9 }, { 0x1F7D, 14 }, { 0x00DD,  9 }, { 0x1FBC, 14 },
 { 0x00EC,  9 }, { 0x37FC, 15 }, { 0x01EC, 10 }, { 0x01ED, 10 },
 { 0x01F4, 10 }, { 0x03BD, 11 }, { 0x03DC, 11 }, { 0x03DD, 11 },
 { 0x03EC, 11 }, { 0x03ED, 11 }, { 0x03F4, 11 }, { 0x077D, 12 },
 { 0x07BC, 12 }, { 0x07BD, 12 }, { 0x0FBC, 13 }, { 0x0FBD, 13 },
 { 0x0FDC, 13 }, { 0x0FDD, 13 }, { 0x1FBD, 14 }, { 0x1FDC, 14 },
 { 0x1FDD, 14 }, { 0x37FD, 15 }, { 0x3BFC, 15 },
 { 0x000B,  4 }, { 0x0078,  8 }, { 0x03F5, 11 }, { 0x0FEC, 13 },
 { 0x1FEC, 14 }, { 0x0012,  5 }, { 0x00ED,  9 }, { 0x07DC, 12 },
 { 0x1FED, 14 }, { 0x3BFD, 15 }, { 0x0013,  5 }, { 0x03F8, 11 },
 { 0x3DFC, 15 }, { 0x0018,  6 }, { 0x07DD, 12 }, { 0x0019,  6 },
 { 0x07EC, 12 }, { 0x0022,  6 }, { 0x0FED, 13 }, { 0x0023,  6 },
 { 0x0FF4, 13 }, { 0x0035,  7 }, { 0x0FF5, 13 }, { 0x0038,  7 },
 { 0x0FF8, 13 }, { 0x0039,  7 }, { 0x0FF9, 13 }, { 0x0042,  7 },
 { 0x1FF4, 14 }, { 0x0043,  7 }, { 0x1FF5, 14 }, { 0x0079,  8 },
 { 0x1FF8, 14 }, { 0x0082,  8 }, { 0x3DFD, 15 }, { 0x0083,  8 },
 { 0x00F4,  9 }, { 0x00F5,  9 }, { 0x00F8,  9 }, { 0x00F9,  9 },
 { 0x0102,  9 }, { 0x0103,  9 }, { 0x01F5, 10 }, { 0x01F8, 10 },
 { 0x01F9, 10 }, { 0x0202, 10 }, { 0x0203, 10 }, { 0x03F9, 11 },
 { 0x0402, 11 }, { 0x0403, 11 }, { 0x07ED, 12 }, { 0x07F4, 12 },
 { 0x07F5, 12 }, { 0x07F8, 12 }, { 0x07F9, 12 }, { 0x0802, 12 },
 { 0x0803, 12 }, { 0x1002, 13 }, { 0x1003, 13 }, { 0x1FF9, 14 },
 { 0x2002, 14 }, { 0x2003, 14 }, { 0x3EFC, 15 }, { 0x3EFD, 15 },
 { 0x3F7C, 15 }, { 0x3F7D, 15 }, { 0x0000,  4 }
};
static const uint8_t s_inter_rvlc_run[ 169 ] = {
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  2,  2,  2, 
  2,  2,  2,  2,  3,  3,  3,  3, 
  3,  3,  3,  4,  4,  4,  4,  4, 
  5,  5,  5,  5,  6,  6,  6,  6, 
  7,  7,  7,  7,  8,  8,  8,  9, 
  9,  9, 10, 10, 11, 11, 12, 12, 
 13, 13, 14, 14, 15, 15, 16, 16,
 17, 17, 18, 19, 20, 21, 22, 23, 
 24, 25, 26, 27, 28, 29, 30, 31, 
 32, 33, 34, 35, 36, 37, 38, 
  0,  0,  0,  0,  0,  1,  1,  1, 
  1,  1,  2,  2,  2,  3,  3,  4, 
  4,  5,  5,  6,  6,  7,  7,  8, 
  8,  9,  9, 10, 10, 11, 11, 12, 
 12, 13, 13, 14, 15, 16, 17, 18, 
 19, 20, 21, 22, 23, 24, 25, 26, 
 27, 28, 29, 30, 31, 32, 33, 34, 
 35, 36, 37, 38, 39, 40, 41, 42, 
 43, 44
};
static const uint8_t s_inter_rvlc_level[ 169 ] = {
  1,  2,  3,  4,  5,  6,  7,  8, 
  9, 10, 11, 12, 13, 14, 15, 16, 
 17, 18, 19,  1,  2,  3,  4,  5, 
  6,  7,  8,  9, 10,  1,  2,  3, 
  4,  5,  6,  7,  1,  2,  3,  4, 
  5,  6,  7,  1,  2,  3,  4,  5, 
  1,  2,  3,  4,  1,  2,  3,  4, 
  1,  2,  3,  4,  1,  2,  3,  1, 
  2,  3,  1,  2,  1,  2,  1,  2, 
  1,  2,  1,  2,  1,  2,  1,  2, 
  1,  2,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1, 
  1,  2,  3,  4,  5,  1,  2,  3, 
  4,  5,  1,  2,  3,  1,  2,  1, 
  2,  1,  2,  1,  2,  1,  2,  1, 
  2,  1,  2,  1,  2,  1,  2,  1, 
  2,  1,  2,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1
};
static SMS_RLTable s_rvlc_rl_inter = {
 169, 103, s_inter_rvlc, s_inter_rvlc_run, s_inter_rvlc_level
};
static const uint16_t s_intra_rvlc[ 170 ][ 2 ] = {
 { 0x0006,  3 }, { 0x0007,  3 }, { 0x000A,  4 }, { 0x0009,  5 },
 { 0x0014,  6 }, { 0x0015,  6 }, { 0x0034,  7 }, { 0x0074,  8 },
 { 0x0075,  8 }, { 0x00DD,  9 }, { 0x00EC,  9 }, { 0x01EC, 10 },
 { 0x01ED, 10 }, { 0x01F4, 10 }, { 0x03EC, 11 }, { 0x03ED, 11 },
 { 0x03F4, 11 }, { 0x077D, 12 }, { 0x07BC, 12 }, { 0x0FBD, 13 },
 { 0x0FDC, 13 }, { 0x07BD, 12 }, { 0x0FDD, 13 }, { 0x1FBD, 14 },
 { 0x1FDC, 14 }, { 0x1FDD, 14 }, { 0x1FFC, 15 }, { 0x0001,  4 },
 { 0x0008,  5 }, { 0x002D,  7 }, { 0x006C,  8 }, { 0x006D,  8 },
 { 0x00DC,  9 }, { 0x01DD, 10 }, { 0x03DC, 11 }, { 0x03DD, 11 },
 { 0x077C, 12 }, { 0x0FBC, 13 }, { 0x1F7D, 14 }, { 0x1FBC, 14 },
 { 0x0004,  5 }, { 0x002C,  7 }, { 0x00BC,  9 }, { 0x01DC, 10 },
 { 0x03BC, 11 }, { 0x03BD, 11 }, { 0x0EFD, 13 }, { 0x0F7C, 13 },
 { 0x0F7D, 13 }, { 0x1EFD, 14 }, { 0x1F7C, 14 }, { 0x0005,  5 },
 { 0x005C,  8 }, { 0x00BD,  9 }, { 0x037D, 11 }, { 0x06FC, 12 },
 { 0x0EFC, 13 }, { 0x1DFD, 14 }, { 0x1EFC, 14 }, { 0x1FFD, 15 },
 { 0x000C,  6 }, { 0x005D,  8 }, { 0x01BD, 10 }, { 0x03FD, 12 },
 { 0x06FD, 12 }, { 0x1BFD, 14 }, { 0x000D,  6 }, { 0x007D,  9 },
 { 0x02FC, 11 }, { 0x05FC, 12 }, { 0x1BFC, 14 }, { 0x1DFC, 14 },
 { 0x001C,  7 }, { 0x017C, 10 }, { 0x02FD, 11 }, { 0x05FD, 12 },
 { 0x2FFC, 15 }, { 0x001D,  7 }, { 0x017D, 10 }, { 0x037C, 11 },
 { 0x0DFD, 13 }, { 0x2FFD, 15 }, { 0x003C,  8 }, { 0x01BC, 10 },
 { 0x0BFD, 13 }, { 0x17FD, 14 }, { 0x003D,  8 }, { 0x01FD, 11 },
 { 0x0DFC, 13 }, { 0x37FC, 15 }, { 0x007C,  9 }, { 0x03FC, 12 },
 { 0x00FC, 10 }, { 0x0BFC, 13 }, { 0x00FD, 10 }, { 0x37FD, 15 },
 { 0x01FC, 11 }, { 0x07FC, 13 }, { 0x07FD, 13 }, { 0x0FFC, 14 },
 { 0x0FFD, 14 }, { 0x17FC, 14 }, { 0x3BFC, 15 },
 { 0x000B,  4 }, { 0x0078,  8 }, { 0x03F5, 11 }, { 0x0FEC, 13 },
 { 0x1FEC, 14 }, { 0x0012,  5 }, { 0x00ED,  9 }, { 0x07DC, 12 },
 { 0x1FED, 14 }, { 0x3BFD, 15 }, { 0x0013,  5 }, { 0x03F8, 11 },
 { 0x3DFC, 15 }, { 0x0018,  6 }, { 0x07DD, 12 }, { 0x0019,  6 },
 { 0x07EC, 12 }, { 0x0022,  6 }, { 0x0FED, 13 }, { 0x0023,  6 },
 { 0x0FF4, 13 }, { 0x0035,  7 }, { 0x0FF5, 13 }, { 0x0038,  7 },
 { 0x0FF8, 13 }, { 0x0039,  7 }, { 0x0FF9, 13 }, { 0x0042,  7 },
 { 0x1FF4, 14 }, { 0x0043,  7 }, { 0x1FF5, 14 }, { 0x0079,  8 },
 { 0x1FF8, 14 }, { 0x0082,  8 }, { 0x3DFD, 15 }, { 0x0083,  8 },
 { 0x00F4,  9 }, { 0x00F5,  9 }, { 0x00F8,  9 }, { 0x00F9,  9 },
 { 0x0102,  9 }, { 0x0103,  9 }, { 0x01F5, 10 }, { 0x01F8, 10 },
 { 0x01F9, 10 }, { 0x0202, 10 }, { 0x0203, 10 }, { 0x03F9, 11 },
 { 0x0402, 11 }, { 0x0403, 11 }, { 0x07ED, 12 }, { 0x07F4, 12 },
 { 0x07F5, 12 }, { 0x07F8, 12 }, { 0x07F9, 12 }, { 0x0802, 12 },
 { 0x0803, 12 }, { 0x1002, 13 }, { 0x1003, 13 }, { 0x1FF9, 14 },
 { 0x2002, 14 }, { 0x2003, 14 }, { 0x3EFC, 15 }, { 0x3EFD, 15 },
 { 0x3F7C, 15 }, { 0x3F7D, 15 }, { 0x0000,  4 }
};
static const uint8_t s_intra_rvlc_run[ 169 ] = {
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  2,  2,  2,  2,  2,  2,  2,  2, 
  2,  2,  2,  3,  3,  3,  3,  3, 
  3,  3,  3,  3,  4,  4,  4,  4, 
  4,  4,  5,  5,  5,  5,  5,  5, 
  6,  6,  6,  6,  6,  7,  7,  7, 
  7,  7,  8,  8,  8,  8,  9,  9, 
  9,  9, 10, 10, 11, 11, 12, 12, 
 13, 14, 15, 16, 17, 18, 19, 
  0,  0,  0,  0,  0,  1,  1,  1, 
  1,  1,  2,  2,  2,  3,  3,  4, 
  4,  5,  5,  6,  6,  7,  7,  8, 
  8,  9,  9, 10, 10, 11, 11, 12, 
 12, 13, 13, 14, 15, 16, 17, 18, 
 19, 20, 21, 22, 23, 24, 25, 26, 
 27, 28, 29, 30, 31, 32, 33, 34, 
 35, 36, 37, 38, 39, 40, 41, 42, 
 43, 44 
};
static const uint8_t s_intra_rvlc_level[ 169 ] = {
  1,  2,  3,  4,  5,  6,  7,  8, 
  9, 10, 11, 12, 13, 14, 15, 16, 
 17, 18, 19, 20, 21, 22, 23, 24, 
 25, 26, 27,  1,  2,  3,  4,  5, 
  6,  7,  8,  9, 10, 11, 12, 13, 
  1,  2,  3,  4,  5,  6,  7,  8, 
  9, 10, 11,  1,  2,  3,  4,  5, 
  6,  7,  8,  9,  1,  2,  3,  4, 
  5,  6,  1,  2,  3,  4,  5,  6, 
  1,  2,  3,  4,  5,  1,  2,  3, 
  4,  5,  1,  2,  3,  4,  1,  2, 
  3,  4,  1,  2,  1,  2,  1,  2, 
  1,  1,  1,  1,  1,  1,  1,  
  1,  2,  3,  4,  5,  1,  2,  3, 
  4,  5,  1,  2,  3,  1,  2,  1, 
  2,  1,  2,  1,  2,  1,  2,  1, 
  2,  1,  2,  1,  2,  1,  2,  1, 
  2,  1,  2,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1
};
static SMS_RLTable s_rvlc_rl_intra = {
 169, 103, s_intra_rvlc, s_intra_rvlc_run, s_intra_rvlc_level
};
const uint16_t s_intra_vlc_aic[ 103 ][ 2 ] = {
{  0x2,  2 }, {  0x6,  3 }, {  0xe,  4 }, {  0xc,  5 }, 
{  0xd,  5 }, { 0x10,  6 }, { 0x11,  6 }, { 0x12,  6 }, 
{ 0x16,  7 }, { 0x1b,  8 }, { 0x20,  9 }, { 0x21,  9 }, 
{ 0x1a,  9 }, { 0x1b,  9 }, { 0x1c,  9 }, { 0x1d,  9 }, 
{ 0x1e,  9 }, { 0x1f,  9 }, { 0x23, 11 }, { 0x22, 11 }, 
{ 0x57, 12 }, { 0x56, 12 }, { 0x55, 12 }, { 0x54, 12 }, 
{ 0x53, 12 }, {  0xf,  4 }, { 0x14,  6 }, { 0x14,  7 }, 
{ 0x1e,  8 }, {  0xf, 10 }, { 0x21, 11 }, { 0x50, 12 }, 
{  0xb,  5 }, { 0x15,  7 }, {  0xe, 10 }, {  0x9, 10 }, 
{ 0x15,  6 }, { 0x1d,  8 }, {  0xd, 10 }, { 0x51, 12 }, 
{ 0x13,  6 }, { 0x23,  9 }, {  0x7, 11 }, { 0x17,  7 }, 
{ 0x22,  9 }, { 0x52, 12 }, { 0x1c,  8 }, {  0xc, 10 }, 
{ 0x1f,  8 }, {  0xb, 10 }, { 0x25,  9 }, {  0xa, 10 }, 
{ 0x24,  9 }, {  0x6, 11 }, { 0x21, 10 }, { 0x20, 10 }, 
{  0x8, 10 }, { 0x20, 11 }, {  0x7,  4 }, {  0xc,  6 }, 
{ 0x10,  7 }, { 0x13,  8 }, { 0x11,  9 }, { 0x12,  9 }, 
{  0x4, 10 }, { 0x27, 11 }, { 0x26, 11 }, { 0x5f, 12 }, 
{  0xf,  6 }, { 0x13,  9 }, {  0x5, 10 }, { 0x25, 11 }, 
{  0xe,  6 }, { 0x14,  9 }, { 0x24, 11 }, {  0xd,  6 }, 
{  0x6, 10 }, { 0x5e, 12 }, { 0x11,  7 }, {  0x7, 10 }, 
{ 0x13,  7 }, { 0x5d, 12 }, { 0x12,  7 }, { 0x5c, 12 }, 
{ 0x14,  8 }, { 0x5b, 12 }, { 0x15,  8 }, { 0x1a,  8 }, 
{ 0x19,  8 }, { 0x18,  8 }, { 0x17,  8 }, { 0x16,  8 }, 
{ 0x19,  9 }, { 0x15,  9 }, { 0x16,  9 }, { 0x18,  9 }, 
{ 0x17,  9 }, {  0x4, 11 }, {  0x5, 11 }, { 0x58, 12 }, 
{ 0x59, 12 }, { 0x5a, 12 }, {  0x3,  7 }
};
const int8_t s_intra_run_aic[ 102 ] = {
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  0,  0,  0,  0, 
  0,  1,  1,  1,  1,  1,  1,  1, 
  2,  2,  2,  2,  3,  3,  3,  3, 
  4,  4,  4,  5,  5,  5,  6,  6, 
  7,  7,  8,  8,  9,  9, 10, 11, 
 12, 13,  0,  0,  0,  0,  0,  0, 
  0,  0,  0,  0,  1,  1,  1,  1, 
  2,  2,  2,  3,  3,  3,  4,  4, 
  5,  5,  6,  6,  7,  7,  8,  9, 
 10, 11, 12, 13, 14, 15, 16, 17, 
 18, 19, 20, 21, 22, 23
};
const int8_t s_intra_level_aic[ 102 ] = {
  1,  2,  3,  4,  5,  6,  7,  8, 
  9, 10, 11, 12, 13, 14, 15, 16, 
 17, 18, 19, 20, 21, 22, 23, 24, 
 25,  1,  2,  3,  4,  5,  6,  7, 
  1,  2,  3,  4,  1,  2,  3,  4, 
  1,  2,  3,  1,  2,  3,  1,  2, 
  1,  2,  1,  2,  1,  2,  1,  1, 
  1,  1,  1,  2,  3,  4,  5,  6, 
  7,  8,  9, 10,  1,  2,  3,  4, 
  1,  2,  3,  1,  2,  3,  1,  2, 
  1,  2,  1,  2,  1,  2,  1,  1, 
  1,  1,  1,  1,  1,  1,  1,  1, 
  1,  1,  1,  1,  1,  1
};
static SMS_RLTable s_rl_intra_aic = {
 102, 58, s_intra_vlc_aic, s_intra_run_aic, s_intra_level_aic
};
const uint8_t s_DCtab_lum[ 13 ][ 2 ] = {
 { 3, 3 }, { 3, 2 }, { 2, 2 }, { 2, 3 }, { 1,  3 }, { 1,  4 }, { 1, 5 },
 { 1, 6 }, { 1, 7 }, { 1, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }
}; 
const uint8_t s_DCtab_chrom[ 13 ][ 2 ] = {
 { 3, 2 }, { 2, 2 }, { 1, 2 }, { 1,  3 }, { 1,  4 }, { 1,  5 }, { 1, 6 },
 { 1, 7 }, { 1, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 }, { 1, 12 }
};
static const uint16_t s_sprite_trajectory_tab[ 15 ][ 2 ] = {
 { 0x000, 2 }, { 0x002,  3 }, { 0x003,  3 }, { 0x004,  3 }, { 0x05, 3 }, { 0x06, 3 },
 { 0x00E, 4 }, { 0x01E,  5 }, { 0x03E,  6 }, { 0x07E,  7 }, { 0xFE, 8 }, 
 { 0x1FE, 9 }, { 0x3FE, 10 }, { 0x7FE, 11 }, { 0xFFE, 12 }
};
static const uint8_t s_mb_type_b_tab[ 4 ][ 2 ] = {
 { 1, 1 }, { 1, 2 }, { 1, 3 }, { 1, 4 }
};
static const uint8_t s_h263_mbtype_b_tab[ 15 ][ 2 ] = {
 { 1, 1 }, { 3, 3 }, { 1,  5 }, { 4, 4 }, { 5, 4 },
 { 6, 6 }, { 2, 4 }, { 3,  4 }, { 7, 6 }, { 4, 6 },
 { 5, 6 }, { 1, 6 }, { 1, 10 }, { 1, 7 }, { 1, 8 }
};
const uint8_t s_cbpc_b_tab[ 4 ][ 2 ] = {
 { 0, 1 }, { 2, 2 }, { 7, 3 }, { 6, 3 }
};

static const SMS_Rational s_pixel_aspect[ 16 ] = {
 {  0,  1 }, {  1,  1 }, { 12, 11 }, { 10, 11 },
 { 16, 11 }, { 40, 33 }, {  0,  1 }, {  0,  1 },
 { 0,   1 }, {  0,  1 }, {  0,  1 }, {  0,  1 },
 { 0,   1 }, {  0,  1 }, {  0,  1 }, {  0,  1 }
};

const int16_t s_default_intra_matrix[ 64 ] = {
  8, 17, 18, 19, 21, 23, 25, 27,
 17, 18, 19, 21, 23, 25, 27, 28,
 20, 21, 22, 23, 24, 26, 28, 30,
 21, 22, 23, 24, 26, 28, 30, 32,
 22, 23, 24, 26, 28, 30, 32, 35,
 23, 24, 26, 28, 30, 32, 35, 38,
 25, 26, 28, 30, 32, 35, 38, 41,
 27, 28, 30, 32, 35, 38, 41, 45,
};

const int16_t s_default_non_intra_matrix[ 64 ] = {
 16, 17, 18, 19, 20, 21, 22, 23,
 17, 18, 19, 20, 21, 22, 23, 24,
 18, 19, 20, 21, 22, 23, 24, 25,
 19, 20, 21, 22, 23, 24, 26, 27,
 20, 21, 22, 23, 25, 26, 27, 28,
 21, 22, 23, 24, 26, 27, 28, 30,
 22, 23, 24, 26, 27, 28, 30, 31,
 23, 24, 25, 27, 28, 30, 31, 33
};

static const uint8_t s_dc_threshold[ 8 ] = {
 99, 13, 15, 17, 19, 21, 23, 0
};

uint8_t s_y_dc_scale_table[ 32 ] = {
  0,  8,  8,  8,  8, 10, 12, 14, 16, 17, 18, 19, 20, 21, 22, 23,
 24, 25, 26, 27, 28, 29, 30, 31, 32, 34, 36, 38, 40, 42, 44, 46
};

uint8_t s_c_dc_scale_table[ 32 ] = {
  0,  8,  8,  8,  8,  9,  9, 10, 10, 11, 11, 12, 12, 13, 13, 14,
 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25
};

static const uint8_t s_chroma_qscale_table[ 32 ] = {
  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

static const int s_mb_type_b_map[ 4 ]= {
 SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_L0L1,
 SMS_MB_TYPE_L0L1    | SMS_MB_TYPE_16x16,
 SMS_MB_TYPE_L1      | SMS_MB_TYPE_16x16,
 SMS_MB_TYPE_L0      | SMS_MB_TYPE_16x16
};

const uint16_t s_mpeg4_resync_prefix[ 8 ] = {
 0x7F00, 0x7E00, 0x7C00, 0x7800, 0x7000, 0x6000, 0x4000, 0x0000
};

static int32_t MPEG4_Init    ( SMS_CodecContext*                            );
static int32_t MPEG4_Decode  ( SMS_CodecContext*, void**, uint8_t*, int32_t );
static void    MPEG4_Destroy ( SMS_CodecContext*                            );

void SMS_Codec_MPEG4_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "mpeg4";
 apCtx -> m_pCodec -> m_pCtx  = calloc (  1, sizeof ( SMS_Codec_MPEG4Context )  );
 apCtx -> m_pCodec -> Init    = MPEG4_Init;
 apCtx -> m_pCodec -> Decode  = MPEG4_Decode;
 apCtx -> m_pCodec -> Destroy = MPEG4_Destroy;

}  /* end SMS_Codec_MPEG4_Open */

static int32_t MPEG4_Init ( SMS_CodecContext* apCtx ) {

 if ( !s_Init ) {

  int i;
  int lYSize;
  int lCSize;
  int lYCSize;

  SMS_VLC_Init (
   &s_IntraMCBPC_vlc, INTRA_MCBPC_VLC_BITS, 9, 
   s_IntraMCBPC_bits, 1, 1,
   s_IntraMCBPC_code, 1, 1
  );
  SMS_VLC_Init (
   &s_InterMCBPC_vlc, INTER_MCBPC_VLC_BITS, 28, 
   s_InterMCBPC_bits, 1, 1,
   s_InterMCBPC_code, 1, 1
  );
  SMS_VLC_Init (
   &s_cbpy_vlc, CBPY_VLC_BITS, 16,
   &s_cbpy_tab[ 0 ][ 1 ], 2, 1,
   &s_cbpy_tab[ 0 ][ 0 ], 2, 1
  );
  SMS_VLC_Init (
   &g_SMS_mv_vlc, SMS_MV_VLC_BITS, 33,
   &s_mvtab[ 0 ][ 1 ], 2, 1,
   &s_mvtab[ 0 ][ 0 ], 2, 1
  );
  SMS_RL_Init ( &s_rl_inter      );
  SMS_RL_Init ( &s_rl_intra      );
  SMS_RL_Init ( &s_rvlc_rl_inter );
  SMS_RL_Init ( &s_rvlc_rl_intra );
  SMS_RL_Init ( &s_rl_intra_aic  );

  SMS_VLC_RL_Init ( &s_rl_inter      );
  SMS_VLC_RL_Init ( &s_rl_intra      );
  SMS_VLC_RL_Init ( &s_rvlc_rl_inter );
  SMS_VLC_RL_Init ( &s_rvlc_rl_intra );
  SMS_VLC_RL_Init ( &s_rl_intra_aic  );

  SMS_VLC_Init (
   &s_dc_lum, DC_VLC_BITS, 10,
   &s_DCtab_lum[ 0 ][ 1 ], 2, 1,
   &s_DCtab_lum[ 0 ][ 0 ], 2, 1
  );
  SMS_VLC_Init (
   &s_dc_chrom, DC_VLC_BITS, 10,
   &s_DCtab_chrom[ 0 ][ 1 ], 2, 1,
   &s_DCtab_chrom[ 0 ][ 0 ], 2, 1
  );
  SMS_VLC_Init (
   &s_sprite_trajectory, SPRITE_TRAJ_VLC_BITS, 15,
   &s_sprite_trajectory_tab[ 0 ][ 1 ], 4, 2,
   &s_sprite_trajectory_tab[ 0 ][ 0 ], 4, 2
  );
  SMS_VLC_Init (
   &s_mb_type_b_vlc, MB_TYPE_B_VLC_BITS, 4,
   &s_mb_type_b_tab[ 0 ][ 1 ], 2, 1,
   &s_mb_type_b_tab[ 0 ][ 0 ], 2, 1
  );
  SMS_VLC_Init (
   &s_h263_mbtype_b_vlc, H263_MBTYPE_B_VLC_BITS, 15,
   &s_h263_mbtype_b_tab[ 0 ][ 1 ], 2, 1,
   &s_h263_mbtype_b_tab[ 0 ][ 0 ], 2, 1
  );
  SMS_VLC_Init (
   &s_cbpc_b_vlc, CBPC_B_VLC_BITS, 4,
   &s_cbpc_b_tab[ 0 ][ 1 ], 2, 1,
   &s_cbpc_b_tab[ 0 ][ 0 ], 2, 1
  );

  SMS_MPEGContext_Init ( apCtx -> m_Width, apCtx -> m_Height );

  lYSize  = BASECTX().m_B8Stride * (  2 * BASECTX().m_MBH + 1  );
  lCSize  = BASECTX().m_MBStride * (      BASECTX().m_MBH + 1  );
  lYCSize = lYSize + 2 * lCSize;

  BASECTX().m_pACValBase = calloc (  1, lYCSize * sizeof ( int16_t ) * 16  );
  BASECTX().m_pACVal[ 0 ] = BASECTX().m_pACValBase + BASECTX().m_B8Stride + 1;
  BASECTX().m_pACVal[ 1 ] = BASECTX().m_pACValBase + lYSize + BASECTX().m_MBStride + 1;
  BASECTX().m_pACVal[ 2 ] = BASECTX().m_pACVal[ 1 ] + lCSize;

  MYCTX() -> m_pBitstreamBuf = calloc ( 1, BITSTREAM_BUFFER_SIZE );
  MYCTX() -> m_pCBPTbl       = calloc ( 1, i = BASECTX().m_MBH * BASECTX().m_MBStride * sizeof ( uint8_t )  );
  MYCTX() -> m_pPredDirTbl   = calloc ( 1, i );

  BASECTX().m_pDCValBase  = calloc (  1, lYCSize * sizeof ( int16_t )  );
  BASECTX().m_pDCVal[ 0 ] = BASECTX().m_pDCValBase + BASECTX().m_B8Stride + 1;
  BASECTX().m_pDCVal[ 1 ] = BASECTX().m_pDCValBase + lYSize + BASECTX().m_MBStride + 1;
  BASECTX().m_pDCVal[ 2 ] = BASECTX().m_pDCVal[ 1 ] + lCSize;

  for ( i = 0; i < lYCSize; ++i ) BASECTX().m_pDCValBase[ i ] = 1024;

  BASECTX().DCT_UnquantizeIntra = SMS_H263_DCTUnquantizeIntra;
  BASECTX().DCT_UnquantizeInter = SMS_H263_DCTUnquantizeInter;
  BASECTX().m_pChromaQScaleTbl  = s_chroma_qscale_table;

 }  /* end if */

 ++s_Init;

 return 1;

}  /* end MPEG4_Init */

static void MPEG4_Destroy ( SMS_CodecContext* apCtx ) {

 if ( s_Init && !--s_Init ) {

  SMS_VLC_Free ( &s_IntraMCBPC_vlc );
  SMS_VLC_Free ( &s_InterMCBPC_vlc );
  SMS_VLC_Free ( &s_cbpy_vlc       );
  SMS_VLC_Free ( &g_SMS_mv_vlc     );

  SMS_RL_Free ( &s_rl_inter      );
  SMS_RL_Free ( &s_rl_intra      );
  SMS_RL_Free ( &s_rvlc_rl_inter );
  SMS_RL_Free ( &s_rvlc_rl_intra );
  SMS_RL_Free ( &s_rl_intra_aic  );

  SMS_VLC_RL_Free ( &s_rl_inter      );
  SMS_VLC_RL_Free ( &s_rl_intra      );
  SMS_VLC_RL_Free ( &s_rvlc_rl_inter );
  SMS_VLC_RL_Free ( &s_rvlc_rl_intra );
  SMS_VLC_RL_Free ( &s_rl_intra_aic  );

  SMS_VLC_Free ( &s_dc_lum            );
  SMS_VLC_Free ( &s_dc_chrom          );
  SMS_VLC_Free ( &s_sprite_trajectory );
  SMS_VLC_Free ( &s_mb_type_b_vlc     );
  SMS_VLC_Free ( &s_h263_mbtype_b_vlc );
  SMS_VLC_Free ( &s_cbpc_b_vlc        );

  SMS_MPEGContext_Destroy ();

  free (  BASECTX().m_pACValBase );
  free (  BASECTX().m_pDCValBase );

  free (  MYCTX() -> m_pBitstreamBuf  );
  free (  MYCTX() -> m_pCBPTbl        );
  free (  MYCTX() -> m_pPredDirTbl    );

  free ( apCtx -> m_pCodec -> m_pCtx );

 }  /* end if */

}  /* end MPEG4_Destroy */

static SMS_INLINE int _get_amv ( int aN ) {

 int       lX, lY, lMBV, lSum, lDX, lDY, lShift;
 int       lLen = 1 << ( g_MPEGCtx.m_FCode + 4 );
 const int lA   = g_MPEGCtx.m_SpriteWarpAccuracy;

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_AMV ) lLen >>= g_MPEGCtx.m_QuarterSample;

 if ( g_MPEGCtx.m_RealSpriteWarpPts == 1 ) {

  if ( g_MPEGCtx.m_DivXVer == 500 && g_MPEGCtx.m_DivXBuild == 413 )

   lSum = g_MPEGCtx.m_SpriteOffset[ 0 ][ aN ] / (  1 << ( lA - g_MPEGCtx.m_QuarterSample )  );

  else lSum = SMS_RSHIFT( g_MPEGCtx.m_SpriteOffset[ 0 ][ aN ] << g_MPEGCtx.m_QuarterSample, lA );

 } else {

  lDX    = g_MPEGCtx.m_SpriteDelta[ aN ][ 0 ];
  lDY    = g_MPEGCtx.m_SpriteDelta[ aN ][ 1 ];
  lShift = g_MPEGCtx.m_SpriteShift[ 0 ];

  if ( aN )

   lDY -= 1 << ( lShift + lA + 1 );

  else lDX -= 1 << ( lShift + lA + 1 );

  lMBV = g_MPEGCtx.m_SpriteOffset[ 0 ][ aN ] + lDX * g_MPEGCtx.m_MBX * 16 + lDY * g_MPEGCtx.m_MBY * 16;
  lSum  = 0;

  for ( lY = 0; lY < 16; ++lY ) {

   int lV = lMBV + lDY * lY;

   for ( lX = 0; lX < 16; ++lX ) {

    lSum += lV >> lShift;
    lV   += lDX;

   }  /* end for */

  }  /* end for */

  lSum = SMS_RSHIFT( lSum, lA + 8 - g_MPEGCtx.m_QuarterSample );

 }  /* end else */

 if ( lSum < -lLen )

  lSum = -lLen;

 else if ( lSum >= lLen ) lSum = lLen - 1;

 return lSum;

}  /* end _get_amv */

static void _decode_vol_header ( SMS_BitContext* apBitCtx ) {

 int lW, lH, lVerID;

 SMS_SkipBits ( apBitCtx, 1 );

 g_MPEGCtx.m_VoType = SMS_GetBits ( apBitCtx, 8 );

 if (  SMS_GetBit ( apBitCtx ) != 0  ) {

  lVerID = SMS_GetBits ( apBitCtx, 4 );
  SMS_SkipBits ( apBitCtx, 3 );

 } else lVerID  = 1;

 g_MPEGCtx.m_AspectRatio = SMS_GetBits ( apBitCtx, 4 );

 if ( g_MPEGCtx.m_AspectRatio == SMS_ASPECT_EXTENDED ) {

  g_MPEGCtx.m_SampleAspectRatio.m_Num = SMS_GetBits ( apBitCtx, 8 );
  g_MPEGCtx.m_SampleAspectRatio.m_Den = SMS_GetBits ( apBitCtx, 8 );

 } else g_MPEGCtx.m_SampleAspectRatio = s_pixel_aspect[ g_MPEGCtx.m_AspectRatio ];

 if (   (  g_MPEGCtx.m_VolCtlPar = SMS_GetBit ( apBitCtx )  )   ) {

  SMS_GetBits ( apBitCtx, 2 );

  g_MPEGCtx.m_LowDelay = SMS_GetBit ( apBitCtx );

  if (  SMS_GetBit ( apBitCtx )  ) {

   SMS_GetBits ( apBitCtx, 15 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 15 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 15 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx,  3 );
   SMS_GetBits ( apBitCtx, 11 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 15 );
   SMS_SkipBit ( apBitCtx );

  }  /* end if */

 } else if ( g_MPEGCtx.m_PicNr == 0 ) g_MPEGCtx.m_LowDelay = 0;

 g_MPEGCtx.m_Shape = SMS_GetBits ( apBitCtx, 2 );

 if ( g_MPEGCtx.m_Shape == SMS_GRAY_SHAPE && lVerID != 1 ) SMS_SkipBits ( apBitCtx, 4 );

 SMS_GetBit ( apBitCtx );  /* check marker */

 g_MPEGCtx.m_TimeIncRes  = SMS_GetBits ( apBitCtx, 16 );
 g_MPEGCtx.m_TimeIncBits = SMS_log2 ( g_MPEGCtx.m_TimeIncRes - 1 ) + 1;

 if ( g_MPEGCtx.m_TimeIncBits < 1 ) g_MPEGCtx.m_TimeIncBits = 1;

 SMS_GetBit ( apBitCtx );  /* check marker */

 if (  SMS_GetBit ( apBitCtx )  ) SMS_SkipBits ( apBitCtx, g_MPEGCtx.m_TimeIncBits );

 g_MPEGCtx.m_TFrame = 0;

 if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE ) {

  if ( g_MPEGCtx.m_Shape == SMS_RECT_SHAPE ) {

   SMS_SkipBit ( apBitCtx );
   lW = SMS_GetBits ( apBitCtx, 13 );
   SMS_SkipBit ( apBitCtx );
   lH = SMS_GetBits ( apBitCtx, 13 );
   SMS_SkipBit ( apBitCtx );

   if ( lW && lH ) {

    g_MPEGCtx.m_Width  = lW;
    g_MPEGCtx.m_Height = lH;

   }  /* end if */

  }  /* end if */

  g_MPEGCtx.m_ProgSeq = 
  g_MPEGCtx.m_ProgFrm = SMS_GetBit ( apBitCtx ) ^ 1;

  SMS_GetBit ( apBitCtx );

  g_MPEGCtx.m_VolSpriteUsage = lVerID == 1 ? SMS_GetBit ( apBitCtx ) : SMS_GetBits ( apBitCtx, 2 );

  if ( g_MPEGCtx.m_VolSpriteUsage == STATIC_SPRITE ||
       g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE
  ) {

   if ( g_MPEGCtx.m_VolSpriteUsage == STATIC_SPRITE ) {

    g_MPEGCtx.m_SpriteWidth  = SMS_GetBits ( apBitCtx, 13 );
    SMS_SkipBit ( apBitCtx );
    g_MPEGCtx.m_SpriteHeight = SMS_GetBits ( apBitCtx, 13 );
    SMS_SkipBit ( apBitCtx );
    g_MPEGCtx.m_SpriteLeft   = SMS_GetBits ( apBitCtx, 13 );
    SMS_SkipBit ( apBitCtx );
    g_MPEGCtx.m_SpriteTop    = SMS_GetBits ( apBitCtx, 13 );
    SMS_SkipBit ( apBitCtx );

   }  /* end if */

   g_MPEGCtx.m_nSpriteWarpPts         = SMS_GetBits ( apBitCtx, 6 );
   g_MPEGCtx.m_SpriteWarpAccuracy     = SMS_GetBits ( apBitCtx, 2 );
   g_MPEGCtx.m_SpriteBrightnessChange = SMS_GetBit ( apBitCtx );

   if ( g_MPEGCtx.m_VolSpriteUsage == STATIC_SPRITE ) g_MPEGCtx.m_LowLatencySprite = SMS_GetBit ( apBitCtx );

  }  /* end if */

  if (  SMS_GetBit ( apBitCtx ) == 1  ) {

   g_MPEGCtx.m_QuantPrec = SMS_GetBits ( apBitCtx, 4 );
   SMS_GetBits ( apBitCtx, 4 );

  } else g_MPEGCtx.m_QuantPrec = 5;

  if (   (  g_MPEGCtx.m_MPEGQuant = SMS_GetBit ( apBitCtx )  )   ) {

   int i, v;

   for ( i = 0; i < 64; ++i ) {

    int j = g_MPEGCtx.m_DSPCtx.m_Permutation[ i ];

    v = s_default_intra_matrix[ i ];

    g_MPEGCtx.m_IntraMatrix      [ j ] =
    g_MPEGCtx.m_ChromaIntraMatrix[ j ] = v;
                
    v = s_default_non_intra_matrix[ i ];

    g_MPEGCtx.m_InterMatrix      [ j ] =
    g_MPEGCtx.m_ChromaInterMatrix[ j ] = v;

   }  /* end for */

   if (  SMS_GetBit ( apBitCtx )  ) {

    int lLast = 0;

    for ( i = 0; i < 64; ++i ) {

     int j;

     v = SMS_GetBits ( apBitCtx, 8 );

     if ( !v ) break;
                    
     lLast = v;
     j     = g_MPEGCtx.m_DSPCtx.m_Permutation[  g_SMS_DSP_zigzag_direct[ i ]  ];

     g_MPEGCtx.m_IntraMatrix      [ j ] =
     g_MPEGCtx.m_ChromaIntraMatrix[ j ] = v;

    }  /* end for */

    for ( ; i < 64; ++i ) {

     int j = g_MPEGCtx.m_DSPCtx.m_Permutation[  g_SMS_DSP_zigzag_direct[ i ]  ];

     g_MPEGCtx.m_IntraMatrix      [ j ] =
     g_MPEGCtx.m_ChromaIntraMatrix[ j ] = lLast;

    }  /* end for */

   }  /* end if */

   if (  SMS_GetBit ( apBitCtx )  ) {

    int lLast = 0;

    for ( i = 0; i < 64; ++i ) {

     int j;

     v = SMS_GetBits ( apBitCtx, 8 );

     if ( !v ) break;

     lLast = v;
     j     = g_MPEGCtx.m_DSPCtx.m_Permutation[  g_SMS_DSP_zigzag_direct[ i ]  ];

     g_MPEGCtx.m_InterMatrix      [ j ] =
     g_MPEGCtx.m_ChromaInterMatrix[ j ] = v;

    }  /* end for */

    for ( ; i < 64; ++i ) {

     int j = g_MPEGCtx.m_DSPCtx.m_Permutation[  g_SMS_DSP_zigzag_direct[ i ]  ];

     g_MPEGCtx.m_InterMatrix      [ j ] =
     g_MPEGCtx.m_ChromaInterMatrix[ j ] = lLast;

    }  /* end for */

   }  /* end if */

  }  /* end if */

  g_MPEGCtx.m_QuarterSample = lVerID != 1 ? SMS_GetBit ( apBitCtx ) : 0;
  SMS_GetBit ( apBitCtx );
  g_MPEGCtx.m_ResyncMarker     = !SMS_GetBit ( apBitCtx );
  g_MPEGCtx.m_DataPartitioning = SMS_GetBit ( apBitCtx );

  if ( g_MPEGCtx.m_DataPartitioning ) g_MPEGCtx.m_RVLC = SMS_GetBit ( apBitCtx );

  if ( lVerID != 1 ) {

   g_MPEGCtx.m_NewPred = SMS_GetBit ( apBitCtx );

   if ( g_MPEGCtx.m_NewPred ) {

    SMS_SkipBits ( apBitCtx, 2 );
    SMS_SkipBit ( apBitCtx );

   }  /* end if */

   g_MPEGCtx.m_ReducedResVop = SMS_GetBit ( apBitCtx );

  } else {

   g_MPEGCtx.m_NewPred       =
   g_MPEGCtx.m_ReducedResVop = 0;

  }  /* end else */

  g_MPEGCtx.m_Scalability = SMS_GetBit ( apBitCtx );

  if ( g_MPEGCtx.m_Scalability ) {

   SMS_BitContext lBitCtx = *apBitCtx;
   int            lRefLayerID;
   int            lRefLayerSamplingDir;
   int            lHSamplingFactorN;
   int            lHSamplingFactorM;
   int            lVSamplingFactorN;
   int            lVSamplingFactorM;
            
   g_MPEGCtx.m_HierachyType    = SMS_GetBit ( apBitCtx );
   lRefLayerID                 = SMS_GetBits ( apBitCtx, 4 );
   lRefLayerSamplingDir        = SMS_GetBit  ( apBitCtx    );
   lHSamplingFactorN           = SMS_GetBits ( apBitCtx, 5 );
   lHSamplingFactorM           = SMS_GetBits ( apBitCtx, 5 );
   lVSamplingFactorN           = SMS_GetBits ( apBitCtx, 5 );
   lVSamplingFactorM           = SMS_GetBits ( apBitCtx, 5 );
   g_MPEGCtx.m_EnhancementType = SMS_GetBit ( apBitCtx );
            
   if ( lHSamplingFactorN == 0 || lHSamplingFactorM == 0 ||
        lVSamplingFactorN == 0 || lVSamplingFactorM == 0
   ) {
               
    g_MPEGCtx.m_Scalability = 0;
    *apBitCtx = lBitCtx;

   }  /* end if */
            
  }  /* end if */

 }  /* end if */

}  /* end _decode_vol_header */

static void _decode_user_data ( SMS_BitContext* apBitCtx ) {

 char  lBuf[ 256 ];
 char* lpBuf = lBuf;
 int   i, lVer = -1, lBuild = -1;
 char  lLast = '\xFF';

 lBuf[ 0 ] = SMS_ShowBits ( apBitCtx, 8 );

 for ( i = 1; i < 256; ++i ) {

  lBuf[ i ]= SMS_ShowBits ( apBitCtx, 16 ) & 0xFF;

  if ( lBuf[ i ] == 0 ) break;

  SMS_SkipBits ( apBitCtx, 8 );

 }  /* end for */

 lBuf[ 255 ] = 0;

 i = 0;

 if (  SMS_FourCC_DivX == SMS_unaligned32 ( lpBuf )  ) {

  lpBuf += 4;

  while (  isdigit ( *lpBuf )  ) lBuf[ i++ ] = *lpBuf++;

  lBuf[ i ] = '\x00'; lVer = atoi ( lBuf );

  if (  !strncmp ( lpBuf, "Build", 5 )  ) {

   lpBuf += 5;
scanBuild:
   i = 0;

   while (  isdigit ( *lpBuf )  ) lBuf[ i++ ] = *lpBuf++;

   lBuf[ i ] = '\x00'; lBuild = atoi ( lBuf );
   lLast     = *lpBuf;

  } else if ( *lpBuf == 'b' ) {

   ++lpBuf;
   goto scanBuild;

  }  /* end if */

  if ( lVer != 0 && lBuild != 0 ) {

   g_MPEGCtx.m_DivXVer   = lVer;
   g_MPEGCtx.m_DivXBuild = lBuild;
   g_MPEGCtx.m_DivXPack  = lLast == 'p';

  }  /* end if */

 } else if (  SMS_FourCC_XviD == SMS_unaligned32 ( lpBuf )  ) {

  lpBuf += 4;

  while (  isdigit ( *lpBuf )  ) lBuf[ i++ ] = *lpBuf++;

  lBuf[ i ] = '\x00'; lBuild = atoi ( lBuf );

  if ( lBuild != 0 ) g_MPEGCtx.m_XviDBuild = lBuild;

 }  /* end if */

}  /* end _decode_user_data */

static void _decode_gop_header ( SMS_BitContext* apBitCtx ) {

 int lH, lM, lS;

 lH = SMS_GetBits ( apBitCtx, 5 );
 lM = SMS_GetBits ( apBitCtx, 6 );
 SMS_SkipBit ( apBitCtx );
 lS = SMS_GetBits ( apBitCtx, 6 );

 g_MPEGCtx.m_TimeBase = lS + 60 * ( lM + 60 * lH );

 SMS_SkipBit ( apBitCtx );
 SMS_SkipBit ( apBitCtx );
    
}  /* end _decode_gop_header */

static int _mpeg4_get_video_packet_prefix_length ( void ) {

 switch ( g_MPEGCtx.m_PicType ){

  case SMS_FT_I_TYPE: return 16;
  case SMS_FT_P_TYPE:
  case SMS_FT_S_TYPE: return g_MPEGCtx.m_FCode + 15;
  case SMS_FT_B_TYPE: return SMS_MAX(  SMS_MAX( g_MPEGCtx.m_FCode, g_MPEGCtx.m_BCode ) + 15, 17  );
  default           : return -1;

 }  /* end switch */

}  /* end _mpeg4_get_video_packet_prefix_length */

static void _mpeg4_decode_sprite_trajectory ( SMS_BitContext* apBitCtx ) {

 int       i;
 int       lA   = 2 << g_MPEGCtx.m_SpriteWarpAccuracy;
 int       lRHO = 3 - g_MPEGCtx.m_SpriteWarpAccuracy;
 int       lR   = 16 / lA;
 const int lVOPRef[ 4 ][ 2 ]= {  { 0, 0 }, { g_MPEGCtx.m_Width, 0 }, { 0, g_MPEGCtx.m_Height }, { g_MPEGCtx.m_Width, g_MPEGCtx.m_Height }  };
 int       lD[ 4 ][ 2 ] = {  { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }  };
 int       lSpriteRef[ 4 ][ 2 ];
 int       lVirtualRef[ 2 ][ 2 ];
 int       lW2, lH2, lW3, lH3;
 int       lAlpha = 0, lBeta = 0;
 int       lW = g_MPEGCtx.m_Width;
 int       lH = g_MPEGCtx.m_Height;
 int       lMinAB;

 for ( i = 0; i < g_MPEGCtx.m_nSpriteWarpPts; ++i ) {

  int lLen;
  int lX = 0, lY = 0;

  lLen = SMS_GetVLC ( apBitCtx, &s_sprite_trajectory );

  if ( lLen ) lX = SMS_GetXBits ( apBitCtx, lLen );

  if (  !( g_MPEGCtx.m_DivXVer == 500 && g_MPEGCtx.m_DivXBuild == 413 )  ) SMS_SkipBit ( apBitCtx ); /* marker bit */
        
  lLen = SMS_GetVLC ( apBitCtx, &s_sprite_trajectory );

  if ( lLen ) lY = SMS_GetXBits ( apBitCtx, lLen );

  SMS_SkipBit ( apBitCtx );

  lD[ i ][ 0 ] = lX;
  lD[ i ][ 1 ] = lY;

 }  /* end for */

 while (  ( 1 << lAlpha ) < lW ) ++lAlpha;
 while (  ( 1 << lBeta  ) < lH ) ++lBeta;

 lW2 = 1 << lAlpha;
 lH2 = 1 << lBeta;

 if ( g_MPEGCtx.m_DivXVer == 500 && g_MPEGCtx.m_DivXBuild == 413 ) {

  lSpriteRef[ 0 ][ 0 ] = lA * lVOPRef[ 0 ][ 0 ] + lD[ 0 ][ 0 ];
  lSpriteRef[ 0 ][ 1 ] = lA * lVOPRef[ 0 ][ 1 ] + lD[ 0 ][ 1 ];
  lSpriteRef[ 1 ][ 0 ] = lA * lVOPRef[ 1 ][ 0 ] + lD[ 0 ][ 0 ] + lD[ 1 ][ 0 ];
  lSpriteRef[ 1 ][ 1 ] = lA * lVOPRef[ 1 ][ 1 ] + lD[ 0 ][ 1 ] + lD[ 1 ][ 1 ];
  lSpriteRef[ 2 ][ 0 ] = lA * lVOPRef[ 2 ][ 0 ] + lD[ 0 ][ 0 ] + lD[ 2 ][ 0 ];
  lSpriteRef[ 2 ][ 1 ] = lA * lVOPRef[ 2 ][ 1 ] + lD[ 0 ][ 1 ] + lD[ 2 ][ 1 ];

 } else {

  lSpriteRef[ 0 ][ 0 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 0 ][ 0 ] + lD[ 0 ][ 0 ] );
  lSpriteRef[ 0 ][ 1 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 0 ][ 1 ] + lD[ 0 ][ 1 ] );
  lSpriteRef[ 1 ][ 0 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 1 ][ 0 ] + lD[ 0 ][ 0 ] + lD[ 1 ][ 0 ] );
  lSpriteRef[ 1 ][ 1 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 1 ][ 1 ] + lD[ 0 ][ 1 ] + lD[ 1 ][ 1 ] );
  lSpriteRef[ 2 ][ 0 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 2 ][ 0 ] + lD[ 0 ][ 0 ] + lD[ 2 ][ 0 ] );
  lSpriteRef[ 2 ][ 1 ] = ( lA >> 1 ) * ( 2 * lVOPRef[ 2 ][ 1 ] + lD[ 0 ][ 1 ] + lD[ 2 ][ 1 ] );

 }  /* end else */

 lVirtualRef[ 0 ][ 0 ] = 16 * ( lVOPRef[ 0 ][ 0 ] + lW2 ) + SMS_ROUNDED_DIV(   (  ( lW - lW2 ) * ( lR * lSpriteRef[ 0 ][ 0 ] - 16 * lVOPRef[ 0 ][ 0 ] ) + lW2 * ( lR * lSpriteRef[ 1 ][ 0 ] - 16 * lVOPRef[ 1 ][ 0 ] )  ), lW   );
 lVirtualRef[ 0 ][ 1 ] = 16 * lVOPRef[ 0 ][ 1 ]           + SMS_ROUNDED_DIV(   (  ( lW - lW2 ) * ( lR * lSpriteRef[ 0 ][ 1 ] - 16 * lVOPRef[ 0 ][ 1 ] ) + lW2 * ( lR * lSpriteRef[ 1 ][ 1 ] - 16 * lVOPRef[ 1 ][ 1 ] )  ), lW   );
 lVirtualRef[ 1 ][ 0 ] = 16 * lVOPRef[ 0 ][ 0 ]           + SMS_ROUNDED_DIV(   (  ( lH - lH2 ) * ( lR * lSpriteRef[ 0 ][ 0 ] - 16 * lVOPRef[ 0 ][ 0 ] ) + lH2 * ( lR * lSpriteRef[ 2 ][ 0 ] - 16 * lVOPRef[ 2 ][ 0 ] )  ), lH   );
 lVirtualRef[ 1 ][ 1 ] = 16 * ( lVOPRef[ 0 ][ 1 ] + lH2 ) + SMS_ROUNDED_DIV(   (  ( lH - lH2 ) * ( lR * lSpriteRef[ 0 ][ 1 ] - 16 * lVOPRef[ 0 ][ 1 ] ) + lH2 * ( lR * lSpriteRef[ 2 ][ 1 ] - 16 * lVOPRef[ 2 ][ 1 ] )  ), lH   );

 switch ( g_MPEGCtx.m_nSpriteWarpPts ) {

  case 0:

   g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] = 0;
   g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] = 0;
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] = 0;
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] = 0;

   g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] = lA;
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] =  0;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] =  0;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] = lA;

   g_MPEGCtx.m_SpriteShift[ 0 ] = 0;
   g_MPEGCtx.m_SpriteShift[ 1 ] = 0;

  break;

  case 1:

   g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] = lSpriteRef[ 0 ][ 0 ] - lA * lVOPRef[ 0 ][ 0 ];
   g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] = lSpriteRef[ 0 ][ 1 ] - lA * lVOPRef[ 0 ][ 1 ];
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] = (  ( lSpriteRef[ 0 ][ 0 ] >> 1 ) | ( lSpriteRef[ 0 ][ 0 ] & 1 )  ) - lA * ( lVOPRef[ 0 ][ 0 ] / 2 );
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] = (  ( lSpriteRef[ 0 ][ 1 ] >> 1 ) | ( lSpriteRef[ 0 ][ 1 ] & 1 )  ) - lA * ( lVOPRef[ 0 ][ 1 ] / 2 );

   g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] = lA;
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] = 0;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] = 0;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] = lA;

   g_MPEGCtx.m_SpriteShift[ 0 ] = 0;
   g_MPEGCtx.m_SpriteShift[ 1 ] = 0;

  break;

  case 2:

   g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] = (  lSpriteRef[ 0 ][ 0 ] << ( lAlpha + lRHO )  )                                 +
                                        ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * ( -lVOPRef[ 0 ][ 0 ] ) +
                                        (  lR * lSpriteRef[ 0 ][ 1 ] - lVirtualRef[ 0 ][ 1 ] ) * ( -lVOPRef[ 0 ][ 1 ] ) +
                                        (  1 << ( lAlpha + lRHO - 1 )  );
   g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] = (  lSpriteRef[ 0 ][1] << ( lAlpha + lRHO )  )                                   +
                                        ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] ) * ( -lVOPRef[ 0 ][ 0 ] ) +
                                        ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * ( -lVOPRef[ 0 ][ 1 ] ) +
                                        (  1 << ( lAlpha + lRHO - 1 )  );
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] = (  ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * ( -2 * lVOPRef[ 0 ][ 0 ] + 1 ) +
                                        (     lR * lSpriteRef[ 0 ][ 1 ] - lVirtualRef[ 0 ][ 1 ] ) * ( -2 * lVOPRef[ 0 ][ 1 ] + 1 ) +
                                        2 * lW2 * lR * lSpriteRef[ 0 ][ 0 ] - 16 * lW2                                             +
                                        (  1 << ( lAlpha + lRHO + 1 )  )   );
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] = (  ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] ) * ( -2 * lVOPRef[ 0 ][ 0 ] + 1 ) +
                                        (    -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * ( -2 * lVOPRef[ 0 ][ 1 ] + 1 ) +
                                        2 * lW2 * lR * lSpriteRef[ 0 ][ 1 ]  - 16 * lW2                                            +
                                        (  1 << ( lAlpha + lRHO + 1 )  )   );
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] = ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] );
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] = (  lR * lSpriteRef[ 0 ][ 1 ] - lVirtualRef[ 0 ][ 1 ] );
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] = ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] );
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] = ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] );
            
   g_MPEGCtx.m_SpriteShift[ 0 ]= lAlpha + lRHO;
   g_MPEGCtx.m_SpriteShift[ 1 ]= lAlpha + lRHO + 2;

  break;

  case 3:

   lMinAB = SMS_MIN( lAlpha, lBeta );
   lW3    = lW2 >> lMinAB;
   lH3    = lH2 >> lMinAB;
   g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] = (  lSpriteRef[ 0 ][ 0 ] << ( lAlpha + lBeta + lRHO - lMinAB )  )                      +
                                        ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * lH3 * ( -lVOPRef[ 0 ][ 0 ] ) +
                                        ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 1 ][ 0 ] ) * lW3 * ( -lVOPRef[ 0 ][ 1 ] ) +
                                        (  1 << ( lAlpha + lBeta + lRHO - lMinAB - 1 )  );
   g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] = (  lSpriteRef[ 0 ][ 1 ] << ( lAlpha + lBeta + lRHO - lMinAB )  )                      +
                                        ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] ) * lH3 * ( -lVOPRef[ 0 ][ 0 ] ) +
                                        ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 1 ][ 1 ] ) * lW3 * ( -lVOPRef[ 0 ][ 1 ] ) +
                                        (  1 << ( lAlpha + lBeta + lRHO - lMinAB - 1 )  );
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] = ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * lH3 * ( -2 * lVOPRef[ 0 ][ 0 ] + 1 ) +
                                        ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 1 ][ 0 ] ) * lW3 * ( -2 * lVOPRef[ 0 ][ 1 ] + 1 ) +
                                        2 * lW2 * lH3 * lR * lSpriteRef[ 0 ][ 0 ] - 16 * lW2 * lH3                                    +
                                        (  1 << ( lAlpha + lBeta + lRHO - lMinAB + 1 )  );
   g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] = ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] ) * lH3 * ( -2 * lVOPRef[ 0 ][ 0 ] + 1 ) +
                                        ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 1 ][ 1 ] ) * lW3 * ( -2 * lVOPRef[ 0 ][ 1 ] + 1 ) +
                                        2 * lW2 * lH3 * lR * lSpriteRef[ 0 ][ 1 ] - 16 * lW2 * lH3                                    +
                                        (  1 << ( lAlpha + lBeta + lRHO - lMinAB + 1 )  );
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] = ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 0 ][ 0 ] ) * lH3;
   g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] = ( -lR * lSpriteRef[ 0 ][ 0 ] + lVirtualRef[ 1 ][ 0 ] ) * lW3;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] = ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 0 ][ 1 ] ) * lH3;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] = ( -lR * lSpriteRef[ 0 ][ 1 ] + lVirtualRef[ 1 ][ 1 ] ) * lW3;
                                   
   g_MPEGCtx.m_SpriteShift[ 0 ] = lAlpha + lBeta + lRHO - lMinAB;
   g_MPEGCtx.m_SpriteShift[ 1 ] = lAlpha + lBeta + lRHO - lMinAB + 2;

  break;

 }  /* end switch */

 if ( g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] == lA << g_MPEGCtx.m_SpriteShift[ 0 ] &&
      g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] == 0                                  &&
      g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] == 0                                  &&
      g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] == lA << g_MPEGCtx.m_SpriteShift[ 0 ]
 ) {

  g_MPEGCtx.m_SpriteOffset[ 0 ][ 0 ] >>= g_MPEGCtx.m_SpriteShift[ 0 ];
  g_MPEGCtx.m_SpriteOffset[ 0 ][ 1 ] >>= g_MPEGCtx.m_SpriteShift[ 0 ];
  g_MPEGCtx.m_SpriteOffset[ 1 ][ 0 ] >>= g_MPEGCtx.m_SpriteShift[ 1 ];
  g_MPEGCtx.m_SpriteOffset[ 1 ][ 1 ] >>= g_MPEGCtx.m_SpriteShift[ 1 ];

  g_MPEGCtx.m_SpriteDelta[ 0 ][ 0 ] = lA;
  g_MPEGCtx.m_SpriteDelta[ 0 ][ 1 ] =  0;
  g_MPEGCtx.m_SpriteDelta[ 1 ][ 0 ] =  0;
  g_MPEGCtx.m_SpriteDelta[ 1 ][ 1 ] = lA;

  g_MPEGCtx.m_SpriteShift[ 0 ] = 0;
  g_MPEGCtx.m_SpriteShift[ 1 ] = 0;

  g_MPEGCtx.m_RealSpriteWarpPts = 1;

 } else {

  int lShiftY = 16 - g_MPEGCtx.m_SpriteShift[ 0 ];
  int lShiftC = 16 - g_MPEGCtx.m_SpriteShift[ 1 ];

  for ( i = 0; i < 2; ++i ) {

   g_MPEGCtx.m_SpriteOffset[ 0 ][ i ] <<= lShiftY;
   g_MPEGCtx.m_SpriteOffset[ 1 ][ i ] <<= lShiftC;

   g_MPEGCtx.m_SpriteDelta[ 0 ][ i ] <<= lShiftY;
   g_MPEGCtx.m_SpriteDelta[ 1 ][ i ] <<= lShiftY;

   g_MPEGCtx.m_SpriteShift[ i ] = 16;

  }  /* end for */

  g_MPEGCtx.m_RealSpriteWarpPts = g_MPEGCtx.m_nSpriteWarpPts;

 }  /* end else */

}  /* end _mpeg4_decode_sprite_trajectory  */

static int _mpeg4_decode_video_packet_header ( void ) {

 int             lMBNBits = SMS_log2 ( g_MPEGCtx.m_MBNum - 1 ) + 1;
 int             lHdrX = 0, lMBNum, lLen;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;
    
 if (  SMS_BitCount ( lpBitCtx ) > lpBitCtx -> m_szInBits - 20  ) return -1;

 for ( lLen = 0; lLen < 32; ++lLen ) if (  SMS_GetBit ( lpBitCtx )  ) break;

 if (  lLen != _mpeg4_get_video_packet_prefix_length ()  ) return -1;
    
 if ( g_MPEGCtx.m_Shape != SMS_RECT_SHAPE ) lHdrX = SMS_GetBit ( lpBitCtx );

 lMBNum = SMS_GetBits ( lpBitCtx, lMBNBits );

 if ( lMBNum >= g_MPEGCtx.m_MBNum ) return -1;

 if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ) {

  while ( g_MPEGCtx.m_NextPic.m_pMBSkipTbl[  g_MPEGCtx.m_pMBIdx2XY[ lMBNum ]  ] ) ++lMBNum;

  if ( lMBNum >= g_MPEGCtx.m_MBNum ) return -1;

 }  /* end if */
    
 g_MPEGCtx.m_MBX = lMBNum % g_MPEGCtx.m_MBW;
 g_MPEGCtx.m_MBY = lMBNum / g_MPEGCtx.m_MBW;

 if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE ) {

  int lQScale = SMS_GetBits ( lpBitCtx, g_MPEGCtx.m_QuantPrec );

  if ( lQScale ) g_MPEGCtx.m_ChromaQScale = g_MPEGCtx.m_QScale = lQScale;

 }  /* end if */

 if ( g_MPEGCtx.m_Shape == SMS_RECT_SHAPE ) lHdrX = SMS_GetBit ( lpBitCtx );

 if ( lHdrX ) {

  int lTimeIncrement;
  int lTimeIncr = 0;

  while (  SMS_GetBit ( lpBitCtx ) != 0  ) ++lTimeIncr;

  SMS_GetBit ( lpBitCtx );  // check marker

  lTimeIncrement = SMS_GetBits ( lpBitCtx, g_MPEGCtx.m_TimeIncBits );

  SMS_GetBit ( lpBitCtx );  // check marker
        
  SMS_SkipBits ( lpBitCtx, 2 );

  if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE ) {

   SMS_SkipBits ( lpBitCtx, 3 );

   if ( g_MPEGCtx.m_PicType        == SMS_FT_S_TYPE &&
        g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE
   ) _mpeg4_decode_sprite_trajectory ( lpBitCtx );
            
   if ( g_MPEGCtx.m_PicType != SMS_FT_I_TYPE ) SMS_GetBits ( lpBitCtx, 3 );
   if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ) SMS_GetBits ( lpBitCtx, 3 );

  }  /* end if */

 }  /* end if */

 return 0;

}  /* end _mpeg4_decode_video_packet_header */

static SMS_INLINE int _mpeg4_is_resync ( SMS_BitContext* apBitCtx ) {

 const int lBitCount = apBitCtx -> m_Idx;
    
 if ( g_MPEGCtx.m_Bugs & SMS_BUG_NO_PADDING ) return 0;

 if ( lBitCount + 8 >= apBitCtx -> m_szInBits ) { 

  int lV  = SMS_ShowBits ( apBitCtx, 8 );
      lV |= 0x7F >> (  7 - ( lBitCount & 7 )  );
                
  if ( lV == 0x7F ) return 1;

 } else {

  if (  SMS_ShowBits ( apBitCtx, 16 ) == s_mpeg4_resync_prefix[ lBitCount & 7 ] ) {

   int            lLen;
   SMS_BitContext lBitCtx = *apBitCtx;
        
   SMS_SkipBits ( apBitCtx, 1 );
   SMS_AlignBits ( apBitCtx );
        
   for ( lLen = 0; lLen < 32; ++lLen ) if (  SMS_GetBit ( apBitCtx )  ) break;

   *apBitCtx = lBitCtx;

   if ( lLen >= _mpeg4_get_video_packet_prefix_length ()  ) return 1;

  }  /* end if */

 }  /* end else */

 return 0;

}  /* end _mpeg4_is_resync */

static int _mpeg4_resync ( void ) {

 int             lLeft, retVal;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;
    
 SMS_SkipBit   ( lpBitCtx );
 SMS_AlignBits ( lpBitCtx );

 if (  SMS_ShowBits ( lpBitCtx, 16 ) == 0  ) {

  retVal = _mpeg4_decode_video_packet_header ();

  if ( retVal >= 0 ) return 0;

 }  /* end if */

 g_MPEGCtx.m_BitCtx = g_MPEGCtx.m_LastResyncBitCtx;

 SMS_AlignBits ( lpBitCtx );

 lLeft = lpBitCtx -> m_szInBits - SMS_BitCount ( lpBitCtx );
    
 for ( ; lLeft > 16 + 1 + 5 + 5; lLeft -= 8 ) { 

  if (  SMS_ShowBits ( lpBitCtx, 16 ) == 0  ) {

   SMS_BitContext lBitCtx = *lpBitCtx;

   retVal = _mpeg4_decode_video_packet_header ();

   if ( retVal >= 0 ) return 0;

   *lpBitCtx = lBitCtx;

  }  /* end if */

  SMS_SkipBits ( lpBitCtx, 8 );

 }  /* end for */
    
 return -1;

}  /* end _mpeg4_resync */

static int _mpeg4_set_direct_mv ( int aMX, int aMY ) {

 const int lMBIdx           = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;
 const int lColocatedMBType = g_MPEGCtx.m_NextPic.m_pMBType[ lMBIdx ];

 int      lXY     = g_MPEGCtx.m_BlockIdx[ 0 ];
 uint16_t lTimePP = g_MPEGCtx.m_PPTime;
 uint16_t lTimePB = g_MPEGCtx.m_PBTime;
 int      i;
    
 if (  SMS_IS_8X8( lColocatedMBType )  ) {

  g_MPEGCtx.m_MVType = SMS_MV_TYPE_8X8;

  for ( i = 0; i < 4; ++i ) {

   lXY = g_MPEGCtx.m_BlockIdx[ i ];

   g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ] * lTimePB / lTimePP + aMX;
   g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] * lTimePB / lTimePP + aMY;
   g_MPEGCtx.m_MV[ 1 ][ i ][ 0 ] = aMX ? g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] - g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ]
                                       : g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ] * ( lTimePB - lTimePP ) / lTimePP;
   g_MPEGCtx.m_MV[ 1 ][ i ][ 1 ] = aMY ? g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] - g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] 
                                       : g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] * ( lTimePB - lTimePP ) / lTimePP;
  }  /* end for */

  return SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_8x8 | SMS_MB_TYPE_L0L1;

 } else if (  SMS_IS_INTERLACED( lColocatedMBType )  ) {

  g_MPEGCtx.m_MVType = SMS_MV_TYPE_FIELD;

  for ( i = 0; i < 2; ++i ) {

   int lFieldSelect = g_MPEGCtx.m_NextPic.m_pRefIdx[ 0 ][  g_MPEGCtx.m_BlockIdx[ 2 * i ]  ];

   if ( g_MPEGCtx.m_TopFieldFirst ) {

    lTimePP = g_MPEGCtx.m_PPFieldTime - lFieldSelect + i;
    lTimePB = g_MPEGCtx.m_PBFieldTime - lFieldSelect + i;

   } else {

    lTimePP = g_MPEGCtx.m_PPFieldTime + lFieldSelect - i;
    lTimePB = g_MPEGCtx.m_PBFieldTime + lFieldSelect - i;

   }  /* end else */

   g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 0 ] * lTimePB / lTimePP + aMX;
   g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 1 ] * lTimePB / lTimePP + aMY;
   g_MPEGCtx.m_MV[ 1 ][ i ][ 0 ] = aMX ? g_MPEGCtx.m_MV[0][i][0] - g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 0 ]
                                       : g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 0 ] * ( lTimePB - lTimePP ) / lTimePP;
   g_MPEGCtx.m_MV[ 1 ][ i ][ 1 ] = aMY ? g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] - g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 1 ] 
                                       : g_MPEGCtx.m_pPFieldMVTbl[ i ][ 0 ][ lMBIdx ][ 1 ] * ( lTimePB - lTimePP ) / lTimePP;
  }  /* end for */

  return SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_16x8 | SMS_MB_TYPE_L0L1 | SMS_MB_TYPE_INTERLACED;

 } else {

  g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = g_MPEGCtx.m_MV[ 0 ][ 1 ][ 0 ] = g_MPEGCtx.m_MV[ 0 ][ 2 ][ 0 ] = g_MPEGCtx.m_MV[ 0 ][ 3 ][ 0 ] = g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ] * lTimePB / lTimePP + aMX;
  g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = g_MPEGCtx.m_MV[ 0 ][ 1 ][ 1 ] = g_MPEGCtx.m_MV[ 0 ][ 2 ][ 1 ] = g_MPEGCtx.m_MV[ 0 ][ 3 ][ 1 ] = g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] * lTimePB / lTimePP + aMY;
  g_MPEGCtx.m_MV[ 1 ][ 0 ][ 0 ] = g_MPEGCtx.m_MV[ 1 ][ 1 ][ 0 ] = g_MPEGCtx.m_MV[ 1 ][ 2 ][ 0 ] = g_MPEGCtx.m_MV[ 1 ][ 3 ][ 0 ] = aMX ? g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] - g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ]
                                                                                                                                      : g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 0 ] * ( lTimePB - lTimePP ) / lTimePP;
  g_MPEGCtx.m_MV[ 1 ][ 0 ][ 1 ] = g_MPEGCtx.m_MV[ 1 ][ 1 ][ 1 ] = g_MPEGCtx.m_MV[ 1 ][ 2 ][ 1 ] = g_MPEGCtx.m_MV[ 1 ][ 3 ][ 1 ] = aMY ? g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] - g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] 
                                                                                                                                      : g_MPEGCtx.m_NextPic.m_pMotionVal[ 0 ][ lXY ][ 1 ] * ( lTimePB  - lTimePP ) / lTimePP;
  if ( !g_MPEGCtx.m_QuarterSample )

   g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

  else g_MPEGCtx.m_MVType = SMS_MV_TYPE_8X8;

  return SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0L1;

 }  /* end else */

}  /* end _mpeg4_set_direct_mv */

void _mpeg4_pred_ac ( SMS_DCTELEM* apBlock, int aN, int aDir ) {

 int           i;
 int16_t*      lpACVal, *lpACVal1;
 int8_t* const lpQScaleTbl = g_MPEGCtx.m_CurPic.m_pQScaleTbl;

 lpACVal  = g_MPEGCtx.m_pACVal[ 0 ][ 0 ] + g_MPEGCtx.m_BlockIdx[ aN ] * 16;
 lpACVal1 = lpACVal;

 if ( g_MPEGCtx.m_ACPred ) {

  if ( aDir == 0 ) {

   const int lXY = g_MPEGCtx.m_MBX - 1 + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;

   lpACVal -= 16;
            
   if ( g_MPEGCtx.m_MBX == 0 || g_MPEGCtx.m_QScale == lpQScaleTbl[ lXY ] || aN == 1 || aN == 3 )

    for ( i = 1; i < 8; ++i ) apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i << 3 ]  ] += lpACVal[ i ];

   else for ( i = 1; i < 8; ++i ) apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i << 3 ]  ] += SMS_ROUNDED_DIV( lpACVal[ i ] * lpQScaleTbl[ lXY ], g_MPEGCtx.m_QScale );

  } else {

   const int lXY = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride - g_MPEGCtx.m_MBStride;

   lpACVal -= 16 * g_MPEGCtx.m_BlockWrap[ aN ];

   if ( g_MPEGCtx.m_MBY == 0 || g_MPEGCtx.m_QScale == lpQScaleTbl[ lXY ] || aN == 2 || aN == 3 )

    for ( i = 1; i < 8; ++i ) apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i ]  ] += lpACVal[ i + 8 ];

   else for ( i = 1; i < 8; ++i ) apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i ]  ] += SMS_ROUNDED_DIV( lpACVal[ i + 8 ] * lpQScaleTbl[ lXY ], g_MPEGCtx.m_QScale );

  }  /* end else */

 }  /* end if */

 for ( i = 1; i < 8; ++i ) lpACVal1[     i ] = apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i << 3 ]  ];
 for ( i = 1; i < 8; ++i ) lpACVal1[ 8 + i ] = apBlock[  g_MPEGCtx.m_DSPCtx.m_Permutation[ i      ]  ];

}  /* end _mpeg4_pred_ac */

static SMS_INLINE int _mpeg4_pred_dc ( int aN, int aLevel, int *apDirPtr ) {

 int       lA, lB, lC, lWrap, lPred, lScale, lRet;
 uint16_t* lpDCVal;

 if ( aN < 4 )

  lScale = g_MPEGCtx.m_Y_DCScale;

 else lScale = g_MPEGCtx.m_C_DCScale;

 lWrap   = g_MPEGCtx.m_BlockWrap[ aN ];
 lpDCVal = g_MPEGCtx.m_pDCVal[ 0 ] + g_MPEGCtx.m_BlockIdx[ aN ];

 lA = lpDCVal[ -1         ];
 lB = lpDCVal[ -1 - lWrap ];
 lC = lpDCVal[ -lWrap     ];

 if ( g_MPEGCtx.m_FirstSliceLine && aN != 3 ) {

  if ( aN != 2                                             ) lB = lC = 1024;
  if ( aN != 1 && g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX ) lB = lA = 1024;

 }  /* end if */

 if ( g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX && g_MPEGCtx.m_MBY == g_MPEGCtx.m_ResyncMBY + 1 )

  if ( aN == 0 || aN == 4 || aN == 5 ) lB = 1024;

 if (  abs ( lA - lB ) < abs ( lB - lC )  ) {

  lPred     = lC;
  *apDirPtr = 1;

 } else {

  lPred     = lA;
  *apDirPtr = 0;

 }  /* end else */

 lPred   = SMS_FASTDIV(   (  lPred + ( lScale >> 1 )  ), lScale   );
 aLevel += lPred;
 lRet    = aLevel;

 if ( g_MPEGCtx.m_pParentCtx -> m_ErrorResilience >= 3 ) {

  if ( aLevel < 0 ) return -1;

  if ( aLevel * lScale > 2048 + lScale ) return -1;

 }  /* end if */

 aLevel *= lScale;

 if (  aLevel & ( ~2047 )  ) {

  if ( aLevel < 0 )

   aLevel = 0;

  else if (  !( g_MPEGCtx.m_Bugs & SMS_BUG_DC_CLIP )  ) aLevel = 2047;

 }  /* end if */

 lpDCVal[ 0 ] = aLevel;

 return lRet;

}  /* end _mpeg4_pred_dc */

static SMS_INLINE int _mpeg4_decode_dc ( int aN, int *apDirPtr ) {

 int             lLevel, lCode;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

 if ( aN < 4 ) 

  lCode = SMS_GetVLC2 ( lpBitCtx, s_dc_lum.m_pTable, DC_VLC_BITS, 1 );

 else lCode = SMS_GetVLC2 ( lpBitCtx, s_dc_chrom.m_pTable, DC_VLC_BITS, 1 );

 if ( lCode < 0 || lCode > 9 ) return -1;

 if ( lCode == 0 )

  lLevel = 0;

 else {

  lLevel = SMS_GetXBits ( lpBitCtx, lCode );

  if ( lCode > 8 && SMS_GetBit ( lpBitCtx ) == 0 && g_MPEGCtx.m_pParentCtx -> m_ErrorResilience >= 2 ) return -1;

 }  /* end else */

 return _mpeg4_pred_dc ( aN, lLevel, apDirPtr );

}  /* end _mpeg4_decode_dc */

static SMS_INLINE int _mpeg4_decode_block (
                       SMS_DCTELEM* apBlock, int aN, int aCoded, int anIntra, int aRVLC
                      ) {

 int              i, lLevel, lLast, lRun;
 int              lDCPredDir;
 SMS_RLTable*     lpRL;
 SMS_RL_VLC_ELEM* lpRLVLC;
 const uint8_t*   lpScanTbl;
 int              lQMul, lQAdd;
    
 if ( anIntra ) {

  if ( g_MPEGCtx.m_QScale < g_MPEGCtx.m_IntraDCThreshold ) {

   if ( g_MPEGCtx.m_PartFrame ) {

    lLevel = g_MPEGCtx.m_pDCVal[ 0 ][  g_MPEGCtx.m_BlockIdx[ aN ]  ];

    if ( aN < 4 )

     lLevel= SMS_FASTDIV(   (  lLevel + ( g_MPEGCtx.m_Y_DCScale >> 1 )  ), g_MPEGCtx.m_Y_DCScale   );

    else lLevel = SMS_FASTDIV(   (  lLevel + ( g_MPEGCtx.m_C_DCScale >> 1 )  ), g_MPEGCtx.m_C_DCScale   );

    lDCPredDir = ( g_MPEGCtx.m_pPredDirTbl[ g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride ] << aN ) & 32;

   } else {

    lLevel = _mpeg4_decode_dc ( aN, &lDCPredDir );

    if ( lLevel < 0 ) return -1;

   }  /* end else */

   apBlock[ 0 ] = lLevel;
   i            = 0;

  } else i = -1;

  if ( !aCoded )  goto not_coded;
        
  if ( aRVLC ) {

   lpRL    = &s_rvlc_rl_intra;
   lpRLVLC = s_rvlc_rl_intra.m_pRLVLC[ 0 ];

  } else {

   lpRL    = &s_rl_intra;
   lpRLVLC = s_rl_intra.m_pRLVLC[ 0 ];

  }  /* end else */

  if ( g_MPEGCtx.m_ACPred ) {

   if ( lDCPredDir == 0 ) 

    lpScanTbl = g_MPEGCtx.m_IntraVScanTbl.m_Permutated;

   else lpScanTbl = g_MPEGCtx.m_IntraHScanTbl.m_Permutated;

  } else lpScanTbl = g_MPEGCtx.m_IntraScanTbl.m_Permutated;

  lQMul = 1;
  lQAdd = 0;

 } else {

  i = -1;

  if ( !aCoded ) {

   g_MPEGCtx.m_BlockLastIdx[ aN ] = i;
   return 0;

  }  /* end if */

  if ( aRVLC )

   lpRL = &s_rvlc_rl_inter;

  else lpRL = &s_rl_inter;
   
  lpScanTbl = g_MPEGCtx.m_IntraScanTbl.m_Permutated;

  if ( g_MPEGCtx.m_MPEGQuant ) {

   lQMul = 1;
   lQAdd = 0;

   if ( aRVLC )

    lpRLVLC = s_rvlc_rl_inter.m_pRLVLC[ 0 ];        

   else lpRLVLC = s_rl_inter.m_pRLVLC[ 0 ];        

  } else {

   lQMul = g_MPEGCtx.m_QScale << 1;
   lQAdd = ( g_MPEGCtx.m_QScale - 1 ) | 1;

   if ( aRVLC )

    lpRLVLC = s_rvlc_rl_inter.m_pRLVLC[ g_MPEGCtx.m_QScale ];        

   else lpRLVLC = s_rl_inter.m_pRLVLC[ g_MPEGCtx.m_QScale ];        

  }  /* end else */

 }  /* end else */

 {  /* begin block */

  SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

  SMS_OPEN_READER( re, lpBitCtx );

  while ( 1 ) {

   SMS_UPDATE_CACHE( re, lpBitCtx );
   SMS_GET_RL_VLC( lLevel, lRun, re, lpBitCtx, lpRLVLC, TEX_VLC_BITS, 2 );

   if ( lLevel == 0 ) {

    if ( aRVLC ) {

     if (  SMS_SHOW_UBITS( re, lpBitCtx, 1 ) == 0  ) return -1;

     SMS_SKIP_CACHE( re, 1 );
 
     lLast = SMS_SHOW_UBITS( re, lpBitCtx, 1 ); SMS_SKIP_CACHE( re, 1 );
     lRun  = SMS_SHOW_UBITS( re, lpBitCtx, 6 ); SMS_LAST_SKIP_CACHE( re, lpBitCtx, 6 );

     SMS_SKIP_COUNTER( re, lpBitCtx, 8 );
     SMS_UPDATE_CACHE( re, lpBitCtx    );
              
     if (  SMS_SHOW_UBITS( re, lpBitCtx, 1 ) == 0  ) return -1;

     SMS_SKIP_CACHE( re, 1 );
 
     lLevel = SMS_SHOW_UBITS( re, lpBitCtx, 11 ); SMS_SKIP_CACHE( re, 11 );
 
     if (  SMS_SHOW_UBITS( re, lpBitCtx, 5 ) != 0x10  ) return -1;

     SMS_SKIP_CACHE( re, 5 );

     lLevel = lLevel * lQMul + lQAdd;
     lLevel = ( lLevel ^ SMS_SHOW_SBITS( re, lpBitCtx, 1 )  ) - SMS_SHOW_SBITS( re, lpBitCtx, 1 );

     SMS_LAST_SKIP_CACHE( re, lpBitCtx, 1 );
     SMS_SKIP_COUNTER( re, lpBi, 18 );

     i += lRun + 1;

     if ( lLast ) i += 192;

    } else {

     int lCache = SMS_GET_CACHE( re, lpBitCtx );

     if ( lCache & 0x80000000 ) {

      if ( lCache & 0x40000000 ) {

       SMS_SKIP_CACHE( re, 2 );

       lLast = SMS_SHOW_UBITS( re, lpBitCtx, 1 ); SMS_SKIP_CACHE( re, 1 );
       lRun  = SMS_SHOW_UBITS( re, lpBitCtx, 6 ); SMS_LAST_SKIP_CACHE( re, lpBitCtx, 6 );

       SMS_SKIP_COUNTER( re, lpBitCtx, 9 );
       SMS_UPDATE_CACHE( re, lpBitCtx );

       if (  SMS_SHOW_UBITS(re, lpBitCtx, 1 ) == 0  ) return -1;

       SMS_SKIP_CACHE( re, 1 );

       lLevel = SMS_SHOW_SBITS( re, lpBitCtx, 12 ); SMS_SKIP_CACHE( re, 12 );

       if (  SMS_SHOW_UBITS( re, lpBitCtx, 1 ) == 0  ) return -1;

       SMS_LAST_SKIP_CACHE( re, lpBitCtx, 1 );
       SMS_SKIP_COUNTER( re, lpBitCtx, 14 );

       if ( lLevel > 0 )

        lLevel= lLevel * lQMul + lQAdd;

       else lLevel= lLevel * lQMul - lQAdd;

       if (  ( unsigned )( lLevel + 2048 ) > 4095  ) {

        if ( g_MPEGCtx.m_pParentCtx -> m_ErrorResilience > SMS_ER_COMPLIANT ) {

         if ( lLevel > 2560 || lLevel < -2560 ) return -1;

        }  /* end if */

        lLevel = lLevel < 0 ? -2048 : 2047;

       }  /* end if */

       i += lRun + 1;

       if ( lLast ) i += 192;

      } else {

       SMS_SKIP_BITS( re, lpBitCtx, 2 );
       SMS_GET_RL_VLC( lLevel, lRun, re, lpBitCtx, lpRLVLC, TEX_VLC_BITS, 2 );

       i     += lRun + lpRL -> m_pMaxRun[ lRun >> 7 ][ lLevel / lQMul ] + 1;
       lLevel = (  lLevel ^ SMS_SHOW_SBITS( re, lpBitCtx, 1 )  ) - SMS_SHOW_SBITS( re, lpBitCtx, 1 );
       SMS_LAST_SKIP_BITS( re, lpBitCtx, 1 );

      }  /* end else */

     } else {

      SMS_SKIP_BITS( re, lpBitCtx, 1 );
      SMS_GET_RL_VLC( lLevel, lRun, re, lpBitCtx, lpRLVLC, TEX_VLC_BITS, 2 );

      i     += lRun;
      lLevel = lLevel + lpRL -> m_pMaxLevel[ lRun >> 7 ][ ( lRun - 1 ) & 63 ] * lQMul;
      lLevel = (  lLevel ^ SMS_SHOW_SBITS( re, lpBitCtx, 1 )  ) - SMS_SHOW_SBITS( re, lpBitCtx, 1 );
      SMS_LAST_SKIP_BITS( re, lpBitCtx, 1 );

     }  /* end else */

    }  /* end else */

   } else {

    i     += lRun;
    lLevel = (  lLevel ^ SMS_SHOW_SBITS( re, lpBitCtx, 1 )  ) - SMS_SHOW_SBITS( re, lpBitCtx, 1 );
    SMS_LAST_SKIP_BITS( re, lpBitCtx, 1 );

   }  /* end else */

   if ( i > 62 ) {

    i -= 192;

    if (  i & ( ~63 )  ) return -1;

    apBlock[  lpScanTbl[ i ]  ] = lLevel;

    break;

   }  /* end if */

   apBlock[  lpScanTbl[ i ]  ] = lLevel;

  }  /* end while */

  SMS_CLOSE_READER( re, lpBitCtx );

 }  /* end block */
not_coded:
 if ( anIntra ) {

  if ( g_MPEGCtx.m_QScale >= g_MPEGCtx.m_IntraDCThreshold ) {

   apBlock[ 0 ] = _mpeg4_pred_dc ( aN, apBlock[ 0 ], &lDCPredDir );
            
   if ( i == -1 ) i = 0;

  }  /* end if */

  _mpeg4_pred_ac ( apBlock, aN, lDCPredDir );

  if ( g_MPEGCtx.m_ACPred ) i = 63;

 }  /* ed if */

 g_MPEGCtx.m_BlockLastIdx[ aN ] = i;

 return 0;

}  /* end _mpeg4_decode_block */

static int _mpeg4_decode_mb ( SMS_DCTELEM aBlock[ 6 ][ 64 ] ) {

 static int8_t lQuantTab[ 4 ] = { -1, -2, 1, 2 };

 const int lXY = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;

 int             i, lCBPC, lCBPY, lCBP, lPredX, lPredY, lMX, lMY, lDQuant;
 int16_t*        lpMotVal;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

 if ( g_MPEGCtx.m_PicType == SMS_FT_P_TYPE || g_MPEGCtx.m_PicType == SMS_FT_S_TYPE ) {

  do {

   if (  SMS_GetBit ( lpBitCtx )  ) {

    g_MPEGCtx.m_MBIntra = 0;

    for ( i = 0; i < 6; ++i ) g_MPEGCtx.m_BlockLastIdx[ i ] = -1;

    g_MPEGCtx.m_MVDir  = SMS_MV_DIR_FORWARD;
    g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

    if ( g_MPEGCtx.m_PicType == SMS_FT_S_TYPE && g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE ) {

     g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_SKIP | SMS_MB_TYPE_GMC | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0;
     g_MPEGCtx.m_MCSel             = 1;
     g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = _get_amv ( 0 );
     g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = _get_amv ( 1 );
     g_MPEGCtx.m_MBSkiped          = 0;

    } else {

     g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_SKIP | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0;
     g_MPEGCtx.m_MCSel             = 0;
     g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = 0;
     g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = 0;
     g_MPEGCtx.m_MBSkiped          = 1;

    }  /* end else */

    goto end;

   }  /* end if */

   lCBPC = SMS_GetVLC2 ( lpBitCtx, s_InterMCBPC_vlc.m_pTable, INTER_MCBPC_VLC_BITS, 2 );

   if ( lCBPC < 0 ) return -1;

  } while ( lCBPC == 20 );

  lDQuant = lCBPC & 8;
  g_MPEGCtx.m_MBIntra = (  ( lCBPC & 4 ) != 0  );

  if ( g_MPEGCtx.m_MBIntra ) goto intra;

  if ( g_MPEGCtx.m_PicType        == SMS_FT_S_TYPE &&
       g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE    &&
       ( lCBPC & 16 ) == 0
  )

   g_MPEGCtx.m_MCSel = SMS_GetBit ( lpBitCtx );

  else g_MPEGCtx.m_MCSel = 0;

  lCBPY = SMS_GetVLC2 ( lpBitCtx, s_cbpy_vlc.m_pTable, CBPY_VLC_BITS, 1 ) ^ 0x0F;
  lCBP  = ( lCBPC & 3 ) | ( lCBPY << 2 );

  if ( lDQuant ) SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale + lQuantTab[ SMS_GetBits ( lpBitCtx, 2 ) ] );

  if (   !g_MPEGCtx.m_ProgSeq && (  lCBP || ( g_MPEGCtx.m_Bugs & SMS_BUG_XVID_ILACE )  )   )

   g_MPEGCtx.m_InterlacedDCT = SMS_GetBit ( lpBitCtx );

  g_MPEGCtx.m_MVDir = SMS_MV_DIR_FORWARD;

  if (  ( lCBPC & 16 ) == 0  ) {

   if ( g_MPEGCtx.m_MCSel ) {

    g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_GMC | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0;
    g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;
    lMX = _get_amv ( 0 );
    lMY = _get_amv ( 1 );

    g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = lMX;
    g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = lMY;

   } else if (  !g_MPEGCtx.m_ProgSeq && SMS_GetBit ( lpBitCtx )  ) {

    g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_16x8 | SMS_MB_TYPE_L0 | SMS_MB_TYPE_INTERLACED;
    g_MPEGCtx.m_MVType = SMS_MV_TYPE_FIELD;
    g_MPEGCtx.m_FieldSelect[ 0 ][ 0 ] = SMS_GetBit ( lpBitCtx );
    g_MPEGCtx.m_FieldSelect[ 0 ][ 1 ] = SMS_GetBit ( lpBitCtx );

    SMS_H263_PredMotion ( 0, 0, &lPredX, &lPredY );
                
    for ( i = 0; i < 2; ++i ) {

     lMX = SMS_H263_DecodeMotion ( lPredX, g_MPEGCtx.m_FCode );

     if ( lMX >= 0xFFFF ) return -1;
            
     lMY = SMS_H263_DecodeMotion ( lPredY / 2, g_MPEGCtx.m_FCode );

     if ( lMY >= 0xFFFF ) return -1;

     g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = lMX;
     g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = lMY;

    }  /* end for */

   } else {

    g_MPEGCtx.m_CurPic.m_pMBType[ lXY ]= SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0; 
    g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

    SMS_H263_PredMotion ( 0, 0, &lPredX, &lPredY );

    lMX = SMS_H263_DecodeMotion ( lPredX, g_MPEGCtx.m_FCode );
            
    if ( lMX >= 0xFFFF ) return -1;
            
    lMY = SMS_H263_DecodeMotion ( lPredY, g_MPEGCtx.m_FCode );
            
    if ( lMY >= 0xFFFF ) return -1;

    g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = lMX;
    g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = lMY;

   }  /* end else */

  } else {

   g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_8x8 | SMS_MB_TYPE_L0; 
   g_MPEGCtx.m_MVType = SMS_MV_TYPE_8X8;

   for ( i = 0; i < 4; ++i ) {

    lpMotVal = SMS_H263_PredMotion ( i, 0, &lPredX, &lPredY );

    lMX = SMS_H263_DecodeMotion ( lPredX, g_MPEGCtx.m_FCode );

    if ( lMX >= 0xFFFF ) return -1;
                
    lMY = SMS_H263_DecodeMotion ( lPredY, g_MPEGCtx.m_FCode );

    if ( lMY >= 0xFFFF ) return -1;

    g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = lMX;
    g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = lMY;

    lpMotVal[ 0 ] = lMX;
    lpMotVal[ 1 ] = lMY;

   }  /* end for */

  }  /* end else */

 } else if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ) {

  int lModB1;
  int lModB2;
  int lMBType;

  g_MPEGCtx.m_MBIntra = 0;
  g_MPEGCtx.m_MCSel   = 0;

  if ( g_MPEGCtx.m_MBX == 0 )

   for ( i = 0; i < 2; ++i )

    g_MPEGCtx.m_LastMV[ i ][ 0 ][ 0 ] = 
    g_MPEGCtx.m_LastMV[ i ][ 0 ][ 1 ] = 
    g_MPEGCtx.m_LastMV[ i ][ 1 ][ 0 ] = 
    g_MPEGCtx.m_LastMV[ i ][ 1 ][ 1 ] = 0;

  g_MPEGCtx.m_MBSkiped = g_MPEGCtx.m_NextPic.m_pMBSkipTbl[ g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride + g_MPEGCtx.m_MBX ];

  if ( g_MPEGCtx.m_MBSkiped ) {

   for ( i = 0; i < 6; ++i ) g_MPEGCtx.m_BlockLastIdx [ i ] = -1;

   g_MPEGCtx.m_MVDir  = SMS_MV_DIR_FORWARD;
   g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;
   g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] =
   g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] =
   g_MPEGCtx.m_MV[ 1 ][ 0 ][ 0 ] =
   g_MPEGCtx.m_MV[ 1 ][ 0 ][ 1 ] = 0;

   g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_SKIP | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0; 

   goto end;

  }  /* end if */

  lModB1 = SMS_GetBit ( lpBitCtx ); 

  if ( lModB1 ) {

   lMBType = SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_SKIP | SMS_MB_TYPE_L0L1;
   lCBP    = 0;

  } else {

   lModB2  = SMS_GetBit ( lpBitCtx );
   lMBType = SMS_GetVLC2 ( lpBitCtx, s_mb_type_b_vlc.m_pTable, MB_TYPE_B_VLC_BITS, 1 );

   if ( lMBType < 0 ) return -1;

   lMBType = s_mb_type_b_map[ lMBType ];

   lCBP = lModB2 ? 0 : SMS_GetBits ( lpBitCtx, 6 );

   if (  !SMS_IS_DIRECT( lMBType ) && lCBP && SMS_GetBit ( lpBitCtx )  )

    SMS_MPEG_SetQScale (  g_MPEGCtx.m_QScale + SMS_GetBit ( lpBitCtx ) * 4 - 2  );

   if ( !g_MPEGCtx.m_ProgSeq ) {

    if ( lCBP ) g_MPEGCtx.m_InterlacedDCT = SMS_GetBit ( lpBitCtx );

    if (  !SMS_IS_DIRECT( lMBType ) && SMS_GetBit ( lpBitCtx )  ) {

     lMBType |=  SMS_MB_TYPE_16x8 | SMS_MB_TYPE_INTERLACED;
     lMBType &= ~SMS_MB_TYPE_16x16;

     if (  SMS_USES_LIST( lMBType, 0 )  ) {

      g_MPEGCtx.m_FieldSelect[ 0 ][ 0 ] = SMS_GetBit ( lpBitCtx );
      g_MPEGCtx.m_FieldSelect[ 0 ][ 1 ] = SMS_GetBit ( lpBitCtx );

     }  /* end if */

     if (  SMS_USES_LIST( lMBType, 1 )  ) {

      g_MPEGCtx.m_FieldSelect[ 1 ][ 0 ] = SMS_GetBit ( lpBitCtx );
      g_MPEGCtx.m_FieldSelect[ 1 ][ 1 ] = SMS_GetBit ( lpBitCtx );

     }  /* end if */

    }  /* end if */

   }  /* end if */

   g_MPEGCtx.m_MVDir = 0;

   if (   (  lMBType & ( SMS_MB_TYPE_DIRECT2 | SMS_MB_TYPE_INTERLACED )  ) == 0   ) {

    g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

    if (  SMS_USES_LIST( lMBType, 0 )  ) {

     g_MPEGCtx.m_MVDir = SMS_MV_DIR_FORWARD;

     lMX = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 0 ], g_MPEGCtx.m_FCode );
     lMY = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 1 ], g_MPEGCtx.m_FCode );

     g_MPEGCtx.m_LastMV[ 0 ][ 1 ][ 0 ] = g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 0 ] = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 0 ] = lMX;
     g_MPEGCtx.m_LastMV[ 0 ][ 1 ][ 1 ] = g_MPEGCtx.m_LastMV[ 0 ][ 0 ][ 1 ] = g_MPEGCtx.m_MV[ 0 ][ 0 ][ 1 ] = lMY;

    }  /* end if */
    
    if (  SMS_USES_LIST( lMBType, 1 )  ) {

     g_MPEGCtx.m_MVDir |= SMS_MV_DIR_BACKWARD;

     lMX = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 0 ], g_MPEGCtx.m_BCode );
     lMY = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 1 ], g_MPEGCtx.m_BCode );

     g_MPEGCtx.m_LastMV[ 1 ][ 1 ][ 0 ] = g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 0 ] = g_MPEGCtx.m_MV[ 1 ][ 0 ][ 0 ] = lMX;
     g_MPEGCtx.m_LastMV[ 1 ][ 1 ][ 1 ] = g_MPEGCtx.m_LastMV[ 1 ][ 0 ][ 1 ] = g_MPEGCtx.m_MV[ 1 ][ 0 ][ 1 ] = lMY;

    }  /* end if */

   } else if (  !SMS_IS_DIRECT( lMBType )  ) {

    g_MPEGCtx.m_MVType = SMS_MV_TYPE_FIELD;

    if (  SMS_USES_LIST( lMBType, 0 )  ) {

     g_MPEGCtx.m_MVDir = SMS_MV_DIR_FORWARD;
                
     for ( i = 0; i < 2; ++i ) {

      lMX = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 0 ][ i ][ 0 ],     g_MPEGCtx.m_FCode );
      lMY = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 0 ][ i ][ 1 ] / 2, g_MPEGCtx.m_FCode );

      g_MPEGCtx.m_LastMV[ 0 ][ i ][ 0 ] =   g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = lMX;
      g_MPEGCtx.m_LastMV[ 0 ][ i ][ 1 ] = ( g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = lMY ) * 2;

     }  /* end for */

    }  /* end if */
    
    if (  SMS_USES_LIST( lMBType, 1 )  ) {

     g_MPEGCtx.m_MVDir |= SMS_MV_DIR_BACKWARD;

     for ( i = 0; i < 2; ++i ) {

      lMX = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 1 ][ i ][ 0 ],     g_MPEGCtx.m_BCode );
      lMY = SMS_H263_DecodeMotion ( g_MPEGCtx.m_LastMV[ 1 ][ i ][ 1 ] / 2, g_MPEGCtx.m_BCode );

      g_MPEGCtx.m_LastMV[ 1 ][ i ][ 0 ] =   g_MPEGCtx.m_MV[ 1 ][ i ][ 0 ] = lMX;
      g_MPEGCtx.m_LastMV[ 1 ][ i ][ 1 ] = ( g_MPEGCtx.m_MV[ 1 ][ i ][ 1 ] = lMY ) * 2;

     }  /* end for */

    }  /* end if */

   }  /* end if */

  }  /* end else */
          
  if (  SMS_IS_DIRECT( lMBType )  ) {

   if (  SMS_IS_SKIP( lMBType )  )

    lMX = lMY = 0;

   else {

    lMX = SMS_H263_DecodeMotion ( 0, 1 );
    lMY = SMS_H263_DecodeMotion ( 0, 1 );

   }  /* end else */
 
   g_MPEGCtx.m_MVDir = SMS_MV_DIR_FORWARD | SMS_MV_DIR_BACKWARD | SMS_MV_DIRECT;
   lMBType |= _mpeg4_set_direct_mv ( lMX, lMY );

  }  /* end if */

  g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = lMBType;

 } else {

  do {

   lCBPC = SMS_GetVLC2 ( lpBitCtx, s_IntraMCBPC_vlc.m_pTable, INTRA_MCBPC_VLC_BITS, 2 );

   if ( lCBPC < 0 ) return -1;

  } while ( lCBPC == 8 );

  lDQuant = lCBPC & 4;
  g_MPEGCtx.m_MBIntra = 1;
intra:
  g_MPEGCtx.m_ACPred = SMS_GetBit ( lpBitCtx );

  if ( g_MPEGCtx.m_ACPred )

   g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_INTRA | SMS_MB_TYPE_ACPRED;

  else g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_INTRA;

  lCBPY = SMS_GetVLC2 ( lpBitCtx, s_cbpy_vlc.m_pTable, CBPY_VLC_BITS, 1 );

  if ( lCBPY < 0 ) return -1;

  lCBP = ( lCBPC & 3 ) | ( lCBPY << 2 );

  if ( lDQuant ) SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale + lQuantTab[ SMS_GetBits ( lpBitCtx, 2 ) ] );
       
  if ( !g_MPEGCtx.m_ProgSeq ) g_MPEGCtx.m_InterlacedDCT = SMS_GetBit ( lpBitCtx );

  for ( i = 0; i < 6; ++i ) {

   if (  _mpeg4_decode_block ( aBlock[ i ], i, lCBP & 32, 1, 0 ) < 0  ) return -1;

   lCBP += lCBP;

  }  /* end for */

  goto end;

 }  /* end else */

 for ( i = 0; i < 6; ++i ) {

  if (  _mpeg4_decode_block ( aBlock[ i ], i, lCBP & 32, 0, 0 ) < 0  ) return -1;

  lCBP += lCBP;

 }  /* end for */
end:
 if (  _mpeg4_is_resync ( lpBitCtx )  ) {

  const int delta = g_MPEGCtx.m_MBX + 1 == g_MPEGCtx.m_MBW ? 2 : 1;

  if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE && g_MPEGCtx.m_NextPic.m_pMBSkipTbl[ lXY + delta ] ) return SMS_SLICE_OK;

  return SMS_SLICE_END;

 }  /* end if */

 return SMS_SLICE_OK;

}  /* end _mpeg4_decode_mb */

static int _mpeg4_decode_partitioned_mb ( SMS_DCTELEM aBlock[ 6 ][ 64 ] ) {

 int       lCBP, lMBType;
 const int lXY = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;

 lMBType = g_MPEGCtx.m_CurPic.m_pMBType[ lXY ];
 lCBP    = g_MPEGCtx.m_pCBPTbl[ lXY ];

 if ( g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] != g_MPEGCtx.m_QScale )

  SMS_MPEG_SetQScale ( g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] );

 if ( g_MPEGCtx.m_PicType == SMS_FT_P_TYPE || g_MPEGCtx.m_PicType == SMS_FT_S_TYPE ) {

  int i;

  for ( i = 0; i < 4; ++i ) {

   g_MPEGCtx.m_MV[ 0 ][ i ][ 0 ] = g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][  g_MPEGCtx.m_BlockIdx[ i ]  ][ 0 ];
   g_MPEGCtx.m_MV[ 0 ][ i ][ 1 ] = g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][  g_MPEGCtx.m_BlockIdx[ i ]  ][ 1 ];

  }  /* end for */

  g_MPEGCtx.m_MBIntra = SMS_IS_INTRA( lMBType );

  if (  SMS_IS_SKIP( lMBType )  ) {

   for ( i = 0; i < 6; ++i ) g_MPEGCtx.m_BlockLastIdx[ i ] = -1;

   g_MPEGCtx.m_MVDir  = SMS_MV_DIR_FORWARD;
   g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

   if ( g_MPEGCtx.m_PicType == SMS_FT_S_TYPE && g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE ) {

    g_MPEGCtx.m_MCSel    = 1;
    g_MPEGCtx.m_MBSkiped = 0;

   } else {

    g_MPEGCtx.m_MCSel    = 0;
    g_MPEGCtx.m_MBSkiped = 1;

   }  /* end else */

  } else if ( g_MPEGCtx.m_MBIntra ) {

   g_MPEGCtx.m_ACPred = SMS_IS_ACPRED( g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] );

  } else if ( !g_MPEGCtx.m_MBIntra ) {

   g_MPEGCtx.m_MVDir  = SMS_MV_DIR_FORWARD;
   g_MPEGCtx.m_MVType = SMS_IS_8X8( lMBType ) ? SMS_MV_TYPE_8X8 : SMS_MV_TYPE_16X16;

  }  /* end if */

 } else {

  g_MPEGCtx.m_MBIntra = 1;
  g_MPEGCtx.m_ACPred  = SMS_IS_ACPRED( g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] );

 }  /* end else */

 if (  !SMS_IS_SKIP( lMBType )  ) {

  int i;

  for ( i = 0; i < 6; ++i ) {

   if (  _mpeg4_decode_block ( aBlock[ i ], i, lCBP & 32, g_MPEGCtx.m_MBIntra, g_MPEGCtx.m_RVLC ) < 0  ) return -1;

   lCBP += lCBP;

  }  /* end for */

 }  /* end if */

 if ( --g_MPEGCtx.m_nMBLeft <= 0 ) {

  if (  _mpeg4_is_resync ( &g_MPEGCtx.m_BitCtx )  )

   return SMS_SLICE_END;

  else return SMS_SLICE_NOEND;

 } else {

  if (  _mpeg4_is_resync ( &g_MPEGCtx.m_BitCtx )  ) {

   const int lDelta = g_MPEGCtx.m_MBX + 1 == g_MPEGCtx.m_MBW ? 2 : 1;

   if ( g_MPEGCtx.m_pCBPTbl[ lXY + lDelta ] ) return SMS_SLICE_END;

  }  /* end if */

  return SMS_SLICE_OK;

 }  /* end else */

 return 0;

}  /* end _mpeg4_decode_partitioned_mb */

static int _decode_vop_header ( SMS_BitContext* apBitCtx ) {

 int lTimeInc[ 2 ];

 g_MPEGCtx.m_PicType = SMS_GetBits ( apBitCtx, 2 ) + SMS_FT_I_TYPE;

 if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE &&
      g_MPEGCtx.m_LowDelay                 &&
      g_MPEGCtx.m_VolCtlPar == 0           &&
      !( g_MPEGCtx.m_Flags & SMS_CODEC_FLAG_LOW_DELAY )
 ) g_MPEGCtx.m_LowDelay = 0;

 g_MPEGCtx.m_PartFrame = g_MPEGCtx.m_DataPart && g_MPEGCtx.m_PicType != SMS_FT_B_TYPE;
 g_MPEGCtx.DecodeMB    = g_MPEGCtx.m_PartFrame ? _mpeg4_decode_partitioned_mb
                                               : _mpeg4_decode_mb;

 if ( g_MPEGCtx.m_TimeIncRes == 0 ) g_MPEGCtx.m_TimeIncRes = 1;

 lTimeInc[ 0 ] = 0;

 while (  SMS_GetBit ( apBitCtx ) != 0  ) ++lTimeInc[ 0 ];

 SMS_GetBit ( apBitCtx );  // check marker

 if ( g_MPEGCtx.m_TimeIncBits == 0 )

  for ( g_MPEGCtx.m_TimeIncBits =  1;
        g_MPEGCtx.m_TimeIncBits < 16;
      ++g_MPEGCtx.m_TimeIncBits
  ) if (  SMS_ShowBits ( apBitCtx, g_MPEGCtx.m_TimeIncBits + 1 ) & 1  ) break;

 lTimeInc[ 1 ] = SMS_GetBits ( apBitCtx, g_MPEGCtx.m_TimeIncBits );

 if ( g_MPEGCtx.m_PicType != SMS_FT_B_TYPE ) {

  g_MPEGCtx.m_LastTimeBase  = g_MPEGCtx.m_TimeBase;
  g_MPEGCtx.m_TimeBase     += lTimeInc[ 0 ];
  g_MPEGCtx.m_Time          = g_MPEGCtx.m_TimeBase   *
                              g_MPEGCtx.m_TimeIncRes + lTimeInc[ 1 ];
  g_MPEGCtx.m_PPTime        = ( uint16_t )(  g_MPEGCtx.m_Time - g_MPEGCtx.m_LastNonBTime  );
  g_MPEGCtx.m_LastNonBTime  = g_MPEGCtx.m_Time;

 } else {

  g_MPEGCtx.m_Time   = ( g_MPEGCtx.m_LastTimeBase + lTimeInc[ 0 ] ) *
                         g_MPEGCtx.m_TimeIncRes   + lTimeInc[ 1 ];
  g_MPEGCtx.m_PBTime = ( uint16_t )(  g_MPEGCtx.m_PPTime - ( g_MPEGCtx.m_LastNonBTime - g_MPEGCtx.m_Time )  );

  if ( g_MPEGCtx.m_PPTime <= g_MPEGCtx.m_PBTime                      ||
       g_MPEGCtx.m_PPTime <= g_MPEGCtx.m_PPTime - g_MPEGCtx.m_PBTime ||
       g_MPEGCtx.m_PPTime <= 0
  ) return SMS_FRAME_SKIPED;

  if ( g_MPEGCtx.m_TFrame == 0 ) g_MPEGCtx.m_TFrame = g_MPEGCtx.m_PBTime;
  if ( g_MPEGCtx.m_TFrame == 0 ) g_MPEGCtx.m_TFrame = 1;

  g_MPEGCtx.m_PPFieldTime = ( uint16_t )(
                             (  SMS_ROUNDED_DIV( g_MPEGCtx.m_LastNonBTime, g_MPEGCtx.m_TFrame ) -
                                SMS_ROUNDED_DIV( g_MPEGCtx.m_LastNonBTime - g_MPEGCtx.m_PPTime, g_MPEGCtx.m_TFrame )
                             ) * 2
                            );
  g_MPEGCtx.m_PBFieldTime = ( uint16_t )(
                             (  SMS_ROUNDED_DIV( g_MPEGCtx.m_Time, g_MPEGCtx.m_TFrame ) -
                                SMS_ROUNDED_DIV( g_MPEGCtx.m_LastNonBTime - g_MPEGCtx.m_PPTime, g_MPEGCtx.m_TFrame )
                             ) * 2
                            );
 }  /* end else */

 g_MPEGCtx.m_pCurPic -> m_PTS = g_MPEGCtx.m_Time * ( int64_t )SMS_TIME_BASE / g_MPEGCtx.m_TimeIncRes;

 SMS_GetBit ( apBitCtx );  // check marker

 if (  SMS_GetBit ( apBitCtx ) != 1  ) return SMS_FRAME_SKIPED;

 if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE &&
      ( g_MPEGCtx.m_PicType == SMS_FT_P_TYPE || (
         g_MPEGCtx.m_PicType == SMS_FT_S_TYPE && g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE
        )
      )
 )

  g_MPEGCtx.m_NoRounding = SMS_GetBit ( apBitCtx );

 else g_MPEGCtx.m_NoRounding = 0;

 if ( g_MPEGCtx.m_Shape != SMS_RECT_SHAPE ) {

  if ( g_MPEGCtx.m_VolSpriteUsage != 1 || g_MPEGCtx.m_PicType != SMS_FT_I_TYPE ) {

   SMS_GetBits ( apBitCtx, 13 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 13 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 13 );
   SMS_SkipBit ( apBitCtx );
   SMS_GetBits ( apBitCtx, 13 );

  }  /* end if */

  SMS_SkipBit ( apBitCtx );
 
  if (  SMS_GetBit ( apBitCtx ) != 0  ) SMS_SkipBits ( apBitCtx, 8 );

 }  /* end if */

 if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE ) {

  g_MPEGCtx.m_IntraDCThreshold = s_dc_threshold[ SMS_GetBits ( apBitCtx, 3 ) ];

  if ( !g_MPEGCtx.m_ProgSeq ) {

   g_MPEGCtx.m_TopFieldFirst = SMS_GetBit ( apBitCtx );
   g_MPEGCtx.m_AltScan       = SMS_GetBit ( apBitCtx );

  } else g_MPEGCtx.m_AltScan = 0;

 }  /* end if */

 if ( g_MPEGCtx.m_AltScan ) {

  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_InterScanTbl,  g_SMS_DSP_alternate_vertical_scan );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraScanTbl,  g_SMS_DSP_alternate_vertical_scan );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraHScanTbl, g_SMS_DSP_alternate_vertical_scan );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraVScanTbl, g_SMS_DSP_alternate_vertical_scan );

 } else {

  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_InterScanTbl,  g_SMS_DSP_zigzag_direct             );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraScanTbl,  g_SMS_DSP_zigzag_direct             );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraHScanTbl, g_SMS_DSP_alternate_horizontal_scan );
  SMS_MPEG_InitScanTable ( g_MPEGCtx.m_DSPCtx.m_Permutation, &g_MPEGCtx.m_IntraVScanTbl, g_SMS_DSP_alternate_vertical_scan   );

 }  /* end else */

 if ( g_MPEGCtx.m_PicType == SMS_FT_S_TYPE && (
       g_MPEGCtx.m_VolSpriteUsage == STATIC_SPRITE ||
       g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE
      )
 ) _mpeg4_decode_sprite_trajectory ( apBitCtx );

 if ( g_MPEGCtx.m_Shape != SMS_BIN_ONLY_SHAPE ) {

  g_MPEGCtx.m_ChromaQScale = g_MPEGCtx.m_QScale = SMS_GetBits ( apBitCtx, g_MPEGCtx.m_QuantPrec );

  if ( g_MPEGCtx.m_QScale == 0 ) return -1;

  if ( g_MPEGCtx.m_PicType != SMS_FT_I_TYPE ) {

   g_MPEGCtx.m_FCode = SMS_GetBits ( apBitCtx, 3 );

   if ( g_MPEGCtx.m_FCode == 0 ) return -1;

  } else g_MPEGCtx.m_FCode = 1;
     
  g_MPEGCtx.m_BCode = g_MPEGCtx.m_PicType == SMS_FT_B_TYPE ? SMS_GetBits ( apBitCtx, 3 ) : 1;

  if ( !g_MPEGCtx.m_Scalability ) {

   if ( g_MPEGCtx.m_Shape != SMS_RECT_SHAPE && g_MPEGCtx.m_PicType != SMS_FT_I_TYPE ) SMS_SkipBit ( apBitCtx );

  } else {

   if ( g_MPEGCtx.m_EnhancementType ) SMS_GetBit ( apBitCtx );

   SMS_SkipBits ( apBitCtx, 2 );

  }  /* end else */

 }  /* end if */

 if ( g_MPEGCtx.m_VoType    == 0 &&
      g_MPEGCtx.m_VolCtlPar == 0 &&
      g_MPEGCtx.m_DivXVer   == 0 &&
      g_MPEGCtx.m_PicNr     == 0
 ) g_MPEGCtx.m_LowDelay = 1;

 ++g_MPEGCtx.m_PicNr;

 g_MPEGCtx.m_pY_DCScaleTbl = s_y_dc_scale_table;
 g_MPEGCtx.m_pC_DCScaleTbl = s_c_dc_scale_table;

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_EDGE ) {

  g_MPEGCtx.m_HEdgePos = g_MPEGCtx.m_Width;
  g_MPEGCtx.m_VEdgePos = g_MPEGCtx.m_Height;

 } /* end if */

 return 1;

}  /* end _decode_vop_header */

static int Codec_MPEG4_DecodeHeader ( void ) {

 int             lStartCode, lV;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

 SMS_AlignBits ( lpBitCtx );

 lStartCode = 0xFF;

 while ( 1 ) {

  lV         = SMS_GetBits ( lpBitCtx, 8 );
  lStartCode = (  ( lStartCode << 8 ) | lV  ) & 0xFFFFFFFF;
        
  if (  SMS_BitCount ( lpBitCtx ) >= lpBitCtx -> m_szInBits )

   return lpBitCtx -> m_szInBits == 8 && g_MPEGCtx.m_DivXVersion ? SMS_FRAME_SKIPED : -1;

  if (  ( lStartCode & 0xFFFFFF00 ) != 0x100  ) continue;

  if( lStartCode >= 0x120 && lStartCode <= 0x12F )

   _decode_vol_header ( lpBitCtx );

  else if ( lStartCode == USER_DATA_STARTCODE )

   _decode_user_data ( lpBitCtx );

  else if ( lStartCode == GOP_STARTCODE )

   _decode_gop_header ( lpBitCtx );

  else if ( lStartCode == VOP_STARTCODE ) return _decode_vop_header ( lpBitCtx );

  SMS_AlignBits ( lpBitCtx );
  lStartCode = 0xFF;

 }  /* end while */

}  /* end Codec_MPEG4_DecodeHeader */

static int _mpeg4_decode_partition_a ( void ) {

 static const int8_t s_QuantTbl[ 4 ] = { -1, -2, 1, 2 };

 int             lMBNum   = 0;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;
    
 g_MPEGCtx.m_FirstSliceLine = 1;

 for ( ; g_MPEGCtx.m_MBY < g_MPEGCtx.m_MBH; ++g_MPEGCtx.m_MBY ) {

  SMS_MPEG_InitBlockIdx ();

  for ( ; g_MPEGCtx.m_MBX < g_MPEGCtx.m_MBW; ++g_MPEGCtx.m_MBX ) {

   const int lXY = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;
   int       lCBPC;
   int       lDir = 0;
            
   ++lMBNum;

   SMS_MPEG_UpdateBlockIdx ();

   if ( g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX &&
        g_MPEGCtx.m_MBY == g_MPEGCtx.m_ResyncMBY + 1
   ) g_MPEGCtx.m_FirstSliceLine = 0;
            
   if ( g_MPEGCtx.m_PicType == SMS_FT_I_TYPE ) {

    int i;

    do {

     if (  SMS_ShowBitsLong ( lpBitCtx, 19 ) == DC_MARKER  ) return lMBNum - 1;

     lCBPC = SMS_GetVLC2 ( lpBitCtx, s_IntraMCBPC_vlc.m_pTable, INTRA_MCBPC_VLC_BITS, 2 );

     if ( lCBPC < 0 ) return -1;

    } while ( lCBPC == 8 );
                
    g_MPEGCtx.m_pCBPTbl         [ lXY ] = lCBPC & 3;
    g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_INTRA;
    g_MPEGCtx.m_MBIntra                 = 1;

    if ( lCBPC & 4 ) SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale + s_QuantTbl[ SMS_GetBits ( lpBitCtx, 2 ) ] );

    g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] = g_MPEGCtx.m_QScale;
    g_MPEGCtx.m_pMBIntraTbl        [ lXY ] = 1;

    for ( i = 0; i < 6; ++i ) {

     int lDCPredDir;
     int lDC = _mpeg4_decode_dc ( i, &lDCPredDir );

     if ( lDC < 0 ) return -1;

     lDir <<= 1;

     if ( lDCPredDir ) lDir |= 1;

    }  /* end for */

    g_MPEGCtx.m_pPredDirTbl[ lXY ] = lDir;

   } else {

    int            lMX, lMY, lPredX, lPredY, lBits;
    int16_t* const lpMotVal = g_MPEGCtx.m_CurPic.m_pMotionVal[ 0 ][  g_MPEGCtx.m_BlockIdx[ 0 ]  ];
    const int      lStride  = g_MPEGCtx.m_B8Stride * 2;
try_again:
    lBits = SMS_ShowBits ( lpBitCtx, 17 );

    if ( lBits == MOTION_MARKER ) return lMBNum - 1;

    SMS_SkipBit ( lpBitCtx );

    if ( lBits & 0x10000 ) {

     if ( g_MPEGCtx.m_PicType        == SMS_FT_S_TYPE &&
          g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE
     ) {

      g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_SKIP | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_GMC | SMS_MB_TYPE_L0;

      lMX = _get_amv ( 0 );
      lMY = _get_amv ( 1 );

     } else {

      g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_SKIP | SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0;

      lMX = lMY = 0;

     }  /* end else */

     lpMotVal[ 0           ] = lpMotVal[ 2           ] =
     lpMotVal[ lStride     ] = lpMotVal[ lStride + 2 ] = lMX;
     lpMotVal[ 1           ] = lpMotVal[ 3           ] =
     lpMotVal[ lStride + 1 ] = lpMotVal[ lStride + 3 ] = lMY;

     if ( g_MPEGCtx.m_pMBIntraTbl[ lXY ] ) SMS_MPEG_CleanIntraTblEntries ();

     continue;

    }  /* end if */

    lCBPC = SMS_GetVLC2 ( lpBitCtx, s_InterMCBPC_vlc.m_pTable, INTER_MCBPC_VLC_BITS, 2 );

    if ( lCBPC < 0 ) return -1;

    if ( lCBPC == 20 ) goto try_again;

    g_MPEGCtx.m_pCBPTbl[ lXY ] = lCBPC & ( 8 + 3 );
    g_MPEGCtx.m_MBIntra        = (  ( lCBPC & 4 ) != 0  );
        
    if ( g_MPEGCtx.m_MBIntra ) {

     g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_INTRA;
     g_MPEGCtx.m_pMBIntraTbl     [ lXY ] = 1;

     lpMotVal[ 0           ] = lpMotVal[ 2           ] = 
     lpMotVal[ lStride     ] = lpMotVal[ lStride + 2 ] = 0;
     lpMotVal[ 1           ] = lpMotVal[ 3           ] =
     lpMotVal[ lStride + 1 ] = lpMotVal[ lStride + 3 ] = 0;

    } else {

     if ( g_MPEGCtx.m_pMBIntraTbl[ lXY ] ) SMS_MPEG_CleanIntraTblEntries ();

     if ( g_MPEGCtx.m_PicType        == SMS_FT_S_TYPE &&
          g_MPEGCtx.m_VolSpriteUsage == GMC_SPRITE    &&
          ( lCBPC & 16 ) == 0
     )

      g_MPEGCtx.m_MCSel = SMS_GetBit ( lpBitCtx );

     else g_MPEGCtx.m_MCSel = 0;
        
     if (  ( lCBPC & 16 ) == 0  ) {

      SMS_H263_PredMotion ( 0, 0, &lPredX, &lPredY );

      if ( !g_MPEGCtx.m_MCSel ) {

       lMX = SMS_H263_DecodeMotion ( lPredX, g_MPEGCtx.m_FCode );

       if ( lMX >= 0xFFFF ) return -1;

       lMY = SMS_H263_DecodeMotion ( lPredY, g_MPEGCtx.m_FCode );

       if ( lMY >= 0xFFFF ) return -1;

       g_MPEGCtx.m_CurPic.m_pMBType[ lXY ]= SMS_MB_TYPE_16x16 | SMS_MB_TYPE_L0;

      } else {

       lMX = _get_amv ( 0 );
       lMY = _get_amv ( 1 );

       g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_16x16 | SMS_MB_TYPE_GMC | SMS_MB_TYPE_L0;

      }  /* end else */

      lpMotVal[ 0           ] = lpMotVal[ 2           ] =
      lpMotVal[ lStride     ] = lpMotVal[ lStride + 2 ] = lMX;
      lpMotVal[ 1           ] = lpMotVal[ 3           ] =
      lpMotVal[ lStride + 1 ] = lpMotVal[ lStride + 3 ] = lMY;

     } else {

      int i;

      g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] = SMS_MB_TYPE_8x8 | SMS_MB_TYPE_L0;

      for ( i = 0; i < 4; ++i ) {

       int16_t* lpMotVal = SMS_H263_PredMotion ( i, 0, &lPredX, &lPredY );

       lMX = SMS_H263_DecodeMotion ( lPredX, g_MPEGCtx.m_FCode );

       if ( lMX >= 0xFFFF ) return -1;
                
       lMY = SMS_H263_DecodeMotion ( lPredY, g_MPEGCtx.m_FCode );

       if ( lMY >= 0xFFFF ) return -1;

       lpMotVal[ 0 ] = lMX;
       lpMotVal[ 1 ] = lMY;

      }  /* end for */

     }  /* end else */

    }  /* end else */

   }  /* end else */

  }  /* end for */

  g_MPEGCtx.m_MBX = 0;

 }  /* end for */

 return lMBNum;

}  /* end _mpeg4_decode_partition_a */

static int _mpeg4_decode_partition_b ( int aMBCount ) {

 static const int8_t s_QuantTbl[ 4 ] = { -1, -2, 1, 2 };

 int             lMBNum   = 0;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;

 g_MPEGCtx.m_MBX            = g_MPEGCtx.m_ResyncMBX;
 g_MPEGCtx.m_FirstSliceLine = 1;

 for ( g_MPEGCtx.m_MBY = g_MPEGCtx.m_ResyncMBY; lMBNum < aMBCount; ++g_MPEGCtx.m_MBY ) {

  SMS_MPEG_InitBlockIdx ();

  for ( ; lMBNum < aMBCount && g_MPEGCtx.m_MBX < g_MPEGCtx.m_MBW; ++g_MPEGCtx.m_MBX ) {

   const int lXY = g_MPEGCtx.m_MBX + g_MPEGCtx.m_MBY * g_MPEGCtx.m_MBStride;

   ++lMBNum;

   SMS_MPEG_UpdateBlockIdx ();

   if ( g_MPEGCtx.m_MBX == g_MPEGCtx.m_ResyncMBX &&
        g_MPEGCtx.m_MBY == g_MPEGCtx.m_ResyncMBY + 1
   ) g_MPEGCtx.m_FirstSliceLine = 0;
            
   if ( g_MPEGCtx.m_PicType == SMS_FT_I_TYPE ) {

    int lACPred = SMS_GetBit ( lpBitCtx );
    int lCBPY   = SMS_GetVLC2 ( lpBitCtx, s_cbpy_vlc.m_pTable, CBPY_VLC_BITS, 1 );

    if ( lCBPY < 0 ) return -1;
                
    g_MPEGCtx.m_pCBPTbl[ lXY ]          |= lCBPY << 2;
    g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] |= lACPred * SMS_MB_TYPE_ACPRED; 

   } else {

    if (  SMS_IS_INTRA( g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] )  ) {

     int i, lDir = 0;
     int lACPred = SMS_GetBit ( lpBitCtx );
     int lCBPY   = SMS_GetVLC2 ( lpBitCtx, s_cbpy_vlc.m_pTable, CBPY_VLC_BITS, 1 );

     if ( lCBPY < 0 ) return -1;
                    
     if ( g_MPEGCtx.m_pCBPTbl[ lXY ] & 8 )

      SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale + s_QuantTbl[ SMS_GetBits ( lpBitCtx, 2 ) ] );

     g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] = g_MPEGCtx.m_QScale;

     for ( i = 0; i < 6; ++i ) {

      int lDCPredDir;
      int lDC = _mpeg4_decode_dc ( i, &lDCPredDir );

      if ( lDC < 0 ) return -1;

      lDir <<= 1;

      if ( lDCPredDir ) lDir |= 1;

     }  /* end for */

     g_MPEGCtx.m_pCBPTbl         [ lXY ] &= 3;
     g_MPEGCtx.m_pCBPTbl         [ lXY ] |= lCBPY << 2;
     g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] |= lACPred * SMS_MB_TYPE_ACPRED; 
     g_MPEGCtx.m_pPredDirTbl     [ lXY ]  = lDir;

    } else if (  SMS_IS_SKIP( g_MPEGCtx.m_CurPic.m_pMBType[ lXY ] )  ) {

     g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] = g_MPEGCtx.m_QScale;
     g_MPEGCtx.m_pCBPTbl            [ lXY ] = 0;

    } else {

     int lCBPY = SMS_GetVLC2 ( lpBitCtx, s_cbpy_vlc.m_pTable, CBPY_VLC_BITS, 1 );

     if ( lCBPY < 0 ) return -1;
                    
     if ( g_MPEGCtx.m_pCBPTbl[ lXY ] & 8 )

      SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale + s_QuantTbl[ SMS_GetBits ( lpBitCtx, 2 ) ] );

     g_MPEGCtx.m_CurPic.m_pQScaleTbl[ lXY ] = g_MPEGCtx.m_QScale;

     g_MPEGCtx.m_pCBPTbl[ lXY ] &= 3;
     g_MPEGCtx.m_pCBPTbl[ lXY ] |= ( lCBPY ^0xF ) << 2;

    }  /* end else */

   }  /* end else */

  }  /* end for */

  if ( lMBNum >= aMBCount ) return 0;

  g_MPEGCtx.m_MBX = 0;

 }  /* end for */

 return 0;

}  /* end _mpeg4_decode_partition_b */

int _mpeg4_decode_partitions ( void ) {

 int             lMBNum;
 SMS_BitContext* lpBitCtx = &g_MPEGCtx.m_BitCtx;
    
 lMBNum = _mpeg4_decode_partition_a ();

 if ( lMBNum < 0 ) return -1;

 if ( g_MPEGCtx.m_ResyncMBX + g_MPEGCtx.m_ResyncMBY * g_MPEGCtx.m_MBW + lMBNum > g_MPEGCtx.m_MBNum ) return -1;

 if ( g_MPEGCtx.m_PicType == SMS_FT_I_TYPE ) {

  while (  SMS_ShowBits ( lpBitCtx, 9 ) == 1  ) SMS_SkipBits ( lpBitCtx, 9 );

   if (  SMS_GetBitsLong ( lpBitCtx, 19 ) != DC_MARKER  ) return -1;

 } else {

  while (  SMS_ShowBits ( lpBitCtx, 10 ) == 1  ) SMS_SkipBits ( lpBitCtx, 10 );

  if (  SMS_GetBits ( lpBitCtx, 17 ) != MOTION_MARKER ) return -1;

 }  /* end else */

 if (  _mpeg4_decode_partition_b ( lMBNum ) < 0  ) return -1;

 return lMBNum;

}  /* end _mpeg4_decode_partitions */

static int32_t _decode_slice ( void ) {

 g_MPEGCtx.m_LastResyncBitCtx = g_MPEGCtx.m_BitCtx;
 g_MPEGCtx.m_FirstSliceLine   = 1;
 g_MPEGCtx.m_ResyncMBX        = g_MPEGCtx.m_MBX;
 g_MPEGCtx.m_ResyncMBY        = g_MPEGCtx.m_MBY;
 g_MPEGCtx.m_pCache           = NULL;

 SMS_MPEG_SetQScale ( g_MPEGCtx.m_QScale );

 if ( g_MPEGCtx.m_PartFrame ) {

  const int lQScale = g_MPEGCtx.m_QScale;

  if (  _mpeg4_decode_partitions () < 0  ) return -1; 
     
  g_MPEGCtx.m_FirstSliceLine = 1;
  g_MPEGCtx.m_MBX = g_MPEGCtx.m_ResyncMBX;
  g_MPEGCtx.m_MBY = g_MPEGCtx.m_ResyncMBY;

  SMS_MPEG_SetQScale ( lQScale );

 }  /* end if */

 for ( ; g_MPEGCtx.m_MBY < g_MPEGCtx.m_MBH; ++g_MPEGCtx.m_MBY ) {

  SMS_MPEG_InitBlockIdx ();

  for ( ; g_MPEGCtx.m_MBX < g_MPEGCtx.m_MBW; ++g_MPEGCtx.m_MBX ) {

   int lRet;

   SMS_MPEG_UpdateBlockIdx ();

   if ( g_MPEGCtx.m_ResyncMBX     == g_MPEGCtx.m_MBX &&
        g_MPEGCtx.m_ResyncMBY + 1 == g_MPEGCtx.m_MBY
   ) g_MPEGCtx.m_FirstSliceLine = 0;

   g_MPEGCtx.m_DSPCtx.IDCT_ClrBlocks ( g_MPEGCtx.m_pBlock[ 0 ] );
            
   g_MPEGCtx.m_MVDir  = SMS_MV_DIR_FORWARD;
   g_MPEGCtx.m_MVType = SMS_MV_TYPE_16X16;

   lRet = g_MPEGCtx.DecodeMB ( g_MPEGCtx.m_pBlock );

   if ( g_MPEGCtx.m_PicType != SMS_FT_B_TYPE ) SMS_H263_UpdateMotionVal ();

   if ( lRet < 0 ) {

    if ( lRet == SMS_SLICE_END ) {

     SMS_MPEG_DecodeMB ( g_MPEGCtx.m_pBlock );

     --g_MPEGCtx.m_PaddingBugScore;
                        
     if ( ++g_MPEGCtx.m_MBX >= g_MPEGCtx.m_MBW ) {

        g_MPEGCtx.m_MBX = 0;
      ++g_MPEGCtx.m_MBY;

     }  /* end if */

     return 0; 

    } else if ( lRet == SMS_SLICE_NOEND ) return -1;

    return -1;

   }  /* end if */

   SMS_MPEG_DecodeMB ( g_MPEGCtx.m_pBlock );

  }  /* end for */

  g_MPEGCtx.m_MBX = 0;

 }  /* end for */

 if (  ( g_MPEGCtx.m_Bugs & SMS_BUG_AUTODETECT )                                  &&
       g_MPEGCtx.m_BitCtx.m_szInBits - SMS_BitCount ( &g_MPEGCtx.m_BitCtx ) >=  0 &&
       g_MPEGCtx.m_BitCtx.m_szInBits - SMS_BitCount ( &g_MPEGCtx.m_BitCtx )  < 48 &&
      !g_MPEGCtx.m_DataPart
 ) {
        
  const int lBitCount = SMS_BitCount ( &g_MPEGCtx.m_BitCtx );
  const int lBitLeft  = g_MPEGCtx.m_BitCtx.m_szInBits - lBitCount;
        
  if ( lBitLeft == 0 )

   g_MPEGCtx.m_PaddingBugScore += 16;

  else if ( lBitLeft > 8 )

   ++g_MPEGCtx.m_PaddingBugScore;

  else if ( lBitLeft != 1 ) {

   int lV  = SMS_ShowBits ( &g_MPEGCtx.m_BitCtx, 8 );
       lV |= 0x7F >> (  7 - ( lBitCount & 7 )  );

   if ( lV == 0x7F )

    --g_MPEGCtx.m_PaddingBugScore;

   else ++g_MPEGCtx.m_PaddingBugScore;

  }  /* end if */

 }  /* end if */

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_NO_PADDING ) {

  int lLeft     = g_MPEGCtx.m_BitCtx.m_szInBits - SMS_BitCount ( &g_MPEGCtx.m_BitCtx );
  int lMaxExtra = 7;
        
  if (  ( g_MPEGCtx.m_Bugs & SMS_BUG_NO_PADDING ) && g_MPEGCtx.m_pParentCtx -> m_ErrorResilience >= 3  )

   lMaxExtra += 48;

  else lMaxExtra += 256 * 256 * 256 * 64;
        
  if ( lLeft < lMaxExtra && lLeft > 0 ) return 0;

 }  /* end if */

 return -1;

}  /* end _decode_slice */

static int32_t MPEG4_Decode ( SMS_CodecContext* apCtx, void** apData, uint8_t* apBuf, int32_t aBufSize ) {

 SMS_FrameBuffer** lpFrame  = ( SMS_FrameBuffer** )apData;
 SMS_BitContext*   lpBitCtx = &g_MPEGCtx.m_BitCtx;
 int32_t           retVal;

 g_MPEGCtx.m_pParentCtx = apCtx;

 if (  g_MPEGCtx.m_BSBufSize && ( g_MPEGCtx.m_DivXPack || aBufSize < 20 )  )

  SMS_InitGetBits ( lpBitCtx, g_MPEGCtx.m_pBSBuf, g_MPEGCtx.m_BSBufSize << 3 );

 else SMS_InitGetBits ( lpBitCtx, apBuf, aBufSize << 3 );

 g_MPEGCtx.m_BSBufSize = 0;

 if ( g_MPEGCtx.m_pCurPic == NULL || g_MPEGCtx.m_pCurPic -> m_pBuf )

  g_MPEGCtx.m_pCurPic = &g_MPEGCtx.m_pPic[ SMS_MPEGContext_FindUnusedPic () ];

 retVal = Codec_MPEG4_DecodeHeader ();

 if ( retVal == SMS_FRAME_SKIPED || retVal < 0 ) return 0;

 apCtx -> m_HasBFrames = !g_MPEGCtx.m_LowDelay;

 if ( g_MPEGCtx.m_XviDBuild == 0 && g_MPEGCtx.m_DivXVer == 0 )

  if ( apCtx -> m_Tag == SMS_FourCC_XVID || 
       apCtx -> m_Tag == SMS_FourCC_XVIX
  ) g_MPEGCtx.m_XviDBuild = -1;

 if ( g_MPEGCtx.m_XviDBuild == 0 && g_MPEGCtx.m_DivXVer == 0 )

  if ( apCtx -> m_Tag == SMS_FourCC_DIVX &&
       g_MPEGCtx.m_VoType    == 0        &&
       g_MPEGCtx.m_VolCtlPar == 0
  ) g_MPEGCtx.m_DivXVer = 400;

 if ( g_MPEGCtx.m_Bugs & SMS_BUG_AUTODETECT ) {

  g_MPEGCtx.m_Bugs &= ~SMS_BUG_NO_PADDING;
        
  if (  g_MPEGCtx.m_PaddingBugScore > -2 &&
       !g_MPEGCtx.m_DataPartitioning     &&
        ( g_MPEGCtx.m_DivXVer || !g_MPEGCtx.m_ResyncMarker )
  ) g_MPEGCtx.m_Bugs |=  SMS_BUG_NO_PADDING;

  if (  apCtx -> m_Tag == SMS_FourCC_XVIX  ) g_MPEGCtx.m_Bugs |= SMS_BUG_XVID_ILACE;
  if (  apCtx -> m_Tag == SMS_FourCC_UMP4  ) g_MPEGCtx.m_Bugs |= SMS_BUG_UMP4;

  if (  g_MPEGCtx.m_DivXVer >= 500 ) g_MPEGCtx.m_Bugs |= SMS_BUG_QPEL_CHROMA;
  if (  g_MPEGCtx.m_DivXVer  > 502 ) g_MPEGCtx.m_Bugs |= SMS_BUG_QPEL_CHROMA2;

  if ( g_MPEGCtx.m_XviDBuild && g_MPEGCtx.m_XviDBuild <= 3 ) g_MPEGCtx.m_PaddingBugScore = 256 * 256 * 256 * 64;
  if ( g_MPEGCtx.m_XviDBuild && g_MPEGCtx.m_XviDBuild <= 1 ) g_MPEGCtx.m_Bugs           |= SMS_BUG_QPEL_CHROMA;
  if ( g_MPEGCtx.m_XviDBuild && g_MPEGCtx.m_XviDBuild <=12 ) g_MPEGCtx.m_Bugs           |= SMS_BUG_EDGE;
  if ( g_MPEGCtx.m_XviDBuild && g_MPEGCtx.m_XviDBuild <=32 ) g_MPEGCtx.m_Bugs           |= SMS_BUG_DC_CLIP;

  if ( g_MPEGCtx.m_DivXVer                                             ) g_MPEGCtx.m_Bugs           |= SMS_BUG_DIRECT_BLOCKSIZE;
  if ( g_MPEGCtx.m_DivXVer == 501 && g_MPEGCtx.m_DivXBuild == 20020416 ) g_MPEGCtx.m_PaddingBugScore = 256 * 256 * 256 * 64;
  if ( g_MPEGCtx.m_DivXVer && g_MPEGCtx.m_DivXVer < 500                ) g_MPEGCtx.m_Bugs           |= SMS_BUG_EDGE;
  if ( g_MPEGCtx.m_DivXVer                                             ) g_MPEGCtx.m_Bugs           |= SMS_BUG_HPEL_CHROMA;

 }  /* end if */

 g_MPEGCtx.m_CurPic.m_Type     = g_MPEGCtx.m_PicType;
 g_MPEGCtx.m_CurPic.m_KeyFrame = g_MPEGCtx.m_PicType == SMS_FT_I_TYPE;

 if (  g_MPEGCtx.m_pLastPic == NULL && ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE || g_MPEGCtx.m_Dropable )  ) return 0;
 if (  apCtx -> m_HurryUp && g_MPEGCtx.m_PicType == SMS_FT_B_TYPE                                        ) return 0;
 if (  apCtx -> m_HurryUp >= 5                                                                           ) return 0;

 if ( g_MPEGCtx.m_NextPFrameDamaged ) {

  if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE )

   return 0;

  else g_MPEGCtx.m_NextPFrameDamaged = 0;

 }  /* end if */

 if (  SMS_MPEG_FrameStart () < 0  ) return 0;

 g_MPEGCtx.m_MBX = 0; 
 g_MPEGCtx.m_MBY = 0;

 _decode_slice ();

 while ( g_MPEGCtx.m_MBY < g_MPEGCtx.m_MBH ) {

  if (  _mpeg4_resync () < 0  ) break;

  _decode_slice ();

 }  /* end while */

 if ( g_MPEGCtx.m_BSBufSize == 0 && g_MPEGCtx.m_DivXPack ) {

  int lCurPos         = SMS_BitCount ( lpBitCtx ) >> 3;
  int lStartCodeFound = 0;

  if ( aBufSize - lCurPos > 5 && aBufSize - lCurPos < BITSTREAM_BUFFER_SIZE ) {

   int i;

   for ( i = lCurPos; i < aBufSize - 3; ++i ) {

    if (  apBuf[ i     ] == 0 &&
          apBuf[ i + 1 ] == 0 &&
          apBuf[ i + 2 ] == 1 &&
          apBuf[ i + 3 ] == 0xB6
    ) {

     lStartCodeFound = 1;
     break;

    }  /* end if */

   }  /* end for */

  }  /* end if */

  if ( lpBitCtx -> m_pBuf == g_MPEGCtx.m_pBSBuf && aBufSize > 20 ) {

   lStartCodeFound = 1;
   lCurPos         = 0;

  }  /* end if */

  if ( lStartCodeFound ) {

   g_MPEGCtx.m_pBSBuf = realloc ( g_MPEGCtx.m_pBSBuf, aBufSize - lCurPos + 8 );
   memcpy ( g_MPEGCtx.m_pBSBuf, apBuf + lCurPos, aBufSize - lCurPos );
   g_MPEGCtx.m_BSBufSize = aBufSize - lCurPos;

  }  /* end if */

 }  /* end if */

 SMS_MPEG_FrameEnd ();

 if ( g_MPEGCtx.m_PicType == SMS_FT_B_TYPE || g_MPEGCtx.m_LowDelay ) {

  *lpFrame = g_MPEGCtx.m_CurPic.m_pBuf;

 } else {

  *lpFrame = g_MPEGCtx.m_LastPic.m_pBuf;

 }  /* end else */

 apCtx -> m_FrameNr = g_MPEGCtx.m_PicNr - 1;

 if (  ( retVal = g_MPEGCtx.m_pLastPic || g_MPEGCtx.m_LowDelay )  )

  ( *lpFrame ) -> m_FrameType = g_MPEGCtx.m_PicType;

 return retVal;

}  /* end MPEG4_Decode */
