#include "pgen.h"


datafileIO::datafileIO(unsigned char *data, int size)
{
	this->data = data;
	fileSize = size;
	filePos = 0;
}

datafileIO::~datafileIO() { }

int datafileIO::open(const char *name, int flags)
{
	filePos = 0;
	return 1;
}

int datafileIO::close(int fd)
{
	filePos = 0;
	return 0;
}

int datafileIO::read(int fd, unsigned char *buffer, int size)
{
	if(size > (fileSize - filePos))
		size = fileSize - filePos;

	memcpy(buffer, data + filePos, size);

	filePos += size;

	return size;
}

int datafileIO::write(int fd, const unsigned char *buffer, int size)
{
	return -1;
}

int datafileIO::lseek(int fd, int offset, int whence)
{
	switch(whence)
	{
		case SEEK_SET:
			filePos = offset;
			break;
			
		case SEEK_CUR:
			filePos += offset;
			break;
			
		case SEEK_END:
			filePos = fileSize + offset;
			break;

		default:
			return -1;
	}

	return filePos;
}

int datafileIO::remove(const char *name)
{
	return -1;
}

int datafileIO::rename(const char *old, const char *newname)
{
	return -1;
}

int datafileIO::mkdir(const char *name)
{
	return -1;
}

int datafileIO::rmdir(const char *name)
{
	return -1;
}

int datafileIO::getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt)
{
	return 0;
}

int datafileIO::getstat(const char *name, t_aioDent *dent)
{
	return -1;
}

u32 datafileIO::getstatus()
{
	return status;
}

void datafileIO::clearstatus(u32 bits)
{
	status &= ~bits;
}

int datafileIO::freespace()
{
	return 0;
}
