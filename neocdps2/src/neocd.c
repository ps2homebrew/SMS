/**************************************
****  NEOCD.C  - Main Source File  ****
**************************************/

// DEGUG DEVELOPMENT MODE CAN BE ACTIVATED IN THE MAKEFILE
// IN THAT CASE, NEOCD WILL LOOK UNDER "host0:./cd" for the games
// (COPY DIRECTLY THE FILES FROM THE CD)


//-- Include Files -----------------------------------------------------------

#include <tamtypes.h>
#include <kernel.h>
#include <fileio.h> 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sifcmd.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <malloc.h>
#include <libcdvd.h>


#include "neocd.h"
#include "video/video.h"


#define REGION_JAPAN  0
#define REGION_USA    1
#define REGION_EUROPE 2

#define REGION REGION_USA // EUROPE RULES !

#define ROM_PADMAN // FOR PAD LOAD MODULE

//-- Global Variables --------------------------------------------------------
char			*neogeo_rom_memory __attribute__((aligned(64))) = NULL;
char			*neogeo_prg_memory __attribute__((aligned(64))) = NULL;
char			*neogeo_fix_memory __attribute__((aligned(64))) = NULL;
char			*neogeo_spr_memory __attribute__((aligned(64))) = NULL;
char			*neogeo_pcm_memory __attribute__((aligned(64))) = NULL;

//char			global_error[80];
unsigned char		neogeo_memorycard[8192] __attribute__((aligned(64)));
int			neogeo_prio_mode = 0;
int			neogeo_ipl_done = 0;
char			neogeo_region=REGION;
//unsigned char		config_game_name[80];

uint32			neocd_time; /* next time marker */

char path_prefix[128]  __attribute__((aligned(64))) = "cdrom0:\\"; 

int boot_mode = BOOT_CD; // by defaut

 
//-- 68K Core related stuff --------------------------------------------------
int				mame_debug = 0;
int				previouspc = 0;
int				ophw = 0;
int				cur_mrhard = 0;


//-- Function Prototypes -----------------------------------------------------
void	neogeo_init(void);
void	neogeo_reset(void);
void	neogeo_hreset(void);
void	neogeo_shutdown(void);
void	MC68000_Cause_Interrupt(int);
void	neogeo_exception(void);
void	neogeo_run(void);
void	draw_main(void);
void	neogeo_quit(void);
void	not_implemented(void);
void	neogeo_machine_settings(void);
void	neogeo_debug_mode(void);
void	neogeo_cdda_check(void);
void	neogeo_cdda_control(void);
void	neogeo_do_cdda( int command, int trck_number_bcd);
void	neogeo_read_gamename(void);

