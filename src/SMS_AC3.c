/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001, 2002 Fabrice Bellard.
# Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
# Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
#
# This file WAS part of a52dec, a free ATSC A-52 stream decoder.
# See http://liba52.sourceforge.net/ for updates.
#
# Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_AC3.h"

#include <string.h>
#include <malloc.h>

typedef struct quantizer_set_t {

 quantizer_t m_Q1[ 2 ];
 quantizer_t m_Q2[ 2 ];
 quantizer_t m_Q4;
 int         m_pQ1;
 int         m_pQ2;
 int         m_pQ4;

} quantizer_set_t;

#define Q( x ) SMS_ROUND ( 32768.0 * x )

#define Q0 Q( -2 / 3 )
#define Q1 Q( 0 )
#define Q2 Q( 2 / 3 )

static const quantizer_t s_Q10[ 32 ] = {
 Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0,
 Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1,
 Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2,
  0,  0,  0,  0,  0
};

static const quantizer_t s_Q11[ 32 ] = {
 Q0, Q0, Q0, Q1, Q1, Q1, Q2, Q2, Q2,
 Q0, Q0, Q0, Q1, Q1, Q1, Q2, Q2, Q2,
 Q0, Q0, Q0, Q1, Q1, Q1, Q2, Q2, Q2,
  0,  0,  0,  0,  0
};

static const quantizer_t s_Q12[ 32 ] = {
 Q0, Q1, Q2, Q0, Q1, Q2, Q0, Q1, Q2,
 Q0, Q1, Q2, Q0, Q1, Q2, Q0, Q1, Q2,
 Q0, Q1, Q2, Q0, Q1, Q2, Q0, Q1, Q2,
  0,  0,  0,  0,  0
};

#undef Q0
#undef Q1
#undef Q2

#define Q0 Q( -4 / 5 )
#define Q1 Q( -2 / 5 )
#define Q2 Q( 0 )
#define Q3 Q( 2 / 5 )
#define Q4 Q( 4 / 5 )

static const quantizer_t s_Q20[ 128 ] = {
 Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0,
 Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1,
 Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2,
 Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3,
 Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4,
 0,   0,  0
};

static const quantizer_t s_Q21[ 128 ] = {
 Q0, Q0, Q0, Q0, Q0, Q1, Q1, Q1, Q1, Q1, Q2, Q2, Q2, Q2, Q2, Q3, Q3, Q3, Q3, Q3, Q4, Q4, Q4, Q4, Q4,
 Q0, Q0, Q0, Q0, Q0, Q1, Q1, Q1, Q1, Q1, Q2, Q2, Q2, Q2, Q2, Q3, Q3, Q3, Q3, Q3, Q4, Q4, Q4, Q4, Q4,
 Q0, Q0, Q0, Q0, Q0, Q1, Q1, Q1, Q1, Q1, Q2, Q2, Q2, Q2, Q2, Q3, Q3, Q3, Q3, Q3, Q4, Q4, Q4, Q4, Q4,
 Q0, Q0, Q0, Q0, Q0, Q1, Q1, Q1, Q1, Q1, Q2, Q2, Q2, Q2, Q2, Q3, Q3, Q3, Q3, Q3, Q4, Q4, Q4, Q4, Q4,
 Q0, Q0, Q0, Q0, Q0, Q1, Q1, Q1, Q1, Q1, Q2, Q2, Q2, Q2, Q2, Q3, Q3, Q3, Q3, Q3, Q4, Q4, Q4, Q4, Q4,
 0,   0,  0
};

static const quantizer_t s_Q22[ 128 ] = {
 Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4,
 Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4,
 Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4,
 Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4,
 Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4, Q0, Q1, Q2, Q3, Q4,
 0,   0,  0
};

#undef Q0
#undef Q1
#undef Q2
#undef Q3
#undef Q4

static const quantizer_t s_Q3[ 8 ] = {
 Q( -6 / 7 ), Q( -4 / 7 ), Q( -2 / 7 ), Q( 0 ), Q( 2 / 7 ), Q( 4 / 7 ), Q( 6 / 7 ), 0
};

#define Q0 Q( -10 / 11 )
#define Q1 Q(  -8 / 11 )
#define Q2 Q(  -6 / 11 )
#define Q3 Q(  -4 / 11 )
#define Q4 Q(  -2 / 11 )
#define Q5 Q( 0 )
#define Q6 Q(  2 / 11 )
#define Q7 Q(  4 / 11 )
#define Q8 Q(  6 / 11 )
#define Q9 Q(  8 / 11 )
#define QA Q( 10 / 11 )

static const quantizer_t s_Q40[ 128 ] = {
 Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0, Q0,
 Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1, Q1,
 Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2, Q2,
 Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3, Q3,
 Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4, Q4,
 Q5, Q5, Q5, Q5, Q5, Q5, Q5, Q5, Q5, Q5, Q5,
 Q6, Q6, Q6, Q6, Q6, Q6, Q6, Q6, Q6, Q6, Q6,
 Q7, Q7, Q7, Q7, Q7, Q7, Q7, Q7, Q7, Q7, Q7,
 Q8, Q8, Q8, Q8, Q8, Q8, Q8, Q8, Q8, Q8, Q8,
 Q9, Q9, Q9, Q9, Q9, Q9, Q9, Q9, Q9, Q9, Q9,
 QA, QA, QA, QA, QA, QA, QA, QA, QA, QA, QA,
  0,  0,  0,  0,  0,  0,  0
};

static const quantizer_t s_Q41[ 128 ] = {
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
 Q0, Q1, Q2, Q3, Q4, Q5, Q6, Q7, Q8, Q9, QA,
  0,  0,  0,  0,  0,  0,  0
};

#undef Q0
#undef Q1
#undef Q2
#undef Q3
#undef Q4
#undef Q5
#undef Q6
#undef Q7
#undef Q8
#undef Q9
#undef QA

static const quantizer_t s_Q5[ 16 ] = {
 Q( -14 / 15 ), Q( -12 / 15 ), Q( -10 / 15 ), Q( -8 / 15 ), Q( -6 / 15 ),
 Q(  -4 / 15 ), Q(  -2 / 15 ), Q(   0      ), Q(  2 / 15 ), Q(  4 / 15 ),
 Q(   6 / 15 ), Q(   8 / 15 ), Q(  10 / 15 ), Q( 12 / 15 ), Q( 14 / 15 ), 0
};

extern void _ac3_imdct_256 ( sample_t*, sample_t*, sample_t );
extern void _ac3_imdct_512 ( sample_t*, sample_t*, sample_t );

static uint8_t s_HalfRate[ 12 ] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3
};

static const int8_t s_Exp1[ 128 ] = {
 -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
 25, 25, 25
};

static const int8_t s_Exp2[ 128 ] = {
 -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
 -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
 -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
 -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
 -2, -2, -2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
 25, 25, 25
};

static const int8_t s_Exp3[ 128 ] = {
 -2, -1,  0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2,
 -2, -1,  0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2,
 -2, -1,  0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2,
 -2, -1,  0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2,
 -2, -1,  0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2, -2, -1, 0, 1, 2,
 25, 25, 25
};

static const uint16_t s_DitherLUT[ 256 ] = {
 0x0000, 0xA011, 0xE033, 0x4022, 0x6077, 0xC066, 0x8044, 0x2055,
 0xC0EE, 0x60FF, 0x20DD, 0x80CC, 0xA099, 0x0088, 0x40AA, 0xE0BB,
 0x21CD, 0x81DC, 0xC1FE, 0x61EF, 0x41BA, 0xE1AB, 0xA189, 0x0198,
 0xE123, 0x4132, 0x0110, 0xA101, 0x8154, 0x2145, 0x6167, 0xC176,
 0x439A, 0xE38B, 0xA3A9, 0x03B8, 0x23ED, 0x83FC, 0xC3DE, 0x63CF,
 0x8374, 0x2365, 0x6347, 0xC356, 0xE303, 0x4312, 0x0330, 0xA321,
 0x6257, 0xC246, 0x8264, 0x2275, 0x0220, 0xA231, 0xE213, 0x4202,
 0xA2B9, 0x02A8, 0x428A, 0xE29B, 0xC2CE, 0x62DF, 0x22FD, 0x82EC,
 0x8734, 0x2725, 0x6707, 0xC716, 0xE743, 0x4752, 0x0770, 0xA761,
 0x47DA, 0xE7CB, 0xA7E9, 0x07F8, 0x27AD, 0x87BC, 0xC79E, 0x678F,
 0xA6F9, 0x06E8, 0x46CA, 0xE6DB, 0xC68E, 0x669F, 0x26BD, 0x86AC,
 0x6617, 0xC606, 0x8624, 0x2635, 0x0660, 0xA671, 0xE653, 0x4642,
 0xC4AE, 0x64BF, 0x249D, 0x848C, 0xA4D9, 0x04C8, 0x44EA, 0xE4FB,
 0x0440, 0xA451, 0xE473, 0x4462, 0x6437, 0xC426, 0x8404, 0x2415,
 0xE563, 0x4572, 0x0550, 0xA541, 0x8514, 0x2505, 0x6527, 0xC536,
 0x258D, 0x859C, 0xC5BE, 0x65AF, 0x45FA, 0xE5EB, 0xA5C9, 0x05D8,
 0xAE79, 0x0E68, 0x4E4A, 0xEE5B, 0xCE0E, 0x6E1F, 0x2E3D, 0x8E2C,
 0x6E97, 0xCE86, 0x8EA4, 0x2EB5, 0x0EE0, 0xAEF1, 0xEED3, 0x4EC2,
 0x8FB4, 0x2FA5, 0x6F87, 0xCF96, 0xEFC3, 0x4FD2, 0x0FF0, 0xAFE1,
 0x4F5A, 0xEF4B, 0xAF69, 0x0F78, 0x2F2D, 0x8F3C, 0xCF1E, 0x6F0F,
 0xEDE3, 0x4DF2, 0x0DD0, 0xADC1, 0x8D94, 0x2D85, 0x6DA7, 0xCDB6,
 0x2D0D, 0x8D1C, 0xCD3E, 0x6D2F, 0x4D7A, 0xED6B, 0xAD49, 0x0D58,
 0xCC2E, 0x6C3F, 0x2C1D, 0x8C0C, 0xAC59, 0x0C48, 0x4C6A, 0xEC7B,
 0x0CC0, 0xACD1, 0xECF3, 0x4CE2, 0x6CB7, 0xCCA6, 0x8C84, 0x2C95,
 0x294D, 0x895C, 0xC97E, 0x696F, 0x493A, 0xE92B, 0xA909, 0x0918,
 0xE9A3, 0x49B2, 0x0990, 0xA981, 0x89D4, 0x29C5, 0x69E7, 0xC9F6,
 0x0880, 0xA891, 0xE8B3, 0x48A2, 0x68F7, 0xC8E6, 0x88C4, 0x28D5,
 0xC86E, 0x687F, 0x285D, 0x884C, 0xA819, 0x0808, 0x482A, 0xE83B,
 0x6AD7, 0xCAC6, 0x8AE4, 0x2AF5, 0x0AA0, 0xAAB1, 0xEA93, 0x4A82,
 0xAA39, 0x0A28, 0x4A0A, 0xEA1B, 0xCA4E, 0x6A5F, 0x2A7D, 0x8A6C,
 0x4B1A, 0xEB0B, 0xAB29, 0x0B38, 0x2B6D, 0x8B7C, 0xCB5E, 0x6B4F,
 0x8BF4, 0x2BE5, 0x6BC7, 0xCBD6, 0xEB83, 0x4B92, 0x0BB0, 0xABA1
};

