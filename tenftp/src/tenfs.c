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
# IOP filesystem driver v1.0
#
*/

#define DEBUG_T

#include "tenGlob.h"


IRX_ID(MODNAME, 1, 1);

// Host base location
static char base[100];

// FileIO structure.
static iop_io_device_t driver;

// Function array for fileio structure.
static iop_io_device_ops_t functions;

static int fd_global;



/*! \brief Store filedescriptor and get client filedescriptor.
 *  \ingroup tenftp 
 */
int fd_save( int fd, iop_io_file_t *f )
{
	f->unit = ++fd_global;
	f->privdata = (void *)fd;
	return f->unit;
}

/*! \brief Get real filedescriptor.
 *  \ingroup tenftp 
 */
int realfd( iop_io_file_t *f )
{
	return (int)f->privdata;
}

/*! \brief Dummy function, for where needed.
 *  \ingroup tenftp 
 */
int dummy()
{
#ifdef DEBUG_T
	printf("tenftp: dummy function called\n");
#endif
	return -5;
}

/*! \brief Initialise fs driver.
 *  \ingroup tenftp 
 */
int fd_initialize( iop_io_device_t *driver)
{
	printf("tenftp: initializing '%s' file driver.\n", driver->name );
	return 0;
}


int LastFtp=-1;


int TenFtp_Init( char *p1, char *p2, char *p3 )
{
#if DEBUG_LEVEL>=1
	printf( "TenFtp_Init: %s %s %s\n", p1, p2, p3 );
#endif
	return (LastFtp=GetFtpConn( p1, p2, p3 ));
}

int TenFtp_Open( char *name )
{
	char *PathBuf;
	char *FileBuf;
	int ret;
	
	int sConn = LastFtp;

	if( ten_strchr( name, '@' ) && name[0]=='/' && name[1]=='/' )
	{//its a addr/user/pw form
		char *c1,*c2,*c3;
		while( name[0]=='/' )
			name++;

		c1=ten_strchr( name, ':' );
		c1[0]=0;
		c1++;

		c2=ten_strchr( c1, '@' );
		c2[0]=0;
		c2++;

		c3=ten_strchr( c2, '/' );
		c3[0]=0;
		c3++;

		sConn = TenFtp_Init( c2/*address*/, name/*user*/, c1/*pass*/ );

		name = c3;
	}

	if( sConn < 0 )
		return -1;

	if( lFtpState[sConn].bBusy )
		return -1;

	lFtpState[sConn].bBusy=1;

	while( name[0]=='/' )
		name++;

	PathBuf=name;
	FileBuf=strrchr( name, '/' );

	if( FileBuf )
	{
		FileBuf[0]=0;
		FileBuf++;
	}
	else
	{//no path
		FileBuf=PathBuf;
		PathBuf="";
	}

#if DEBUG_LEVEL>=1
	printf( "PathBuf FileBuf %s %s ", PathBuf, FileBuf );
#endif

	//change dir:
	ret = FtpCommand( sConn, "CWD", "/" );
	if( ret != 250 )
		return -2;

	if( PathBuf[0] )
	{
		ret = FtpCommand( sConn, "CWD", PathBuf );
		if( ret != 250 )
			return -3;
	}

	if( strlen(FileBuf) )
	{//we want a file
		ret = FtpCommand( sConn, "TYPE", "I" );
		if( ret != 200 )
			return -4;
	}
	else
	{//we want a listing
		ret = FtpCommand( sConn, "TYPE", "A" );
		if( ret != 200 )
			return -5;
	}

	FtpDataConn( sConn );

	if( strlen(FileBuf) )
	{//we want a file
		ret = FtpCommand( sConn, "RETR", FileBuf );
		if( ret != 150 )
			return -4;
	}
	else
	{//we want a listing
		ret = FtpCommand( sConn, "LIST", "-al" );
		if( ret != 150 )
			return -5;
	}	
	return sConn;
}




/*! \brief Handle open request.
 *  \ingroup tenftp 
 */
int fd_open( iop_io_file_t *f, const char *name, int mode)
{
	char NameBuf[ 250 ]="";
	char cmd[ 250 ]="";
	char param1[ 250 ]="";
	char param2[ 250 ]="";
	char param3[ 250 ]="";
	char *c;
	char *c2;
	int bCommand=0;
	int ret;

	strcpy( NameBuf, name );

#if DEBUG_LEVEL>=1
	printf( "tenftp: open %i %s %s\n", f->unit, name, NameBuf);
#endif


	c=ten_strchr( NameBuf, '|' );
	if( c )
	{
#if DEBUG_LEVEL>=1
		printf( "c: %s\n", c);
		printf( "NameBuf: %s\n", NameBuf);
#endif
		bCommand=1;
		c[0]=0;
#if DEBUG_LEVEL>=1
		printf( "c: %s\n", c);
		printf( "NameBuf: %s\n", NameBuf);
#endif
		strcpy( cmd, NameBuf );
#if DEBUG_LEVEL>=1
		printf( "cmd: %s\n", cmd);
#endif

		c2=c+1;
		c=ten_strchr( c2, '|' );
		if( c )
		{
			c[0]=0;
			strcpy( param1, c2 );
#if DEBUG_LEVEL>=1
			printf( "p1: %s\n", param1);
#endif
			c2=c+1;
		}

		c=ten_strchr( c2, '|' );
		if( c )
		{
			c[0]=0;
			strcpy( param2, c2 );
#if DEBUG_LEVEL>=1
			printf( "p2: %s\n", param2);
#endif
			c2=c+1;
		}

		c=ten_strchr( c2, '|' );
		if( c )
		{
			c[0]=0;
			strcpy( param3, c2 );
#if DEBUG_LEVEL>=1
			printf( "p3: %s\n", param3);
#endif
			c2=c+1;
		}
	}

	if( bCommand )	
	{
#if DEBUG_LEVEL>=1
		printf( "tenftp: command %s %s %s %s\n", cmd, param1, param2, param3 );
#endif
		if( !strcmp( cmd, "init" ) )
			return TenFtp_Init( param1, param2, param3 );
		else
			return -1;
	}
	else
	{
#if DEBUG_LEVEL>=1
		printf( "tenftp: open %s\n", NameBuf );
#endif
		ret = TenFtp_Open( NameBuf );
		if( ret < 0 )
		{
			FtpClose( LastFtp );
			LastFtp=-1;
			return ret;
		}
		return fd_save( ret, f );
	}
}

