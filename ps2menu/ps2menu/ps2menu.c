/*
	PS2Menu v2.0
	Adam Metcalf 2003/4
	Thomas Hawcroft 2003/4	- changes to make stable code
					- added host file copy list - through elflist.txt
					- 	dos command "dir *.elf /b /s >elflist.txt"
					- added scrolling filelist and screen text clipping
					- added partition/volume switching via PAD L1 button
					- added create folder on pfs0: via PAD SQUARE button
					- added on screen keyboard function
					- added pfs0: directory changing via PAD CROSS button
					- added delete file or empty folder via PAD CIRCLE button
					- added a user confirmation option
					- added drawline functions to tart-up display
					- added PCX import and display functions
					- changed display resolution to 512x512
					- added two new larger fonts and display routines
					- added fakehost to loader and iuntar to menu
					- added triangle button to quit pop-up windows
					- cleared screen buffer before redraw at start of program
					- changed R1 to toggle host: / HDD view
					- changed pad-left and pad-right to semi-page-up/page-down
					- added help screen

	based on mcbootmenu.c - James Higgs 2003 (based on mc_example.c (libmc API sample))
	and libhdd v1.0, ps2drv, ps2link v1.2
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
#include "fontset.c"
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

#define DEBUG
#ifdef DEBUG
#define dbgprintf(args...) printf(args)
#else
#define dbgprintf(args...) do { } while(0)
#endif

// ELF-loading stuff
#define ELF_MAGIC		0x464c457f
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

extern u8 *iomanx_irx;			// (c)2003 Marcus R. Brown <mrbrown@0xd6.org> IOP module
extern int size_iomanx_irx;		// from PS2DRV to handle 'standard' PS2 device IO
extern u8 *filexio_irx;			// (c)2003 adresd <adresd_ps2dev@yahoo.com> et al IOP module
extern int size_filexio_irx;		// from PS2DRV to handle 'extended' PS2 device IO
extern u8 *ps2dev9_irx;			// (c)2003 Marcus R. Brown <mrbrown@0xd6.org> IOP module
extern int size_ps2dev9_irx;		// from PS2DRV to handle low-level HDD device
extern u8 *ps2atad_irx;			// (c)2003 Marcus R. Brown <mrbrown@0xd6.org> IOP module
extern int size_ps2atad_irx;		// from PS2DRV to handle low-level ATA for HDD
extern u8 *ps2hdd_irx;			// (c)2003 Vector IOP module for PS2 HDD
extern int size_ps2hdd_irx;		// from LIBHDD v1.0
extern u8 *ps2fs_irx;			// (c)2003 Vector IOP module for PS2 Filesystem
extern int size_ps2fs_irx;		// from LIBHDD v1.0
extern u8 *poweroff_irx;		// (c)2003 Vector IOP module to handle PS2 reset/shutdown
extern int size_poweroff_irx;		// from LIBHDD v1.0
extern u8 *iuntar_irx;			// (c)2004 adresd <adresd_ps2dev@yahoo.com> et al IOP module
extern int size_iuntar_irx;		// from PS2DRV to handle host:.TGZ extraction to HDD
extern u8 *loader_elf;
extern int size_loader_elf;
extern u8 *ps2menu_pcx;
extern int size_ps2menu_pcx;
//extern u8 *mainlogo_pcx;
//extern int size_mainlogo_pcx;

extern void *_end;

#define TYPE_MC
//#define TYPE_XMC
#define ROM_PADMAN
//#define DOWEFORMAT					// 
								// Only remove '//' if we really don't
								// mind wiping incompatible filesystem

#define WIDTH	512
#define HEIGHT	512
#define FRAMERATE	4
#define STATUS_Y	432
#define MAX_PARTITIONS	10

unsigned char *Img;					// pntr to blit buffer
int g_nWhichBuffer = 0;
int show_logo = 0;
int paletteindex = 0;
char sStatus[256];
char foldername[128]="\0";

iox_dirent_t dirbuf __attribute__((aligned(16)));
char HDDfiles[256][256];
unsigned int HDDstats[256];
int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;
char HDDpath[256];
t_hddFilesystem parties[MAX_PARTITIONS] __attribute__((aligned(64)));

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
int topfil=0, elfload=0;

////////////////////////////////////////////////////////////////////////
// Tests for valid ELF file 
int checkELFheader(char *filename)
{
	u8 *boot_elf = (u8 *)0x1800000;
	elf_header_t *eh = (elf_header_t *)boot_elf;

	int fd, size, ret;
	char fullpath[256];

	ret = fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDONLY);
	strcpy(fullpath,HDDpath);
	strcat(fullpath,filename);
	if ((fd = fileXioOpen(fullpath, O_RDONLY, fileMode)) < 0)
	{
		dbgprintf("Failed in fileXioOpen %s\n",fullpath);
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
	fileXioRead(fd, boot_elf, 52);
	fileXioClose(fd);
	fileXioUmount("pfs0:");

	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
		dbgprintf("Not a recognised ELF.\n");
		goto error;
		}
	return 1;
error:
	return -1;
	}

////////////////////////////////////////////////////////////////////////
// Fills screen with a colour
void clrEmuScreen(unsigned char colour)
{
	unsigned char *pp;
	unsigned int numbytes;

	pp = pScreen;
	numbytes = g_nScreen_X * g_nScreen_Y;
	while(numbytes--) *pp++ = colour;
	}

////////////////////////////////////////////////////////////////////////
// draw a char using the system font (8x8)
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

////////////////////////////////////////////////////////////////////////
// draw a char using selected font (16x16)
void drawBIGChar(char c, int x, int y, unsigned int font, unsigned int colour)
{
	unsigned int i, j;
	u16 cc;
	unsigned char *pp;
	u16 *pc;

	if(font>1) font=1;
// set character pointer
	pc = &fontset[((c-32)*16)+(font*2048)];

// set screen pointer
	pp = pScreen + x + (g_nScreen_X * y);
	for(i=0; i<16; i++) {
		cc = *pc++;
		for(j=0; j<16; j++) {
			if(cc & 0x8000) *pp = colour;
			cc = cc << 1;
			pp++;
			}
		pp += (g_nScreen_X - 16);
		}
	}


////////////////////////////////////////////////////////////////////////
// draw a horizontal line
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

////////////////////////////////////////////////////////////////////////
// draw a vertical line
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

////////////////////////////////////////////////////////////////////////
// draw a string of characters (8x8) forces 'CR-LF' at screen limit
void printXY(char *s, int x, int y, unsigned int colour)
{
	while(*s) {
		drawChar(*s++, x, y, colour);
		x += 8;
		if(x>=502)
		{
			x=0;
			y=y+8;
			}
		}
	}

////////////////////////////////////////////////////////////////////////
// draw a string of characters (16x16) trimmed to (16x12)
// forces 'CR-LF' at screen limit
void printBIGXY(char *s, int x, int y, unsigned int font, unsigned int colour)
{
	while(*s) {
		drawBIGChar(*s++, x, y, font, colour);
		x += 12;
		if(x>=502)
		{
			x=0;
			y=y+16;
			}
		}
	}

////////////////////////////////////////////////////////////////////////
// copies screen buffer to display
void PutImage(void)
{
	UpdateScreen();					// uploads+and renders new screen.
								// (palette is also updated every frame for effects)
	while (TestVRstart() < FRAMERATE);		// wait for FRD number of vblanks
	ClearVRcount();
	g_nWhichBuffer ^= 1;
	SetupScreen( g_nWhichBuffer );		// FLIP!!!
	}

////////////////////////////////////////////////////////////////////////
// draws a PCX file from program memory to screen buffer
void drawPCX(u8 *pcxfile, int pcxlength, int xpos, int ypos)
{
	typedef struct
	{
		unsigned char	manufacturer;
		unsigned char	version;
		unsigned char	encoding;
		unsigned char	bitsperpel;
		unsigned short	xmin;
		unsigned short	ymin;
		unsigned short	xmax;
		unsigned short	ymax;
		unsigned short	hrez;
		unsigned short	vrez;
		unsigned char	colormap[48];
		unsigned char	reserved;
		unsigned char	nplanes;
		unsigned short	bytesperline;
		unsigned short	paletteinfo;
		unsigned char	padding[58];
	} pcxHead;

	pcxHead *pcxHeader;
	unsigned char *pp;
	unsigned char *colors;
	int color,num;
	int xsize,ysize,row,col,imagelength;

	pcxHeader=pcxfile;
	xsize=(pcxHeader->xmax-pcxHeader->xmin)+1;
	ysize=(pcxHeader->ymax-pcxHeader->ymin)+1;
	imagelength=pcxlength-(128+769);
	colors=(unsigned char *)pcxfile+128;
	color=0;
	for(row=0;row<ysize;row++)
	{
		pp = pScreen + xpos + (g_nScreen_X * (ypos+row));
		for(col=0;col<xsize;col++)
		{
			if(colors[color] >= 192)
			{
				num=colors[color] - 192;
				color++;
				col--;
				for(num=num;num>0;num--)
				{
					*pp=(colors[color]+pcxHeader->reserved);
					pp++;
					col++;
				}
				color++;
			}
			else
			{
				*pp=(colors[color]+pcxHeader->reserved);
				pp++;
				color++;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// reads PCX in program memory, mainly to extract and set up palette
void readPCXheader(u8 *pcxfile, int pcxlength)
{
	typedef struct
	{
		unsigned char	manufacturer;
		unsigned char	version;
		unsigned char	encoding;
		unsigned char	bitsperpel;
		unsigned short	xmin;
		unsigned short	ymin;
		unsigned short	xmax;
		unsigned short	ymax;
		unsigned short	hrez;
		unsigned short	vrez;
		unsigned char	colormap[48];
		unsigned char	reserved;
		unsigned char	nplanes;
		unsigned short	bytesperline;
		unsigned short	paletteinfo;
		unsigned char	padding[58];
	} pcxHead;

	pcxHead *pcxHeader;
	unsigned char *colors;
	int palette,color,num;

	pcxHeader=pcxfile;
	if(pcxHeader->nplanes==1)				// check for 256 color palette
	{
		palette=pcxlength-769;
		colors=(unsigned char *)pcxfile+(pcxlength-769);

		if(colors[0]==12)
		{
			num=paletteindex;
			pcxHeader->reserved=paletteindex;
			for(color=1;color<(256*3);color=color+3)
			{
				if(((colors[color] + colors[color+1]) + colors[color+2])==0) color=(256*3);
				else
				{
					SetPaletteEntry(((colors[color+2]<<16)+(colors[color+1]<<8))+(colors[color]), num);
				}
				num++;
			}
			num--;
			paletteindex=num;
		}
	}
}

////////////////////////////////////////////////////////////////////////
// reads version of PS2 ROM 
void GetROMVersion(void)
{
	int fd = fioOpen("rom0:ROMVER", O_RDONLY);
	fioRead(fd, romver, sizeof romver);
	fioClose(fd);
	romver[15] = 0;
	}

////////////////////////////////////////////////////////////////////////
// try to format HDD and create default partition if one could not be
// detected
int dowereformat()
{
	int ret;

#ifdef DOWEFORMAT
	dbgprintf("This will erase all data on the HDD!!!\n");
	if (hddFormat() < 0)
	{
		dbgprintf("ERROR: could not format HDD!\n");
		return -1;
		}
	else
	{
		dbgprintf("HDD is connected and formatted.\n");
		}
#endif
	ret = hddMakeFilesystem(4096, "PS2MENU", FS_GROUP_COMMON);
	if (ret < 0)
	{
		dbgprintf("ERROR: failed to create PS2MENU filesystem: %d\n", ret);
		return -1;
		}
	else
	{
		dbgprintf("Created PS2MENU filesystem with size: %dMB.\n",ret);
		}

	return 1;
	}

////////////////////////////////////////////////////////////////////////
// reads current partition/folder contents to our directory contents
// array
void ReadHDDFiles()
{
	int rv,fd=0;
	char filesname[256];


	fileXioUmount("pfs0:");
	rv = fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDONLY);
	if(rv < 0)
	{
		dbgprintf("ERROR: failed to mount filesystem: %s %d\n", parties[party].filename, rv);
		}
	if(rv == 0)
	{
		fd = fileXioDopen(HDDpath);
		num_hdd_files=0;
		while((rv=fileXioDread(fd, &dirbuf)))
		{
			strcpy(filesname, (char *)&dirbuf.name);
			sprintf (HDDfiles[num_hdd_files],"%s",filesname);
			HDDstats[num_hdd_files]=dirbuf.stat.mode;
			if ((HDDstats[num_hdd_files] & FIO_S_IFDIR) || (HDDstats[num_hdd_files] & FIO_S_IFREG)) num_hdd_files++;
			}
		}
	fileXioDclose(fd);
	fileXioUmount("pfs0:");
}

////////////////////////////////////////////////////////////////////////
// C standard strrchr func.. returns pointer to the last occurance of a
// character in a string, or NULL if not found
// PS2Link (C) 2003 Tord Lindstrom (pukko@home.se)
//         (C) 2003 adresd (adresd_ps2dev@yahoo.com)
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

////////////////////////////////////////////////////////////////////////
// Fill selectElf window with up to 20 rows of files from our directory
// contents array, folders are marked accordingly
void PrintHDDFiles(int highlighted)
{
	int i,texcol,maxrows,maxchars;
	char s[256];
	char textfit[(WIDTH/12)-2];
	char *ptr;

	texcol=1;
	maxrows=20;
	maxchars=(WIDTH/12)-3;
	textfit[maxchars+1]='\0';
	for(i=0; i<num_hdd_files && i < maxrows; i++)
	{
		ptr = strrchr(HDDfiles[i+topfil], '/');
		if (ptr == NULL)
		{
			ptr = strrchr(HDDfiles[i+topfil], '\\');
			if (ptr == NULL)
			{
				ptr = strrchr(HDDfiles[i+topfil], ':');
				}
			}

		if(ptr == NULL)
		{
			if ((HDDstats[i+topfil])&FIO_S_IFDIR)
			{
				strncpy(s, HDDfiles[i+topfil], maxchars-6);
				strcat(s, " (dir)");
				}
			if ((HDDstats[i+topfil])&FIO_S_IFREG)
			{
				strncpy(s, HDDfiles[i+topfil], maxchars);
				}
			}
		else
		{
			ptr++;
			if ((HDDstats[i+topfil])&FIO_S_IFDIR)
			{
				strncpy(s, ptr, maxchars-6);
				strcat(s, " (dir)");
				}
			if ((HDDstats[i+topfil])&FIO_S_IFREG)
			{
				strncpy(s, ptr, maxchars);
				}
			}
		if(highlighted == i+topfil) texcol=2;
		printBIGXY(s, 16, i*16 + 40, 0, texcol);
		texcol=1;
		}
	}

////////////////////////////////////////////////////////////////////////
// Reads a list of paths from host:elflist.txt to our directory contents
// array. This will have to do until host: support is extended to handle
// DRead, etc.
void ReadHostDir(void)
{
	int fd,rv,elflist_size,pathlen;
	char* elflist_buffer;
	char botcap;

	num_hdd_files=0;
	fd=fioOpen("host:elflist.txt", O_RDONLY);
	if(fd<=0)
	{
		dbgprintf("elflist.txt not found on host.\n");
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

////////////////////////////////////////////////////////////////////////
// Attempt to copy specified file to active HDD partition/folder
int tomcopy(char *sourcefile)
{
	int ret;
	int boot_fd; 
	size_t boot_size;
	char *boot_buffer, *ptr, *argv[2];
	char empty='\0',destination[256],filetype[32];
	static char iuntar[512];

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
	ptr = strrchr(sourcefile, '.');
	if (ptr != NULL)
	{
		ptr++;
		strcpy(filetype,ptr);
		if((!strncmp(filetype,"tgz",3))||(!strncmp(filetype,"tar",3))||(!strncmp(filetype,"gz",2)))
		{
			argv[1]=sourcefile;
			argv[2]=parties[party].filename;
			dbgprintf("Loading %s %s\n", argv[1],argv[2]);
			sprintf(iuntar,"%s%c%s%c", argv[1], empty, argv[2], empty);
			SifExecModuleBuffer(&iuntar_irx, size_iuntar_irx, strlen(argv[1])+strlen(argv[2])+2, iuntar, &ret);
			return ret;
			}
		}

	fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);	
	boot_fd = fioOpen(sourcefile, O_RDONLY);
	if(boot_fd <= 0)
	{
		dbgprintf("Open %s Failed\n",sourcefile);
		return boot_fd;
		}
	else
	{
		boot_size = fioLseek(boot_fd,0,SEEK_END);
		fioLseek(boot_fd,0,SEEK_SET);
		boot_buffer = malloc(boot_size);
		if ((boot_buffer)==NULL)
		{
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
		dbgprintf("Open %s Failed\n",destination);
		}
	else
	{
		ret = fileXioWrite(boot_fd,boot_buffer,boot_size);
		fileXioClose(boot_fd);
		}
	fileXioUmount("pfs0:"); 
	return ret;
	}

////////////////////////////////////////////////////////////////////////
// Main screen drawing function
void ReDraw(int highlighted)
{
	int numc;
	char txt[2];
	char fullpath[262];

	txt[1]='\0';
	numc=1;
	clrEmuScreen(0);
	drawPCX(&ps2menu_pcx,size_ps2menu_pcx, 264, 0);
	printXY("Adam & Tom's HDD Boot/Copy Menu", 4, 0, 1);
	printXY("Working volume:", 4, 16, 1);
	printXY(parties[party].filename, 128, 16, 2);
	if(elfhost==1) printXY("Select an ELF to run:", 4, 24, 1);
	if(elfhost==2) printXY("Select an ELF to copy:", 4, 24, 1);
	drawHorizontal(8, 36, 496, 1);
	drawHorizontal(12, 38, 488, 1);
	drawVertical(8, 36, 332, 1);
	drawVertical(12, 38, 328, 1);
	drawVertical(502, 36, 332, 1);
	drawVertical(498, 38, 328, 1);
	drawHorizontal(8, 368, 496, 1);
	drawHorizontal(11, 366, 489, 1);
	PrintHDDFiles(highlighted);
	if(elfhost==1)
	{
		strcpy(fullpath, HDDpath);
		strcat(fullpath, HDDfiles[highlighted]);
		}
	else strcpy(fullpath, HDDfiles[highlighted]);
	printXY(fullpath, 4, 374, 2);
	printBIGXY("Press SELECT for help screen", 4, 390, 0, 1);
//	printXY(sStatus, 4, STATUS_Y, 2);
	PutImage();
	}

////////////////////////////////////////////////////////////////////////
// Display a window with a list of Joypad buttons and their function
void showhelp()
{
	int triangle = 0, i, ret;
	struct padButtonStatus buttons;
	u32 paddata;
	u32 old_pad = 0;
	u32 new_pad;

	triangle=0;
	for(i=96;i<=264;i++)
	{
		drawHorizontal(58, i, 408, 0);
		}
	drawHorizontal(58,96,408,2);
	drawHorizontal(58,264,408,2);
	drawVertical(58,96,168,2);
	drawVertical(466,96,168,2);
	printXY("Functions of the Joypad in PS2MENU", 66, 104, 1);
	printXY("D-PAD:", 66, 120, 1);
	printXY("UP:       move highlight one step up in list.", 66, 128, 1);
	printXY("DOWN:     move highlight one step down in list.", 66, 136, 1);
	printXY("LEFT:     move highlight ten steps up in list.", 66, 144, 1);
	printXY("RIGHT:    move highlight ten steps down in list.", 66, 152, 1);
	printXY("BUTTONS:", 66, 168, 1);
	printXY("SQUARE:   Create folder in current path.", 66, 176, 1);
	printXY("CIRCLE:   Delete current highlight.", 66, 184, 1);
	printXY("TRIANGLE: Cancel pop-up from Create/Delete/HELP.", 66, 192, 1);
	printXY("CROSS:    PS2 PARTITION view, execute highlighted", 66, 200, 1);
	printXY("|         file or chdir to highlighted folder.", 66, 208, 1);
	printXY("|         HOST:ELFLIST.TXT view, copy highlighted", 66, 216, 1);
	printXY("|         file to active PS2 folder, or extract", 66, 224, 1);
	printXY("|________ .tgz file to active PS2 partition.", 66, 232, 1);
	printXY("L1:       Toggle active partition on the PS2 HDD.", 66, 240, 1);
	printXY("R1:       Toggle host:elflist.txt / PS2 HDD view.", 66, 248, 1);
	PutImage();
	while(!triangle)
	{
		ret = padRead(0, 0, &buttons); // port, slot, buttons
            
		if (ret != 0)
		{
			paddata = 0xffff ^ ((buttons.btns[0] << 8) | buttons.btns[1]);
			new_pad = paddata & ~old_pad;
			old_pad = paddata;
			if(new_pad & PAD_TRIANGLE) triangle=1;
			}
		}
	} 

////////////////////////////////////////////////////////////////////////
// Display a simple YES/NO window for user confirmation of argument
int ConfirmYN(char *s)
{
	int enterkey = 0, keycol=0, ret, i;
	struct padButtonStatus buttons;
	u32 paddata;
	u32 old_pad = 0;
	u32 new_pad;

	enterkey=0;
	while(!enterkey)
	{
		for (i=79;i<112;i++)
		{
			drawHorizontal(138, i, 73, 0);
			}
		drawHorizontal(138, 87, 73, 1);
		drawVertical(138, 87, 25, 1);
		drawHorizontal(138, 112, 73, 1);
		drawVertical(211, 87, 25, 1);
		printXY(s, 146, 79, 2);
		printXY("YES  NO", 146, 95, 1);
		if(keycol==0)
		{
			printXY("NO", 186, 95, 2);
			}
		else
		{
			printXY("YES", 146, 95, 2);
			}
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
				if(keycol==0) keycol=1;
				else keycol=0;
				}
			if(new_pad & PAD_RIGHT)
			{
				if(keycol==0) keycol=1;
				else keycol=0;
				}
			if(new_pad & PAD_CROSS)
			{
				enterkey=1;
				}
			if(new_pad & PAD_TRIANGLE)
			{
				keycol=0;
				enterkey=1;
				}
			}
		}
	return keycol;
	}
	
////////////////////////////////////////////////////////////////////////
// Simple on screen keyboard for user input, only used for creating a
// folder thus far. Text is written to global string foldername, and
// limited to 24 characters at the moment
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
	foldername[nameptr]='\0';
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
		printXY((char *)&foldername, 83, 71, 1);
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
			if(new_pad & PAD_TRIANGLE)
			{
				foldername[0]='\0';
				enterkey=1;
				}
			}
		}
	}

////////////////////////////////////////////////////////////////////////
// Print user feedback to the status line on the screen
void jprintf(char *s)
{
	dbgprintf("%s\n", s);
	printXY(s, 4, STATUS_Y, 2);
	ReDraw(0);
	}

////////////////////////////////////////////////////////////////////////
// main user loop, returns a pointer to a pathname if a valid ELF file
// is selected from the list of HDDfiles
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
				for(i=0;i<10;i++)
				{
					if((topfil==highlighted) && (0 < highlighted))
					{
						topfil--;
						highlighted--;
						}
					else if(0 < highlighted) highlighted--;
					}
				}
            	if(new_pad & PAD_DOWN)
			{
				if((highlighted-topfil > 18) & (highlighted < num_hdd_files-1))
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
				for(i=0;i<10;i++)
				{
					if((highlighted-topfil > 18) & (highlighted < num_hdd_files-1))
					{
						topfil++;
						highlighted++;
						}
					else
					{
						if(highlighted < num_hdd_files-1) highlighted++;
						}
					}
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
			if(new_pad & PAD_R1)
			{
				elfhost--;
				if (elfhost<1) elfhost=2;
				highlighted=0;
				topfil=0;
				changed=1;
				}
			if((new_pad & PAD_CIRCLE) && elfhost==1)
			{
				if(ConfirmYN("Delete?"))
				{
				if(HDDstats[highlighted]&FIO_S_IFDIR)
				{
					fileXioMount("pfs0:", parties[party].filename, FIO_MT_RDWR);
					strcpy(sStatus,HDDpath);
					strcat(sStatus,HDDfiles[highlighted]);
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
				else
				{
				highlighted=0;
				topfil=0;
				changed=1;
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
					fileXioMkdir(HDDpath, fileMode);
					fileXioUmount("pfs0:");
					strcat(HDDpath, "/\0");
					}
				changed=1;
				highlighted=0;
				topfil=0;
				}
			if(new_pad & PAD_SELECT)
			{
				showhelp();
				changed=1;
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
						if(checkELFheader(HDDfiles[highlighted])==1)
						{
							sprintf(sStatus, "-  %s -", HDDfiles[highlighted]);
							selected = 1;
							}
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

	sprintf(s, "It is currently using mode %d", padInfoMode(port, slot, PAD_MODECURID, 0));
	jprintf(s);

// If modes == 0, this is not a Dual shock controller 
// (it has no actuator engines)
	if (modes == 0)
	{
		return 1;
		}

	return 1;
	}

void poweroffHandler(int i)
{
	hddPowerOff();
	}

// *************************************************************************
// *** MAIN
// *************************************************************************
int main(int argc, char *argv[])
{
	int i=0,ret,elfsubdir=0;
	char *pc;
	char s[256];


// Initialise
//	SifInitRpc(0);

	if (argc == 0) // Naplink
	{
		argc = 1;
		strcpy(s,"host:");
		elfload = 2;
	}
	else
	{
		strcpy(s,argv[0]);
		if(!strncmp(s, "host:", 5)) elfload=1;		// assume loading from PS2LINK
		else if(!strncmp(s, "mc0:", 4)) elfload=2;	// loading from memory card
	}

	init_scr();
	scr_printf("Welcome to PS2MENU v2.0\nPlease wait...\n");
	if(argc!=2)
	{
		hddPreparePoweroff();
		hddSetUserPoweroffCallback((void *)poweroffHandler,(void *)i);
		LoadModules();
		}

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

	*GS_BGCOLOR =	0x5A5A5A;			// GREY

//	readPCXheader(&mainlogo_pcx,size_mainlogo_pcx);
//	drawPCX(&mainlogo_pcx,size_mainlogo_pcx, 0, 0);

// set palette
	for(i=0; i<16; i++)
	{
		SetPaletteEntry(0x005A5A5A, 0);
		SetPaletteEntry(0x00000000, 1);
		SetPaletteEntry(0x00FFFFFF, 2);
		}
	paletteindex=3;
	clrEmuScreen(0);
	PutImage();
	readPCXheader(&ps2menu_pcx,size_ps2menu_pcx);
// *** Required BEFORE calling ReadMCDir etc
	GetROMVersion();
//#ifdef DEBUG
	dbgprintf("Rom version: %s\n", romver);
//#endif

	strcpy(sStatus, "Ready.\0");
	if(hddCheckPresent() < 0)
	{
		jprintf("Error: supported HDD not connected!\n");
		while(1);
		}
	if(hddCheckFormatted() < 0)
	{
		jprintf("Error: HDD is not properly formatted!\n");
		if(dowereformat()<0)
		{
			while(1);
			}
		}
	i=hddGetFilesystemList(parties, MAX_PARTITIONS);
	nparty=i-1;
	party=-1;
	for(i=nparty;i>0;i--)
	{
		dbgprintf("Found %s\n",parties[i].name);
		if(!strncmp(parties[i].name, "PS2MENU", 7)) party=i;
		}
	if(party<0)
	{
		dbgprintf("PS2MENU partition not found!\nAttempting to create\n");
		if(dowereformat()<0)
		{
			jprintf("Error: Partition could not be created!\n");
			}
		else
		{
			i=hddGetFilesystemList(parties, MAX_PARTITIONS);
			nparty=i-1;
			}
		party=nparty;
		}
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

	if(!initializePad(0, 0))
	{
		jprintf("pad initalization failed!");
		SleepThread();
		}

	ret=padGetState(0, 0);
	while((ret != PAD_STATE_STABLE) && (ret != PAD_STATE_FINDCTP1))
	{
		if(ret==PAD_STATE_DISCONN)
		{
			jprintf("Pad is disconnected");
			}
		ret=padGetState(0, 0);
		}

	dbgprintf("Starting SelectELF()...\n");

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
	dbgprintf("Executing %s ...", s);
	ReDraw(0);

	LoadAndRunMCElf(s);
	}

////////////////////////////////////////////////////////////////////////
// loads all IOP modules from program space or ROM
void LoadModules()
{
	int ret;
	static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20";
	static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40" /*"\0" "-debug"*/;

	if(elfload==2)
	{
		scr_printf("Loading SIO2MAN\n");
		SifLoadModule("rom0:SIO2MAN", 0, NULL);
#ifdef ROM_PADMAN
		scr_printf("Loading PADMAN\n");
		ret = SifLoadModule("rom0:PADMAN", 0, NULL);
#else
		scr_printf("Loading XPADMAN\n");
		ret = SifLoadModule("rom0:XPADMAN", 0, NULL);
#endif 
		if (ret < 0)
		{
			scr_printf("Failed to load PAD module");
			while(1);
//			SleepThread();
			}
 		scr_printf("Loading poweroff.irx %i bytes\n", size_poweroff_irx);
		SifExecModuleBuffer(&poweroff_irx, size_poweroff_irx, 0, NULL, &ret);
 		scr_printf("Loading iomanx.irx %i bytes\n", size_iomanx_irx);
		SifExecModuleBuffer(&iomanx_irx, size_iomanx_irx, 0, NULL, &ret);
		scr_printf("Loading filexio.irx %i bytes\n", size_filexio_irx);
		SifExecModuleBuffer(&filexio_irx, size_filexio_irx, 0, NULL, &ret);
		scr_printf("Loading ps2dev9.irx %i bytes\n", size_ps2dev9_irx);
		SifExecModuleBuffer(&ps2dev9_irx, size_ps2dev9_irx, 0, NULL, &ret);
		scr_printf("Loading ps2atad.irx %i bytes\n", size_ps2atad_irx);
		SifExecModuleBuffer(&ps2atad_irx, size_ps2atad_irx, 0, NULL, &ret);
		scr_printf("Loading ps2hdd.irx %i bytes\n", size_ps2hdd_irx);
		SifExecModuleBuffer(&ps2hdd_irx, size_ps2hdd_irx, sizeof(hddarg), hddarg, &ret);
		scr_printf("Loading ps2fs.irx %i bytes\n", size_ps2fs_irx);
		SifExecModuleBuffer(&ps2fs_irx, size_ps2fs_irx, sizeof(pfsarg), pfsarg, &ret); 
		}
	else
	{
		scr_printf("Loading SIOMAN\n");
		SifLoadModule("rom0:SIO2MAN", 0, NULL);
#ifdef ROM_PADMAN
		scr_printf("Loading PADMAN\n");
		ret = SifLoadModule("rom0:PADMAN", 0, NULL);
#else
		scr_printf("Loading XPADMAN\n");
		ret = SifLoadModule("rom0:XPADMAN", 0, NULL);
#endif 
		if (ret < 0)
		{
			scr_printf("Failed to load PAD module");
			while(1);
//			SleepThread();
			}
 		scr_printf("Loading poweroff.irx %i bytes\n", size_poweroff_irx);
		SifExecModuleBuffer(&poweroff_irx, size_poweroff_irx, 0, NULL, &ret);
 		scr_printf("Loading filexio.irx %i bytes\n", size_filexio_irx);
		SifExecModuleBuffer(&filexio_irx, size_filexio_irx, 0, NULL, &ret);
		scr_printf("Loading ps2atad.irx %i bytes\n", size_ps2atad_irx);
		SifExecModuleBuffer(&ps2atad_irx, size_ps2atad_irx, 0, NULL, &ret);
		scr_printf("Loading ps2hdd.irx %i bytes\n", size_ps2hdd_irx);
		SifExecModuleBuffer(&ps2hdd_irx, size_ps2hdd_irx, sizeof(hddarg), hddarg, &ret);
		scr_printf("Loading ps2fs.irx %i bytes\n", size_ps2fs_irx);
		SifExecModuleBuffer(&ps2fs_irx, size_ps2fs_irx, sizeof(pfsarg), pfsarg, &ret);
		}
	}

////////////////////////////////////////////////////////////////////////
// loads LOADER.ELF from program memory and passes args of selected ELF
// and partition to it
void LoadAndRunMCElf(char *filename)
{
	u8 *boot_elf = (u8 *)&loader_elf;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;
	void *pdata;
	int i;
	char *argv[1];

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
//	fioExit();
//	SifResetIop();

//	SifExitRpc();
	SifInitRpc(0);
	SifExitRpc();
	FlushCache(0);
	FlushCache(2);

	argv[0] = filename;
	argv[1] = party[parties].filename;

	ExecPS2((void *)eh->entry, 0, 2, argv);

	return;
error:
	while (1) ;

	}
