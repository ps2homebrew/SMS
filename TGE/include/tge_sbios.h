/*
 * tge_sbios.h -  Main SBIOS interface - types, definitions and core prototypes.
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */


#ifndef TGE_SBIOS_H
#define TGE_SBIOS_H

#include "tge_types.h"
#include "tge_defs.h"

#define TGE_SBIOS_VERSION	0x666

/* Calls dispatched through tge_sbios().  */
typedef enum {
	/* Misc. calls.  */
	TGE_SBCALL_GETVER,
	TGE_SBCALL_HALT,
	TGE_SBCALL_SETDVE,
	TGE_SBCALL_PUTC,
	TGE_SBCALL_GETC,
	TGE_SBCALL_SETGSCRT,
	TGE_SBCALL_SETRGBYC,

	/* SIF initialization and DMA.  */
	TGE_SBCALL_SIFINIT = 16,
	TGE_SBCALL_SIFEXIT,
	TGE_SBCALL_SIFSETDMA,
	TGE_SBCALL_SIFDMASTAT,
	TGE_SBCALL_SIFSETDCHAIN,
	TGE_SBCALL_SIFSETREG,
	TGE_SBCALL_SIFGETREG,
	TGE_SBCALL_SIFSTOPDMA,

	/* SIF command interface.  */
	TGE_SBCALL_SIFINITCMD = 32,
	TGE_SBCALL_SIFEXITCMD,
	TGE_SBCALL_SIFSENDCMD,
	TGE_SBCALL_SIFCMDINTRHDLR,
	TGE_SBCALL_SIFADDCMDHANDLER,
	TGE_SBCALL_SIFREMOVECMDHANDLER,
	TGE_SBCALL_SIFSETCMDBUFFER,
	TGE_SBCALL_SIFSETSREG,
	TGE_SBCALL_SIFGETSREG,
	TGE_SBCALL_SIFGETDATATABLE,
	TGE_SBCALL_SIFSETSYSCMDBUFFER,

	/* SIF RPC.  */
	TGE_SBCALL_SIFINITRPC = 48,
	TGE_SBCALL_SIFEXITRPC,
	TGE_SBCALL_SIFGETOTHERDATA,
	TGE_SBCALL_SIFBINDRPC,
	TGE_SBCALL_SIFCALLRPC,
	TGE_SBCALL_SIFCHECKSTATRPC,
	TGE_SBCALL_SIFSETRPCQUEUE,
	TGE_SBCALL_SIFREGISTERRPC,
	TGE_SBCALL_SIFREMOVERPC,
	TGE_SBCALL_SIFREMOVERPCQUEUE,
	TGE_SBCALL_SIFGETNEXTREQUEST,
	TGE_SBCALL_SIFEXECREQUEST,

	/* IOP heap RPC.  */
	TGE_SBCALL_IOPHEAPINIT = 64,
	TGE_SBCALL_IOPHEAPALLOC,
	TGE_SBCALL_IOPHEAPFREE,

	/* Pad RPC.  */
	TGE_SBCALL_PADINIT = 80,
	TGE_SBCALL_PADEND,
	TGE_SBCALL_PADPORTOPEN,
	TGE_SBCALL_PADPORTCLOSE,
	TGE_SBCALL_PADSETMAINMODE,
	TGE_SBCALL_PADSETACTDIRECT,
	TGE_SBCALL_PADSETACTALIGN,
	TGE_SBCALL_PADINFOPRESSMODE,
	TGE_SBCALL_PADENTERPRESSMODE,
	TGE_SBCALL_PADEXITPRESSMODE,
	TGE_SBCALL_PADREAD,
	TGE_SBCALL_PADGETSTATE,
	TGE_SBCALL_PADGETREQSTATE,
	TGE_SBCALL_PADINFOACT,
	TGE_SBCALL_PADINFOCOMB,
	TGE_SBCALL_PADINFOMODE,

	/* Sound (freesd/sdr) RPC.  */
	TGE_SBCALL_SOUNDINIT = 112,
	TGE_SBCALL_SOUNDEND,
	TGE_SBCALL_SOUNDGETREG,
	TGE_SBCALL_SOUNDSETREG,
	TGE_SBCALL_SOUNDGETCOREATTR,
	TGE_SBCALL_SOUNDSETCOREATTR,
	TGE_SBCALL_SOUNDTRANSFER,
	TGE_SBCALL_SOUNDTRANSFERSTAT,
	TGE_SBCALL_SOUNDTRANSFERCALLBACK,
	TGE_SBCALL_SOUNDVOICETRANSFER,
	TGE_SBCALL_SOUNDVOICETRANFERSTAT,
	TGE_SBCALL_SOUNDREMOTE,

	/* Memory Card RPC.  */
	TGE_SBCALL_MCINIT = 144,
	TGE_SBCALL_MCOPEN,
	TGE_SBCALL_MCMKDIR,
	TGE_SBCALL_MCCLOSE,
	TGE_SBCALL_MCSEEK,
	TGE_SBCALL_MCREAD,
	TGE_SBCALL_MCWRITE,
	TGE_SBCALL_MCGETINFO,
	TGE_SBCALL_MCGETDIR,
	TGE_SBCALL_MCFORMAT,
	TGE_SBCALL_MCDELETE,
	TGE_SBCALL_MCFLUSH,
	TGE_SBCALL_MCSETFILEINFO,
	TGE_SBCALL_MCRENAME,
	TGE_SBCALL_MCUNFORMAT,
	TGE_SBCALL_MCGETENTSPACE,
	TGE_SBCALL_MCRPC,

	/* CD/DVD RPC.  */
	TGE_SBCALL_CDVDINIT = 176,
	TGE_SBCALL_CDVDRESET,
	TGE_SBCALL_CDVDREADY,
	TGE_SBCALL_CDVDREAD,
	TGE_SBCALL_CDVDSTOP,
	TGE_SBCALL_CDVDGETTOC,
	TGE_SBCALL_CDVDREADRTC,
	TGE_SBCALL_CDVDWRITERTC,
	TGE_SBCALL_CDVDMMODE,
	TGE_SBCALL_CDVDGETERROR,
	TGE_SBCALL_CDVDGETDISCTYPE,
	TGE_SBCALL_CDVDTRAYREQUEST,
	TGE_SBCALL_CDVDPOWERHOOK,
	TGE_SBCALL_CDVDDASTREAM,
	TGE_SBCALL_CDVDREADSUBQ,
	TGE_SBCALL_CDVDOPENCONFIG,
	TGE_SBCALL_CDVDCLOSECONFIG,
	TGE_SBCALL_CDVDREADCONFIG,
	TGE_SBCALL_CDVDWRITECONFIG,
	TGE_SBCALL_CDVDRCBYPASSCTL,

	/* Remote controller (SIO2 and Mechacon) RPC.  */
	TGE_SBCALL_REMOCONINIT = 208,
	TGE_SBCALL_REMOCONEND,
	TGE_SBCALL_REMOCONPORTOPEN,
	TGE_SBCALL_REMOCONPORTCLOSE,
	TGE_SBCALL_REMOCONREAD,
	TGE_SBCALL_REMOCON2INIT,
	TGE_SBCALL_REMOCON2END,
	TGE_SBCALL_REMOCON2PORTOPEN,
	TGE_SBCALL_REMOCON2PORTCLOSE,
	TGE_SBCALL_REMOCON2READ,
	TGE_SBCALL_REMOCON2IRFEATURE
} tge_sbcall_t;

