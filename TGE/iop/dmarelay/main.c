/*
 * main.c - TGE DMA relay
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "dmarelay.h"

IRX_ID(MODNAME, 1, 2);

struct eng_args eng_args = { -1, -1 };

static int dev9_dma_handler(void *arg)
{
	int evflg = *(int *)arg;

	iSetEventFlag(evflg, EF_DMA_DONE);
	return 1;
}

int _start(int argc, char *argv[])
{
	iop_event_t event;
	int semid, evflg, res;

	if ((semid = CreateMutex(IOP_MUTEX_UNLOCKED)) < 0) {
		E_PRINTF("Unable to create %s (error %d).\n", "semaphore", semid);
		return 1;
	}

	eng_args.semid = semid;

	event.attr = event.bits = 0;
	if ((evflg = CreateEventFlag(&event)) < 0) {
		E_PRINTF("Unable to create %s (error %d).\n", "event flag", evflg);
		return 1;
	}

	eng_args.evflg = evflg;

	CpuEnableIntr();
	DisableIntr(IOP_IRQ_DMA_DEV9, NULL);
	if ((res = RegisterIntrHandler(IOP_IRQ_DMA_DEV9, 1, dev9_dma_handler, &eng_args.evflg))) {
		E_PRINTF("Unable to register 0x%02x intr handler (error %d).\n", IOP_IRQ_DMA_DEV9, res);
		return 1;
	}

	_sw(_lw(0xbf801570) | 0x80, 0xbf801570);

	if ((res = ata_engine_init(&eng_args)) < 0) {
		E_PRINTF("Unable to initialize the %s DMA engine.\n", "ATA");
		return 1;
	}

	if ((res = smap_engine_init(&eng_args)) < 0) {
		E_PRINTF("Unable to initialize the %s DMA engine.\n", "SMAP");
		return 1;
	}

	M_PRINTF("ATA/SMAP DMA relay module initialized.\n");
	return 0;
}
