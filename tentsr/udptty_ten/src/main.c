/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003  Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# UDP-based TTY for Pukklink-compatible clients.
*/

#include "udptty.h"

IRX_ID(MODNAME, 1, 1);

udptty_param_t udptty_param = {
	{ UDPTTY_ETH_DST },
	{ UDPTTY_ETH_SRC },
	UDPTTY_IP_ADDR_DST, UDPTTY_IP_ADDR_SRC,
	UDPTTY_IP_PORT_DST, UDPTTY_IP_PORT_SRC
};

int naplinkRpcInit(void);

int _start(int argc, char *argv[])
{
	if( argc==0 || argv[1][0]!='t' || argv[1][1]!='e' || argv[1][2]!='n' )
	{
		close(0);
		close(1);
		DelDrv(DEVNAME);

		if (AddDrv(&tty_device) < 0)
			return 1;

		open(DEVNAME "00:", 0x1000|O_RDWR);
		open(DEVNAME "00:", O_WRONLY);

		printf( "udPutty should work now\n" );
		naplinkRpcInit();
	}
	else
	{
	/*	int i;
		printf( "nArg: %d\n", argc );
		for( i=0;i<argc;i++ )
		{
			printf( "lArg[%d]>> %s\n", i, argv[i] );
		}
	*/
		printf( ">> %s\n", argv[2] );
		return -1;
	}

	return 0;
}
