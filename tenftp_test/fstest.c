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
# libmc API sample.
*/

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <fileio.h>
#include <malloc.h>
#include <libmc.h>
#include <debug.h>


#define FTP_PATH "ftp://ps2:saves_ten852@192.168.0.192:1705/"
/*
#define init_scr() {}
#define scr_printf( ... )  {}
*/
void LoadModules(void);

int main() {

	int fd, ret;
	char tmp[256];
	fio_dirent_t dirent;
	char data[256]="Test Data for Read/Write Test !\n2nd Line\n3rd";

	// Initialise
	SifInitRpc(0);
	LoadModules();

	init_scr();


	scr_printf( "dir read test:\n" );
	fd = fioDopen( FTP_PATH "" );
	if( fd <=0 )
	{
		scr_printf("fioDopen error\nret:%d",fd);
	}
	while( (ret=fioDread( fd, &dirent ))>0 )
	{
		scr_printf( "%s | %d | %d\n",dirent.name, dirent.stat.mode, dirent.stat.size );		
	}
	if( ret < 0 )
		scr_printf("fioDread error\nret:%d",ret);
	fioDclose( fd );
	scr_printf( "done\n" );



	scr_printf( "mkdir test\n" );
	ret=fioMkdir( FTP_PATH "test_dir" );
	if( ret < 0 )
		scr_printf("fioMkdir error\nret:%d",ret);
	scr_printf( "done\n" );


	scr_printf( "dir read test of /test_dir/:\n" );
	fd = fioDopen( FTP_PATH "test_dir/" );
	if( fd <=0 )
	{
		scr_printf("fioDopen error\nret:%d",fd);
	}
	while( (ret=fioDread( fd, &dirent ))>0 )
	{
		scr_printf( "%s | %d | %d\n",dirent.name, dirent.stat.mode, dirent.stat.size );		
	}
	if( ret < 0 )
		scr_printf("fioDread error\nret:%d",ret);
	fioDclose( fd );
	scr_printf( "done\n" );



	scr_printf( "rmdir test\n" );
	ret=fioRmdir( FTP_PATH "test_dir/" );
	if( ret < 0 )
		scr_printf("fioRmdir error\nret:%d",ret);
	scr_printf( "done\n" );


	

	scr_printf( "file write test\n" );
	fd = fioOpen( FTP_PATH "testfile.txt", O_CREAT | O_WRONLY);
	if(fd <= 0) {
		scr_printf("fioOpen error\nret:%d",fd);
	} 
	else 
	{
		if( (ret=fioWrite(fd, data, strlen(data))) <=0 ) 
			scr_printf("fioWrite error\nret:%d",ret);
		fioClose(fd);
	}
	scr_printf( "done\n" );



	scr_printf( "dir read test:\n" );
	fd = fioDopen( FTP_PATH "" );
	if( fd <=0 )
	{
		scr_printf("fioDopen error\nret:%d",fd);
	}
	while( (ret=fioDread( fd, &dirent ))>0 )
	{
		scr_printf( "%s | %d | %d\n",dirent.name, dirent.stat.mode, dirent.stat.size );		
	}
	if( ret < 0 )
		scr_printf("fioDread error\nret:%d",ret);
	fioDclose( fd );
	scr_printf( "done\n" );


	
	scr_printf( "file read test\n" );
	fd = fioOpen( FTP_PATH "testfile.txt", O_RDONLY);
	if(fd <= 0) {
		scr_printf("fioOpen error\nret:%d",fd);
	} 
	else 
	{
		if( (ret=fioRead(fd, tmp, strlen(data))) <=0 ) 
			scr_printf("fioRead error\nret:%d",ret);
		
		if( strcmp( tmp, data ) )
			scr_printf( "Written vs. Read data difference.\n" );

		fioClose(fd);
	}
	scr_printf( "done\n" );





	// Return to the browser
	SifExitRpc();
	SleepThread();

	return 1;
}

/*
#include "intrman.h"
#include "iomanX.h"
#include "ps2ip.h"
#include "sifcmd.h"
#include "sifman.h"
#include "stdio.h"
#include "sysclib.h"
#include "sysmem.h"
#include "thbase.h"
#include "dns.h"
#include "ioman_mod.h"
*/

void LoadModules(void)
{
    int ret;

	ret = SifLoadModule("rom0:XSIO2MAN", 0, NULL);
	if (ret < 0) {
		scr_printf("Failed to load module: SIO2MAN");
//		SleepThread();
	}

	ret = SifLoadModule("rom0:IOMANX", 0, NULL);
	if (ret < 0) {
		scr_printf("Failed to load module: IOMANX");
//		SleepThread();
	}

	ret = SifLoadModule("rom0:SIFCMD", 0, NULL);
	if (ret < 0) {
		scr_printf("Failed to load module: SIFCMD");
//		SleepThread();
	}

	ret = SifLoadModule("rom0:SIFMAN", 0, NULL);
	if (ret < 0) {
		scr_printf("Failed to load module: SIFMAN");
//		SleepThread();
	}

	ret = SifLoadModule("host:tenftp.irx", 0, NULL);
	if (ret < 0) {
		scr_printf("Failed to load module: tenfs.irx");
//		SleepThread();
	}

}
