/*
	Hddboot
	Adam Metcalf 2003
	Thomas Hawcroft 2003	- changes to make stable code
					- added host file copy list - through elflist.txt
					- 	dos command "dir *.elf /b /s >elflist.txt"
					- added scrolling filelist and screen text clipping
					- added partition/volume switching via PAD L1 button
					- added create folder on pfs0: via PAD SQUARE button
					- added on screen keyboard function
					- added pfs0: directory changing via PAD CROSS button
					- added delete file or empty folder via PAD CIRCLE button
					-		still requires a user confirmation option
					- added drawline functions to tart-up display

	based on mcbootmenu.c
	and libhdd v1
*/

#include "tamtypes.h"
#include "kernel.h"
#include "sifrpc.h"
#include "loadfile.h"
#include "fileio.h"
#include "iopcontrol.h"
#include "hw.h"
#include "hardware.h"
#include "font5200.c"
#include "stdarg.h"
#include "string.h"
#include "malloc.h"
#include "libpad.h"
#include "libmc.h"
#include "iopheap.h"
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fileXio_rpc.h"
#include "errno.h"
#include "libhdd.h"

static char padBuf[256] __attribute__((aligned(64)));

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

extern void *_end;

#define TYPE_MC
//#define TYPE_XMC
#define ROM_PADMAN
//#define DOWEFORMAT					// DMS3 formatted HDD is not supported
								// Only remove '//' if we really don't
								// mind wiping incompatible filesystem
#define PS2LINKVER
//#define PUKKLINKVER					// LibHdd does not work with Pukklink v1.0b

//#define MCARDBOOT					// Copying HDDMENU.ELF to mc0:/BxDATA-SYSTEM/BOOT.ELF
								// Should allow booting this program via Exploit.
								// assumes all IRX files are in mc0:/BWLINUX
								// - this path can be changed in LoadModules
#define WIDTH	366
#define HEIGHT	256
#define FRAMERATE	4
#define STATUS_Y	232

unsigned char *Img;					// pntr to blit buffer
int g_nWhichBuffer = 0;
int show_logo = 0;
char sStatus[256];
char foldername[26]="\0";

iox_dirent_t dirbuf __attribute__((aligned(64)));
char HDDfiles[128][256];
unsigned int HDDstats[128];
int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;
char HDDpath[256];
t_hddFilesystem parties[10] __attribute__((aligned(64)));

void drawChar(char c, int x, int y, unsigned int colour);
void printXY(char *s, int x, int y, unsigned int colour);
void drawHorizontal(int x, int y, int length, unsigned int colour);
void drawVertical(int x, int y, int length, unsigned int colour);
int do_select_menu(void);
int showDir(char *dir);
int dowereformat();
void ReadHDDFiles();
void LoadModules();
void LoadAndRunMCElf(char *filename);
void MenuKeyboard(char *s);

#define ARRAY_ENTRIES	64
int num_hdd_files,elfhost=1,party=0,nparty;
unsigned char romver[16];
int topfil=0;


void clrEmuScreen(unsigned char colour)
{
	unsigned char *pp;
	unsigned int numbytes;

	pp = pScreen;
	numbytes = g_nScreen_X * g_nScreen_Y;
	while(numbytes--) *pp++ = colour;
	}

// draw a char using the system font
void drawChar(char c, int x, int y, unsigned int colour)
{
	unsigned int i, j;
	unsigned char cc;
	unsigned char *pp;
	unsigned char *pc;

// set character pointer
	pc = &font5200[(c-32)*8];

// set screen pointer
	pp = pScreen + x + (g_nScreen_X * y);
	for(i=0; i<8; i++) {
		cc = *pc++;
		for(j=0; j<8; j++) {
			if(cc & 0x80) *pp = colour;
			cc = cc << 1;
			pp++;
			}
		pp += (g_nScreen_X - 8);
		}
	}

void drawHorizontal(int x, int y, int length, unsigned int colour)
{
	unsigned char *pp;
	unsigned int i;

	pp = pScreen + x + (g_nScreen_X * y);
	for(i=0; i<length; i++)
	{
		*pp = colour;
		pp++;
		}
	}

void drawVertical(int x, int y, int length, unsigned int colour)
{
	unsigned char *pp;
	unsigned int i;

	pp = pScreen + x + (g_nScreen_X * y);
	for(i=0; i<length; i++)
	{
		*pp = colour;
		pp += (g_nScreen_X);
		}
	}