/* The calls listed above accept specific arguments passed to tge_sbios()'s
   arg argument.  Also, the calls that are SIF RPC client calls use the
   following structure, with the call-specific argument passed in sbarg.  These
   calls are marked with "[RPC]" below.  See the documentation for RPC calls
   that take no arguments.  */
typedef struct {
	int	result;
	void	*sbarg;		/* The tge_sbcall_*_arg_t.  */
	void	(*endfunc)(void *efarg, int result);
	void	*efarg;
} tge_sbcall_rpc_arg_t;


/* TGE_SBCALL_HALT */
typedef enum {
	TGE_SB_HALT_MODE_POWEROFF = 0,
	TGE_SB_HALT_MODE_HALT,
	TGE_SB_HALT_MODE_RESTART,
} tge_sb_halt_mode_t;

typedef struct {
	tge_sb_halt_mode_t mode;
} tge_sbcall_halt_arg_t;

/* TGE_SBCALL_SETDVE */
typedef struct {
	int	mode;
} tge_sbcall_setdve_arg_t;

/* TGE_SBCALL_PUTC */
typedef struct {
	int	c;
} tge_sbcall_putc_arg_t;

/* TGE_SBCALL_SETGSCRT */
typedef struct {
	int	interlace;
	int	output_mode;
	int	field_frame_mode;
	int	*dx1, *dy1, *dx2, *dy2;
} tge_sbcall_setgscrt_arg_t;

