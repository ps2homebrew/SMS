/*
 *  pd4990a.h - Emulation for the NEC PD4990A.
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
 *
 */

#ifndef _PD4990A_H
#define _PD4990A_H

struct pd4990a_s
{
	int seconds;
	int minutes;
	int hours;
	int days;
	int month;
	int year;
	int weekday;
};

extern struct pd4990a_s pd4990a;

void pd4990a_addretrace (void);
void pd4990a_init(int fps_rate);
int pd4990a_testbit_r (void);
int pd4990a_databit_r (void);
void pd4990a_control_w(unsigned short);
void pd4990a_control_16_w (int,int);
void pd4990a_increment_day(void);
void pd4990a_increment_month(void);

#endif /* _PD4990A_H */

