/*
 *  timer.c
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

#ifndef __TIMER_H
#define __TIMER_H

/*
	Very high-resolution timer stuff.
	Last update: 09-29-2000

	Last addition is timer_getinterval, a function to measure a short
	interval in a specified frequency. This function compensates for
	any and all rounding errors, making it extremely precise!

	Valid frequencies are 1 through 1193181.

*/


// cpu frequency
#define GETTIME_FREQ_PAL  15625
#define GETTIME_FREQ_NTSC 15734 

void timer_init(void);

void timer_exit(void);

unsigned timer_gettime(void);

unsigned timer_getinterval(unsigned freq);

#endif