//----------------------------------------------------------------------------
int	main(int argc, char* argv[])
{
	int fd=0; 
	
	char bootpath[256];

	int	result;
	char * fixtmp;
	
	// Displays version number, date and time
	printf(VERSION1);
	printf(VERSION2);
	printf(AUTHOR);
	
	// detect host : strip elf from path
	if (argc>=1)
	{
	    char *p;
	    if ((p = strrchr(argv[0], '/'))!=NULL) {
	      snprintf(path_prefix, sizeof(path_prefix), "%s", argv[0]);
	      p = strrchr(path_prefix, '/');
	      if (p!=NULL)
	        p[1]='\0';
	    } else if ((p = strrchr(argv[0], '\\'))!=NULL) {
	      snprintf(path_prefix, sizeof(path_prefix), "%s", argv[0]);
	      p = strrchr(path_prefix, '\\');
	      if (p!=NULL)
	        p[1]='\0';
	    } else if ((p = strchr(argv[0], ':'))!=NULL) {
	      snprintf(path_prefix, sizeof(path_prefix), "%s", argv[0]);
	      p = strchr(path_prefix, ':');
	      if (p!=NULL)
	        p[1]='\0';
	    }
	}
	
  	if (!strncmp(path_prefix, "cdrom", strlen("cdrom"))) {
        	printf("Booting from cd\n");
        	boot_mode = BOOT_CD;
        } else if(!strncmp(path_prefix, "mc", strlen("mc"))) {
        	printf("Booting from mc\n");
        	boot_mode = BOOT_MC;
    	} else if(!strncmp(path_prefix, "host", strlen("host"))) {
	        printf("Booting from host\n");
	        boot_mode = BOOT_HO;
    	}	


	printf("rpc init\n");
	SifInitRpc(0);
	// load modules
  	loadModules();  	
  	 	
  	// init CDROM
  	cdInit(CDVD_INIT_INIT);
  		
	// Video init
	printf("\ninitializing video.... ");
	result = video_init();
	if (result==-1) 
	{
		printf("failed !\n");
		return 0;
	}
	if (result==PAL_MODE)
	   printf("PAL MODE SET\n");
	if (result==NTSC_MODE)
	   printf("NTSC MODE SET\n");
	
	// display the loading screen
	display_splashscreen();
	
	
	
	//printf("----> Starting timer : %d\n", PS2_InitTicks());
	
	// Allocate needed memory buffers	
	printf("NEOGEO: Allocating memory :\n");
	printf("PRG (2MB) ... ");
	neogeo_prg_memory = (char*)malloc(0x200000);
	if (neogeo_prg_memory==NULL) {
		printf("failed !\n");
		return 0;
	}
	printf("DONE!\n");

	printf("SPR (4MB) ... ");
	neogeo_spr_memory = (char*)malloc(0x400000);
	if (neogeo_spr_memory==NULL) {
		printf("failed !\n");
		return 0;
	}
	printf("DONE!\n");
	
	printf("ROM (512kb) ... ");
	neogeo_rom_memory = (char*)malloc(0x80000);
	if (neogeo_rom_memory==NULL) {
		printf("failed !\n");
		return 0;
	}
	printf("DONE!\n");

	printf("FIX (128kb) ... ");
	neogeo_fix_memory = (char*)malloc(0x20000);
	if (neogeo_fix_memory==NULL) {
		printf("failed !\n");
		return 0;
	}
	printf("DONE!\n");

	printf("PCM (1Mb) ... ");
	neogeo_pcm_memory = (char*)malloc(0x100000);
	if (neogeo_pcm_memory==NULL) {
		printf("failed !\n");
		return 0;
	}
	printf("DONE!\n");
	
	// Initialize Memory Mapping
	initialize_memmap();
	
	//init fio
	fioInit();

	// Read memory card	
	printf("Loading memcard...\n");
	fioClose(fd);
	strcpy (bootpath,path_prefix);
	if (boot_mode == BOOT_CD) 
	  strcat (bootpath,"ROMDISK\\MEMCARD.BIN;1");
	else strcat (bootpath,"romdisk\\memcard.bin");
	fd = fioOpen(bootpath, O_RDONLY);
	if (fd<0)
	{
	   printf("Fatal Error: Could not load MEMCARD.BIN\n");
	   return (0);
	}
	fioRead(fd, neogeo_memorycard, 8192);
	fioClose(fd);
	printf("DONE!\n");

	// Load BIOS
	printf("Loading BIOS...\n");
	strcpy (bootpath,path_prefix);
	if (boot_mode == BOOT_CD) 
	  strcat (bootpath,"BIOS\\NEOCD.BIN;1");
	else strcat (bootpath,"bios\\neocd.bin");
	fd = fioOpen(bootpath, O_RDONLY);
	if (fd<0)
	{  
		printf("Fatal Error: Could not load NEOCD.BIN\n");
		return 0;
	}
	fioRead(fd, neogeo_rom_memory, 0x80000);
	fioClose(fd); 
		
	fixtmp=(char*)malloc(65536);
	memcpy(fixtmp,&neogeo_rom_memory[458752],65536);
	
	fix_conv(fixtmp,&neogeo_rom_memory[458752],65536,rom_fix_usage);
	//swab(neogeo_rom_memory, neogeo_rom_memory, 131072);
	
	printf("DONE!\n");
	free(fixtmp);
	fixtmp=NULL;

	printf("Loading startup ram...\n");
	// Load startup RAM
	strcpy (bootpath,path_prefix);
	if (boot_mode == BOOT_CD) 
	  strcat (bootpath,"ROMDISK\\STARTUP.BIN;1");
	else strcat (bootpath,"romdisk\\startup.bin");
	fd = fioOpen(bootpath, O_RDONLY);
	if (fd<0)
	{ 
	   printf("Fatal Error: Could not load STARTUP.BIN\n");
	   return 0;
	} 
	fioRead(fd, neogeo_prg_memory + 0x10F300, 3328); 
	fioClose(fd);
	printf("DONE!\n");
		
	swab(neogeo_prg_memory + 0x10F300, neogeo_prg_memory + 0x10F300, 3328);


	printf("patching BIOS...");
	// Check BIOS validity
	
	if (*((short*)(neogeo_rom_memory+0xA822)) != 0x4BF9)
	{
		printf("Fatal Error: Invalid BIOS file.\n");
		return 0;
	}
	printf(" OK\n");

	// Patch BIOS load files w/ now loading message/
	*((short*)(neogeo_rom_memory+0x552)) = 0xFABF;
	*((short*)(neogeo_rom_memory+0x554)) = 0x4E75;
	// Patch BIOS load files w/out now loading/
	*((short*)(neogeo_rom_memory+0x564)) = 0xFAC0;
	*((short*)(neogeo_rom_memory+0x566)) = 0x4E75;
	// Patch BIOS CDROM Check/
	*((short*)(neogeo_rom_memory+0xB040)) = 0x4E71;
	*((short*)(neogeo_rom_memory+0xB042)) = 0x4E71;
	// Patch BIOS upload command/
	*((short*)(neogeo_rom_memory+0x546)) = 0xFAC1;
	*((short*)(neogeo_rom_memory+0x548)) = 0x4E75;

	// Patch BIOS CDDA check/
	*((short*)(neogeo_rom_memory+0x56A)) = 0xFAC3;
	*((short*)(neogeo_rom_memory+0x56C)) = 0x4E75;

	// Full reset, please/
	*((short*)(neogeo_rom_memory+0xA87A)) = 0x4239;
	*((short*)(neogeo_rom_memory+0xA87C)) = 0x0010;
	*((short*)(neogeo_rom_memory+0xA87E)) = 0xFDAE;

	//Trap exceptions/
	*((short*)(neogeo_rom_memory+0xA5B6)) = 0x4AFC;
	
	printf("DONE!\n");

	//current_time2 = PS2_GetTicks();
	//printf("----> %d msec elapsed since beginning\n",current_time2);

	// Initialise input
	printf("Initialize input...\n");
	input_init();

	// Initialize CD-ROM
	printf("Initialize cdrom...\n");

	cdrom_init1();
	// display loading screen
	display_loadingscreen();
	
	cdda_init();
	
	// Sound init
	//init_audio();
	
	sound_shutdown();

	// Initialize everything
	neogeo_init();
	pd4990a_init();
	neogeo_run();

	return 0;

}

