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

/* extern u8 *iomanX_irx;
extern u8 *iomanX_irx_end;
extern u8 *fileXio_irx;
extern u8 *fileXio_irx_end;
extern u8 *ps2dev9_irx;
extern u8 *ps2dev9_irx_end;
extern u8 *ps2atad_irx;
extern u8 *ps2atad_irx_end;
extern u8 *ps2hdd_irx;
extern u8 *ps2hdd_irx_end;
extern u8 *ps2fs_irx;
extern u8 *ps2fs_irx_end; */
extern u32 *_end;


int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;
char elfName[256];
char elfPath[256];
char HDDpath[256]="host:hddmenu.elf";
char partition[40];
char iomanX_path[256];
char fileXio_path[256];
char ps2dev9_path[256];
char ps2atad_path[256];
char ps2hdd_path[256];
char ps2fs_path[256];

const char *eeloadimg = "rom0:UDNL rom0:EELOADCNF";
char *imgcmd;
int elfhost=2,elfload=0;

void LoadModules();
void LoadAndRunMCElf(char *filename);

void
wipeUserMem(void)
{
    int i;
    // Whipe user mem
    for (i = 0x100000; i < 0x2000000 ; i += 64) {
        asm (
            "\tsq $0, 0(%0) \n"
            "\tsq $0, 16(%0) \n"
            "\tsq $0, 32(%0) \n"
            "\tsq $0, 48(%0) \n"
            :: "r" (i) );
    }
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

static void
setPathInfo(char *path)
{
    char *ptr;

    strncpy(elfName, path, 255);
    strncpy(elfPath, path, 255);
    elfName[255] = '\0'; 
    elfPath[255] = '\0';


    ptr = strrchr(elfPath, '/');
    if (ptr == NULL) {
        ptr = strrchr(elfPath, '\\');
        if (ptr == NULL) {
            ptr = strrchr(elfPath, ':');
            if (ptr == NULL) {
                scr_printf("Did not find path (%s)!\n", path);
                SleepThread();
            }
        }
    }
    
    ptr++;
    *ptr = '\0';

    /* Paths to modules.  */
    sprintf(iomanX_path, "%s%s", elfPath, "IOMANX.IRX");
    sprintf(ps2dev9_path, "%s%s", elfPath, "PS2DEV9.IRX");
    sprintf(fileXio_path, "%s%s", elfPath, "FILEXIO.IRX");
    sprintf(ps2atad_path, "%s%s", elfPath, "PS2ATAD.IRX");
    sprintf(ps2hdd_path, "%s%s", elfPath, "PS2HDD.IRX");
    sprintf(ps2fs_path, "%s%s", elfPath, "PS2FS.IRX");
    sprintf(HDDpath, "%s%s", elfPath, "HDDMENU.ELF");
    }

// *************************************************************************
// *** MAIN
// *************************************************************************
int main(int argc, char *argv[])
{
	char s[256];

// Initialise
	SifInitRpc(0);
	init_scr();
	scr_printf("Welcome to PS2Menu v1.0\nPlease wait...loading.\n");

	strcpy(s,argv[0]);
	if (argc==1)
	{									// hopefully this is the first
										// time loader has run
		setPathInfo(s);
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
		}
	if (argc==2)				// if call came from hddmenu.elf
	{
		strcpy(partition,argv[1]);
		printf("argv[1] = %s\n", partition);

		if(hddCheckPresent() < 0)
		{
			scr_printf("Error: supported HDD not connected!\n");
			while(1);
			}
		if(hddCheckFormatted() < 0)
		{
			scr_printf("Error: HDD is not properly formatted!\n");
			while(1);
			}
		scr_printf("HDD is connected and formatted.\n");
		strcpy(HDDpath,s);
		elfhost=1;
		}

	scr_printf("HDDmenu module\n");
	LoadAndRunMCElf(HDDpath);
	return 0;
	SleepThread();
error:
	return -1;
	}

////////////////////////////////////////////////////////////////////////
// Wrapper to load module from disc/rom/mc
// Max irx size hardcoded to 300kb atm..
static void
pkoLoadMcModule(char *path, int argc, char *argv)
{
    void *iop_mem;
    int ret;

    scr_printf("LoadMcModule %s\n", path);
    iop_mem = SifAllocIopHeap(1024*300);
    if (iop_mem == NULL) {
        scr_printf("allocIopHeap failed\n");
        SleepThread();
    }
    ret = SifLoadIopHeap(path, iop_mem);
    if (ret < 0) {
        scr_printf("loadIopHeap %s ret %d\n", path, ret);
        SleepThread();
    }
    else {
        ret = SifLoadModuleBuffer(iop_mem, argc, argv);
        if (ret < 0) {
            scr_printf("loadModuleBuffer %s ret %d\n", path, ret);
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
/*	int ret,fd,size;
	int size_iomanX_irx;
	int size_fileXio_irx;
	int size_ps2dev9_irx;
	int size_ps2atad_irx;
	int size_ps2hdd_irx;
	int size_ps2fs_irx; */

	static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20";
	static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40" /*"\0" "-debug"*/;

/*	size_iomanX_irx=&iomanX_irx_end-&iomanX_irx;
	size_fileXio_irx=&fileXio_irx_end-&fileXio_irx;
	size_ps2dev9_irx=&ps2dev9_irx_end-&ps2dev9_irx;
	size_ps2atad_irx=&ps2atad_irx_end-&ps2atad_irx;
	size_ps2hdd_irx=&ps2hdd_irx_end-&ps2hdd_irx;
	size_ps2fs_irx=&ps2fs_irx_end-&ps2fs_irx; */

	if(elfload==2)
	{
/* 		scr_printf("Loading iomanX.irx %i bytes\n", size_iomanX_irx);
		SifExecModuleBuffer(&iomanX_irx, size_iomanX_irx, 0, NULL, &ret);
		scr_printf("Loading fileXio.irx %i bytes\n", size_fileXio_irx);
		SifExecModuleBuffer(&fileXio_irx, size_fileXio_irx, 0, NULL, &ret);
		scr_printf("Loading ps2dev9.irx %i bytes\n", size_ps2dev9_irx);
		SifExecModuleBuffer(&ps2dev9_irx, size_ps2dev9_irx, 0, NULL, &ret);
		scr_printf("Loading ps2atad.irx %i bytes\n", size_ps2atad_irx);
		SifExecModuleBuffer(&ps2atad_irx, size_ps2atad_irx, 0, NULL, &ret);
		scr_printf("Loading ps2hdd.irx %i bytes\n", size_ps2hdd_irx);
		SifExecModuleBuffer(&ps2hdd_irx, size_ps2hdd_irx, sizeof(hddarg), hddarg, &ret);
		scr_printf("Loading ps2fs.irx %i bytes\n", size_ps2fs_irx);
		SifExecModuleBuffer(&ps2fs_irx, size_ps2fs_irx, sizeof(pfsarg), pfsarg, &ret); */

		SifLoadModule("rom0:SIO2MAN", 0, NULL);
		SifLoadModule("rom0:MCMAN", 0, NULL);
		pkoLoadMcModule(iomanX_path, 0, NULL);
//		SifLoadModule(iomanX_path, 0, NULL);
//		pkoLoadMcModule("mc0:/BWLINUX/POWEROFF.IRX", 0, NULL);
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

/* 		scr_printf("Loading fileXio.irx %i bytes\n", size_fileXio_irx);
		SifExecModuleBuffer(&fileXio_irx, size_fileXio_irx, 0, NULL, &ret);
		scr_printf("Loading ps2atad.irx %i bytes\n", size_ps2atad_irx);
		SifExecModuleBuffer(&ps2atad_irx, size_ps2atad_irx, 0, NULL, &ret);
		scr_printf("Loading ps2hdd.irx %i bytes\n", size_ps2hdd_irx);
		SifExecModuleBuffer(&ps2hdd_irx, size_ps2hdd_irx, sizeof(hddarg), hddarg, &ret);
		scr_printf("Loading ps2fs.irx %i bytes\n", size_ps2fs_irx);
		SifExecModuleBuffer(&ps2fs_irx, size_ps2fs_irx, sizeof(pfsarg), pfsarg, &ret); */
		}
	}

void LoadAndRunMCElf(char *filename)
{
	u8 *boot_elf = (u8 *)0x1800000;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;
	void *pdata;

	int fd, size, i, ret;
	char *argv[1];

	scr_printf("Start of LoadAndRunElf\nelfhost=%i\nfilename=%s\n",elfhost,filename);

	if(elfhost==1)
	{
		ret = fileXioMount("pfs0:", partition, FIO_MT_RDONLY);
		if ((fd = fileXioOpen(filename, O_RDONLY, fileMode)) < 0)
		{
			scr_printf("Failed in fileXioOpen %s\n",filename);
			goto error;
			}

		size = fileXioLseek(fd, 0, SEEK_END);
		if (!size)
		{
			scr_printf("Failed in fileXioLseek\n");
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
/* Load the ELF into RAM.  */
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
		goto error;
		}

	eph = (elf_pheader_t *)(boot_elf + eh->phoff);

/* Scan through the ELF's program headers and copy them into RAM, then
									zero out any non-loaded regions.  */
	for (i = 0; i < eh->phnum; i++)
	{
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		pdata = (void *)(boot_elf + eph[i].offset);
		scr_printf("%x %x %x\n",eph[i].vaddr,pdata,eph[i].filesz);
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);
		}

/* Let's go.  */

	scr_printf("Starting %s\n",elfName);
//	fioExit(); //*
	SifExitRpc(); //*
//	while(!SifIopSync());
//	SifResetIop(); //*
//	while(!SifIopSync());
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