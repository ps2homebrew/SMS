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

#include "../defines.h"
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
#define GIF_CHCR		((volatile uint32 *)(gif_chcr))

#define SET_CHCR(WHICH,DIR,MOD,ASP,TTE,TIE,STR,TAG) \
	*WHICH = \
	((uint32)(DIR)	<< 0)		| \
	((uint32)(MOD)	<< 2)		| \
	((uint32)(ASP)	<< 4)		| \
	((uint32)(TTE)	<< 6)		| \
	((uint32)(TIE)	<< 7)		| \
	((uint32)(STR)	<< 8)		| \
	((uint32)(TAG)	<< 16)

#define DMA_WAIT(WHICH) \
	while((*WHICH) & (1<<8))

//---------------------------------------------------------------------------
// MADR Register - Transfer Address Register
//---------------------------------------------------------------------------
#define GIF_MADR		((volatile uint32 *)(gif_madr))

#define SET_MADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((uint32)(ADDR)	<< 0)		| \
	((uint32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// TADR Register - Tag Address Register
//---------------------------------------------------------------------------
#define GIF_TADR		((volatile uint32 *)(gif_tadr))

#define SET_TADR(WHICH,ADDR,SPR) \
	*WHICH = \
	((uint32)(ADDR)	<< 0)		| \
	((uint32)(SPR)	<< 31)

//---------------------------------------------------------------------------
// QWC Register - Transfer Data Size Register
//---------------------------------------------------------------------------
#define GIF_QWC		((volatile uint32 *)(gif_qwc))

#define SET_QWC(WHICH,SIZE) \
	*WHICH = (uint32)(SIZE)

#endif // DMA_H

