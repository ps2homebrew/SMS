/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: ps2dev9.c 1090 2005-05-14 00:24:07Z pixel $
# DEV9 Device Driver.
*/
#include "types.h"
#include "defs.h"
#include "loadcore.h"
#include "intrman.h"
#include "dmacman.h"
#include "thbase.h"
#include "thsemap.h"
#include "ioman.h"
#include "dev9.h"
#include "sys/ioctl.h"

#include "dev9regs.h"
#include "speedregs.h"
#include "smapregs.h"

#define DRIVERNAME "dev9"

IRX_ID( "dev9_driver", 1, 1 );

#define DEV9_INTR		13

/* SSBUS registers.  */
#define SSBUS_R_1418		0xbf801418
#define SSBUS_R_141c		0xbf80141c
#define SSBUS_R_1420		0xbf801420

__asm__(
".equ SSBUS_REGBASE, 0xBF80\n\t"
".equ SSBUS_1418_32, 0x1418\n\t"
".equ SSBUS_141C_32, 0x141C\n\t"
".equ SSBUS_1420_32, 0x1420\n\t"
".equ SSBUS_1460_16, 0x1460\n\t"
".equ SSBUS_1462_16, 0x1462\n\t"
".equ SSBUS_1464_16, 0x1464\n\t"
".equ SSBUS_1466_16, 0x1466\n\t"
".equ SSBUS_1468_16, 0x1468\n\t"
".equ SSBUS_146A_16, 0x146A\n\t"
".equ SSBUS_146C_16, 0x146C\n\t"
".equ SSBUS_REV_16,  0x146E\n\t"
".equ SSBUS_1474_16, 0x1474\n\t"
".equ SSBUS_147A_16, 0x147A\n\t"
".equ SSBUS_147C_16, 0x147C\n\t"
".equ SSBUS_147E_16, 0x147E\n\t"
".equ SSBUS_DMAC_DEV9_MADR_32, 0x1510\n\t"
".equ SSBUS_DMAC_DEV9_BCR_32,  0x1514\n\t"
".equ SSBUS_DMAC_DEV9_CHCR_32, 0x1518\n\t"
".equ SSBUS_DMAC_DPCR2_32,     0x1570\n\t"
".equ SPD_REGBASE,      0xB000\n\t"
".equ SPD_REV_0_16,     0x0000\n\t"
".equ SPD_REV_1_16,     0x0002\n\t"
".equ SPD_REV_3_16,     0x0004\n\t"
".equ SPD_DMACTRL_16,   0x0024\n\t"
".equ SPD_INTR_STAT_16, 0x0028\n\t"
".equ SPD_INTR_MASK_16, 0x002A\n\t"
".equ SPD_PIODIR_8,     0x002C\n\t"
".equ SPD_PIODATA_8,    0x002E\n\t"
);

static int dev9type = -1;	/* 0 for PCMCIA, 1 for expansion bay */
static int using_aif = 0;	/* 1 if using AIF on a T10K */

static int dma_lock_sem = -1;	/* used to arbitrate DMA */

static int pcic_cardtype;	/* Translated value of bits 0-1 of 0xbf801462 */
static int pcic_voltage;	/* Translated value of bits 2-3 of 0xbf801462 */

static s16 eeprom_data[5];	/* 2-byte EEPROM status (0/-1 = invalid, 1 = valid),
				   6-byte MAC address,
				   2-byte MAC address checksum.  */

/* Each driver can register callbacks that correspond to each bit of the
   SMAP interrupt status register (0xbx000028).  */
static dev9_intr_cb_t dev9_intr_cbs[7];
static dev9_dma_cb_t  dev9_pre_dma_cbs[2];
static dev9_dma_cb_t  dev9_post_dma_cbs[2];

static iop_device_t s_Driver;

static int dev9_intr_dispatch(int flag);

extern int dummy ( void );

static int dev9_ioctl ( iop_file_t*, unsigned long, void* );

static void smap_set_stat(int stat);
static int read_eeprom_data(void);

static int smap_device_probe(void);
static int smap_device_reset(void);
static int smap_subsys_init(void);
static int smap_device_init(void);

static void pcmcia_set_stat(int stat);
static int pcic_ssbus_mode(int voltage);
static int pcmcia_device_probe(void);
static int pcmcia_device_reset(void);
static int card_find_manfid(u32 manfid);
static int pcmcia_init(void);