//----------------------------------------------------------------------------
void	neogeo_init(void)
{
 	printf("Reset M68K...\n");
	m68k_pulse_reset();
 
}

//----------------------------------------------------------------------------
void	neogeo_hreset(void)
{


	//FILE * fp;

	printf("NEOGEO Hard Reset...\n");
	
	// read game name
	//neogeo_read_gamename(); for what now ???

	/* TO BE READDED LATER
	// Special patch for Samurai Spirits RPG
	if (strcmp(config_game_name, "TEST PROGRAM USA") == 0)
	{
		strcpy(config_game_name, "SAMURAI SPIRITS RPG");
		
		fp = fopen("patch.prg", "rb");
		if (fp == NULL) {
			printf("Fatal Error: Couldnt open patch.prg.\n");
			exit(1);
		}
		
		fread(neogeo_prg_memory + 0x132000, 1, 112, fp);
		fclose(fp);
		swab(neogeo_prg_memory + 0x132000, neogeo_prg_memory + 0x132000, 112);
	
	}
	*/
	

	// First time init
	printf("m68K pulse reset...\n");
	m68k_pulse_reset();
	printf("m68K set reg...\n");
	m68k_set_reg(M68K_REG_PC,0xc0a822);
	m68k_set_reg(M68K_REG_SR,0x2700);
	m68k_set_reg(M68K_REG_A7,0x10F300);
	m68k_set_reg(M68K_REG_ISP,0x10F300);
	m68k_set_reg(M68K_REG_USP,0x10F300);
	printf("m68K set mem 1...\n");
	m68k_write_memory_32(0x10F6EE, m68k_read_memory_32(0x68)); // $68 *must* be copied at 10F6EE
	printf("m68K set mem 2...\n");
	if (m68k_read_memory_8(0x107)&0x7E)
	{
		if (m68k_read_memory_16(0x13A))
		{
			m68k_write_memory_32(0x10F6EA, (m68k_read_memory_16(0x13A)<<1) + 0xE00000);
		}
		else
		{
			m68k_write_memory_32(0x10F6EA, 0);
			m68k_write_memory_8(0x00013B, 0x01);
		}
	}
	else
		m68k_write_memory_32(0x10F6EA, 0xE1FDF0);

	// Set System Region
	printf("set system region...\n");
	m68k_write_memory_8(0x10FD83,neogeo_region);

	cdda_current_track = 0;
	cdda_get_disk_info();
	
	printf("init z80...\n");
	z80_init();
	printf("done...\n");
 
}	

