/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright 2004 - Chris "Neovanglist" Gilbert <Neovanglist@LainOS.org>
#           2005 - Adopted for SMS by Eugene Plotnikov
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __DMA_H
# define __DMA_H

# define DMA_ADDR( a ) (  ( volatile unsigned int* )( a )  )

typedef enum DMADirection {
 DMADirection_To_Mem   = 0,
 DMADirection_From_Mem = 1
} DMADirection;

typedef enum DMATransferMode {
 DMATransferMode_Normal     = 0,
 DMATransferMode_Chain      = 1,
 DMATransferMode_Interleave = 2
} DMATransferMode;

# define DMA_ASP_NO_PUSH 0
# define DMA_ASP_1_PUSH  1
# define DMA_ASP_2_PUSH  2

# define DMA_STOP  0
# define DMA_START 1

# define DMA_SET_CHCR( DIR, MODE, ASP, TTE, TIE, STR, TAG )                                             \
 ( unsigned int )(  ( DIR ) & 0x00000001  ) <<  0 | ( unsigned int )(  ( MODE ) & 0x00000003  ) <<  2 | \
 ( unsigned int )(  ( ASP ) & 0x00000003  ) <<  4 | ( unsigned int )(  ( TTE  ) & 0x00000001  ) <<  6 | \
 ( unsigned int )(  ( TIE ) & 0x00000001  ) <<  7 | ( unsigned int )(  ( STR  ) & 0x00000001  ) <<  8 | \
 ( unsigned int )(  ( TAG ) & 0x0000FFFF  ) << 16

# define DMA_SET_MADR( ADR, SPR ) \
 ( unsigned int )(  ( ADR ) & 0x7FFFFFFF  ) <<  0 | \
 ( unsigned int )(  ( SPR ) & 0x00000001  ) << 31

# define DMA_SET_TADR( A, B ) \
 ( unsigned int )(  ( A ) & 0x7FFFFFFF  ) <<  0 | \
 ( unsigned int )(  ( B ) & 0x00000001  ) << 31

# define DMA_SET_ASR0( A, B ) \
 ( unsigned int )(  ( A ) & 0x7FFFFFFF  ) <<  0 | \
 ( unsigned int )(  ( B ) & 0x00000001  ) << 31

# define DMA_SET_ASR1( A, B ) \
 ( unsigned int )(  ( A ) & 0x7FFFFFFF  ) <<  0 | \
 ( unsigned int )(  ( B ) & 0x00000001  ) << 31

# define DMA_SET_SADR( A ) \
 ( unsigned int )(  ( A ) & 0x00003FFF  ) << 0

# define DMA_SET_SIZE( A ) \
 ( unsigned int )(  ( A ) & 0x0000FFFF  ) << 0

# define DMA_SET_SQWC( T, S ) \
 ( unsigned int )(  ( T ) & 0x000000FF  ) << 16 | ( unsigned int )(  ( S ) & 0x000000FF  )

# define DMA_MAX_SIZE 0xFFFF

# define DMA_REFE 0x0
# define DMA_CNT  0x1
# define DMA_NEXT 0x2
# define DMA_REF  0x3
# define DMA_REFS 0x4
# define DMA_CALL 0x5
# define DMA_RET  0x6
# define DMA_END  0x7

# define DMA_TAG( QWC, PCE, ID, IRQ, ADDR, SPR ) (                                      \
 (  ( unsigned long int )( QWC  ) <<  0  ) | (  ( unsigned long int )( PCE ) << 26  ) | \
 (  ( unsigned long int )( ID   ) << 28  ) | (  ( unsigned long int )( IRQ ) << 31  ) | \
 (  ( unsigned long int )( ADDR ) << 32  ) | (  ( unsigned long int )( SPR ) << 63  )   \
)

