/*
 * sif.h - SIF protypes header file
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_SIF_H
#define TGE_SIF_H

#include "tge_sifdma.h"

int sif_set_dma(tge_sifdma_transfer_t *dmat, u32 tcount);

void sif_set_dchain(void);

u32 sif_set_reg(u32 reg, u32 val);
u32 sif_get_reg(u32 reg);

int sif_cmd_init(void);
int sif_cmd_exit(void);

u32 sif_cmd_send(u32 fid, void *packet, u32 packet_size, void *src, void *dest, u32 size);

void sif_cmd_add_handler(u32 fid, void *handler, void *harg);
u32 sif_cmd_get_sreg(u32 reg);

#endif /* TGE_SIF_H */
