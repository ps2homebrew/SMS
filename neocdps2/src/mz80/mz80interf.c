/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#ifdef USE_MAMEZ80

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include "emu.h"
//#include "memory.h"
#include "../neocd.h" 
#include "z80.h"
//#include "state.h"
#include "mz80interf.h"

//static uint8 *z80map1, *z80map2, *z80map3, *z80map4;

//static uint16 z80_bank[4]; 

uint8 mame_z80mem[Z80_MEMSIZE]   __attribute__((aligned(16)));

int		sound_code;
int		pending_command;
int		result_code;

#if 0
/* Memory and port IO handler */
void mame_z80_writemem16(uint16 addr, uint8 val)
{
    //  printf("Writemem %x=%x\n",addr,val);
    //if (addr >= 0xf800)
//	memory.z80_ram[addr - 0xf800] = val;
    
    mame_z80mem[addr] = val;

}

uint8 mame_z80_readmem16(uint16 addr)
{
    /*
    if (addr <= 0x7fff)
	return memory.sm1[addr];
    if (addr <= 0xbfff)
	return z80map1[addr - 0x8000];
    if (addr <= 0xdfff)
	return z80map2[addr - 0xc000];
    if (addr <= 0xefff)
	return z80map3[addr - 0xe000];
    if (addr <= 0xf7ff)
	return z80map4[addr - 0xf000];
    return memory.z80_ram[addr - 0xf800];
    */
    return mame_z80mem[addr];
}


uint8 mame_z80_readop(uint16 addr)
{
    return mame_z80_readmem16(addr);
}

uint8 mame_z80_readop_arg(uint16 addr)
{
    return mame_z80_readmem16(addr);
}
#endif

void mame_z80_writeport16(uint16 port, uint8 value)
{
    //printf("Write port %d=%d\n",port,value);
    z80_port_write(port, value);
}

uint8 mame_z80_readport16(uint16 port)
{
    //printf("Read port %d\n",port);
    return z80_port_read(port);
}


/* cpu interface implementation */
/*

void cpu_z80_switchbank(uint8 bank, uint16 PortNo)
{
    if (bank<=3)
	z80_bank[bank]=PortNo;

    switch (bank) {
    case 0:
	z80map1 = memory.sm1 + (0x4000 * ((PortNo >> 8) & 0x0f));
	memcpy(mame_z80mem + 0x8000, z80map1, 0x4000);
	break;
    case 1:
	z80map2 = memory.sm1 + (0x2000 * ((PortNo >> 8) & 0x1f));
	memcpy(mame_z80mem + 0xc000, z80map2, 0x2000);
	break;
    case 2:
	z80map3 = memory.sm1 + (0x1000 * ((PortNo >> 8) & 0x3f));
	memcpy(mame_z80mem + 0xe000, z80map3, 0x1000);
	break;
    case 3:
	z80map4 = memory.sm1 + (0x0800 * ((PortNo >> 8) & 0x7f));
	memcpy(mame_z80mem + 0xf000, z80map4, 0x0800);
	break;
    }
}
*/
/*
int mame_z80_irq_callback(int a)
{
    return 0;
}
*/


void cpu_z80_init(void)
{
    //  init_mamez80_mem();
    z80_init();

    /* bank initalisation */
    /*
    z80map1 = memory.sm1 + 0x8000;
    z80map2 = memory.sm1 + 0xc000;
    z80map3 = memory.sm1 + 0xe000;
    z80map4 = memory.sm1 + 0xf000;

    z80_bank[0]=0x8000;
    z80_bank[1]=0xc000;
    z80_bank[2]=0xe000;
    z80_bank[3]=0xf000;
    */
    //memcpy(mame_z80mem, memory.sm1, 0xf800);
    z80_reset(NULL);
    //set_irq_callback(mame_z80_irq_callback);
    //z80_init_save_state();
}

inline void cpu_z80_run(int nbcycle)
{
    //printf("%x\n",z80_get_reg(Z80_PC));
    z80_execute(nbcycle);
}
inline void cpu_z80_nmi(void)
{
    set_irq_line(INPUT_LINE_NMI, ASSERT_LINE);
    set_irq_line(INPUT_LINE_NMI, CLEAR_LINE);
}
inline void cpu_z80_raise_irq(int l)
{
    set_irq_line(l, ASSERT_LINE);
}
inline void cpu_z80_lower_irq(void)
{
    set_irq_line(0, CLEAR_LINE);
}

inline uint16 cpu_z80_get_pc(void)
{
    return 0;
}

/* Z80 IO port handler */
inline uint8 z80_port_read(uint16 PortNo)
{
//    printf("z80_port_read PC=%04x p=%04x ",cpu_z80_get_pc(),PortNo);
    switch (PortNo & 0xff) 
    {
    case 0x0:
	pending_command = 0;
	return sound_code;
	break;

    case 0x4:
	//printf("v=%02x\n",YM2610_status_port_0_A_r(0));
	return YM2610_status_port_0_A_r(0);
	break;

    case 0x5:
	//printf("v=%02x\n",YM2610_read_port_0_r(0));
	return YM2610_read_port_0_r(0);
	break;

    case 0x6:
	//printf("v=%02x\n",YM2610_status_port_0_B_r(0));
	return YM2610_status_port_0_B_r(0);
	break;
/*
    case 0x08:
	//printf("v=00 (sb3)\n");
	//cpu_z80_switchbank(3, PortNo);
	return 0;
	break;

    case 0x09:
	//printf("v=00 (sb2)\n");
	//cpu_z80_switchbank(2, PortNo);
	return 0;
	break;

    case 0x0a:
	//printf("v=00 (sb1)\n");
	//cpu_z80_switchbank(1, PortNo);
	return 0;
	break;

    case 0x0b:
	//printf("v=00 (sb0)\n");
	//cpu_z80_switchbank(0, PortNo);
	return 0;
	break;
*/
    default: break; 
    };

    return 0;
}

inline void z80_port_write(uint16 PortNb, uint8 value)
{
    //uint8 data = Value;
    //printf("z80_port_write PC=%04x OP=%02x p=%04x v=%02x\n",cpu_z80_get_pc(),memory.sm1[cpu_z80_get_pc()],PortNb,Value);
    switch (PortNb & 0xff) 
    {
    case 0x4:
	YM2610_control_port_0_A_w(0, value); //data
	break;

    case 0x5:
	YM2610_data_port_0_A_w(0, value);//data
	break;

    case 0x6:
	YM2610_control_port_0_B_w(0, value);//data
	break;

    case 0x7:
	YM2610_data_port_0_B_w(0, value);//data
	break;

    case 0xC:
	result_code = value;
	break;
    
    default: break; 
    }
}
 
#endif
