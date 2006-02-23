/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2003 Marcus R. Brown <mrbrown@0xd6.org>
# Adopted for SMS in 2006 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include <kernel.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <string.h>

static char s_pProgName[ 1024 ];

void _start (  char* apProgName, void ( *error ) ( void )  ) {

 t_ExecData lXData;
 char*      lpArgv[ 1 ] = { s_pProgName };
 char*      lpDst       = s_pProgName;

 while ( *apProgName ) *lpDst++ = *apProgName++;

 *lpDst = '\x00';

 SifLoadElf ( s_pProgName, &lXData );

 if ( lXData.epc ) {

  FlushCache ( 0 );
  FlushCache ( 2 );
  ExecPS2 (  ( void* )lXData.epc, ( void* )lXData.gp, 1, lpArgv  );

 }  /* end if */

 error ();

}  /* end _start */
