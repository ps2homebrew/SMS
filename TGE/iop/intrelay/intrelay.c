/*
 * intrelay.c - TGE interupt relay
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "types.h"
#include "defs.h"
#include "irx.h"

#include "intrman.h"
#include "thbase.h"
#include "stdio.h"

#include "tge_hwdefs.h"

IRX_ID("intrelay", 1, 1);

#define SIF_SMFLAG	0xbd000030

int intr_usb_handler(void *unused)
{
	_sw(TGE_SBUS_IRQ_USB, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
	
	return 1;
}

int intr_dev9_handler(void *unused)
{
	_sw(TGE_SBUS_IRQ_DEV9, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
	
	return 1;
}

int intr_ilink_handler(void *unused)
{
	_sw(TGE_SBUS_IRQ_ILINK, SIF_SMFLAG);

	_sw(_lw(0xbf801450) | 2, 0xbf801450);
	_sw(_lw(0xbf801450) & 0xfffffffd, 0xbf801450);
	_lw(0xbf801450);
	
	return 1;
}

void nullthread(void *unused)
{
	while (1)
		SleepThread();
}

int _start(int argc, char *argv[])
{
	iop_thread_t thread;
	int res;

	if ((res = RegisterIntrHandler(IOP_IRQ_DEV9, 1, intr_dev9_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_DEV9, res);
		return -1;
	}

	if ((res = RegisterIntrHandler(IOP_IRQ_USB, 1, intr_usb_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_USB, res);
		return -1;
	}

	if ((res = RegisterIntrHandler(IOP_IRQ_ILINK, 1, intr_ilink_handler, NULL))) {
		printf("intr 0x%02x, error %d\n", IOP_IRQ_ILINK, res);
		return -1;
	}

	CpuEnableIntr();
	EnableIntr(IOP_IRQ_DEV9);
	EnableIntr(IOP_IRQ_USB);
	EnableIntr(IOP_IRQ_ILINK);

	thread.attr = TH_C;
	thread.option = 0;
	thread.thread = nullthread;
	thread.stacksize = 512;
	thread.priority = 126;

	if ((res = CreateThread(&thread)) < 0) {
		printf("cannot create thread\n");
		return -1;
	}

	StartThread(res, NULL);
	return 0;
}
