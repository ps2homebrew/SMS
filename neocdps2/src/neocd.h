/*
 *  neocd.h - Main file
 *  Copyright (C) 2001-2003 Foster (Original Code)
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include "static/modules.h"

#include "cpu_z80/z80intrf.h"
#include "cpu_68k/cpu68k.h"

#include "gs/hw.h"
#include "gs/gfxpipe.h"

#include "gui/menu.h"
#include "gui/ps2print.h"

#include "sound/sound.h"
#include "sound/2610intf.h"
#include "sound/timer.h"
#include "sound/sjpcm.h"
#include "misc/misc.h"
#include "pd4990a.h"


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

/*-- Version, date & time to display on startup ----------------------------*/
#define VERSION1 "NeoCD/PS2 "
#define VERSION2_MAJOR 0
#define VERSION2_MINOR 5
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

//extern int 	z80_run_cycles;


// Structure will be used directly in the options save/load code.
typedef struct
{
	uint8 version;
	uint8 region;			// 0 = JAP, 1 = USA, 2 = Europe
	uint8 renderFilter;		// 0 = Nearest, 1 = Linear
	uint8 soundOn;			// 0 = Sound off, 1 = Sound on
	uint8 showFPS;			// 0 = off, 1 = on
	uint8 frameskip;		// 0 = Save off, 1 = save on
	uint8 CDDAOn;			// 0 = CDDA off, 1 = CDDA on
	uint8 SaveOn;			// 0 = Save off, 1 = save on
	uint8 dispXPAL, dispYPAL;	// X & Y offset parameters for both
	uint8 dispXNTSC, dispYNTSC;	// PAL and NTSC display modes.
	uint8 fullscreen;		// 0 = no, 1 = yes		
	uint8 rfu_2;			// RFU
	uint8 rfu_3,rfu_4;		// RFU.

} struct_neocdSettings __attribute__((aligned(64))) ;

extern struct_neocdSettings neocdSettings  __attribute__((aligned(64)));

typedef struct
{
	u32  	vdph;			// video heigth
	u32 	vidsys;			// video mode
	u32 	fps_rate;		// fps_rate
	int 	snd_sample;		// snd sample
	int   	m68k_cycles;		// 68K cycles/frame
	u32   	z80_cycles;		// z80 cycles/frame
	u32   	z80_cycles_slice;	// z80_cycles / 256 !
	u32  	y1_offset;			// y1 texture offset
	u32  	y2_offset;			// y2 texture offset
	int 	dispx;			// X screen offset 
	int 	dispy;			// Y screen offset 
	
} struct_machine  __attribute__((aligned(64)));
extern struct_machine machine_def __attribute__((aligned(64))) ;


void loadModules(void);
void neogeo_reset(void);
void neogeo_hreset(void);

#endif /* NEOCD_H */
