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
 
#include <kernel.h>
#include <tamtypes.h>
#include "timer.h"

#define T0_COUNT ((u32 *)(0x10000000))
#define T0_MODE ((u32 *)(0x10000010))
#define T_MODE_CLKS_M 3
#define T_MODE_CUE_M 128

// to be set
int unsigned vsync_freq = GETTIME_FREQ_PAL; 

static unsigned lastinterval;

unsigned timer_gettime(void) {
  return (*T0_COUNT) & 0xFFFF;
}

void timer_init(void) {
  *T0_MODE = T_MODE_CLKS_M | T_MODE_CUE_M;
  *T0_COUNT = 0;
  lastinterval = timer_gettime();
}

void timer_exit(void) { }

unsigned timer_getinterval(unsigned freq) {
  unsigned tickspassed,ebx,blocksize;
  ebx=(timer_gettime()-lastinterval) & 0xFFFF;
  blocksize=vsync_freq/freq;
  ebx+=vsync_freq%freq;
  tickspassed=ebx/blocksize;
  ebx-=ebx%blocksize;
  lastinterval+=ebx;
  return tickspassed;
}
