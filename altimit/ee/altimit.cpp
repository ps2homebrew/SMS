/*========================================================================
==				Altimit.cpp main program file.	                		==
==				(c) 2004 t0mb0la (tomhawcroft@comcast.net)				==
== Refer to the file LICENSE in the main folder for license information	==
========================================================================*/

#include "altimit.h"
//#include "smod.h"
//#include "smem.h"

//#define VERBOSE 1					// output extra runtime info to screen or tty
//#define LOADIRX 1					// use 'make LOADIRX=1' instead
//#define LOADTTY 1					// udptty.irx needs to be compiled with your own IP
#define IPCONF_MAX_LEN  (3*16)		// max size for IP setting strings

int loadhttp = true;
int boot;							// assigned a value corresponding to load path
altimitFS *abstractFS1, *abstractFS2, *activeFS; // file system abstract layers
int activeHDD = false, loadedHDD = false; // status of devices and device modules loaded
int activeMC = false, loadedMC = false;
int activeHTTP = false, loadedHTTP = false;
int activeHOST = false, loadedHOST = false;
int activeCDFS = false, loadedCDFS = false;
int loadedPAD = false, dualshockPAD = false; // status of joypad and joypad type
int loadedMOUSE = false, loadedKEYBD = false;
int activeMOUSE = false, activeKEYBD = false;
int usepointer = true;				// on screen pointer option
extern altimitGS altGS;				// GS configuration and program options
extern gsDriver altGsDriver;		// GSlib pipe
extern gsFont altFont;				// GSfont pipe
extern u8 *loader_elf;
extern int size_loader_elf;
extern char keypress;

#ifdef LOADIRX
extern u8 *poweroff_irx;
extern int size_poweroff_irx;
extern u8 *iomanx_irx;
extern int size_iomanx_irx;
extern u8 *filexio_irx;
extern int size_filexio_irx;
extern u8 *ps2dev9_irx;
extern int size_ps2dev9_irx;
extern u8 *ps2ip_irx;
extern int size_ps2ip_irx;
extern u8 *ps2smap_irx;
extern int size_ps2smap_irx;
extern u8 *ps2host_irx;
extern int size_ps2host_irx;
extern u8 *ps2netfs_irx;
extern int size_ps2netfs_irx;
extern u8 *ps2ftpd_irx;
extern int size_ps2ftpd_irx;
extern u8 *dns_irx;
extern int size_dns_irx;
extern u8 *ps2http_irx;
extern int size_ps2http_irx;
extern u8 *ps2atad_irx;
extern int size_ps2atad_irx;
extern u8 *ps2hdd_irx;
extern int size_ps2hdd_irx;
extern u8 *ps2fs_irx;
extern int size_ps2fs_irx;
extern u8 *cdvd_irx;
extern int size_cdvd_irx;
extern u8 *ps2mouse_irx;
extern int size_ps2mouse_irx;
extern u8 *ps2kbd_irx;
extern int size_ps2kbd_irx;
#endif

unsigned int loadscrY = 0;
char elfloadpath[MAX_PATHNAME];		// full path/filename of program
char loadpath[MAX_PATHNAME];		// path to program folder
char elfrunpath[MAX_PATHNAME];
char filesystem[40];
char copytostring[80];
char printstring[80];				// screen output text
int heldtime, heldbutton;			// counter for held pad button, and button involved
int insmode = false;
int pointerX, pointerY;				// on screen pointer location
int button[MAX_BUTTONS][5];			// array of button data for main screen buttons
char buttontip[MAX_BUTTONS][30];	// array of labels for main screen buttons
TooltipWindow buttontips;			// structure for one line windows
TextWindow testscreen, cfgscreen, oskscreen; // structures for simple windows
FilelistWindow winfilelist;			// structure for file browser window
FunctionWindow filefunctions;
int FONT_WIDTH;						// maximum width of loaded on screen font
unsigned int paddata = 0, old_pad = 0, new_pad = 0;
//_smod_mod_info *loadedMODs;

char if_conf[IPCONF_MAX_LEN];		// IP arguments to be passed when loading PS2SMAP.IRX
int if_conf_len;					// total length of arguments

char ip[16] __attribute__((aligned(16))) = "192.168.0.10"; // default PS2 IP address
char netmask[16] __attribute__((aligned(16))) = "255.255.255.0"; // default IP net mask
char gw[16] __attribute__((aligned(16))) = "192.168.0.1"; // default gateway IP address

extern "C" {
extern void _init(void); }
void initGS();						// function to initialise GS
void ButtonMenu();					// main screen
void infoModule();					// info module
int oskModule(char *edittext, char *osktitle);	// on screen keyboard module
void configModule();				// configuration module
void browserModule();				// file browser module
void RunLoaderElf(char *filename, char *filesystem);


////////////////////////////////////////////////////////////////////////
// Parse network configuration from IPCONFIG.DAT
// Note: parsing really should be made more robust...
// PS2Link (C) 2003 Tord Lindstrom (pukko@home.se)
//         (C) 2003 adresd (adresd_ps2dev@yahoo.com)
static void
getIpConfig(void)
{
	int fd;
	int i;
	int t;
	int len;
	char c;
	char buf[IPCONF_MAX_LEN];

	fd = fioOpen("mc0:/SYS-CONF/IPCONFIG.DAT", O_RDONLY);

	if (fd < 0) 
	{
		scr_printf("Could not find IPCONFIG.DAT, using defaults\n"
					"Net config: %s  %s  %s\n", ip, netmask, gw);
		// Set defaults
		memset(if_conf, 0x00, IPCONF_MAX_LEN);
		i = 0;
		strncpy(&if_conf[i], ip, 15);
		i += strlen(ip) + 1;

		strncpy(&if_conf[i], netmask, 15);
		i += strlen(netmask) + 1;

		strncpy(&if_conf[i], gw, 15);
		i += strlen(gw) + 1;

		if_conf_len = i;
		return;
	}

	memset(if_conf, 0x00, IPCONF_MAX_LEN);
	memset(buf, 0x00, IPCONF_MAX_LEN);

	len = fioRead(fd, buf, IPCONF_MAX_LEN - 1); // Let the last byte be '\0'
	fioClose(fd);

	if (len < 0) {
		scr_printf("Error reading ipconfig.dat\n");
		return;
	}

	i = 0;
	// Clear out spaces (and potential ending CR/LF)
	while ((c = buf[i]) != '\0') {
		if ((c == ' ') || (c == '\r') || (c == '\n'))
			buf[i] = '\0';
		i++;
	}

	scr_printf("Net config: ");
	for (t = 0, i = 0; t < 3; t++) {
		strncpy(&if_conf[i], &buf[i], 15);
		scr_printf("%s  ", &if_conf[i]);
		i += strlen(&if_conf[i]) + 1;
	}
	scr_printf("\n");

	if_conf_len = i;
}

////////////////////////////////////////////////////////////////////////
// Loads program configuration from memory card, or sets defaults and
// creates file if not found or size of file is wrong. Could probably use
// stricter error checking here.
void loadMCconfig()
{
 int fd, fdir, fsize;
 char *cfgbuffer;
 char *sysconf = "mc0:/SYS-CONF";

 cfgbuffer=(char *)&altGS.WIDTH;
 fd = fioOpen("mc0:/SYS-CONF/ALTIMIT.CFG", O_RDONLY);
 if (fd >= 0)
 {
	fsize = fioLseek(fd, 0, SEEK_END);
	fioLseek(fd, 0, SEEK_SET);
 }
 if (fd < 0 || fsize != sizeof(altimitGS))
 {
	altGS.WIDTH = 640;
	altGS.HEIGHT = 480;
	altGS.OFFSETX = 160;
	altGS.OFFSETY = 20;
	altGS.PALORNTSC = GS_TV_AUTO;
	altGS.INTERLACING = GS_TV_INTERLACE;
	altGS.SCREENCOL1 = GS_SET_RGBA(0x80,0x80,0x80,0x80);
	altGS.SCREENCOL2 = GS_SET_RGBA(0x90,0x90,0x80,0x80);
	altGS.POINTERCOL = GS_SET_RGBA(0xFF,0x00,0x00,0xFF);
	altGS.WINBACKCOL = GS_SET_RGBA(0x00,0x00,0x30,0x30);
	altGS.WINFORECOL = GS_SET_RGBA(0x9C,0x9C,0x9C,0xFF);
	altGS.WINBACKHEAD = GS_SET_RGBA(0x00,0x00,0x00,0xFF);
	altGS.WINFOREHEAD = GS_SET_RGBA(0xFF,0x80,0x40,0xFF);
	altGS.LOADNETFS = false;
	altGS.LOADHOST = false;
	altGS.LOADHDDS = false;
	altGS.USEPOINTER = false;
	altGS.LOADMOUSE = false;
	altGS.LOADKEYBD = false;
	altGS.LOADFTPD = false;
 	if ((fdir = fioDopen(sysconf) < 0)) fioMkdir(sysconf); // create mc folder if not present
	else fioDclose(fdir);
	if (fd >= 0) fioClose(fd);
	fd = fioOpen("mc0:/SYS-CONF/ALTIMIT.CFG", O_CREAT | O_WRONLY | O_TRUNC); // create file
	fioWrite(fd, cfgbuffer, sizeof(altimitGS)); // write config data
	fioClose(fd);
 }
 else
 {
	fioRead(fd, cfgbuffer, sizeof(altimitGS)); // read config data
	fioClose(fd);
 }
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
	if (*sp == c) last = sp;
	sp++;
 }
 if (*sp == c) last = sp;
 return (char *) last;
}

////////////////////////////////////////////////////////////////////////
// Loads IOP module from location it is first found in
int tSifLoadModule(char *pathname, int argc, char *argv)
{
 char *mcmodules = "mc0:/SYS-MODULES/\0";	// MrBrown suggested module location
 char *bwmodules = "mc0:/BWLINUX/\0";		// Reload location?
 char *hddmodules = "pfs0:/SYS-MODULES/\0"; // This one is probably a bit too optimistic
 char *pathptr = NULL;
 int ret;

#ifndef VERBOSE
 scr_printf(".");
#endif
 pathptr = strrchr(pathname, ':');			// check if full path was specified
 if (pathptr == NULL)
 {
	strcpy(loadpath, elfloadpath);			// if not, try program folder first
	strncat(loadpath, pathname, MAX_PATHNAME-1);
	if (boot == CD_BOOT) strcat(loadpath, ";1"); // and append if this was loaded off CD
#ifdef VERBOSE
	scr_printf("Trying %s\n", loadpath);
#endif
	ret = SifLoadModule(loadpath, argc, argv);
	if (ret < 0)
	{
		strcpy(loadpath, mcmodules);		// if not found, try Marcus' suggestion first
		strncat(loadpath, pathname, MAX_PATHNAME-1);
#ifdef VERBOSE
		scr_printf("Trying %s\n", loadpath);
#endif
		ret = SifLoadModule(loadpath, argc, argv);
		if (ret < 0)
		{
			strcpy(loadpath, bwmodules);	// try PS2LINUX location if still not found
			strncat(loadpath, pathname, MAX_PATHNAME-1);
#ifdef VERBOSE
			scr_printf("Trying %s\n", loadpath);
#endif
			ret = SifLoadModule(loadpath, argc, argv);
			if (ret < 0)
			{
				strcpy(loadpath, hddmodules); // what the hell, it can only fail now
				strncat(loadpath, pathname, MAX_PATHNAME-1);
#ifdef VERBOSE
				scr_printf("Trying %s\n", loadpath);
#endif
				ret = SifLoadModule(loadpath, argc, argv);
			}
		}
	}
 }
 else
 {
#ifdef VERBOSE
 scr_printf("Loading module %s\n", pathname);
#endif
 	ret = SifLoadModule(pathname, argc, argv);	// only get here when full path given
 }												// i.e. when loading rom0: module
 if (ret < 0)
 {
	scr_printf("Failed loading module %s (%d)\n", pathname, ret);
	return false;
 }
 return true;
}

#ifdef LOADIRX
void tSifExecModuleBuffer(char *name, void *module, int size, int arglen, const char *args, int *ret)
{
#ifndef VERBOSE
 scr_printf(".");
#endif
#ifdef VERBOSE
 scr_printf("Loading buffer module %s\n", name);
#endif
 SifExecModuleBuffer(module, size, arglen, args, ret);
}

