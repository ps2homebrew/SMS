#include "tamtypes.h"
#include "kernel.h"
#include "sifrpc.h"
#include "loadfile.h"
#include "fileio.h"
#include "iopcontrol.h"
#include "stdarg.h"
#include "string.h"
#include "malloc.h"
#include "libmc.h"
#include "iopheap.h"
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fileXio_rpc.h"
#include "errno.h"
#include "libhdd.h"

#define DEBUG
#ifdef DEBUG
#define dbgprintf(args...) scr_printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

// ELF-loading stuff
#define ELF_MAGIC	0x464c457f
#define ELF_PT_LOAD	1

typedef struct
{
	u8	ident[16];
	u16	type;
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
	u32	type;
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
	} elf_pheader_t;

t_ExecData elfdata;

extern u8 *fakehost_irx;
extern int size_fakehost_irx;
extern u8 *poweroff_irx;
extern int size_poweroff_irx;

void LoadAndRunMCElf(char *filename);

int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;
char elfName[256];
char elfPath[256];
char HDDpath[256]="host:hddmenu.elf";
char partition[40];

const char *eeloadimg = "rom0:UDNL rom0:EELOADCNF";
char *imgcmd;
int elfhost=2,elfload=0;

static int pkoLoadElf(char *path);

int userThreadID = 0;
static char userThreadStack[16*1024] __attribute__((aligned(16)));

#define MAX_ARGS 16
#define MAX_ARGLEN 256

struct argData
{
    int flag;                     // Contains thread id atm
    int argc;
    char *argv[MAX_ARGS];
} __attribute__((packed)) userArgs;

static int tLoadElf(char *filename)
{
	u8 *boot_elf = (u8 *)0x1800000;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;

	int fd, size, i, ret;

	ret = fileXioMount("pfs0:", partition, FIO_MT_RDONLY);
	if ((fd = fileXioOpen(filename, O_RDONLY, fileMode)) < 0)
	{
		dbgprintf("Failed in fileXioOpen %s\n",filename);
		goto error;
		}
	dbgprintf("Opened file %s\n",filename);
	size = fileXioLseek(fd, 0, SEEK_END);
	dbgprintf("File size = %i\n",size);
	if (!size)
	{
		dbgprintf("Failed in fileXioLseek\n");
		fileXioClose(fd);
		goto error;
		}
	fileXioLseek(fd, 0, SEEK_SET);
	fileXioRead(fd, boot_elf, 52);
	dbgprintf("Read elf header from file\n");
	fileXioLseek(fd, (void *)eh->phoff, SEEK_SET);
	eph = (elf_pheader_t *)(boot_elf + eh->phoff);
	size=eh->phnum*eh->phentsize;
	size=fileXioRead(fd, (void *)eph, size);
	dbgprintf("Read %i bytes of program header(s) from file\n",size);
	for (i = 0; i < eh->phnum; i++)
	{
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		fileXioLseek(fd, eph[i].offset, SEEK_SET);
		size=eph[i].filesz;
		size=fileXioRead(fd, eph[i].vaddr, size);
		dbgprintf("Read %i bytes to %x\n", size, eph[i].vaddr);
		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);
		}		

	fileXioClose(fd);
//	fileXioUmount("pfs0:");
/* Load the ELF into RAM.  */

	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
		dbgprintf("Not a recognised ELF.\n");
		goto error;
		}
	
	dbgprintf("entry=%x\n",eh->entry);
	elfdata.epc=(int *)eh->entry;
error:
//	while (1) ;

	}

////////////////////////////////////////////////////////////////////////
// Load the actual elf, and create a thread for it
// Return the thread id
static int
pkoLoadElf(char *path)
{
    ee_thread_t th_attr;
    int ret=0;
    int pid;

    if(!strncmp(path, "host", 4)) ret = SifLoadElf(path, &elfdata);
    else if(!strncmp(path, "mc0", 3)) ret = SifLoadElf(path, &elfdata);
    else if(!strncmp(path, "pfs0", 4)) ret = tLoadElf(path);

    FlushCache(0);
    FlushCache(2);

    dbgprintf("EE: LoadElf returned %d\n", ret);

    dbgprintf("EE: Creating user thread (ent: %x, gp: %x, st: %x)\n", 
              elfdata.epc, elfdata.gp, elfdata.sp);

    if (elfdata.epc == 0) {
        dbgprintf("EE: Could not load file\n");
        return -1;
    }

    th_attr.func = (void *)elfdata.epc;
    th_attr.stack = userThreadStack;
    th_attr.stack_size = sizeof(userThreadStack);
    th_attr.gp_reg = (void *)elfdata.gp;
    th_attr.initial_priority = 64;

    pid = CreateThread(&th_attr);
    if (pid < 0) {
        dbgprintf("EE: Create user thread failed %d\n", pid);
        return -1;
    }
    dbgprintf("EE: Created user thread: %d\n", pid);

    return pid;
}