// draw a string of characters
void printXY(char *s, int x, int y, unsigned int colour)
{
	while(*s) {
		drawChar(*s++, x, y, colour);
		x += 8;
		}
	}

void PutImage(void)
{
//    U8 *pScr;
//    U8 *pLine;
//    int x, y;

	UpdateScreen();					// uploads+and renders new screen.
								// (palette is also updated every frame for effects)
	while (TestVRstart() < FRAMERATE);		// wait for FRD number of vblanks
	ClearVRcount();
	g_nWhichBuffer ^= 1;
	SetupScreen( g_nWhichBuffer );		// FLIP!!!
	}

void GetROMVersion(void)
{
	int fd = fioOpen("rom0:ROMVER", O_RDONLY);
	fioRead(fd, romver, sizeof romver);
	fioClose(fd);
	romver[15] = 0;
	}

int dowereformat()
{
	int ret;

#ifdef DOWEFORMAT
	printf("This will erase all data on the HDD!!!\n");
	if (hddFormat() < 0)
	{
		printf("ERROR: could not format HDD!\n");
		return -1;
		}
	else
	{
		printf("HDD is connected and formatted.\n");
		}
#endif
	ret = hddMakeFilesystem(4096, "HDDMENU", FS_GROUP_COMMON);
	if (ret < 0)
	{
		printf("ERROR: failed to create APPS filesystem: %d\n", ret);
		return -1;
		}
	else
	{
		printf("Created APPS filesystem with size: %dMB.\n",ret);
		}

//	ret = fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);
	return 1;
	}

void ReadHDDFiles()
{
	int rv,fd;
	char filesname[256];

	rv = fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDONLY);
	printf("Mounted %s\n", parties[party].filename);
	if(rv < 0)
	{
		printf("ERROR: failed to mount filesystem: %d\n", rv);
//		rv = dowereformat();
		}
	if(rv == 0)
	{
		printf("%s\n",HDDpath);
		fd = fileXioDopen(HDDpath);
		num_hdd_files=0;
		while((rv=fileXioDread(fd, &dirbuf)))
		{
			strcpy(filesname, &dirbuf.name);
			sprintf (HDDfiles[num_hdd_files],"%s",filesname);
			HDDstats[num_hdd_files]=dirbuf.stat.mode;
			if ((HDDstats[num_hdd_files] & FIO_S_IFDIR) || (HDDstats[num_hdd_files] & FIO_S_IFREG)) num_hdd_files++;
			}
		}
	fileXioDclose(fd);
	fileXioUmount("pfs0:");
}

void PrintHDDFiles(int highlighted)
{
	int i,texcol,nchars;
	char s[256];
	char textfit[41];

	texcol=1;
	for(i=0; i<num_hdd_files && i < 20; i++)
	{
		if ((HDDstats[i+topfil])&FIO_S_IFDIR)
		{
			for(nchars=0;nchars<38;nchars++)
			{
				textfit[nchars]=HDDfiles[i+topfil][nchars];
				}
			sprintf(s, "%s (dir)",textfit);
			}
		if ((HDDstats[i+topfil])&FIO_S_IFREG)
		{
			for(nchars=0;nchars<42;nchars++)
			{
				textfit[nchars]=HDDfiles[i+topfil][nchars];
				}
			sprintf(s, "%s", textfit);
			}

		if(highlighted == i+topfil) texcol=2; //s[0] = '*';
		printXY(s, 16, i*8 + 40, texcol);
		texcol=1;
		}
	}

