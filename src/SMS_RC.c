/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2001-2004, ps2dev - http://www.ps2dev.org
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_RC.h"
#include "SMS_SIF.h"

#include <fileio.h>
#include <string.h>
#include <malloc.h>
#include <loadfile.h>
#include <sifrpc.h>

#define RC_SERVER  0x80000C00
#define RCX_SERVER 0x80000C01

#define RCX_CMD_END   0x00000001
#define RCX_CMD_INIT  0x00000002
#define RCX_CMD_CLOSE 0x00000003
#define RCX_CMD_OPEN  0x00000004

#define RC_CMD_END      0x00000001
#define RC_CMD_INIT     0x00000003
#define RC_CMD_CLOSE    0x00000004
#define RC_CMD_OPEN     0x00000005

static unsigned char s_pROM1RMMAN [] __attribute__(   (  section( ".data" )  )   ) = "rom1:RMMAN";
static unsigned char s_pSMSRMMAN  [] __attribute__(   (  section( ".data" )  )   ) = "mc0:/SMS/RMMAN.IRX";
static unsigned char s_pSIO2MAN   [] __attribute__(   (  section( ".data" )  )   ) = "sio2man";
static unsigned char s_pROM1RMMAN2[] __attribute__(   (  section( ".data" )  )   ) = "rom1:RMMAN2";

static SifRpcClientData_t s_ClientRC    __attribute__(   (  aligned( 64 ), section ( ".bss" )  )   );
static SifRpcClientData_t s_ClientRCX   __attribute__(   (  aligned( 64 ), section ( ".bss" )  )   );
static unsigned int       s_CmdBuf[ 5 ] __attribute__(   (  aligned( 64 ), section ( ".bss" )  )   );

extern unsigned char g_RCData[ 256 ] __attribute__(   (  aligned( 64 ), section ( ".sbss" )  )   );

extern unsigned int RC_ReadDummy ( void );
extern unsigned int RC_ReadX     ( void );
extern unsigned int RC_ReadI     ( void );

unsigned int ( *RC_Read ) ( void ) = RC_ReadDummy;

static unsigned char s_RCMask = 0;

static void inline _patch_version ( unsigned char* apData, int aSize, char* apName, unsigned int aVersion ) {

 unsigned int* lpBegin = ( unsigned int* )apData;
 unsigned int* lpEnd   = lpBegin + (  ( aSize + 3 ) >> 2  ) - 1;
 int           lLen    = strlen ( apName );

 while ( lpBegin != lpEnd ) {

  if (  lpBegin[ 0 ] == 0x41E00000 &&
        lpBegin[ 1 ] == 0x00000000 &&
        !memcmp ( lpBegin + 3, apName, lLen )
  ) {

   lpBegin[ 2 ] = aVersion;
   return;

  }  /* end if */

  ++lpBegin;

 }  /* end while */

}  /* end _patch_version */

static int inline _load_rmman ( void ) {

 static char* s_Paths[] __attribute__(   (  section( ".data" )  )   ) = {
  s_pROM1RMMAN, s_pSMSRMMAN, NULL
 };

 char** lppPtr = s_Paths;
 int    retVal = -1;

 while ( *lppPtr ) {

  int lFD  = fioOpen ( *lppPtr, O_RDONLY );
  int lRes = -1;

  if ( lFD >= 0 ) {

   int lSize = fioLseek ( lFD, 0, SEEK_END );

   fioLseek ( lFD, 0, SEEK_SET );

   if ( lSize > 0 ) {

    unsigned char* lpData = ( unsigned char* )malloc ( lSize );

    fioRead ( lFD, lpData, lSize );
    _patch_version ( lpData, lSize, s_pSIO2MAN, 0x00000102 );
    lRes = SifExecModuleBuffer ( lpData, lSize, 0, NULL, &retVal );

    free ( lpData );

   }  /* end if */

   fioClose ( lFD );

  }  /* end if */

  if ( lRes >= 0 && retVal >= 0 ) {

   retVal = lRes;
   break;

  }  /* end if */

  ++lppPtr;

 }  /* end while */

 return retVal;

}  /* end _load_rmman */