/* TGE_SBCALL_SETRGBYC */
typedef struct {
	int	rgbyc;
} tge_sbcall_setrgbyc_arg_t;


/* SIF DMA.  */

/* TGE_SBCALL_SIFSETDMA */
typedef struct {
	void	*transfer;/* This matches tge_sifdma_transfer_t.  */
	u32	tcount;	/* The number of transfers we point to.  */
} tge_sbcall_sifsetdma_arg_t;

/* TGE_SBCALL_SIFDMASTAT */
typedef struct {
	int	id;	/* The transfer id returned from sbcall_sifsetdma().  */
} tge_sbcall_sifdmastat_arg_t;

/* TGE_SBCALL_SIFSETREG */
typedef struct {
	u32	reg;
	u32	val;
} tge_sbcall_sifsetreg_arg_t;

/* TGE_SBCALL_SIFGETREG */
typedef struct {
	u32	reg;
} tge_sbcall_sifgetreg_arg_t;


/* SIF Command.  */

/* TGE_SBCALL_SIFSENDCMD */
typedef struct {
	u32	fid;
	void	*packet;
	u32	packet_size;
	void	*src;
	void	*dest;
	u32	size;
} tge_sbcall_sifsendcmd_arg_t;

/* TGE_SBCALL_SIFADDCMDHANDLER */
typedef struct {
	u32	fid;
	void	*handler;
	void	*harg;
} tge_sbcall_sifaddcmdhandler_arg_t;

/* TGE_SBCALL_SIFREMOVECMDHANDLER */
typedef struct {
	u32	fid;
} tge_sbcall_sifremovecmdhandler_arg_t;

/* TGE_SBCALL_SIFSETCMDBUFFER */
typedef struct {
	void	*buf;
	u32	size;
} tge_sbcall_sifsetcmdbuffer_arg_t;

/* TGE_SBCALL_SIFSETSREG */
typedef struct {
	u32	reg;
	u32	val;
} tge_sbcall_sifsetsreg_arg_t;

/* TGE_SBCALL_SIFGETSREG */
typedef struct {
	u32     reg;
} tge_sbcall_sifgetsreg_arg_t;

/* TGE_SBCALL_SIFSETSYSCMDBUFFER */
typedef struct {
	void	*buf;
	u32	size;
} tge_sbcall_sifsetsyscmdbuffer_arg_t;


/* SIF RPC.  */

/* TGE_SBCALL_SIFGETOTHERDATA */
typedef struct {
	void	*rd;
	void	*src;
	void	*dest;
	u32	size;
	u32	mode;
	void	*endfunc;
	void	*efarg;
} tge_sbcall_sifgetotherdata_arg_t;

/* TGE_SBCALL_SIFBINDRPC */
typedef struct {
	void	*cd;
	u32	sid;
	u32	mode;
	void	*endfunc;
	void	*efarg;
} tge_sbcall_sifbindrpc_arg_t;

/* TGE_SBCALL_SIFCALLRPC */
typedef struct {
	void	*cd;
	u32	func;
	u32	mode;
	void	*send;
	u32	send_size;
	void	*recv;
	u32	recv_size;
	void	*endfunc;
	void	*efarg;
} tge_sbcall_sifcallrpc_arg_t;

