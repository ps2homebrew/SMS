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
#ifndef __SMS_H
# define __SMS_H

extern int  g_Trace;
extern int  g_SMSFlags;

# define SMS_FLAG_DEV9 0x00000001
# define SMS_FLAG_USB  0x00000002
# define SMS_FLAG_NET  0x00000004

typedef signed   char  int8_t;
typedef unsigned char uint8_t;

typedef signed   short  int16_t;
typedef unsigned short uint16_t;

typedef signed   int  int32_t;
typedef unsigned int uint32_t;

typedef short SMS_DCTELEM;

extern const uint32_t g_SMS_InvTbl[ 256 ];

# define SMS_AUDSRV_SIZE      5365
# define SMS_IDCT_CONST_SIZE   368
# define SMS_IOMANX_SIZE      7417
# define SMS_FILEXIO_SIZE     8089
# define SMS_PS2DEV9_SIZE     9905
# define SMS_PS2ATAD_SIZE    11805
# define SMS_PS2HDD_SIZE     26257
# define SMS_PS2FS_SIZE      54785
# define SMS_POWEROFF_SIZE    2925
# define SMS_USB_MASS_SIZE   33413
# define SMS_CDVD_SIZE       20065
# define SMS_PS2IP_SIZE      78909
# define SMS_PS2SMAP_SIZE    12625
# define SMS_PS2HOST_SIZE    16349

# define SMS_AUDSRV_OFFSET     0
# define SMS_IDCT_CONST_OFFSET (  ( SMS_AUDSRV_OFFSET     + SMS_AUDSRV_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_IOMANX_OFFSET     (  ( SMS_IDCT_CONST_OFFSET + SMS_IDCT_CONST_SIZE + 15 ) & 0xFFFFFFF0  )
# define SMS_FILEXIO_OFFSET    (  ( SMS_IOMANX_OFFSET     + SMS_IOMANX_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2DEV9_OFFSET    (  ( SMS_FILEXIO_OFFSET    + SMS_FILEXIO_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2ATAD_OFFSET    (  ( SMS_PS2DEV9_OFFSET    + SMS_PS2DEV9_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2HDD_OFFSET     (  ( SMS_PS2ATAD_OFFSET    + SMS_PS2ATAD_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2FS_OFFSET      (  ( SMS_PS2HDD_OFFSET     + SMS_PS2HDD_SIZE     + 15 ) & 0xFFFFFFF0  )
# define SMS_POWEROFF_OFFSET   (  ( SMS_PS2FS_OFFSET      + SMS_PS2FS_SIZE      + 15 ) & 0xFFFFFFF0  )
# define SMS_USB_MASS_OFFSET   (  ( SMS_POWEROFF_OFFSET   + SMS_POWEROFF_SIZE   + 15 ) & 0xFFFFFFF0  )
# define SMS_CDVD_OFFSET       (  ( SMS_USB_MASS_OFFSET   + SMS_USB_MASS_SIZE   + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2IP_OFFSET      (  ( SMS_CDVD_OFFSET       + SMS_CDVD_SIZE       + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2SMAP_OFFSET    (  ( SMS_PS2IP_OFFSET      + SMS_PS2IP_SIZE      + 15 ) & 0xFFFFFFF0  )
# define SMS_PS2HOST_OFFSET    (  ( SMS_PS2SMAP_OFFSET    + SMS_PS2SMAP_SIZE    + 15 ) & 0xFFFFFFF0  )
# define SMS_DATA_BUFFER_SIZE  (  ( SMS_PS2HOST_OFFSET    + SMS_PS2HOST_SIZE    + 15 ) & 0xFFFFFFF0  )

extern uint8_t g_DataBuffer[ SMS_DATA_BUFFER_SIZE ];

# define SMS_FT_I_TYPE 1
# define SMS_FT_P_TYPE 2
# define SMS_FT_B_TYPE 3
# define SMS_FT_S_TYPE 4

# define SMS_ROUND( v ) (    ( int )(  ( v ) + (  ( v ) > 0 ? 0.5F : -0.5F  )   )    )

# define SMS_FASTDIV( a, b ) (     ( uint32_t )(    (   (  ( uint64_t )a  ) * g_SMS_InvTbl[ b ]   ) >> 32    )     )

# define SMS_MAX( a, b ) (  ( a ) > ( b ) ? ( a ) : ( b )  )
# define SMS_MIN( a, b ) (  ( a ) > ( b ) ? ( b ) : ( a )  )