static int s_HtHTab[ 3 ][ 50 ] = {
 { 0x730, 0x730, 0x7C0, 0x800, 0x820, 0x840, 0x850, 0x850, 0x860, 0x860,
   0x860, 0x860, 0x860, 0x870, 0x870, 0x870, 0x880, 0x880, 0x890, 0x890,
   0x8A0, 0x8A0, 0x8B0, 0x8B0, 0x8C0, 0x8C0, 0x8D0, 0x8E0, 0x8F0, 0x900,
   0x910, 0x910, 0x910, 0x910, 0x900, 0x8F0, 0x8C0, 0x870, 0x820, 0x7E0,
   0x7A0, 0x770, 0x760, 0x7A0, 0x7C0, 0x7C0, 0x6E0, 0x400, 0x3C0, 0x3C0
 },
 { 0x710, 0x710, 0x7A0, 0x7F0, 0x820, 0x830, 0x840, 0x850, 0x850, 0x860,
   0x860, 0x860, 0x860, 0x860, 0x870, 0x870, 0x870, 0x880, 0x880, 0x880,
   0x890, 0x890, 0x8A0, 0x8A0, 0x8B0, 0x8B0, 0x8C0, 0x8C0, 0x8E0, 0x8F0,
   0x900, 0x910, 0x910, 0x910, 0x910, 0x900, 0x8E0, 0x8B0, 0x870, 0x820,
   0x7E0, 0x7B0, 0x760, 0x770, 0x7A0, 0x7C0, 0x780, 0x5D0, 0x3C0, 0x3C0
 },
 { 0x680, 0x680, 0x750, 0x7B0, 0x7E0, 0x810, 0x820, 0x830, 0x840, 0x850,
   0x850, 0x850, 0x860, 0x860, 0x860, 0x860, 0x860, 0x860, 0x860, 0x860,
   0x870, 0x870, 0x870, 0x870, 0x880, 0x880, 0x880, 0x890, 0x8A0, 0x8B0,
   0x8C0, 0x8D0, 0x8E0, 0x8F0, 0x900, 0x910, 0x910, 0x910, 0x900, 0x8F0,
   0x8D0, 0x8B0, 0x840, 0x7F0, 0x790, 0x760, 0x7A0, 0x7C0, 0x7B0, 0x720
 }
};

static int8_t s_BAPTab[ 305 ] = {
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 14, 14, 14, 14, 14, 14,
 14, 12, 12, 12, 12, 11, 11, 11, 11, 10, 10, 10, 10,  9,  9,  9,
  9,  8,  8,  8,  8,  7,  7,  7,  7,  6,  6,  6,  6,  5,  5,  5,
  5,  4,  4, -3, -3,  3,  3,  3, -2, -2, -1, -1, -1, -1, -1,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0
};

static int s_BndTab[ 30 ] = {
 21, 22,  23,  24,  25,  26,  27,  28,  31,  34,
 37, 40,  43,  46,  49,  55,  61,  67,  73,  79,
 85, 97, 109, 121, 133, 157, 181, 205, 229, 253
};

static int8_t s_LATab[ 256 ] = {
 -64, -63, -62, -61, -60, -59, -58, -57, -56, -55, -54, -53,
 -52, -52, -51, -50, -49, -48, -47, -47, -46, -45, -44, -44,
 -43, -42, -41, -41, -40, -39, -38, -38, -37, -36, -36, -35,
 -35, -34, -33, -33, -32, -32, -31, -30, -30, -29, -29, -28,
 -28, -27, -27, -26, -26, -25, -25, -24, -24, -23, -23, -22,
 -22, -21, -21, -21, -20, -20, -19, -19, -19, -18, -18, -18,
 -17, -17, -17, -16, -16, -16, -15, -15, -15, -14, -14, -14,
 -13, -13, -13, -13, -12, -12, -12, -12, -11, -11, -11, -11,
 -10, -10, -10, -10, -10,  -9,  -9,  -9,  -9,  -9,  -8,  -8,
  -8,  -8,  -8,  -8,  -7,  -7,  -7,  -7,  -7,  -7,  -6,  -6,
  -6,  -6,  -6,  -6,  -6,  -6,  -5,  -5,  -5,  -5,  -5,  -5,
  -5,  -5,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
  -4,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
  -3,  -3,  -3,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0
};

static SMS_Codec_AC3Context s_AC3Ctx;

#define MYCTX() (  ( SMS_Codec_AC3Context* )apCtx -> m_pCodec -> m_pCtx  )

static int32_t AC3_Init    ( SMS_CodecContext*                            );
static int32_t AC3_Decode  ( SMS_CodecContext*, void**, uint8_t*, int32_t );
static void    AC3_Destroy ( SMS_CodecContext*                            );

void SMS_Codec_AC3_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "ac3";
 apCtx -> m_pCodec -> m_pCtx  = &s_AC3Ctx;
 apCtx -> m_pCodec -> Init    = AC3_Init;
 apCtx -> m_pCodec -> Decode  = AC3_Decode;
 apCtx -> m_pCodec -> Destroy = AC3_Destroy;

}  /* end SMS_Codec_AC3_Open */

static int32_t AC3_Init ( SMS_CodecContext* apCtx ) {

 s_AC3Ctx.m_pInBuf    = s_AC3Ctx.m_InBuf;
 s_AC3Ctx.m_FrameSize = 0;

 MYCTX() -> m_pOutBuffer = SMS_InitAudioBuffer ();
 MYCTX() -> m_pSamples   = ( sample_t* )calloc (  16, 256 * 12 * sizeof ( sample_t )  );
 MYCTX() -> m_Downmixed  = 1;
 MYCTX() -> m_LFSRState  = 1;

 return 0;

}  /* end AC3_Init */

static void AC3_Destroy ( SMS_CodecContext* apCtx ) {

 MYCTX() -> m_pOutBuffer -> Destroy ();
 free (  MYCTX() -> m_pSamples  );

}  /* end AC3_Destroy */

static int _ac3_syncinfo ( uint8_t* apBuf, int* apFlags, int* apSampleRate, int* apBitRate ) {

 static int s_Rate[ 19 ] = {
   32,  40,  48,  56,  64,  80,  96, 112,
  128, 160, 192, 224, 256, 320, 384, 448,
  512, 576, 640
 };
 static uint8_t s_LFEOn[ 8 ] = {
  0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01
 };

 int lFrmSizeCod;
 int lBitRate;
 int lHalf;
 int lACMod;

 if ( apBuf[ 0 ] != 0x0B || apBuf[ 1 ] != 0x77 || apBuf[ 5 ] >= 0x60 ) return 0;

 lHalf  = s_HalfRate[  apBuf[ 5 ] >> 3  ];
 lACMod = apBuf[ 6 ] >> 5;

 *apFlags = (    (   (  ( apBuf[ 6 ] & 0xF8 ) == 0x50  ) ? AC3_DOLBY : lACMod   ) |
	             (  ( apBuf[ 6 ] & s_LFEOn[ lACMod ] ) ? AC3_LFE : 0  )
            );
 
 if (  ( lFrmSizeCod = apBuf[ 4 ] & 63 ) >= 38  ) return 0;

 lBitRate   = s_Rate[ lFrmSizeCod >> 1 ];
 *apBitRate = ( lBitRate * 1000) >> lHalf;

 switch ( apBuf[ 4 ] & 0xC0 ) {

  case 0x00: *apSampleRate = 48000 >> lHalf; return 4 * lBitRate;
  case 0x40: *apSampleRate = 44100 >> lHalf; return 2 * (  320 * lBitRate / 147 + ( lFrmSizeCod & 1 )  );
  case 0x80: *apSampleRate = 32000 >> lHalf; return 6 * lBitRate;

 }  /* end switch */

 return 0;

}  /* end _ac3_syncinfo */

static int _ac3_downmix_init (
            int anInput, int aFlags, level_t* apLevel, level_t aCLev, level_t aSLev
           ) {

 static uint8_t s_Table[ 11 ][ 8 ] = {
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO },
  {     AC3_MONO,  AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO, AC3_STEREO },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO,     AC3_3F, AC3_STEREO,     AC3_3F, AC3_STEREO,     AC3_3F },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO, AC3_STEREO,   AC3_2F1R,   AC3_2F1R,   AC3_2F1R,   AC3_2F1R },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO, AC3_STEREO,   AC3_2F1R,   AC3_3F1R,   AC3_2F1R,   AC3_3F1R },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO,     AC3_3F,   AC3_2F2R,   AC3_2F2R,   AC3_2F2R,   AC3_2F2R },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO,     AC3_3F,   AC3_2F2R,   AC3_3F2R,   AC3_2F2R,   AC3_3F2R },
  { AC3_CHANNEL1,  AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO },
  { AC3_CHANNEL2,  AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO,   AC3_MONO },
  {  AC3_CHANNEL, AC3_DOLBY, AC3_STEREO,  AC3_DOLBY,  AC3_DOLBY,  AC3_DOLBY,   AC3_DOLBY, AC3_DOLBY }
 };

 int lOutput = aFlags & AC3_CHANNEL_MASK;

 if ( lOutput > AC3_DOLBY ) return -1;

 lOutput = s_Table[ lOutput ][ anInput & 7 ];

 if (    lOutput == AC3_STEREO && (   anInput == AC3_DOLBY || (  anInput == AC3_3F && aCLev == LEVEL( LEVEL_3DB )  )   )    ) lOutput = AC3_DOLBY;

 if ( aFlags & AC3_ADJUST_LEVEL ) {

  level_t lAdjust;

  switch (  CONVERT( anInput & 7, lOutput )  ) {

   case CONVERT( AC3_3F, AC3_MONO ):
    lAdjust = ( level_t )DIV(  LEVEL_3DB, LEVEL( 1 ) + aCLev  );
   break;

   case CONVERT( AC3_STEREO, AC3_MONO ):
   case CONVERT(   AC3_2F2R, AC3_2F1R ):
   case CONVERT(   AC3_3F2R, AC3_3F1R ):
level_3db:
    lAdjust = LEVEL( LEVEL_3DB );
   break;

   case CONVERT( AC3_3F2R, AC3_2F1R ):

    if (  aCLev < LEVEL( LEVEL_PLUS3DB - 1 )  ) goto level_3db;

   case CONVERT(   AC3_3F, AC3_STEREO ):
   case CONVERT( AC3_3F1R,   AC3_2F1R ):
   case CONVERT( AC3_3F1R,   AC3_2F2R ):
   case CONVERT( AC3_3F2R,   AC3_2F2R ):
    lAdjust = ( level_t )DIV(  1, LEVEL( 1 ) + aCLev  );
   break;

   case CONVERT( AC3_2F1R, AC3_MONO ):
    lAdjust = ( level_t )DIV(  LEVEL_PLUS3DB, LEVEL( 2 ) + aSLev  );
   break;

   case CONVERT( AC3_2F1R, AC3_STEREO ):
   case CONVERT( AC3_3F1R,     AC3_3F ):
    lAdjust = ( level_t )DIV(  1, LEVEL( 1 ) + MUL_C ( aSLev, LEVEL_3DB )  );
   break;

   case CONVERT( AC3_3F1R, AC3_MONO ):
    lAdjust = ( level_t )DIV(  LEVEL_3DB, LEVEL( 1 ) + aCLev + MUL_C( aSLev, 0.5F )  );
   break;

   case CONVERT( AC3_3F1R, AC3_STEREO ):
    lAdjust = ( level_t )DIV(  1, LEVEL( 1 ) + aCLev + MUL_C( aSLev, LEVEL_3DB )  );
   break;

   case CONVERT( AC3_2F2R, AC3_MONO ):
    lAdjust = ( level_t )DIV(  LEVEL_3DB, LEVEL( 1 ) + aSLev  );
   break;

   case CONVERT( AC3_2F2R, AC3_STEREO ):
   case CONVERT( AC3_3F2R,     AC3_3F ):
    lAdjust = ( level_t )DIV(  1, LEVEL( 1 ) + aSLev  );
   break;

   case CONVERT( AC3_3F2R, AC3_MONO ):
    lAdjust = ( level_t )DIV(  LEVEL_3DB, LEVEL( 1 ) + aCLev + aSLev  );
   break;

   case CONVERT( AC3_3F2R, AC3_STEREO ):
    lAdjust = ( level_t )DIV(  1, LEVEL( 1 ) + aCLev + aSLev );
   break;

   case CONVERT( AC3_MONO, AC3_DOLBY ):
    lAdjust = LEVEL( LEVEL_PLUS3DB );
   break;

   case CONVERT(   AC3_3F, AC3_DOLBY ):
   case CONVERT( AC3_2F1R, AC3_DOLBY ):
    lAdjust = LEVEL(  1 / ( 1 + LEVEL_3DB )  );
   break;

   case CONVERT( AC3_3F1R, AC3_DOLBY ):
   case CONVERT( AC3_2F2R, AC3_DOLBY ):
    lAdjust = LEVEL(  1 / ( 1 + 2 * LEVEL_3DB )  );
   break;

   case CONVERT( AC3_3F2R, AC3_DOLBY ):
    lAdjust = LEVEL(  1 / ( 1 + 3 * LEVEL_3DB )  );
   break;

   default: return lOutput;

  }  /* end switch */

  *apLevel = MUL_L( *apLevel, lAdjust );

 }  /* end if */

 return lOutput;

}  /* end _ac3_downmix_init */