# define DMA_SET_CTRL( A, B, C, D, E, F )                                                        \
 ( unsigned int )(  ( A ) & 0x00000001  ) << 0 | ( unsigned int )(  ( B ) & 0x00000001  ) << 1 | \
 ( unsigned int )(  ( C ) & 0x00000003  ) << 2 | ( unsigned int )(  ( D ) & 0x00000003  ) << 4 | \
 ( unsigned int )(  ( E ) & 0x00000003  ) << 6 | ( unsigned int )(  ( F ) & 0x00000007  ) << 8

# define DMA_CHANNEL_VIF0     0
# define DMA_CHANNEL_VIF1     1
# define DMA_CHANNEL_GIF      2
# define DMA_CHANNEL_FROM_IPU 3
# define DMA_CHANNEL_TO_IPU   4
# define DMA_CHANNEL_SIF0     5
# define DMA_CHANNEL_SIF1     6
# define DMA_CHANNEL_SIF2     7
# define DMA_CHANNEL_FROM_SPR 8
# define DMA_CHANNEL_TO_SPR   9

# define DMA_REG_CTRL (  ( volatile unsigned int* )0x1000E000  )
# define DMA_REG_STAT (  ( volatile unsigned int* )0x1000E010  )
# define DMA_REG_PCR  (  ( volatile unsigned int* )0x1000E020  )
# define DMA_REG_SQWC (  ( volatile unsigned int* )0x1000E030  )
# define DMA_REG_RBSR (  ( volatile unsigned int* )0x1000E040  )
# define DMA_REG_RBOR (  ( volatile unsigned int* )0x1000E050  )

# define D_CTRL_RELE_OFF 0
# define D_CTRL_RELE_ON  1

# define D_CTRL_MFD_OFF 0
# define D_CTRL_MFD_RES 1
# define D_CTRL_MFD_VIF 2
# define D_CTRL_MFD_GIF 3

# define D_CTRL_STS_UNSPEC 0
# define D_CTRL_STS_SIF    1
# define D_CTRL_STS_SPR    2
# define D_CTRL_STS_IPU    3

# define D_CTRL_STD_OFF 0
# define D_CTRL_STD_VIF 1
# define D_CTRL_STD_GIF 2
# define D_CTRL_STD_SIF 3

# define D_CTRL_RCYC_8   0
# define D_CTRL_RCYC_16  1
# define D_CTRL_RCYC_32  2
# define D_CTRL_RCYC_64  3
# define D_CTRL_RCYC_128 4
# define D_CTRL_RCYC_256 5

# define DMA_RecvSPRi( dst, src, size, line, stride )                    \
 *DMA_REG_SQWC           = DMA_SET_SQWC(  ( line ), ( stride )        ); \
 *DMA_ADDR( 0x1000D480 ) = DMA_SET_SADR(  ( unsigned int )( dst )     ); \
 *DMA_ADDR( 0x1000D410 ) = DMA_SET_MADR(  ( unsigned int )( src ), 0  ); \
 *DMA_ADDR( 0x1000D420 ) = ( size );                                     \
 *DMA_ADDR( 0x1000D400 ) = DMA_SET_CHCR(                                 \
   DMADirection_From_Mem, DMATransferMode_Interleave, 0, 0, 0, 1, 0      \
  )

# define DMA_RecvSPR( dst, src, size )                                   \
 *DMA_ADDR( 0x1000D480 ) = DMA_SET_SADR(  ( unsigned int )( dst )     ); \
 *DMA_ADDR( 0x1000D410 ) = DMA_SET_MADR(  ( unsigned int )( src ), 0  ); \
 *DMA_ADDR( 0x1000D420 ) = ( size );                                     \
 *DMA_ADDR( 0x1000D400 ) = DMA_SET_CHCR(                                 \
   DMADirection_From_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0          \
  )

# define DMA_SendSPRToMem( dst, src, size )                              \
 *DMA_ADDR( 0x1000D080 ) = DMA_SET_SADR(  ( unsigned int )( src )     ); \
 *DMA_ADDR( 0x1000D010 ) = DMA_SET_MADR(  ( unsigned int )( dst ), 0  ); \
 *DMA_ADDR( 0x1000D020 ) = ( size );                                     \
 *DMA_ADDR( 0x1000D000 ) = DMA_SET_CHCR(                                 \
   DMADirection_To_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0            \
  )

