/*
 * smap.c - TGE DMA relay for PS2 SMAP
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 * Copyright (c) 2004 James Forshaw <tiraniddo@hotmail.com>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "dmarelay.h"

static int tx_thid, tx_end_thid, rx_thid, rx_end_thid;
static SifRpcDataQueue_t tx_qd, tx_end_qd, rx_qd, rx_end_qd;
static SifRpcServerData_t tx_sd, tx_end_sd, rx_sd, rx_end_sd;

static u8 tx_rpcbuf[32] __attribute__((aligned(8)));
static u8 rx_rpcbuf[32] __attribute__((aligned(8)));

static u8 dma_tx_buffer[0x600] __attribute__((aligned(8)));
static u8 dma_rx_buffer[0x600] __attribute__((aligned(8)));

static u32 tx_result;
static u32 rx_result;

static struct eng_args *smap_args;

struct smap_rpc_data

{
    u16 ptr;
    u16 dummy;
    u32 size;
} __attribute__((packed));

static inline int do_transmit(void)

{
    USE_SPD_REGS;
    USE_SMAP_REGS;
    volatile iop_dmac_chan_t *dev9_chan = (volatile iop_dmac_chan_t *)DEV9_DMAC_BASE;
    int res;
    struct smap_rpc_data *t = (struct smap_rpc_data *) tx_rpcbuf;

    res = WaitSema(smap_args->semid);
    if(res != 0)
    {
	return -1;
    }

    ClearEventFlag(smap_args->evflg, 0);

    /* Disable channel */
    SPD_REG8(SPD_R_DMA_CTRL) = 7;
    dev9_chan->chcr = 0;

    EnableIntr(IOP_IRQ_DMA_DEV9);

    /* Setup DMA channel */
    dev9_chan->madr = (u32) dma_tx_buffer;
    dev9_chan->bcr  = ((t->size / 128) << 16) | 32;
    dev9_chan->chcr = 0x01000201;

    /* Setup SMAP destination stuff */
    SMAP_REG16(SMAP_R_TXFIFO_WR_PTR) = t->ptr;
    SMAP_REG16(SMAP_R_TXFIFO_SIZE) = (t->size / 128);
    SMAP_REG8(SMAP_R_TXFIFO_CTRL) = SMAP_TXFIFO_DMAEN;

    WaitEventFlag(smap_args->evflg, EF_DMA_DONE | EF_SMAP_DONE, 0x11, &res);

    if(res & 4) /* Cancelled */
    {
	DisableIntr(IOP_IRQ_DMA_DEV9, 0);
	dev9_chan->chcr = 0;
	SMAP_REG8(SMAP_R_TXFIFO_CTRL) = 0;
	SignalSema(smap_args->semid);
	return -1;
    }

    DisableIntr(IOP_IRQ_DMA_DEV9, 0);
    SignalSema(smap_args->semid);

    return 0;
}

static void *tx_rpc_func(int fid, void *data, int size)

{
    int *res = &tx_result;

    switch(fid)
    {
	case RPCF_SMAP_GetBufAddr : *res = (int) dma_tx_buffer;
				    break;
	case RPCF_SMAP_Go : *res = do_transmit();
			    break;
	default: *res = -1;
		 break;
    }

    return res;
}

static void tx_rpc_thread(void *arg)

{
    if (!sceSifCheckInit())
	sceSifInit();

    sceSifInitRpc(0);

    sceSifSetRpcQueue(&tx_qd, GetThreadId());

    sceSifRegisterRpc(&tx_sd, RPCS_SMAP_TX_BEGIN, tx_rpc_func, tx_rpcbuf, NULL, NULL, &tx_qd);

    sceSifRpcLoop(&tx_qd);
}

static void *tx_rpc_end_func(int fid, void *data, int size)

{
    SetEventFlag(smap_args->evflg, 4);
    ReleaseWaitThread(tx_thid);

    return NULL;
}

static void tx_rpc_end_thread(void *arg)

{
    if (!sceSifCheckInit())
	sceSifInit();

    sceSifInitRpc(0);

    sceSifSetRpcQueue(&tx_end_qd, GetThreadId());

    sceSifRegisterRpc(&tx_end_sd, RPCS_SMAP_TX_END, tx_rpc_end_func, NULL, NULL, NULL, &tx_end_qd);

    sceSifRpcLoop(&tx_end_qd);
}

static inline int do_receive(void)

