/*
 * PbFile.c - File IO functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbFile.h"
#include <tamtypes.h>
#include <fileio.h>
#include <kernel.h>

///////////////////////////////////////////////////////////////////////////////
// int PbFileWrite
///////////////////////////////////////////////////////////////////////////////

int PbFileWrite( const char* pName, void* pData, int size )
{
	int fd;
	fd = fioOpen( pName, O_CREAT|O_WRONLY );
	
	if( fd > 0 )
	{
		if( fioWrite( fd, pData, size) != size )
		{
			fioClose(fd);
			SleepThread();
		}
		fioClose(fd);
	}
	else
	{
		return -1;
		SleepThread();
	}

  return -1;
}


