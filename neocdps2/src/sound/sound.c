/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
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
#include "neocd.h"


int16 *ps2_soundbuf[2] __attribute__((aligned(64)));

int init_audio(void)
{

   if(SjPCM_Init(1) < 0)
   { 
   	printf("SjPCM Bind failed!!");
	return -1;
   } 
   
   YM2610_sh_start();
   
   SjPCM_Clearbuff();
   SjPCM_Setvol(0x3fff); // max
   
   if (neocdSettings.soundOn)
   	SjPCM_Play();
      
   ps2_soundbuf[0] = (int16 *)memalign(64, machine_def.snd_sample * 2);
   ps2_soundbuf[1] = (int16 *)memalign(64, machine_def.snd_sample * 2);
   
   memset(&ps2_soundbuf[0][0], 0, machine_def.snd_sample); 
   memset(&ps2_soundbuf[1][0], 0, machine_def.snd_sample); 

   
   printf("audio started\n");   
   
   return 0;
}


// update sound for one frame
void play_audio(void)
{

     YM2610UpdateOne(ps2_soundbuf, machine_def.snd_sample);
     SjPCM_Enqueue(ps2_soundbuf[0],ps2_soundbuf[1], machine_def.snd_sample,0);
}

void sound_toggle(void) 
{
	// pause Audio ...
	neocdSettings.soundOn ^= 1;
	if (neocdSettings.soundOn) SjPCM_Play();
	else SjPCM_Pause();
}





