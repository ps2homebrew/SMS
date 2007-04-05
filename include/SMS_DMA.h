/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DMA_H
# define __SMS_DMA_H

typedef struct DMAChannel {

 volatile unsigned int m_CHCR __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_MADR __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_QWC  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_TADR __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_ASR0 __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_ASR1 __attribute__(   (  aligned( 16 )  )   );

} DMAChannel;

typedef struct DMACRegs {

 volatile unsigned int m_CTRL  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_STAT  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_PCR   __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_SQWC  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_RBSR  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_RBOR  __attribute__(   (  aligned( 16 )  )   );
 volatile unsigned int m_STADR __attribute__(   (  aligned( 16 )  )   );

} DMACRegs;

# define DMAC (  ( volatile DMACRegs* )0x1000E000  )

# define DMATAG_ID_REFE 0x0
# define DMATAG_ID_CNT  0x1
# define DMATAG_ID_NEXT 0x2
# define DMATAG_ID_REF  0x3
# define DMATAG_ID_REFS 0x4
# define DMATAG_ID_CALL 0x5
# define DMATAG_ID_RET  0x6
# define DMATAG_ID_END  0x7

typedef union DMATag {

 struct {

  unsigned long QWC  : 16 __attribute__ (  ( packed )  );
  unsigned long m_Pad: 10 __attribute__ (  ( packed )  );
  unsigned long PCE  :  2 __attribute__ (  ( packed )  );
  unsigned long ID   :  3 __attribute__ (  ( packed )  );
  unsigned long IRQ  :  1 __attribute__ (  ( packed )  );
  unsigned long ADDR : 31 __attribute__ (  ( packed )  );
  unsigned long SPR  :  1 __attribute__ (  ( packed )  );

 } __attribute__ (  ( packed )  );

 unsigned long m_Value __attribute__ (  ( packed )  );

} DMATag __attribute__(   (  aligned( 16 )  )   );

# define DMA_TAG( QWC, PCE, ID, IRQ, ADDR, SPR ) (                                                \
 (  ( unsigned long )(                  QWC   ) <<  0  ) | (  ( unsigned long )( PCE ) << 26  ) | \
 (  ( unsigned long )(                  ID    ) << 28  ) | (  ( unsigned long )( IRQ ) << 31  ) | \
 (  ( unsigned long )(  ( unsigned int )ADDR  ) << 32  ) | (  ( unsigned long )( SPR ) << 63  )   \
)

# define DMA_SET_CTRL( A, B, C, D, E, F )                                                        \
 ( unsigned int )( A ) << 0 | ( unsigned int )( B ) << 1 | \
 ( unsigned int )( C ) << 2 | ( unsigned int )( D ) << 4 | \
 ( unsigned int )( E ) << 6 | ( unsigned int )( F ) << 8

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

# define D_PCR_CPC_VIF0         0x00000001
# define D_PCR_CPC_VIF1         0x00000002
# define D_PCR_CPC_GIF          0x00000004
# define D_PCR_CPC_FROM_IPU     0x00000008
# define D_PCR_CPC_TO_IPU       0x00000010
# define D_PCR_CPC_FROM_SIF0    0x00000020
# define D_PCR_CPC_TO_SIF1      0x00000040
# define D_PCR_CPC_SIF2         0x00000080
# define D_PCR_CPC_FROM_SPR     0x00000100
# define D_PCR_CPC_TO_SPR       0x00000200

# define D_PCR_CDE_VIF0         0x80010000
# define D_PCR_CDE_VIF1         0x80020000
# define D_PCR_CDE_GIF          0x80040000
# define D_PCR_CDE_FROM_IPU     0x80080000
# define D_PCR_CDE_TO_IPU       0x80100000
# define D_PCR_CDE_FROM_SIF0    0x80200000
# define D_PCR_CDE_TO_SIF1      0x80400000
# define D_PCR_CDE_SIF2         0x80800000
# define D_PCR_CDE_FROM_SPR     0x81000000
# define D_PCR_CDE_TO_SPR       0x82000000

# define DMAC_VIF0      (  ( volatile DMAChannel* )0x10008000  )
# define DMAC_VIF1      (  ( volatile DMAChannel* )0x10009000  )
# define DMAC_GIF       (  ( volatile DMAChannel* )0x1000A000  )
# define DMAC_FROM_IPU  (  ( volatile DMAChannel* )0x1000B000  )
# define DMAC_TO_IPU    (  ( volatile DMAChannel* )0x1000B400  )
# define DMAC_FROM_SIF0 (  ( volatile DMAChannel* )0x1000C000  )
# define DMAC_TO_SIF1   (  ( volatile DMAChannel* )0x1000C400  )
# define DMAC_SIF2      (  ( volatile DMAChannel* )0x1000C800  )
# define DMAC_FROM_SPR  (  ( volatile DMAChannel* )0x1000D000  )
# define DMAC_TO_SPR    (  ( volatile DMAChannel* )0x1000D400  )