static void _ac3_bitstream_set_ptr ( uint8_t* apBuf ) {

 uint32_t lAlign = ( uint32_t )apBuf & 3;

 SMS_InitGetBits ( &s_AC3Ctx.m_BitCtx, apBuf - lAlign, 0xFFFF );
 SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, lAlign * 8 );

}  /* end _ac3_bitstream_set_ptr */

static int _ac3_frame ( uint8_t* apBuf, int* apFlags, level_t* apLevel, sample_t aBias ) {

 static level_t s_CLev[ 4 ] = {
  LEVEL( LEVEL_3DB ), LEVEL( LEVEL_45DB ),
  LEVEL( LEVEL_6DB ), LEVEL( LEVEL_45DB )
 };
 static level_t s_SLev[ 4 ] = {
  LEVEL( LEVEL_3DB ), LEVEL( LEVEL_6DB ),
                   0, LEVEL( LEVEL_6DB )
 };

 int lChanInfo;
 int lACMod;

 s_AC3Ctx.m_FSCod    = apBuf[ 4 ] >> 6;
 s_AC3Ctx.m_HalfRate = s_HalfRate[  apBuf[ 5 ] >> 3  ];
 s_AC3Ctx.m_ACMmod   = lACMod = apBuf[ 6 ] >> 5;

 _ac3_bitstream_set_ptr ( apBuf + 6 );

 SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 3 );

 if (   ( lACMod == 2 ) && (  SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 ) == 2  )   ) lACMod = AC3_DOLBY;

 s_AC3Ctx.m_CLev = s_AC3Ctx.m_SLev = 0;

 if (  ( lACMod & 1 ) && ( lACMod != 1 )  ) s_AC3Ctx.m_CLev = s_CLev[  SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 ) ];

 if ( lACMod & 4 ) s_AC3Ctx.m_SLev = s_SLev[ SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 ) ];

 s_AC3Ctx.m_LFEOn  = SMS_GetBit ( &s_AC3Ctx.m_BitCtx );
 s_AC3Ctx.m_Output = _ac3_downmix_init ( lACMod, *apFlags, apLevel, s_AC3Ctx.m_CLev, s_AC3Ctx.m_SLev );

 if ( s_AC3Ctx.m_Output < 0 ) return 1;

 if ( s_AC3Ctx.m_LFEOn && ( *apFlags & AC3_LFE )  ) s_AC3Ctx.m_Output |= AC3_LFE;

 *apFlags = s_AC3Ctx.m_Output;

 s_AC3Ctx.m_DynRng  = s_AC3Ctx.m_Level = MUL_C( *apLevel, 2 );
 s_AC3Ctx.m_Bias    = aBias;
 s_AC3Ctx.m_DynRnge = 1;
 s_AC3Ctx.m_CplBA.m_DeltBAE   = DELTA_BIT_NONE;
 s_AC3Ctx.m_BA[ 0 ].m_DeltBAE =
 s_AC3Ctx.m_BA[ 1 ].m_DeltBAE =
 s_AC3Ctx.m_BA[ 2 ].m_DeltBAE =
 s_AC3Ctx.m_BA[ 3 ].m_DeltBAE =
 s_AC3Ctx.m_BA[ 4 ].m_DeltBAE = DELTA_BIT_NONE;

 lChanInfo = !lACMod;

 do {

  SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 5 );

  if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 8 );
  if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 8 );
  if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 7 );

 } while ( lChanInfo-- );

 SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 2 );

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 14 );
 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 14 );

 if (  SMS_GetBit  ( &s_AC3Ctx.m_BitCtx )  ) {

  int lAddBSIL = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 6 );

  do

   SMS_SkipBits ( &s_AC3Ctx.m_BitCtx, 8 );

  while ( lAddBSIL-- );

 }  /* end if */

 return 0;

}  /* end _ac3_frame */

static int _ac3_parse_exponents ( int anExpStr, int aNGrps, uint8_t anExp, uint8_t* apDest ) {

 int lExps;

 while ( aNGrps-- ) {

  lExps = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

  anExp += s_Exp1[ lExps ];

  if ( anExp > 24 ) return 1;

  switch ( anExpStr ) {

   case EXP_D45:

    *( apDest++ ) = anExp;
    *( apDest++ ) = anExp;

   case EXP_D25:

    *( apDest++ ) = anExp;

   case EXP_D15:

    *( apDest++ ) = anExp;

  }  /* end switch */

  anExp += s_Exp2[ lExps ];

  if ( anExp > 24 ) return 1;

  switch ( anExpStr ) {

   case EXP_D45:

    *( apDest++ ) = anExp;
    *( apDest++ ) = anExp;

   case EXP_D25:

    *( apDest++ ) = anExp;

   case EXP_D15:

    *( apDest++ ) = anExp;

  }  /* end switch */

  anExp += s_Exp3[ lExps ];

  if ( anExp > 24 ) return 1;

  switch ( anExpStr ) {

   case EXP_D45:

    *( apDest++ ) = anExp;
    *( apDest++ ) = anExp;

   case EXP_D25:

    *( apDest++ ) = anExp;

   case EXP_D15:

    *( apDest++ ) = anExp;

  }  /* end switch */

 }  /* end while */

 return 0;

}  /* end _ac3_parse_exponents */

static int _ac3_parse_deltba ( int8_t* apDeltBA ) {

 int j, lDeltNSeg, lDeltLen, lDelta;

 memset ( apDeltBA, 0, 50 );

 lDeltNSeg = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 3 );

 j = 0;

 do {

  j       += SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 5 );
  lDeltLen = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );
  lDelta   = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 3 );
  lDelta  -= lDelta >= 4 ? 3 : 4;

  if ( !lDeltLen ) continue;

  if ( j + lDeltLen >= 50 ) return 1;

  while ( lDeltLen-- ) apDeltBA[ j++ ] = lDelta;

 } while ( lDeltNSeg-- );

 return 0;

}  /* end _ac3_parse_deltba */

static SMS_INLINE int _ac3_zero_snr_offsets ( int aNFChans ) {

 int i;

 if ( ( s_AC3Ctx.m_CSNROffst                              ) ||
      ( s_AC3Ctx.m_ChInCpl && s_AC3Ctx.m_CplBA.m_BAI >> 3 ) ||
      ( s_AC3Ctx.m_LFEOn   && s_AC3Ctx.m_LFEBA.m_BAI >> 3 )
 ) return 0;

 for ( i = 0; i < aNFChans; ++i )

  if ( s_AC3Ctx.m_BA[ i ].m_BAI >> 3 ) return 0;

 return 1;

}  /* end _ac3_zero_snr_offsets */

static SMS_INLINE int16_t _ac3_dither_gen ( void ) {

 int16_t lnState = s_DitherLUT[ s_AC3Ctx.m_LFSRState >> 8 ] ^ ( s_AC3Ctx.m_LFSRState << 8 );
	
 s_AC3Ctx.m_LFSRState = ( uint16_t )lnState;

 return ( 3 * lnState ) >> 2;

}  /* end _ac3_dither_gen */

static void _ac3_coeff_get (
             sample_t* apCoeff, SMS_ExpBAP* apExpBAP, quantizer_set_t* apQuant,
		     level_t   aLevel,  int         aDither,  int              anEnd
            ) {

 int      i;
 uint8_t* lpExp = apExpBAP -> m_Exp;
 int8_t*  lpBAP = apExpBAP -> m_BAP;

 for ( i = 0; i < anEnd; ++i ) {

  int lBAPI = lpBAP[ i ];

  switch ( lBAPI ) {

   case 0:

    if ( aDither ) {

     COEFF( apCoeff[ i ], _ac3_dither_gen (), aLevel, lpExp[ i ] );
     continue;

    } else {

     apCoeff[ i ] = 0;
     continue;

    }  /* end else */

   case -1:

    if ( apQuant -> m_pQ1 >= 0 ) {

     COEFF ( apCoeff[ i ], apQuant -> m_Q1[ apQuant -> m_pQ1-- ], aLevel, lpExp[ i ] );
     continue;

    } else {

     int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 5 );

     apQuant -> m_pQ1 = 1;
     apQuant -> m_Q1[ 0 ] = s_Q12[ lCode ];
	 apQuant -> m_Q1[ 1 ] = s_Q11[ lCode ];
     COEFF( apCoeff[ i ], s_Q10[ lCode ], aLevel, lpExp[ i ] );
     continue;

    }  /* end else */

   case -2:

    if ( apQuant -> m_pQ2 >= 0 ) {

     COEFF( apCoeff[ i ], apQuant -> m_Q2[ apQuant -> m_pQ2-- ], aLevel, lpExp[ i ] );
     continue;

    } else {

     int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

     apQuant -> m_pQ2 = 1;
     apQuant -> m_Q2[ 0 ] = s_Q22[ lCode ];
     apQuant -> m_Q2[ 1 ] = s_Q21[ lCode ];
     COEFF( apCoeff[ i ], s_Q20[ lCode ], aLevel, lpExp[ i ] );
     continue;

    }  /* end else */

   case 3:

    COEFF( apCoeff[ i ], s_Q3[ SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 3 ) ], aLevel, lpExp[ i ] );
    continue;

   case -3:

    if ( apQuant -> m_pQ4 == 0 ) {

     apQuant -> m_pQ4 = -1;
     COEFF( apCoeff[ i ], apQuant -> m_Q4, aLevel, lpExp[ i ] );
     continue;

    } else {

     int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

     apQuant -> m_pQ4 = 0;
     apQuant -> m_Q4  = s_Q41[ lCode ];
     COEFF( apCoeff[ i ], s_Q40[ lCode ], aLevel, lpExp[ i ] );
     continue;

    }  /* end else */

   case 4:

    COEFF( apCoeff[ i ], s_Q5[ SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 ) ], aLevel, lpExp[ i ] );
    continue;

   default:

    COEFF( apCoeff[ i ], SMS_GetSBits ( &s_AC3Ctx.m_BitCtx, lBAPI ) << ( 16 - lBAPI ), aLevel, lpExp[ i ] );

  }  /* end switch */

 }  /* end for */

}  /* end _ac3_coeff_get */

