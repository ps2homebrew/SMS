/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on ffmpeg project (no copyright notes in the original source code)
# (c) 2005-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_H
#define __SMS_H

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

#include <tamtypes.h>

extern unsigned char* g_pSPRTop;
extern void*          g_pSynthBuffer;
extern int            g_XShift;
extern int            g_MCSlot;
extern int            g_RandSeed;

#if __GNUC__ > 3
#include <stdint.h>
#else
typedef signed   char  int8_t;
typedef unsigned char uint8_t;

typedef signed   short  int16_t;
typedef unsigned short uint16_t;

typedef signed   int  int32_t;
typedef unsigned int uint32_t;
#endif

typedef short SMS_DCTELEM;

#define SMS_THREAD_PRIORITY 64

#define SMS_FA_FLAGS_AVI 0x00000001
#define SMS_FA_FLAGS_SUB 0x00000002
#define SMS_FA_FLAGS_MSG 0x00000004

#define SMS_FT_I_TYPE 1
#define SMS_FT_P_TYPE 2
#define SMS_FT_B_TYPE 3
#define SMS_FT_S_TYPE 4
#define SMS_FT_T_TYPE 0x80000000

#define SMS_ROUND( v ) (    ( int )(  ( v ) + (  ( v ) > 0 ? 0.5F : -0.5F  )   )    )

#define SMS_FASTDIV( a, b ) (  ( a ) / ( b )  )

#define SMS_MAX( a, b ) (  ( a ) > ( b ) ? ( a ) : ( b )  )
#define SMS_MIN( a, b ) (  ( a ) > ( b ) ? ( b ) : ( a )  )

#define SMS_ROUNDED_DIV( a, b ) (   (  ( a ) > 0 ? ( a ) + (  ( b ) >> 1  ) : ( a ) - (  ( b ) >> 1  )  ) / ( b )   )

#define SMS_RSHIFT( a, b ) (     ( a ) > 0 ? (    ( a ) + (   (  1 << ( b )  ) >> 1   )    ) >> ( b ) : (    ( a ) + (   (  1 << ( b )  ) >> 1   ) - 1    ) >> ( b )     )

#define SMS_NEG_USR32( a, s ) (   (  ( uint32_t )( a )  ) >> (  32 - ( s )  )   )
#define SMS_NEG_SSR32( a, s ) (   (  ( int32_t  )( a )  ) >> (  32 - ( s )  )   )

#define SMS_BUG_AUTODETECT       0x00000001
#define SMS_BUG_OLD_MSMPEG4      0x00000002
#define SMS_BUG_XVID_ILACE       0x00000004
#define SMS_BUG_UMP4             0x00000008
#define SMS_BUG_NO_PADDING       0x00000010
#define SMS_BUG_AMV              0x00000020
#define SMS_BUG_QPEL_CHROMA      0x00000040
#define SMS_BUG_STD_QPEL         0x00000080
#define SMS_BUG_QPEL_CHROMA2     0x00000100
#define SMS_BUG_DIRECT_BLOCKSIZE 0x00000200
#define SMS_BUG_EDGE             0x00000400
#define SMS_BUG_HPEL_CHROMA      0x00000800
#define SMS_BUG_DC_CLIP          0x00001000

#define SMS_PKT_FLAG_KEY 0x00000001
#define SMS_PKT_FLAG_SUB 0x00000002
#define SMS_PKT_FLAG_NWC 0x00000004

#ifndef NULL
#  define NULL (  ( void* )0  )
#endif  /* NULL */

#if __GNUC__ <= 3
typedef s64                int64_t;
typedef u64               uint64_t;
#endif
typedef unsigned int     uint128_t __attribute__(   (  mode( TI )  )   );

typedef struct SMS_Unaligned32 {
 uint32_t m_Val __attribute__(  ( packed )  );
} SMS_Unaligned32;

typedef struct SMS_Unaligned64 {
 uint64_t m_Val __attribute__(  ( packed )  );
} SMS_Unaligned64;

