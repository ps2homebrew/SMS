//---------------------------------------------------------------------------
// File:	gif.h
// Author:	Tony Saveski, t_saveski@yahoo.com
// Notes:	Macros to create and populate DMA buffers to send to the PS2 GIF
//---------------------------------------------------------------------------
#ifndef GIF_H
#define GIF_H

#include "../defines.h"
#include "regs.h"

//---------------------------------------------------------------------------
#define GIF_AD		0x0e
#define GIF_NOP		0x0f

//---------------------------------------------------------------------------
// GS_PACKET macros
//---------------------------------------------------------------------------

#define DECLARE_GS_PACKET(NAME,ITEMS) \
	uint64 __attribute__((aligned(64))) NAME[ITEMS*2+2]; \
	int NAME##_cur; \
	int NAME##_dma_size

#define DECLARE_EXTERN_GS_PACKET(NAME) \
	extern uint64 __attribute__((aligned(64))) NAME[]; \
	extern int NAME##_cur; \
	extern int NAME##_dma_size

#define BEGIN_GS_PACKET(NAME) \
	NAME##_cur = 0

#define GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,NREG,REGS) \
	NAME##_dma_size = NLOOP+1; \
	NAME[NAME##_cur++] = \
		((uint64)(NLOOP)<< 0)		| \
		((uint64)(EOP)	<< 15)		| \
		((uint64)(PRE)	<< 46)		| \
		((uint64)(PRIM)	<< 47)		| \
		((uint64)(FLG)	<< 58)		| \
		((uint64)(NREG)	<< 60);		\
	NAME[NAME##_cur++] = (uint64)REGS

#define GIF_TAG_AD(NAME,NLOOP,EOP,PRE,PRIM,FLG) \
	GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,1,GIF_AD)

#define GIF_TAG_IMG(NAME,QSIZE) \
	GIF_TAG(NAME,(QSIZE),1,0,0,2,0,0); \
	NAME##_dma_size = 1 \

#define GIF_DATA_AD(NAME,REG,DAT) \
	NAME[NAME##_cur++] = (uint64)DAT; \
	NAME[NAME##_cur++] = (uint64)REG

#define SEND_GS_PACKET(NAME) \
	ps2_flush_cache(0);							\
	SET_QWC(GIF_QWC, NAME##_dma_size);			\
	SET_MADR(GIF_MADR, NAME, 0);				\
	SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);	\
	DMA_WAIT(GIF_CHCR)

#endif // GIF_H

