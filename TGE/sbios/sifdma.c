/*
 * sifdma.c - Low-level SIF and SIF DMA
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"
#include "tge_sifdma.h"

#include "sbcalls.h"
#include "sif.h"
#include "hwreg.h"
#include "core.h"

static int initialized;

static u32 sif_reg_table[32] __attribute__((aligned(64)));
static u8 sif1_dmatag_index;
static u16 sif1_dma_count;
static ee_dmatag_t sif1_dmatags[32];

/* Describes the tag accepted by the IOP on SIF1 DMA transfers.  */
typedef struct {
	u32	addr;
	u32	size;
	u64	pad64;
	u128	data[7];	/* 7 qwords of data.  */
} iop_dmatag_t ALIGNED(16);

static iop_dmatag_t iop_dmatags[32];

static void sif_dma_init(void);

/* Initialization.  */
int sbcall_sifinit()
{
	int i;

	if (initialized)
		return 0;

	initialized = 1;

	for (i = 0; i < 32; i++)
		sif_reg_table[i] = 0;

	_sw(0xff, EE_SIF_UNKNF260);

	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF1_CHCR);

	sbcall_sifsetdchain();
	sif_dma_init();

	return 0;
}

int sbcall_sifexit()
{
	initialized = 0;
	return 0;
}

int sbcall_sifsetdchain()
{
	u32 status;

	core_save_disable(&status);

	/* Stop DMA.  */
	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF0_QWC);

	/* Chain mode; enable tag interrupt; start DMA.  */
	_sw(0x184, EE_DMAC_SIF0_CHCR);
	_lw(EE_DMAC_SIF0_CHCR);

	core_restore(status);
	return 0;
}

int sbcall_sifstopdma()
{
	_sw(0, EE_DMAC_SIF0_CHCR);
	_sw(0, EE_DMAC_SIF0_QWC);
	_lw(EE_DMAC_SIF0_QWC);
	return 0;
}

static void sif_dma_init()
{
	ee_dmatag_t *tag;

	sif1_dma_count = 0xffff;
	sif1_dmatag_index = 30;

	/* This causes the DMAC to loop to the first tag when it hits this tag;
	   it's never touched again by us.  */
	tag = KSEG1ADDR(&sif1_dmatags[31]);
	tag->id_qwc = EE_DMATAG_ID(EE_DMATAG_ID_NEXT);
	tag->addr = PHYSADDR(&sif1_dmatags);

	_sw(0, EE_DMAC_SIF1_QWC);
	_sw(PHYSADDR(&sif1_dmatags), EE_DMAC_SIF1_TADR);
}

#define SET_NEXT_DMATAG()			\
	{					\
		sif1_dmatag_index++;		\
		if (sif1_dmatag_index == 31) {	\
			sif1_dmatag_index = 0;	\
			sif1_dma_count++;	\
		}				\
	}

static void sif_dma_transfer(void *src, void *dest, u32 size, u32 attr, ee_dmatag_id_t id)
{
	ee_dmatag_t *tag;
	iop_dmatag_t *dtag;
	u128 *s128, *d128;
	u32 data_qwc, i, qwc = (size + 15) / 16;

	SET_NEXT_DMATAG();
	tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);

	if (attr & TGE_SIFDMA_ATTR_DMATAG) {
		tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc) |
			((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
		dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
		tag->addr = PHYSADDR(dtag);

		qwc--;
	} else {
		if (qwc >= 8) {
			/* Split the data if it's more than 7 qwords.  */
			tag->id_qwc = EE_DMATAG_ID_QWC(EE_DMATAG_ID_REF, 8);
			dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
			tag->addr = PHYSADDR(dtag);
			data_qwc = 7;

			SET_NEXT_DMATAG();
			tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);
			tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc - 7) |
				((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
			tag->addr = PHYSADDR((u32)src + 112);
		} else {
			tag->id_qwc = EE_DMATAG_ID_QWC(id, qwc + 1) |
				((attr & TGE_SIFDMA_ATTR_INT_I) ? EE_DMATAG_IRQ : 0);
			dtag = KSEG1ADDR(&iop_dmatags[sif1_dmatag_index]);
			tag->addr = PHYSADDR(dtag);
			data_qwc = qwc;
		}

		/* Copy the source data into the destination tag.  */
		for (i = 0, s128 = KSEG1ADDR(src), d128 = dtag->data; i < data_qwc; i++)
			*d128++ = *s128++;
	}

	dtag->size = qwc * 4;
	dtag->addr = ((u32)dest & 0x00ffffff) |
		((attr & TGE_SIFDMA_ATTR_INT_O) ? 0x40000000 : 0) |
		((attr & TGE_SIFDMA_ATTR_ERT) ? 0x80000000 : 0);
}

