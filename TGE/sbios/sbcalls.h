/*
 * sbcalls.h - SBIOS call prototypes
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#ifndef TGE_SBCALLS_H
#define TGE_SBCALLS_H

#include "tge_sbios.h"

/* misc.c */
int sbcall_getver(void);
int sbcall_halt(tge_sbcall_halt_arg_t *arg);
int sbcall_setdve(tge_sbcall_setdve_arg_t *arg);
int sbcall_putc(tge_sbcall_putc_arg_t *arg);
int sbcall_getc(void);
int sbcall_setgscrt(tge_sbcall_setgscrt_arg_t *arg);
int sbcall_setrgbyc(tge_sbcall_setrgbyc_arg_t *arg);

/* sifdma.c */
int sbcall_sifinit(void);
int sbcall_sifexit(void);
int sbcall_sifsetdma(tge_sbcall_sifsetdma_arg_t *arg);
int sbcall_sifdmastat(tge_sbcall_sifdmastat_arg_t *arg);
int sbcall_sifsetdchain(void);
u32 sbcall_sifsetreg(tge_sbcall_sifsetreg_arg_t *arg);
u32 sbcall_sifgetreg(tge_sbcall_sifgetreg_arg_t *arg);
int sbcall_sifstopdma(void);

/* sifcmd.c */
int sbcall_sifinitcmd(void);
int sbcall_sifexitcmd(void);
int sbcall_sifsendcmd(tge_sbcall_sifsendcmd_arg_t *arg);
int sbcall_sifcmdintrhdlr(void);
int sbcall_sifaddcmdhandler(tge_sbcall_sifaddcmdhandler_arg_t *arg);
int sbcall_sifremovecmdhandler(tge_sbcall_sifremovecmdhandler_arg_t *arg);
int sbcall_sifsetcmdbuffer(tge_sbcall_sifsetcmdbuffer_arg_t *arg);
int sbcall_sifsetsreg(tge_sbcall_sifsetsreg_arg_t *arg);
int sbcall_sifgetsreg(tge_sbcall_sifgetsreg_arg_t *arg);
int sbcall_sifgetdatatable(void);
int sbcall_sifsetsyscmdbuffer(tge_sbcall_sifsetsyscmdbuffer_arg_t *arg);

#endif /* TGE_SBCALLS_H */
