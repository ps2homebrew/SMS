/*
 *  cdrom.h
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
 */

#ifndef	CDROM_H
#define CDROM_H

#ifdef LOWERCASEFILES
 #define CHANGECASE  tolower
 #define IPL_TXT  "ipl.txt"
 #define PRG      "prg"
 #define FIX      "fix"
 #define SPR      "spr"
 #define OBJ      "obj"
 #define Z80      "z80"
 #define PAT      "pat"
 #define PCM      "pcm"
 #define JUE      "jue"
 #define TITLE_X_SYS "title_x.sys"

#else
 #define CHANGECASE  toupper
 #define IPL_TXT  "IPL.TXT"
 #define PRG      "PRG"
 #define FIX      "FIX"
 #define SPR      "SPR"
 #define OBJ      "OBJ"
 #define Z80      "Z80"
 #define PAT      "PAT"
 #define PCM      "PCM"
 #define JUE      "JUE"
 #define TITLE_X_SYS "TITLE_X.SYS"
#endif

/*-- cdrom.c functions ------------------------------------------------------*/
int	cdrom_init1(void);
int	cdrom_load_prg_file(char *, unsigned int);
int	cdrom_load_z80_file(char *, unsigned int);
int	cdrom_load_fix_file(char *, unsigned int);
int	cdrom_load_spr_file(char *, unsigned int);
int	cdrom_load_pcm_file(char *, unsigned int);
int	cdrom_load_pat_file(char *, unsigned int, unsigned int);
int	cdrom_process_ipl(void);
void	cdrom_shutdown(void);
void	cdrom_load_title(void);

void	fix_conv(unsigned char *, unsigned char *, int, unsigned char *);
void	spr_conv(unsigned char *, unsigned char *, int, unsigned char *);

/*-- extract8.asm functions -------------------------------------------------*/
void		extract8(char *, char *);
unsigned int	motorola_peek(unsigned char*);


#endif /* CDROM_H */
