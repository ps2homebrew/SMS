//---------------------------------------------------------------------------
// File:	dma.h
// Author:	Tony Saveski, t_saveski@yahoo.com
// Notes:	Playstation2 DMA Convenience Routines and Macros
//---------------------------------------------------------------------------
// * Data is transfered in qword (128bit, 16bytes) units
// * Data should be aligned on qword boundary (.align 7, aligned(16))
//---------------------------------------------------------------------------
#ifndef DMA_H
#define DMA_H

#include "regs.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void dma_reset(void);

#ifdef __cplusplus
}
#endif

//---------------------------------------------------------------------------
// CHCR Register - Channel Control Register
//---------------------------------------------------------------------------
#define GIF_CHCR		((volatile u32 *)(gif_chcr))
#define VIF1_CHCR		((volatile u32 *)(vif1_chcr))

#define SET_CHCR(WHICH,DIR,MOD,ASP,TTE,TIE,STR,TAG) \
	*WHICH = \
	((u32)(DIR)	<< 0)		| \
	((u32)(MOD)	<< 2)		| \
	((u32)(ASP)	<< 4)		| \
	((u32)(TTE)	<< 6)		| \
	((u32)(TIE)	<< 7)		| \
	((u32)(STR)	<< 8)		| \
	((u32)(TAG)	<< 16)

#define DMA_WAIT(WHICH) \
	while((*WHICH) & (1<<8))

//---------------------------------------------------------------------------
// MADR Register - Transfer Address Register
//---------------------------------------------------------------------------
#define GIF_MADR		((volatile u32 *)(gif_madr))
#define VIF1_MADR		((volatile u32 *)(vif1_madr))

#define SET_MADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((u32)(ADDR)	<< 0)		| \
	((u32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// TADR Register - Tag Address Register
//---------------------------------------------------------------------------
#define GIF_TADR		((volatile u32 *)(gif_tadr))
#define VIF1_TADR		((volatile u32 *)(vif1_tadr))

#define SET_TADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((u32)(ADDR)	<< 0)		| \
	((u32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// QWC Register - Transfer Data Size Register
//---------------------------------------------------------------------------
#define GIF_QWC		((volatile u32 *)(gif_qwc))
#define VIF1_QWC	((volatile u32 *)(vif1_qwc))

#define SET_QWC(WHICH,SIZE) \
	*WHICH = (u32)(SIZE)

enum 

  {
    REFE = 0,
    CNT = 1,
    NEXT = 2, 
    REF = 3,
    REFS = 4,
    CALL = 5,
    RET = 6,
    END = 7
  } chain_tags;

#define CHAIN_TAG(QWC, PCE, ID, IRQ, ADDR, SPR) (u64) (QWC) | ((u64) (PCE) << 26) | ((u64) (ID) << 28) | ((u64) (IRQ) << 31) | ((u64) (ADDR) << 32) | ((u64) (SPR) << 63)

#define CTAG(QWC, ID) (((u32) (QWC)) | ((u32) (ID) << 28))
#endif // DMA_H