//----------------------------------------------------------------------------
void neogeo_reset(void)
{

	printf("NEOGEO Soft Reset...\n");	
	m68k_pulse_reset();
	m68k_set_reg(M68K_REG_PC,0x122);
	m68k_set_reg(M68K_REG_SR,0x2700);
	m68k_set_reg(M68K_REG_A7,0x10F300);
	m68k_set_reg(M68K_REG_ISP,0x10F300);
	m68k_set_reg(M68K_REG_USP,0x10F300);

	m68k_write_memory_8(0x10FD80, 0x82);
	m68k_write_memory_8(0x10FDAF, 0x01);
	m68k_write_memory_8(0x10FEE1, 0x0A);
	m68k_write_memory_8(0x10F675, 0x01);
	m68k_write_memory_8(0x10FEBF, 0x00);
	m68k_write_memory_32(0x10FDB6, 0);
	m68k_write_memory_32(0x10FDBA, 0);

	// System Region
	m68k_write_memory_8(0x10FD83,neogeo_region);

	cdda_current_track = 0;
	cdda_get_disk_info(); 


	printf("init z80...\n");
	z80_init();
	printf("done...\n");

}

//----------------------------------------------------------------------------
void	neogeo_shutdown(void)
{

	//FILE	*fp;
	
	// Close everything and free memory
	cdda_shutdown();
	cdrom_shutdown();
	sound_shutdown();
	input_shutdown();
	video_shutdown();

	printf("NEOGEO: System Shutdown.\n");

	/*
	fp = fopen("memcard.bin", "wb");
	if (fp != NULL) {	
		fwrite(neogeo_memorycard, 1, 8192, fp);
		fclose(fp);
	} else {
		printf("Error: Couldn't open memcard.bin for writing.\n");
	}
	*/

	free(neogeo_prg_memory);
	free(neogeo_rom_memory);
	free(neogeo_spr_memory);
	free(neogeo_fix_memory);
	free(neogeo_pcm_memory);
	
    	fioExit();
	SleepThread();
	return;
}

//----------------------------------------------------------------------------
void	neogeo_exception(void)
{
	printf("NEOGEO: Exception Trapped at %08x !\n", previouspc);
	exit(0);
}	

//----------------------------------------------------------------------------
void MC68000_Cause_Interrupt(int level)
{

	m68k_set_irq(level);

}

//----------------------------------------------------------------------------
void	neogeo_exit(void)
{
	printf("NEOGEO: Exit requested by software...\n");
	exit(0);
}

