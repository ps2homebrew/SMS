/*
 * usb_mass.c - USB Mass storage driver for PS2
 */

#define MAJOR_VER 1
#define MINOR_VER 0

#include <tamtypes.h>
#include <thbase.h>
#include <thsemap.h>
#include <sifrpc.h>
#include <loadcore.h>
#include <ioman.h>
#include <sysclib.h>
#include <stdio.h>

#include <cdvdman.h>
#include <sysclib.h>

#include "mass_stor.h"
#include "fat_driver.h"
#include "mass_debug.h"
#include "usbhd_common.h"

extern int InitFS();
extern int InitUSB();

int _start( int argc, char **argv)
{

	FlushDcache();

    // initialize the USB driver
	if(InitUSB() != 0)
    {
        return(1);
    }

    // initialize the file system driver	    
    if(InitFS() != 0)
    {
        return(1);
    }

    // return resident
    return(0);
}