void
wipeUserMem(void)
{
    int i;
    for (i = 0x100000; i < 0x2000000 ; i += 64) {
        asm (
            "\tsq $0, 0(%0) \n"
            "\tsq $0, 16(%0) \n"
            "\tsq $0, 32(%0) \n"
            "\tsq $0, 48(%0) \n"
            :: "r" (i) );
    }
}

void poweroffHandler(int i)
{
	dbgprintf("Trying to delete thread %i\n",i);
	TerminateThread(i);
	DeleteThread(i);
	elfhost=1;
	LoadAndRunMCElf("pfs0:/PS2MENU.ELF");
//	HddPowerOff();
	}

char *strrchr(const char *sp, int i)
{
	const char *last = NULL;
	char c = i;

	while (*sp)
	{
		if (*sp == c)
		{
			last = sp;
			}
		sp++;
		}

	if (*sp == c)
	{
		last = sp;
		}

	return (char *) last;
}

// *************************************************************************
// *** MAIN
// *************************************************************************
int main(int argc, char *argv[])
{
	char s[256],fakepart[128], *ptr;
	int pid,ret;

// Initialise
	SifInitRpc(0);
	hddPreparePoweroff();
	hddSetUserPoweroffCallback((void *)poweroffHandler,(void *)pid);
	init_scr();
	wipeUserMem();
	scr_printf("Welcome to PS2Menu Loader v2.0\nPlease wait...loading.\n");
	scr_printf("Loading poweroff.irx %i bytes\n", size_poweroff_irx);
	SifExecModuleBuffer(&poweroff_irx, size_poweroff_irx, 0, NULL, &ret);

	strcpy(s,argv[0]);
	dbgprintf("argv[0] = %s\n",s);
	if (argc==1)
	{									// hopefully this is the first
		while(1);
/*		setPathInfo(s);
		if(!strncmp(s, "host:", 5)) elfload=1;		// assume loading from PS2LINK
		else if(!strncmp(s, "mc0:", 4)) 			// loading from memory card
		{
			elfload=2;
			imgcmd = (char *)eeloadimg;
			fioExit();
			SifExitRpc();

//			scr_printf("reset iop\n");
			SifIopReset(imgcmd, 0);
			while (!SifIopSync()) ;

//			wipeUserMem();
//			scr_printf("rpc init\n");
			SifInitRpc(0);
			}
		strcpy(elfName,s);

		LoadModules();
		LoadAndRunMCElf(HDDpath);*/
		}
	if (argc==2)				// if call came from hddmenu.elf
	{
		strcpy(partition,argv[1]);
		dbgprintf("argv[1] = %s\n", partition);

/*		if(hddCheckPresent() < 0)
		{
			scr_printf("Error: supported HDD not connected!\n");
			while(1);
			}
		if(hddCheckFormatted() < 0)
		{
			scr_printf("Error: HDD is not properly formatted!\n");
			while(1);
			}
		scr_printf("HDD is connected and formatted.\n");*/
		strcpy(HDDpath,s);
		elfhost=1;
		}

	dbgprintf("Loading %s\n",HDDpath);
	pid = pkoLoadElf(HDDpath);
	dbgprintf("pkoLoadElf returned %i\n",pid);
	if (pid < 0) {
        scr_printf("Could not execute file %s\n", HDDpath);
        return -1;
    }
	strcpy(fakepart,HDDpath);
	ptr=strrchr(fakepart,'/');
	if(ptr==NULL) strcpy(fakepart,"pfs0:");
	else
	{
		ptr++;
		*ptr='\0';
		}
	ptr=strrchr(s,'/');
	if(ptr==NULL) ptr=strrchr(s,':');
	if(ptr!=NULL)
	{
		ptr++;
		strcpy(HDDpath,"host:");
		strcat(HDDpath,ptr);
		}
	scr_printf("Loading fakehost.irx %i bytes\n", size_fakehost_irx);
	scr_printf("%s\n", fakepart);
	SifExecModuleBuffer(&fakehost_irx, size_fakehost_irx, strlen(fakepart), fakepart, &ret);

    FlushCache(0);
    FlushCache(2);

    userThreadID = pid;

    userArgs.argc=1;
    userArgs.argv[0]=HDDpath;
//    userArgs.argv[1]=elfName;
    userArgs.flag = (int)&userThreadID;

    ret = StartThread(userThreadID, &userArgs);
    if (ret < 0) {
        scr_printf("EE: Start user thread failed %d\n", ret);
        DeleteThread(userThreadID);
        return -1;
    }
    SleepThread();
	}

