#ifndef _AIO_H
#define _AIO_H

#include <kernel.h>
#include <libmc.h>
#define	AIO_FLAG_WRITE			0x01

#define AIO_ATTRIB_DIR			0x01
#define AIO_ATTRIB_SYMLINK		0x02

#define AIO_STATE_ERROR			0x01

typedef struct {
	char name[256];
	u32 attrib;
	u32 size;
} t_aioDent;

// Base class for IO abstraction
class abstractIO {

	public:

		// All pure virtual functions
		virtual int open(const char *name, int flags) = 0;
		virtual int close(int fd) = 0;
		virtual int read(int fd, unsigned char *buffer, int size) = 0;
		virtual int write(int fd, const unsigned char *buffer, int size) = 0;
		virtual int lseek(int fd, int offset, int whence) = 0;
		virtual int remove(const char *name) = 0;
		virtual int rename(const char *old, const char *newname) = 0;
		virtual int mkdir(const char *name) = 0;
		virtual int rmdir(const char *name) = 0;
		virtual int getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt) = 0;
		virtual int getstat(const char *name, t_aioDent *dent) = 0;
		virtual int freespace() = 0; // Free space on device, in kilobytes
		virtual u32 getstatus() = 0;
		virtual void clearstatus(u32 bits) = 0;	// bits in "bits" which are 1, are cleared in state

		virtual ~abstractIO() { };

	protected:

		// Device name (ie: pfs0:, mc0:, cdrom0: etc)
		char devname[32];

		// Current state of device/mount
		u32 status;
};

#define AIO_HDD_NUMDEV	2

class hddIO : public abstractIO {

	public:
		
		hddIO(const char *blockdev);
		~hddIO();

		int open(const char *name, int flags);
		int close(int fd);
		int read(int fd, unsigned char *buffer, int size);
		int write(int fd, const unsigned char *buffer, int size);
		int lseek(int fd, int offset, int whence);
		int remove(const char *name);
		int rename(const char *old, const char *newname);
		int mkdir(const char *name);
		int rmdir(const char *name);
		int getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt);
		int getstat(const char *name, t_aioDent *dent);
		u32 getstatus();
		void clearstatus(u32 bits);
		int freespace();

	private:

		int tocEntryCompare(char* filename, const char* extensions);

		int currentDevice;
};

class mcIO : public abstractIO {

	public:

		mcIO(int port, int maxEntries);
		~mcIO();

		int open(const char *name, int flags);
		int close(int fd);
		int read(int fd, unsigned char *buffer, int size);
		int write(int fd, const unsigned char *buffer, int size);
		int lseek(int fd, int offset, int whence);
		int remove(const char *name);
		int rename(const char *old, const char *newname);
		int mkdir(const char *name);
		int rmdir(const char *name);
		int getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt);
		int getstat(const char *name, t_aioDent *dent);
		u32 getstatus();
		void clearstatus(u32 bits);
		int freespace();

	private:

		int tocEntryCompare(char* filename, const char* extensions);

		mcTable *mcEntries;
		int port;
};

class cdvdIO : public abstractIO {

	public:

		cdvdIO(int maxEntries);
		~cdvdIO();

		int open(const char *name, int flags);
		int close(int fd);
		int read(int fd, unsigned char *buffer, int size);
		int write(int fd, const unsigned char *buffer, int size);
		int lseek(int fd, int offset, int whence);
		int remove(const char *name);
		int rename(const char *old, const char *newname);
		int mkdir(const char *name);
		int rmdir(const char *name);
		int getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt);
		int getstat(const char *name, t_aioDent *dent);
		u32 getstatus();
		void clearstatus(u32 bits);
		int freespace();

	private:

		struct TocEntry *tocEntry;

};

class datafileIO : public abstractIO {

	public:

		datafileIO(unsigned char *data, int size);
		~datafileIO();

		int open(const char *name, int flags);
		int close(int fd);
		int read(int fd, unsigned char *buffer, int size);
		int write(int fd, const unsigned char *buffer, int size);
		int lseek(int fd, int offset, int whence);
		int remove(const char *name);
		int rename(const char *old, const char *newname);
		int mkdir(const char *name);
		int rmdir(const char *name);
		int getdir(const char *name, const char *extensions, t_aioDent dentBuf[], int maxEnt);
		int getstat(const char *name, t_aioDent *dent);
		u32 getstatus();
		void clearstatus(u32 bits);
		int freespace();

	private:

		unsigned char *data;
		int fileSize;
		int filePos;
};


#include "aio-fio.h"

#endif /* _AIO_H */