# define DMAC_I_VIF0      0
# define DMAC_I_VIF1      1
# define DMAC_I_GIF       2
# define DMAC_I_FROM_IPU  3
# define DMAC_I_TO_IPU    4
# define DMAC_I_FROM_SIF0 5
# define DMAC_I_TO_SIF1   6
# define DMAC_I_SIF2      7
# define DMAC_I_FROM_SPR  8
# define DMAC_I_TO_SPR    9

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

static void inline DMA_Wait ( volatile DMAChannel* apChan ) {
 while ( apChan -> m_CHCR & 0x00000100 );
}  /* end DMA_Wait */

static void inline DMA_Send ( volatile DMAChannel* apChan, void* apAddr, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "1:\n\t"
  "lw   $at, 0(%0)\n\t"
  "nop\n\t"
  "nop\n\t"
  "andi $at, $at, 0x0100\n\t"
  "nop\n\t"
  "bne  $at, $zero, 1b\n\t"
  "li   $at, 0x0101\n\t"
  "sw   %1, 16(%0)\n\t"
  "sw   %2, 32(%0)\n\t"
  "sw   $at, 0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ), "r"( aQWC ) : "at"
 );
}  /* end DMA_Send */

static void inline DMA_SendA ( volatile DMAChannel* apChan, void* apAddr, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "li   $at, 0x0101\n\t"
  "sw   %1, 16(%0)\n\t"
  "sw   %2, 32(%0)\n\t"
  "sw   $at, 0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ), "r"( aQWC ) : "at"
 );
}  /* end DMA_SendA */

static void inline DMA_SendChain ( volatile DMAChannel* apChan, void* apAddr ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "1:\n\t"
  "lw   $at, 0(%0)\n\t"
  "nop\n\t"
  "nop\n\t"
  "andi $at, $at, 0x0100\n\t"
  "nop\n\t"
  "bne  $at, $zero, 1b\n\t"
  "li   $at, 0x0105\n\t"
  "sw   $zero, 32(%0)\n\t"
  "sw   %1,    48(%0)\n\t"
  "sw   $at,    0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ) : "at"
 );
}  /* end DMA_SendChain */

static void inline DMA_SendChainA ( volatile DMAChannel* apChan, void* apAddr ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "li   $at, 0x0105\n\t"
  "sw   $zero, 32(%0)\n\t"
  "sw   %1,    48(%0)\n\t"
  "sw   $at,    0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ) : "at"
 );
}  /* end DMA_SendChainA */

static void inline DMA_SendChainT ( volatile DMAChannel* apChan, void* apAddr ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "1:\n\t"
  "lw   $at, 0(%0)\n\t"
  "nop\n\t"
  "nop\n\t"
  "andi $at, $at, 0x0100\n\t"
  "nop\n\t"
  "bne  $at, $zero, 1b\n\t"
  "li   $at, 0x0145\n\t"
  "sw   $zero, 32(%0)\n\t"
  "sw   %1,    48(%0)\n\t"
  "sw   $at,    0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ) : "at"
 );
}  /* end DMA_SendChainT */

static void inline DMA_RecvA ( volatile DMAChannel* apChan, void* apAddr, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "li   $at, 0x0100\n\t"
  "sw   %1, 16(%0)\n\t"
  "sw   %2, 32(%0)\n\t"
  "sw   $at, 0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apAddr ), "r"( aQWC ) : "at"
 );
}  /* end DMA_RecvA */

static void inline DMA_RecvS ( volatile DMAChannel* apChan, void* apDst, void* apSrc, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "li   $at, 0x0101\n\t"
  "sw   %2,  16(%0)\n\t"
  "sw   %3,  32(%0)\n\t"
  "sw   %1, 128(%0)\n\t"
  "sw   $at,  0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apDst ), "r"( apSrc ), "r"( aQWC ) : "at"
 );

}  /* end DMA_RecvS */

static void inline DMA_SendS ( volatile DMAChannel* apChan, void* apDst, void* apSrc, unsigned int aQWC ) {
 __asm__ __volatile__(
  ".set noreorder\n\t"
  ".set noat\n\t"
  "li   $at, 0x0100\n\t"
  "sw   %1,  16(%0)\n\t"
  "sw   %3,  32(%0)\n\t"
  "sw   %2, 128(%0)\n\t"
  "sw   $at,  0(%0)\n\t"
  ".set at\n\t"
  ".set reorder\n\t"
  ::"r"( apChan ), "r"( apDst ), "r"( apSrc ), "r"( aQWC ) : "at"
 );
}  /* end DMA_SendS */

void DMA_Stop ( volatile DMAChannel* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_DMA_H */