static void expbay_set_stat(int stat);
static int expbay_device_probe(void);
static int expbay_device_reset(void);
static int expbay_init(void);

struct irx_export_table _exp_dev9;

static int dev9Init ( void ) {

 USE_DEV9_REGS;

 int retVal = 1;
 u16 dev9hw = DEV9_REG( DEV9_R_REV ) & 0xf0;

 if ( dev9hw == 0x20 ) {  /* CXD9566 (PCMCIA) */
  dev9type = 0;
  retVal = pcmcia_init ();
 } else if ( dev9hw == 0x30 ) {  /* CXD9611 (Expansion Bay) */
  dev9type = 1;
  retVal = expbay_init ();
 }  /* end if */

 return retVal;

}  /* end dev9Init */

int _start ( int argc, char** argv ) {

 int                  res = 1;
 iop_library_table_t* libtable;
 iop_library_t*       libptr;

 libtable = GetLibraryEntryTable ();
 libptr   = libtable -> tail;

 while ( libptr ) {
  int i;
  for ( i = 0; i <= sizeof ( DRIVERNAME ); ++i ) if ( libptr -> name[ i ] != DRIVERNAME[ i ] ) break;
  if (  i > sizeof ( DRIVERNAME )  ) return 1;
  libptr = libptr -> prev;
 }  /* end while */

 if (   (  res = dev9Init ()  )   ) return res;
	
 if (  RegisterLibraryEntries ( &_exp_dev9 ) != 0  ) return 1;

 DelDrv ( "dev9x" );

 return AddDrv ( &s_Driver );

}  /* end _start */

int __attribute__(  ( unused )  ) shutdown ( void ) {

 return 0;

}  /* end shutdown */

void dev9RegisterIntrCb ( int intr, dev9_intr_cb_t cb ) {

 dev9_intr_cbs[ intr ] = cb;

}  /* end dev9RegisterIntrCb */

void dev9RegisterPreDmaCb ( int aDev, dev9_dma_cb_t aCB ) {

 dev9_pre_dma_cbs[ aDev ] = aCB;

}  /* end dev9RegisterPreDmaCb */

void dev9RegisterPostDmaCb ( int aDev, dev9_dma_cb_t aCB ) {

 dev9_post_dma_cbs[ aDev ] = aCB;

}  /* end dev9RegisterPostDmaCb */

static int dev9_intr_dispatch(int flag)
{
	USE_SPD_REGS;
	int i, bit;

	if (flag) {
		for (i = 0; i < sizeof(dev9_intr_cbs)/sizeof(dev9_intr_cbs[0]); ++i)
			if (dev9_intr_cbs[i])
				dev9_intr_cbs[i](flag);
	}

	while (SPD_REG16(SPD_R_INTR_STAT) & SPD_REG16(SPD_R_INTR_MASK)) {
		for (i = 0; i < sizeof(dev9_intr_cbs)/sizeof(dev9_intr_cbs[0]); ++i) {
			if (dev9_intr_cbs[i]) {
				bit = (SPD_REG16(SPD_R_INTR_STAT) &
					SPD_REG16(SPD_R_INTR_MASK)) >> i;
				if (bit & 0x01)
					dev9_intr_cbs[i](flag);
			}
		}
	}
	
	return 0;
}

static void smap_set_stat(int stat)
{
	if (dev9type == 0)
		pcmcia_set_stat(stat);
	else if (dev9type == 1)
		expbay_set_stat(stat);
}

static int smap_device_probe()
{
	if (dev9type == 0)
		return pcmcia_device_probe();
	else if (dev9type == 1)
		return expbay_device_probe();

	return -1;
}

static int smap_device_reset()
{
	if (dev9type == 0)
		return pcmcia_device_reset();
	else if (dev9type == 1)
		return expbay_device_reset();

	return -1;
}