void ReadHostDir(void)
{
	int fd,rv,elflist_size,pathlen;
	char* elflist_buffer;
	char botcap;

	num_hdd_files=0;
	fd=fioOpen("host:elflist.txt", O_RDONLY);
	if(fd<=0)
	{
		printf("elflist.txt not found on host.\n");
		}
	else
	{
		elflist_size=fioLseek(fd,0,SEEK_END);
		fioLseek(fd,0,SEEK_SET);
		elflist_buffer = malloc(elflist_size);
		fioRead(fd, elflist_buffer, elflist_size);
		fioClose(fd);
		free(elflist_buffer);
		for(pathlen=0;pathlen<257;pathlen++)
		{
			HDDfiles[num_hdd_files][pathlen]=(0x00);
			}
		pathlen=5;
		strcpy(HDDfiles[num_hdd_files],"host:");
		for(rv=0;rv<elflist_size;rv++)
		{
			botcap=(elflist_buffer[rv]);
			if(botcap==(0x0d))
			{
				rv=rv++;
				HDDstats[num_hdd_files]=FIO_S_IFREG;
				num_hdd_files++;
				for(pathlen=0;pathlen<257;pathlen++)
				{
					HDDfiles[num_hdd_files][pathlen]=(0x00);
					}
				if(rv<elflist_size)
				{
					pathlen=5;
					strcpy(HDDfiles[num_hdd_files],"host:");
					}
				}
			else
			{
//				if((botcap==0x5c)) botcap='/';
//				if((botcap==0x3a))
//				{
//					pathlen--;
//					pathlen--;
//					rv++;
//					}
				HDDfiles[num_hdd_files][pathlen]=botcap; // else
				pathlen++;
				}
			}
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

int tomcopy(char *sourcefile)
{
	int ret;
	int boot_fd; //,boot_fd2;
	size_t boot_size;
	char *boot_buffer, *ptr;
	char destination[256];

	ptr = strrchr(sourcefile, '/');
	if (ptr == NULL)
	{
		ptr = strrchr(sourcefile, '\\');
		if (ptr == NULL)
		{
			ptr = strrchr(sourcefile, ':');
			}
		}
	ptr++;

	strcpy(destination,"pfs0:");
	strcpy(destination,HDDpath);
	strcat(destination,ptr);
	fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);	
	boot_fd = fioOpen(sourcefile, O_RDONLY);
	if(boot_fd <= 0)
	{
		printf("Open %s Failed\n",sourcefile);
		return boot_fd;
		}
	else
	{
		printf("Open %s Success\n",sourcefile);
		boot_size = fioLseek(boot_fd,0,SEEK_END);
		fioLseek(boot_fd,0,SEEK_SET);
		boot_buffer = malloc(boot_size);
		if ((boot_buffer)==NULL)
		{
			printf("Malloc failed. %i\n", boot_buffer);
//			boot_fd2 = fileXioOpen(destination, O_WRONLY | O_TRUNC | O_CREAT, fileMode);
//			if(boot_fd2 < 0)
//			{
//				printf("Open %s Failed\n",destination);
//				}
//			else
//			{
//				while(boot_size>=128)
//				{
//					boot_size=boot_size-128;
//					fioRead(boot_fd, &sStatus, 128);
//					printf("%i%s\n", boot_size, sStatus);
//					fileXioWrite(boot_fd2,&sStatus,128);
//					}
//				if(boot_size>0)
//				{
//					fioRead(boot_fd,&sStatus,boot_size);
//					fileXioWrite(boot_fd2,&sStatus,boot_size);
//					}
//				fileXioClose(boot_fd2);
//				} The above 'chunk copy' was removed because it appeared not to work
			fioClose(boot_fd);
			return -1; // a file too big for memory cannot be copied
			}
		fioRead(boot_fd, boot_buffer, boot_size);
		fioClose(boot_fd);
		free(boot_buffer);
		}
	boot_fd = fileXioOpen(destination, O_WRONLY | O_TRUNC | O_CREAT, fileMode);
	ret = boot_fd;
	if(boot_fd < 0)
	{
		printf("Open %s Failed\n",destination);
		}
	else
	{
		ret = fileXioWrite(boot_fd,boot_buffer,boot_size);
		fileXioClose(boot_fd);
		}
	fileXioUmount("pfs0:"); 
	return ret;
	}

void ReDraw(int highlighted)
{

	clrEmuScreen(0);
	printXY("Adam & Tom's HDD Boot/Copy Menu", 4, 4, 1);
	printXY("Working volume:", 4, 16, 1);
	printXY(parties[party].filename, 132, 16, 2);
	if(elfhost==1) printXY("Select an ELF to run:", 4, 24, 1);
	if(elfhost==2) printXY("Select an ELF to copy:", 4, 24, 1);
	drawHorizontal(8, 36, 348, 1);
	drawHorizontal(11, 38, 343, 1);
	drawVertical(8, 36, 168, 1);
	drawVertical(11, 38, 164, 1);
	drawVertical(356, 36, 168, 1);
	drawVertical(353, 38, 164, 1);
	drawHorizontal(8, 204, 348, 1);
	drawHorizontal(11, 202, 343, 1);
	PrintHDDFiles(highlighted);
	if(elfhost==1) printXY(HDDpath, 4, 208, 2);
	printXY(sStatus, 4, STATUS_Y, 2);
	PutImage();
	}

void MenuKeyboard(char *s)
{
	int i,ret,keyrow=0,keycol=0,nameptr=0;
	int enterkey = 0;
	struct padButtonStatus buttons;
	u32 paddata;
	u32 old_pad = 0;
	u32 new_pad;
	char keysrow1[130]="0 1 2 3 4 5 6 7 8 9 ( ) .\0A B C D E F G H I J K L M\0N O P Q R S T U V W X Y Z\0a b c d e f g h i j k l m\0n o p q r s t u v w x y z\0";
	char funcrow[19]="SPACE   DEL   ENTER";

	enterkey=0;
	nameptr=0;
	while(!enterkey)
	{
		for(i=63;i<128;i++)
		{
			drawHorizontal(82, i, 202, 0);
			}
		printXY(s, 83, 63, 2);
		printXY(&keysrow1[0], 83, 80, 1);
		printXY(&keysrow1[26], 83, 88, 1);
		printXY(&keysrow1[52], 83, 96, 1);
		printXY(&keysrow1[78], 83, 104, 1);
		printXY(&keysrow1[104], 83, 112, 1);
		printXY(funcrow, 107, 120, 1);
		drawHorizontal(82,71,202,2);
		drawHorizontal(82,79,202,2);
		drawHorizontal(82,128,202,2);
		drawVertical(82,71,128-71,2);
		drawVertical(82+202,71,128-71,2);
		if (keyrow<5)
		{
			drawChar(keysrow1[(keyrow*26)+(keycol*2)],83+(keycol*16), 80+(keyrow*8), 2);
			}
		else
		{
			if(keycol==0) printXY("SPACE", 107, 120, 2);
			if(keycol==1) printXY("DEL", 170, 120, 2);
			if(keycol==2) printXY("ENTER", 218, 120, 2);
			}
		printXY(&foldername, 83, 71, 1);
		PutImage();
		ret = padRead(0, 0, &buttons); // port, slot, buttons
            
		if (ret != 0)
		{
			paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
			new_pad = paddata & ~old_pad;
			old_pad = paddata;

// Directions
			if(new_pad & PAD_LEFT)
			{
				keycol--;
				if(keyrow<5)
				{
					if (keycol<0) keycol=12;
					}
				else if (keycol<0) keycol=2;
				}
            	if(new_pad & PAD_DOWN)
			{
				keyrow++;
				if(keyrow>=5)
				{
					keyrow=5;
					keycol=0;
					}
				}
			if(new_pad & PAD_RIGHT)
			{
				keycol++;
				if(keyrow<5)
				{
					if (keycol>12) keycol=0;
					}
				else if (keycol>2) keycol=0;
				}
			if(new_pad & PAD_UP)
			{
				keyrow--;
				if(keyrow<0) keyrow=0;
				}
			if(new_pad & PAD_CROSS)
			{
				if(keyrow<5)
				{
					if(nameptr<25)
					{
						foldername[nameptr]=keysrow1[(keyrow*26)+(keycol*2)];
						nameptr++;
						foldername[nameptr]='\0';
						}
					}
				else
				{
					if(keycol==0 && nameptr<25)
					{
						foldername[nameptr]=' ';
						nameptr++;
						foldername[nameptr]='\0';
						}
					if(keycol==1 && nameptr>0)
					{
						nameptr--;
						foldername[nameptr]='\0';
						}
					if(keycol==2)
					{
						enterkey=1;
						}
					}
				}
			}
		}
	}

void jprintf(char *s)
{
#ifdef DEBUG
	printf("%s\n", s);
#endif
	printXY(s, 4, 232, 2);
	}

char *SelectELF(void)
{
	int ret, i;
	int selected = 0;
	int highlighted = 0;
	struct padButtonStatus buttons;
	u32 paddata;
	u32 old_pad = 0;
	u32 new_pad;
	int changed=1;
	char botcap,botcap2;

	strcpy(HDDpath,"pfs0:/\0");
	while(!selected)
	{
		ret = padRead(0, 0, &buttons); // port, slot, buttons
            
		if (ret != 0)
		{
			paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
			new_pad = paddata & ~old_pad;
			old_pad = paddata;

// Directions
			if(new_pad & PAD_LEFT)
			{
				elfhost--;
				if (elfhost<1) elfhost=2;
				highlighted=0;
				topfil=0;
				changed=1;
				}
            	if(new_pad & PAD_DOWN)
			{
				if((highlighted-topfil > 18) && (highlighted < num_hdd_files-1))
				{
					topfil++;
					highlighted++;
					}
				else
				{
					if(highlighted < num_hdd_files-1) highlighted++;
					}
				}
			if(new_pad & PAD_RIGHT)
			{
				elfhost++;
				if (elfhost>2) elfhost=1;
				highlighted=0;
				topfil=0;
				changed=1;
				}
			if(new_pad & PAD_UP)
			{
				if((topfil==highlighted) && (0 < highlighted))
				{
					topfil--;
					highlighted--;
					}
				else if(0 < highlighted) highlighted--;
				}
// Buttons
			if(new_pad & PAD_L1)
			{
				party++;
				if(party>nparty) party=0;
				highlighted=0;
				topfil=0;
				changed=1;
				elfhost=1;
				strcpy(HDDpath,"pfs0:/\0");
				}
			if((new_pad & PAD_CIRCLE) && elfhost==1)
			{
				if(HDDstats[highlighted]&FIO_S_IFDIR)
				{
					fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);
					strcpy(sStatus,HDDpath);
					strcat(sStatus,HDDfiles[highlighted]);
					printf("Remove %s\n",sStatus);
					fileXioRmdir(sStatus);
					fileXioUmount("pfs0:");
					changed=1;
					highlighted=0;
					topfil=0;
					}
				else
				{
					fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);
					strcpy(sStatus,HDDpath);
					strcat(sStatus,HDDfiles[highlighted]);
					fileXioRemove(sStatus);
					fileXioUmount("pfs0:");
					changed=1;
					highlighted=0;
					topfil=0;
					}
				}
			if((new_pad & PAD_SQUARE) && elfhost==1)
			{
				MenuKeyboard("Create folder, enter name");
				if(strlen(foldername)>0)
				{
					fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);
					fileXioChdir(HDDpath);
					strcat(HDDpath,foldername);
					printf("%s\n", HDDpath);
					fileXioMkdir(HDDpath, fileMode);
					fileXioUmount("pfs0:");
					strcat(HDDpath, "/\0");
					changed=1;
					highlighted=0;
					topfil=0;
					}
				}
			else if((new_pad & PAD_START) || (new_pad & PAD_CROSS))
			{
				if(elfhost==1)
				{
					if(HDDstats[highlighted]&FIO_S_IFDIR)
					{
						botcap=HDDfiles[highlighted][0];
						botcap2=HDDfiles[highlighted][1];
						if((botcap=='.')&(botcap2=='.'))
						{
							i=0;
							while(botcap!='\0')
							{
								botcap=HDDpath[i];
								i++;
								}
							i--;
							i--;
							while((i>6)&(botcap!='/'))
							{
								i--;
								botcap=HDDpath[i];
								if(botcap=='/')
								{
									HDDpath[i+1]='\0';
									}
								if(i==6) HDDpath[i]='\0';
								}
							changed=1;
							highlighted=0;
							topfil=0;
							}
						else
						{
							if(botcap=='.')
							{
								printf("Trying to change to '.'\n");
								}
							else
							{
								strcat(HDDpath, HDDfiles[highlighted]);
								strcat(HDDpath, "/\0");
								changed=1;
								highlighted=0;
								topfil=0;
								}
							}
						}
					else
					{
						sprintf(sStatus, "-  %s -", HDDfiles[highlighted]);
						selected = 1;
						}
					}
				else
				{
					tomcopy(HDDfiles[highlighted]);
					}
				}
			}
		if (changed)
		{
			if (elfhost==1) ReadHDDFiles();
			if (elfhost==2) ReadHostDir();
			changed=0;
			}
		ReDraw(highlighted);
		}

	return HDDfiles[highlighted];
	}

