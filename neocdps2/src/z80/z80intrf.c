/************************
*** Z80 CPU Interface ***
************************/

//-- Include Files ----------------------------------------------------------
#include	"../neocd.h"
#include 	<stdio.h>
#include 	<stdlib.h>
#include	"mz80.h"


//-- Exported Functions -----------------------------------------------------
void	PortWrite(UINT16 PortNo, UINT8 data, struct z80PortWrite *ptr);
UINT16	PortRead(UINT16 PortNo, struct z80PortRead *ptr);

//-- Structures -------------------------------------------------------------
struct	z80PortRead ReadPorts[] =
{
	{0x0000,	0xffff,		PortRead,	NULL},
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

struct	z80PortWrite WritePorts[] =
{
	{0x0000,	0xffff,		PortWrite,	NULL},
	{(UINT16)-1,	(UINT16)-1,	NULL,		NULL}
};

struct MemoryReadByte	MemRead[] =
{
	{0xFFFF0000,	0xffffffff,	NULL,		NULL},
	{(UINT32)-1,	(UINT32)-1,	NULL,		NULL}
};

struct MemoryWriteByte	MemWrite[] =
{
	{0xFFFF0000,	0xffffffff,	NULL,		NULL},
	{(UINT32)-1,	(UINT32)-1,	NULL,		NULL}
};

//-- Variables --------------------------------------------------------------
CONTEXTMZ80	subcpu_context;
UINT8		subcpu_memspace[65536];// __attribute__((aligned(16)));
int		sound_code = 0;
int		pending_command = 0;
int		result_code = 0;
int		z80_cycles = Z80_VBL_CYCLES;

//---------------------------------------------------------------------------
void z80_init(void)
{
	subcpu_context.z80Base = subcpu_memspace;
	
	subcpu_context.z80IoRead  = ReadPorts;
	subcpu_context.z80IoWrite = WritePorts;
	
	subcpu_context.z80MemRead = MemRead;
	subcpu_context.z80MemWrite = MemWrite;
	
	mz80SetContext((void *)&subcpu_context);
	
	mz80init();
	mz80reset();

	// Let Z80 do its initialization
	mz80exec(100000);
}

//---------------------------------------------------------------------------
void PortWrite(UINT16 PortNo, UINT8 data, struct z80PortWrite *ptr)
{
	// sound routines broken... so returns directly...
	return;
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
UINT16 PortRead(UINT16 PortNo, struct z80PortRead *ptr)
{
	static int bank[4];
	// sound routines broken... so returns directly...
	return 0;
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
	default:
		//printf("Unimplemented Z80 Read Port: %d\n",PortNo&0xff);
		break;
	};	
	return 0;
}
