/*
 * sifcmd.c - Native SIF Command interface.
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

extern u32 sbios_iopaddr;

u32 sif_cmd_send(u32 fid, void *packet, u32 packet_size, void *src, void *dest, u32 size)
{
	return 0;
}

static void sif_cmd_interrupt()
{
}

static u32 sif_cmd_set_sreg(u32 reg, u32 val)
{
	return 0;
}

u32 sif_cmd_get_sreg(u32 reg)
{
	return 0;
}

static void *sif_cmd_data_table()
{
	return NULL;
}

int sif_cmd_init()
{
	return 0;
}

int sif_cmd_exit()
{
	return 0;
}

void sif_cmd_add_handler(u32 fid, void *handler, void *harg)
{
}

static void sif_cmd_del_handler(u32 fid)
{
}

static void *sif_cmd_set_buffer(void *buf, u32 size)
{
	return NULL;
}

static void *sif_cmd_set_sys_buffer(void *buf, u32 size)
{
	return NULL;
}

/* SBIOS interface.  */
int sbcall_sifinitcmd()
{
	return sif_cmd_init();
}

int sbcall_sifexitcmd()
{
	return sif_cmd_exit();
}

int sbcall_sifsendcmd(tge_sbcall_sifsendcmd_arg_t *arg)
{
	return sif_cmd_send(arg->fid, arg->packet, arg->packet_size, arg->src,
			arg->dest, arg->size);
}

int sbcall_sifcmdintrhdlr()
{
	sif_cmd_interrupt();
	return 0;
}

int sbcall_sifaddcmdhandler(tge_sbcall_sifaddcmdhandler_arg_t *arg)
{
	sif_cmd_add_handler(arg->fid, arg->handler, arg->harg);
	return 0;
}

int sbcall_sifremovecmdhandler(tge_sbcall_sifremovecmdhandler_arg_t *arg)
{
	sif_cmd_del_handler(arg->fid);
	return 0;
}

int sbcall_sifsetcmdbuffer(tge_sbcall_sifsetcmdbuffer_arg_t *arg)
{
	return (int)sif_cmd_set_buffer(arg->buf, arg->size);
}

int sbcall_sifsetsreg(tge_sbcall_sifsetsreg_arg_t *arg)
{
	return sif_cmd_set_sreg(arg->reg, arg->val);
}

int sbcall_sifgetsreg(tge_sbcall_sifgetsreg_arg_t *arg)
{
	return sif_cmd_get_sreg(arg->reg);
}

int sbcall_sifgetdatatable()
{
	return (int)sif_cmd_data_table();
}

int sbcall_sifsetsyscmdbuffer(tge_sbcall_sifsetsyscmdbuffer_arg_t *arg)
{
	return (int)sif_cmd_set_sys_buffer(arg->buf, arg->size);
}
