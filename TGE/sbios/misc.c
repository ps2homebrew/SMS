/*
 * misc.c - Misc SBIOS calls
 *
 * Copyright (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
 *
 * See the file LICENSE, located within this directory, for licensing terms.
 */


#include "tge_types.h"
#include "tge_defs.h"

#include "tge_sbios.h"

#include "hwreg.h"

int sbcall_getver()
{
	return TGE_SBIOS_VERSION;
}

static int sio_putc(int c)
{
	/* Wait for free space in the TX FIFO.  */
	while (_lw(EE_SIO_ISR) & 0x8000)
		;

	_sb(c, EE_SIO_TXFIFO);
	return c;
}

/* Output a character over the serial port.  */
int sbcall_putc(tge_sbcall_putc_arg_t *arg)
{
	/* Translate \n to \r\n.  */
	if (arg->c == '\n') {
		sio_putc('\r');
		return sio_putc('\n');
	}

	return sio_putc(arg->c);
}

/* This MUST return 0 if there are no characters in the RX FIFO.  */
int sbcall_getc()
{
	if (_lw(EE_SIO_ISR) & 0xf00)
		return _lb(EE_SIO_RXFIFO);

	return 0;
}

int sbcall_halt(tge_sbcall_halt_arg_t *arg)
{
	return -1;
}

int sbcall_setdve(tge_sbcall_setdve_arg_t *arg)
{
	return -1;
}

int sbcall_setgscrt(tge_sbcall_setgscrt_arg_t *arg)
{
	return -1;
}

int sbcall_setrgbyc(tge_sbcall_setrgbyc_arg_t *arg)
{
	return 0;
}
