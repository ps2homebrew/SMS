/*
 *  mc.h - MC management
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

#ifndef	MC_H
#define MC_H 


#define SAVE_SIZE 8192

#define MC_SAVE_PATH "mc0:NEOCDPS2"
#define MC_SAVE_FILE "mc0:NEOCDPS2/neocd.save"
#define MC_CONF_FILE "mc0:NEOCDPS2/neocd.cnf"

extern unsigned char neogeo_memorycard[SAVE_SIZE] __attribute__((aligned(64))) __attribute__ ((section (".bss")));

void mc_initSave();
void mc_writeSave();

void mc_readSettings();
void mc_saveSettings();

#endif