#  define SMS_INT64( c )    c##LL
#  define SMS_INLINE        inline
#  define SMS_ALIGN( d, a ) d __attribute__(   (  aligned( a )  )   )
#  define SMS_DATA_SECTION  __attribute__(   (  section( ".data" )  )   )
#  define SMS_BSS_SECTION   __attribute__(   (  section( ".bss"  )  )   )
#  define _U( p )           (    ( uint64_t* )(   (  ( uint32_t )( p )  ) | 0x20000000   )    )

#  define SMS_MPEG_SPR_MB_0   (  ( SMS_MacroBlock* )0x70000280  )
#  define SMS_QM_INTRA        (  ( short*          )0x70000400  )
#  define SMS_QM_NON_INTRA    (  ( short*          )0x70000500  )
#  define SMS_MPEG_SPR_BLOCKS (  ( SMS_DCTELEM*    )0x70000580  )
#  define SMS_AUD_SPR         (  ( void*           )0x700008B0  )
#  define SMS_MPEG_SPR_MB_1   (  ( SMS_MacroBlock* )0x700019B0  )
#  define SMS_SPR_FREE        (  ( uint8_t*        )0x70001B30  )

typedef SMS_ALIGN( unsigned char SMS_GUID[ 16 ], 16 );

static inline uint32_t SMS_bswap32 ( uint32_t aVal ) {
 uint32_t retVal;
 __asm__(
  "pextlb   %1, $zero, %1\n\t"
  "prevh    %1, %1\n\t"
  "ppacb    %0, $zero, %1\n\t"
  : "=r"( retVal ) : "r"( aVal )
 );
 return retVal;
}  /* end SMS_bswap32 */

static SMS_INLINE int SMS_log2 ( unsigned int aVal ) {
 int retVal;
 __asm__(
  "srl      $t0, %1, 1\n\t"		
  "addiu    $t1, $zero, 31\n\t"
  "plzcw    $t0, $t0\n\t"
  "subu     %0, $t1, $t0\n\t"
  : "=r"( retVal ) : "r"( aVal ) : "t0", "t1"
 );
 return retVal;
}  /* end SMS_log2 */

static SMS_INLINE float SMS_sqrtf ( float aVal ) {
 float retVal;
 __asm__(
  "sqrt.s %0, %1\n\t"
  : "=f"( retVal ) : "f"( aVal )
 );
 return retVal;
}  /* end SMS_sqrtf */

static inline uint32_t SMS_unaligned32 ( const void* apData ) {
 return (  ( const SMS_Unaligned32* )apData  ) -> m_Val;
}  /* end SMS_unaligned32 */

static inline uint64_t SMS_unaligned64 ( const void* apData ) {
 return (  ( const SMS_Unaligned64* )apData  ) -> m_Val;
}  /* end SMS_unaligned64 */

static inline int SMS_abs ( int aVal ) {
 int retVal;
 __asm__(
  "pextlw  %0, $zero, %1\n\t" 
  "pabsw   %0, %0\n"
  : "=r"( retVal ) : "r"( aVal )
 );
 return retVal;
}  /* end SMS_abs */

static int inline SMS_rand ( void ) {
 return g_RandSeed = ( 161140751 * g_RandSeed + 13 ) % 219441163;
}  /* end SMS_rand */

#define SMS_MAXINT64 SMS_INT64( 0x7FFFFFFFFFFFFFFF )
#define SMS_MININT64 SMS_INT64( 0x8000000000000000 )

#define SMS_NOPTS_VALUE SMS_INT64( 0x8000000000000000 )
#define SMS_STPTS_VALUE SMS_INT64( 0xC000000000000000 )
#define SMS_TIME_BASE   1000

typedef struct SMS_HuffTable {

 int             m_XSize;
 const uint8_t*  m_pBits;
 const uint16_t* m_pCodes;

} SMS_HuffTable;

typedef struct SMS_Rational {
 int m_Num; 
 int m_Den;
} SMS_Rational;

typedef struct SMS_MacroBlock {

 uint8_t m_Y [ 16 ][ 16 ];
 uint8_t m_Cb[  8 ][  8 ];
 uint8_t m_Cr[  8 ][  8 ];

} SMS_MacroBlock;

