/*
 * tge_sifdma.h - TGE SIF DMA header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_SIFDMA_H
#define TGE_SIFDMA_H

#define TGE_SIFDMA_ATTR_INT_I	0x02
#define TGE_SIFDMA_ATTR_INT_O	0x04
#define TGE_SIFDMA_ATTR_DMATAG	0x20
#define TGE_SIFDMA_ATTR_ERT	0x40

typedef struct {
	void	*src;
	void	*dest;
	u32	size;
	u32	attr;
} tge_sifdma_transfer_t;

#endif /* TGE_SIFDMA_H */
