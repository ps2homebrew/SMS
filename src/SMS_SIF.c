/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 200X... - PS2Dev
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_SIF.h"

#include <kernel.h>

int SIF_BindRPC ( SifRpcClientData_t* apData, int anID ) {

 int i, retVal = 0;

 if ( apData -> server ) return 1;

 while ( 1 ) {

  if (  SifBindRpc ( apData, anID, 0 ) < 0  ) break;

  if ( apData -> server ) {

   retVal = 1;
   break;

  }  /* end if */

  i = 10000; while ( i-- );

 }  /* end while */

 return retVal;

}  /* end SIF_BindRPC */

int SIF_SyncIOP ( void ) {

 return SifGetReg ( 4 ) & 0x00040000; 

}  /* end SIF_SyncIOP */
