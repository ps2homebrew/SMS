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
 

#ifndef _TIMER_H_
#define _TIMER_H_

/*-- Defines ---------------------------------------------------------------*/
#define PAL_MODE 50
#define NTSC_MODE 60 

typedef struct timer_struct {
    float time;		// when
    unsigned int odo_debut;
    unsigned int nb_cycle;
    int param;
    unsigned int del_it;
    void (*func) (int param);
    struct timer_struct *next;
} timer_struct;

extern float timer_counta;

timer_struct *insert_timer(float duration, int param, void (*func) (int));
void del_timer(timer_struct * ts);
void my_timer(void);
float timer_get_time(void);
void free_all_timer(void);

#endif
