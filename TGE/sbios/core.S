/*
 * core.S -
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "regs.h"

	.globl	core_dcache_writeback

	.text
	.set	noreorder
	.p2align 3

	.type	core_dcache_writeback,@function
	.ent	core_dcache_writeback, 0

core_dcache_writeback:

	lui	t9, 0xffff
	ori	t9, t9, 0xffc0
	blez	a1, last
	addu	t2, a0, a1
	and	t0, a0, t9
	addiu	t2, t2, -1
	and	t1, t2, t9
	subu	t2, t1, t0
	srl	t3, t2, 0x6
	addiu	t3, t3, 1
	andi	t1, t3, 0x7
	beqz	t1, eight
	srl	t2, t3, 0x3
loop1:
	sync
	cache	0x18, 0(t0)
	sync
	addiu	t1, t1, -1
	nop
	bgtz	t1, loop1
	addiu	t0, t0, 64

eight:
	beqz	t2, last
loop8:
	addiu	t2, t2, -1
	sync
	cache	0x18, 0(t0)
	sync
	cache	0x18, 64(t0)
	sync
	cache	0x18, 128(t0)
	sync
	cache	0x18, 192(t0)
	sync
	cache	0x18, 256(t0)
	sync
	cache	0x18, 320(t0)
	sync
	cache	0x18, 384(t0)
	sync
	cache	0x18, 448(t0)
	sync
	bgtz	t2, loop8
	addiu	t0, t0, 512
last:
	jr	$31
	nop

	.end	core_dcache_writeback
	.size	core_dcache_writeback, .-core_dcache_writeback