/* TGE_SBCALL_SIFCHECKSTATRPC */
typedef struct {
	void	*cd;
} tge_sbcall_sifcheckstatrpc_arg_t;

/* TGE_SBCALL_SIFSETRPCQUEUE */
typedef struct {
	void	*dq;
	void	*queuefunc;
	void	*qfarg;
} tge_sbcall_sifsetrpcqueue_arg_t;

/* TGE_SBCALL_SIFREGISTERRPC */
typedef struct {
	void	*sd;
	u32	sid;
	void	*func;
	void	*arg;
	void	*cfunc;
	void	*carg;
	void	*dq;
} tge_sbcall_sifregisterrpc_arg_t;

/* TGE_SBCALL_SIFREMOVERPC */
typedef struct {
	void	*sd;
	void	*dq;
} tge_sbcall_sifremoverpc_arg_t;

/* TGE_SBCALL_SIFREMOVERPCQUEUE */
typedef struct {
	void	*dq;
} tge_sbcall_sifremoverpcqueue_arg_t;

/* TGE_SBCALL_SIFGETNEXTREQUEST */
typedef struct {
	void	*dq;
} tge_sbcall_sifgetnextrequest_arg_t;

/* TGE_SBCALL_SIFEXECREQUEST */
typedef struct {
	void	*sd;
} tge_sbcall_sifexecrequest_arg_t;


/* IOP Heap.  */

/* [RPC] TGE_SBCALL_IOPHEAPALLOC */
typedef struct {
	u32	size;
} tge_sbcall_iopheapalloc_arg_t;

/* [RPC] TGE_SBCALL_IOPHEAPFREE */
typedef struct {
	void	*iopaddr;
} tge_sbcall_iopheapfree_arg_t;


/* Pad.  */

/* [RPC] TGE_SBCALL_PADINIT */
typedef struct {
	u32	mode;
} tge_sbcall_padinit_arg_t;

/* [RPC] TGE_SBCALL_PADPORTOPEN */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_padportopen_arg_t;

/* [RPC] TGE_SBCALL_PADPORTCLOSE */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_padportclose_arg_t;

/* [RPC] TGE_SBCALL_PADSETMAINMODE */
typedef struct {
	int	port;
	int	slot;
	int	offset;
	int	lock;
} tge_sbcall_padsetmainmode_arg_t;

/* [RPC] TGE_SBCALL_PADSETACTDIRECT */
typedef struct {
	int	port;
	int	slot;
	void	*data;
} tge_sbcall_padsetactdirect_arg_t;

/* [RPC] TGE_SBCALL_PADSETACTALIGN */
typedef struct {
	int	port;
	int	slot;
	void	*data;
} tge_sbcall_padsetactalign_arg_t;

/* [RPC] TGE_SBCALL_PADINFOPRESSMODE */
/* [RPC] TGE_SBCALL_PADENTERPRESSMODE */
/* [RPC] TGE_SBCALL_PADEXITPRESSMODE */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_padpressmode_arg_t;

/* TGE_SBCALL_PADREAD */
typedef struct {
	int	port;
	int	slot;
	void	*data;
} tge_sbcall_padread_arg_t;

/* TGE_SBCALL_PADGETSTATE */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_padgetstate_arg_t;

/* TGE_SBCALL_PADGETREQSTATE */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_padgetreqstate_arg_t;

/* TGE_SBCALL_PADINFOACT */
typedef struct {
	int	port;
	int	slot;
	int	actno;
	int	term;
} tge_sbcall_padinfoact_arg_t;

/* TGE_SBCALL_PADINFOCOMB */
typedef struct {
	int	port;
	int	slot;
	int	listno;
	int	offset;
} tge_sbcall_padinfocomb_arg_t;

/* TGE_SBCALL_PADINFOMODE */
typedef struct {
	int	port;
	int	slot;
	int	term;
	int	offset;
} tge_sbcall_padinfomode_arg_t;


