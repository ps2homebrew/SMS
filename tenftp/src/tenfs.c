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
int fd_save( int fd, int bWrite, iop_io_file_t *f )
{
	f->unit = ++fd_global;
	if( bWrite )
		fd |= (1<<30);
	f->privdata = (void *)fd;
	return f->unit;
}

/*! \brief Get real filedescriptor.
 *  \ingroup tenftp 
 */
int realfd( iop_io_file_t *f )
{
	return ((int)f->privdata)&((1<<30)-1);
}

int realbWr( iop_io_file_t *f )
{
	return ((int)f->privdata)&((1<<30));
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


#include "fnc_file_openclose.h"
#include "fnc_file_readwrite.h"

#include "fnc_dir_openreadclose.h"
#include "fnc_dir_createremove.h"


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
	functions.io_write =	fd_write;
	functions.io_lseek =	dummy;
	functions.io_ioctl =	dummy;
	functions.io_remove =	dummy;
	functions.io_mkdir =	fd_mkdir;
	functions.io_rmdir =	fd_rmdir;
	functions.io_dopen =	fd_dopen;
	functions.io_dclose =	fd_dclose;
	functions.io_dread =	fd_dread;
	functions.io_getstat =	dummy;
	functions.io_chstat =	dummy;

	printf( "HOST final step, bye\n" );

    // now install the fileio driver
//	io_DelDrv( FS_REPNAME );
	io_AddDrv( &driver );
	return 0;
}


