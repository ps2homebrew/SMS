/*
 *  defines.h - PS2 Specific defines
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
#ifndef _DEFINES_H
#define _DEFINES_H

#define SUCCESS		1
#define FAILURE		0


typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned int		uint32;
typedef unsigned long int	uint64;
/*
typedef unsigned int		uint; 
*/

#ifdef _EE
typedef unsigned int		uint128 __attribute__(( mode(TI) ));
#endif

typedef char			int8;
typedef short			int16;
typedef int			int32;
typedef long int		int64;

#ifdef _EE
typedef int			int128 __attribute__(( mode(TI) ));
#endif 
/*
#ifndef NULL
#define NULL	(void *)0
#endif 
*/

#endif // DEFINES_H