{
    USE_SPD_REGS;
    USE_SMAP_REGS;
    volatile iop_dmac_chan_t *dev9_chan = (volatile iop_dmac_chan_t *)DEV9_DMAC_BASE;
    int res;
    struct smap_rpc_data *t = (struct smap_rpc_data *) tx_rpcbuf;

    res = WaitSema(smap_args->semid);
    if(res != 0)
    {
	return -1;
    }

    ClearEventFlag(smap_args->evflg, 0);

    /* Disable channel */
    SPD_REG8(SPD_R_DMA_CTRL) = 7;
    dev9_chan->chcr = 0;

    EnableIntr(IOP_IRQ_DMA_DEV9);

    /* Setup DMA channel */
    dev9_chan->madr = (u32) dma_tx_buffer;
    dev9_chan->bcr  = ((t->size / 128) << 16) | 32;
    dev9_chan->chcr = 0x01000200;

    /* Setup SMAP destination stuff */
    SMAP_REG16(SMAP_R_RXFIFO_RD_PTR) = t->ptr;
    SMAP_REG16(SMAP_R_RXFIFO_SIZE) = (t->size / 128);
    SMAP_REG8(SMAP_R_RXFIFO_CTRL) = SMAP_RXFIFO_DMAEN;

    WaitEventFlag(smap_args->evflg, EF_DMA_DONE | EF_SMAP_DONE, 0x11, &res);

    if(res & 4) /* Cancelled */
    {
	DisableIntr(IOP_IRQ_DMA_DEV9, 0);
	dev9_chan->chcr = 0;
	SMAP_REG8(SMAP_R_RXFIFO_CTRL) = 0;
	SignalSema(smap_args->semid);
	return -1;
    }

    DisableIntr(IOP_IRQ_DMA_DEV9, 0);
    SignalSema(smap_args->semid);

    return 0;
}

static void *rx_rpc_func(int fid, void *data, int size)

{
    int *res = &rx_result;

    switch(fid)
    {
	case RPCF_SMAP_GetBufAddr : *res = (int) dma_rx_buffer;
				    break;
	case RPCF_SMAP_Go : *res = do_receive();
			    break;
	default: *res = -1;
		 break;
    }

    return res;
}

static void rx_rpc_thread(void *arg)

{
    if (!sceSifCheckInit())
	sceSifInit();

    sceSifInitRpc(0);

    sceSifSetRpcQueue(&rx_qd, GetThreadId());

    sceSifRegisterRpc(&rx_sd, RPCS_SMAP_RX_BEGIN, rx_rpc_func, rx_rpcbuf, NULL, NULL, &rx_qd);

    sceSifRpcLoop(&rx_qd);
}

static void *rx_rpc_end_func(int fid, void *data, int size)

{
    SetEventFlag(smap_args->evflg, 4);
    ReleaseWaitThread(rx_thid);

    return NULL;
}

static void rx_rpc_end_thread(void *arg)

{
    if (!sceSifCheckInit())
	sceSifInit();

    sceSifInitRpc(0);

    sceSifSetRpcQueue(&rx_end_qd, GetThreadId());

    sceSifRegisterRpc(&rx_end_sd, RPCS_SMAP_RX_END, rx_rpc_end_func, NULL, NULL, NULL, &rx_end_qd);

    sceSifRpcLoop(&rx_end_qd);
}

int smap_engine_init(struct eng_args *args)
{
    iop_thread_t thread;

    smap_args = args;

    thread.attr = TH_C;
    thread.thread = tx_rpc_thread;
    thread.stacksize = 4096;
    thread.priority = 20;
    if ((tx_thid = CreateThread(&thread)) < 0)
	return tx_thid;

    StartThread(tx_thid, args);

    thread.attr = TH_C;
    thread.thread = tx_rpc_end_thread;
    thread.stacksize = 1024;
    thread.priority = 20;
    if ((tx_end_thid = CreateThread(&thread)) < 0)
	return tx_end_thid;

    StartThread(tx_end_thid, args);

    thread.attr = TH_C;
    thread.thread = rx_rpc_thread;
    thread.stacksize = 4096;
    thread.priority = 20;
    if ((rx_thid = CreateThread(&thread)) < 0)
	return rx_thid;

    StartThread(rx_thid, args);

    thread.attr = TH_C;
    thread.thread = rx_rpc_end_thread;
    thread.stacksize = 1024;
    thread.priority = 20;
    if ((rx_end_thid = CreateThread(&thread)) < 0)
	return rx_end_thid;
    
    StartThread(rx_end_thid, args);

    return 0;
}
