/*
 *  z80intrf.h - Z80 Interface
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

#ifndef	Z80INTRF_H
#define Z80INRTF_H

#ifdef CPUZ80_MAMEZ80_CORE

#include "mamez80/z80.h"
#define _z80exec z80_execute
#define _z80nmi() z80_set_irq_line(INPUT_LINE_NMI, ASSERT_LINE);z80_set_irq_line(INPUT_LINE_NMI, CLEAR_LINE)
#define _z80raise(VEC) z80_set_irq_line(VEC, ASSERT_LINE)
#define _z80lower() z80_set_irq_line(0, CLEAR_LINE)
#define _z80reset() z80_reset(NULL)

//extern uint16 z80_bank[4];

#endif

#ifdef CPUZ80_CZ80_CORE

#include "cz80/cz80.h"
extern cz80_struc neocd_cz80_struc;
#define _z80raise(VEC) Cz80_Set_IRQ(&neocd_cz80_struc, VEC)
#define _z80lower() Cz80_Clear_IRQ(&neocd_cz80_struc)
#define _z80exec(CIC) Cz80_Exec(&neocd_cz80_struc,CIC)
#define _z80nmi() Cz80_Set_NMI(&neocd_cz80_struc)
#define _z80reset() Cz80_Reset(&neocd_cz80_struc)

#endif


void _z80_init(void);

void mame_z80_writeport16(uint16 PortNo, uint8 data);
uint8 mame_z80_readport16(uint16 PortNo);

#define Z80_MEMSIZE 0x10000

extern uint8 mame_z80mem[Z80_MEMSIZE];
#define subcpu_memspace mame_z80mem

extern uint32 sound_code;
extern uint32 result_code;
extern uint32 pending_command;

#endif /* Z80INTRF_H */