static void _ac3_coeff_get_coupling (
             int anFChans, level_t* apCoeff,
             sample_t ( *apSamples )[ 256 ],
             quantizer_set_t* apQuant, uint8_t aDithFlag[ 5 ]
            ) {

 level_t  lCPLCo[ 5 ];
 int      lCplBndStrc, lBnd, i, i_end, lCh;
 uint8_t* lpExp = s_AC3Ctx.m_CplExpBAP.m_Exp;
 int8_t*  lpBAP = s_AC3Ctx.m_CplExpBAP.m_BAP;

 lBnd        = 0;
 lCplBndStrc = s_AC3Ctx.m_CplBndStrc;
 i           = s_AC3Ctx.m_CplStrtMant;

 while ( i < s_AC3Ctx.m_CplEndMant ) {

  i_end = i + 12;

  while ( lCplBndStrc & 1 ) {

   lCplBndStrc >>=  1;
   i_end        += 12;

  }  /* end while */

  lCplBndStrc >>= 1;

  for ( lCh = 0; lCh < anFChans; ++lCh ) lCPLCo[ lCh ] = MUL_L( s_AC3Ctx.m_CplCo[ lCh ][ lBnd ], apCoeff[ lCh ] );

  ++lBnd;

  while ( i < i_end ) {

   quantizer_t lCplCoeff;
   int         lBAPI = lpBAP[ i ];

   switch ( lBAPI ) {

    case 0:

     for ( lCh = 0; lCh < anFChans; ++lCh )

      if (  ( s_AC3Ctx.m_ChInCpl >> lCh ) & 1  ) {

       if ( aDithFlag[ lCh ] ) {

        COEFF( apSamples[ lCh ][ i ], _ac3_dither_gen (), lCPLCo[ lCh ], lpExp[ i ] );

       } else apSamples[ lCh ][ i ] = 0;

      }  /* end if */

     ++i; continue;

    case -1:

     if ( apQuant -> m_pQ1 >= 0 ) {

      lCplCoeff = apQuant -> m_Q1[ apQuant -> m_pQ1-- ];
      break;

     } else {

      int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 5 );

      apQuant -> m_pQ1 = 1;
	  apQuant -> m_Q1[ 0 ] = s_Q12[ lCode ];
	  apQuant -> m_Q1[ 1 ] = s_Q11[ lCode ];
	  lCplCoeff = s_Q10[ lCode ];
	  break;

     }  /* end else */

    case -2:

     if ( apQuant -> m_pQ2 >= 0 ) {

      lCplCoeff = apQuant -> m_Q2[ apQuant -> m_pQ2-- ];
      break;

     } else {

      int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

      apQuant -> m_pQ2 = 1;
      apQuant -> m_Q2[ 0 ] = s_Q22[ lCode ];
      apQuant -> m_Q2[ 1 ] = s_Q21[ lCode ];
      lCplCoeff = s_Q20[ lCode ];
      break;

     }  /* end else */

    case 3:

     lCplCoeff = s_Q3[ SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 3 ) ];

    break;

    case -3:

     if ( apQuant -> m_pQ4 == 0 ) {

      apQuant -> m_pQ4 = -1;
      lCplCoeff = apQuant -> m_Q4;
      break;

     } else {

      int lCode = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

      apQuant -> m_pQ4 = 0;
      apQuant -> m_Q4  = s_Q41[ lCode ];
	  lCplCoeff = s_Q40[ lCode ];
      break;

     }  /* end else */

    case 4:

     lCplCoeff = s_Q5[ SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 ) ];

    break;

    default: lCplCoeff = SMS_GetSBits ( &s_AC3Ctx.m_BitCtx, lBAPI ) << ( 16 - lBAPI );

   }  /* end switch */

   for ( lCh = 0; lCh < anFChans; ++lCh )

    if (  ( s_AC3Ctx.m_ChInCpl >> lCh ) & 1  ) COEFF( apSamples[ lCh ][ i ], lCplCoeff, lCPLCo[ lCh ], lpExp[ i ] );

   ++i;

  }  /* end while */

 }  /* end while */

}  /* end _ac3_coeff_get_coupling */

void _ac3_bit_allocate (
      SMS_BA* apBA, int aBndStart,
      int aStart, int anEnd, int aFastLeak, int aSlowLeak,
      SMS_ExpBAP* apExpBAP
     ) {

 static int s_SlowGain[ 4 ] = { 0x540, 0x4D8, 0x478, 0x410 };
 static int s_DbPbTab [ 4 ] = { 0xC00, 0x500, 0x300, 0x100 };
 static int s_FloorTab[ 8 ] = { 0x910, 0x950, 0x990, 0x9D0, 0xA10, 0xA90, 0xB10, 0x1400 };

 int      i, j;
 uint8_t* lpExp;
 int8_t*  lpBAP;
 int      lFDecay, lFGain, lSDecay, lSGain, lDbKnee, lFloor, lSnrOffset;
 int      lPSD, lMask;
 int8_t*  lpDeltBA;
 int*     lpHtH;
 int      lHalfRate;

 lHalfRate  = s_AC3Ctx.m_HalfRate;
 lFDecay    = (   63 + 20 * (  ( s_AC3Ctx.m_BAI >> 7 ) & 3  )   ) >> lHalfRate;
 lFGain     = 128 + 128 * ( apBA -> m_BAI & 7 );
 lSDecay    = (  15 + 2 * ( s_AC3Ctx.m_BAI >> 9 )  ) >> lHalfRate;
 lSGain     = s_SlowGain[ ( s_AC3Ctx.m_BAI >> 5 ) & 3 ];
 lDbKnee    = s_DbPbTab [ ( s_AC3Ctx.m_BAI >> 3 ) & 3 ];
 lpHtH      = s_HtHTab[ s_AC3Ctx.m_FSCod ];
 lpDeltBA   = apBA -> m_DeltBAE == DELTA_BIT_NONE ? s_BAPTab + 156 : apBA -> m_DeltBA;
 lFloor     = s_FloorTab[ s_AC3Ctx.m_BAI & 7 ];
 lSnrOffset = 960 - 64 * s_AC3Ctx.m_CSNROffst - 4 * ( apBA -> m_BAI >> 3 ) + lFloor;
 lFloor   >>= 5;

 lpExp = apExpBAP -> m_Exp;
 lpBAP = apExpBAP -> m_BAP;

 i = aBndStart;
 j = aStart;

 if ( !aStart ) {

  int lLowComp = 0;

  j = anEnd - 1;

  do {

   if ( i < j ) {

    if ( lpExp[ i + 1 ] == lpExp[ i ] - 2 )

     lLowComp = 384;

    else if (  lLowComp && ( lpExp[ i + 1 ] > lpExp[ i ] )  ) lLowComp -= 64;

   }  /* end if */

   lPSD  = 128 * lpExp[ i ];
   lMask = lPSD + lFGain + lLowComp;

   COMPUTE_MASK();

   lpBAP[ i ] = ( s_BAPTab + 156 )[  lMask + 4 * lpExp[ i ]  ];
   ++i;

  } while (   ( i < 3 ) || (  ( i < 7 ) && ( lpExp[ i ] > lpExp[ i - 1 ] )  )   );

  aFastLeak = lPSD + lFGain;
  aSlowLeak = lPSD + lSGain;

  while ( i < 7 ) {

   if ( i < j ) {

    if ( lpExp[ i + 1 ] == lpExp[ i ] - 2 )

     lLowComp = 384;

    else if (  lLowComp && ( lpExp[ i + 1 ] > lpExp[ i ] )  ) lLowComp -= 64;

   }  /* end if */

   lPSD = 128 * lpExp[ i ];

   UPDATE_LEAK();

   lMask = ( aFastLeak + lLowComp < aSlowLeak ) ? aFastLeak + lLowComp : aSlowLeak;

   COMPUTE_MASK();

   lpBAP[ i ] = ( s_BAPTab + 156 )[  lMask + 4 * lpExp[ i ]  ];
   ++i;

  }  /* end while */

  if ( anEnd == 7 ) return;

  do {

   if ( lpExp[ i + 1 ] == lpExp[ i ] - 2 )

    lLowComp = 320;

   else if (  lLowComp && ( lpExp[ i + 1 ] > lpExp[ i ] )  ) lLowComp -= 64;

   lPSD = 128 * lpExp[ i ];

   UPDATE_LEAK();

   lMask = ( aFastLeak + lLowComp < aSlowLeak ) ? aFastLeak + lLowComp : aSlowLeak;

   COMPUTE_MASK();

   lpBAP[ i ] = ( s_BAPTab + 156 )[  lMask + 4 * lpExp[ i ]  ];
   ++i;

  } while ( i < 20 );

  while ( lLowComp > 128 ) {

   lLowComp -= 128;
   lPSD      = 128 * lpExp[ i ];

   UPDATE_LEAK();

   lMask = ( aFastLeak + lLowComp < aSlowLeak ) ? aFastLeak + lLowComp : aSlowLeak;

   COMPUTE_MASK();

   lpBAP[ i ] = ( s_BAPTab + 156 )[  lMask + 4 * lpExp[ i ]  ];
   ++i;

  }  /* end while */

  j = i;

 }  /* end if */

 do {

  int lStartBand, lEndBand;

  lStartBand = j;
  lEndBand   = s_BndTab[ i - 20 ] < anEnd ? s_BndTab[ i - 20 ] : anEnd;
  lPSD       = 128 * lpExp[ j++ ];

  while ( j < lEndBand ) {

   int lNext  = 128 * lpExp[ j++ ];
   int lDelta = lNext - lPSD;

   switch ( lDelta >> 9 ) {

    case -6:
    case -5:
    case -4:
    case -3:
    case -2:

     lPSD = lNext;

    break;

    case -1:

     lPSD = lNext + s_LATab[ ( -lDelta ) >> 1 ];

    break;

    case 0:

     lPSD += s_LATab[ lDelta >> 1 ];

    break;

   }  /* end switch */

  }  /* end while */

  UPDATE_LEAK();

  lMask = aFastLeak < aSlowLeak ? aFastLeak : aSlowLeak;

  COMPUTE_MASK();

  ++i;
  j = lStartBand;

  do {

   lpBAP[ j ] = ( s_BAPTab + 156 )[  lMask + 4 * lpExp[ j ]  ];

  } while ( ++j < lEndBand );

 } while ( j < anEnd );

}  /* end _ac3_bit_allocate */

