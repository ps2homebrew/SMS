#include <tamtypes.h>
#include <kernel.h>
#include <string.h>
#include <sifrpc.h>
#include <libpad.h>
#include <loadfile.h>
#include <iopheap.h>
#include <malloc.h>
#include <libmc.h>

// PS2DRV includes
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fileXio_rpc.h"
#include "errno.h"

#include "aio.h"
#include "libhdd.h"

//#define AIOHDD_DEBUG


// HACK: Compiler/linker doesnt like static variables.. cant resolve references to static
//       members for some reason.
char hddIOdeviceUsed[AIO_HDD_NUMDEV];
static int fileMode =	FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP |
						FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;

hddIO::hddIO(const char *blockdev)
{
	int i;

#ifdef AIOHDD_DEBUG
	printf("hddIO constructor!\n");
#endif

	strcpy(devname, "");

	for(i = 0; i < AIO_HDD_NUMDEV; i++)
	{
		if(!hddIOdeviceUsed[i])
		{
			currentDevice = i;
			sprintf(devname, "pfs%d:", currentDevice);
			break;
		}
	}

	if(!strcmp(devname, "") || (fileXioMount(devname, blockdev, FIO_MT_RDWR) < 0))
	{
		printf("Mount of %s failed!\n", blockdev);
		status |= AIO_STATE_ERROR;
		return;
	}

	hddIOdeviceUsed[currentDevice] = 1;

	printf("%s mounted successfully.\n", devname);
	status = 0;
}

hddIO::~hddIO()
{
#ifdef AIOHDD_DEBUG
	printf("hddIO destructor.\n");
#endif

	if(fileXioUmount(devname) < 0)
	{
		printf("PANIC: fileXioUmount(\"%s\"); failed!\n", devname);
//		SleepThread();
	}

	hddIOdeviceUsed[currentDevice] = 0;
}

int hddIO::open(const char *name, int flags)
{
	char openString[1024];

#ifdef AIOHDD_DEBUG
//	printf("hddIO open\n");
#endif

	snprintf(openString, 1024, "%s%s", devname, name);

	return fileXioOpen(openString, flags, fileMode);
}

int hddIO::close(int fd)
{
#ifdef AIOHDD_DEBUG
	printf("hddIO close\n");
#endif

	return fileXioClose(fd);
}

int hddIO::read(int fd, unsigned char *buffer, int size)
{
#ifdef AIOHDD_DEBUG
//	printf("hddIO read\n");
#endif

	return fileXioRead(fd, buffer, size);
}

int hddIO::write(int fd, const unsigned char *buffer, int size)
{
#ifdef AIOHDD_DEBUG
//	printf("hddIO write\n");
#endif

	return fileXioWrite(fd, (unsigned char *)buffer, size);
}

int hddIO::lseek(int fd, int offset, int whence)
{
#ifdef AIOHDD_DEBUG
	printf("hddIO lseek\n");
#endif

	return fileXioLseek(fd, offset, whence);
}

int hddIO::remove(const char *name)
{
	char removeString[1024];
#ifdef AIOHDD_DEBUG
	printf("hddIO remove\n");
#endif

	snprintf(removeString, 1024, "%s%s", devname, name);

	return fileXioRemove(removeString);
}

int hddIO::rename(const char *old, const char *newname)
{
	char oldString[1024];
	char newString[1024];
#ifdef AIOHDD_DEBUG
	printf("hddIO rename\n");
#endif

	snprintf(oldString, 1024, "%s%s", devname, old);
	snprintf(newString, 1024, "%s%s", devname, newname);

	return fileXioRename(oldString, newString);
}

int hddIO::mkdir(const char *name)
{
	char dirname[1024];
#ifdef AIOHDD_DEBUG
	printf("hddIO mkdir: %s\n", name);
#endif

	snprintf(dirname, 1024, "%s%s", devname, name);

	return fileXioMkdir(dirname, fileMode);
}

int hddIO::rmdir(const char *name)
{
	char dirname[1024];

#ifdef AIOHDD_DEBUG
	printf("hddIO rmdir\n");
#endif

	snprintf(dirname, 1024, "%s%s", devname, name);

	return fileXioRmdir(dirname);
}

int hddIO::getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt)
{
	iox_dirent_t thisDir;
	int count = 0, dirFd, rv;
	char openString[1024];

	snprintf(openString, 1024, "%s%s", devname, name);

#ifdef AIOHDD_DEBUG
	printf("hddIO getdir. openString = %s\n", openString);
#endif

	dirFd = fileXioDopen(openString);
	if(dirFd < 0)
		return dirFd;

	rv = fileXioDread(dirFd, &thisDir);
	while((rv > 0) && (count < maxEnt))
	{
		if(strcmp(thisDir.name, "."))
		{
			if(	(tocEntryCompare(thisDir.name, extensions)) ||
				(FIO_S_ISDIR(thisDir.stat.mode)))
			{

				strncpy(dentBuf[count].name, thisDir.name, 256);
				dentBuf[count].name[255] = '\0'; // for safety
				dentBuf[count].attrib = 0;
				if(FIO_S_ISDIR(thisDir.stat.mode))
					dentBuf[count].attrib |= AIO_ATTRIB_DIR;
				if(FIO_S_ISLNK(thisDir.stat.mode))
					dentBuf[count].attrib |= AIO_ATTRIB_SYMLINK;
				dentBuf[count].size = thisDir.stat.size;

				count++;
			}
		}
		rv = fileXioDread(dirFd, &thisDir);
	}

	fileXioDclose(dirFd);

#ifdef AIOHDD_DEBUG
	printf("hddIO getdir returning.. count = %d\n", count);
#endif

	return count;
}

u32 hddIO::getstatus()
{
#ifdef AIOHDD_DEBUG
	printf("hddIO status\n");
#endif

	return status;
}

void hddIO::clearstatus(u32 bits)
{
#ifdef AIOHDD_DEBUG
	printf("hddIO clearstatus\n");
#endif

	status &= ~bits;
}

iox_stat_t thisStat __attribute__((aligned(64)));

int hddIO::getstat(const char *name, t_aioDent *dent)
{
//	iox_stat_t thisStat;
	char filename[1024];
	int rv;

#ifdef AIOHDD_DEBUG
	printf("hddIO getstat\n");
#endif

	// Special case: name = "/"
	if(!strcmp(name, "/"))
	{
		strcpy(dent->name, "/");
		dent->attrib = AIO_ATTRIB_DIR;
		dent->size = 512;
	}
	else
	{
		snprintf(filename, 1024, "%s%s", devname, name);

		rv = fileXioGetStat(filename, &thisStat);
		if(rv < 0)
			return rv;

		strcpy(dent->name, name);
		dent->attrib = 0;
		if(FIO_S_ISDIR(thisStat.mode))
			dent->attrib |= AIO_ATTRIB_DIR;
		if(FIO_S_ISLNK(thisStat.mode))
			dent->attrib |= AIO_ATTRIB_SYMLINK;
		dent->size = thisStat.size;
	}

	return 0;
}

int hddIO::freespace()
{
#ifdef AIOHDD_DEBUG
	printf("hddIO freespace\n");
#endif

	int zoneFree = fileXioDevctl(devname, PFSCTL_GET_ZONE_FREE, NULL, 0, NULL, 0);
	int zoneSize = fileXioDevctl(devname, PFSCTL_GET_ZONE_SIZE, NULL, 0, NULL, 0);

	// Return value as kb's
	return zoneFree * zoneSize / 1024;
}

int hddIO::tocEntryCompare(char* filename, const char* extensions)
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