struct SMS_FrameBuffer;

typedef struct SMS_Frame {

 int16_t                 ( *m_pMotionValBase[ 2 ] )[ 2 ];
 int16_t                 ( *m_pMotionVal    [ 2 ] )[ 2 ];
 int8_t*                 m_pRefIdx[ 2 ];
 short                   m_Width;
 short                   m_Height;
 short                   m_Type;
 short                   m_Ref;
 struct SMS_FrameBuffer* m_pBuf;
 uint32_t*               m_pMBType;
 uint8_t*                m_pMBSkipTbl;
 int8_t*                 m_pQScaleTbl;

} SMS_Frame;

typedef struct SMS_AudioFrame {

 uint8_t* m_pData;
 uint8_t* m_pPos;
 int      m_Len;

} SMS_AudioFrame;

typedef struct SMS_AVPacket {

 int64_t  m_PTS;
 int64_t  m_DTS;
 uint8_t* m_pData;
 uint32_t m_Size;
 uint32_t m_AllocSize;
 uint32_t m_StmIdx;
 uint32_t m_Flags;
 int32_t  m_Duration;

} SMS_AVPacket;

typedef struct SMS_AudioInfo {

 int m_SampleRate;
 int m_nChannels;
 int m_BitRate;

} SMS_AudioInfo;

static SMS_INLINE int SMS_clip ( int aVal, int aMin, int aMax ) {
 int retVal;
 __asm__(
  "pmaxw    %0, %1, %2\n\t"
  "pminw    %0, %0, %3\n\t"
  "sra      %0, %0, 0\n\t"
  : "=r"( retVal ) : "r"( aVal ), "r"( aMin ), "r"( aMax )
 );
 return retVal;
}  /* end SMS_clip */

static SMS_INLINE int SMS_mid_pred ( int anA, int aB, int aC ) {
 if ( anA > aB ) {
  if ( aC > aB ) aB = aC > anA ? anA : aC;
 } else if ( aB > aC ) aB = aC > anA ? aC : anA;
 return aB;
}  /* end SMS_mid_pred */

typedef struct SMS_LZMA2Data {
 size_t m_Size;
 size_t m_PackSize;
 const void*  m_pData;
} SMS_LZMA2Data;

struct DMAChannel;

#if __GNUC__ > 3
#define ASM_REG_ACC "$ACC"
#define ASM_REG_Q "$Q"
#define ASM_REG_I "$I"
#else
#define ASM_REG_ACC "ACC"
#define ASM_REG_Q "Q"
#define ASM_REG_I "I"
#endif
#define ASM_REG_T0 "$8"
#define ASM_REG_T1 "$9"
#define ASM_REG_T2 "$10"
#define ASM_REG_T3 "$11"
#define ASM_REG_T4 "$12"
#define ASM_REG_T5 "$13"
#define ASM_REG_T6 "$14"
#define ASM_REG_T7 "$15"
#define ASM_REG_T8 "$24"
#define ASM_REG_T9 "$25"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
void     SMS_Initialize       ( void*                                       );
void*    SMS_Realloc          ( void*, unsigned int*, unsigned int          );
uint32_t SMS_Align            ( unsigned int, unsigned int                  );
void     SMS_SetSifCmdHandler ( void ( * ) ( void* ), int                   );
int64_t  SMS_Rescale          ( int64_t, int64_t, int64_t                   );
void     SMS_StartNetwork     ( void*                                       );
void     SMS_ResetIOP         ( void                                        );
char*    SMS_ReverseString    ( char*, int                                  );
void     SMS_SetDirButtons    ( void                                        );
void     SMS_InitBitBlt       ( void*, int, int, int                        );
void*    SMS_SyncMalloc       ( int                                         );
void     SMS_SetExecPath      ( const char*                                 );
int      SMS_FileID           ( const char*                                 );
int      SMS_ContID           ( const char*                                 );
int      SMS_SubContID        ( const char*                                 );
void*    SMS_LZMA2Decompress  ( SMS_LZMA2Data*                              );
int      SMS_InitVU           ( volatile struct DMAChannel*, SMS_LZMA2Data* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_H */
