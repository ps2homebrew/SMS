/*
 * dmarelay.h -
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef DMARELAY_H
#define DMARELAY_H

#include "types.h"
#include "defs.h"
#include "irx.h"

#include "sysmem.h"
#include "intrman.h"
#include "dmacman.h"
#include "thbase.h"
#include "thevent.h"
#include "thsemap.h"
#include "sifman.h"
#include "sifcmd.h"
#include "stdio.h"

#include "dev9regs.h"
#include "speedregs.h"
#include "smapregs.h"

#define MODNAME "dmarelay"

#define M_PRINTF(format, args...)	\
	printf(MODNAME ": " format, ## args)
#define E_PRINTF(format, args...)	\
	printf(MODNAME ": Fatal: " format, ## args)

#define ATA_MAX_ENTRIES		256
#define ATA_BUFFER_SIZE		(512 * ATA_MAX_ENTRIES)

typedef struct {
	int	command;
	int	size;
	int	count;
	int	devctrl;
	SifDmaTransfer_t dmat[ATA_MAX_ENTRIES];
} ata_dma_transfer_t;

/* Event flags for eng_args.evflg.  */
#define EF_DMA_DONE		0x01
#define EF_ATA_DONE		0x02
#define EF_SMAP_DONE		0x04

struct eng_args {
	int	semid;
	int	evflg;
};

extern struct eng_args eng_args;

/* RPC server ID's and function ID's.  */
#define RPCS_ATA_DMA_BEGIN	0x2000
#define  RPCF_GetBufAddr	0
#define  RPCF_DmaRead		1
#define  RPCF_DmaWrite		2

#define RPCS_ATA_DMA_END	0x2001

#define RPCS_SMAP_TX_BEGIN	0x2002
/* These are repeated for TX and RX */
#define RPCF_SMAP_GetBufAddr	0
#define RPCF_SMAP_Go		1

#define RPCS_SMAP_TX_END	0x2003
#define RPCS_SMAP_RX_BEGIN	0x2004
#define RPCS_SMAP_RX_END	0x2005

int ata_engine_init(struct eng_args *args);
int smap_engine_init(struct eng_args *args);

#endif /* DMARELAY_H */
