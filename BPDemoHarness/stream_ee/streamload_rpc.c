/*

streamload.h - EE RPC functions for streamload IRX

Copyright (c) 2004 adresd <adresd_ps2dev@yahoo.com>

*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <stdarg.h>
#include <string.h>
#include <loadfile.h>

#include "streamload.h"

#define BUFFER_SIZE 2048

static unsigned sbuff[64] __attribute__((aligned (64)));
static u8 fftdata[BUFFER_SIZE] __attribute__((aligned (64)));
static struct t_SifRpcClientData cd0;

int streamload_inited = 0;

// StreamLoad Functions
int StreamLoad_loadModules(int cdmode)
{
  static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20";
  static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40"/* "\0" "-debug"*/;

  SifLoadModule("rom0:SIO2MAN", 0, NULL);
  SifLoadModule("rom0:MCMAN", 0, NULL);
  SifLoadModule("rom0:MCSERV", 0, NULL);
  SifLoadModule("rom0:LIBSD", 0, NULL);
  SifLoadModule("host:SJPCMSTR.IRX", 0, NULL);

  if (cdmode == 1)
  {
    SifLoadModule("rom0:CDVDMAN", 0, NULL);
  }
  if (cdmode == 0)
  {
//SifLoadModule("mc0:/SYS-MODULES/IOMANX.IRX", 0, NULL);
  SifLoadModule("mc0:/SYS-MODULES/FILEXIO.IRX", 0, NULL);

//SifLoadModule("mc0:/SYS-MODULES/PS2DEV9.IRX", 0, NULL);
  SifLoadModule("mc0:/SYS-MODULES/PS2ATAD.IRX", 0, NULL);
  SifLoadModule("mc0:/SYS-MODULES/PS2HDD.IRX", sizeof(hddarg), hddarg);
  SifLoadModule("mc0:/SYS-MODULES/PS2FS.IRX", sizeof(pfsarg), pfsarg);
  }
  return 0;
}

int StreamLoad_Init(int cdmode,char *partitionname, int palmode)
{
  int i;
  int rv;
  int sync;
  SifInitRpc(0);

  rv = StreamLoad_loadModules(cdmode);
  if (rv < 0)
  {
    printf("ERROR: failed to load modules: %d\n",rv);
    return -1;
  }

  sync = 0;
  while(1){
    if (SifBindRpc( &cd0, STREAMLOAD_IRX, 0) < 0) return -1; // bind error
    if (cd0.server != 0) break;
    i = 0x10000;
    while(i--);
  }

  sbuff[0] = sync;
  sbuff[1] = cdmode;
  sbuff[2] = palmode;
  strncpy( ((char *)sbuff)+12,partitionname,64-12);

  SifCallRpc(&cd0,STREAMLOAD_INIT,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);

  FlushCache(0);

  streamload_inited = 1;
  return 0;
}
void StreamLoad_Close()
{
  if(!streamload_inited) return;

  // Shutdown STREAMLOAD
  SifCallRpc(&cd0,STREAMLOAD_QUIT,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
  streamload_inited = 0;
}
void StreamLoad_SetupTune(char *pathname)
{
  // This has to be done after pfs is mounted
  if(!streamload_inited) return;

  memcpy((char*)(&sbuff[0]),pathname,64);
  SifCallRpc(&cd0,STREAMLOAD_TUNE,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
}

void StreamLoad_Play(unsigned int volume)
{
  if(!streamload_inited) return;

  sbuff[0] = volume&0x3fff;
  SifCallRpc(&cd0,STREAMLOAD_PLAY,0,(void*)(&sbuff[0]),8,(void*)(&sbuff[0]),8,0,0);
}
void StreamLoad_Pause()
{
  if(!streamload_inited) return;

  SifCallRpc(&cd0,STREAMLOAD_PAUSE,0,(void*)(&sbuff[0]),0,(void*)(&sbuff[0]),0,0,0);
}

int StreamLoad_Position()
{
  if (!streamload_inited) return -1;
  SifCallRpc(&cd0,STREAMLOAD_GETPOS,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
  return sbuff[3];
}

void StreamLoad_SetPosition(int position)
{
  if (!streamload_inited) return;
  sbuff[3] = position;
  SifCallRpc(&cd0,STREAMLOAD_SETPOS,0,(void*)(&sbuff[0]),64,(void*)(&sbuff[0]),64,0,0);
}

u16 *StreamLoad_GetFFT()
{
  if(!streamload_inited) return NULL;
  SifCallRpc(&cd0, STREAMLOAD_GETFFT, 0, (void *)(&sbuff[0]),64, (void *)(&fftdata[0]), BUFFER_SIZE, 0 ,0); 
  FlushCache(0);
  return (u16 *) fftdata;
}