int _ac3_downmix_coeff (
     level_t* apCoeff, int    anACMod,
     int     anOutput, level_t aLevel,
     level_t    aCLev, level_t aSLev
    ) {

 level_t lLevel3DB = MUL_C( aLevel, LEVEL_3DB );

 switch (  CONVERT( anACMod, anOutput & AC3_CHANNEL_MASK )  ) {

  case CONVERT(AC3_CHANNEL, AC3_CHANNEL ):
  case CONVERT(AC3_MONO,    AC3_MONO    ):
  case CONVERT(AC3_STEREO,  AC3_STEREO  ):
  case CONVERT(AC3_3F,      AC3_3F      ):
  case CONVERT(AC3_2F1R,    AC3_2F1R    ):
  case CONVERT(AC3_3F1R,    AC3_3F1R    ):
  case CONVERT(AC3_2F2R,    AC3_2F2R    ):
  case CONVERT(AC3_3F2R,    AC3_3F2R    ):
  case CONVERT(AC3_STEREO,  AC3_DOLBY   ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = apCoeff[ 2 ] = apCoeff[ 3 ] = apCoeff[ 4 ] = aLevel;

  return 0;

  case CONVERT( AC3_CHANNEL, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = MUL_C( aLevel, LEVEL_6DB );

  return 3;

  case CONVERT( AC3_STEREO, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = lLevel3DB;

  return 3;

  case CONVERT( AC3_3F, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = lLevel3DB;
   apCoeff[ 1 ] = MUL_C(  MUL_L ( lLevel3DB, aCLev ), LEVEL_PLUS6DB  );

  return 7;

  case CONVERT( AC3_2F1R, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = lLevel3DB;
   apCoeff[ 2 ] = MUL_L ( lLevel3DB, aSLev );

  return 7;

  case CONVERT( AC3_2F2R, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = lLevel3DB;
   apCoeff[ 2 ] = apCoeff[ 3 ] = MUL_L ( lLevel3DB, aSLev );

  return 15;

  case CONVERT( AC3_3F1R, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = lLevel3DB;
   apCoeff[ 1 ] = MUL_C(  MUL_L ( lLevel3DB, aCLev), LEVEL_PLUS6DB  );
   apCoeff[ 3 ] = MUL_L ( lLevel3DB, aSLev );

  return 15;

  case CONVERT( AC3_3F2R, AC3_MONO ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = lLevel3DB;
   apCoeff[ 1 ] = MUL_C(  MUL_L ( lLevel3DB, aCLev ), LEVEL_PLUS6DB  );
   apCoeff[ 3 ] = apCoeff[ 4 ] = MUL_L ( lLevel3DB, aSLev );

  return 31;

  case CONVERT( AC3_MONO, AC3_DOLBY ):

   apCoeff[ 0 ] = lLevel3DB;

  return 0;

  case CONVERT( AC3_3F, AC3_DOLBY ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = apCoeff[ 3 ] = apCoeff[ 4 ] = aLevel;
   apCoeff[ 1 ] = lLevel3DB;

  return 7;

  case CONVERT( AC3_3F,   AC3_STEREO ):
  case CONVERT( AC3_3F1R, AC3_2F1R   ):
  case CONVERT( AC3_3F2R, AC3_2F2R   ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = apCoeff[ 3 ] = apCoeff[ 4 ] = aLevel;
   apCoeff[ 1 ] = MUL_L ( aLevel, aCLev );

  return 7;

  case CONVERT( AC3_2F1R, AC3_DOLBY ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = aLevel;
   apCoeff[ 2 ] = lLevel3DB;

  return 7;

  case CONVERT( AC3_2F1R, AC3_STEREO ):

   apCoeff[ 0 ] = apCoeff[1] = aLevel;
   apCoeff[ 2 ] = MUL_L ( lLevel3DB, aSLev );

  return 7;

  case CONVERT( AC3_3F1R, AC3_DOLBY ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 1 ] = apCoeff[ 3 ] = lLevel3DB;

  return 15;

  case CONVERT( AC3_3F1R, AC3_STEREO ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 1 ] = MUL_L ( aLevel,    aCLev );
   apCoeff[ 3 ] = MUL_L ( lLevel3DB, aSLev );

  return 15;

  case CONVERT( AC3_2F2R, AC3_DOLBY ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = aLevel;
   apCoeff[ 2 ] = apCoeff[ 3 ] = lLevel3DB;

  return 15;

  case CONVERT( AC3_2F2R, AC3_STEREO ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = aLevel;
   apCoeff[ 2 ] = apCoeff[ 3 ] = MUL_L ( aLevel, aSLev );

  return 15;

  case CONVERT( AC3_3F2R, AC3_DOLBY ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 1 ] = apCoeff[ 3 ] = apCoeff[ 4 ] = lLevel3DB;

  return 31;

  case CONVERT( AC3_3F2R, AC3_2F1R ):

   apCoeff[ 0 ] = apCoeff[ 2] = aLevel;
   apCoeff[ 1 ] = MUL_L ( aLevel, aCLev );
   apCoeff[ 3 ] = apCoeff[ 4 ] = lLevel3DB;

  return 31;

  case CONVERT( AC3_3F2R, AC3_STEREO ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 1 ] = MUL_L ( aLevel, aCLev );
   apCoeff[ 3 ] = apCoeff[ 4 ] = MUL_L ( aLevel, aSLev );

  return 31;

  case CONVERT( AC3_3F1R, AC3_3F ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 3 ] = MUL_L ( lLevel3DB, aSLev );

  return 13;

  case CONVERT( AC3_3F2R, AC3_3F ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 3 ] = apCoeff[ 4 ] = MUL_L ( aLevel, aSLev );

  return 29;

  case CONVERT( AC3_2F2R, AC3_2F1R ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = aLevel;
   apCoeff[ 2 ] = apCoeff[ 3 ] = lLevel3DB;

  return 12;

  case CONVERT( AC3_3F2R, AC3_3F1R ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 3 ] = apCoeff[ 4 ] = lLevel3DB;

  return 24;

  case CONVERT( AC3_2F1R, AC3_2F2R ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = aLevel;
   apCoeff[ 2 ] = lLevel3DB;

  return 0;

  case CONVERT( AC3_3F1R, AC3_2F2R ):

   apCoeff[ 0 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 1 ] = MUL_L ( aLevel, aCLev );
   apCoeff[ 3 ] = lLevel3DB;

  return 7;

  case CONVERT( AC3_3F1R, AC3_3F2R ):

   apCoeff[ 0 ] = apCoeff[ 1 ] = apCoeff[ 2 ] = aLevel;
   apCoeff[ 3 ] = lLevel3DB;

  return 0;

  case CONVERT( AC3_CHANNEL, AC3_CHANNEL1 ):

   apCoeff[ 0 ] = aLevel;
   apCoeff[ 1 ] = 0;

  return 0;

  case CONVERT( AC3_CHANNEL, AC3_CHANNEL2 ):

   apCoeff[ 0 ] = 0;
   apCoeff[ 1 ] = aLevel;

  return 0;

 }  /* end switch */

 return -1;

}  /* end _ac3_downmix_coeff */

static SMS_INLINE void _ac3_zero ( sample_t* apSamples ) {

 memset (  apSamples, 0, 256 * sizeof ( sample_t )  );

}  /* end _ac3_zero */

void _ac3_upmix ( sample_t* apSamples, int anACMod, int anOutput ) {

 switch (  CONVERT( anACMod, anOutput & AC3_CHANNEL_MASK )  ) {

  case CONVERT( AC3_CHANNEL, AC3_CHANNEL2 ):

   memcpy ( apSamples + 256, apSamples, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_3F2R, AC3_MONO ):

   _ac3_zero ( apSamples + 1024 );

  case CONVERT( AC3_3F1R, AC3_MONO ):
  case CONVERT( AC3_2F2R, AC3_MONO ):

   _ac3_zero ( apSamples + 768 );

  case CONVERT( AC3_3F,   AC3_MONO ):
  case CONVERT( AC3_2F1R, AC3_MONO ):

   _ac3_zero ( apSamples + 512 );

  case CONVERT( AC3_CHANNEL, AC3_MONO ):
  case CONVERT( AC3_STEREO,  AC3_MONO ):

   _ac3_zero ( apSamples + 256 );

  break;

  case CONVERT( AC3_3F2R, AC3_STEREO ):
  case CONVERT( AC3_3F2R, AC3_DOLBY  ):

   _ac3_zero ( apSamples + 1024 );

  case CONVERT( AC3_3F1R, AC3_STEREO ):
  case CONVERT( AC3_3F1R, AC3_DOLBY  ):

   _ac3_zero ( apSamples + 768 );

  case CONVERT( AC3_3F, AC3_STEREO ):
  case CONVERT( AC3_3F, AC3_DOLBY  ):
mix_3to2:
   memcpy (  apSamples + 512, apSamples + 256, 256 * sizeof ( sample_t )  );
   _ac3_zero ( apSamples + 256 );

  break;

  case CONVERT( AC3_2F2R, AC3_STEREO ):
  case CONVERT( AC3_2F2R, AC3_DOLBY  ):

   _ac3_zero ( apSamples + 768 );

  case CONVERT( AC3_2F1R, AC3_STEREO ):
  case CONVERT( AC3_2F1R, AC3_DOLBY  ):

   _ac3_zero ( apSamples + 512 );

  break;

  case CONVERT( AC3_3F2R, AC3_3F ):

   _ac3_zero ( apSamples + 1024 );

  case CONVERT( AC3_3F1R, AC3_3F   ):
  case CONVERT( AC3_2F2R, AC3_2F1R ):

   _ac3_zero ( apSamples + 768 );

  break;

  case CONVERT( AC3_3F2R, AC3_3F1R ):

   _ac3_zero ( apSamples + 1024 );

  break;

  case CONVERT( AC3_3F2R, AC3_2F1R ):

   _ac3_zero ( apSamples + 1024 );

  case CONVERT( AC3_3F1R, AC3_2F1R ):
mix_31to21:
   memcpy (  apSamples + 768, apSamples + 512, 256 * sizeof ( sample_t )  );

  goto mix_3to2;

  case CONVERT( AC3_3F2R, AC3_2F2R ):

   memcpy (  apSamples + 1024, apSamples + 768, 256 * sizeof ( sample_t )  );

  goto mix_31to21;

 }  /* end switch */

}  /* end _ac3_upmix */

static void _ac3_mix2to1 ( sample_t* apDest, sample_t* apSrc ) {

 int i;

 for ( i = 0; i < 256; ++i ) apDest[ i ] += apSrc[ i ];

}  /* end _ac3_mix2to1 */

static void _ac3_mix3to1 ( sample_t* apSamples ) {

 int i;

 for ( i = 0; i < 256; ++i ) apSamples[ i ] += apSamples[ i + 256 ] + apSamples[ i + 512 ];

}  /* end _ac3_mix3to1 */

static void _ac3_mix4to1 ( sample_t* apSamples ) {

 int i;

 for ( i = 0; i < 256; ++i )

  apSamples[ i ] += apSamples[ i + 256 ] + apSamples[ i + 512 ] + apSamples[ i + 768 ];

}  /* end _ac3_mix4to1 */

static void _ac3_mix5to1 ( sample_t* apSamples ) {

 int i;

 for ( i = 0; i < 256; ++i ) apSamples[ i ] += apSamples[ i + 256 ] + apSamples[ i + 512 ] + apSamples[ i + 768 ] + apSamples[ i + 1024 ];

}  /* end _ac3_mix5to1 */

static void _ac3_mix3to2 ( sample_t* apSamples ) {

 int      i;
 sample_t lCommon;

 for ( i = 0; i < 256; ++i ) {

  lCommon = apSamples[ i + 256 ];
  apSamples[ i       ] += lCommon;
  apSamples[ i + 256 ]  = apSamples[ i + 512 ] + lCommon;

 }  /* end for */

}  /* end _ac3_mix3to2 */

static void _ac3_mix21to2 ( sample_t* apLeft, sample_t* apRight ) {

 int      i;
 sample_t lCommon;

 for ( i = 0; i < 256; ++i ) {

  lCommon = apRight[ i + 256 ];
  apLeft [ i ] += lCommon;
  apRight[ i ] += lCommon;

 }  /* end for */

}  /* end _ac3_mix21to2 */

static void _ac3_mix21toS ( sample_t* apSamples ) {

 int      i;
 sample_t lSurround;

 for ( i = 0; i < 256; ++i ) {

  lSurround = apSamples[ i + 512 ];
  apSamples[ i       ] += -lSurround;
  apSamples[ i + 256 ] +=  lSurround;

 }  /* end for */

}  /* end _ac3_mix21toS */

static void _ac3_mix31to2 ( sample_t* apSamples ) {

 int i;
 sample_t lCommon;

 for ( i = 0; i < 256; ++i ) {

  lCommon = apSamples[ i + 256 ] + apSamples[ i + 768 ];
  apSamples[ i       ] += lCommon;
  apSamples[ i + 256 ]  = apSamples[ i + 512 ] + lCommon;

 }  /* end for */

}  /* end _ac3_mix31to2 */

static void _ac3_mix31toS ( sample_t* apSamples ) {

 int      i;
 sample_t lCommon, lSurround;

 for ( i = 0; i < 256; ++i ) {

  lCommon   = apSamples[ i + 256 ];
  lSurround = apSamples[ i + 768 ];

  apSamples[ i       ] += lCommon - lSurround;
  apSamples[ i + 256 ]  = apSamples[ i + 512 ] + lCommon + lSurround;

 }  /* end for */

}  /* end _ac3_mix31toS */

static void _ac3_mix22toS ( sample_t* apSamples ) {

 int      i;
 sample_t lSurround;

 for ( i = 0; i < 256; ++i ) {

  lSurround = apSamples[ i + 512 ] + apSamples[ i + 768 ];
  apSamples[ i       ] += -lSurround;
  apSamples[ i + 256 ] +=  lSurround;

 }  /* end for */

}  /* end _ac3_mix22toS */

static void _ac3_mix32to2 ( sample_t* apSamples ) {

 int      i;
 sample_t lCommon;

 for ( i = 0; i < 256; ++i ) {

  lCommon = apSamples[ i + 256 ];
  apSamples[ i       ] += lCommon + apSamples[ i + 768 ];
  apSamples[ i + 256 ]  = lCommon + apSamples[ i + 512 ] + apSamples[ i + 1024 ];

 }  /* end for */

}  /* end _ac3_mix32to2 */

static void _ac3_mix32toS ( sample_t* apSamples ) {

 int      i;
 sample_t lCommon, lSurround;

 for ( i = 0; i < 256; ++i ) {

  lCommon   = apSamples[ i + 256 ];
  lSurround = apSamples[ i + 768 ] + apSamples[ i + 1024 ];
  apSamples[ i       ] += lCommon - lSurround;
  apSamples[ i + 256 ]  = apSamples[ i + 512 ] + lCommon + lSurround;

 }  /* end for */

}  /* end _ac3_mix32toS */

static void _ac3_move2to1 ( sample_t* apSrc, sample_t* apDest ) {

 int i;

 for (i = 0; i < 256; i++) apDest[ i ] = apSrc[ i ] + apSrc[ i + 256 ];

}  /* end _ac3_move2to1 */

void _ac3_downmix ( sample_t* apSamples, int anACMod, int anOutput, sample_t aBias, level_t aCLev, level_t aSLev ) {

 switch (  CONVERT( anACMod, anOutput & AC3_CHANNEL_MASK )  ) {

  case CONVERT( AC3_CHANNEL, AC3_CHANNEL2 ):

   memcpy (  apSamples, apSamples + 256, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_CHANNEL, AC3_MONO ):
  case CONVERT( AC3_STEREO,  AC3_MONO ):
mix_2to1:
   _ac3_mix2to1 ( apSamples, apSamples + 256 );

  break;

  case CONVERT( AC3_2F1R, AC3_MONO ):

   if ( aSLev == 0 ) goto mix_2to1;

  case CONVERT( AC3_3F, AC3_MONO ):
mix_3to1:
   _ac3_mix3to1 ( apSamples );

  break;

  case CONVERT( AC3_3F1R, AC3_MONO ):

   if ( aSLev == 0 ) goto mix_3to1;

  case CONVERT( AC3_2F2R, AC3_MONO ):

   if ( aSLev == 0 ) goto mix_2to1;

   _ac3_mix4to1 ( apSamples );

  break;

  case CONVERT( AC3_3F2R, AC3_MONO ):

   if ( aSLev == 0 ) goto mix_3to1;

   _ac3_mix5to1 ( apSamples );

  break;

  case CONVERT( AC3_MONO, AC3_DOLBY ):

   memcpy (  apSamples + 256, apSamples, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_3F, AC3_STEREO ):
  case CONVERT( AC3_3F, AC3_DOLBY  ):
mix_3to2:
   _ac3_mix3to2 ( apSamples );

  break;

  case CONVERT( AC3_2F1R, AC3_STEREO ):

   if ( aSLev == 0 ) break;

   _ac3_mix21to2 ( apSamples, apSamples + 256 );

  break;

  case CONVERT( AC3_2F1R, AC3_DOLBY ):

   _ac3_mix21toS ( apSamples );

  break;

  case CONVERT( AC3_3F1R, AC3_STEREO ):

   if ( aSLev == 0 ) goto mix_3to2;

   _ac3_mix31to2 ( apSamples );

  break;

  case CONVERT( AC3_3F1R, AC3_DOLBY ):

   _ac3_mix31toS ( apSamples );

  break;

  case CONVERT( AC3_2F2R, AC3_STEREO ):

   if ( aSLev == 0 ) break;

   _ac3_mix2to1 ( apSamples,       apSamples + 512 );
   _ac3_mix2to1 ( apSamples + 256, apSamples + 768 );

  break;

  case CONVERT( AC3_2F2R, AC3_DOLBY ):

   _ac3_mix22toS ( apSamples );

  break;

  case CONVERT( AC3_3F2R, AC3_STEREO ):

   if ( aSLev == 0 ) goto mix_3to2;

   _ac3_mix32to2 ( apSamples );

  break;

  case CONVERT( AC3_3F2R, AC3_DOLBY ):

   _ac3_mix32toS ( apSamples );

  break;

  case CONVERT( AC3_3F1R, AC3_3F ):

   if ( aSLev == 0 ) break;

   _ac3_mix21to2 ( apSamples, apSamples + 512 );

  break;

  case CONVERT( AC3_3F2R, AC3_3F ):

   if ( aSLev == 0 ) break;

   _ac3_mix2to1 ( apSamples,       apSamples +  768 );
   _ac3_mix2to1 ( apSamples + 512, apSamples + 1024 );

  break;

  case CONVERT( AC3_3F1R, AC3_2F1R ):

   _ac3_mix3to2 ( apSamples );
   memcpy (  apSamples + 512, apSamples + 768, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_2F2R, AC3_2F1R ):

   _ac3_mix2to1 ( apSamples + 512, apSamples + 768 );

  break;

  case CONVERT( AC3_3F2R, AC3_2F1R ):

   _ac3_mix3to2 ( apSamples );
   _ac3_move2to1 ( apSamples + 768, apSamples + 512 );

  break;

  case CONVERT( AC3_3F2R, AC3_3F1R ):

   _ac3_mix2to1 ( apSamples + 768, apSamples + 1024 );

  break;

  case CONVERT( AC3_2F1R, AC3_2F2R ):

   memcpy (  apSamples + 768, apSamples + 512, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_3F1R, AC3_2F2R ):

   _ac3_mix3to2 ( apSamples );
   memcpy (  apSamples + 512, apSamples + 768, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_3F2R, AC3_2F2R ):

   _ac3_mix3to2 ( apSamples );
   memcpy (  apSamples + 512, apSamples +  768, 256 * sizeof ( sample_t )  );
   memcpy (  apSamples + 768, apSamples + 1024, 256 * sizeof ( sample_t )  );

  break;

  case CONVERT( AC3_3F1R, AC3_3F2R ):

   memcpy (  apSamples + 1024, apSamples + 768, 256 * sizeof ( sample_t )  );

  break;

 }  /* end switch */

}  /* end _ac3_downmix */

static int _ac3_block ( void ) {

 static const uint8_t s_nFChansTbl[ 11 ] = {
  2, 1, 2, 3, 3, 4, 4, 5, 1, 1, 2
 };
 static int s_RematrixBand[ 4 ] = {
  25, 37, 61, 253
 };

 int             i, lnFChans, lChanInfo;
 uint8_t         lCplExpStr, lChExpStr[ 5 ], lLFEExpStr, lDoBitAlloc, lDoneCpl;
 uint8_t         lBlkSw[ 5 ], lDithFlag[ 5 ];
 level_t         lCoeff[ 5 ];
 int             lChanBias;
 quantizer_set_t lQuant;
 sample_t*       lpSamples;

 lnFChans = s_nFChansTbl[ s_AC3Ctx.m_ACMmod ];

 for ( i = 0; i < lnFChans; ++i )

  lBlkSw[ i ] = SMS_GetBit ( &s_AC3Ctx.m_BitCtx );

 for ( i = 0; i < lnFChans; ++i )

  lDithFlag[ i ] = SMS_GetBit ( &s_AC3Ctx.m_BitCtx );

 lChanInfo = !s_AC3Ctx.m_ACMmod;

 do {

  if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

   int lDynRng = SMS_GetSBits ( &s_AC3Ctx.m_BitCtx, 8 );

   if ( s_AC3Ctx.m_DynRnge ) {

    level_t lRange = (  ( lDynRng & 0x1F ) | 0x20  ) << (  21 + ( lDynRng >> 5 )  );

    s_AC3Ctx.m_DynRng = MUL_L( s_AC3Ctx.m_Level, lRange );

   }  /* end if */

  }  /* end if */

 } while ( lChanInfo-- );

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  s_AC3Ctx.m_ChInCpl = 0;

  if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

   static uint8_t s_BndTab[ 16 ] = {
    31, 35, 37, 39, 41, 42, 43, 44,
    45, 45, 46, 46, 47, 47, 48, 48
   };
   int lCplBegF;
   int lCplEndF;
   int lnCplSubnd;

   for ( i = 0; i < lnFChans; ++i ) s_AC3Ctx.m_ChInCpl |= SMS_GetBit ( &s_AC3Ctx.m_BitCtx ) << i;

   switch ( s_AC3Ctx.m_ACMmod ) {

    case 0:
    case 1: return 1;

    case 2: s_AC3Ctx.m_PhsFlgInU = SMS_GetBit ( &s_AC3Ctx.m_BitCtx );

   }  /* end switch */

   lCplBegF = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );
   lCplEndF = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );

   if ( lCplEndF + 3 - lCplBegF < 0 ) return 1;

   s_AC3Ctx.m_nCplBnd     = lnCplSubnd = lCplEndF + 3 - lCplBegF;
   s_AC3Ctx.m_CplStrtBnd  = s_BndTab[ lCplBegF ];
   s_AC3Ctx.m_CplStrtMant = lCplBegF * 12 + 37;
   s_AC3Ctx.m_CplEndMant  = lCplEndF * 12 + 73;
   s_AC3Ctx.m_CplBndStrc  = 0;

   for ( i = 0; i < lnCplSubnd - 1; ++i )

    if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

     s_AC3Ctx.m_CplBndStrc |= 1 << i;
     --s_AC3Ctx.m_nCplBnd;

    }  /* end if */

  }  /* end if */

 }  /* end if */

 if ( s_AC3Ctx.m_ChInCpl ) {

  int j, lCplCoe = 0;

  for ( i = 0; i < lnFChans; ++i )

   if (  ( s_AC3Ctx.m_ChInCpl ) >> i & 1  )

    if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

     int lMStrCplCo, lCplCoExp, lCplCoMant;

     lCplCoe    = 1;
     lMStrCplCo = 3 * SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

     for ( j = 0; j < s_AC3Ctx.m_nCplBnd; ++j ) {

      lCplCoExp  = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );
	  lCplCoMant = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );

      if ( lCplCoExp == 15 )

       lCplCoMant <<= 14;

      else lCplCoMant = ( lCplCoMant | 0x10 ) << 13;

      s_AC3Ctx.m_CplCo[ i ][ j ] = ( lCplCoMant << 11 ) >> ( lCplCoExp + lMStrCplCo );

     }  /* end for */

    }  /* end if */

  if ( s_AC3Ctx.m_ACMmod == 2  && s_AC3Ctx.m_PhsFlgInU && lCplCoe )

   for ( j = 0; j < s_AC3Ctx.m_nCplBnd; ++j )

    if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) s_AC3Ctx.m_CplCo[ 1 ][ j ] = -s_AC3Ctx.m_CplCo[ 1 ][ j ];

 }  /* end if */

 if (  s_AC3Ctx.m_ACMmod == 2 && SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  int lEnd;

  s_AC3Ctx.m_RematFlg = 0;
  lEnd = s_AC3Ctx.m_ChInCpl ? s_AC3Ctx.m_CplStrtMant : 253;

  i = 0;

  do

   s_AC3Ctx.m_RematFlg  |= SMS_GetBit ( &s_AC3Ctx.m_BitCtx ) << i;

  while ( s_RematrixBand[ i++ ] < lEnd );

 }  /* end if */

 lCplExpStr = EXP_REUSE;
 lLFEExpStr = EXP_REUSE;

 if ( s_AC3Ctx.m_ChInCpl ) lCplExpStr = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

 for ( i = 0; i < lnFChans; ++i ) lChExpStr[ i ] = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

 if (s_AC3Ctx.m_LFEOn ) lLFEExpStr = SMS_GetBit ( &s_AC3Ctx.m_BitCtx );

 for ( i = 0; i < lnFChans; ++i )

  if ( lChExpStr[ i ] != EXP_REUSE ) {

   if (  ( s_AC3Ctx.m_ChInCpl >> i ) & 1  )

    s_AC3Ctx.m_EndMant[ i ] = s_AC3Ctx.m_CplStrtMant;

   else {

    int lChBwCod = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 6 );

    if ( lChBwCod > 60 ) return 1;

    s_AC3Ctx.m_EndMant[ i ] = lChBwCod * 3 + 73;

   }  /* end else */

  }  /* end if */

 lDoBitAlloc = 0;

 if ( lCplExpStr != EXP_REUSE ) {

  int lCplAbsExp, lnCplGrps;

  lDoBitAlloc = 64;
  lnCplGrps   = ( s_AC3Ctx.m_CplEndMant - s_AC3Ctx.m_CplStrtMant ) /
		        (  3 << ( lCplExpStr - 1 )  );
  lCplAbsExp = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 ) << 1;

  if (  _ac3_parse_exponents (
         lCplExpStr, lnCplGrps, lCplAbsExp,
         s_AC3Ctx.m_CplExpBAP.m_Exp + s_AC3Ctx.m_CplStrtMant
        )
  ) return 1;

 }  /* end if */

 for ( i = 0; i < lnFChans; ++i )

  if ( lChExpStr[ i ] != EXP_REUSE ) {

   int lGrpSize, lnChGrps;

   lDoBitAlloc |= 1 << i;
   lGrpSize     = 3 << ( lChExpStr[ i ] - 1 );
   lnChGrps     = ( s_AC3Ctx.m_EndMant[ i ] + lGrpSize - 4) / lGrpSize;

   s_AC3Ctx.m_FBWExpBAP[ i ].m_Exp[ 0 ] = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );

   if (  _ac3_parse_exponents (
          lChExpStr[ i ], lnChGrps, s_AC3Ctx.m_FBWExpBAP[ i ].m_Exp[ 0 ],
          s_AC3Ctx.m_FBWExpBAP[ i ].m_Exp + 1
         )
   ) return 1;

   SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

  }  /* end if */

 if ( lLFEExpStr != EXP_REUSE ) {

  lDoBitAlloc |= 32;
  s_AC3Ctx.m_LFEExpBAP.m_Exp[ 0 ] = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 4 );

  if (  _ac3_parse_exponents (
         lLFEExpStr, 2, s_AC3Ctx.m_LFEExpBAP.m_Exp[ 0 ],
         s_AC3Ctx.m_LFEExpBAP.m_Exp + 1
        )
  ) return 1;

 }  /* end if */

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  lDoBitAlloc = 127;
  s_AC3Ctx.m_BAI = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 11 );

 }  /* end if */

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  lDoBitAlloc = 127;
  s_AC3Ctx.m_CSNROffst = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 6 );

  if ( s_AC3Ctx.m_ChInCpl ) s_AC3Ctx.m_CplBA.m_BAI = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

  for ( i = 0; i < lnFChans; ++i ) s_AC3Ctx.m_BA[ i ].m_BAI = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

  if ( s_AC3Ctx.m_LFEOn ) s_AC3Ctx.m_LFEBA.m_BAI = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 7 );

 }  /* end if */

 if (  s_AC3Ctx.m_ChInCpl && SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  lDoBitAlloc |= 64;
  s_AC3Ctx.m_CplFLeak = 9 - SMS_GetBits  (&s_AC3Ctx.m_BitCtx, 3 );
  s_AC3Ctx.m_CplSLeak = 9 - SMS_GetBits  (&s_AC3Ctx.m_BitCtx, 3 );

 }  /* end if */

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  lDoBitAlloc = 127;

  if ( s_AC3Ctx.m_ChInCpl )	s_AC3Ctx.m_CplBA.m_DeltBAE = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

  for ( i = 0; i < lnFChans; ++i ) s_AC3Ctx.m_BA[ i ].m_DeltBAE = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 2 );

  if (  s_AC3Ctx.m_ChInCpl && s_AC3Ctx.m_CplBA.m_DeltBAE == DELTA_BIT_NEW && _ac3_parse_deltba ( s_AC3Ctx.m_CplBA.m_DeltBA )  ) return 1;

  for ( i = 0; i < lnFChans; ++i )

   if ( s_AC3Ctx.m_BA[ i ].m_DeltBAE == DELTA_BIT_NEW && _ac3_parse_deltba ( s_AC3Ctx.m_BA[ i ].m_DeltBA )  ) return 1;

 }  /* end if */

 if ( lDoBitAlloc ) {

  if (  _ac3_zero_snr_offsets ( lnFChans )  ) {

   memset (  s_AC3Ctx.m_CplExpBAP.m_BAP, 0, sizeof ( s_AC3Ctx.m_CplExpBAP.m_BAP )  );

   for ( i = 0; i < lnFChans; ++i ) memset (  s_AC3Ctx.m_FBWExpBAP[ i ].m_BAP, 0, sizeof ( s_AC3Ctx.m_FBWExpBAP[ i ].m_BAP )  );

   memset (  s_AC3Ctx.m_LFEExpBAP.m_BAP, 0, sizeof ( s_AC3Ctx.m_LFEExpBAP.m_BAP )  );

  } else {

   if (  s_AC3Ctx.m_ChInCpl && ( lDoBitAlloc & 64 )  )

    _ac3_bit_allocate (
     &s_AC3Ctx.m_CplBA, s_AC3Ctx.m_CplStrtBnd,
     s_AC3Ctx.m_CplStrtMant, s_AC3Ctx.m_CplEndMant,
     s_AC3Ctx.m_CplFLeak << 8, s_AC3Ctx.m_CplSLeak << 8,
     &s_AC3Ctx.m_CplExpBAP
    );

   for ( i = 0; i < lnFChans; ++i )

    if (  lDoBitAlloc & ( 1 << i )  )

     _ac3_bit_allocate (
      s_AC3Ctx.m_BA + i, 0, 0,
      s_AC3Ctx.m_EndMant[ i ], 0, 0,
      s_AC3Ctx.m_FBWExpBAP + i
     );

   if (  s_AC3Ctx.m_LFEOn && ( lDoBitAlloc & 32 )  ) {

    s_AC3Ctx.m_LFEBA.m_DeltBAE = DELTA_BIT_NONE;

    _ac3_bit_allocate (
     &s_AC3Ctx.m_LFEBA, 0, 0, 7, 0, 0, &s_AC3Ctx.m_LFEExpBAP
    );

   }  /* end if */

  }  /* end else */

 }  /* end if */

 if (  SMS_GetBit ( &s_AC3Ctx.m_BitCtx )  ) {

  i = SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 9 );

  while ( i-- ) SMS_GetBits ( &s_AC3Ctx.m_BitCtx, 8 );

 }  /* end if */

 lpSamples = s_AC3Ctx.m_pSamples;

 if ( s_AC3Ctx.m_Output & AC3_LFE ) lpSamples += 256;

 lChanBias = _ac3_downmix_coeff (
  lCoeff, s_AC3Ctx.m_ACMmod, s_AC3Ctx.m_Output,
  s_AC3Ctx.m_DynRng, s_AC3Ctx.m_CLev, s_AC3Ctx.m_SLev
 );

 lQuant.m_pQ1 = lQuant.m_pQ2 = lQuant.m_pQ4 = -1;
 lDoneCpl     = 0;

 for ( i = 0; i < lnFChans; ++i ) {

  int j;

  _ac3_coeff_get (
   lpSamples + 256 * i, s_AC3Ctx.m_FBWExpBAP + i, &lQuant,
   lCoeff[ i ], lDithFlag[ i ], s_AC3Ctx.m_EndMant[ i ]
  );

  if (  ( s_AC3Ctx.m_ChInCpl >> i ) & 1  ) {

   if ( !lDoneCpl ) {

    lDoneCpl = 1;

    _ac3_coeff_get_coupling (
     lnFChans, lCoeff, (  sample_t ( * )[ 256 ] )lpSamples, &lQuant, lDithFlag
    );

   }  /* end if */

   j = s_AC3Ctx.m_CplEndMant;

  } else j = s_AC3Ctx.m_EndMant[ i ];

  do

   ( lpSamples + 256 * i )[ j ] = 0;

  while ( ++j < 256 );

 }  /* end for */

 if ( s_AC3Ctx.m_ACMmod == 2 ) {

  int j, lEnd, lBand, lRematFlg;

  lEnd = s_AC3Ctx.m_EndMant[ 0 ] < s_AC3Ctx.m_EndMant[ 1 ] ? s_AC3Ctx.m_EndMant[ 0 ] : s_AC3Ctx.m_EndMant[ 1 ];

  i         =  0;
  j         = 13;
  lRematFlg = s_AC3Ctx.m_RematFlg;

  do {

   if (  !( lRematFlg & 1 )  ) {

    lRematFlg >>= 1;
    j           = s_RematrixBand[ i++ ];
    continue;

   }  /* end if */

   lRematFlg >>= 1;
   lBand       = s_RematrixBand[ i++ ];

   if ( lBand > lEnd ) lBand = lEnd;

   do {

    sample_t lTmp0, lTmp1;

    lTmp0 = lpSamples[ j ];
    lTmp1 = ( lpSamples + 256 )[ j ];
    lpSamples[ j ] = lTmp0 + lTmp1;
    ( lpSamples + 256 )[ j ] = lTmp0 - lTmp1;

   } while ( ++j < lBand);

  } while ( j < lEnd);

 }  /* end if */

 if ( s_AC3Ctx.m_LFEOn ) {

  if ( s_AC3Ctx.m_Output & AC3_LFE ) {

   _ac3_coeff_get (
    lpSamples - 256, &s_AC3Ctx.m_LFEExpBAP, &lQuant,
    s_AC3Ctx.m_DynRng, 0, 7
   );

   for ( i = 7; i < 256; ++i ) ( lpSamples - 256 )[ i ] = 0;

   _ac3_imdct_512 ( lpSamples - 256, lpSamples + 1536 - 256, s_AC3Ctx.m_Bias );

  } else _ac3_coeff_get ( lpSamples + 1280, &s_AC3Ctx.m_LFEExpBAP, &lQuant, 0, 0, 7 );

 }  /* end if */

 i = 0;

 if ( s_nFChansTbl[ s_AC3Ctx.m_Output & AC3_CHANNEL_MASK ] < lnFChans )

  for ( i = 1; i < lnFChans; ++i ) if ( lBlkSw[ i ] != lBlkSw[ 0 ] ) break;

 if ( i < lnFChans ) {

  if ( s_AC3Ctx.m_Downmixed ) {

   s_AC3Ctx.m_Downmixed = 0;
   _ac3_upmix ( lpSamples + 1536, s_AC3Ctx.m_ACMmod, s_AC3Ctx.m_Output );

  }  /* end if */

  for ( i = 0; i < lnFChans; ++i ) {

   sample_t lBias = 0;

   if (   !(  lChanBias & ( 1 << i )  )   ) lBias = s_AC3Ctx.m_Bias;

   if ( lCoeff[ i ] ) {

    if ( lBlkSw[ i ] )

     _ac3_imdct_256 ( lpSamples + 256 * i, lpSamples + 1536 + 256 * i, lBias );

    else _ac3_imdct_512 ( lpSamples + 256 * i, lpSamples + 1536 + 256 * i, lBias );

   } else {

    int j;

    for ( j = 0; j < 256; ++j ) ( lpSamples + 256 * i )[ j ] = lBias;

   }  /* end else */

  }  /* end for */

  _ac3_downmix (
   lpSamples, s_AC3Ctx.m_ACMmod, s_AC3Ctx.m_Output, s_AC3Ctx.m_Bias,
   s_AC3Ctx.m_CLev, s_AC3Ctx.m_SLev
  );

 } else {

  lnFChans = s_nFChansTbl[ s_AC3Ctx.m_Output & AC3_CHANNEL_MASK ];

  _ac3_downmix (
   lpSamples, s_AC3Ctx.m_ACMmod, s_AC3Ctx.m_Output, 0,
   s_AC3Ctx.m_CLev, s_AC3Ctx.m_SLev
  );

  if ( !s_AC3Ctx.m_Downmixed ) {

   s_AC3Ctx.m_Downmixed = 1;

   _ac3_downmix (
    lpSamples + 1536, s_AC3Ctx.m_ACMmod, s_AC3Ctx.m_Output, 0,
    s_AC3Ctx.m_CLev, s_AC3Ctx.m_SLev
   );

  }  /* end if */

  if ( lBlkSw[ 0 ] )

   for ( i = 0; i < lnFChans; ++i )

    _ac3_imdct_256 (
     lpSamples + 256 * i, lpSamples + 1536 + 256 * i, s_AC3Ctx.m_Bias
    );

  else for ( i = 0; i < lnFChans; ++i ) _ac3_imdct_512 (
                                         lpSamples + 256 * i, lpSamples + 1536 + 256 * i, s_AC3Ctx.m_Bias
                                        );
 }  /* end else */

 return 0;

}  /* end _ac3_block */

