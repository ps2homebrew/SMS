#include <kernel.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sifrpc.h>
#include <string.h>

static char s_pProgName[ 1024 ];

int SIF_SyncIOP ( void );

void _start (  char* apProgName, void ( *error ) ( void )  ) {

 t_ExecData lXData;
 char*      lpArgv[ 1 ] = { s_pProgName };
 char*      lpDst       = s_pProgName;

 while ( *apProgName ) *lpDst++ = *apProgName++;

 *lpDst = '\x00';

 SifLoadElf ( s_pProgName, &lXData );

 if ( lXData.epc ) {

  SifLoadFileExit ();
  SifExitRpc      ();
  SifResetIop     ();

  while (  !SIF_SyncIOP ()  );

  SifLoadModule ( "rom0:SIO2MAN", 0, NULL );
  SifLoadModule ( "rom0:MCMAN",   0, NULL );
  SifLoadModule ( "rom0:MCMAN",   0, NULL );

  FlushCache ( 0 );
  FlushCache ( 2 );
  ExecPS2 (  ( void* )lXData.epc, ( void* )lXData.gp, 1, lpArgv  );

 }  /* end if */

 error ();

}  /* end _start */

int SIF_SyncIOP ( void ) {

 return SifGetReg ( 4 ) & 0x00040000; 

}  /* end SIF_SyncIOP */