//----------------------------------------------------------------------------
void	neogeo_run(void)
{
 
	//uint32 now;

    	int	i;

	printf("START EMULATION...\n");
	
	// If IPL.TXT not loaded, load it !
	if (!neogeo_ipl_done)
	{
		// Display Title
		#ifndef DEBUG
		 cdrom_load_title();
		#endif

		// Process IPL.TXT
		if (!cdrom_process_ipl()) {
			printf("Error: Error while processing IPL.TXT.\n");
			return;
		}
		
		// Reset everything
		neogeo_ipl_done = 1;
		neogeo_hreset();
	}

	// get time for speed throttle

    	//neocd_time=PS2_GetTicks()+REFRESHTIME;

	
	// Main loop
	my_timer();
	
	// Speed Throttle
	//PS2_StartTicks();
	z80_cycles = Z80_VBL_CYCLES/256;
	while(1)
	{
		// Execute Z80 timeslice (one VBL)
		mz80int(0);
		/* TO BE DONE TO BE DONE
		// Z80 runs anyway to check perf
		if (AUDIO_PLAYING) {*/
		
		    for (i = 0; i < 256; i++)
		    {
			if (z80_cycles>0) {
			    //printf("exec z80\n");
			    mz80exec(z80_cycles);
			    z80_cycles=0;
			    my_timer();
			}
			z80_cycles += Z80_VBL_CYCLES/256;
		    }
		//}


		// One-vbl timeslice
		//printf("execute 68k\n");
		m68k_execute(200000);
		//printf("setirq 68k\n");
		m68k_set_irq(2);
		
		// update pd4990a
		//printf("update pd4990a\n");
		pd4990a_addretrace();
		
		// check the watchdog
		if (watchdog_counter > 0) {
		    if (--watchdog_counter == 0) {
			printf("reset caused by the watchdog\n");
			neogeo_reset();
		    }
		}
		
		// check for memcard writes
		if (memcard_write > 0) {
		   memcard_write--;
		   if(memcard_write==0) {
		     // write memory card here
			 // if you need to keep file up to date
		   }
		}

		// Call display routine
		//printf("call display routine\n");
		video_draw_screen1();

		// Check if there are pending commands for CDDA
		neogeo_cdda_check();
		cdda_loop_check();

		// Update keys and Joystick
		processEvents();


		// Speed Throttle
		/*
		now=PS2_GetTicks();
		if (now < neocd_time)
			PS2_Delay(neocd_time-now);
		neocd_time+=REFRESHTIME;
		*/

	}

	// Stop CDDA
	cdda_stop();
		
	return;
 
}

//----------------------------------------------------------------------------
// This is a really dirty hack to make SAMURAI SPIRITS RPG work
void	neogeo_prio_switch(void)
{
 
	if (m68k_get_reg(NULL,M68K_REG_D7) == 0xFFFF)
		return;
	
	if (m68k_get_reg(NULL,M68K_REG_D7) == 9 && 
	    m68k_get_reg(NULL,M68K_REG_A3) == 0x10DED9 &&
		(m68k_get_reg(NULL,M68K_REG_A2) == 0x1081d0 ||
		(m68k_get_reg(NULL,M68K_REG_A2)&0xFFF000) == 0x102000)) {
		neogeo_prio_mode = 0;
		return;
	}
	
	if (m68k_get_reg(NULL,M68K_REG_D7) == 8 && 
	    m68k_get_reg(NULL,M68K_REG_A3) == 0x10DEC7 && 
		m68k_get_reg(NULL,M68K_REG_A2) == 0x102900) {
		neogeo_prio_mode = 0;
		return;
	}
	
	if (m68k_get_reg(NULL,M68K_REG_A7) == 0x10F29C)
	{
		if ((m68k_get_reg(NULL,M68K_REG_D4)&0x4010) == 0x4010)
		{
			neogeo_prio_mode = 0;
			return;
		}
		
		neogeo_prio_mode = 1;
	}
	else
	{
		if (m68k_get_reg(NULL,M68K_REG_A3) == 0x5140)
		{
			neogeo_prio_mode = 1;
			return;
		}

		if ( (m68k_get_reg(NULL,M68K_REG_A3)&~0xF) == (m68k_get_reg(NULL,M68K_REG_A4)&~0xF) )
			neogeo_prio_mode = 1;
		else
			neogeo_prio_mode = 0;
	}

}

