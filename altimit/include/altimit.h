/*========================================================================
==				Altimit.h main header file.		                		==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#ifndef altimit_h
#define altimit_h

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <fileio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <libpad.h>
#include <libmc.h>
#include <iopheap.h>

#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fileXio_rpc.h"
#include "errno.h"
#include "libhdd.h"
#include "sbv_patches.h"
#include "cdvd_rpc.h"
#include "gsDefs.h"
#include "gsDriver.h"
#include "gsPipe.h"
#include "gsFont.h"
#include "libkbd.h"
#include "libmouse.h"
#include "debug.h"

//#define DEBUG
#ifdef DEBUG
#define dbgprintf(args...) printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

#define ELF_MAGIC		0x464c457f
#define ELF_PT_LOAD	1
#define FONT_HEIGHT 16
#define FONT_HEIGHT2 14
#define MAX_FILENAME 256
#define MAX_PATHNAME 1024
#define MAX_DEVNAME 5
#define MAX_PARTITIONS 10
#define MAX_ENTRIES 4096
#define MAX_DEVICES 10
#define MAX_BUTTONS 12
#define CD_INIT_WAIT 0
#define CD_INIT_NOWAIT 1
#define CD_INIT_BIND_RPC 0x80000592
#define CD_EXIT 5
#define CD_BOOT 1
#define MC_BOOT 2
#define HOST_BOOT 3
#define PFS_BOOT 4
#define VFS_BOOT 5
#define UNK_BOOT 6

typedef struct
{
 u8	ident[16];			// struct definition for ELF object header
 u16 type;
 u16	machine;
 u32	version;
 u32	entry;
 u32	phoff;
 u32	shoff;
 u32	flags;
 u16	ehsize;
 u16	phentsize;
 u16	phnum;
 u16	shentsize;
 u16	shnum;
 u16	shstrndx;
 } elf_header_t;

typedef struct
{
 u32	type;	   // struct definition for ELF program section header
 u32	offset;
 void *vaddr;
 u32	paddr;
 u32	filesz;
 u32	memsz;
 u32	flags;
 u32	align;
 } elf_pheader_t;

typedef struct
{
 char filename[MAX_FILENAME];
 u32 mode;
 u32 size;
 void *fsdevice;
} altDentry;

typedef struct
{
 int Xpos;
 int Ypos;
 int Zpos;
 int Xsize;
 int Ysize;
 unsigned int Foreground;
 unsigned int Background;
 unsigned int Forehead;
 unsigned int Backhead;
 char *Title;
 char *Content;
 int Top;
} TextWindow;

typedef struct
{
 int Xpos;
 int Ypos;
 int Zpos;
 unsigned int Foreground;
 unsigned int Background;
 char *Content;
} TooltipWindow;

typedef struct
{
 int Xpos;
 int Ypos;
 int Zpos;
 int Xsize;
 unsigned int Foreground;
 unsigned int Background;
 unsigned int Forehead;
 unsigned int Backhead;
 char *Title;
 unsigned int total;
 unsigned int done;
} PercentWindow;

typedef struct
{
 int Xpos;
 int Ypos;
 int Zpos;
 int Xsize;
 int Ysize;
 unsigned int Foreground;
 unsigned int Background;
 unsigned int Forehead;
 unsigned int Backhead;
 char *Title;
 altDentry *Content;
 int Top;
 int Highlighted;
} FilelistWindow;

typedef struct
{
 int Xpos;
 int Ypos;
 int Zpos;
 unsigned int Foreground;
 unsigned int Background;
 char *Content;
 int Highlighted;
} FunctionWindow;

typedef struct
{
 unsigned int WIDTH;
 unsigned int HEIGHT;
 unsigned int OFFSETX;
 unsigned int OFFSETY;
 unsigned int PALORNTSC;
 unsigned int INTERLACING;
 unsigned int SCREENCOL1;
 unsigned int SCREENCOL2;
 unsigned int POINTERCOL;
 unsigned int WINBACKCOL;
 unsigned int WINFORECOL;
 unsigned int WINFOREHEAD;
 unsigned int WINBACKHEAD;
 unsigned int LOADNETFS;
 unsigned int LOADHOST;
 unsigned int LOADHDDS;
 unsigned int USEPOINTER;
 unsigned int LOADMOUSE;
 unsigned int LOADKEYBD;
 unsigned int LOADFTPD;
 } altimitGS;

class altimitIO
{
 public:
 virtual int open(const char *pathname, int mode) = 0;
 virtual int lseek(int handle, int offset, int start) = 0;
 virtual int read(int handle, unsigned char *buffer, int size) = 0;
 virtual int write(int handle, unsigned char *buffer, int size) = 0;
 virtual int close(int handle) = 0;
 virtual int remove(const char *pathname) = 0;
 virtual int rename(const char *pathname, const char *newpathname) = 0;
 virtual int mkdir(const char *pathname) = 0;
 virtual int rmdir(const char *pathname) = 0;
 virtual int getdir(const char *pathname, altDentry contents[]) = 0;
 virtual int getpath(const char *pathname, char *fullpath) = 0;
 virtual int getstat(const char *pathname, iox_stat_t *filestat) = 0;
 virtual int chstat(const char *pathname, iox_stat_t *filestat) = 0;
 virtual int freespace() = 0;
 virtual int getstatus() = 0;

 virtual ~altimitIO() { };

 protected:
 char device[5];
 int devicestatus;
};

class hddIO : public altimitIO
{
 public:
 hddIO();
 ~hddIO();
 int open(const char *pathname, int mode);
 int lseek(int handle, int offset, int start);
 int read(int handle, unsigned char *buffer, int size);
 int write(int handle, unsigned char *buffer, int size);
 int close(int handle);
 int remove(const char *pathname);
 int rename(const char *pathname, const char *newpathname);
 int mkdir(const char *pathname);
 int rmdir(const char *pathname);
 int getdir(const char *pathname, altDentry contents[]);
 int getpath(const char *pathname, char *fullpath);
 int getstat(const char *pathname, iox_stat_t *filestat);
 int chstat(const char *pathname, iox_stat_t *filestat);
 int freespace();
 int getstatus();

 private:
 iox_dirent_t *hddcontents;
 int pfsmounted, parts;
 char *hddpathname;
 char *hddnewname;
 t_hddFilesystem *partitions;
};

class httpIO : public altimitIO
{
 public:
 httpIO();
 ~httpIO();
 int open(const char *pathname, int mode);
 int lseek(int handle, int offset, int start);
 int read(int handle, unsigned char *buffer, int size);
 int write(int handle, unsigned char *buffer, int size);
 int close(int handle);
 int remove(const char *pathname);
 int rename(const char *pathname, const char *newpathname);
 int mkdir(const char *pathname);
 int rmdir(const char *pathname);
 int getdir(const char *pathname, altDentry contents[]);
 int getpath(const char *pathname, char *fullpath);
 int getstat(const char *pathname, iox_stat_t *filestat);
 int chstat(const char *pathname, iox_stat_t *filestat);
 int freespace();
 int getstatus();

 private:
 unsigned char *httpcontents;
 char *httppathname;
};

class hostIO : public altimitIO
{
 public:
 hostIO();
 ~hostIO();
 int open(const char *pathname, int mode);
 int lseek(int handle, int offset, int start);
 int read(int handle, unsigned char *buffer, int size);
 int write(int handle, unsigned char *buffer, int size);
 int close(int handle);
 int remove(const char *pathname);
 int rename(const char *pathname, const char *newpathname);
 int mkdir(const char *pathname);
 int rmdir(const char *pathname);
 int getdir(const char *pathname, altDentry contents[]);
 int getpath(const char *pathname, char *fullpath);
 int getstat(const char *pathname, iox_stat_t *filestat);
 int chstat(const char *pathname, iox_stat_t *filestat);
 int freespace();
 int getstatus();

 private:
 fio_dirent_t *hostcontents;
 int elflist;
 char *hostpathname;
 char *hostnewname;
};

class mcIO : public altimitIO
{
 public:
 mcIO(int mcport);
 ~mcIO();
 int open(const char *pathname, int mode);
 int lseek(int handle, int offset, int start);
 int read(int handle, unsigned char *buffer, int size);
 int write(int handle, unsigned char *buffer, int size);
 int close(int handle);
 int remove(const char *pathname);
 int rename(const char *pathname, const char *newpathname);
 int mkdir(const char *pathname);
 int rmdir(const char *pathname);
 int getdir(const char *pathname, altDentry contents[]);
 int getpath(const char *pathname, char *fullpath);
 int getstat(const char *pathname, iox_stat_t *filestat);
 int chstat(const char *pathname, iox_stat_t *filestat);
 int freespace();
 int getstatus();

 private:
 mcTable *mccontents;
 int mcport;
 int mcfree, changed;
 char *mcpathname;
 char *mcnewname;
};

class cdfsIO : public altimitIO
{
 public:
 cdfsIO();
 ~cdfsIO();
 int open(const char *pathname, int mode);
 int lseek(int handle, int offset, int start);
 int read(int handle, unsigned char *buffer, int size);
 int write(int handle, unsigned char *buffer, int size);
 int close(int handle);
 int remove(const char *pathname);
 int rename(const char *pathname, const char *newpathname);
 int mkdir(const char *pathname);
 int rmdir(const char *pathname);
 int getdir(const char *pathname, altDentry contents[]);
 int getpath(const char *pathname, char *fullpath);
 int getstat(const char *pathname, iox_stat_t *filestat);
 int chstat(const char *pathname, iox_stat_t *filestat);
 int freespace();
 int getstatus();

 private:
 struct TocEntry *cdfscontents;
 char *cdpathname;
};

class altimitFS
{
 public:
 altimitIO *activedevice, *http, *host, *mc0, *mc1, *cdfs, *pfs;
 int activeHDD, activeHTTP, activeHOST, activeMC0, activeMC1, activeCDFS;
 altDentry *dircontents;
 char *currentdev;
 char *currentdir;

 altimitFS(int ps2hd = true, int iphd = true, int pchd = true, int mc = true, int cd = true);
 ~altimitFS();
 private:
 altDentry *altroot;
};

int waitPadReady(int port, int slot);
int initializePad(int port, int slot);
void startpad();
int readpadbutton();
int cdInit(int mode);
void cdExit(void);
void drawTooltipWindow(TooltipWindow tooltip);
void drawFunctionWindow(FunctionWindow functions);
void drawTextWindow(TextWindow textbox);
void drawFilelistWindow(FilelistWindow textbox);
void drawPercentWindow(PercentWindow percentbox, char *operation);
void drawButtons(int drawbuttons[][5], char drawtips[][30], int max);
void drawActiveButton(int drawbuttons[][5], char drawtips[][30], int activebutton);
void drawMainWindow();
void drawPointer();

#endif