void loadIOPbuffers()
{
 static char dnsarg[] = "204.127.198.4"; // this should NOT be hard coded
 static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20"; // hdd module arguments
 static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40";
 int ret;

 if((tSifLoadModule("rom0:SIO2MAN", 0, NULL))
 && (tSifLoadModule("rom0:MCMAN", 0, NULL))
 && (tSifLoadModule("rom0:MCSERV", 0, NULL))) loadedMC = true; // all needed for MC
 loadMCconfig();		// load or initialise configuration
 if((tSifLoadModule("rom0:PADMAN", 0, NULL))) loadedPAD = true; // definately need pad
 tSifExecModuleBuffer("iomanx.irx", &iomanx_irx, size_iomanx_irx, 0, NULL, &ret);
 tSifExecModuleBuffer("filexio.irx", &filexio_irx, size_filexio_irx, 0, NULL, &ret);
 tSifExecModuleBuffer("poweroff.irx", &poweroff_irx, size_poweroff_irx, 0, NULL, &ret);
 if(boot != HOST_BOOT)
 {
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADFTPD || loadhttp) getIpConfig();// reads IPCONFIG.DAT
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADHDDS || altGS.LOADFTPD || loadhttp)
		tSifExecModuleBuffer("ps2dev9.irx", &ps2dev9_irx, size_ps2dev9_irx, 0, NULL, &ret);
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADFTPD || loadhttp)
	{
		tSifExecModuleBuffer("ps2ip.irx", &ps2ip_irx, size_ps2ip_irx, 0, NULL, &ret);
		if (ret == 0)
		{
			tSifExecModuleBuffer("ps2smap.irx", &ps2smap_irx, size_ps2smap_irx, if_conf_len, &if_conf[0], &ret);
			if (ret == 0)
			{
				if (altGS.LOADHOST)
				{
					tSifExecModuleBuffer("ps2host.irx", &ps2host_irx, size_ps2host_irx, 0, NULL, &ret);
					if (ret == 0) loadedHOST = true;
				}
				else if (altGS.LOADNETFS) tSifExecModuleBuffer("ps2netfs.irx", &ps2netfs_irx, size_ps2netfs_irx, 0, NULL, &ret);
				else if (altGS.LOADFTPD) tSifExecModuleBuffer("ps2ftpd.irx", &ps2ftpd_irx, size_ps2ftpd_irx, 0, NULL, &ret);
				else if (loadhttp)
				{
					tSifExecModuleBuffer("dns.irx", &dns_irx, size_dns_irx, sizeof(dnsarg), dnsarg, &ret);
					if (ret == 0)
					{
						tSifExecModuleBuffer("ps2http.irx", &ps2http_irx, size_ps2http_irx, 0, NULL, &ret);
						if (ret == 0) loadedHTTP = true;
					}
				}
			}
		}
	}
#ifdef LOADTTY
	tSifLoadModule("UDPTTY.IRX", 0, NULL);
#endif
 }
 if (altGS.LOADHDDS)
 {
	tSifExecModuleBuffer("ps2atad.irx", &ps2atad_irx, size_ps2atad_irx, 0, NULL, &ret);
	if (ret == 0)
	{
		tSifExecModuleBuffer("ps2hdd.irx", &ps2hdd_irx, size_ps2hdd_irx, sizeof(hddarg), hddarg, &ret);
		if (ret == 0)
		{
			tSifExecModuleBuffer("ps2fs.irx", &ps2fs_irx, size_ps2fs_irx, sizeof(pfsarg), pfsarg, &ret);
			if (ret == 0) loadedHDD = true;
		}
	}
 }
 if (altGS.LOADMOUSE || altGS.LOADKEYBD)
 {
	if(tSifLoadModule("USBD.IRX", 0, NULL))
	{
		if(altGS.LOADMOUSE)
		{
			tSifExecModuleBuffer("ps2mouse.irx", &ps2mouse_irx, size_ps2mouse_irx, 0, NULL, &ret);
			if (ret == 0) loadedMOUSE = true;
		}
		if(altGS.LOADKEYBD)
		{
			tSifExecModuleBuffer("ps2kbd.irx", &ps2kbd_irx, size_ps2kbd_irx, 0, NULL, &ret);
			if (ret == 0) loadedKEYBD = true;
		}
	}
 }
 tSifExecModuleBuffer("cdvd.irx", &cdvd_irx, size_cdvd_irx, 0, NULL, &ret);
 if (ret == 0) loadedCDFS = true;
}
#endif

#ifndef LOADIRX
////////////////////////////////////////////////////////////////////////
// loads all required modules, depending on configuration and how this
// program was loaded
void loadIOPmodules()
{
 static char dnsarg[] = "204.127.198.4";
 static char hddarg[] = "-o" "\0" "4" "\0" "-n" "\0" "20"; // hdd module arguments
 static char pfsarg[] = "-m" "\0" "4" "\0" "-o" "\0" "10" "\0" "-n" "\0" "40";

// ugh, what a freaking mess, but it works (I think :P)
 if((tSifLoadModule("rom0:SIO2MAN", 0, NULL))
 && (tSifLoadModule("rom0:MCMAN", 0, NULL))
 && (tSifLoadModule("rom0:MCSERV", 0, NULL))) loadedMC = true; // all needed for MC
 loadMCconfig();		// load or initialise configuration
 if((tSifLoadModule("rom0:PADMAN", 0, NULL))) loadedPAD = true; // definately need pad
 tSifLoadModule("IOMANX.IRX", 0, NULL);	// always required
 tSifLoadModule("FILEXIO.IRX", 0, NULL); // this too
 tSifLoadModule("POWEROFF.IRX", 0, NULL); // regain control of power/reset button
 if(boot != HOST_BOOT)	// HOST_BOOT hopefully means ps2link, which loads these for us
 {						// so we only load these when we think we know we need them :)
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADFTPD || loadhttp) getIpConfig();// reads IPCONFIG.DAT
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADHDDS || altGS.LOADFTPD || loadhttp)
		tSifLoadModule("PS2DEV9.IRX", 0, NULL); // required for any network or hdd support
	if (altGS.LOADNETFS || altGS.LOADHOST || altGS.LOADFTPD || loadhttp)
	if ((tSifLoadModule("PS2IP.IRX", 0, NULL)) // required for network
	&& (tSifLoadModule("PS2SMAP.IRX", if_conf_len, &if_conf[0]))) // likewise
	{
		if (altGS.LOADHOST) if (tSifLoadModule("PS2HOST.IRX", 0, NULL)) loadedHOST = true;
		else if (altGS.LOADNETFS) if (tSifLoadModule("PS2NETFS.IRX", 0, NULL)) loadedHOST = true; // either of these work well
		else if (altGS.LOADFTPD) tSifLoadModule("PS2FTPD.IRX", 0, NULL);
		else if (loadhttp)
		{
			if (tSifLoadModule("DNS.IRX", sizeof(dnsarg), dnsarg)
				&& tSifLoadModule("PS2HTTP.IRX", 0, NULL)) loadedHTTP = true;
		}
	}
#ifdef LOADTTY
	tSifLoadModule("UDPTTY.IRX", 0, NULL);
#endif
 }
 if (altGS.LOADHDDS)
 {
 	if((tSifLoadModule("PS2ATAD.IRX", 0, NULL)) // all these needed for full hdd support
	&& (tSifLoadModule("PS2HDD.IRX", sizeof(hddarg), hddarg))
	&& (tSifLoadModule("PS2FS.IRX", sizeof(pfsarg), pfsarg))) loadedHDD = true;
 }
 if (altGS.LOADMOUSE || altGS.LOADKEYBD)
 {
	if(tSifLoadModule("USBD.IRX", 0, NULL))
	{
		if(altGS.LOADMOUSE && (tSifLoadModule("PS2MOUSE.IRX", 0, NULL))) loadedMOUSE = true;
		if(altGS.LOADKEYBD && (tSifLoadModule("PS2KBD.IRX", 0, NULL))) loadedKEYBD = true;
	}
 }
 if (tSifLoadModule("CDVD.IRX", 0, NULL)) loadedCDFS = true;
 scr_printf("\n");
}

#endif

////////////////////////////////////////////////////////////////////////
// if on screen pointer is being used, check if it is pointing at
// one of the buttons. returns active button (0->max) or -1 if none
int checkbuttons(int buttons[][5], int max)
{
 int i;

 i = 0;
 while(i < max && buttons[i][0] != -1)
 {
	if(pointerX > buttons[i][0] && pointerX < buttons[i][2] && 
		pointerY > buttons[i][1] && pointerY < buttons[i][3]) break;
	i++;
 }
 if (i < max && buttons[i][0] != -1) return i;
 return -1;
}

////////////////////////////////////////////////////////////////////////
// initial loading of program
int main (int argc, char **argv)
{
 int fdir; //, countmods, modvhi, modvlo;
 char *img = "";
 char *pathptr = NULL;
// char modname[MAX_FILENAME];

 _init();
// gsDriver altGsDriver;
 init_scr();								// initialise text screen
 scr_printf("argc = %d\n", argc);
 if (argc == 0)
 {
	strcpy(elfloadpath,"host:");			// special case for naplink
 }
 else if (argc != 1)
 {
	strcpy(elfloadpath,"mc0:/BWLINUX/\0");	// special case for reload
 }
 else
 {
	strcpy(elfloadpath,argv[0]);			// get loadpath and attempt to parse
	pathptr = strrchr(elfloadpath,'/');
	if (pathptr == NULL)
	{
		pathptr = strrchr(elfloadpath,'\\');
		if (pathptr == NULL)
		{
			pathptr = strrchr(elfloadpath,':');
			if (pathptr == NULL)
			{
				scr_printf("Fatal, unrecognised path (%s)!\n", elfloadpath);
				SleepThread();				// maybe we could proceed here
			}								// or maybe better safe than sorry
		}
	}
	if (pathptr)							// this should be true, but double check anyway
	{
		pathptr++;
		*pathptr = '\0';					// strip down to bare path, no filename
	}
 }
#ifdef VERBOSE
 scr_printf("Loading from %s\n", elfloadpath);
#endif
 boot = 0;
 if(!strncmp(elfloadpath, "cdrom", 5)) boot = CD_BOOT;		// CD needs special treatment
 else if(!strncmp(elfloadpath, "mc", 2)) boot = MC_BOOT;	// MC doesn't
 else if(!strncmp(elfloadpath, "host", 4)) boot = HOST_BOOT;// still nice to have, PS2LINK is still an option
 else if(!strncmp(elfloadpath, "pfs", 3)) boot = PFS_BOOT;	// this is unlikely to occur
 else if(!strncmp(elfloadpath, "vfs", 3)) boot = VFS_BOOT;	// as is this
 else boot = UNK_BOOT;						// who can say what will happen in this case?
 scr_printf("Welcome to Altimit v0.1\n");	// display welcome message
 SifInitRpc(0);
 hddPreparePoweroff();
 if (boot == CD_BOOT)						// just reset IOP for CD boot atm.
 {											// although it may be worth doing in all
	printf("CD BOOT?\n");					// cases eventually. it is currently useful
	cdInit(CD_EXIT);						// to still have ps2link functioning in the
	cdExit();								// background
	fioExit();
	SifExitIopHeap();
	SifLoadFileExit();
	SifExitRpc();
#ifdef VERBOSE
	scr_printf("Reset IOP\n");
#endif
	SifIopReset(img, 0);
#ifdef VERBOSE
	scr_printf("Wait for IOP sync\n");
#endif
	while (SifIopSync());
#ifdef VERBOSE
	scr_printf("Init RPC\n");
#endif
	SifInitRpc(0);
	cdInit(CD_INIT_NOWAIT);
 }
 scr_printf("Init sbv_patches\n");
 sbv_patch_enable_lmb();
 sbv_patch_disable_prefix_check();
 scr_printf("Initialising, Please wait...\n\n");
#ifndef LOADIRX
 loadIOPmodules();							// this is pretty damn important
#endif
#ifdef LOADIRX
 loadIOPbuffers();
#endif
/* loadedMODs = (_smod_mod_info *)malloc(sizeof(_smod_mod_info));
 countmods = 0;
 smod_get_next_mod(0, loadedMODs);
 if(loadedMODs)
 {
	countmods++;
	smem_read(loadedMODs->name, modname, MAX_FILENAME);
	modvhi = (loadedMODs->version / 256);
	modvlo = (loadedMODs->version & 255);
	printf("%s v[%d.%d]\n", modname, modvhi, modvlo);
	while (smod_get_next_mod(loadedMODs, loadedMODs))
	{
		countmods++;
		smem_read(loadedMODs->name, modname, MAX_FILENAME);
		modvhi = (loadedMODs->version / 256);
		modvlo = (loadedMODs->version & 255);
		printf("%s v[%d.%d]\n", modname, modvhi, modvlo);
	}
 }
 printf("Found %d modules on IOP\n", countmods);
 free(loadedMODs);*/
 if (loadedMOUSE) {
	if (PS2MouseInit() > 0) activeMOUSE = true; }
 if (loadedKEYBD) {
	if (PS2KbdInit() > 0) activeKEYBD = true; }
 if (loadedPAD) startpad();					// I bloody well hope PADloaded
 if (loadedHDD) {							// if HDD modules loaded, check hdd is valid
	if (hddCheckPresent() >= 0 && hddCheckFormatted() >= 0) activeHDD = true; }
 if (loadedHOST) activeHOST = true;			// should just require client to connect for host:
 else if (boot == HOST_BOOT)
 {
	if ((fdir = fioOpen("host:elflist.txt", O_RDONLY)) >= 0)	// fileXioDopen banz0rs on inlink
	{ fioClose(fdir); activeHOST = true; }						// so check this option first
	else if ((fdir = fileXioDopen("host:")) >= 0) // if program was loaded from host: check we can
	{ fioDclose(fdir); activeHOST = true; }	// 'Dread' it. ok, I know this isn't working yet
 }											// but it works for me using modified clients & cygwin
											// we're all waiting for the host: 'stat' issues to
											// be resolved.
 if (loadedMC) {							// MC should always be available
	if (mcInit(MC_TYPE_MC) >= 0) activeMC = true; }	// still, check it
 if (loadedCDFS) {							// CDFS should always be available too
	if (CDVD_Init() >= 0) activeCDFS = true; }	// and check this too
 if (loadedHTTP) activeHTTP = true;			// well, one day, maybe =) - still needs some work
 usepointer = altGS.USEPOINTER;
 initGS();									// initialise GS
 ButtonMenu();								// enter main program loop
}