static u32 sif_dma_index()
{
	u32 index;

	index = (_lw(EE_DMAC_SIF1_TADR) - PHYSADDR(&sif1_dmatags)) / sizeof(ee_dmatag_t);
	index = index > 0 ? index - 1 : 30;

	if (index == sif1_dmatag_index)
		return _lw(EE_DMAC_SIF1_QWC) ? 30 : 31;

	if (index < sif1_dmatag_index)
		return index - sif1_dmatag_index + 30;

	return index - sif1_dmatag_index - 1;
}

static void sif_dma_setup_tag(u32 index)
{
	ee_dmatag_t *tag;

	if (index == 31)
		return;

	tag = KSEG1ADDR(&sif1_dmatags[sif1_dmatag_index]);

	if (index == 30) {
		/* Keep the previous IRQ and PCE bits.  */
		tag->id_qwc &= 0x8c000000;
		tag->id_qwc |= EE_DMATAG_ID(EE_DMATAG_ID_REF);
		tag->id_qwc |= _lw(EE_DMAC_SIF1_QWC);
		tag->addr = _lw(EE_DMAC_SIF1_MADR);

		_sw(0, EE_DMAC_SIF1_QWC);
		_sw(PHYSADDR(tag), EE_DMAC_SIF1_TADR);
	} else {
		tag->id_qwc |= EE_DMATAG_ID(EE_DMATAG_ID_REF);
	}
}

int sif_set_dma(tge_sifdma_transfer_t *transfer, u32 tcount)
{
	tge_sifdma_transfer_t *t;
	u32 status, index, i, ntags, start, count;
	int id = 0;

	core_save_disable(&status);

	/* Suspend all DMA to stop any active SIF1 transfers.  */
	_sw(_lw(EE_DMAC_ENABLER) | EE_DMAC_CPND, EE_DMAC_ENABLEW);
	_sw(0, EE_DMAC_SIF1_CHCR);
	_lw(EE_DMAC_SIF1_CHCR);
	_sw(_lw(EE_DMAC_ENABLER) & ~EE_DMAC_CPND, EE_DMAC_ENABLEW);

	index = sif_dma_index();

	/* Find out how many tags we'll need for the transfer.  */
	for (i = tcount, t = transfer, ntags = 0; i; i--, t++) {
		if ((t->attr & TGE_SIFDMA_ATTR_DMATAG) || t->size <= 112)
			ntags++;
		else
			ntags += 2;
	}

	/* We can only transfer if we have enough tags free.  */
	if (ntags <= index) {
		start = ((sif1_dmatag_index + 1) % 31) & 0xff;
		count = start ? sif1_dma_count : sif1_dma_count + 1;
		id = (count << 16) | (start << 8) | (ntags & 0xff);

		sif_dma_setup_tag(index);

		for (i = tcount - 1, t = transfer; i; i--, t++)
			sif_dma_transfer(t->src, t->dest, t->size, t->attr, EE_DMATAG_ID_REF);

		sif_dma_transfer(t->src, t->dest, t->size, t->attr, EE_DMATAG_ID_REFE);
	}

	/* Start the DMA transfer.  */
	_sw(_lw(EE_DMAC_SIF1_CHCR) | 0x184, EE_DMAC_SIF1_CHCR);
	_lw(EE_DMAC_SIF1_CHCR);

	core_restore(status);
	return id;
}