/* Sound.  */
/* TGE_SBCALL_SOUNDINIT */
typedef enum {
	TGE_SB_SOUNDINIT_MODE_COLD,
	TGE_SB_SOUNDINIT_MODE_HOT
} tge_sb_soundinit_mode_t;

typedef struct {
	tge_sb_soundinit_mode_t mode;
} tge_sbcall_soundinit_arg_t;

/* TGE_SBCALL_SOUNDGETREG */
typedef struct {
	u32	index;
} tge_sbcall_soundgetreg_arg_t;

/* TGE_SBCALL_SOUNDSETREG */
typedef struct {
	u32	index;
	u32	val;
} tge_sbcall_soundsetreg_arg_t;

/* TGE_SBCALL_SOUNDGETCOREATTR */
typedef struct {
	u32	index;
} tge_sbacall_soundgetcoreattr_arg_t;

/* TGE_SBCALL_SOUNDSETCOREATTR */
typedef struct {
	u32	index;
	u32	attr;
} tge_sbacall_soundsetcoreattr_arg_t;

/* TGE_SBCALL_SOUNDTRANSFER */
typedef struct {
	int	channel;
	u32	mode;
	u32	addr;
	u32	size;
	u32	start_addr;
} tge_sbcall_soundtransfer_arg_t;

/* TGE_SBCALL_SOUNDTRANSFERSTAT */
typedef struct {
	int	channel;
	int	flag;
} tge_sbcall_soundtransferstat_arg_t;

/* TGE_SBCALL_SOUNDTRANSFERCALLBACK */
typedef struct {
	int	channel;
	int	(*transfer_cb)(void *, int);
	void	*tcbarg;
	int	(*old_cb)(void *, int);
	void	*oldarg;
} tge_sbcall_soundtransfercallback_arg_t;

/* TGE_SBCALL_SOUNDVOICETRANSFER */
/* TGE_SBCALL_SOUNDVOICETRANFERSTAT */

/* TGE_SBCALL_SOUNDREMOTE */
typedef struct {
	u32	command;
	int	args[126];
} tge_sbcall_soundremote_arg_t;


/* Memory card.  */
/* TGE_SBCALL_MCOPEN */
typedef struct {
	int	port;
	int	slot;
	const char *name;
	u32	mode;
} tge_sbcall_mcopen_arg_t;

/* TGE_SBCALL_MCMKDIR */
typedef struct {
	int	port;
	int	slot;
	const char *name;
} tge_sbcall_mcmkdir_arg_t;

/* TGE_SBCALL_MCCLOSE */
typedef struct {
	int	fd;
} tge_sbcall_mcclose_arg_t;

/* TGE_SBCALL_MCSEEK */
typedef struct {
	int	fd;
	int	offset;
	int	whence;
} tge_sbcall_mcseek_arg_t;

/* TGE_SBCALL_MCREAD */
typedef struct {
	int	fd;
	void	*buf;
	u32	size;
} tge_sbcall_mcread_arg_t;

/* TGE_SBCALL_MCWRITE */
typedef struct {
	int	fd;
	void	*buf;
	u32	size;
} tge_sbcall_mcwrite_arg_t;

/* TGE_SBCALL_MCGETINFO */
typedef struct {
	int	port;
	int	slot;
	int	*type;
	int	*free;
	int	*format;
} tge_sbcall_mcgetinfo_arg_t;

/* TGE_SBCALL_MCGETDIR */
typedef struct {
	int	port;
	int	slot;
	const char *name;
	u32	mode;
	int	max_entries;
	void	*dirents;
} tge_sbcall_mcgetdir_arg_t;

/* TGE_SBCALL_MCFORMAT */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_mcformat_arg_t;

/* TGE_SBCALL_MCDELETE */
typedef struct {
	int	port;
	int	slot;
	const char *name;
} tge_sbcall_mcdelete_arg_t;

/* TGE_SBCALL_MCFLUSH */
typedef struct {
	int	fd;
} tge_sbcall_mcflush_arg_t;

/* TGE_SBCALL_MCSETFILEINFO */
typedef struct {
	int	port;
	int	slot;
	const char *name;
	void	*dirent;
	u32	modes;
} tge_sbcall_mcsetfileinfo_arg_t;