////////////////////////////////////////////////////////////////////////
// Initialise main screen icons and tooltips
void ButtonMenuSet()
{
 int i, reso;
 if (altGS.HEIGHT==480) reso=2;				// stretch icons vertically in hi-res mode
 else reso=1;
 for (i = 0; i < 5; i++)					// 5 icons should be enough for the moment
 {
	button[i][0] = 16; button[i][1] = (8+(i*40))*reso;	// X1, Y1
	button[i][2] = 80; button[i][3] = (40+(i*40))*reso; // X2, Y2
	button[i][4] = 0;									// animation counter
 }
 button[5][0] = -1;							// this always used to mark end
 strcpy (buttontip[0], "Application List");
 strcpy (buttontip[1], "File Explorer");
 strcpy (buttontip[2], "Add / Remove Program");
 strcpy (buttontip[3], "Altimit Configuration");
 strcpy (buttontip[4], "Altimit Information");	// info screen
}

////////////////////////////////////////////////////////////////////////
// main program loop
void ButtonMenu()
{
 int pad, activebutton;

 pad = 0;
 activebutton = -1;
 ButtonMenuSet();
 while(loadedPAD)
 {
	snprintf(copytostring, 79, "Welcome to Altimit (c)2004 t0mb0la");
	pad = readpadbutton();
	drawMainWindow();
	if (usepointer && pointerX < 96)
	{
		activebutton = checkbuttons(button, MAX_BUTTONS);
	}
	else {
		if (pad & PAD_DOWN) { activebutton++; if (activebutton>4) activebutton=0; }
		if (pad & PAD_UP) { activebutton--; if (activebutton<0) activebutton=4; }
		}
	if(activebutton >= 0)
	{
		if(button[activebutton][4] == 0) button[activebutton][4] = 30;
		altGsDriver.drawPipe.RectLine(button[activebutton][0],
			button[activebutton][1], button[activebutton][2],
			button[activebutton][3], 3, altGS.WINFORECOL);
		buttontips.Xpos = button[activebutton][0] + 96;
		buttontips.Ypos = button[activebutton][1];
		buttontips.Zpos = 3;
		buttontips.Foreground = altGS.WINFORECOL;
		buttontips.Background = altGS.WINBACKCOL;
		buttontips.Content = buttontip[activebutton];
		drawTooltipWindow(buttontips);
	}
	if ((pad & PAD_CROSS) && activebutton==0)
	{
	}
	if ((pad & PAD_CROSS) && activebutton==1)
	{
		browserModule();
	}
	if ((pad & PAD_CROSS) && activebutton==2)
	{
	}
	if ((pad & PAD_CROSS) && activebutton==3)
	{
		configModule();
	}
	if ((pad & PAD_CROSS) && activebutton==4)
	{
		infoModule();
	}
	drawPointer();
	altGsDriver.drawPipe.Flush();
	altGsDriver.WaitForVSync();
	altGsDriver.swapBuffers();
 }
}

////////////////////////////////////////////////////////////////////////
// draws the main screen
void drawMainWindow()
{
	int i;

	altGsDriver.drawPipe.setZTestEnable(GS_DISABLE);	// disable Z-depth
	altGsDriver.drawPipe.RectGouraud(0, 0, altGS.SCREENCOL1,
		altGS.WIDTH,altGS.HEIGHT, altGS.SCREENCOL2, 0);	// to clear screen
//	altGsDriver.drawPipe.TextureSet(altGsDriver.getTextureBufferBase(),
//		256, GS_TEX_SIZE_256, GS_TEX_SIZE_512, GS_PSMCT32, 0, 0, 0, 0);
	for (i=0;i<=4;i++)							// draw icons with texture at animation point
	{
//		altGsDriver.drawPipe.RectTexture(button[i][0], button[i][1], 
//			((button[i][4]/10)*64), 256+(i*32), button[i][2], button[i][3],
//			(((button[i][4]/10)+1)*64), 256+((i+1)*32), 2,
//			GS_SET_RGBA(0x80,0x80,0x80,0x60));

		altGsDriver.drawPipe.RectLine(button[i][0], button[i][1],
			button[i][2], button[i][3], 2, GS_SET_RGBA(0x00,0x00,0x00,0x80)); // icon border
		if(button[i][4] > 0) button[i][4]--;	// animation runs in reverse until stop
	}
	altFont.Print(0, altGS.WIDTH, button[4][3] + 1, 2,
		GS_SET_RGBA(0x00,0x00,0x00,0x80), GSFONT_ALIGN_LEFT, copytostring);
	altFont.Print(0, altGS.WIDTH, button[4][3] + (FONT_HEIGHT2 - 2), 2,
		GS_SET_RGBA(0x00,0x00,0x00,0x80), GSFONT_ALIGN_LEFT, printstring);
	altGsDriver.drawPipe.setZTestEnable(GS_ENABLE);
}

////////////////////////////////////////////////////////////////////////
// returns number of text lines at textptr. just counts \n's really
// until it finds a \0
int countlines(char *textptr)
{
 int totallines = 0;
 char parse;

 while((parse = *textptr++) != '\0')
 {
	if (parse == '\n') totallines++;
 }
 return totallines;
}

////////////////////////////////////////////////////////////////////////
// returns number of files found in the array of files. just counts
// files until it finds a \0
int countfiles(altDentry *filelist)
{
 int totallines = 0;

 while(strcmp(filelist[totallines].filename, "\0")) totallines++;
 return totallines;
}

////////////////////////////////////////////////////////////////////////
// very simple elf header analysis, returns 1 if we think it is
// a valid executable, -1 if we don't
int checkElfHeader(altimitFS *source, char *filename)
{
 unsigned char *boot_elf;
 elf_header_t *eh;

 int fd;
 char pathname[MAX_PATHNAME];

 snprintf(pathname, MAX_PATHNAME-1, "%s%s", source->currentdir, filename);
 dbgprintf("Checking %s for ELF\n", pathname);
 if ((fd = source->activedevice->open(pathname, O_RDONLY)) < 0) return -1;
 boot_elf = (unsigned char *)malloc(sizeof(elf_header_t));
 eh = (elf_header_t *)boot_elf;
 if ((source->activedevice->read(fd, boot_elf, sizeof(elf_header_t)) < 0)
	|| ((_lw((u32)&eh->ident) != ELF_MAGIC) || eh->type != 2))
 {
	source->activedevice->close(fd);
	free(boot_elf);
	dbgprintf("Probably not an ELF\n");
	return -1;
 }
 source->activedevice->close(fd);
 free(boot_elf);
 dbgprintf("Seems to be an ELF\n");
 return 1;
}

////////////////////////////////////////////////////////////////////////
// copies file, size and filename should be supplied,
// from source->currentdir to destination->currentdir (assumes both
// these devices are active. returns 0 on success or -1 * error
int copyfile(altimitFS *source, altimitFS *destination, char *filename, int size)
{
 PercentWindow copystat;
 int fd, dfd, rv;
 unsigned int bytescopied, bytesremain;
 unsigned char *buffer;
 char sourcepath[MAX_PATHNAME], destpath[MAX_PATHNAME];

 snprintf(sourcepath, MAX_PATHNAME-1, "%s%s", source->currentdir, filename);
 snprintf(destpath, MAX_PATHNAME-1, "%s%s", destination->currentdir, filename);
 dbgprintf("Copy [%d] %s:%s\n", size, source->currentdev, sourcepath);
 dbgprintf("To %s:%s\n", destination->currentdev, destpath);
 buffer = (unsigned char *)memalign(64,32768);
 if (!buffer) return -1;
 if (size < 32768)
 {
	if ((fd = source->activedevice->open(sourcepath, O_RDONLY)) < 0) {free (buffer); return -1;}
	rv = source->activedevice->read(fd, buffer, size);
	source->activedevice->close(fd);
	if (rv < 0) { free(buffer); return -2; }
	if ((fd = destination->activedevice->open(destpath, O_WRONLY | O_TRUNC | O_CREAT)) < 0)
	{
		free(buffer);
		return -3;
	}
	rv = destination->activedevice->write(fd, buffer, size);
	destination->activedevice->close(fd);
	free(buffer);
	if (rv < 0) return -4;
 }
 else
 {
	copystat.Xpos = (altGS.WIDTH - 320)/2;
	copystat.Ypos = (altGS.HEIGHT - 32)/2;
	copystat.Zpos = 3;
	copystat.Xsize = 320;
	copystat.Foreground = altGS.WINFORECOL;
	copystat.Background = altGS.WINBACKCOL;
	copystat.Forehead = altGS.WINFOREHEAD;
	copystat.Backhead = altGS.WINBACKHEAD;
	copystat.Title = sourcepath;
	copystat.total = size;
	copystat.done = 0;
	if ((fd = source->activedevice->open(sourcepath, O_RDONLY)) < 0) {free (buffer); return -1;}
	if ((dfd = destination->activedevice->open(destpath, O_WRONLY | O_TRUNC | O_CREAT)) < 0)
	{
		source->activedevice->close(fd);
		free (buffer);
		return -3;
	}
	bytesremain = size;
	bytescopied = 0;
	while (bytesremain>=32768)
	{
		rv = source->activedevice->read(fd, buffer, 32768);
		if (rv < 0)
		{
			source->activedevice->close(fd);
			destination->activedevice->close(dfd);
			free(buffer);
			return -2;
		}
		rv = destination->activedevice->write(dfd, buffer, 32768);
		if (rv < 0)
		{
			source->activedevice->close(fd);
			destination->activedevice->close(dfd);
			free(buffer);
			return -4;
		}
		bytescopied+=32768;
		bytesremain-=32768;
		drawMainWindow();
		copystat.done = bytescopied;
		drawPercentWindow(copystat, "Copying");
		drawPointer();
		altGsDriver.drawPipe.Flush();
		altGsDriver.WaitForVSync();
		altGsDriver.swapBuffers();
	}
	if (bytesremain>0)
	{
		rv = source->activedevice->read(fd, buffer, bytesremain);
		if (rv < 0)
		{
			source->activedevice->close(fd);
			destination->activedevice->close(dfd);
			free(buffer);
			return -2;
		}
		rv = destination->activedevice->write(dfd, buffer, bytesremain);
		if (rv < 0)
		{
			source->activedevice->close(fd);
			destination->activedevice->close(dfd);
			free(buffer);
			return -4;
		}
		bytescopied+=bytesremain;
		bytesremain = 0;
		drawMainWindow();
		copystat.done = bytescopied;
		drawPercentWindow(copystat, "Copying");
		drawPointer();
		altGsDriver.drawPipe.Flush();
		altGsDriver.WaitForVSync();
		altGsDriver.swapBuffers();
	}
	source->activedevice->close(fd);
	destination->activedevice->close(dfd);
	free(buffer);
 }		
 return 0;
}