/* Export 6 */
void dev9Shutdown ( void ) {
 USE_DEV9_REGS;
 if ( dev9type == 0 ) {	/* PCMCIA */
  DEV9_REG( DEV9_R_146C ) = 0;
  DEV9_REG( DEV9_R_1474 ) = 0;
 } else if ( dev9type == 1 ) {
  DEV9_REG( DEV9_R_1466 ) = 1;
  DEV9_REG( DEV9_R_1464 ) = 0;
  DEV9_REG( DEV9_R_1460 ) = DEV9_REG( DEV9_R_1464 );
  DEV9_REG( DEV9_R_146C ) = DEV9_REG( DEV9_R_146C ) & 0xFFFB;
  DEV9_REG( DEV9_R_146C ) = DEV9_REG( DEV9_R_146C ) & 0xFFFE;
 }  /* end if */
 DelayThread ( 1000000 );

}  /* end dev9Shutdown */

/* Export 7 */
void dev9IntrEnable(int mask)
{
	USE_SPD_REGS;
	int flags;

	CpuSuspendIntr(&flags);
	SPD_REG16(SPD_R_INTR_MASK) = SPD_REG16(SPD_R_INTR_MASK) | mask;
	CpuResumeIntr(flags);
}

/* Export 8 */
void dev9IntrDisable(int mask)
{
	USE_SPD_REGS;
	int flags;

	CpuSuspendIntr(&flags);
	SPD_REG16(SPD_R_INTR_MASK) = SPD_REG16(SPD_R_INTR_MASK) & ~mask;
	CpuResumeIntr(flags);
}

/* Export 5 */
/* This differs from the "official" dev9 in that it puts the calling thread to
   sleep when doing the actual DMA transfer.  I'm not sure why SCEI didn't do
   this in dev9.irx, when they do it in PS2Linux's dmarelay.irx.  Anyway,
   since this no longer blocks, it should speed up anything else on the IOP
   when HDD or SMAP are doing DMA.  */
int dev9DmaTransfer(int ctrl, void *buf, int bcr, int dir)
{
	USE_SPD_REGS;
	volatile iop_dmac_chan_t *dev9_chan = (iop_dmac_chan_t *)DEV9_DMAC_BASE;
    int lVal;
	WaitSema(dma_lock_sem);

    lVal = SPD_REG16( SPD_R_REV_1 );

	if ( lVal < 17 )
     lVal = ( ctrl & 0x03 ) | 0x04;
	else lVal = ( ctrl & 0x01 ) | 0x06;

	SPD_REG16( SPD_R_DMA_CTRL ) = lVal;

    if ( dev9_pre_dma_cbs[ ctrl ] ) dev9_pre_dma_cbs[ ctrl ] ( bcr, dir );

	dev9_chan->madr = (u32)buf;
	dev9_chan->bcr  = bcr;
	dev9_chan->chcr = DMAC_CHCR_30|DMAC_CHCR_TR|DMAC_CHCR_CO|
		(dir & DMAC_CHCR_DR);
    while ( dev9_chan->chcr & DMAC_CHCR_TR );

    if ( dev9_post_dma_cbs[ ctrl ] ) dev9_post_dma_cbs[ ctrl ] ( bcr, dir );

	SignalSema(dma_lock_sem);
	return 0;
}

static int read_eeprom_data()
{
	USE_SPD_REGS;
	int i, j, res = -2;
	u8 val;

	if (eeprom_data[0] < 0)
		goto out;

	SPD_REG8(SPD_R_PIO_DIR)  = 0xe1;
	DelayThread(1);
	SPD_REG8(SPD_R_PIO_DATA) = 0x80;
	DelayThread(1);

	for (i = 0; i < 2; i++) {
		SPD_REG8(SPD_R_PIO_DATA) = 0xa0;
		DelayThread(1);
		SPD_REG8(SPD_R_PIO_DATA) = 0xe0;
		DelayThread(1);
	}
	for (i = 0; i < 7; i++) {
		SPD_REG8(SPD_R_PIO_DATA) = 0x80;
		DelayThread(1);
		SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
		DelayThread(1);
	}
	SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
	DelayThread(1);

	val = SPD_REG8(SPD_R_PIO_DATA);
	DelayThread(1);
	if (val & 0x10) {	/* Error.  */
		SPD_REG8(SPD_R_PIO_DATA) = 0;
		DelayThread(1);
		res = -1;
		eeprom_data[0] = 0;
		goto out;
	}

	SPD_REG8(SPD_R_PIO_DATA) = 0x80;
	DelayThread(1);

	/* Read the MAC address and checksum from the EEPROM.  */
	for (i = 0; i < 4; i++) {
		eeprom_data[i+1] = 0;

		for (j = 15; j >= 0; j--) {
			SPD_REG8(SPD_R_PIO_DATA) = 0xc0;
			DelayThread(1);
			val = SPD_REG8(SPD_R_PIO_DATA);
			if (val & 0x10)
				eeprom_data[i+1] |= (1<<j);
			SPD_REG8(SPD_R_PIO_DATA) = 0x80;
			DelayThread(1);
		}
	}

	SPD_REG8(SPD_R_PIO_DATA) = 0;
	DelayThread(1);
	eeprom_data[0] = 1;	/* The EEPROM data is valid.  */
	res = 0;

out:
	SPD_REG8(SPD_R_PIO_DIR) = 1;
	return res;
}

