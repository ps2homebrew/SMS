/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

  
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "fm.h"
#include "2610intf.h"
#include "sound.h"
#include "sjpcm.h" 
#include "../neocd.h"


int16 *sndbuffer[2] __attribute__((aligned(64)));

int16 ps2_soundbuf[2][960] __attribute__((aligned(64)))  __attribute__ ((section (".bss"))); 

int init_audio(void)
{

   if(SjPCM_Init(1) < 0)
   { 
   	printf("SjPCM Bind failed!!");
	return -1;
   } 
   
   //streams_sh_start();
   YM2610_sh_start();
   
   SjPCM_Clearbuff();
   SjPCM_Setvol(0x3fff); // max
   
   if (neocdSettings.soundOn)
   	SjPCM_Play();
      
   // into 16kb scratch pad @ 70001000 (why not?)
   sndbuffer[0] = (int16 *)(0x70001000);
   sndbuffer[1] = (int16 *)(0x70001800);
   
   // Sound output 
   //sndbuffer[0] = (signed short int *)memalign(64, machine_def.snd_sample * 2);
   //sndbuffer[1] = (signed short int *)memalign(64, machine_def.snd_sample * 2); 
   //memset(&sndbuffer[0], 0, machine_def.snd_sample * 2); 
   //memset(&sndbuffer[1], 0, machine_def.snd_sample * 2); 
   

   memset(&ps2_soundbuf[0][0], 0, machine_def.snd_sample * 2); 
   memset(&ps2_soundbuf[1][0], 0, machine_def.snd_sample * 2); 

   
   printf("audio started\n");   
   
   return 0;
}

// this is a hack to avoid 48Khz sound emulation needed by sjpcm
// it's awfull, but still faster that emulate sound at such a high frequency !!
static inline void up_samples(int16 *sleft, int16 *sright, int16 *dleft, int16 *dright, int dest_samples, int orig_samples) 
{
	register int i;
	//int ratio = dest_samples/orig_samples; -> 4 !!
	for(i=dest_samples;i--;)
	{
		*dleft++ = *sleft;
		*dright++ = *sright;
		if(!(i%4/*ratio*/) && i){sleft++;sright++;}
	}
}
 

void play_audio(void)
{
     YM2610UpdateOne(sndbuffer, (machine_def.snd_sample >> 2));
     up_samples(&sndbuffer[0][0], &sndbuffer[1][0], &ps2_soundbuf[0][0], &ps2_soundbuf[1][0], machine_def.snd_sample, (machine_def.snd_sample >> 2)); 
     SjPCM_Enqueue(ps2_soundbuf[0],ps2_soundbuf[1], machine_def.snd_sample,0);
}

void sound_toggle(void) 
{
	// pause Audio ...
	neocdSettings.soundOn ^= 1;
	if (neocdSettings.soundOn) SjPCM_Play();
	else SjPCM_Pause();
}





