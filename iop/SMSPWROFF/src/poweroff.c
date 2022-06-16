/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: poweroff.c 1377 2007-02-01 07:38:47Z Herben $
*/

#include "types.h"
#include "defs.h"
#include "irx.h"
#include "loadcore.h"
#include "sysmem.h"
#include "stdio.h"
#include "sysclib.h"
#include "sifcmd.h"
#include "thbase.h"
#include "intrman.h"
#include "loadcore.h"
#include "poweroff.h"
#include "../../SMSDev9/include/dev9.h"

//#define DEBUG

#define INT_CDROM	0x02
#define TYPE_C		1
#define CDVDreg_PWOFF	(*(volatile unsigned char*)0xBF402008)

#define POFF_SIF_CMD	18
#define MAX_CALLBACKS	4

IRX_ID("Poweroff_Handler", 1, 1);

extern struct irx_export_table _exp_poweroff;

//---------------------------------------------------------------------

int  _start(int, char**);


//---------------------------------------------------------------------
typedef int (*intrhandler)(void*);

intrhandler	old=0;

struct handlerTableEntry{
	intrhandler	handler;
	void		*param;
};

struct CallbackEntry
{
	pwoffcb cb;
	void *data;
} CallbackTable[MAX_CALLBACKS];

//---------------------------------------------------------------------

static int cmdData[ 4 ];
static int s_ThreadID;

static int myCdHandler(void *param)
{

	if (((CDVDreg_PWOFF & 1)==0) && (CDVDreg_PWOFF & 4))
	{
        cmdData[ 3 ] = 5;
		/* can't seem to register a sif cmd callback in ps2link so... */
		/* Clear interrupt bit */
		CDVDreg_PWOFF = 4;
		isceSifSendCmd(POFF_SIF_CMD, cmdData, 16, NULL, NULL, 0);
		iWakeupThread ( s_ThreadID );
	}

	return old(param);
}

//---------------------------------------------------------------------
//-----------------------------------------------------------entrypoint
//---------------------------------------------------------------------

static void pCallbackThread(void *arg)
{
	int i, h = 0;
	while(1)
	{
		SleepThread ();
		/* Do callbacks in reverse order */
		for(i = MAX_CALLBACKS-1, h = 0; i >= 0; i--)
		{
			if(CallbackTable[i].cb)
			{
				CallbackTable[i].cb(CallbackTable[i].data);
				h = 1;
			}
		}
	}
}

void AddPowerOffHandler(pwoffcb func, void* param)
{
	int i;

	for(i = 0; i < MAX_CALLBACKS; i++)
	{
		if(CallbackTable[i].cb == 0)
		{
			CallbackTable[i].cb = func;
			CallbackTable[i].data = param;
			break;
		}
	}

}

void RemovePowerOffHandler(pwoffcb func)
{
	int i;

	for(i = 0; i < MAX_CALLBACKS; i++)
	{
		if(CallbackTable[i].cb == func)
		{
			break;
		}
	}

	if(i < MAX_CALLBACKS)
	{
		for(; i < (MAX_CALLBACKS-1); i++)
		{
			CallbackTable[i] = CallbackTable[i+1];
		}
		memset(&CallbackTable[i], 0, sizeof(struct CallbackEntry));
	}
}

int _start ( int argc, char* argv[] ) {

 register struct handlerTableEntry* handlers = ( struct handlerTableEntry* )0x480;
 iop_thread_t                       lThreadParam;

 if (  RegisterLibraryEntries ( &_exp_poweroff )  ) return 1;

 FlushDcache ();
 CpuEnableIntr ( 0 );

 if ( handlers[ INT_CDROM ].handler == 0 ) return 1;

 if (   (  ( int )handlers[ INT_CDROM ].handler & 3  ) != TYPE_C   ) return 1;

 old = ( intrhandler )(  ( int )handlers[ INT_CDROM ].handler & ~3  );
 handlers[ INT_CDROM ].handler = ( intrhandler )(  ( int )myCdHandler | TYPE_C  );

 memset (  CallbackTable, 0, sizeof ( struct CallbackEntry ) * MAX_CALLBACKS  );

 lThreadParam.attr      = 0x02000000;
 lThreadParam.option    = 0;
 lThreadParam.thread    = pCallbackThread;
 lThreadParam.stacksize = 0x800;
 lThreadParam.priority  = 0x27;
 StartThread (  s_ThreadID = CreateThread ( &lThreadParam ), NULL  );

 return 0;

}  /* end _start */
