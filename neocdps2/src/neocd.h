/**
 * NeoCD/PS2 main header file
 **
 * 2004 Evilo
 */
 
#ifndef NEOCD_H
#define NEOCD_H

#define REFRESHTIME 1000/60

#define Z80_VBL_CYCLES 100000
#define SAMPLE_RATE    22050

#include "defines.h"

#include "cdaudio/cdaudio.h"
#include "cdrom/cdrom.h"
#include "mc68000/m68k.h"
#include "memory/memory.h"
#include "video/video.h"
#include "input/input.h"
#include "z80/z80intrf.h"
#include "sound/sound.h"
#include "sound/streams.h"
#include "sound/2610intf.h"
#include "sound/timer.h"
#include "misc/misc.h"
#include "pd4990a.h"


#define BOOT_CD 0
#define BOOT_MC 1
#define BOOT_HO 2

/*-- Version, date & time to display on startup ----------------------------*/
#define VERSION1 "NeoCD/PS2 0.0.2a\n"
#define VERSION2 "Compiled on: "__DATE__" "__TIME__"\n"

#define AUTHOR   "PS2 version by [evilo]\n"

/*-- functions -------------------------------------------------------------*/


/*-- globals ---------------------------------------------------------------*/
//extern char	global_error[80];

extern char	*neogeo_rom_memory __attribute__((aligned(64)));
extern char	*neogeo_prg_memory __attribute__((aligned(64)));
extern char	*neogeo_fix_memory __attribute__((aligned(64)));
extern char	*neogeo_spr_memory __attribute__((aligned(64)));
extern char	*neogeo_pcm_memory __attribute__((aligned(64)));

extern unsigned char neogeo_memorycard[8192] __attribute__((aligned(64))); ;

extern char path_prefix[128] __attribute__((aligned(64)));

extern int      neogeo_ipl_done;

extern 		uint32 neocd_time;

void loadModules(void);

#endif /* NEOCD_H */
