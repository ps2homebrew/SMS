/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright 2001-2004, ps2dev - http://www.ps2dev.org
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id$
# EE TSR to init debug printf v1.0
#
*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <fileio.h>
#include <malloc.h>
#include <libmc.h>
#include <debug.h>
#include <string.h>
#include <debug.h>
#include <stdio.h>
#include <fileio.h>
#include <sys\stat.h>


#define tnop asm volatile (	"nop\n" );


// ELF-header structures and identifiers
#define ELF_MAGIC	0x464c457f
#define ELF_PT_LOAD	1

typedef struct
{
	u8	ident[16];
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
	} elf_header_t;

typedef struct
{
	u32	type;
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
	} elf_pheader_t;



char *ElfMem;
char *ElfMem2;
u32	ElfBaseAddr;
u32	ElfBaseSize;

u32	ElfStartAddr;
int fileMode =  FIO_S_IRUSR | FIO_S_IWUSR | FIO_S_IXUSR | FIO_S_IRGRP | FIO_S_IWGRP | FIO_S_IXGRP | FIO_S_IROTH | FIO_S_IWOTH | FIO_S_IXOTH;

void LoadElfToMem( char *path )
{
	int fd, size, i;
	printf("Open file %s\n",path);

	if( (fd = fioOpen(path, O_RDONLY)) < 0 )
	{
		printf("Failed in fioOpen %s\n",path);
		goto error;
	}

	printf("Opened file %s\n",path);
	size = fioLseek(fd, 0, SEEK_END);
	printf("File size = %i\n",size);
	if (!size)
	{
		printf("Failed in fioLseek\n");
		fioClose(fd);
		goto error;
	}

	ElfBaseSize=size+4;
	ElfMem= malloc( ElfBaseSize );
	ElfMem2= malloc( ElfBaseSize );

	u8 *boot_elf = ElfMem;
	elf_header_t *eh = (elf_header_t *)boot_elf;
	elf_pheader_t *eph;

	fioLseek(fd, 0, SEEK_SET);
	fioRead(fd, boot_elf, 52);
	printf("Read elf header from file\n");
	fioLseek(fd, eh->phoff, SEEK_SET);
	eph = (elf_pheader_t *)(boot_elf + eh->phoff);
	size=eh->phnum*eh->phentsize;
	size=fioRead(fd, (void *)eph, size);
	printf("Read %i bytes of program header(s) from file\n",size);
/*	for (i = 0; i < eh->phnum; i++)
	{
		if (eph[i].type != ELF_PT_LOAD)
		continue;
*/
		i=0;
		fioLseek(fd, eph[i].offset, SEEK_SET);
		size=eph[i].filesz;
	//	size=fioRead(fd, eph[i].vaddr, size);
		size=fioRead(fd, ElfMem2, size);
		ElfBaseSize=size;ElfBaseAddr=(u32)eph[i].vaddr;
		printf("Read %i bytes to %x\n", size, (u32)eph[i].vaddr);
		if (eph[i].memsz > eph[i].filesz)
			memset(eph[i].vaddr + eph[i].filesz, 0,
					eph[i].memsz - eph[i].filesz);
//	}		

	fioClose(fd);

	if (_lw((u32)&eh->ident) != ELF_MAGIC)		// this should have already been
	{								// done by menu, but a double-check
		printf("Not a recognised ELF.\n");	// doesn't do any harm
		goto error;
		}
	
	printf("entry=%x\n",eh->entry);
	ElfStartAddr=eh->entry;
	return;
error:
	ElfStartAddr=0;
	return;}



typedef u32 (*funcptr_ten_ret32)();
typedef void (*funcptr_ten_get32)( u32 * ) ;

/*
#define inject( c, func )  ((( (0x08000000+c*0x04000000) + (		(	( ((u32)func)-TEN_START) + (TEN_START-0x00100000) ) >>2) )))
*/

int main() 
{
	
/* wont need this
	SifInitRpc(0);
	sbv_patch_enable_lmb();			
	sbv_patch_disable_prefix_check();
*/

	printf( "loading %s...\n", "host:tentsr_worker.elf" );
	LoadElfToMem( "host:tentsr_worker.elf" );

	printf( "Area %x %x\n",ElfBaseAddr,ElfBaseSize );
	printf( "Orig %x %x\n",(u32)ElfMem,(u32)ElfMem2 );
	printf( "Start %x\n",(u32)ElfStartAddr );



//cpy elf into ram
	FlushCache(0);
	FlushCache(2);
	iFlushCache(0);
	iFlushCache(2);
	ee_kmode_enter();
	{
		u32 *s1,*s2,*d1;
		s1=(u32*)ElfMem2;
		s2=(u32*)(ElfMem2+ElfBaseSize);
		d1=(u32*)ElfBaseAddr;

		while( s1<s2 )
		{
			*d1=*s1;

			s1++;
			d1++;
		}
	}

//exit kmode and flush cache, just to make sure proggy is stored correctly
	ee_kmode_exit();
	FlushCache(0);
	FlushCache(2);
	iFlushCache(0);
	iFlushCache(2);






//kernel mode again, to call proggy & setup injects
	FlushCache(0);
	FlushCache(2);
	iFlushCache(0);
	iFlushCache(2);
	ee_kmode_enter();

	u32 _tenfunc;
	funcptr_ten_get32 fptr=(funcptr_ten_get32)ElfStartAddr;
	tnop
	fptr( &_tenfunc );
	tnop

	//last reboot printf -> tenfunc
	*((u32*)0xa000dca4)=0x08000000 | _tenfunc>>2;
	//last reboot no mc printf -> tenfunc
	*((u32*)0xa000dd9c)=0x08000000 | _tenfunc>>2;

	ee_kmode_exit();
	FlushCache(0);
	FlushCache(2);
	iFlushCache(0);
	iFlushCache(2);


	printf( "_tenfunc %8.8x %8.8x\n",_tenfunc, 0x08000000 | _tenfunc>>2 );
	printf( "Done.\n" );



	free( ElfMem );
	free( ElfMem2 );

	LoadExecPS2( "cdrom0:\\SLUS_203.12", 0, 0 );

	return 0;
}