/* TGE_SBCALL_MCRENAME */
typedef struct {
	int	port;
	int	slot;
	const char *origname;
	const char *newname;
} tge_sbcall_mcrename_arg_t;

/* TGE_SBCALL_MCUNFORMAT */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_mcunformat_arg_t;

/* TGE_SBCALL_MCGETENTSPACE */
typedef struct {
	int	port;
	int	slot;
	const char *name;
} tge_sbcall_mcgetentspace_arg_t;

/* TGE_SBCALL_MCRPC */
typedef struct {
	void	*rpcarg;
} tge_sbcall_mcrpc_arg_t;


/* CD/DVD.  */
/* TGE_SBCALL_CDVDREADY */
typedef struct {
	u32	mode;
} tge_sbcall_cdvdready_arg_t;

/* TGE_SBCALL_CDVDREAD */
typedef struct {
	u32	lbn;
	u32	sectors;
	void	*buf;
	void	*readmode;
} tge_sbcall_cdvdread_arg_t;

/* TGE_SBCALL_CDVDGETTOC */
typedef struct {
	void	*buf;
	u32	len;
	int	media;
} tge_sbcall_cdvdgettoc_arg_t;

/* TGE_SBCALL_CDVDWRITERTC */
typedef struct {
	u8	status;
	u8	second;
	u8	minute;
	u8	hour;
	u8	unused;
	u8	day;
	u8	month;
	u8	year;
} tge_sbcall_cdvdwritertc_arg_t;

/* TGE_SBCALL_CDVDMMODE */
typedef struct {
	int	media;
} tge_sbcall_cdvdmmode_arg_t;

/* TGE_SBCALL_CDVDTRAYREQUEST */
typedef struct {
	u32	request;
	u32	traycount;
} tge_sbcall_cdvdtrayrequest_arg_t;

/* TGE_SBCALL_CDVDPOWERHOOK */
typedef struct {
	void	(*powerfunc)(void *);
	void	*pfarg;
} tge_sbcall_cdvdpowerhook_arg_t;

/* TGE_SBCALL_CDVDDASTREAM */
typedef struct {
} tge_sbcall_cdvddastream_arg_t;

/* TGE_SBCALL_CDVDREADSUBQ */
typedef struct {
} tge_sbcall_cdvdreadsubq_arg_t;

/* TGE_SBCALL_CDVDOPENCONFIG */
/* TGE_SBCALL_CDVDREADCONFIG */
/* TGE_SBCALL_CDVDWRITECONFIG */
typedef struct {
	u32	device;
	u32	mode;
	u32	block;
	void	*data;
	int	status;
} tge_sbcall_cdvdconfig_arg_t;

/* TGE_SBCALL_CDVDRCBYPASSCTL */
typedef struct {
	int	param;
	int	status;
} tge_sbcall_cdvdrcbypassctl_arg_t;


/* Remote control.  */
/* TGE_SBCALL_REMOCONINIT */
/* TGE_SBCALL_REMOCON2INIT */
typedef struct {
	u32	mode;
} tge_sbcall_remoconinit_arg_t;

/* TGE_SBCALL_REMOCONPORTOPEN */
/* TGE_SBCALL_REMOCON2PORTOPEN */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_remoconportopen_arg_t;

/* TGE_SBCALL_REMOCONPORTCLOSE */
/* TGE_SBCALL_REMOCON2PORTCLOSE */
typedef struct {
	int	port;
	int	slot;
} tge_sbcall_remoconportclose_arg_t;

/* TGE_SBCALL_REMOCONREAD */
/* TGE_SBCALL_REMOCON2READ */
typedef struct {
	int	port;
	int	slot;
	u32	len;
	void	*buf;
} tge_sbcall_remoconread_arg_t;

/* TGE_SBCALL_REMOCON2IRFEATURE */
typedef struct {
	u8	feature;
} tge_sbcall_remocon2irfeature_arg_t;


extern int (*tge_sbios)(tge_sbcall_t sbcall, void *arg);

#endif /* TGE_SBIOS_H */