# define SMS_ROUNDED_DIV( a, b ) (   (  ( a ) > 0 ? ( a ) + (  ( b ) >> 1  ) : ( a ) - (  ( b ) >> 1  )  ) / ( b )   )

# define SMS_RSHIFT( a, b ) (     ( a ) > 0 ? (    ( a ) + (   (  1 << ( b )  ) >> 1   )    ) >> ( b ) : (    ( a ) + (   (  1 << ( b )  ) >> 1   ) - 1    ) >> ( b )     )

# define SMS_NEG_USR32( a, s ) (   (  ( uint32_t )( a )  ) >> (  32 - ( s )  )   )
# define SMS_NEG_SSR32( a, s ) (   (  ( int32_t  )( a )  ) >> (  32 - ( s )  )   )

# define SMS_BUG_AUTODETECT       0x00000001
# define SMS_BUG_OLD_MSMPEG4      0x00000002
# define SMS_BUG_XVID_ILACE       0x00000004
# define SMS_BUG_UMP4             0x00000008
# define SMS_BUG_NO_PADDING       0x00000010
# define SMS_BUG_AMV              0x00000020
# define SMS_BUG_QPEL_CHROMA      0x00000040
# define SMS_BUG_STD_QPEL         0x00000080
# define SMS_BUG_QPEL_CHROMA2     0x00000100
# define SMS_BUG_DIRECT_BLOCKSIZE 0x00000200
# define SMS_BUG_EDGE             0x00000400
# define SMS_BUG_HPEL_CHROMA      0x00000800
# define SMS_BUG_DC_CLIP          0x00001000

# define SMS_PKT_FLAG_KEY 0x00000001

# define SMS_RB_CREATE( rb, type ) \
 struct {                          \
  type* m_pBeg;                    \
  type* m_pEnd;                    \
  type* m_pInp;                    \
  type* m_pOut;                    \
 } rb

# define SMS_RB_INIT( rb, start, count )  \
 ( rb ).m_pInp  =                         \
 ( rb ).m_pOut  =                         \
 ( rb ).m_pBeg  = start;                  \
 ( rb ).m_pEnd  = &( rb ).m_pBeg[ count ]

# define SMS_RB_SLOT( rb, slot )  (  ( slot ) == ( rb ).m_pEnd ? ( rb ).m_pBeg : ( slot )  )
# define SMS_RB_EMPTY( rb )       (  ( rb ).m_pInp == ( rb ).m_pOut  )
# define SMS_RB_FULL( rb )        (   SMS_RB_SLOT(  rb, ( rb ).m_pInp + 1  ) == ( rb ).m_pOut   )
# define SMS_RB_PUSHSLOT( rb )    (  ( rb ).m_pInp  )
# define SMS_RB_POPSLOT( rb )     (  ( rb ).m_pOut  )
# define SMS_RB_PUSHADVANCE( rb ) (   ( rb ).m_pInp = SMS_RB_SLOT(  ( rb ), ( rb ).m_pInp + 1  )   )
# define SMS_RB_POPADVANCE( rb )  (   ( rb ).m_pOut = SMS_RB_SLOT(  ( rb ), ( rb ).m_pOut + 1  )   )

# ifndef NULL
#  define NULL (  ( void* )0  )
# endif  /* NULL */

# ifdef _WIN32

#  include <sys/timeb.h>

typedef signed   __int64  int64_t;
typedef unsigned __int64 uint64_t;

typedef struct u128 {

 uint64_t m_Low;
 uint64_t m_High;

} u128;

#  define SMS_INT64( c )    c##i64
#  define SMS_INLINE        __inline
#  define SMS_ALIGN( d, a ) __declspec(  align( a )  ) d
#  define SMS_DATA_SECTION

#  pragma warning( disable: 4035 )
static __inline uint32_t SMS_bswap32 ( uint32_t aVal ) {
 __asm mov   eax, [ aVal ]
 __asm bswap eax
}  /* end SMS_bswap32 */

static __inline int32_t SMS_ModP2 ( int32_t aVal, uint32_t aDiv ) {
 __asm mov  eax, [ aVal ]
 __asm cdq
 __asm and  edx, [ aDiv ]
 __asm add  eax, edx
 __asm and  eax, [ aDiv ]
 __asm sub  eax, edx
}  /* end SMS_ModP2 */
#  pragma warning( default: 4035 )

