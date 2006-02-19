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
#ifndef __SMS_VLC_H
# define __SMS_VLC_H

# ifndef __SMS_Bitio_H
#  include "SMS_Bitio.h"
# endif  /* __SMS_Bitio_H */

# define SMS_MAX_RUN     64
# define SMS_MAX_LEVEL   64
# define SMS_MV_VLC_BITS  9

typedef struct SMS_VLC {

 int32_t m_Bits;
 int16_t ( *m_pTable )[ 2 ];
 int32_t m_TableSize;
 int32_t m_TableAlloc;

} SMS_VLC;

typedef struct SMS_RL_VLC_ELEM {

 int16_t m_Level;
 int8_t  m_Len;
 uint8_t m_Run;

} SMS_RL_VLC_ELEM;

typedef struct SMS_RLTable {

 int               m_n;
 int               m_Last;
 const uint16_t    ( *m_pTableVLC )[ 2 ];
 const int8_t*     m_pTableRun;
 const int8_t*     m_pTableLevel;
 int8_t*           m_pMaxLevel[ 2 ];
 int8_t*           m_pMaxRun  [ 2 ];
 SMS_VLC           m_VLC;
 SMS_RL_VLC_ELEM*  m_pRLVLC[ 32 ];

} SMS_RLTable;

extern SMS_VLC g_SMS_mv_vlc;

# define SMS_GET_VLC( code, name, gb, table, bits, max_depth ) { \
 int n, index, nb_bits;                                          \
 index = SMS_SHOW_UBITS( name, gb, bits );                       \
 code  = table[ index ][ 0 ];                                    \
 n     = table[ index ][ 1 ];                                    \
 if ( max_depth > 1 && n < 0 ) {                                 \
  SMS_LAST_SKIP_BITS( name, gb, bits )                           \
  SMS_UPDATE_CACHE( name, gb )                                   \
  nb_bits = -n;                                                  \
  index   = SMS_SHOW_UBITS( name, gb, nb_bits ) + code;          \
  code    = table[ index ][ 0 ];                                 \
  n       = table[ index ][ 1 ];                                 \
  if( max_depth > 2 && n < 0 ) {                                 \
   SMS_LAST_SKIP_BITS( name, gb, nb_bits )                       \
   SMS_UPDATE_CACHE( name, gb )                                  \
   nb_bits = -n;                                                 \
   index   = SMS_SHOW_UBITS( name, gb, nb_bits ) + code;         \
   code    = table[ index ][ 0 ];                                \
   n       = table[ index ][ 1 ];                                \
  }                                                              \
 }                                                               \
 SMS_SKIP_BITS( name, gb, n )                                    \
}

# define SMS_GET_RL_VLC( level, run, name, gb, table, bits, max_depth ) { \
 int n, index, nb_bits;                                                   \
 index = SMS_SHOW_UBITS( name, gb, bits );                                \
 level = table[ index ].m_Level;                                          \
 n     = table[ index ].m_Len;                                            \
 if ( max_depth > 1 && n < 0 ) {                                          \
  SMS_LAST_SKIP_BITS( name, gb, bits )                                    \
  SMS_UPDATE_CACHE( name, gb )                                            \
  nb_bits = -n;                                                           \
  index   = SMS_SHOW_UBITS( name, gb, nb_bits ) + level;                  \
  level   = table[ index ].m_Level;                                       \
  n       = table[ index ].m_Len;                                         \
 }                                                                        \
 run = table[ index ].m_Run;                                              \
 SMS_SKIP_BITS( name, gb, n )                                             \
}

static SMS_INLINE int SMS_GetVLC ( SMS_BitContext* apCtx, SMS_VLC* apVLC ) {
 int        lCode;
 int16_t ( *lpTbl )[ 2 ] = apVLC -> m_pTable;
 SMS_OPEN_READER( re, apCtx )
  SMS_UPDATE_CACHE( re, apCtx )
  SMS_GET_VLC( lCode, re, apCtx, lpTbl, apVLC-> m_Bits, 3 )
 SMS_CLOSE_READER( re, apCtx )
 return lCode;
}  /* end SMS_GetVLC */

static SMS_INLINE int SMS_GetVLC2 ( SMS_BitContext* apCtx, int16_t ( *apTbl )[ 2 ], int aBits, int aMaxDepth ) {
 int lCode;
 SMS_OPEN_READER( re, apCtx )
  SMS_UPDATE_CACHE( re, apCtx )
  SMS_GET_VLC( lCode, re, apCtx, apTbl, aBits, aMaxDepth )
 SMS_CLOSE_READER( re, apCtx )
 return lCode;
}  /* end SMS_GetVLC2 */

# ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int  SMS_VLC_Init ( SMS_VLC*, int, int, const void*, int, int, const void*, int, int );
void SMS_VLC_Free ( SMS_VLC* );

void SMS_RL_Init ( SMS_RLTable* );
void SMS_RL_Free ( SMS_RLTable* );

void SMS_VLC_RL_Init ( SMS_RLTable* );
void SMS_VLC_RL_Free ( SMS_RLTable* );

# ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_VLC_H */