# define DMA_SendToGIF( data, size )                            \
 *DMA_ADDR( 0x1000A020 ) = ( size );                            \
 *DMA_ADDR( 0x1000A010 ) = ( unsigned int )( data );            \
 *DMA_ADDR( 0x1000A000 ) = DMA_SET_CHCR(                        \
   DMADirection_From_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0 \
  )

# define DMA_SendChainToGIF( data, size )                      \
 *DMA_ADDR( 0x1000A020 ) = 0;                                  \
 *DMA_ADDR( 0x1000A030 ) = ( unsigned int )( data );           \
 *DMA_ADDR( 0x1000A000 ) = DMA_SET_CHCR(                       \
   DMADirection_From_Mem, DMATransferMode_Chain, 0, 0, 0, 1, 0 \
  )

# define DMA_SendChainToGIF_SPR( data, size )                           \
 *DMA_ADDR( 0x1000A020 ) = 0;                                           \
 *DMA_ADDR( 0x1000A030 ) = (  ( unsigned int )( data )  ) | 0x80000000; \
 *DMA_ADDR( 0x1000A000 ) = DMA_SET_CHCR(                                \
   DMADirection_From_Mem, DMATransferMode_Chain, 0, 0, 0, 1, 0          \
  )

# define DMA_SendSPRToIPU( data, size )                              \
 *DMA_ADDR( 0x1000B410 ) = DMA_SET_MADR(  ( unsigned int )( data ), 1  ); \
 *DMA_ADDR( 0x1000B420 ) = ( size );                                      \
 *DMA_ADDR( 0x1000B400 ) = DMA_SET_CHCR( 0, 0, 0, 0, 0, 1, 0 )

# define DMA_SendToIPU( data, size )                                      \
 *DMA_ADDR( 0x1000B410 ) = DMA_SET_MADR(  ( unsigned int )( data ), 0  ); \
 *DMA_ADDR( 0x1000B420 ) = ( size );                                      \
 *DMA_ADDR( 0x1000B400 ) = DMA_SET_CHCR( 0, 0, 0, 0, 0, 1, 0 )

# define DMA_RecvFromIPU( data, size )                        \
 *DMA_ADDR( 0x1000B020 ) = ( size );                          \
 *DMA_ADDR( 0x1000B010 ) = ( unsigned int )( data );          \
 *DMA_ADDR( 0x1000B000 ) = DMA_SET_CHCR(                      \
   DMADirection_To_Mem, DMATransferMode_Normal, 0, 0, 0, 1, 0 \
  );

# define DMA_WaitIPU()   while (   *DMA_ADDR( 0x1000B000 ) & 0x00000100   );
# define DMA_WaitGIF()   while (   *DMA_ADDR( 0x1000A000 ) & 0x00000100   );
# define DMA_WaitToSPR() while (   *DMA_ADDR( 0x1000D400 ) & 0x00000100   );

extern const unsigned int DMA_CHCR[ 10 ];
extern const unsigned int DMA_MADR[ 10 ];
extern const unsigned int DMA_SIZE[ 10 ];
extern const unsigned int DMA_TADR[ 10 ];
extern const unsigned int DMA_ASR0[ 10 ];
extern const unsigned int DMA_ASR1[ 10 ];
extern const unsigned int DMA_SADR[ 10 ];
extern const unsigned int DMA_QWC [ 10 ];

void DMA_Init        ( unsigned int, unsigned int, unsigned int, unsigned int, unsigned int );
void DMA_ChannelInit ( int                                                                  );
void DMA_Wait        ( int                                                                  );
void DMA_Send        ( int, unsigned long int*, int                                         );
void DMA_Recv        ( int, unsigned long int*, int                                         );
void DMA_SendChain   ( int, unsigned long int*, int                                         );
void DMA_SendSPR     ( int, unsigned char*, int                                             );
#endif  /* __DMA_H */