static SMS_INLINE int SMS_log2 ( unsigned int aVal ) {
 int retVal = 31;
 if ( !aVal ) return 0;
 while (  !( aVal & 0x80000000 )  ) {
  aVal <<= 1;
  --retVal;
 }  /* end while */
 return retVal;
}  /* end SMS_log2 */

static __inline uint32_t SMS_unaligned32 ( const void* apData ) {
 return *( const uint32_t* )apData;
}  /* end SMS_unaligned32 */

static __inline uint64_t SMS_unaligned64 ( const void* apData ) {
 return *( const uint64_t* )apData;
}  /* end SMS_unaligned64 */

static SMS_INLINE uint64_t SMS_Time ( void ) {
 struct _timeb lTime; _ftime ( &lTime );
 return ( uint64_t )lTime.time * 1000i64 + ( uint64_t )lTime.millitm;
}  /* end SMS_Time */
# else  /* PS2 */

typedef signed   long int  int64_t;
typedef unsigned long int uint64_t;
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
#  define _U( p )           (    ( uint64_t* )(   (  ( uint32_t )( p )  ) | 0x20000000   )    )

#  define SMS_MPEG_SPR_DMA_MB_BUF  (  ( uint8_t* )0  )
#  define SMS_MPEG_SPR_DMA_Y_BUF   ( SMS_MPEG_SPR_DMA_MB_BUF + 1536 )
#  define SMS_MPEG_SPR_DMA_Cb_BUF  ( SMS_MPEG_SPR_DMA_Y_BUF  + 4096 )
#  define SMS_MPEG_SPR_DMA_Cr_BUF  ( SMS_MPEG_SPR_DMA_Cb_BUF + 1024 )
#  define SMS_MPEG_SPR_DMA_BLOCKS  ( SMS_MPEG_SPR_DMA_Cr_BUF + 1024 )
#  define SMS_DSP_SPR_DMA_FULL     ( SMS_MPEG_SPR_DMA_BLOCKS +  768 )
#  define SMS_DSP_SPR_DMA_HALF     ( SMS_DSP_SPR_DMA_FULL    +  416 )
#  define SMS_DSP_SPR_DMA_HALF_HV  ( SMS_DSP_SPR_HALF        +  272 )
#  define SMS_MPEG_SPR_DMA_MB_0    ( SMS_DSP_SPR_HALF_HV     +  256 )
#  define SMS_MPEG_SPR_DMA_MB_1    ( SMS_MPEG_SPR_DMA_MB_0   +  384 )
#  define SMS_DSP_SPR_DMA_CONST    ( SMS_MPEG_SPR_DMA_MB_1   +  384 )
#  define SMS_SPR_DMA_FREE         ( SMS_DSP_SPR_DMA_CONST   +  368 )

#  define SMS_MPEG_SPR_MB_BUF (  ( SMS_MacroBlock* )0x70000000  )
#  define SMS_MPEG_SPR_Y_BUF  (  ( uint128_t*      )( SMS_MPEG_SPR_MB_BUF +   4 )  )
#  define SMS_MPEG_SPR_Cb_BUF (  ( uint64_t*       )( SMS_MPEG_SPR_Y_BUF  + 256 )  )
#  define SMS_MPEG_SPR_Cr_BUF (  ( uint64_t*       )( SMS_MPEG_SPR_Cb_BUF + 128 )  )
#  define SMS_MPEG_SPR_BLOCKS (  ( SMS_DCTELEM*    )( SMS_MPEG_SPR_Cr_BUF + 128 )  )
#  define SMS_DSP_SPR_FULL    (  ( uint8_t*        )( SMS_MPEG_SPR_BLOCKS + 384 )  )
#  define SMS_DSP_SPR_HALF    (  ( uint8_t*        )( SMS_DSP_SPR_FULL    + 416 )  )
#  define SMS_DSP_SPR_HALF_HV (  ( uint8_t*        )( SMS_DSP_SPR_HALF    + 272 )  )
#  define SMS_MPEG_SPR_MB_0   (  ( SMS_MacroBlock* )( SMS_DSP_SPR_HALF_HV + 256 )  )
#  define SMS_MPEG_SPR_MB_1   (  ( SMS_MacroBlock* )( SMS_MPEG_SPR_MB_0   +   1 )  )
#  define SMS_DSP_SPR_CONST   (  ( uint16_t*       )( SMS_MPEG_SPR_MB_1   +   1 )  )
#  define SMS_SPR_FREE        (  ( uint8_t*        )( SMS_DSP_SPR_CONST   + 184 )  )

