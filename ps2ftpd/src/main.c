/*
 * main.c - FTP startup
 *
 * Copyright (C) 2004 Jesper Svennevid
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * c0cfb60b c7d2fe44 c3c9fc45
 */

#include "FtpServer.h"

#ifndef LINUX
#include "irx_imports.h"
#else
#include <stdio.h>
#endif

int server(void* argv);

#ifndef LINUX

// placed out here to end up on the heap, not the stack
FtpServer srv;

int process_args( FtpServer* pServer, int argc, char* argv[] )
{
	argc--; argv++;

	while( argc > 0 )
	{
		if(!strcmp("-port",argv[0]))
		{
			if( argc <= 0 )
				return -1;

			argc--; argv++;
			FtpServer_SetPort( pServer, strtol(argv[0],NULL,10) );
		}
		else if(!strcmp("-anonymous",argv[0]))
		{
			// enable anonymous logins
			FtpServer_SetAnonymous( pServer, 1 );
		}
		else if(!strcmp("-user",argv[0]))
		{
			// set new username

			if( argc <= 0 )
				return -1;

			argc--; argv++;
			FtpServer_SetUsername( pServer, argv[0] );
		}
		else if(!strcmp("-pass",argv[0]))
		{
			// set new password

			if( argc <= 0 )
				return -1;

			argc--; argv++;
			FtpServer_SetPassword( pServer, argv[0] );
		}

		argc--;
		argv++;
	}

	return 0;
}

s32 _start(int argc, char* argv[])
{
  iop_thread_t mythread;
  int pid;
  int i;

	// TODO: fix CD/DVD support
	// printf("cdinit: %d\n",sceCdInit(CdMmodeDvd));

	printf("ps2ftpd: starting\n");

	// create server

	FtpServer_Create(&srv);

	// process arguments

	if( process_args( &srv, argc, argv ) < 0 )
	{
		printf("ps2ftpd: could not parse arguments\n" );
		return MODULE_NO_RESIDENT_END;
	}

	// setup server

	FtpServer_SetPort(&srv,21);


	if( -1 == FtpServer_Start(&srv) )
	{
		FtpServer_Destroy(&srv);
		printf("ps2ftpd: could not create server\n" );
		return MODULE_NO_RESIDENT_END;
	}

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
			FtpServer_Destroy(&srv);
      printf("ps2ftpd: StartThread failed (%d)\n", i);
      return MODULE_NO_RESIDENT_END;
    }
  } 
  else 
  {
		FtpServer_Destroy(&srv);
    printf("ps2ftpd: CreateThread failed (%d)\n", pid);
    return MODULE_NO_RESIDENT_END;
  }
    
	return MODULE_RESIDENT_END;
}


int server(void* argv)
{
	// run server

	while(FtpServer_IsRunning(&srv))
		FtpServer_HandleEvents(&srv);

	FtpServer_Destroy(&srv);
	return 0;
}

#else

int main(int argc, char* argv[])
{
	FtpServer srv;

	// create server

	FtpServer_Create(&srv);

	// setup server

	FtpServer_SetPort(&srv,2500); // 21 privileged under linux
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

#endif