//----------------------------------------------------------------------------
void not_implemented(void)
{
		printf("Error: This function isn't implemented.\n");
}

//----------------------------------------------------------------------------
void neogeo_quit(void)
{
		exit(0);
}

//----------------------------------------------------------------------------
void neogeo_cdda_check(void)
{
  
	int		Offset;
	
	Offset = m68k_read_memory_32(0x10F6EA);
	if (Offset < 0xE00000)	// Invalid addr
		return;

	Offset -= 0xE00000;
	Offset >>= 1;
	
	neogeo_do_cdda(subcpu_memspace[Offset], subcpu_memspace[Offset+1]);
  
}

//----------------------------------------------------------------------------
void neogeo_cdda_control(void)
{

	neogeo_do_cdda( (m68k_get_reg(NULL,M68K_REG_D0)>>8)&0xFF, 
	                 m68k_get_reg(NULL,M68K_REG_D0)&0xFF );

}

//----------------------------------------------------------------------------
void neogeo_do_cdda( int command, int track_number_bcd)
{

	int		track_number;
	int		offset;

	if ((command == 0)&&(track_number_bcd == 0))
		return;

	m68k_write_memory_8(0x10F64B, track_number_bcd);
	m68k_write_memory_8(0x10F6F8, track_number_bcd);
	m68k_write_memory_8(0x10F6F7, command);
	m68k_write_memory_8(0x10F6F6, command);

	offset = m68k_read_memory_32(0x10F6EA);

	if (offset)
	{
		offset -= 0xE00000;
		offset >>= 1;

		m68k_write_memory_8(0x10F678, 1);

		subcpu_memspace[offset] = 0;
		subcpu_memspace[offset+1] = 0;
	}

	switch( command )
	{
		case	0:
		case	1:
		case	5:
		case	4:
		case	3:
		case	7:
			track_number = ((track_number_bcd>>4)*10) + (track_number_bcd&0x0F);
			if ((track_number == 0)&&(!cdda_playing))
			{
				//ORI sound_mute();
				cdda_resume();
			}
			else if ((track_number>1)&&(track_number<99))
			{
				//sound_mute(); ORI
				cdda_play(track_number);
				cdda_autoloop = !(command&1);
			}
			break;
		case	6:
		case	2:
			if (cdda_playing)
			{
				//sound_mute(); ORI
				cdda_pause();
			}
			break;
	}

}
//----------------------------------------------------------------------------
/*void neogeo_read_gamename(void)
{

	unsigned char	*Ptr;
	int		temp;

	Ptr = neogeo_prg_memory + m68k_read_memory_32(0x11A);
	swab(Ptr, config_game_name, 80);
	for(temp=0;temp<80;temp++) {
		if (!isprint(config_game_name[temp])) {
			config_game_name[temp]=0;
			break;
		}
	}

}
*/

/*
 * loadModules()
 */
void loadModules(void)
{
    int ret;

    
    #ifdef ROM_PADMAN
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    #else
    ret = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
    #endif
    if (ret < 0) {
        printf("sifLoadModule sio failed: %d\n", ret);
        SleepThread();
    }    

    #ifdef ROM_PADMAN
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    #else
    ret = SifLoadModule("rom0:XPADMAN", 0, NULL);
    #endif 
    if (ret < 0) {
        printf("sifLoadModule pad failed: %d\n", ret);
        SleepThread();
    }
    if (boot_mode == BOOT_MC)
    {
	SifLoadModule("rom0:MCMAN", 0, NULL);  	
	SifLoadModule("rom0:MCSERV", 0, NULL); 
    }
    SifLoadModule("rom0:CDVDMAN", 0, NULL); 
}