/* Export 9 */
int dev9GetEEPROM(u16 *buf)
{
	int i;

	if (!eeprom_data[0])
		return -1;
	if (eeprom_data[0] < 0)
		return -2;

	/* We only return the MAC address and checksum.  */
	for (i = 0; i < 4; i++)
		buf[i] = eeprom_data[i+1];

	return 0;
}

void dev9LEDCtl ( int aCtl ) {

 USE_SPD_REGS;
 SPD_REG8( SPD_R_PIO_DATA ) = ( aCtl == 0 );

}  /* end dev9LEDCtl */

static int smap_subsys_init ( void ) {

 int i, stat, flags;

 if ( dma_lock_sem < 0 ) dma_lock_sem = CreateMutex ( IOP_MUTEX_UNLOCKED );

 DisableIntr ( IOP_IRQ_DMA_DEV9, &stat );

 CpuSuspendIntr ( &flags );
  dmac_set_dpcr2 (  dmac_get_dpcr2 () | 0x80  );
 CpuResumeIntr ( flags );

 smap_set_stat ( 0x103 );
 dev9IntrDisable ( 0xFFFF );

 for (  i = 0; i < sizeof ( dev9_intr_cbs     ) / sizeof ( dev9_intr_cbs   [ 0 ] ); ++i  ) dev9_intr_cbs[ i ] = NULL;
 for (  i = 0; i < sizeof ( dev9_pre_dma_cbs  ) / sizeof ( dev9_pre_dma_cbs[ 0 ] ); ++i  ) {
  dev9_pre_dma_cbs [ i ] = NULL;
  dev9_post_dma_cbs[ i ] = NULL;
 }  /* end for */

 read_eeprom_data ();
 dev9LEDCtl ( 0 );

 return 0;

}  /* end smap_subsys_init */

static int smap_device_init(void)
{
	USE_SPD_REGS;
	int idx, res;
	u16 spdrev;

	eeprom_data[0] = 0;

	if (smap_device_probe() < 0)return -1;

	smap_device_reset();

	/* Locate the SPEED Lite chip and get the bus ready for the
	   PCMCIA device.  */
	if (dev9type == 0) {
		res = card_find_manfid(0xf15300);

		if (!res && (res = pcic_ssbus_mode(5)));
		if (res) {
			dev9Shutdown();
			return -1;
		}
	}

	/* Print out the SPEED chip revision.  */
	spdrev = SPD_REG16(SPD_R_REV_1);
	idx    = (spdrev & 0xffff) - 14;
	if (spdrev == 9)
		idx = 1;	/* TS */
	else if (spdrev < 9 || (spdrev < 16 || spdrev > 17))
		idx = 0;	/* Unknown revision */

	return 0;
}

static int pcic_get_cardtype()
{
	USE_DEV9_REGS;
	u16 val = DEV9_REG(DEV9_R_1462) & 0x03;

	if (!val)
		return 1;	/* 16-bit */
	else if (val != 3)
		return 2;	/* CardBus */
	return 0;
}

static int pcic_get_voltage()
{
	USE_DEV9_REGS;
	u16 val = DEV9_REG(DEV9_R_1462) & 0x0c;

	if (val == 0x04)
		return 3;
	if (val == 0 || val == 0x08)
		return 1;
	if (val == 0x0c)
		return 2;
	return 0;
}

static int pcic_power(int voltage, int flag)
{
	USE_DEV9_REGS;
	u16 cstc1, cstc2;
	u16 val = (voltage == 1) << 2;

	DEV9_REG(DEV9_R_146C) = 0;

	if (voltage == 2)
		val |= 0x08;
	if (flag == 1)
		val |= 0x10;

	DEV9_REG(DEV9_R_146C) = val;
	DelayThread(22000);

	if (DEV9_REG(DEV9_R_1462) & 0x100)
		return 0;

	DEV9_REG(DEV9_R_146C) = 0;
	DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
	DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
	return -1;
}

