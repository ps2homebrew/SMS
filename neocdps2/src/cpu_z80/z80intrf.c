/*
 *  z80intrf.c - Z80 Interface
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

#include 	<stdio.h>
#include 	<stdlib.h>
#include	"../neocd.h"
//#include	"z80intrf.h"


#ifdef CPUZ80_CZ80_CORE

cz80_struc neocd_cz80_struc;

static u32 cpu_readmem8(u32 address)
{
	//printf ("readmem8 %x _at_ %x \n",mame_z80mem[address&0xFFFF], (address&0xFFFF));
	return (mame_z80mem[(u16)address&0xFFFF]);
}

#if CZ80_USE_WORD_HANDLER
static unsigned short cpu_readmem16(u32 address)
{
	return cpu_readmem8(address) | (cpu_readmem8(address + 1) << 8);
}
#endif

static void cpu_writemem8(u32 address, u32 data)
{
	//printf ("readmem8 %x _at_ %x \n",data, (address&0xFFFF));
	mame_z80mem[(u16)address&0xFFFF]=(u8)data;
}

#if CZ80_USE_WORD_HANDLER
static void cpu_writemem16(unsigned int address, unsigned int data)
{
	cpu_writemem8(address, data & 0xFF);
	cpu_writemem8(address + 1, data >> 8);
}
#endif
#endif


uint8 mame_z80mem[Z80_MEMSIZE]  __attribute__((aligned(32)));
uint32 sound_code;
uint32 result_code;
uint32 pending_command;

/*
#ifdef CPUZ80_MAMEZ80_CORE
uint16 z80_bank[4];
static uint8 *z80map1, *z80map2, *z80map3, *z80map4;
#endif
*/

int cpu_z80_irq_callback(int a)
{
	neogeo_sound_irq(a);
	return 0;
}



void _z80_init(void)
{
#ifdef CPUZ80_CZ80_CORE
	Cz80_Init(&neocd_cz80_struc);
	Cz80_Set_Fetch(&neocd_cz80_struc,0x0000,0xFFFF,(u32)((void *)&mame_z80mem));
	Cz80_Set_ReadB(&neocd_cz80_struc,&cpu_readmem8);
	Cz80_Set_WriteB(&neocd_cz80_struc,&cpu_writemem8);
#if CZ80_USE_WORD_HANDLER
	Cz80_Set_ReadW(&neocd_cz80_struc,&cpu_readmem16);
	Cz80_Set_WriteW(&neocd_cz80_struc,&cpu_writemem16);
#endif
	Cz80_Set_INPort(&neocd_cz80_struc,(CZ80_READ *)&mame_z80_readport16);
	Cz80_Set_OUTPort(&neocd_cz80_struc,(CZ80_WRITE *)&mame_z80_writeport16);
	Cz80_Set_IRQ_Callback(&neocd_cz80_struc,cpu_z80_irq_callback);
	Cz80_Reset(&neocd_cz80_struc);
	//Cz80_Exec(&neocd_cz80_struc,100000);
#endif
#ifdef CPUZ80_MAMEZ80_CORE
	z80_init();
	/*
	z80map1 = mame_z80mem + 0x8000;
	z80map2 = mame_z80mem + 0xc000;
	z80map3 = mame_z80mem + 0xe000;
	z80map4 = mame_z80mem + 0xf000;
	z80_bank[0]=0x8000;
	z80_bank[1]=0xc000;
	z80_bank[2]=0xe000;
	z80_bank[3]=0xf000;
	*/
	z80_reset(NULL);
	//z80_set_irq_callback(cpu_z80_irq_callback);
#endif
}

//---------------------------------------------------------------------------
#ifdef CPUZ80_MAMEZ80_CORE
inline void mame_z80_writeport16(uint16 PortNo, uint8 data)
#else
void mame_z80_writeport16(uint16 PortNo, uint8 data)
#endif
{
	//printf ("write _port_ %x _data_ %x \n",(PortNo & 0xff), data);
	
	switch( PortNo & 0xff)
	{
	case	0x4:
		YM2610_control_port_0_A_w(0,data);
		break;

	case	0x5:
		YM2610_data_port_0_A_w(0,data);
		break;

	case	0x6:
		YM2610_control_port_0_B_w(0,data);
		break;

	case	0x7:
		YM2610_data_port_0_B_w(0,data);
		break;


	case	0x8:
		/* NMI enable / acknowledge? (the data written doesn't matter) */
		break;

	case	0xc:
		result_code = data;
		break;
	case	0x18:
		/* NMI disable? (the data written doesn't matter) */
		break;
	default:
		//printf("Unimplemented Z80 Write Port: %x data: %x\n",PortNo&0xff,data);
		break;
	}
}

//---------------------------------------------------------------------------

#ifdef CPUZ80_MAMEZ80_CORE
inline uint8 mame_z80_readport16(uint16 PortNo)
#else
uint8 mame_z80_readport16(uint16 PortNo)
#endif
{
	//static int bank[4];
	
	//printf ("read _port_ %x \n",(PortNo & 0xff));
	switch( PortNo & 0xff)
	{
	case	0x0:
		pending_command = 0;
		return sound_code;
		break;
	
	case	0x4:
		return YM2610_status_port_0_A_r(0);
		break;
	
	case	0x5:
		return YM2610_read_port_0_r(0);
		break;
	
	case	0x6:
		return YM2610_status_port_0_B_r(0);
		break;
/*
	case 0x08:
		{
		    	bank[3] = 0x0800 * ((PortNo >> 8) & 0x7f);
			return 0;
			break;
		}
	case 0x09:
		{
			bank[2] = 0x1000 * ((PortNo >> 8) & 0x3f);
			return 0;
			break;
		}	
	case 0x0a:
		{
			bank[1] = 0x2000 * ((PortNo >> 8) & 0x1f);
			return 0;
			break;
		}
	case 0x0b:
		{
			bank[0] = 0x4000 * ((PortNo >> 8) & 0x0f);
			return 0;
			break;
		}
*/
	default:
		//printf("Unimplemented Z80 Read Port: %d\n",PortNo&0xff);
		break;
	};	
	return 0;
}