static void _set_reader ( int aPort ) {

 if ( !aPort ) {

  if ( s_RCMask & 1 )
   RC_Close ( 0 );
  else if ( s_RCMask & 2 ) RC_Close ( 1 );

  RC_Read  = RC_ReadI;
  s_RCMask = 4;

 } else {

  if ( s_RCMask & 4 ) RCX_Close ();

  if (  ( aPort & 1 ) && ( s_RCMask & 2 )  )
   RC_Close ( 1 );
  else if ( s_RCMask & 1 ) RC_Close ( 0 );

  s_RCMask = aPort;
  RC_Read  = RC_ReadX;

 }  /* end else */

}  /* end _set_reader */

static void _reset_reader ( void ) {

 RC_Read  = RC_ReadDummy;
 s_RCMask = 0;

}  /* end _reset_reader */

int RC_Load ( void ) {

 int retVal = _load_rmman () >= 0;

 if ( retVal ) SIF_BindRPC ( &s_ClientRC, RC_SERVER );

 return retVal;

}  /* end RC_Load */

int RCX_Load ( void ) {

 int retVal = SifLoadModule ( s_pROM1RMMAN2, 0, NULL ) >= 0;

 if ( retVal ) SIF_BindRPC ( &s_ClientRCX, RCX_SERVER );

 return retVal;

}  /* end RCX_Load */

int RC_Start ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RC_CMD_INIT;

 if (  SifCallRpc (
        &s_ClientRC, 0, 0, s_CmdBuf, 4, s_CmdBuf, 16, 0, 0
       ) >= 0
 ) retVal = s_CmdBuf[ 3 ];

 return retVal;

}  /* end RC_Start */

int RC_Shutdown ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RC_CMD_END;

 if (  SifCallRpc (
        &s_ClientRC, 0, 0, s_CmdBuf, 4, s_CmdBuf, 16, 0, 0
       ) >= 0
 ) retVal = s_CmdBuf[ 3 ];

 return retVal;

}  /* end RC_Shutdown */

int RCX_Start ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RCX_CMD_INIT;

 if (  SifCallRpc (
        &s_ClientRCX, 0, 0, s_CmdBuf, 4, s_CmdBuf, 8, 0, 0
       ) >= 0
 ) retVal = s_CmdBuf[ 1 ];

 return retVal;

}  /* end RCX_Start */

int RCX_Shutdown ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RCX_CMD_END;

 if (  SifCallRpc (
        &s_ClientRCX, 0, 0, s_CmdBuf, 4, s_CmdBuf, 8, 0, 0
       ) >= 0
 ) retVal = s_CmdBuf[ 1 ];

 return retVal;

}  /* end RCX_Shutdown */

int RC_Open ( int aPort ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RC_CMD_OPEN;
 s_CmdBuf[ 1 ] = aPort;
 s_CmdBuf[ 2 ] = 0;
 s_CmdBuf[ 4 ] = ( unsigned int )&g_RCData[ 0 ];

 if (  SifCallRpc (
        &s_ClientRC, 0, 0, s_CmdBuf, 20, s_CmdBuf, 16, 0, 0
       ) >= 0
 ) {

  retVal = s_CmdBuf[ 3 ];
  _set_reader ( 1 << aPort );

 }  /* end if */

 return retVal;

}  /* end RC_Open */

int RC_Close ( int aPort ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RC_CMD_CLOSE;
 s_CmdBuf[ 1 ] = aPort;
 s_CmdBuf[ 2 ] = 0;

 if (  SifCallRpc (
        &s_ClientRC, 0, 0, s_CmdBuf, 12, s_CmdBuf, 16, 0, 0
       ) >= 0
 ) {

  retVal = s_CmdBuf[ 3 ];
  _reset_reader ();

 }  /* end if */

 return retVal;

}  /* end RC_Close */

int RCX_Open ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RCX_CMD_OPEN;
 s_CmdBuf[ 2 ] = ( unsigned int )&g_RCData[ 0 ];

 if (  SifCallRpc (
        &s_ClientRCX, 0, 0, s_CmdBuf, 12, s_CmdBuf, 8, 0, 0
       ) >= 0
 ) {

  retVal = s_CmdBuf[ 1 ];
  _set_reader ( 0 );

 }  /* end if */

 return retVal;

}  /* end RCX_Open */

int RCX_Close ( void ) {

 int retVal = 0;

 s_CmdBuf[ 0 ] = RCX_CMD_CLOSE;

 if (  SifCallRpc (
        &s_ClientRCX, 0, 0, s_CmdBuf, 4, s_CmdBuf, 8, 0, 0
       ) >= 0
 ) {

  retVal = s_CmdBuf[ 1 ];
  _reset_reader ();

 }  /* end if */

 return retVal;

}  /* end RCX_Close */