static void pcmcia_set_stat(int stat)
{
	USE_DEV9_REGS;
	u16 val = stat & 0x01;

	if (stat & 0x10)
		val = 1;
	if (stat & 0x02)
		val |= 0x02;
	if (stat & 0x20)
		val |= 0x02;
	if (stat & 0x04)
		val |= 0x08;
	if (stat & 0x08)
		val |= 0x10;
	if (stat & 0x200)
		val |= 0x20;
	if (stat & 0x100)
		val |= 0x40;
	if (stat & 0x400)
		val |= 0x80;
	if (stat & 0x800)
		val |= 0x04;
	DEV9_REG(DEV9_R_1476) = val & 0xff;
}

static int pcic_ssbus_mode(int voltage)
{
	USE_DEV9_REGS;
	USE_SPD_REGS;
	u16 stat = DEV9_REG(DEV9_R_1474) & 7;

	if (voltage != 3 && voltage != 5)
		return -1;

	DEV9_REG(DEV9_R_1460) = 2;
	if (stat)
		return -1;

	if (voltage == 3) {
		DEV9_REG(DEV9_R_1474) = 1;
		DEV9_REG(DEV9_R_1460) = 1;
		SPD_REG8(0x20) = 1;
		DEV9_REG(DEV9_R_1474) = voltage;
	} else if (voltage == 5) {
		DEV9_REG(DEV9_R_1474) = voltage;
		DEV9_REG(DEV9_R_1460) = 1;
		SPD_REG8(0x20) = 1;
		DEV9_REG(DEV9_R_1474) = 7;
	}
	_sw(0xe01a3043, SSBUS_R_1418);

	DelayThread(5000);
	DEV9_REG(DEV9_R_146C) = DEV9_REG(DEV9_R_146C) & 0xfffe;
	return 0;
}

static int pcmcia_device_probe()
{
	int voltage;

	pcic_voltage = pcic_get_voltage();
	pcic_cardtype = pcic_get_cardtype();
	voltage = (pcic_voltage == 2 ? 5 : (pcic_voltage == 1 ? 3 : 0));

	if (pcic_voltage == 3 || pcic_cardtype != 1)
		return -1;

	return 0;
}

static int pcmcia_device_reset(void)
{
	USE_DEV9_REGS;
	u16 cstc1, cstc2;

	/* The card must be 16-bit (type 2?) */
	if (pcic_cardtype != 1)
		return -1;

	DEV9_REG(DEV9_R_147E) = 1;
	if (pcic_power(pcic_voltage, 1) < 0)
		return -1;

	DEV9_REG(DEV9_R_146C) = DEV9_REG(DEV9_R_146C) | 0x02;
	DelayThread(500000);

	DEV9_REG(DEV9_R_146C) = DEV9_REG(DEV9_R_146C) | 0x01;
	DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
	DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
	return 0;
}

static int card_find_manfid(u32 manfid)
{
	USE_DEV9_REGS;
	USE_SPD_REGS;
	u32 spdaddr, spdend, next, tuple;
	u8 hdr, ofs;

	DEV9_REG(DEV9_R_1460) = 2;
	_sw(0x1a00bb, SSBUS_R_1418);

	/* Scan the card for the MANFID tuple.  */
	spdaddr = 0;
	spdend =  0x1000;
	/* I hate this code, and it hates me.  */
	while (spdaddr < spdend) {
		hdr = SPD_REG8(spdaddr) & 0xff;
		spdaddr += 2;
		if (!hdr)
			continue;
		if (hdr == 0xff)
			break;
		if (spdaddr >= spdend)
			goto error;

		ofs = SPD_REG8(spdaddr) & 0xff;
		spdaddr += 2;
		if (ofs == 0xff)
			break;

		next = spdaddr + (ofs * 2);
		if (next >= spdend)
			goto error;

		if (hdr == 0x20) {
			if ((spdaddr + 8) >= spdend)
				goto error;

			tuple = (SPD_REG8(spdaddr + 2) << 24)|
				(SPD_REG8(spdaddr) << 16)|
				(SPD_REG8(spdaddr + 6) << 8)|
				 SPD_REG8(spdaddr + 4);
			if (manfid == tuple)
				return 0;
			return -1;
		}
		spdaddr = next;
	}
	return -1;
error:
	return -1;
}

