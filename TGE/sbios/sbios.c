/*
 * sbios.c -
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */

#include "sbcalls.h"

#define SBCALL_MAX	256

static void *dispatch[SBCALL_MAX] __attribute__((section(".text"))) = {
	sbcall_getver, sbcall_halt, sbcall_setdve, sbcall_putc, sbcall_getc,
	sbcall_setgscrt, sbcall_setrgbyc,
	0, 0, 0, 0, 0, 0, 0, 0, 0,

	sbcall_sifinit, sbcall_sifexit, sbcall_sifsetdma, sbcall_sifdmastat,
	sbcall_sifsetdchain, sbcall_sifsetreg, sbcall_sifgetreg, sbcall_sifstopdma,
	0, 0, 0, 0, 0, 0, 0, 0,

	sbcall_sifinitcmd, sbcall_sifexitcmd, sbcall_sifsendcmd, sbcall_sifcmdintrhdlr,
	sbcall_sifaddcmdhandler, sbcall_sifremovecmdhandler, sbcall_sifsetcmdbuffer,
	sbcall_sifsetsreg, sbcall_sifgetsreg, sbcall_sifgetdatatable, sbcall_sifsetsyscmdbuffer,
	0, 0, 0, 0, 0,
};

int sbios(tge_sbcall_t sbcall, void *arg)
{
	int (*sbfunc)(void *) = dispatch[sbcall];

	if (!sbfunc)
		return -1;

	return sbfunc(arg);
}
