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
 

#include <stdlib.h>
#include <stdio.h>
#include "timer.h"
#include "neocd.h"

#define MAX_TIMER 3

float timer_counta;
timer_struct *timer_list;
timer_struct timers[MAX_TIMER];


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