/*
 * waitPadReady()
 */
int waitPadReady(int port, int slot)
{
	int state;
	int lastState;
	char stateString[16];

	state = padGetState(port, slot);
	lastState = -1;
	while((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1))
	{
		if (state != lastState) {
		padStateInt2String(state, stateString);
		}
	lastState = state;
	state=padGetState(port, slot);
	}
// Were the pad ever 'out of sync'?
	if (lastState != -1)
	{
		jprintf("Pad OK!\n");
		}
	}


/*
 * initializePad()
 */
int initializePad(int port, int slot)
{

	int modes;
	int i;
	char s[128];

	waitPadReady(port, slot);

// How many different modes can this device operate in?
// i.e. get # entrys in the modetable
	modes = padInfoMode(port, slot, PAD_MODETABLE, -1);
	sprintf(s, "The device has %d modes", modes);
	jprintf(s);

	if (modes > 0)
	{
		for (i = 0; i < modes; i++)
		{
			sprintf(s, "uses mode %d ", padInfoMode(port, slot, PAD_MODETABLE, i));
			jprintf(s);
			}
		}

	sprintf(s, "It is currently using mode %d",
	padInfoMode(port, slot, PAD_MODECURID, 0));
	jprintf(s);

// If modes == 0, this is not a Dual shock controller 
// (it has no actuator engines)
	if (modes == 0)
	{
		return 1;
		}

	return 1;
	}