////////////////////////////////////////////////////////////////////////
// copies folder and contents recursively, foldername should be supplied,
// from source->currentdir to destination->currentdir (assumes both
// these devices are active. returns 0 on success or -1 on error
int recursivecopy(altimitFS *source, altimitFS *destination, char *folder)
{
 int entries, rv;
 char *sourcepath, *destpath;
 altDentry *filelist;

 rv = 0;
 filelist = (altDentry *)malloc(sizeof(altDentry)*MAX_ENTRIES);
 sourcepath = (char *)malloc(MAX_PATHNAME);
 destpath = (char *)malloc(MAX_PATHNAME);
 strcpy(sourcepath, source->currentdir);
 strcpy(destpath, destination->currentdir);
 strncat(source->currentdir, folder, MAX_PATHNAME-1);
 dbgprintf("Recursive copy of %s\n", folder);
 if ((entries = source->activedevice->getdir(source->currentdir, filelist)) <= 0)
 {
	dbgprintf("Error reading source folder\n");
	rv = -1;
	goto rcend;
 }
 strncat(source->currentdir, "/", MAX_PATHNAME-1);
 strncat(destination->currentdir, folder, MAX_PATHNAME-1);
 dbgprintf("Found %d entries\n", entries);
 if (entries == 1)
 {
	dbgprintf("Empty folder, just mkdir\n");
	destination->activedevice->mkdir(destination->currentdir);
	goto rcend;
 }
 else
 {
	destination->activedevice->mkdir(destination->currentdir);
	strncat(destination->currentdir, "/", MAX_PATHNAME-1);
	entries--;
	while (entries)
	{
		if (filelist[entries].mode == FIO_S_IFDIR)
		{
			if (recursivecopy(source, destination,
				filelist[entries].filename) < 0)
			{
				dbgprintf("Error copying sub-folder\n");
				rv = -1;
				goto rcend;
			}
		}
		else
		{
			if (copyfile(source, destination,
				filelist[entries].filename, filelist[entries].size) < 0)
			{
				dbgprintf("Error copying file\n");
				rv = -1;
				goto rcend;
			}
		}
		entries--;
	}
	dbgprintf("All done?\n");
 }
rcend:
 strcpy(source->currentdir, sourcepath);
 strcpy(destination->currentdir, destpath);
 free(destpath);
 free(sourcepath);
 free(filelist);
 return rv;
}

////////////////////////////////////////////////////////////////////////
// recursively deletes folder and contents, 
// from source->currentdir (assumes device is active.
// returns 0 on success or -1 on error
int recursivedelete(altimitFS *source, char *folder)
{
 int entries,rv;
 char *sourcepath;
 altDentry *filelist;

 rv = 0;
 filelist = (altDentry *)malloc(sizeof(altDentry)*MAX_ENTRIES);
 sourcepath = (char *)malloc(MAX_PATHNAME);
 snprintf(sourcepath, MAX_PATHNAME-1, "%s", folder);
 dbgprintf("Recursive delete of %s\n", sourcepath);
 if ((entries = source->activedevice->getdir(sourcepath, filelist)) <= 0)
 {
	dbgprintf("Error reading folder\n");
	rv = -1;
	goto rdend;
 }
 dbgprintf("Found %d entries\n", entries);
 if (entries == 1)
 {
	dbgprintf("Empty folder, just rmdir\n");
 	if ((source->activedevice->rmdir(sourcepath)) < 0)
	{
		dbgprintf("Error in rmdir\n");
		rv = -1;
		goto rdend;
	}
	goto rdend;
 }
 else
 {
	entries--;
	while (entries)
	{
		snprintf(sourcepath, MAX_PATHNAME-1, "%s/%s", folder, filelist[entries].filename);
		if (filelist[entries].mode == FIO_S_IFDIR)
		{
			if ((recursivedelete(source, sourcepath)) < 0)
			{
				rv = -1;
				goto rdend;
			}
		}
		else
		{
			dbgprintf("Delete file %s\n", sourcepath);
			if ((source->activedevice->remove(sourcepath)) < 0)
			{
				dbgprintf("Error in remove\n");
				rv = -1;
				goto rdend;
			}
		}
		entries--;
	}
	snprintf(sourcepath, MAX_PATHNAME-1, "%s", folder);
 	if ((source->activedevice->rmdir(sourcepath)) < 0)
	{
		dbgprintf("Error in rmdir\n");
		rv = -1;
		goto rdend;
	}
	dbgprintf("All done?\n");
 }
rdend:
 free(sourcepath);
 free(filelist);
 return rv;
}