static int pcmcia_intr(void *unused)
{
	USE_DEV9_REGS;
	u16 cstc1, cstc2;

	cstc1 = DEV9_REG(DEV9_R_1464);
	cstc2 = DEV9_REG(DEV9_R_1466);

	if (using_aif) {
		if (_lh(0xb4000004) & 0x04)
			_sh(0x04, 0xb4000004);
		else
			return 0;		/* Unknown interrupt.  */
	}

	/* Acknowledge the interrupt.  */
	DEV9_REG(DEV9_R_1464) = cstc1;
	DEV9_REG(DEV9_R_1466) = cstc2;
	if (cstc1 & 0x03 || cstc2 & 0x03) {	/* Card removed or added? */
			dev9_intr_dispatch(1);
		dev9Shutdown();			/* Shutdown the card.  */
		pcmcia_device_probe();
	}
	if (cstc1 & 0x80 || cstc2 & 0x80) {
			dev9_intr_dispatch(0);
	}

	DEV9_REG(DEV9_R_147E) = 1;
	DEV9_REG(DEV9_R_147E) = 0;
	return 1;
}

static int pcmcia_init(void)
{
	USE_DEV9_REGS;
	int *mode;
	int flags;
	u16 cstc1, cstc2;

	_sw(0x51011, SSBUS_R_1420);
	_sw(0x1a00bb, SSBUS_R_1418);
	_sw(0xef1a3043, SSBUS_R_141c);

	/* If we are a T10K, then we go through AIF.  */
	if ((mode = QueryBootMode(6)) != NULL) {
		if ((*(u16 *)mode & 0xfe) == 0x60) {
			if (_lh(0xb4000000) == 0xa1) {
				_sh(4, 0xb4000006);
				using_aif = 1;
			} else {
				return 1;
			}
		}
	}

	if (DEV9_REG(DEV9_R_146C) == 0) {
		DEV9_REG(DEV9_R_146C) = 0;
		DEV9_REG(DEV9_R_147E) = 1;
		DEV9_REG(DEV9_R_1460) = 0;
		DEV9_REG(DEV9_R_1474) = 0;
		DEV9_REG(DEV9_R_1464) = cstc1 = DEV9_REG(DEV9_R_1464);
		DEV9_REG(DEV9_R_1466) = cstc2 = DEV9_REG(DEV9_R_1466);
		DEV9_REG(DEV9_R_1468) = 0x10;
		DEV9_REG(DEV9_R_146A) = 0x90;
		DEV9_REG(DEV9_R_147C) = 1;
		DEV9_REG(DEV9_R_147A) = DEV9_REG(DEV9_R_147C);

		pcic_voltage = pcic_get_voltage();
		pcic_cardtype = pcic_get_cardtype();

		if (smap_device_init() != 0)
			return 1;
	} else {
		_sw(0xe01a3043, SSBUS_R_1418);
	}

	if (smap_subsys_init() != 0)
		return 1;

	CpuSuspendIntr(&flags);
	RegisterIntrHandler(DEV9_INTR, 1, pcmcia_intr, NULL);
	EnableIntr(DEV9_INTR);
	CpuResumeIntr(flags);

	DEV9_REG(DEV9_R_147E) = 0;
	return 0;
}

static void expbay_set_stat(int stat)
{
	USE_DEV9_REGS;
	DEV9_REG(DEV9_R_1464) = stat & 0x3f;
}

static int expbay_device_probe()
{
	USE_DEV9_REGS;
	return (DEV9_REG(DEV9_R_1462) & 0x01) ? -1 : 0;
}

static int expbay_device_reset(void)
{
	USE_DEV9_REGS;

	if (expbay_device_probe() < 0)
		return -1;

	DEV9_REG(DEV9_R_146C) = (DEV9_REG(DEV9_R_146C) & 0xfffe) | 0x04;
	DelayThread(500000);

	DEV9_REG(DEV9_R_1460) = DEV9_REG(DEV9_R_1460) | 0x01;
	DEV9_REG(DEV9_R_146C) = DEV9_REG(DEV9_R_146C) | 0x01;
	DelayThread(500000);
	return 0;
}

