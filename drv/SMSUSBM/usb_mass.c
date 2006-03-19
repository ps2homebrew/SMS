/*
 * usb_mass.c - USB Mass storage driver for PS2
 *
 * (C) 2001, Gustavo Scotti (gustavo@scotti.com)
 * (C) 2002, David Ryan ( oobles@hotmail.com )
 * (C) 2004, Marek Olejnik (ole00@post.cz)
 * (C) 2006, Eugene Plotnikov (SMS version)
 *
 * IOP file io driver
 *
 * See the file LICENSE included with this distribution for licensing terms.
 */
#include <loadcore.h>
#include <ioman.h>

#include "mass_stor.h"
#include "fat_driver.h"

static iop_device_t     s_Driver;
static iop_device_ops_t s_Func;

static void initFsDriver ( void ) {

 int i;

 s_Driver.name    = "mass";
 s_Driver.type    = IOP_DT_FS;
 s_Driver.version = 1;
 s_Driver.desc    = "SMS USB Driver";
 s_Driver.ops     = &s_Func;

 s_Func.init    = fs_init;
 s_Func.deinit  = fs_deinit;
 s_Func.format  = fs_format;
 s_Func.open    = fs_open;
 s_Func.close   = fs_close;
 s_Func.read    = fs_read;
 s_Func.write   = fs_write;
 s_Func.lseek   = fs_lseek;
 s_Func.ioctl   = fs_ioctl;
 s_Func.remove  = fs_remove;
 s_Func.mkdir   = fs_mkdir;
 s_Func.rmdir   = fs_rmdir;
 s_Func.dopen   = fs_dopen;
 s_Func.dclose  = fs_dclose;
 s_Func.dread   = fs_dread;
 s_Func.getstat = fs_getstat;
 s_Func.chstat  = fs_chstat;

 DelDrv ( "mass"    );
 AddDrv ( &s_Driver );

}  /* end initFsDriver */

int _start ( int argc, char** argv ) {

 FlushDcache    ();
 initFsDriver   ();
 mass_stor_init ();

 return MODULE_RESIDENT_END;

}  /* end _start */
