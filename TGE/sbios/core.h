/*
 * core.h - TGE EE Core utilities header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_CORE_H
#define TGE_CORE_H

#include "tge_types.h"
#include "regs.h"

/* Get the physical address.  */
#define PHYSADDR(addr)		(((u32)(addr)) & 0x1fffffff)

/* Convert a pointer to an address in KSEG0 or KSEG1.  */
#define KSEG0ADDR(addr)		((__typeof__(addr))(PHYSADDR(addr) | K0BASE))
#define KSEG1ADDR(addr)		((__typeof__(addr))(PHYSADDR(addr) | K1BASE))

/* Disable interrupts and save the previous contents of COP0 Status.  */
static inline void core_save_disable(u32 *status)
{
	__asm__ volatile (
	".set	push\n\t"	\
	".set	noreorder\n\t"	\
	".set	noat\n\t"	\
	"mfc0	%0,$12\n\t"	\
	"ori	$1,%0,1\n\t"	\
	"xori	$1,1\n\t"	\
	"mtc0	$1,$12\n\t"	\
	"sync.p\n\t"		\
	".set	pop\n\t"
	: "=r" (*status) : : "$1", "memory");
}

/* Restore the previous contents of COP0 Status.  */
static inline void core_restore(u32 status)
{
	__asm__ volatile (
	".set	push\n\t"	\
	".set	mips3\n\t"	\
	".set	noreorder\n\t"	\
	"mfc0	$8,$12\n\t"	\
	"li	$9,0xff00\n\t"	\
	"and	$8,$9\n\t"	\
	"nor	$9,$0,$9\n\t"	\
	"and	%0,$9\n\t"	\
	"or	%0,$8\n\t"	\
	"mtc0	%0,$12\n\t"	\
	"sync.p\n\t"		\
	".set	pop\n\t"
	: : "r" (status) : "$8", "$9", "memory");
}

extern void core_dcache_writeback(void *d, u32 size);

#endif /* TGE_CORE_H */