// *************************************************************************
// *** MAIN
// *************************************************************************
int main(int argc, char *argv[])
{
	int i,ret,elfsubdir=0;
	char *pc;
	char s[256];


// Initialise
	SifInitRpc(0);
	LoadModules();
	hddPreparePoweroff();

// Initialise platform stuff
// detect system, and reset GS into that mode
	if(pal_ntsc() == 3)
	{
		initGraph(3);
		}
	else
	{
		initGraph(2);
		}

	g_nWhichBuffer = 0;
	SetupScreen( g_nWhichBuffer );		// set up MY screen!!!
	*GS_PMODE =		0x0066;

	*GS_BGCOLOR =	0x00FF00;			// GREEN

	install_VRstart_handler();

	*GS_BGCOLOR =	0x00FFFF;			// YELLOW

// set palette
	for(i=0; i<16; i++)
	{
		SetPaletteEntry(0x00000000, 0);
		SetPaletteEntry(0x00808080, 1);
		SetPaletteEntry(0x00FFFFFF, 2);
		}

// *** Required BEFORE calling ReadMCDir etc
	GetROMVersion();
#ifdef DEBUG
	printf("Rom version: %s\n", romver);
#endif

	strcpy(sStatus, "Ready.\0");
	if(hddCheckPresent() < 0)
	{
		printf("Error: supported HDD not connected!\n");
		}
	if(hddCheckFormatted() < 0)
	{
		printf("Error: HDD is not properly formatted!\n");
		if(dowereformat()<0)
		{
			while(1);
			}
		}
	printf("HDD is connected and formatted.\n");
	i=hddGetFilesystemList(parties, 10);
	nparty=i-1;
	strcpy(s,argv[0]);
	i=0;
	while(s[i]!='\0')
	{
		if(s[i]==0x5c) elfsubdir++;
		i++;
		}
		ReDraw(0);
// get the pad going
	padInit(0);
	if((ret = padPortOpen(0, 0, padBuf)) == 0)
	{
		jprintf("padOpenPort failed");
		SleepThread();
		}

	jprintf("padPortOpen() OK\n");

	if(!initializePad(0, 0))
	{
		jprintf("pad initalization failed!");
		SleepThread();
		}

	jprintf("initializePad() OK\n");

	ret=padGetState(0, 0);
	while((ret != PAD_STATE_STABLE) && (ret != PAD_STATE_FINDCTP1))
	{
		if(ret==PAD_STATE_DISCONN)
		{
			jprintf("Pad is disconnected");
			}
		ret=padGetState(0, 0);
		}

	jprintf("Starting SelectELF()...\n");

// main ELF select loop
	pc = SelectELF();

// build correct path to ELF
	padPortClose(0,0);
	if(elfhost==2)
	{
		strcpy(s, "host:");
		for(i=0;i<(elfsubdir-1);i++) strcat(s,"..\\");
		}
	if(elfhost==1) strcpy(s, HDDpath);
	strcat(s, pc);
#ifdef DEBUG
	printf("Path: %s\n", s);
#endif
	sprintf(sStatus, "Executing %s ...", s);
	ReDraw(0);

	LoadAndRunMCElf(s);
	SleepThread();
	}

