/**
 * NeoCD/PS2 main header file
 *
 * 2004 Evilo
 */
 
#ifndef NEOCD_H
#define NEOCD_H

#include "defines.h"
#include "cdaudio/cdaudio.h"
#include "cdrom/cdrom.h"
#include "cdrom/cdvd_rpc.h"
#include "memory/memory.h"
#include "video/video.h"
#include "input/input.h"
#ifdef USE_MAMEZ80
#include "mz80/z80.h"
#include "mz80/mz80interf.h"
#else
#include "z80/mz80.h"
#endif
#include "sound/sound.h"
#include "sound/2610intf.h"
#include "sound/timer.h"
#include "misc/misc.h"
#include "pd4990a.h"
#include "cpu/cpu68k.h"

// boot mode
#define BOOT_CD 0
#define BOOT_MC 1
#define BOOT_HO 2
#define BOOT_HD 3
#define BOOT_UNKNOW 4

// game mode
#define BOOT_HOST 4


#define REGION_JAPAN  0
#define REGION_USA    1
#define REGION_EUROPE 2

#define FPS_PAL  50
#define FPS_NTSC 60

//#define REFRESHTIME 1000/60

//#define Z80_VBL_CYCLES 66666 //z80 4Mhz
//#define Z80_VBL_CYCLES_DIV256 Z80_VBL_CYCLES/256


/*-- Version, date & time to display on startup ----------------------------*/
#define VERSION1 "NeoCD/PS2 "
#define VERSION2_MAJOR 0
#define VERSION2_MINOR 4
#define VERSION3 "Compiled on: "__DATE__" "__TIME__"\n"
#define AUTHOR   "PS2 version by [evilo]\n"

/*-- functions -------------------------------------------------------------*/

/*-- globals ---------------------------------------------------------------*/

extern char	*neogeo_rom_memory __attribute__((aligned(64)));
extern char	*neogeo_prg_memory __attribute__((aligned(64)));
extern char	*neogeo_fix_memory __attribute__((aligned(64)));
extern char	*neogeo_spr_memory __attribute__((aligned(64)));
extern char	*neogeo_pcm_memory __attribute__((aligned(64)));

extern char 	path_prefix[128] __attribute__((aligned(64)));

extern int      neogeo_ipl_done;

extern 		uint32 neocd_time;

extern char	*OP_ROM;
extern char 	neogeo_game_vectors[128]  __attribute__((aligned(64)));


extern int	boot_mode;
extern int	game_boot_mode;


// Structure will be used directly in the options save/load code.
typedef struct
{
	uint8 version;
	uint8 region;			// 0 = JAP, 1 = USA, 2 = Europe
	uint8 renderFilter;		// 0 = Nearest, 1 = Linear
	uint8 soundOn;			// 0 = Sound off, 1 = Sound on
	uint8 rfu;			// RFU
	uint8 CDDAOn;			// 0 = CDDA off, 1 = CDDA on
	uint8 SaveOn;			// 0 = Save off, 1 = save on
	uint8 dispXPAL, dispYPAL;	// X & Y offset parameters for both
	uint8 dispXNTSC, dispYNTSC;	// PAL and NTSC display modes.

} struct_neocdSettings;
extern struct_neocdSettings neocdSettings  __attribute__((aligned(64)));

typedef struct
{
	uint  	vdph;			// video heigth
	uint 	vidsys;			// video mode
	uint 	fps_rate;		// fps_rate
	int 	snd_sample;		// snd sample
	int   	m68k_cycles;		// 68K cycles/frame
	int   	z80_cycles;		// z80 cycles/frame
	int   	z80_cycles_slice;	// z80_cycles / 256 !
	u32  	y1_offset;			// y1 texture offset
	u32  	y2_offset;			// y2 texture offset
	int 	dispx;			// X screen offset 
	int 	dispy;			// Y screen offset 
	
} struct_machine  __attribute__((aligned(64)));
extern struct_machine machine_def;


void loadModules(void);
void neogeo_reset(void);
void neogeo_hreset(void);

#endif /* NEOCD_H */