////////////////////////////////////////////////////////////////////////
// file browser module, starts with a list of all available devices
// two abstract FS's are mounted, so pad SELECT button allows switching
// between these, arbritrarily called source and destination.
// this is where copying, creating, renaming, deleting, etc... will
// eventually be possible. 
void browserModule()
{
 int closemodule = false, activebutton, done, refresh;
 int remain, pad, i, rv, parts;
 int maxlines, maxchars;
 int winbutton[MAX_BUTTONS][5];		// array of button data for window buttons
 char *functions =	"Add Folder\n"
					"Delete\n"
					"Rename\n"
					"Copy\n"
					"Execute\n"
					"Cancel\n\0";
 char edittext[64];
 char winbuttontip[MAX_BUTTONS][30];	// array of labels for window buttons
 char browsertitle[80];
 char *devfolder, *pathfile, *newfile;
 char device[4];
 char *devptr, *ptr;
 altDentry *filelist;

 dbgprintf("before abstractFS1\n");
 delete abstractFS1;
 abstractFS1 = new altimitFS(activeHDD, activeHTTP, activeHOST, activeMC, activeCDFS);
 dbgprintf("before abstractFS2\n");
 delete abstractFS2;
 abstractFS2 = new altimitFS(activeHDD, activeHTTP, activeHOST, activeMC, activeCDFS);
 filelist = (altDentry *)malloc(sizeof(altDentry)*MAX_ENTRIES);
 devfolder = (char *)malloc(MAX_PATHNAME);
 pathfile = (char *)malloc(MAX_PATHNAME);
 newfile = (char *)malloc(MAX_PATHNAME);
 filefunctions.Xpos = 500;
 filefunctions.Ypos = 40;
 filefunctions.Zpos = 5;
 filefunctions.Foreground = altGS.WINFORECOL;
 filefunctions.Background = GS_SET_RGBA(0x00,0x00,0x00,0xFF);
 filefunctions.Content = functions;
 filefunctions.Highlighted = 0;
 winfilelist.Xpos = 96;
 winfilelist.Ypos = 8;
 winfilelist.Zpos = 3;
 winfilelist.Xsize = 528;
 if (altGS.HEIGHT == 224 || altGS.HEIGHT == 256) winfilelist.Ysize = 192;
 else { winfilelist.Ysize = 384; winfilelist.Ypos = 16; }
 winfilelist.Foreground = altGS.WINFORECOL;
 winfilelist.Background = altGS.WINBACKCOL;
 winfilelist.Forehead = altGS.WINFOREHEAD;
 winfilelist.Backhead = altGS.WINBACKHEAD;
 winfilelist.Title = browsertitle;
 winfilelist.Content = abstractFS1->dircontents;
 winfilelist.Top = 0;
 winfilelist.Highlighted = 0;
 winbutton[0][0] = (winfilelist.Xpos + winfilelist.Xsize) - FONT_WIDTH*2;
 winbutton[0][1] = winfilelist.Ypos;
 winbutton[0][2] = winbutton[0][0] + FONT_WIDTH*2;
 winbutton[0][3] = winbutton[0][1] + FONT_HEIGHT;
 strcpy (winbuttontip[0], "X");
 winbutton[1][0] = (winfilelist.Xpos + winfilelist.Xsize) - FONT_WIDTH*2;
 winbutton[1][1] = winfilelist.Ypos + FONT_HEIGHT;
 winbutton[1][2] = winbutton[1][0] + FONT_WIDTH*2;
 winbutton[1][3] = winbutton[1][1] + FONT_HEIGHT;
 strcpy (winbuttontip[1], "^");
 winbutton[2][0] = (winfilelist.Xpos + winfilelist.Xsize) - FONT_WIDTH*2;
 winbutton[2][1] = (winfilelist.Ypos + winfilelist.Ysize) - FONT_HEIGHT;
 winbutton[2][2] = winbutton[2][0] + FONT_WIDTH*2;
 winbutton[2][3] = winbutton[2][1] + FONT_HEIGHT;
 strcpy (winbuttontip[2], "v");
 winbutton[3][0] = winfilelist.Xpos;
 winbutton[3][1] = winfilelist.Ypos + FONT_HEIGHT;
 winbutton[3][2] = (winfilelist.Xpos + winfilelist.Xsize) - FONT_WIDTH*2;
 winbutton[3][3] = winfilelist.Ypos + winfilelist.Ysize;
 winbutton[4][0] = -1;
 maxlines = (winfilelist.Ysize - FONT_HEIGHT)/FONT_HEIGHT;
 maxchars = (winfilelist.Xsize - FONT_WIDTH)/FONT_WIDTH;
 activeFS=abstractFS1;
 strcpy(devfolder,activeFS->currentdir);
 strcpy(device,activeFS->currentdev);
 snprintf(copytostring, 79, "Destination not set, SELECT to set active folder as destination");
 activebutton=-1;
 if (activeFS->activedevice)
 {
	winfilelist.Content = filelist;
	rv = activeFS->activedevice->getdir(devfolder, filelist);
	if (rv < 0) snprintf(printstring, 79, "ERROR %i in getdir", rv);
	else snprintf(printstring, 79, "%u entries found", rv);
 }	
 while (!closemodule)
 {
	strcpy(activeFS->currentdir,devfolder);
	strcpy(activeFS->currentdev,device);
	if (activeFS->activedevice && (remain = activeFS->activedevice->freespace()))
	{
		if (remain >= 1024)
		{
			remain/=1024;
			snprintf(browsertitle, 79, "[%uMB free] %s%s", remain, device, devfolder);
		}
		else snprintf(browsertitle, 79, "[%uKB free] %s%s", remain, device, devfolder);
	}
	else snprintf(browsertitle, 79, "%s%s",device,devfolder);
	pad = readpadbutton();
	drawMainWindow();
	drawFilelistWindow(winfilelist);
	drawButtons(winbutton, winbuttontip, 2);
	if (usepointer) activebutton = checkbuttons(winbutton, MAX_BUTTONS);
	if (activebutton == 3)
	{
		i = (pointerY - (winfilelist.Ypos+FONT_HEIGHT)) / FONT_HEIGHT;
		if ((winfilelist.Top + i) < countfiles(winfilelist.Content))
			winfilelist.Highlighted = winfilelist.Top + i;
	}
	else if (activebutton >= 0) drawActiveButton(winbutton, winbuttontip, activebutton);
	if (pad & PAD_TRIANGLE || (pad & PAD_CROSS && activebutton==0)) closemodule=true;
	else if (heldbutton & PAD_UP || pad & PAD_UP || (pad & PAD_CROSS && activebutton==1))
	{
		if (winfilelist.Highlighted > 0) winfilelist.Highlighted--;
		if (winfilelist.Highlighted < winfilelist.Top) winfilelist.Top--;
	}
	else if (heldbutton & PAD_DOWN || pad & PAD_DOWN || (pad & PAD_CROSS && activebutton==2))
	{
		if (winfilelist.Highlighted < countfiles(winfilelist.Content)-1)
			winfilelist.Highlighted++;
		if (winfilelist.Highlighted > (winfilelist.Top+maxlines)-1)
			winfilelist.Top++;
	}
	else if (pad & PAD_LEFT || ((pad & PAD_CROSS || pad & PAD_RIGHT) && (activeFS->activedevice && !strcmp(filelist[winfilelist.Highlighted].filename,".."))))
	{
		if(!strcmp(devfolder, "/"))
		{
			activeFS->activedevice = NULL;
			winfilelist.Content = activeFS->dircontents;
			winfilelist.Highlighted = 0;
			winfilelist.Top = 0;
			CDVD_Stop();
			strcpy(device,"\0");
			strcpy(devfolder,"/");
		}
		else
		{
			devfolder[strlen(devfolder)-1] = '\0';
			devptr = strrchr(devfolder,'/');
			if (devptr==NULL) strcpy(devfolder, "/");
			else { devptr++; *devptr = '\0'; }
			rv = activeFS->activedevice->getdir(devfolder, filelist);
			if (rv < 0) snprintf(printstring, 79, "ERROR %i in getdir", rv);
			else snprintf(printstring, 79, "%u entries found", rv);
			winfilelist.Highlighted = 0;
			winfilelist.Top = 0;
		}
	}
	else if ((pad & PAD_RIGHT) || (pad & PAD_CROSS && (activeFS->activedevice==NULL || filelist[winfilelist.Highlighted].mode==FIO_S_IFDIR)))
	{
		if(activeFS->activedevice==NULL)
		{
			activeFS->activedevice=(altimitIO *)activeFS->dircontents[winfilelist.Highlighted].fsdevice;
			strcpy(device, activeFS->dircontents[winfilelist.Highlighted].filename);
			if(activeFS->activedevice)
			{
				winfilelist.Content=filelist;
				rv = activeFS->activedevice->getdir("/", filelist);
				if (rv < 0) 
				{
					snprintf(printstring, 79, "ERROR %i in getdir", rv);
					strcpy(devfolder,activeFS->currentdir);
					strcpy(device,activeFS->currentdev);
				}
				else snprintf(printstring, 79, "%u entries found", rv);
				winfilelist.Highlighted = 0;
				winfilelist.Top = 0;
			}
		}
		else
		{
			if(filelist[winfilelist.Highlighted].mode==FIO_S_IFDIR)
			{
				strncat(devfolder, filelist[winfilelist.Highlighted].filename, MAX_PATHNAME-1);
				strncat(devfolder, "/", MAX_PATHNAME-1);
				rv = activeFS->activedevice->getdir(devfolder, filelist);
				if (rv < 0)
				{
					snprintf(printstring, 79, "ERROR %i in getdir", rv);
					strcpy(devfolder,activeFS->currentdir);
				}
				else snprintf(printstring, 79, "%u entries found", rv);
				winfilelist.Highlighted = 0;
				winfilelist.Top = 0;
			}
		}
	}
	else if (pad & PAD_SELECT)
	{
		if(activeFS==abstractFS1) activeFS=abstractFS2;
		else activeFS=abstractFS1;
		snprintf(copytostring, 79, "Destination: %s%s", device, devfolder);
		strcpy(devfolder,activeFS->currentdir);
		strcpy(device,activeFS->currentdev);
		if(activeFS->activedevice)
		{
			winfilelist.Content=filelist;
			rv = activeFS->activedevice->getdir(devfolder, filelist);
			if (rv < 0) snprintf(printstring, 79, "ERROR %i in getdir", rv);
			else snprintf(printstring, 79, "%u entries found", rv);
		}
		else winfilelist.Content = activeFS->dircontents;
		winfilelist.Highlighted = 0;
		winfilelist.Top = 0;
	}
	else if (pad & PAD_SQUARE && activeFS->activedevice)
	{
		drawPointer();
		altGsDriver.drawPipe.Flush();
		altGsDriver.WaitForVSync();
		altGsDriver.swapBuffers();
		done = false;
		refresh = false;
		while(!done)
		{
			pad = readpadbutton();
			drawMainWindow();
			drawFilelistWindow(winfilelist);
			drawButtons(winbutton, winbuttontip, 2);
			drawFunctionWindow(filefunctions);
			if (usepointer && pointerX > filefunctions.Xpos
					&& pointerX < (filefunctions.Xpos + 96)
					&& pointerY > filefunctions.Ypos
					&& pointerY < filefunctions.Ypos + (countlines(functions) * FONT_HEIGHT))
				filefunctions.Highlighted = (pointerY - filefunctions.Ypos) / FONT_HEIGHT;
			if (pad & PAD_TRIANGLE) done = true;
			else if (pad & PAD_UP && filefunctions.Highlighted > 0)
				filefunctions.Highlighted--;
			else if (pad & PAD_DOWN && filefunctions.Highlighted < countlines(functions)-1)
				filefunctions.Highlighted++;
			else if (pad & PAD_CROSS)
			{
				if (filefunctions.Highlighted == 0)
				{
					strcpy(edittext, "");
					if (oskModule(edittext, "Enter name for new folder") < 0) { }
					else
					{
						if (strlen(edittext) > 0)
						{
							strcpy(pathfile, devfolder);
							strncat(pathfile, edittext, MAX_PATHNAME-1);
							if (activeFS->activedevice->mkdir(pathfile) >= 0)
							{
								snprintf(printstring, 79, "%s%s created", device, pathfile);
								refresh = true;
							}
							else snprintf(printstring, 79, "ERROR creating %s%s", device, pathfile);
						}
						done = true;
					}
				}
				else if (filefunctions.Highlighted == 1
					&& strcmp(filelist[winfilelist.Highlighted].filename,".."))
				{
					snprintf(printstring, 79, "Press circle to confirm delete, triangle to cancel");
					drawMainWindow();
					drawFilelistWindow(winfilelist);
					drawButtons(winbutton, winbuttontip, 2);
					drawFunctionWindow(filefunctions);
					drawPointer();
					altGsDriver.drawPipe.Flush();
					altGsDriver.WaitForVSync();
					altGsDriver.swapBuffers();
					while (!(pad & PAD_TRIANGLE || pad & PAD_CIRCLE)) pad = readpadbutton();
					if (pad & PAD_CIRCLE)
					{
						strcpy(pathfile, devfolder);
						strncat(pathfile, filelist[winfilelist.Highlighted].filename, MAX_PATHNAME-1);
						if (filelist[winfilelist.Highlighted].mode==FIO_S_IFDIR)
						{
							if (recursivedelete(activeFS, pathfile) == 0)
							{
								snprintf(printstring, 79, "%s%s deleted", device, pathfile);
								refresh = true;
							}
							else snprintf(printstring, 79, "ERROR deleting %s%s", device, pathfile);
						}
						else
						{
							if (activeFS->activedevice->remove(pathfile) >= 0)
							{
								snprintf(printstring, 79, "%s%s deleted", device, pathfile);
								refresh = true;
							}
							else snprintf(printstring, 79, "ERROR deleting %s%s", device, pathfile);
						}
					}
					done = true;
				}
				else if (filefunctions.Highlighted == 2
					&& strcmp(filelist[winfilelist.Highlighted].filename,".."))

				{
					strncpy(edittext, filelist[winfilelist.Highlighted].filename, 63);
					if (oskModule(edittext, "Enter new name") < 0) { }
					else
					{ 
						if (strlen(edittext) > 0)
						{
							strcpy(pathfile, devfolder);
							strncat(pathfile, filelist[winfilelist.Highlighted].filename, MAX_PATHNAME-1);
							strcpy(newfile, devfolder);
							strncat(newfile, edittext, MAX_PATHNAME-1);
							if (activeFS->activedevice->rename(pathfile, newfile) >= 0)
							{
								snprintf(printstring, 79, "%s%s is now %s%s", device, pathfile, device, newfile);
								refresh = true;
							}
							else snprintf(printstring, 79, "ERROR renaming %s%s", device, pathfile);
						}
						done = true;
					}
				}
				else if (filefunctions.Highlighted == 3)
				{
					if (activeFS == abstractFS1 && abstractFS2->activedevice)
					{
						if (filelist[winfilelist.Highlighted].mode == FIO_S_IFDIR)
						{
							rv = recursivecopy(activeFS, abstractFS2,
								filelist[winfilelist.Highlighted].filename);
							if (rv < 0) snprintf(printstring, 79, "ERROR %i in recursive copy", rv);
						}
						else
						{
							rv = copyfile(activeFS, abstractFS2,
								filelist[winfilelist.Highlighted].filename,
								filelist[winfilelist.Highlighted].size);
							if (rv < 0) snprintf(printstring, 79, "ERROR %i in copy", rv);
							else snprintf(printstring, 79, "Copied %s", filelist[winfilelist.Highlighted].filename);
						}
					}
					else if (activeFS == abstractFS2 && abstractFS1->activedevice)
					{
						if (filelist[winfilelist.Highlighted].mode == FIO_S_IFDIR)
						{
							rv = recursivecopy(activeFS, abstractFS1,
								filelist[winfilelist.Highlighted].filename);
							if (rv < 0) snprintf(printstring, 79, "ERROR %i in recursive copy", rv);
						}
						else
						{
							rv = copyfile(activeFS, abstractFS1,
								filelist[winfilelist.Highlighted].filename,
								filelist[winfilelist.Highlighted].size);
							if (rv < 0) snprintf(printstring, 79, "ERROR %i in copy", rv);
							else snprintf(printstring, 79, "Copied %s", filelist[winfilelist.Highlighted].filename);
						}
					}
					done = true;
				}
				else if (filefunctions.Highlighted == 4)
				{
					if (activeFS->activedevice && !(filelist[winfilelist.Highlighted].mode == FIO_S_IFDIR))
					{
						strcpy(elfrunpath, devfolder);
						strncat(elfrunpath, filelist[winfilelist.Highlighted].filename, MAX_PATHNAME-1);
						if (activeFS->activedevice->getpath(elfrunpath, elfrunpath) < 0) strcpy(elfrunpath, "unknown");
						if (checkElfHeader(activeFS, filelist[winfilelist.Highlighted].filename) >=0)
						{
							snprintf(printstring, 79, "%s Valid ELF file", elfrunpath);
							if(!strncmp(elfrunpath,"pfs",3))
							{
								parts = 0;
								ptr = (char *)devfolder; ptr++;
								while (*ptr != '/' && *ptr != '\0') filesystem[parts++]=*ptr++;
								if (*ptr == '\0') strcpy(filesystem, "\0");
								else filesystem[parts]='\0';
							}
							else strcpy(filesystem, "\0");
							RunLoaderElf(elfrunpath, filesystem);
						}
						else
							snprintf(printstring, 79, "%s is not executable", elfrunpath);
						done = true;
					}
				}
				else if (filefunctions.Highlighted == 5) done = true;
			}
			if (refresh)
			{
				winfilelist.Highlighted = 0;
				winfilelist.Top = 0;
				rv = activeFS->activedevice->getdir(devfolder, filelist);
				if (rv < 0) snprintf(printstring, 79, "ERROR %i in getdir", rv);
				else snprintf(printstring, 79, "%u entries found", rv);
				refresh = false;
			}
			drawPointer();
			altGsDriver.drawPipe.Flush();
			altGsDriver.WaitForVSync();
			altGsDriver.swapBuffers();
		}
	}	
	drawPointer();
	altGsDriver.drawPipe.Flush();
	altGsDriver.WaitForVSync();
	altGsDriver.swapBuffers();
 }
 free(newfile);
 free(pathfile);
 free(devfolder);
 free(filelist);
 strcpy(printstring,"\0");
}