////////////////////////////////////////////////////////////////////////
// Wrapper to load module from disc/rom/mc
// Max irx size hardcoded to 300kb atm..
static void
pkoLoadMcModule(char *path, int argc, char *argv)
{
    void *iop_mem;
    int ret;

    printf("LoadMcModule %s\n", path);
    iop_mem = SifAllocIopHeap(1024*300);
    if (iop_mem == NULL) {
        printf("allocIopHeap failed\n");
        SleepThread();
    }
    ret = SifLoadIopHeap(path, iop_mem);
    if (ret < 0) {
        printf("loadIopHeap %s ret %d\n", path, ret);
        SleepThread();
    }
    else {
        ret = SifLoadModuleBuffer(iop_mem, argc, argv);
        if (ret < 0) {
            printf("loadModuleBuffer %s ret %d\n", path, ret);
            SleepThread();
        }
    }
    SifFreeIopHeap(iop_mem);
}

void LoadModules()
{
	int ret;

	static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20";
	static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40" /*"\0" "-debug"*/;

#ifdef MCARDBOOT
	SifLoadModule("rom0:SIO2MAN", 0, NULL);
	SifLoadModule("rom0:MCMAN", 0, NULL);  	
//	SifLoadModule("rom0:MCSERV", 0, NULL); 
	pkoLoadMcModule("mc0:/BWLINUX/POWEROFF.IRX", 0, NULL);
	pkoLoadMcModule("mc0:/BWLINUX/IOMANX.IRX", 0, NULL);
	pkoLoadMcModule("mc0:/BWLINUX/FILEXIO.IRX", 0, NULL);
	pkoLoadMcModule("mc0:/BWLINUX/PS2DEV9.IRX", 0, NULL);
	pkoLoadMcModule("mc0:/BWLINUX/PS2ATAD.IRX", 0, NULL);
	pkoLoadMcModule("mc0:/BWLINUX/PS2HDD.IRX", sizeof(hddarg), hddarg);
	pkoLoadMcModule("mc0:/BWLINUX/PS2FS.IRX", sizeof(pfsarg), pfsarg);
#else
	SifLoadModule("host:poweroff.irx", 0, NULL);
	#ifdef PUKKLINKVER
		SifLoadModule("host:iomanX.irx", 0, NULL);
	#endif
	SifLoadModule("host:fileXio.irx", 0, NULL);
	#ifdef PUKKLINKVER
		SifLoadModule("host:ps2dev9.irx", 0, NULL);
	#endif
	SifLoadModule("host:ps2atad.irx", 0, NULL);
	SifLoadModule("host:ps2hdd.irx", sizeof(hddarg), hddarg);
	SifLoadModule("host:ps2fs.irx", sizeof(pfsarg), pfsarg);
#endif

#ifdef ROM_PADMAN
	ret = SifLoadModule("rom0:PADMAN", 0, NULL);
#else
	ret = SifLoadModule("rom0:XPADMAN", 0, NULL);
#endif 
	if (ret < 0)
	{
		printf("Failed to load PAD module");
		SleepThread();
		}

	}

