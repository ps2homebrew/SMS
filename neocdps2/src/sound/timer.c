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
 
 /*YOU USE THIS PROGRAM AT YOUR OWN RISK, I CANNOT BE HELD RESPONSIBLE FOR ANY LOSS OR DAMAGE CAUSED. 

THIS PROGRAM REQUIRES THE ORIGINAL SNK CD ROMS. THESE CD ROMS ARE OWNED BY AND COPYRIGHTED BY SNK. I CANNOT BE HELD RESPONSIBLE FOR ANY REACH OF THE COPYRIGHT. 

PLEASE DO NOT ASK ME TO SUPPLY YOU CD ROMS, OR BIOS IMAGE: I CANNOT HELP YOU. 
PLEASE DO NOT SEND ME ANY CD-ROM. 
DO NOT SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST. 

NEOCD IS FREE, SOURCE CODE IS FREE. SELLING IS NOT ALLOWED. 
YOU CANNOT PROVIDE NEOCD AND NEOGEO GAME SOFTWARE ON THE SAME PHYSICAL MEDIUM. 

YOU CAN REUSE SOURCE CODE AND TECHNICAL KNOWLEDGE AS LONG AS IT IS NOT FOR COMMERCIAL PURPOSES.
LPGL */

#include <stdlib.h>
#include <stdio.h>
#include "timer.h"
#include "../neocd.h"

#define MAX_TIMER 3

float timer_counta;
timer_struct *timer_list;
timer_struct timers[MAX_TIMER];
//int nb_interlace=256;
//int nb_timer=0;

float timer_get_time(void) {
    return timer_counta;
}

timer_struct *insert_timer(float duration, int param, void (*func) (int))
{
    int i;
    //for (i = 0; i < MAX_TIMER; i++) 
    for (i = MAX_TIMER; i--;) 
    {
	if (timers[i].del_it) {
	    timers[i].time = timer_counta + duration;
	    timers[i].param = param;
	    timers[i].func = func;
	    //printf("Insert_timer %d duration=%f param=%d\n",i,timers[i].time,timers[i].param);
	    timers[i].del_it = 0;
	    return &timers[i];
	}
    }
    //printf("YM2610: No timer free!\n");
    return NULL;		/* No timer free */
}

void free_all_timer(void) {
    int i;
    //for (i = 0; i < MAX_TIMER; i++) 
    for (i = MAX_TIMER; i--;) 
    {
	timers[i].del_it=1;
    }
}

void del_timer(timer_struct * ts)
{
    ts->del_it = 1;
}

static float inc;

void my_timer(void)
{
    static int init = 1;
    int i;

    if (init) 
    {
	init = 0;
	if (machine_def.vidsys == PAL_MODE)
	    inc = (float) (0.02 / 256);

	else
	    inc = (float) (0.01666 / 256);

	for (i = MAX_TIMER; i--;) 
	    timers[i].del_it = 1;
    }

    timer_counta += inc;		/* 16ms/20ms par frame */

    for (i = MAX_TIMER; i--;) 
    {
	if (timer_counta >= timers[i].time && timers[i].del_it == 0) 
	{
	    if (timers[i].func) timers[i].func(timers[i].param);
	    timers[i].del_it = 1;
	}
    }
}