static int expbay_intr ( void* apUnused ) {

 USE_DEV9_REGS;
 USE_SPD_REGS;

 int i,     bit;
 u16 lStat, lMask;

 while (   (  lStat = SPD_REG16( SPD_R_INTR_STAT )  ) & (  lMask = SPD_REG16( SPD_R_INTR_MASK )  )   )

  for (  i = 0; i < sizeof ( dev9_intr_cbs ) / sizeof ( dev9_intr_cbs[ 0 ] ); ++i  )

   if ( dev9_intr_cbs[ i ] ) {

    bit = ( lStat & lMask ) >> i;

    if ( bit & 1 ) dev9_intr_cbs[ i ] ( 0 );

   }  /* end if */

 DEV9_REG( DEV9_R_1466 ) = 1;
 DEV9_REG( DEV9_R_1466 ) = 0;

 return 1;

}  /* end expbay_intr */

static int expbay_init ( void ) {

 USE_DEV9_REGS;

 int flags;

 _sw( 0x00051011, SSBUS_R_1420 );
 _sw( 0xE01A3043, SSBUS_R_1418 );
 _sw( 0xEF1A3043, SSBUS_R_141c );

 if (   (  DEV9_REG( DEV9_R_146C ) & 0x04  ) == 0   ) {

  DEV9_REG( DEV9_R_1466 ) = 1;
  DEV9_REG( DEV9_R_1464 ) = 0;
  DEV9_REG( DEV9_R_1460 ) = DEV9_REG( DEV9_R_1464 );

  if (  smap_device_init ()  ) return 1;

 }  /* end if */

 if (  smap_subsys_init () != 0  ) return 1;

 CpuSuspendIntr ( &flags );
  RegisterIntrHandler ( DEV9_INTR, 1, expbay_intr, dev9_intr_cbs );
  EnableIntr ( DEV9_INTR );
 CpuResumeIntr ( flags );

 DEV9_REG( DEV9_R_1466 ) = 0;

 return 0;

}  /* end expbay_init */

static iop_device_ops_t s_DriverOps = {
 (  int ( * )( iop_device_t*                                 )  )dummy,       /* init    */
 (  int	( * )( iop_device_t*                                 )  )dummy,       /* deinit  */
 (  int	( * )( iop_file_t*, ...                              )  )dummy,       /* format  */
 (  int	( * )( iop_file_t*, const char*, int, ...            )  )dummy,       /* open    */
 (  int	( * )( iop_file_t*                                   )  )dummy,       /* close   */
 (  int	( * )( iop_file_t*, void*, int                       )  )dummy,       /* read    */
 (  int	( * )( iop_file_t*, void*, int                       )  )dummy,       /* write   */
 (  int	( * )( iop_file_t*, unsigned long, int               )  )dummy,       /* lseek   */
 (  int	( * )( iop_file_t*, unsigned long, void*             )  )dev9_ioctl,  /* ioctl   */
 (  int	( * )( iop_file_t*, const char*                      )  )dummy,       /* remove  */
 (  int	( * )( iop_file_t*, const char*                      )  )dummy,       /* mkdir   */
 (  int	( * )( iop_file_t*, const char*                      )  )dummy,       /* rmdir   */
 (  int	( * )( iop_file_t*, const char*                      )  )dummy,       /* dopen   */
 (  int	( * )( iop_file_t*                                   )  )dummy,       /* dclose  */
 (  int	( * )( iop_file_t*, void*                            )  )dummy,       /* dread   */
 (  int	( * )( iop_file_t*, const char*, void*               )  )dummy,       /* getstat */
 (  int	( * )( iop_file_t*, const char*, void*, unsigned int )  )dummy        /* chstat  */
};

static iop_device_t s_Driver = { "dev9x", IOP_DT_FS, 2, "DEV9", &s_DriverOps };

static int dev9_ioctl ( iop_file_t* apFile, unsigned long aCmd, void* apParam ) {

 if ( aCmd == DEV9CTLSHUTDOWN )
  dev9Shutdown ();
 else if ( aCmd == DEV9CTLTYPE )
  return dev9type;
 else if ( aCmd == DEV9CTLINIT )
  return dev9Init ();

 return 0;

}  /* end dev9x_fs_devctl */