////////////////////////////////////////////////////////////////////////
// Wrapper to load module from disc/rom/mc
// Max irx size hardcoded to 300kb atm..
/*static void
pkoLoadMcModule(char *path, int argc, char *argv)
{
    void *iop_mem;
    int ret;

    scr_printf("LoadMcModule %s\n", path);
    iop_mem = SifAllocIopHeap(1024*300);
    if (iop_mem == NULL) {
        scr_printf("SifallocIopHeap failed\n");
        SleepThread();
    }
    ret = SifLoadIopHeap(path, iop_mem);
    if (ret < 0) {
        scr_printf("SifloadIopHeap %s ret %d\n", path, ret);
        SleepThread();
    }
    else {
        ret = SifLoadModuleBuffer(iop_mem, argc, argv);
        if (ret < 0) {
            scr_printf("SifloadModuleBuffer %s ret %d\n", path, ret);
            SleepThread();
        }
    }
    SifFreeIopHeap(iop_mem);
}

static void
pkoSifLoadModule(char *path, int argc, char *argv)
{
	int ret;

	scr_printf("Loading %s\n",path);
	ret = SifLoadModule(path, argc, argv);
	if (ret < 0)
	{
		scr_printf("Failed\n");
		SleepThread();
		}
	}
	
void LoadModules()
{
	static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20";
	static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40";

	if(elfload==2)
	{
		SifLoadModule("rom0:SIO2MAN", 0, NULL);
		SifLoadModule("rom0:MCMAN", 0, NULL);
		pkoLoadMcModule(iomanX_path, 0, NULL);
		pkoLoadMcModule(fileXio_path, 0, NULL);
		pkoLoadMcModule(ps2dev9_path, 0, NULL);
		pkoLoadMcModule(ps2atad_path, 0, NULL);
		pkoLoadMcModule(ps2hdd_path, sizeof(hddarg), hddarg);
		pkoLoadMcModule(ps2fs_path, sizeof(pfsarg), pfsarg);
		}
	else
	{
		pkoSifLoadModule(fileXio_path, 0, NULL);
		pkoSifLoadModule(ps2atad_path, 0, NULL);
		pkoSifLoadModule(ps2hdd_path, sizeof(hddarg), hddarg);
		pkoSifLoadModule(ps2fs_path, sizeof(pfsarg), pfsarg);
		}
	}*/

void LoadAndRunMCElf(char *filename)
{
	u8 *boot_elf = (u8 *)0x1800000;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;
	void *pdata;

	int fd, size, i, ret;
	char *argv[1];

	dbgprintf("Start of LoadAndRunElf\nelfhost=%i\nfilename=%s\n",elfhost,filename);

	if(elfhost==1)
	{
		ret = fileXioMount("pfs0:", "hdd0:+PS2MENU", FIO_MT_RDONLY);
		if ((fd = fileXioOpen(filename, O_RDONLY, fileMode)) < 0)
		{
			dbgprintf("Failed in fileXioOpen %s\n",filename);
			goto error;
			}

		size = fileXioLseek(fd, 0, SEEK_END);
		if (!size)
		{
			dbgprintf("Failed in fileXioLseek\n");
			fileXioClose(fd);
			goto error;
			}

		fileXioLseek(fd, 0, SEEK_SET);
		fileXioRead(fd, boot_elf, size);
		fileXioClose(fd);
		fileXioUmount("pfs0:");
		}
	if(elfhost==2)
	{
		if ((fd = fioOpen(filename, O_RDONLY)) < 0)
		{
			scr_printf("%i Failed in fioOpen %s\n",fd,filename);
			goto error;
			}

		scr_printf("Before fioLseek\n");
		size = fioLseek(fd, 0, SEEK_END);
		if (!size)
		{
			scr_printf("Failed in fioLseek\n");
			fioClose(fd);
			goto error;
			}

		if(fioLseek(fd, 0, SEEK_SET)<0) scr_printf("Error in SEEK_SET.\n");
		if(fioRead(fd, boot_elf, size)<0) scr_printf("Error in Read.\n");
		if(fioClose(fd)<0) scr_printf("Error in Close.\n");
		}
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
		goto error;
		}

	eph = (elf_pheader_t *)(boot_elf + eh->phoff);

	for (i = 0; i < eh->phnum; i++)
	{
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		pdata = (void *)(boot_elf + eph[i].offset);
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);
		}


	dbgprintf("Starting %x\n",(void *)eh->entry);
	SifExitRpc();
	SifInitRpc(0);
	SifExitRpc();
	FlushCache(0);
	FlushCache(2);
	argv[0] = filename;
	argv[1] = elfName;
	ExecPS2((void *)eh->entry, 0, 2, argv);

error:
	while (1) ;

	}