static inline uint32_t SMS_bswap32 ( uint32_t aVal ) {
 uint32_t retVal;
 __asm__ __volatile__ (
  "pextlb   %1, %1, %1\n\t"
  "prevh    %1, %1\n\t"
  "ppacb    %0, %1, %1\n\t"
  : "=r"( retVal ) : "r"( aVal )
 );
 return retVal;
}  /* end SMS_bswap32 */

static inline int SMS_ModP2 ( int aVal, unsigned int aDiv ) {
 int retVal;
 __asm__ __volatile__ (
  "dsll      $v1,  %1,  31\n\t"
  "dsra32    $v1, $v1,  31\n\t"
  "and       $v1, $v1,  %2\n\t"
  "addu       %0,  %1, $v1\n\t"
  "and        %0,  %0,  %2\n\t"
  "subu       %0,  %0, $v1\n\t"
  : "=r"( retVal ) : "r"( aVal ), "r"( aDiv ) : "$3"
 );
 return retVal;
}  /* end SMS_ModP2 */

static SMS_INLINE int SMS_log2 ( unsigned int aVal ) {
 int retVal;
 __asm__ __volatile__ (
  "srl      $t0, %1, 1\n\t"		
  "addiu    $t1, $zero, 31\n\t"
  "plzcw    $t0, $t0\n\t"
  "subu     %0, $t1, $t0\n\t"
  : "=r"( retVal ) : "r"( aVal ) : "t0", "t1"
 );
 return retVal;
}  /* end SMS_log2 */ 

static inline uint32_t SMS_unaligned32 ( const void* apData ) {
 return (  ( const SMS_Unaligned32* )apData  ) -> m_Val;
}  /* end SMS_unaligned32 */

static inline uint64_t SMS_unaligned64 ( const void* apData ) {
 return (  ( const SMS_Unaligned64* )apData  ) -> m_Val;
}  /* end SMS_unaligned64 */

static SMS_INLINE uint64_t SMS_Time ( void ) {
 volatile extern uint64_t g_Timer;
 return g_Timer;
}  /* end SMS_Time */
# endif  /* _WIN32 */

# define SMS_MAXINT64 SMS_INT64( 0x7FFFFFFFFFFFFFFF )
# define SMS_MININT64 SMS_INT64( 0x8000000000000000 )

# define SMS_NOPTS_VALUE SMS_INT64( 0x8000000000000000 )
# define SMS_STPTS_VALUE SMS_INT64( 0xC000000000000000 )
# define SMS_TIME_BASE   1000

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
 int64_t                 m_PTS;
 int8_t*                 m_pRefIdx[ 2 ];
 int                     m_Width;
 int                     m_Height;
 int                     m_Type;
 int                     m_KeyFrame;
 int                     m_Ref;
 int                     m_CodedPicNr;
 int                     m_Linesize;
 int                     m_Age;
 struct SMS_FrameBuffer* m_pBuf;
 uint32_t*               m_pMBType;
 uint8_t*                m_pMBSkipTbl;
 int8_t*                 m_pQScaleTbl;
 uint8_t                 m_MotionSubsampleLog2;

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
 void*    m_pCtx;
 void     ( *Alloc   ) ( struct SMS_AVPacket*, int );
 void     ( *Destroy ) ( struct SMS_AVPacket*      );

} SMS_AVPacket;

static SMS_INLINE int SMS_clip ( int aVal, int aMin, int aMax ) {
 if ( aVal < aMin )
  return aMin;
 else if ( aVal > aMax )
  return aMax;
 else return aVal;
}  /* end SMS_clip */

static SMS_INLINE int SMS_mid_pred ( int anA, int aB, int aC ) {
 if ( anA > aB ) {
  if ( aC > aB ) aB = aC > anA ? anA : aC;
 } else if ( aB > aC ) aB = aC > anA ? aC : anA;
 return aB;
}  /* end SMS_mid_pred */

extern char g_CWD[ 1024 ] SMS_DATA_SECTION;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */
void     SMS_Initialize       ( void*                                );
uint32_t SMS_Linesize         ( unsigned int, unsigned int*          );
void*    SMS_Realloc          ( void*, unsigned int*, unsigned int   );
uint32_t SMS_Align            ( unsigned int, unsigned int           );
void     SMS_SetSifCmdHandler ( void ( * ) ( void* ), int            );
int64_t  SMS_Rescale          ( int64_t, int64_t, int64_t            );
void     SMS_StartNetwork     ( void*                                );
void     SMS_ResetIOP         ( void                                 );
# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_H */