////////////////////////////////////////////////////////////////////////
// Altimit configuration module still needs expansion but so far
// allows resolution changes, screen centering, FS device options and
// on screen pointer option
void configModule()
{
 int fd,len;
 int closemodule = false, activebutton, activeoptions;
 int pad, i, j, resetmode, moved, modded;//, magv, magh;
 int winbutton[MAX_BUTTONS][5];		// array of button data for window buttons
 int winbutton2[MAX_BUTTONS][5];
 char winbuttontip[MAX_BUTTONS][30];	// array of labels for window buttons
 char winbuttontip2[MAX_BUTTONS][30];
 char *cfgtip = "Use D-PAD to center, Triangle when done\0";
 char *cfgtitle = "Altimit Configuration\0";
 char *cfgtext = "Configuration is saved to mc0:/SYS-CONF/ALTIMIT.CFG\n"
 "Use the following buttons to customise your settings.\n\0";
 char *cfgbuffer;
 char buf[IPCONF_MAX_LEN];
 cfgscreen.Xpos = 96;
 cfgscreen.Ypos = 8;
 cfgscreen.Zpos = 3;
 cfgscreen.Xsize = 528;
 if (altGS.HEIGHT == 224 || altGS.HEIGHT == 256) cfgscreen.Ysize = 192;
 else { cfgscreen.Ysize = 384; cfgscreen.Ypos = 16; }
 cfgscreen.Foreground = altGS.WINFORECOL;
 cfgscreen.Background = altGS.WINBACKCOL;
 cfgscreen.Forehead = altGS.WINFOREHEAD;
 cfgscreen.Backhead = altGS.WINBACKHEAD;
 cfgscreen.Title = cfgtitle;
 cfgscreen.Content = cfgtext;
 cfgscreen.Top = 1;
 buttontips.Xpos = cfgscreen.Xpos + (cfgscreen.Xsize / 2) - ((strlen(cfgtip) * FONT_WIDTH) / 2);
 buttontips.Ypos = cfgscreen.Ypos + (cfgscreen.Ysize / 2);
 buttontips.Zpos = 5;
 buttontips.Foreground = altGS.WINFORECOL;
 buttontips.Background = altGS.WINBACKCOL;
 buttontips.Content = cfgtip;
 winbutton[0][0] = (cfgscreen.Xpos + cfgscreen.Xsize) - FONT_WIDTH*2;
 winbutton[0][1] = cfgscreen.Ypos;
 winbutton[0][2] = winbutton[0][0] + FONT_WIDTH*2;
 winbutton[0][3] = winbutton[0][1] + FONT_HEIGHT;
 strcpy (winbuttontip[0], "X");
 winbutton[1][0] = (cfgscreen.Xpos + cfgscreen.Xsize) - FONT_WIDTH*2;
 winbutton[1][1] = cfgscreen.Ypos + FONT_HEIGHT;
 winbutton[1][2] = winbutton[1][0] + FONT_WIDTH*2;
 winbutton[1][3] = winbutton[1][1] + FONT_HEIGHT;
 strcpy (winbuttontip[1], "^");
 winbutton[2][0] = (cfgscreen.Xpos + cfgscreen.Xsize) - FONT_WIDTH*2;
 winbutton[2][1] = (cfgscreen.Ypos + cfgscreen.Ysize) - FONT_HEIGHT;
 winbutton[2][2] = winbutton[2][0] + FONT_WIDTH*2;
 winbutton[2][3] = winbutton[2][1] + FONT_HEIGHT;
 strcpy (winbuttontip[2], "v");
 for (i=0;i<3;i++)
 {
	for (j=0;j<4;j++)
	{
		winbutton2[i][j] = winbutton[i][j];
	}
	strcpy(winbuttontip2[i], winbuttontip[i]);
 }
 for (i=3;i<=6;i++)
 {
	winbutton[i][0] = cfgscreen.Xpos + (FONT_WIDTH*2);
	winbutton[i][1] = cfgscreen.Ypos + (((i-1)*FONT_HEIGHT)*2);
	winbutton[i][2] = winbutton[i][0] + (FONT_WIDTH*20);
	winbutton[i][3] = winbutton[i][1] + FONT_HEIGHT;
	winbutton[i+4][0] = winbutton[i][2] + (FONT_WIDTH*24);
	winbutton[i+4][1] = winbutton[i][1];
	winbutton[i+4][2] = winbutton[i+4][0] + (FONT_WIDTH*10);
	winbutton[i+4][3] = winbutton[i][3];
 }
 for (i=3;i<=6;i++)
 {
	for (j=0;j<4;j++)
	{
		winbutton2[i][j] = winbutton[i][j];
	}
 }
 strcpy (winbuttontip[3], "Screen Size");
 strcpy (winbuttontip2[3], "Load Mouse");
 strcpy (winbuttontip[4], "Video Format");
 strcpy (winbuttontip2[4], "Load Keyboard");
 strcpy (winbuttontip[5], "Screen Offset");
 strcpy (winbuttontip2[5], "IPCONFIG.DAT");
 strcpy (winbuttontip[6], "More Options");
 strcpy (winbuttontip2[6], "Use Pointer");
 strcpy (winbuttontip[7], "PS2HDD");
 strcpy (winbuttontip[8], "PS2HOST");
 strcpy (winbuttontip[9], "PS2NETFS");
 strcpy (winbuttontip[10], "PS2FTPD");
 winbutton[11][0] = -1;
 winbutton2[7][0] = -1;
 resetmode = false;
 moved = false;
 modded = false;
 activebutton = 3;
 activeoptions = 0;
 while (!closemodule)
 {
	pad = readpadbutton();
	drawMainWindow();
	drawTextWindow(cfgscreen);
 if (activeoptions == 0)
 {
	drawButtons(winbutton, winbuttontip, 0);
	sprintf(buf, "%ix%i", altGS.WIDTH, altGS.HEIGHT);
	altFont.Print(winbutton[3][2]+(FONT_WIDTH*4), winbutton[3][2]+(FONT_WIDTH*24),
		winbutton[3][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, buf);
	if (altGS.PALORNTSC == GS_TV_PAL) altFont.Print(winbutton[4][2]+(FONT_WIDTH*4), winbutton[4][2]+(FONT_WIDTH*24),
		winbutton[4][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "PAL");
	else altFont.Print(winbutton[4][2]+(FONT_WIDTH*4), winbutton[4][2]+(FONT_WIDTH*24),
		winbutton[4][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "NTSC");
	if (altGS.LOADHDDS) altFont.Print(winbutton[7][2]+(FONT_WIDTH*2), winbutton[7][2]+(FONT_WIDTH*8),
		winbutton[7][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton[7][2]+(FONT_WIDTH*2), winbutton[7][2]+(FONT_WIDTH*8),
		winbutton[7][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if (altGS.LOADHOST) altFont.Print(winbutton[8][2]+(FONT_WIDTH*2), winbutton[8][2]+(FONT_WIDTH*8),
		winbutton[8][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton[8][2]+(FONT_WIDTH*2), winbutton[8][2]+(FONT_WIDTH*8),
		winbutton[8][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if (altGS.LOADNETFS) altFont.Print(winbutton[9][2]+(FONT_WIDTH*2), winbutton[9][2]+(FONT_WIDTH*8),
		winbutton[9][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton[9][2]+(FONT_WIDTH*2), winbutton[9][2]+(FONT_WIDTH*8),
		winbutton[9][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if (altGS.LOADFTPD) altFont.Print(winbutton[10][2]+(FONT_WIDTH*2), winbutton[10][2]+(FONT_WIDTH*8),
		winbutton[10][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton[10][2]+(FONT_WIDTH*2), winbutton[10][2]+(FONT_WIDTH*8),
		winbutton[10][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if ((usepointer) &&
		pointerX > cfgscreen.Xpos && 
		pointerX < (cfgscreen.Xpos + cfgscreen.Xsize) &&
		pointerY > cfgscreen.Ypos &&
		pointerY < (cfgscreen.Ypos + cfgscreen.Ysize)) 
	{
		i = checkbuttons(winbutton, MAX_BUTTONS);
		if (i >=0 ) activebutton = i;
	}
	else if (activebutton < 3) activebutton = 3;
	if (activebutton >= 0) // if a button is active, illuminate it
	{
		drawActiveButton(winbutton, winbuttontip, activebutton);
	}
	if (pad & PAD_TRIANGLE || (pad & PAD_CROSS && activebutton==0)) closemodule=true;
	else if (pad & PAD_UP)
	{
		if (activebutton>3) activebutton--;
		else activebutton=10;
	}
	else if (pad & PAD_DOWN)
	{
		if (activebutton<10) activebutton++;
		else activebutton=3;
	}
	else if (pad & PAD_LEFT)
	{
		if (activebutton>6) activebutton-=4;
		else activebutton+=4;
	}
	else if (pad & PAD_RIGHT)
	{
		if (activebutton<7) activebutton+=4;
		else activebutton-=4;
	}
	if (pad & PAD_CROSS)
	{
		if (activebutton == 3)
		{
			if (altGS.PALORNTSC == GS_TV_NTSC)
			{
				if (altGS.HEIGHT == 224) { altGS.HEIGHT = 480; altGS.INTERLACING = GS_TV_INTERLACE; }
				else { altGS.HEIGHT = 224; altGS.INTERLACING = GS_TV_NONINTERLACE; }
			}
			else
			{
				if (altGS.HEIGHT == 256) { altGS.HEIGHT = 480; altGS.INTERLACING = GS_TV_INTERLACE; }
				else { altGS.HEIGHT = 256; altGS.INTERLACING = GS_TV_NONINTERLACE; }
			}
			resetmode = true;
		}
		else if (activebutton == 4)
		{
			if (altGS.PALORNTSC == GS_TV_PAL) 
			{
				altGS.PALORNTSC = GS_TV_NTSC;
				if (altGS.HEIGHT == 256) altGS.HEIGHT = 224;
				else altGS.HEIGHT = 480;
			}
			else
			{
				altGS.PALORNTSC = GS_TV_PAL;
				if (altGS.HEIGHT == 224) altGS.HEIGHT = 256;
				else altGS.HEIGHT = 480;
			}
			resetmode = true;
		}
		else if (activebutton == 5)
		{
			while (!(pad & PAD_TRIANGLE))
			{
				pad = readpadbutton();
				if (((pad & PAD_UP) || (heldbutton & PAD_UP)) && altGS.OFFSETY > 0) { altGS.OFFSETY--; moved = true; }
				if (((pad & PAD_DOWN) || (heldbutton & PAD_DOWN)) && altGS.OFFSETY < 255) { altGS.OFFSETY++; moved = true; }
				if (((pad & PAD_LEFT) || (heldbutton & PAD_LEFT)) && altGS.OFFSETX > 0) { altGS.OFFSETX--; moved = true; }
				if (((pad & PAD_RIGHT) || (heldbutton & PAD_RIGHT)) && altGS.OFFSETX < 255 ) { altGS.OFFSETX++; moved = true; }
				drawMainWindow();
				drawPointer();
				altGsDriver.drawPipe.RectLine(0, 0, altGS.WIDTH, altGS.HEIGHT, 0, altGS.WINFORECOL);
				drawTooltipWindow(buttontips);
				altGsDriver.drawPipe.Flush();
				altGsDriver.WaitForVSync();
				altGsDriver.swapBuffers();
				if (moved)
				{
					__asm__(" di ");

					GS_PMODE = 0xFF61; // Read Circuit Enabled

					GS_DISPLAY1 = GS_SET_DISPLAY(altGS.WIDTH, altGS.HEIGHT, altGS.OFFSETX, altGS.OFFSETY);

					__asm__(" ei ");
					
				}
			}
			closemodule = true;
		}
		else if (activebutton == 6) { activeoptions = 1; activebutton = 3; }
		else if (activebutton == 7)
		{
			if (altGS.LOADHDDS) altGS.LOADHDDS = false;
			else altGS.LOADHDDS = true;
			modded = true;
		}
		else if (activebutton == 8)
		{
			if (altGS.LOADHOST) altGS.LOADHOST = false;
			else { altGS.LOADHOST = true; altGS.LOADNETFS = false; altGS.LOADFTPD = false; }
			modded = true;
		}
		else if (activebutton == 9)
		{
			if (altGS.LOADNETFS) altGS.LOADNETFS = false;
			else { altGS.LOADNETFS = true; altGS.LOADHOST = false; altGS.LOADFTPD = false; }
			modded = true;
		}
		else if (activebutton == 10)
		{
			if (altGS.LOADFTPD) altGS.LOADFTPD = false;
			else { altGS.LOADFTPD = true; altGS.LOADHOST = false; altGS.LOADNETFS = false; }
			modded = true;
		}
	}
 }
 else
 {
	drawButtons(winbutton2, winbuttontip2, 0);
	if (altGS.LOADMOUSE) altFont.Print(winbutton2[3][2]+(FONT_WIDTH*4), winbutton2[3][2]+(FONT_WIDTH*24),
		winbutton2[3][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton2[3][2]+(FONT_WIDTH*4), winbutton2[3][2]+(FONT_WIDTH*24),
		winbutton2[3][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if (altGS.LOADKEYBD) altFont.Print(winbutton2[4][2]+(FONT_WIDTH*4), winbutton2[4][2]+(FONT_WIDTH*24),
		winbutton2[4][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "On");
	else altFont.Print(winbutton2[4][2]+(FONT_WIDTH*4), winbutton2[4][2]+(FONT_WIDTH*24),
		winbutton2[4][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Off");
	if (altGS.USEPOINTER) altFont.Print(winbutton2[6][2]+(FONT_WIDTH*4), winbutton2[6][2]+(FONT_WIDTH*24),
		winbutton2[6][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "Yes");
	else altFont.Print(winbutton2[6][2]+(FONT_WIDTH*4), winbutton2[6][2]+(FONT_WIDTH*24),
		winbutton2[6][1], 4, altGS.WINFORECOL, GSFONT_ALIGN_LEFT, "No");
	if ((usepointer) &&
		pointerX > cfgscreen.Xpos && 
		pointerX < (cfgscreen.Xpos + cfgscreen.Xsize) &&
		pointerY > cfgscreen.Ypos &&
		pointerY < (cfgscreen.Ypos + cfgscreen.Ysize))
	{
		i = checkbuttons(winbutton2, MAX_BUTTONS);
		if (i>=0) activebutton = i;
	}
	else if (activebutton < 3) activebutton = 3;
	if (activebutton >= 0) // if a button is active, illuminate it
	{
		drawActiveButton(winbutton2, winbuttontip2, activebutton);
	}
	if (pad & PAD_TRIANGLE || (pad & PAD_CROSS && activebutton==0)) closemodule=true;
	else if (pad & PAD_UP)
	{
		if (activebutton>3) activebutton--;
		else activebutton=6;
	}
	else if (pad & PAD_DOWN)
	{
		if (activebutton<6) activebutton++;
		else activebutton=3;
	}
	if (pad & PAD_CROSS)
	{
		if (activebutton == 3)
		{
			if (altGS.LOADMOUSE) altGS.LOADMOUSE = false;
			else altGS.LOADMOUSE = true;
			modded = true;
		}
		else if (activebutton == 4)
		{
			if (altGS.LOADKEYBD) altGS.LOADKEYBD = false;
			else altGS.LOADKEYBD = true;
			modded = true;
		}
		else if (activebutton == 5)
		{
			fd = fioOpen("mc0:/SYS-CONF/IPCONFIG.DAT", O_RDONLY);
			if (fd < 0) 
			{
				sprintf(buf, "%s %s %s", ip, netmask, gw);
			}
			else
			{
				memset(buf, 0x00, IPCONF_MAX_LEN);
				len = fioRead(fd, buf, IPCONF_MAX_LEN - 1); // Let the last byte be '\0'
				fioClose(fd);
			}
			if (oskModule(buf, "Edit IPCONFIG.DAT") >= 0)
			{
				fd = fioOpen("mc0:/SYS-CONF/IPCONFIG.DAT", O_CREAT | O_WRONLY | O_TRUNC);
				fioWrite(fd, buf, strlen(buf));
				fioClose(fd);
			}
		}
		else if (activebutton == 6)
		{
			if (altGS.USEPOINTER) altGS.USEPOINTER = false;
			else altGS.USEPOINTER = true;
			moved = true;
			usepointer = altGS.USEPOINTER;
		}
	}
 }
	drawPointer();
	altGsDriver.drawPipe.Flush();
	altGsDriver.WaitForVSync();
	altGsDriver.swapBuffers();
 }
 if (moved || resetmode || modded)		// save changes to config file. probably shouldn't
 {										// be hardcoded to mc0, and could do with error check
	cfgbuffer=(char *)&altGS.WIDTH;
	i = fioOpen("mc0:/SYS-CONF/ALTIMIT.CFG", O_CREAT | O_WRONLY | O_TRUNC);
	fioWrite(i, cfgbuffer, sizeof(altimitGS));
	fioClose(i);
 }
 if (resetmode)
 {
	initGS();
	ButtonMenuSet();
 }
}					// would be nice to handle changes to loaded modules here, instead of
					// forcing the user to reload for changes to take effect.
					// I played with reset IOP and calling loadIOPbuffers/loadIOPmodules,
					// didn't work too well. Would probably work if USBD.IRX was cached before
					// reset
					
////////////////////////////////////////////////////////////////////////
// on screen keyboard module (could be prettier)
// uses supplied text as default and returns edited text when done
// returns -1 if cancelled, and leaves supplied text unchanged
// returns 0 otherwise (it will trim lines if they are too long)
int oskModule(char *edittext, char *osktitle)
{
 static char *nocaps = "1" "\0" "2" "\0" "3" "\0" "4" "\0" "5" "\0" "6" "\0" "7" "\0" "8"
	"\0" "9" "\0" "0" "\0" "-" "\0" "=" "\0" "\\" "\0"
	"q" "\0" "w" "\0" "e" "\0" "r" "\0" "t" "\0" "y" "\0" "u" "\0" "i" "\0" "o" "\0" "p"
	"\0" "[" "\0" "]" "\0"
	"a" "\0" "s" "\0" "d" "\0" "f" "\0" "g" "\0" "h" "\0" "j" "\0" "k" "\0" "l" "\0" ";"
	"\0" "<" "\0" ">" "\0" "done" "\0"
	"z" "\0" "x" "\0" "c" "\0" "v" "\0" "b" "\0" "n" "\0" "m" "\0" "," "\0" "." "\0" "'"
	"\0" "/" "\0" "<<" "\0" ">>" "\0"
	"caps" "\0" "space" "\0" "ins" "\0" "clear" "\0";
 static char *oncaps = "!" "\0" "@" "\0" "#" "\0" "$" "\0" "%" "\0" "^" "\0" "&" "\0" "*"
	"\0" "(" "\0" ")" "\0" "_" "\0" "+" "\0" "|" "\0"
	"Q" "\0" "W" "\0" "E" "\0" "R" "\0" "T" "\0" "Y" "\0" "U" "\0" "I" "\0" "O" "\0" "P"
	"\0" "{" "\0" "}" "\0"
	"A" "\0" "S" "\0" "D" "\0" "F" "\0" "G" "\0" "H" "\0" "J" "\0" "K" "\0" "L" "\0" ":"
	"\0" "<" "\0" ">" "\0" "done" "\0"
	"Z" "\0" "X" "\0" "C" "\0" "V" "\0" "B" "\0" "N" "\0" "M" "\0" "," "\0" "." "\0" "~"
	"\0" "?" "\0" "<<" "\0" ">>" "\0"
	"caps" "\0" "space" "\0" "ins" "\0" "clear" "\0";
 char *keyptr;
// char *osktitle = "Altimit on screen keyboard\0";
 char *osktext = "Use PAD to highlight key, Cross to select, Triangle to quit\n"
 "L1 & R1 to move cursor, Circle to delete.\n\0";
 char copytext[64];
 char osktip[55][30];
 int oskkey[55][5];
 int winbutton[MAX_BUTTONS][5];		// array of button data for window buttons
 char winbuttontip[MAX_BUTTONS][30];	// array of labels for window buttons
 int capson = false, closemodule = false, activebutton;
 int pad, key, textX, textY;
 unsigned int maxchars = 62, cursorpos, blinker, i;

 memset(copytext, '\0', maxchars+1); // blank text area
 strncpy(copytext, edittext, maxchars); // copy any supplied text
 cursorpos = strlen(copytext); // cursor at end of text (if any)
 oskscreen.Xpos = 96;
 oskscreen.Ypos = 8;
 oskscreen.Zpos = 3;
 oskscreen.Xsize = 528;
 if (altGS.HEIGHT == 224 || altGS.HEIGHT == 256) oskscreen.Ysize = 192;
 else { oskscreen.Ysize = 384; oskscreen.Ypos = 16; }
 oskscreen.Foreground = altGS.WINFORECOL;
 oskscreen.Background = altGS.WINBACKCOL;
 oskscreen.Forehead = altGS.WINFOREHEAD;
 oskscreen.Backhead = altGS.WINBACKHEAD;
 oskscreen.Title = osktitle;
 oskscreen.Content = osktext;
 oskscreen.Top = 1;
 textX = oskscreen.Xpos + FONT_WIDTH;
 textY = oskscreen.Ypos + FONT_HEIGHT * 4;
 key = 0;
 for (i=18; i<486; i+=36) // row 1 of keyboard
 {
	oskkey[key][0] = oskscreen.Xpos + i;
	oskkey[key][1] = oskscreen.Ypos + FONT_HEIGHT * 5;
	oskkey[key][2] = oskkey[key][0] + 32;
	oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
	key++;
 }
 for (i=36; i<468; i+=36) // row 2 of keyboard
 {
	oskkey[key][0] = oskscreen.Xpos + i;
	oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 6) + 2;
	oskkey[key][2] = oskkey[key][0] + 32;
	oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
	key++;
 }
 for (i=0; i<432; i+=36) // row 3 of keyboard
 {
	oskkey[key][0] = oskscreen.Xpos + i;
	oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 7) + 4;
	oskkey[key][2] = oskkey[key][0] + 32;
	oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
	key++;
 }
 oskkey[key][0] = oskscreen.Xpos + 432; // done key
 oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 7) + 4;
 oskkey[key][2] = oskkey[key][0] + 64;
 oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
 key++;
 for (i=18; i<486; i+=36) // row 4 of keyboard
 {
	oskkey[key][0] = oskscreen.Xpos + i;
	oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 8) + 6;
	oskkey[key][2] = oskkey[key][0] + 32;
	oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
	key++;
 }
 oskkey[key][0] = oskscreen.Xpos; // caps key
 oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 9) + 8;
 oskkey[key][2] = oskkey[key][0] + 64;
 oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
 key++;
 oskkey[key][0] = oskscreen.Xpos + 68; // space bar
 oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 9) + 8;
 oskkey[key][2] = oskkey[key][0] + 252;
 oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
 key++;
 oskkey[key][0] = oskscreen.Xpos + 324; // ins key
 oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 9) + 8;
 oskkey[key][2] = oskkey[key][0] + 64;
 oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
 key++;
 oskkey[key][0] = oskscreen.Xpos + 392; // clear key
 oskkey[key][1] = oskscreen.Ypos + (FONT_HEIGHT * 9) + 8;
 oskkey[key][2] = oskkey[key][0] + 64;
 oskkey[key][3] = oskkey[key][1] + FONT_HEIGHT;
 key++;
 oskkey[key][0] = -1; // end identifier
 keyptr = nocaps;
 for (i=0;i<55;i++) // set up keyboard with characters
 {
	strcpy(osktip[i], keyptr);
	while (*keyptr++ != '\0') {}
 }
 winbutton[0][0] = (oskscreen.Xpos + oskscreen.Xsize) - FONT_WIDTH*2;
 winbutton[0][1] = oskscreen.Ypos;
 winbutton[0][2] = winbutton[0][0] + FONT_WIDTH*2;
 winbutton[0][3] = winbutton[0][1] + FONT_HEIGHT;
 strcpy (winbuttontip[0], "X"); // close window button
 winbutton[1][0] = (oskscreen.Xpos + oskscreen.Xsize) - FONT_WIDTH*2;
 winbutton[1][1] = oskscreen.Ypos + FONT_HEIGHT;
 winbutton[1][2] = winbutton[1][0] + FONT_WIDTH*2;
 winbutton[1][3] = winbutton[1][1] + FONT_HEIGHT;
 strcpy (winbuttontip[1], "^"); // scroll up button (not used here)
 winbutton[2][0] = (oskscreen.Xpos + oskscreen.Xsize) - FONT_WIDTH*2;
 winbutton[2][1] = (oskscreen.Ypos + oskscreen.Ysize) - FONT_HEIGHT;
 winbutton[2][2] = winbutton[2][0] + FONT_WIDTH*2;
 winbutton[2][3] = winbutton[2][1] + FONT_HEIGHT;
 strcpy (winbuttontip[2], "v"); // scroll down button (not used here)
 winbutton[3][0] = -1;
 activebutton=0;
 blinker=0;
 keypress='\0';
 while (!closemodule)
 {
	if (blinker==20) blinker=0; // reduce speed of cursor blinking
	blinker++;
	pad = readpadbutton();
	drawMainWindow();
	drawTextWindow(oskscreen);
	altGsDriver.drawPipe.RectFlat(textX - FONT_WIDTH, textY, textX + (oskscreen.Xsize - (FONT_WIDTH * 2)),
		textY + FONT_HEIGHT, 4, altGS.WINBACKHEAD); // make a backdrop for editing text
	altFont.Print(textX, textX + (oskscreen.Xsize - (FONT_WIDTH * 2)), textY, 5,
		altGS.WINFOREHEAD, GSFONT_ALIGN_LEFT, copytext); // print editing text
	if (insmode)
	{
		if (blinker>=10) altGsDriver.drawPipe.RectFlat(textX + (cursorpos * FONT_WIDTH), textY+1,
			textX + (cursorpos * FONT_WIDTH)+3, (textY + FONT_HEIGHT)-1, 6, altGS.WINFOREHEAD);
	} // show insert cursor
	else
	{
		if (blinker>=10) altGsDriver.drawPipe.RectFlat(textX + (cursorpos * FONT_WIDTH), textY+1,
			textX + (cursorpos * FONT_WIDTH)+FONT_WIDTH, (textY + FONT_HEIGHT)-1, 6, altGS.WINFOREHEAD);
	} // show overwrite cursor
	drawButtons(oskkey, osktip, 0);
	drawButtons(winbutton, winbuttontip, 0);
	if ((usepointer) && // if using pointer, check for 'X' close button
		pointerX > (oskscreen.Xpos + oskscreen.Xsize) - FONT_WIDTH * 2 && 
		pointerX < (oskscreen.Xpos + oskscreen.Xsize) &&
		pointerY > oskscreen.Ypos &&
		pointerY < (oskscreen.Ypos + oskscreen.Ysize))
	{
		activebutton = checkbuttons(winbutton, MAX_BUTTONS);
		if (activebutton >= 0) drawActiveButton(winbutton, winbuttontip, activebutton);
		if (pad & PAD_CROSS && activebutton == 0) return -1; // close
	}
	if ((usepointer) && // if using pointer, and its in the window, where is it pointing?
		pointerX > oskscreen.Xpos && 
		pointerX < (oskscreen.Xpos + oskscreen.Xsize) &&
		pointerY > oskscreen.Ypos &&
		pointerY < (oskscreen.Ypos + oskscreen.Ysize)) activebutton = checkbuttons(oskkey,55);
	else if (pad & PAD_LEFT || heldbutton & PAD_LEFT) // retreat to previous key
	{
		if (activebutton > 0) activebutton--;
		else activebutton = 54;
	}
	else if (pad & PAD_RIGHT || heldbutton & PAD_RIGHT) // advance to next key
	{
		if (activebutton < 54) activebutton++;
		else activebutton = 0;
	}
	else if (pad & PAD_UP) // handle moving one row up on keyboard
	{
		if (activebutton > 50)
		{
			if (activebutton == 54) activebutton=49;
			else if (activebutton == 53) activebutton=47;
			else if (activebutton == 52) activebutton=43;
			else if (activebutton == 51) activebutton=38;
		}
		else if (activebutton > 37) activebutton-=13;
		else if (activebutton > 24)
		{
			if (activebutton == 25) activebutton=13;
			else activebutton-=13;
		}
		else if (activebutton > 12) activebutton-=12;
	}
	else if (pad & PAD_DOWN) // handle moving one row down on keyboard
	{
		if (activebutton < 13)
		{
			if (activebutton == 12) activebutton=24;
			else activebutton+=13;
		}
		else if (activebutton < 38) activebutton+=13;
		else if (activebutton < 40) activebutton=51;
		else if (activebutton < 47) activebutton=52;
		else if (activebutton < 49) activebutton=53;
		else if (activebutton < 51) activebutton=54;
	}
	if (activebutton >= 0) // if a button is active, illuminate it
	{
		drawActiveButton(oskkey, osktip, activebutton);
	}
	if (pad & PAD_TRIANGLE) return -1; // cancel/close
	else if ((pad & PAD_L1 || heldbutton & PAD_L1) && cursorpos > 0)
		cursorpos--; // cursor left
	else if ((pad & PAD_R1 || heldbutton & PAD_R1) && cursorpos < strlen(copytext))
		cursorpos++; // cursor right
	else if (pad & PAD_CIRCLE && cursorpos > 0)
	{
		for (i = cursorpos-1;i<maxchars;i++) copytext[i]=copytext[i+1];
		cursorpos--; // delete character left of cursor
	}
	else if (keypress != '\0')
	{
		if (keypress == 0x0a) closemodule = true;
		else if (insmode)
		{
			if (strlen(copytext)<maxchars)
			{
				for(i=strlen(copytext);i>cursorpos;i--) copytext[i] = copytext[i-1];
				copytext[cursorpos]=keypress;
				cursorpos++;
			}
		}
		else
		{
			if (cursorpos<maxchars)
			{
				copytext[cursorpos]=keypress;
				cursorpos++;
			}
		}
		keypress='\0';
	}
	else if (pad & PAD_CROSS && activebutton >= 0) // handle a keypress
	{
		switch (activebutton)
		{
			case 37 : { closemodule = true; break; } // done
			case 49 : { if (cursorpos > 0) cursorpos--; break; } // cursor left
			case 50 : { if (cursorpos < strlen(copytext)) cursorpos++; break; } // cursor right
			case 51 :
						{	if (capson) { capson = false; keyptr = nocaps; }
							else { capson = true; keyptr = oncaps; }
							for (i=0;i<55;i++) // assign characters to keys accordingly
							{
								strcpy(osktip[i], keyptr);
								while (*keyptr++ != '\0') {} // for keys with more than 1 char
							}
							break; } // toggle caps
			case 52 : { if (insmode)
						{	if (strlen(copytext)<maxchars)
							{
								for (i=strlen(copytext);i>cursorpos;i--) copytext[i] = copytext[i-1];
								copytext[cursorpos]=' ';
								cursorpos++;
							}
						} // insert space
						else
						{	if (cursorpos<maxchars)
							{
								copytext[cursorpos]=' ';
								cursorpos++;
							}
						} // overwrite space
						break; }
			case 53 :
						{	if (insmode) insmode = false;
							else insmode = true;
							break; } // toggle insert/overwrite
			case 54 :
						{	memset (copytext, '\0', maxchars+1);
							cursorpos=0;
							break; } // clear text
			default : { if (insmode)
						{	if (strlen(copytext)<maxchars)
							{
								for (i=strlen(copytext);i>cursorpos;i--) copytext[i] = copytext[i-1];
								copytext[cursorpos]=osktip[activebutton][0];
								cursorpos++;
							}
						} // insert character
						else
						{	if (cursorpos<maxchars)
							{
								copytext[cursorpos]=osktip[activebutton][0];
								cursorpos++;
							}
						} // overwrite character
						break; }
		}
	}
	drawPointer();
	altGsDriver.drawPipe.Flush();
	altGsDriver.WaitForVSync();
	altGsDriver.swapBuffers();
 }
 strncpy(edittext, copytext, maxchars+1); // done, return edited text
 return 0;
}

////////////////////////////////////////////////////////////////////////
// test module, should probably change this to a help screen or similar
// just an example of a scrollable textwindow really
// would be nice to add an image display here too. text wrapping would
// be nice, but isn't essential
void infoModule()
{
 int closemodule = false, activebutton;
 int pad;
 int maxlines;
 int winbutton[MAX_BUTTONS][5];		// array of button data for window buttons
 char winbuttontip[MAX_BUTTONS][30];	// array of labels for window buttons
 char *testtitle = "Welcome to Altimit v0.1\0";
 char *testtext = "This program is brought to you by t0mb0la as a logical\n"
"progression from his first PS2 project, PS2MENU.\n"
"\n"
"It is essentially the same program; that is, a tool for\n"
"loading homebrew PS2 applications, tools, demos and such\n"
"from the various file devices made available to us by\n"
"Sony's hardware and the dedicated work of individuals\n"
"associated with ps2dev.org / efnet #ps2dev.\n"
"\n"
"Without the libraries developed and made available to\n"
"us by these wise and hard working people, this program\n"
"would never have been produced. So before I write any\n"
"more words, it is only right that I give them the\n"
"credit and mention that they all very much deserve.\n"
"\n"
"Many thanks to (in no particular order): Oobles, Hiryu,\n"
"MrBrown, Pukko, Sjeep, adresd, Pixel, Drakonite, ooPo,\n"
"Tyranid, Blackdroid, emoon, jar, raizor, Warren, Karmix.\n"
"I apologise if I've missed anyone else, give me a kick\n"
"and I'll make sure you get mentioned in an update.\n"
"Pretty much all the regulars on efnet #ps2dev have\n"
"contributed in one way or another; be it with code,\n"
"inspiration, advice or whatever. You are all very much\n"
"appreciated.\n"
"\n"
"OK, back to the program. As I said above, it is the\n"
"same as PS2MENU, but very different in its approach.\n"
"It has been rewritten from scratch, with more care\n"
"and planning taken during the design stage. PS2MENU\n"
"was, is, and always will be a mess of code; there was no\n"
"plan, no design, but a lot of lessons were learned. That's\n"
"not to say that Altimit is really a great deal better,\n"
"this I'll leave to your judgement. I'll always have an\n"
"open ear to your comments and suggestions, so please let\n"
"me hear them.\n"
"For now, bear in mind that Altimit is still in development\n"
"and some features may not be ready to use yet. So I'm going\n"
"to end this text here and get back to coding.\n\0";

 testscreen.Xpos = 96;
 testscreen.Ypos = 8;
 testscreen.Zpos = 3;
 testscreen.Xsize = 528;
 if (altGS.HEIGHT == 224 || altGS.HEIGHT == 256) testscreen.Ysize = 192;
 else { testscreen.Ysize = 384; testscreen.Ypos = 16; }
 testscreen.Foreground = altGS.WINFORECOL;
 testscreen.Background = altGS.WINBACKCOL;
 testscreen.Forehead = altGS.WINFOREHEAD;
 testscreen.Backhead = altGS.WINBACKHEAD;
 testscreen.Title = testtitle;
 testscreen.Content = testtext;
 testscreen.Top = 1;
 winbutton[0][0] = (testscreen.Xpos + testscreen.Xsize) - FONT_WIDTH*2;
 winbutton[0][1] = testscreen.Ypos;
 winbutton[0][2] = winbutton[0][0] + FONT_WIDTH*2;
 winbutton[0][3] = winbutton[0][1] + FONT_HEIGHT;
 strcpy (winbuttontip[0], "X");
 winbutton[1][0] = (testscreen.Xpos + testscreen.Xsize) - FONT_WIDTH*2;
 winbutton[1][1] = testscreen.Ypos + FONT_HEIGHT;
 winbutton[1][2] = winbutton[1][0] + FONT_WIDTH*2;
 winbutton[1][3] = winbutton[1][1] + FONT_HEIGHT;
 strcpy (winbuttontip[1], "^");
 winbutton[2][0] = (testscreen.Xpos + testscreen.Xsize) - FONT_WIDTH*2;
 winbutton[2][1] = (testscreen.Ypos + testscreen.Ysize) - FONT_HEIGHT;
 winbutton[2][2] = winbutton[2][0] + FONT_WIDTH*2;
 winbutton[2][3] = winbutton[2][1] + FONT_HEIGHT;
 strcpy (winbuttontip[2], "v");
 winbutton[3][0] = -1;
 maxlines = (testscreen.Ysize - FONT_HEIGHT)/FONT_HEIGHT;
 activebutton = -1;
 while (!closemodule)
 {
	pad = readpadbutton();
	drawMainWindow();
	drawTextWindow(testscreen);
	drawButtons(winbutton, winbuttontip, 0);
	if (usepointer) activebutton = checkbuttons(winbutton, MAX_BUTTONS);
	if (activebutton >= 0) drawActiveButton(winbutton, winbuttontip, activebutton);
	if (pad & PAD_TRIANGLE || (pad & PAD_CROSS && activebutton==0)) closemodule=true;
	if (heldbutton & PAD_UP || pad & PAD_UP || (pad & PAD_CROSS && activebutton==1))
		if (testscreen.Top > 1) testscreen.Top--;
	if (heldbutton & PAD_DOWN || pad & PAD_DOWN || (pad & PAD_CROSS && activebutton==2))
		if (testscreen.Top < (countlines(testtext)-(maxlines-1))) testscreen.Top++;
	drawPointer();
	altGsDriver.drawPipe.Flush();
	altGsDriver.WaitForVSync();
	altGsDriver.swapBuffers();
 }
}

////////////////////////////////////////////////////////////////////////
// loads LOADER.ELF from program memory and passes args of selected ELF
// and partition to it
// Modified version of loader from Independence
//	(C) 2003 Marcus R. Brown <mrbrown@0xd6.org>
//
void RunLoaderElf(char *filename, char *partition)
{
	u8 *boot_elf = (u8 *)&loader_elf;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;
	void *pdata;
	int i, argc;
	char *argv[1];

	padPortClose(0,0);
/* Load the ELF into RAM.  */
	if (_lw((u32)&eh->ident) != ELF_MAGIC)
	{
//		*GS_BGCOLOR =	0x0000FF;		// BLUE
		goto error;
		}

	if (strlen(partition) > 0) argc = 2;
	else argc = 1;
	eph = (elf_pheader_t *)(boot_elf + eh->phoff);

/* Scan through the ELF's program headers and copy them into RAM, then
									zero out any non-loaded regions.  */
	for (i = 0; i < eh->phnum; i++)
	{
		if (eph[i].type != ELF_PT_LOAD)
		continue;

		pdata = (void *)(boot_elf + eph[i].offset);
		memcpy(eph[i].vaddr, pdata, eph[i].filesz);

/*		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);*/
		}	// hmm, C++ doesn't seem to like the above part which wipes non-loaded regions
			// fortunately LOADER.ELF doesn't seem to mind this not being done
/* Let's go.  */
	fioExit();
//	SifResetIop();

//	SifExitRpc();
	SifInitRpc(0);
	SifExitRpc();
	FlushCache(0);
	FlushCache(2);

	argv[0] = filename;
	if (argc == 2) argv[1] = partition;

	ExecPS2((void *)eh->entry, 0, argc, argv);

	return;
error:
	while (1) ;

	}
