//#include "pgen.h"
#include "aio.h"
#include <string.h>
#include <malloc.h>
//#define AIOMC_DEBUG

mcIO::mcIO(int port, int maxEntries)
{
	int type, free, format;
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO constructor.\n");
#endif

	this->port = port;
	mcEntries = NULL;

	// Make sure memory card was detected
	mcGetInfo(port, 0, &type, &free, &format);
	mcSync(MC_WAIT, NULL, &rv);

	if((rv < -1)) //|| !format //Since we're not using XMC format is always 0
	{
		printf("mcgetinfo failed\n");
		status = AIO_STATE_ERROR;
		return;
	}

	mcEntries = (mcTable *)memalign(64, sizeof(mcTable) * maxEntries);
	if(!mcEntries)
		printf("Failed to allocate memory!");

	// TODO: remove when not used anymore
	sprintf(devname, "mc%d:", port);
	status = 0;
}

mcIO::~mcIO()
{
#ifdef AIOMC_DEBUG
	printf("mcIO destructor.\n");
#endif

	if(mcEntries)
		free(mcEntries);
}

int mcIO::open(const char *name, int flags)
{
	int rv;

#ifdef AIOMC_DEBUG
//	printf("mcIO open\n");
#endif

	mcOpen(port, 0, name, flags);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::close(int fd)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO close\n");
#endif

	mcClose(fd);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::read(int fd, unsigned char *buffer, int size)
{
	int rv;

#ifdef AIOMC_DEBUG
//	printf("mcIO read\n");
#endif

	mcRead(fd, buffer, size);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::write(int fd, const unsigned char *buffer, int size)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO write\n");
#endif

	mcWrite(fd, buffer, size);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::lseek(int fd, int offset, int whence)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO lseek\n");
#endif

	mcSeek(fd, offset, whence);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::remove(const char *name)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO remove\n");
#endif

	mcDelete(port, 0, name);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::rename(const char *old, const char *newname)
{
	int rv;
#ifdef AIOMC_DEBUG
	printf("mcIO rename\n");
#endif

	mcRename(port, 0, old, newname);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::mkdir(const char *name)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO mkdir\n");
#endif

	mcMkDir(port, 0, name);
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

int mcIO::rmdir(const char *name)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO rmdir\n");
#endif

	mcDelete(port, 0, name);	
	mcSync(MC_WAIT, NULL, &rv);
	return rv;
}

// Hacked for PGEN - add a ".." to the top of the list if its not there already
int mcIO::getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt)
{
	char thisName[1024];
	int numEntries;
	int i;
	int count = 0;

#ifdef AIOMC_DEBUG
	printf("mcIO getdir - %s, maxEnt = %d\n", name, maxEnt);
#endif

	if(!strcmp(name, "/"))
		strcpy(thisName, "/*");
	else
		snprintf(thisName, 1024, "%s/*", name);

	mcGetDir(port, 0, thisName, 0, maxEnt, mcEntries);
	mcSync(MC_WAIT, NULL, &numEntries);
	if(numEntries < -1)
		return numEntries;

//	printf("mcIO getdir: got %d entries\n", numEntries);

	if(!strcmp(name, "/"))
	{
		strcpy(dentBuf[0].name, "..");
		dentBuf[0].attrib = AIO_ATTRIB_DIR;
		dentBuf[0].size = 512;
		count++;
	}

	for(i = 0; (i < numEntries) && (count < maxEnt); i++)
	{
//		printf("mcEntries[%d].name = %s\n", i, mcEntries[i].name);

		if(strcmp((char *)mcEntries[i].name, "."))
		{
			if(	(tocEntryCompare((char *)mcEntries[i].name, extensions)) ||
				(mcEntries[i].attrFile & MC_ATTR_SUBDIR) )
			{
				strncpy(dentBuf[count].name, (char *)mcEntries[i].name, 256);
				dentBuf[count].name[255] = '\0'; // for safety
				dentBuf[count].attrib = 0;
				if(mcEntries[i].attrFile & MC_ATTR_SUBDIR)
					dentBuf[count].attrib |= AIO_ATTRIB_DIR;
				dentBuf[count].size = mcEntries[i].fileSizeByte;

//				printf("dentBuf[%d].name = %s\n", count, dentBuf[count].name);

				count++;
			}
		}
	}

//	printf("returning with %d entries!\n", count);

	return count;
}

int mcIO::getstat(const char *name, t_aioDent *dent)
{
	int rv;

#ifdef AIOMC_DEBUG
	printf("mcIO getstat\n");
#endif

	mcGetDir(port, 0, name, 0, 1, mcEntries);
	mcSync(MC_WAIT, NULL, &rv);
	if(rv <= 0)
		return -1;

	strncpy(dent->name, (char *)mcEntries[0].name, 256);
	dent->name[255] = '\0'; // for safety
	dent->attrib = 0;
	if(mcEntries[0].attrFile & MC_ATTR_SUBDIR)
		dent->attrib |= AIO_ATTRIB_DIR;
	dent->size = mcEntries[0].fileSizeByte;

	return 0;
}

u32 mcIO::getstatus()
{
#ifdef AIOMC_DEBUG
	printf("mcIO getstatus\n");
#endif

	return status;
}

void mcIO::clearstatus(u32 bits)
{
#ifdef AIOMC_DEBUG
	printf("mcIO clearstatus\n");
#endif

	status &= ~bits;
}

int mcIO::freespace()
{
	int type, free, format;
	int rv;

	mcGetInfo(port, 0, &type, &free, &format);
	mcSync(MC_WAIT, NULL, &rv);
	
	if(rv < -1)
		return rv;
	else
		return free;
}

int mcIO::tocEntryCompare(char* filename, const char* extensions)
{
	static char ext_list[129];
	char* token;
	char* ext_point;

	if(!extensions)
		return 1;
	
	strncpy(ext_list,extensions,128);
	ext_list[128]=0;

	token = strtok( ext_list, " ," );
	while( token != NULL )
	{
		// if 'token' matches extension of 'filename'
		// then return a match
		ext_point = strrchr(filename,'.');
		if(ext_point == NULL) return 0;

		if(strcasecmp(ext_point, token) == 0)
			return 1;
		
		/* Get next token: */
		token = strtok( NULL, " ," );
	}
	
	// If not match found then return FALSE
	return 0;	
}
