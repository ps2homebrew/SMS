/* Defines and data to handle GIF TAGS */
/* Borrowed from Tony Saveski (t_saveski@yahoo.com), hope he doesnt mind ;P */

#ifndef __GIF_H__
#define __GIF_H__

#define GIF_AD		0x0e
#define GIF_NOP		0x0f

#define DECLARE_GS_PACKET(NAME,ITEMS) \
	u64 __attribute__((aligned(64))) NAME[ITEMS*2+2]; \
	int NAME##_cur; \
	int NAME##_dma_size

#define DECLARE_EXTERN_GS_PACKET(NAME) \
	extern u64 __attribute__((aligned(64))) NAME[]; \
	extern int NAME##_cur; \
	extern int NAME##_dma_size

#define BEGIN_GS_PACKET(NAME) \
	NAME##_cur = 0

#define GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,NREG,REGS) \
	NAME##_dma_size = NLOOP+1; \
	NAME[NAME##_cur++] = \
		((u64)(NLOOP)<< 0)		| \
		((u64)(EOP)	<< 15)		| \
		((u64)(PRE)	<< 46)		| \
		((u64)(PRIM)	<< 47)		| \
		((u64)(FLG)	<< 58)		| \
		((u64)(NREG)	<< 60);		\
	NAME[NAME##_cur++] = (u64)REGS

#define WRITE_GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,NREG,REGS) \
	NAME##_dma_size = NLOOP+1; \
	NAME[0] = \
		((u64)(NLOOP)<< 0)		| \
		((u64)(EOP)	<< 15)		| \
		((u64)(PRE)	<< 46)		| \
		((u64)(PRIM)	<< 47)		| \
		((u64)(FLG)	<< 58)		| \
		((u64)(NREG)	<< 60);		\
	NAME[1] = (u64)REGS

#define GIF_TAG_AD(NAME,NLOOP,EOP,PRE,PRIM,FLG) \
	GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,1,GIF_AD)

#define WRITE_GIF_TAG_AD(NAME,NLOOP,EOP,PRE,PRIM,FLG) \
	WRITE_GIF_TAG(NAME,NLOOP,EOP,PRE,PRIM,FLG,1,GIF_AD)

#define GIF_DATA_AD(NAME,REG,DAT) \
	NAME[NAME##_cur++] = (u64)DAT; \
	NAME[NAME##_cur++] = (u64)REG

#define GIF_TAG_IMG(NAME,QSIZE) \
	GIF_TAG(NAME,(QSIZE),1,0,0,2,0,0); \
	NAME##_dma_size = 1 \

#ifdef __PS2LINUX__

#define SEND_GS_PACKET(NAME) send_gs_packet(NAME, NAME##_cur)

#else

#define SEND_GS_PACKET(NAME) \
	FlushCache(0);							\
	SET_QWC(GIF_QWC, NAME##_dma_size);			\
	SET_MADR(GIF_MADR, &(NAME), 0);				\
	SET_CHCR(GIF_CHCR, 1, 0, 0, 0, 0, 1, 0);	\
	DMA_WAIT(GIF_CHCR)

#endif

#endif