static SMS_INLINE int _ac3_blah ( int32_t i ) {

 i >>= 15;

 return i > 32767 ? 32767 : i < -32768 ? -32768 : i;

}  /* end _ac3_blah */

static SMS_INLINE void _ac3_float_to_int ( sample_t* apSamples, int16_t* apOutput, int anChannels ) {

 int      i, j, c;
 int32_t* lpSamples = ( int32_t* )apSamples;

 j           =   0;
 anChannels *= 256;

 for ( i = 0; i < 256; ++i )

  for ( c = 0; c < anChannels; c += 256 )

   apOutput[ j++ ] = _ac3_blah ( lpSamples[ i + c ] );

}  /* end _ac3_float_to_int */

static int32_t AC3_Decode ( SMS_CodecContext* apCtx, void** apData, uint8_t* apBuf, int32_t aBufSize ) {

 static const int s_AC3Channels[ 8 ] = { 2, 1, 2, 3, 3, 4, 4, 5 };

 uint8_t*          lpBuf = apBuf;
 int               lFlags, i, lLen;
 int               lSampleRate, lBitRate;
 sample_t          lLevel;
 SMS_AudioBuffer** lppBuffer = ( SMS_AudioBuffer** )apData;
 short*            lpOutBuf;
 int               retVal = 0;

 *lppBuffer = s_AC3Ctx.m_pOutBuffer;

 if ( s_AC3Ctx.m_pOutBuffer -> m_Len != 0 ) {

  lpBuf    = apBuf = s_AC3Ctx.m_pOutBuffer -> m_pPos;
  aBufSize = s_AC3Ctx.m_pOutBuffer -> m_Len;

 } else {

  s_AC3Ctx.m_pOutBuffer -> m_pPos = lpBuf = apBuf;
  s_AC3Ctx.m_pOutBuffer -> m_Len  = aBufSize;

 }  /* end else */

 while ( aBufSize > 0 ) {

  lLen = s_AC3Ctx.m_pInBuf - s_AC3Ctx.m_InBuf;

  if ( s_AC3Ctx.m_FrameSize == 0 ) {

   lLen = HEADER_SIZE - lLen;

   if ( lLen > aBufSize ) lLen = aBufSize;

   memcpy ( s_AC3Ctx.m_pInBuf, lpBuf, lLen );

   lpBuf             += lLen;
   s_AC3Ctx.m_pInBuf += lLen;
   aBufSize          -= lLen;

   if ( s_AC3Ctx.m_pInBuf - s_AC3Ctx.m_InBuf == HEADER_SIZE ) {

    lLen = _ac3_syncinfo (
     s_AC3Ctx.m_InBuf, &s_AC3Ctx.m_Flags, &lSampleRate, &lBitRate
    );

    if ( lLen == 0 ) {

     memcpy ( s_AC3Ctx.m_InBuf, s_AC3Ctx.m_InBuf + 1, HEADER_SIZE - 1 );
     --s_AC3Ctx.m_pInBuf;

    } else {

     s_AC3Ctx.m_FrameSize  = lLen;
     apCtx -> m_SampleRate = lSampleRate;
     s_AC3Ctx.m_nChannels  = s_AC3Channels[ s_AC3Ctx.m_Flags & 7 ];

     if ( s_AC3Ctx.m_Flags & AC3_LFE ) ++s_AC3Ctx.m_nChannels;

     if ( apCtx -> m_Channels == 0 )

      apCtx -> m_Channels = s_AC3Ctx.m_nChannels;

     else if ( s_AC3Ctx.m_nChannels < apCtx -> m_Channels )

      apCtx -> m_Channels = s_AC3Ctx.m_nChannels;

    }  /* end else */

    apCtx -> m_BitRate = lBitRate;

   }  /* end if */

  } else if ( lLen < s_AC3Ctx.m_FrameSize ) {

   lLen = s_AC3Ctx.m_FrameSize - lLen;

   if ( lLen > aBufSize ) lLen = aBufSize;

   memcpy ( s_AC3Ctx.m_pInBuf, lpBuf, lLen );

   lpBuf             += lLen;
   s_AC3Ctx.m_pInBuf += lLen;
   aBufSize          -= lLen;

  } else {

   lFlags = s_AC3Ctx.m_Flags;

   if ( apCtx -> m_Channels == 1 )

    lFlags = AC3_MONO;

   else if ( apCtx -> m_Channels == 2 )

    lFlags = AC3_STEREO;

   else lFlags |= AC3_ADJUST_LEVEL;

   lLevel = 12 << 24;

   if (  _ac3_frame ( s_AC3Ctx.m_InBuf, &lFlags, &lLevel, 384 )  ) {

    s_AC3Ctx.m_pInBuf    = s_AC3Ctx.m_InBuf;
    s_AC3Ctx.m_FrameSize = 0;

    continue;

   }  /* end if */

   retVal   = 6 * apCtx -> m_Channels * 256 * sizeof ( short );
   lpOutBuf = ( short* )s_AC3Ctx.m_pOutBuffer -> Alloc ( retVal );

   for ( i = 0; i < 6; ++i ) {

    _ac3_block ();
    _ac3_float_to_int (
     s_AC3Ctx.m_pSamples, lpOutBuf + i * 256 * apCtx -> m_Channels, apCtx -> m_Channels
    );

   }  /* end for */

   s_AC3Ctx.m_pInBuf    = s_AC3Ctx.m_InBuf;
   s_AC3Ctx.m_FrameSize = 0;

   break;

  }  /* end else */

 }  /* end while */

 lLen = lpBuf - s_AC3Ctx.m_pOutBuffer -> m_pPos;

 s_AC3Ctx.m_pOutBuffer -> m_Len  -= lLen;
 s_AC3Ctx.m_pOutBuffer -> m_pPos += lLen;

 return retVal;

}  /* end AC3_Decode */