/*! \brief Handle close request.
 *  \ingroup tenftp 
 *
 *  \param f     Pointer to io_device structure.
 *  \return Status (as for fileio close).
 *
 */
int fd_close( iop_io_file_t *f )
{
	int sConn = realfd(f);

	if( sConn < 0 )
		return -1;

	if( ! lFtpState[sConn].bBusy )
		return -2;

	FtpEmpty( sConn );

	lFtpState[sConn].bBusy=0;

	if( ! lFtpState[sConn].bData )
		return -3;

	FtpDataClose( sConn );

	return 1;
}

/*! \brief Handle read request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param buffer  Pointer to read buffer.
 *  \param size    Size of buffer.
 *  \return Status (as for fileio read).
 *
 */
int fd_read( iop_io_file_t *f, char * buffer, int size )
{
	int ret,sConn = realfd(f);

#if DEBUG_LEVEL>=1
	printf( "read: %p %d\n", buffer, size );
//	printf( "read sConn %d\n", sConn );
#endif

	if( sConn < 0 )
	{
		printf( "sConn < 0\n" );
		return -1;
	}

//	printf( "lFtpState[sConn].bBusy...\n" );
	if( ! lFtpState[sConn].bBusy )
	{
		printf( "! lFtpState[sConn].bBusy\n" );
		return -2;
	}

//	printf( "lFtpState[sConn].bData...\n" );
	if( ! lFtpState[sConn].bData )
	{
		printf( "! lFtpState[sConn].bData\n" );
		return -3;
	}

//	printf( "IsDataFromFtpDataConnAvail...\n" );
	if( !IsDataFromFtpDataConnAvail( sConn ) )
	{
		DelayThread( 500 );
		if( !IsDataFromFtpDataConnAvail( sConn ) )
			return 0;
	}

//	printf( "GetDataFromFtpDataConn...\n" );
	ret = GetDataFromFtpDataConn( sConn, buffer, size );
	if( ret < 0 )
	{
		printf( "! GetDataFromFtpDataConn\n" );
		return -4;
	}

#if DEBUG_LEVEL>=1
	printf( "read res: %d\n", ret );
#endif
	return ret; 
}

/*! \brief Handle write request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param buffer  Pointer to read buffer.
 *  \param size    Size of buffer.
 *  \return Status (as for fileio write).
 *
 */
int fd_write( iop_io_file_t *fd, void *buffer, int size )
{
	return write( realfd(fd), buffer, size );
}

/*! \brief Handle lseek request.
 *  \ingroup tenftp 
 *
 *  \param f       Pointer to io_device structure.
 *  \param offset  Offset for seek.
 *  \param whence  Base for seek.
 *  \return Status (as for fileio lseek).
 *
 */
int fd_lseek( iop_io_file_t *fd, unsigned long offset, int whence)
{
	return lseek( realfd(fd), offset, whence );
}

/*! \brief Entry point for IRX.
 *  \ingroup tenftp 
 *
 *  if argc != 2 , quit, as it needs parameter
 *  if argc == 2 , use arv[1] as basename.
 *
 *  \param argc Number of arguments.
 *  \param argv Pointer to array of arguments.
 *  \return Module Status on Exit.
 *
 *  This initialises the tenftp driver, setting up the 'host:' fs driver
 *  basename redirection, and naplink compatible rpc hander.
 *
 *  return values:
 *    MODULE_RESIDENT_END if loaded and registered as library.
 *    MODULE_NO_RESIDENT_END if just exiting normally.
 */
int _start( int argc, char **argv )
{
	printf( "tenftp: Copyright (c) 2004 Tentacle\n" );

	strcpy( base, "mc0:");

	// Copy the base location.
	strncpy( base, argv[1] ,sizeof(base));
    printf("redirecting '%s:' to TentacleFtpClient\n",FS_REPNAME );
	fd_global = 1;

	driver.name = FS_REPNAME;
	driver.type = 16;
	driver.version = 1;
	driver.desc = "ftp access driver";
	driver.ops = &functions;

	functions.io_init =		fd_initialize;
	functions.io_deinit =	dummy;
	functions.io_format =	dummy;
	functions.io_open =		fd_open;
	functions.io_close =	fd_close;
	functions.io_read =		fd_read;
	functions.io_write =	dummy;
	functions.io_lseek =	dummy;//fd_lseek;
	functions.io_ioctl =	dummy;
	functions.io_remove =	dummy;
	functions.io_mkdir =	dummy;
	functions.io_rmdir =	dummy;
	functions.io_dopen =	dummy;
	functions.io_dclose =	dummy;
	functions.io_dread =	dummy;
	functions.io_getstat =	dummy;
	functions.io_chstat =	dummy;

	printf( "HOST final step, bye\n" );

    // now install the fileio driver
//	io_DelDrv( FS_REPNAME );
	io_AddDrv( &driver );
	return 0;
}


