/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
	sys_ps2.c -- PlayStation 2 sys driver
	
	by Nicolas Plourde a.k.a nic067 <nicolasplourde@hotmail.com>
	
	See http://www.ps2dev.org for all your ps2 coding need.
*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <fileio.h>

#include "quakedef.h"
#include "errno.h"

cvar_t  sys_linerefresh = {"sys_linerefresh","0"};// set for entity display

qboolean			isDedicated;

void LoadModules(void)
{
    int ret;

	ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: SIO2MAN");
		SleepThread();
	}

	ret = SifLoadModule("rom0:MCMAN", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: MCMAN");
		SleepThread();
	}

	ret = SifLoadModule("rom0:MCSERV", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: MCSERV");
		SleepThread();
	}


	ret = SifLoadModule("host:irx/usbd.irx", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: USBD");
		SleepThread();
	}

	ret = SifLoadModule("host:irx/ps2kbd.irx", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: PS2KBD");
		SleepThread();
	}	

	ret = SifLoadModule("host:irx/ps2mouse.irx", 0, NULL);
	if (ret < 0) {
		printf("Failed to load module: PS2MOUSE");
		SleepThread();
	}
}

/*
===============================================================================

FILE IO

===============================================================================
*/

#define MAX_HANDLES 10
int sys_handles[MAX_HANDLES];

void inithandle (void)
{
	int i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
	{
		sys_handles[i] =-1;

	}
}

int findhandle (void)
{
	int i;
	
	for (i=1 ; i<MAX_HANDLES ; i++)
	{
		if(sys_handles[i] == -1)
		{
			return i;
		}
	}
	Sys_Error ("out of handles");
	return -1;
}

/*
================
filelength
================
*/
int filelength (int f)
{
	int pos;
	int end;
	
	end = fioLseek(f,0,SEEK_END);
	pos = fioLseek(f,0,SEEK_SET);

	return end;
}

int Sys_FileOpenRead (char *path, int *hndl)
{
	int f;
	int i;
	
	i = findhandle ();

	f = fioOpen(path,O_RDONLY);
	if (!f)
	{
		*hndl = -1;
		return -1;
	}
	sys_handles[i] = f;
	*hndl = i;
	
	if(filelength(f) < 0)
		return -1;
		
	return filelength(f);
}

int Sys_FileOpenWrite (char *path)
{
	int    f;
	int             i;
	
	i = findhandle ();

	f = fioOpen(path,O_WRONLY | O_CREAT);
	//FIXME
	//if(!f)
	//{
	//	Sys_Error ("Error opening %s: %s", path,strerror(errno));
	//}
	sys_handles[i] = f;
	
	return i;
}

void Sys_FileClose (int handle)
{
	fioClose(sys_handles[handle]);
	sys_handles[handle] = -1;
}

void Sys_FileSeek (int handle, int position)
{
	fioLseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fioRead(sys_handles[handle], dest, count);
}

int Sys_FileWrite (int handle, void *data, int count)
{
	return fioWrite(sys_handles[handle], data, count);
}

int     Sys_FileTime (char *path)
{
	FILE    *f;
	
	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}
	
	return -1;
}

void Sys_mkdir (char *path)
{
	fioMkdir(path);
}


/*
===============================================================================

SYSTEM IO

===============================================================================
*/

void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{
}


void Sys_Error (char *error, ...)
{
	va_list         argptr;

	printf ("Sys_Error: ");   
	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}

void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	
	va_start (argptr,fmt);
	vprintf (fmt,argptr);
	va_end (argptr);
}

void Sys_Quit (void)
{
	exit (0);
}

double Sys_FloatTime (void)
{
	static double t;
	
	t += 0.1;
	
	return t;
}

char *Sys_ConsoleInput (void)
{
	return NULL;
}

void Sys_Sleep (void)
{
}

void Sys_SendKeyEvents (void)
{
}

void Sys_HighFPPrecision (void)
{
}

void Sys_LowFPPrecision (void)
{
}

//=============================================================================

int main (int argc, char **argv)
{
	static quakeparms_t    parms;

	signal(SIGFPE, SIG_IGN);
	SifInitRpc(0);
	LoadModules();
/*
	if(mcInit(MC_TYPE_MC) < 0) 
	{
		printf("Failed to initialise memcard\n");
		SleepThread();
	}
*/
	inithandle();
	
	parms.memsize = 8*1024*1024;
	parms.membase = malloc (parms.memsize);
	parms.basedir = ".";

	COM_InitArgv (argc, argv);

	parms.argc = com_argc;
	parms.argv = com_argv;

	printf ("Host_Init\n");
	Host_Init (&parms);
	while (1)
	{
		Host_Frame (0.1);
	}
	
	return 0;
}