void LoadAndRunMCElf(char *filename)
{
	u8 *boot_elf = (u8 *)&_end;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;
	void *pdata;
	int fd, size, i, ret;
	char *argv[1];

	if(elfhost==1)
	{
		ret = fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDONLY);
		if ((fd = fileXioOpen(filename, O_RDONLY, fileMode)) < 0)
		{
			*GS_BGCOLOR =	0xFF0000;		// RED
			goto error;
			}

		size = fileXioLseek(fd, 0, SEEK_END);
		if (!size)
		{
			fileXioClose(fd);
			*GS_BGCOLOR =	0x00FF00;		// GREEN
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
			*GS_BGCOLOR =	0xFF0000;		// RED
			goto error;
			}

		size = fioLseek(fd, 0, SEEK_END);
		if (!size)
		{
			fioClose(fd);
			*GS_BGCOLOR =	0x00FF00;		// GREEN
			goto error;
			}

		if(fioLseek(fd, 0, SEEK_SET)<0) printf("Error in SEEK_SET.\n");
		if(fioRead(fd, boot_elf, size)<0) printf("Error in Read.\n");
		if(fioClose(fd)<0) printf("Error in Close.\n");
		}
/* Load the ELF into RAM.  */
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
		*GS_BGCOLOR =	0x0000FF;		// BLUE
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
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);
		}

/* Let's go.  */
	argv[0] = filename;

	fioExit();
	SifResetIop();

	FlushCache(0);
	FlushCache(2);

	ExecPS2((void *)eh->entry, 0, 1, argv);


error:
	while (1) ;

	}
// Shutdown cleanly
// SifResetIOP();
// SifInitRpc(0);
// SifExitRpc();
// do a padPortClose before any of above 
// and de-register any dmac or interrupt handlers you have registered
