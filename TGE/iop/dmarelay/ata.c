/*
 * ata.c - TGE DMA relay for PS2 ATA
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "dmarelay.h"

static ata_dma_transfer_t dma_transfer;

static SifRpcDataQueue_t qd, end_qd;
static SifRpcServerData_t sd, end_sd;

static void *dma_buffer;

static int read_thid, write_thid, cur_thid;
static int rpc_result;

static void dma_setup(int val)
{
	USE_SPD_REGS;
	u8 if_ctrl;

	if_ctrl = SPD_REG8(SPD_R_IF_CTRL) & 0x01;

	if (!val)
		if_ctrl |= 0x7e;
	else
		if_ctrl |= 0x7c;

	SPD_REG8(SPD_R_IF_CTRL) = if_ctrl;

	SPD_REG8(SPD_R_XFR_CTRL) = val | 0x02;
	SPD_REG8(SPD_R_DMA_CTRL) = 0x06;
	SPD_REG8(0x38) = 0x03;

	_sw(0, DEV9_DMAC_CHCR);
}

static void dma_stop(int val)
{
	USE_SPD_REGS;
	u8 if_ctrl;

	_sw(0, DEV9_DMAC_CHCR);

	SPD_REG8(SPD_R_XFR_CTRL) = 0;
	SPD_REG8(SPD_R_IF_CTRL) = SPD_REG8(SPD_R_IF_CTRL) & 0xfb;

	if (val) {
		if_ctrl = SPD_REG8(SPD_R_IF_CTRL);
		SPD_REG8(SPD_R_IF_CTRL) = SPD_IF_ATA_RESET;
		DelayThread(100);
		SPD_REG8(SPD_R_IF_CTRL) = if_ctrl;
	}

	/*M_PRINTF("ATA DMA force break\n");*/
}

static void read_thread(void *arg)
{
	USE_SPD_REGS;
	volatile iop_dmac_chan_t *dev9_chan = (volatile iop_dmac_chan_t *)DEV9_DMAC_BASE;
	struct eng_args *args = (struct eng_args *)arg;
	ata_dma_transfer_t *t = &dma_transfer;
	u32 res;

	while (1) {
		while (SleepThread() || WaitSema(args->semid))
			;

		ClearEventFlag(args->evflg, 0);

		dma_setup(0);
		EnableIntr(IOP_IRQ_DMA_DEV9);

	}
}

static void write_thread(void *arg)
{
	USE_SPD_REGS;
	volatile iop_dmac_chan_t *dev9_chan = (volatile iop_dmac_chan_t *)DEV9_DMAC_BASE;
	struct eng_args *args = (struct eng_args *)arg;
	ata_dma_transfer_t *t = &dma_transfer;
	u32 res;

	while (1) {
		while (SleepThread() || WaitSema(args->semid))
			;

		ClearEventFlag(args->evflg, 0);

		dma_setup(1);
		EnableIntr(IOP_IRQ_DMA_DEV9);

		/* Initiate the DMA transfer.  */
		dev9_chan->madr = (u32)dma_buffer;
		dev9_chan->bcr  = ((t->size / 128) << 16) | 32;
		dev9_chan->chcr = 0x01000201;

		SPD_REG8(0x4e) = t->command;	/* ATA command register.  */
		SPD_REG8(SPD_R_PIO_DIR) = 1;
		SPD_REG8(SPD_R_PIO_DATA) = 0;
		SPD_REG8(SPD_R_XFR_CTRL) |= 0x80;

		WaitEventFlag(args->evflg, (EF_DMA_DONE|EF_ATA_DONE), 0x11, &res);

		SPD_REG8(SPD_R_XFR_CTRL) &= 0x7f;

		DisableIntr(IOP_IRQ_DMA_DEV9, NULL);

		/* If we got the ATA end signal, force stop the transfer.  */
		if (res & EF_ATA_DONE)
			dma_stop(1);

		SignalSema(args->semid);
	}
}

static void *rpc_func(int fid, void *buf, int bufsize)
{
	int *res = &rpc_result;
	int thid = -1;

	switch (fid) {
		case RPCF_GetBufAddr:
			*res = (u32)dma_buffer;
			break;
		case RPCF_DmaRead:
			thid = read_thid;
			WakeupThread(thid);
			break;
		case RPCF_DmaWrite:
			thid = write_thid;
			WakeupThread(thid);
			break;

		default:
			*res = -1;
	}

	if (thid >= 0) {
		cur_thid = thid;
		*res = 0;
	}

	return res;
}

static void rpc_thread(void *unused)
{
	if (!sceSifCheckInit())
		sceSifInit();

	sceSifInitRpc(0);

	sceSifSetRpcQueue(&qd, GetThreadId());

	sceSifRegisterRpc(&sd, RPCS_ATA_DMA_BEGIN, rpc_func, &dma_transfer, NULL, NULL, &qd);

	sceSifRpcLoop(&qd);
}

static void *rpc_end_func(int fid, void *buf, int bufsize)
{
	SetEventFlag(eng_args.evflg, EF_ATA_DONE);

	ReleaseWaitThread(cur_thid);
	return NULL;
}

static void rpc_end_thread(void *unused)
{
	if (!sceSifCheckInit())
		sceSifInit();

	sceSifInitRpc(0);

	sceSifSetRpcQueue(&end_qd, GetThreadId());

	sceSifRegisterRpc(&end_sd, RPCS_ATA_DMA_END, rpc_end_func, NULL, NULL, NULL, &end_qd);

	sceSifRpcLoop(&end_qd);
}

int ata_engine_init(struct eng_args *args)
{
	iop_thread_t thread;
	int thid;

	/* DMA read thread.  */
	thread.attr = TH_C;
	thread.thread = read_thread;
	thread.stacksize = 4096;
	thread.priority = 21;
	if ((read_thid = CreateThread(&thread)) < 0)
		return read_thid;

	StartThread(read_thid, args);

	/* DMA write thread.  */
	thread.attr = TH_C;
	thread.thread = write_thread;
	thread.stacksize = 4096;
	thread.priority = 21;
	if ((write_thid = CreateThread(&thread)) < 0)
		return write_thid;

	StartThread(write_thid, args);

	/* RPC thread.  */
	thread.attr = TH_C;
	thread.thread = rpc_thread;
	thread.stacksize = 4096;
	thread.priority = 20;
	if ((thid = CreateThread(&thread)) < 0)
		return thid;

	StartThread(thid, NULL);

	/* RPC end thread.  */
	thread.attr = TH_C;
	thread.thread = rpc_end_thread;
	thread.stacksize = 1024;
	thread.priority = 20;
	if ((thid = CreateThread(&thread)) < 0)
		return thid;

	StartThread(thid, NULL);

	if (!(dma_buffer = AllocSysMemory(0, ATA_BUFFER_SIZE, NULL)))
		return -12;

	return 0;
}
