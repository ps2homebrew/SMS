#include "PbFile.h"
#include <tamtypes.h>
#include <fileio.h>
#include <stdlib.h>
#include <kernel.h>

///////////////////////////////////////////////////////////////////////////////
// int PbFile_Write
///////////////////////////////////////////////////////////////////////////////

int PbFile_Write( const char* pName, void* pData, int size )
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