/* XXX: Figure out the logic of this routine and finish it.  */
int sif_dma_stat(int id)
{
	u32 status, index, ntags, starttag, count, lasttag, dma_count;

	if (!(_lw(EE_DMAC_SIF1_CHCR) & 0x100))
		return -1;

	core_save_disable(&status);

	ntags = id & 0xff;
	starttag = (id >> 8) & 0xff;
	lasttag = (starttag + ntags - 1) % 31;
	count = lasttag ? id >> 16 : (id >> 16) + 1;

	index = (_lw(EE_DMAC_SIF1_TADR) - PHYSADDR(&sif1_dmatags)) / sizeof(ee_dmatag_t);
	index = index > 0 ? index - 1 : 30;

	if (sif1_dmatag_index < index)
		dma_count = sif1_dma_count - 1;
	else
		dma_count = sif1_dma_count;

	if ((dma_count == (id >> 16)) || dma_count == count) {
		if (index >= starttag && index <= lasttag)
			return 0;
	}

	core_restore(status);
	return -1;
}

int sbcall_sifsetdma(tge_sbcall_sifsetdma_arg_t *arg)
{
	return sif_set_dma(arg->transfer, arg->tcount);
}

int sbcall_sifdmastat(tge_sbcall_sifdmastat_arg_t *arg)
{
	return sif_dma_stat(arg->id);
}

/* Register manipulation.  */
static u32 sif_get_msflag()
{
	u32 val;

	do {
		val = _lw(EE_SIF_MSFLAG);

		/* If the IOP has updated this register, wait for it.  */
		asm ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		     "nop\nnop\nnop\nnop\nnop\nnop\nnop");
	} while (val != _lw(EE_SIF_MSFLAG));

	return val;
}

static u32 sif_set_msflag(u32 val)
{
	_sw(val, EE_SIF_MSFLAG);
	return sif_get_msflag();
}

static u32 sif_get_smflag()
{
	u32 val;

	do {
		val = _lw(EE_SIF_SMFLAG);

		/* If the IOP has updated this register, wait for it.  */
		asm ("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n" \
		     "nop\nnop\nnop\nnop\nnop\nnop\nnop");
	} while (val != _lw(EE_SIF_SMFLAG));

	return val;
}

static u32 sif_set_smflag(u32 val)
{
	_sw(val, EE_SIF_SMFLAG);
	return sif_get_smflag();
}

u32 sif_set_reg(u32 reg, u32 val)
{
	switch (reg) {
		case 1:
			_sw(val, EE_SIF_MAINADDR);
			return _lw(EE_SIF_MAINADDR);
		case 3:
			return sif_set_msflag(val);
		case 4:
			return sif_set_smflag(val);
	}

	/* Is bit 31 set? */
	if ((int)reg < 0) {
		reg &= 0x7fffffff;
		if (reg < 32)
			sif_reg_table[reg] = val;
	}

	return 0;
}

u32 sif_get_reg(u32 reg)
{
	switch (reg) {
		case 1:
			return _lw(EE_SIF_MAINADDR);
		case 2:
			return _lw(EE_SIF_SUBADDR);
		case 3:
			return sif_get_msflag();
		case 4:
			return sif_get_smflag();
	}

	/* Is bit 31 set? */
	if ((int)reg < 0) {
		reg &= 0x7fffffff;
		if (reg < 32)
			return sif_reg_table[reg];
	}

	return 0;
}

u32 sbcall_sifsetreg(tge_sbcall_sifsetreg_arg_t *arg)
{
	return sif_set_reg(arg->reg, arg->val);
}

u32 sbcall_sifgetreg(tge_sbcall_sifgetreg_arg_t *arg)
{
	return sif_get_reg(arg->reg);
}
