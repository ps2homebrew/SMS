/*
 * tge_hwdefsh - TGE Hardware definitions header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_HWDEFS_H
#define TGE_HWDEFS_H

/* SBUS interrupts relayed to the EE (see intrelay.irx).  */
#define TGE_SBUS_IRQ_DEV9	(1 << 8)
#define TGE_SBUS_IRQ_USB	(1 << 10)
#define TGE_SBUS_IRQ_ILINK	(1 << 12)

#endif /* TGE_HWDEFS_H */
