/*
 * hwreg.h - Hardware register definitions header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_HWREG_H
#define TGE_HWREG_H

#include "tge_types.h"
#include "tge_defs.h"

/* DMAC */
#define EE_DMAC_SIF0_CHCR	0xb000c000
#define EE_DMAC_SIF0_MADR	0xb000c010
#define EE_DMAC_SIF0_QWC	0xb000c020

#define EE_DMAC_SIF1_CHCR	0xb000c400
#define EE_DMAC_SIF1_MADR	0xb000c410
#define EE_DMAC_SIF1_QWC	0xb000c420
#define EE_DMAC_SIF1_TADR	0xb000c430

/* Serial I/O */
#define EE_SIO_ISR      	0xb000f130
#define EE_SIO_TXFIFO   	0xb000f180
#define EE_SIO_RXFIFO   	0xb000f1c0

/* SIF */
#define EE_SIF_MAINADDR 	0xb000f200
#define EE_SIF_SUBADDR  	0xb000f210
#define EE_SIF_MSFLAG   	0xb000f220
#define EE_SIF_SMFLAG   	0xb000f230
#define EE_SIF_SUBCTRL  	0xb000f240
#define EE_SIF_UNKNF260		0xb000f260

/* DMAC */
#define EE_DMAC_ENABLER		0xb000f520
#define EE_DMAC_ENABLEW		0xb000f590
#define   EE_DMAC_CPND	(1<<16)

/* DMA tags */
typedef struct {
	u32	id_qwc;
	u32	addr;
	u64	pad64;
} ee_dmatag_t ALIGNED(16);

/* Values for the ID field.  */
typedef enum {
	EE_DMATAG_ID_REFE,
	EE_DMATAG_ID_CNT,
	EE_DMATAG_ID_NEXT,
	EE_DMATAG_ID_REF,
	EE_DMATAG_ID_REFS,
	EE_DMATAG_ID_CALL,
	EE_DMATAG_ID_RET,
	EE_DMATAG_ID_END
} ee_dmatag_id_t;

#define EE_DMATAG_ID(id)		((id) << 28)
#define EE_DMATAG_ID_QWC(id, qwc)	(EE_DMATAG_ID(id)|(qwc))
#define EE_DMATAG_IRQ			(1<<31)
#define EE_DMATAG_PCE(pce)		((pce) << 26)

#endif /* TGE_HWREG_H */
