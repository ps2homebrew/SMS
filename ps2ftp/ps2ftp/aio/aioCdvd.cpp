//#include "pgen.h"
#include "aio.h"
#include "cdvd_rpc.h"
#include <malloc.h>
#include <fileio.h>
//#define AIOCVDV_DEBUG

cdvdIO::cdvdIO(int maxEntries)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO constructor.\n");
#endif

	tocEntry = (struct TocEntry *)memalign(64, maxEntries * sizeof(struct TocEntry));
	if(!tocEntry)
		status |= AIO_STATE_ERROR;

	strcpy(devname, "cdfs:");
	CDVD_FlushCache();
	status = 0;
}

cdvdIO::~cdvdIO()
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO destructor.\n");
#endif
	if(tocEntry)
		free(tocEntry);
}

int cdvdIO::open(const char *name, int flags)
{
	char openString[1024];

#ifdef AIOCVDV_DEBUG
//	printf("cdvdIO open\n");
#endif

	snprintf(openString, 1024, "%s%s", devname, name);

	return fioOpen(openString, flags);
}

int cdvdIO::close(int fd)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO close\n");
#endif

	return fioClose(fd);
}

int cdvdIO::read(int fd, unsigned char *buffer, int size)
{
#ifdef AIOCVDV_DEBUG
//	printf("cdvdIO read\n");
#endif

	return fioRead(fd, buffer, size);
}

int cdvdIO::write(int fd, const unsigned char *buffer, int size)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO write\n");
#endif

	return -1;
}

int cdvdIO::lseek(int fd, int offset, int whence)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO lseek\n");
#endif

	return fioLseek(fd, offset, whence);
}

int cdvdIO::remove(const char *name)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO remove\n");
#endif

	return -1;
}

int cdvdIO::rename(const char *old, const char *newname)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO rename\n");
#endif

	return -1;
}

int cdvdIO::mkdir(const char *name)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO mkdir\n");
#endif

	return -1;
}

int cdvdIO::rmdir(const char *name)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO rmdir\n");
#endif

	return -1;
}

// Hacked for PGEN - add a ".." to the top of the list if its not there already
int cdvdIO::getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt)
{
	int numEntries;
	int i;
	int dIdx = 0;

#ifdef AIOCVDV_DEBUG
	printf("cdvdIO getdir\n");
#endif

	numEntries = CDVD_getdir(name, extensions, CDVD_GET_FILES_AND_DIRS, tocEntry, maxEnt, NULL);

	if(!strcmp(name, "/"))
	{
		strcpy(dentBuf[0].name, "..");
		dentBuf[0].attrib = AIO_ATTRIB_DIR;
		dentBuf[0].size = 512;
		dIdx++;
	}

	for(i = 0; (i < numEntries) && (dIdx < maxEnt); i++, dIdx++)
	{
		strcpy(dentBuf[dIdx].name, tocEntry[i].filename);
		if(tocEntry[i].fileProperties & 0x02)
			dentBuf[dIdx].attrib = AIO_ATTRIB_DIR;
		else
			dentBuf[dIdx].attrib = 0;
		dentBuf[dIdx].size = tocEntry[i].fileSize;
	}

	return dIdx;
}

int cdvdIO::getstat(const char *name, t_aioDent *dent)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO getstat\n");
#endif

	if(CDVD_FindFile(name, &tocEntry[0]) == 0)
		return -1;

	strcpy(dent->name, tocEntry[0].filename);
	if(tocEntry[0].fileProperties & 0x02)
		dent->attrib = AIO_ATTRIB_DIR;
	else
		dent->attrib = 0;
	dent->size = tocEntry[0].fileSize;

	return 0;
}

u32 cdvdIO::getstatus()
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO getstatus\n");
#endif

	return status;
}

void cdvdIO::clearstatus(u32 bits)
{
#ifdef AIOCVDV_DEBUG
	printf("cdvdIO clearstatus\n");
#endif

	status &= ~bits;
}

int cdvdIO::freespace()
{
	return 0;
}
