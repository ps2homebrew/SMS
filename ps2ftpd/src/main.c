/*
 * main.c - FTP startup
 *
 * Copyright (C) 2004 Jesper Svennevid
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 */

#include "FtpServer.h"

#ifndef LINUX
#include "irx_imports.h"
#else
#include <stdio.h>
#endif

int server(void* argv);

#ifndef LINUX
s32 _start(char **argv, int argc)
{
  iop_thread_t mythread;
  int pid;
  int i;

	printf("ps2ftpd: starting\n");

  // Start socket server thread

  mythread.attr = 0x02000000; // attr
  mythread.option = 0; // option
  mythread.thread = (void *)server; // entry
  mythread.stacksize = 0x1000;
  mythread.priority = 0x43; // just above ps2link

  pid = CreateThread(&mythread);

  if (pid > 0) 
  {
    if ((i=StartThread(pid, NULL)) < 0) 
    {
      printf("ps2ftpd: StartThread failed (%d)\n", i);
      return MODULE_NO_RESIDENT_END;
    }
  } 
  else 
  {
    printf("ps2ftpd: CreateThread failed (%d)\n", pid);
    return MODULE_NO_RESIDENT_END;
  }
    
	return MODULE_RESIDENT_END;
}
#endif

int main(int argc, char* argv[])
{
	return server(NULL);
}

// placed out here to end up on the heap, not the stack
FtpServer srv;
int server(void* argv)
{
	// create server

	FtpServer_Create(&srv);

	// setup server

#ifndef LINUX
	FtpServer_SetPort(&srv,21);
#else
	FtpServer_SetPort(&srv,2500); // 21 privileged under linux
#endif

	if( -1 == FtpServer_Start(&srv) )
	{
		FtpServer_Destroy(&srv);
		return 1;
	}

	// run server

	while(FtpServer_IsRunning(&srv))
		FtpServer_HandleEvents(&srv);

	FtpServer_Destroy(&srv);
	return 0;
}
