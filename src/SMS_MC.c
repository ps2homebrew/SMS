/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 200X ps2dev -> http://www.ps2dev.org
# Adopted for SMS in 2006 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_MC.h"
#include "SMS_SIF.h"

#include <kernel.h>
#include <string.h>

#define MC_SERVER 0x80000400

#define MC_RPCCMD_INIT     0x70
#define MC_RPCCMD_OPEN     0x71
#define MC_RPCCMD_CLOSE    0x72
#define MC_RPCCMD_READ     0x73
#define MC_RPCCMD_GET_DIR  0x76
#define MC_RPCCMD_GET_INFO 0x78

static struct {

 int  m_Port;
 int  m_Slot;
 int  m_Flags;
 int  m_MaxEnt;
 int  m_Table;
 char m_Name[ 1024 ];

} s_MCmd __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );

static struct {

 int  m_FD;
 int  m_Pad0;
 int  m_Pad1;
 int  m_Size;
 int  m_Offset;
 int  m_Origin;
 int  m_Buffer;
 int  m_Param;
 char m_Data[ 16 ];

} s_MCFileCmd __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );

static SifRpcClientData_t s_Client            __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned char      s_RecvData [ 2048 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static unsigned int       s_MCInfoCmd[   12 ] __attribute__(   (  aligned( 64 ), section( ".bss" )  )   );
static int                s_EndParam [   48 ] __attribute__(   (                 section( ".bss" )  )   );
static int*               s_pType             __attribute__(   (                 section( ".bss" )  )   );
static int*               s_pFree             __attribute__(   (                 section( ".bss" )  )   );
static int*               s_pFormat           __attribute__(   (                 section( ".bss" )  )   );
static unsigned int       s_LastCmd           __attribute__(   (                 section( ".bss" )  )   );

int MC_Init ( void ) {

 int retVal = 0;

 if ( s_Client.server == NULL              &&
      SIF_BindRPC ( &s_Client, MC_SERVER ) &&
      SifCallRpc (
       &s_Client, MC_RPCCMD_INIT, 0,
       &s_MCmd, 48, s_RecvData, 12, 0, 0
      ) >= 0
 ) retVal = *( int *)UNCACHED_SEG( s_RecvData );

 s_LastCmd = 0;

 return retVal;

}  /* end MC_Init */

int MC_Sync ( int* apRes ) {

 int retVal = -1;

 if ( s_LastCmd ) {

  while (  SifCheckStatRpc ( &s_Client )  );

  if ( apRes ) *apRes = *( int* )s_RecvData;

  s_LastCmd = 0;
  retVal    = 1;

 }  /* end if */
	
 return retVal;

}  /* end MC_Sync */

static void _get_info_cb ( volatile int* apArg ) {

 apArg = UNCACHED_SEG( apArg );

 if ( s_pType   ) *s_pType   = apArg[ 0 ];
 if ( s_pFree   ) *s_pFree   = apArg[ 1 ];
 if ( s_pFormat ) *s_pFormat = apArg[ 0 ];

}  /* end _get_info_cb */

int MC_GetInfo ( int aPort, int aSlot, int* apType, int* apFree, int* apFormat ) {

 int retVal = 0;
	
 if ( s_LastCmd ) return retVal;
	
 s_MCInfoCmd[ 1 ] = aPort;
 s_MCInfoCmd[ 2 ] = aSlot;
 s_MCInfoCmd[ 3 ] = apType   ? 1 : 0;
 s_MCInfoCmd[ 4 ] = apFree   ? 1 : 0;
 s_MCInfoCmd[ 5 ] = apFormat ? 1 : 0;
 s_MCInfoCmd[ 7 ] = ( int )s_EndParam;

 s_pType   = apType;
 s_pFree   = apFree;
 s_pFormat = apFormat;

 SifWriteBackDCache ( s_EndParam, 192 );

 if (  SifCallRpc ( &s_Client, MC_RPCCMD_GET_INFO, 1, s_MCInfoCmd, 48, s_RecvData, 4, ( SifRpcEndFunc_t )_get_info_cb, s_EndParam ) >= 0  ) {

  retVal    = 1;
  s_LastCmd = MC_RPCCMD_GET_INFO;

 }  /* end if */

 return retVal;

}  /* end MC_GetInfo */

int MC_GetDir ( int aPort, int aSlot, const char* apName, unsigned aMode, int aMaxEnt, SMS_MCTable* apTable ) {

 int retVal = 0;
	
 if ( s_LastCmd ) return retVal;

 s_MCmd.m_Port   = aPort;
 s_MCmd.m_Slot   = aSlot;
 s_MCmd.m_Flags  = aMode;
 s_MCmd.m_MaxEnt = aMaxEnt;
 s_MCmd.m_Table  = ( int )apTable;
 strcpy ( s_MCmd.m_Name, apName );

 SifWriteBackDCache (  apTable, aMaxEnt * sizeof ( SMS_MCTable )  );
	
 if (  SifCallRpc (
        &s_Client, MC_RPCCMD_GET_DIR, 1, &s_MCmd, 1044, s_RecvData, 4, 0, 0
       ) >= 0
 ) {

  retVal    = 1;
  s_LastCmd = MC_RPCCMD_GET_DIR;

 }  /* end if */

 return retVal;

}  /* end MC_GetDir */

int MC_Open ( int aPort, int aSlot, const char* apName, int aMode ) {

 int retVal = 0;
	
 if ( s_LastCmd ) return retVal;
	
 s_MCmd.m_Port  = aPort;
 s_MCmd.m_Slot  = aSlot;
 s_MCmd.m_Flags = aMode;
 strcpy ( s_MCmd.m_Name, apName );
	
 if (  SifCallRpc (
        &s_Client, MC_RPCCMD_OPEN, 1, &s_MCmd, 1044, s_RecvData, 4, 0, 0
       ) >= 0
 ) {

  retVal    = 1;
  s_LastCmd = MC_RPCCMD_OPEN;

 }  /* end if */

 return retVal;

}  /* end MC_Open */

static void _read_cb ( volatile int* apData ) {

 int*           lpPkt = UNCACHED_SEG( apData );
 unsigned char* lpSrc = ( unsigned char* )&lpPkt[ 4 ];
 unsigned char* lpDst = ( unsigned char* ) lpPkt[ 2 ];
	
 if ( lpDst ) memcpy ( lpDst, lpSrc, lpPkt[ 0 ] );
	
 lpSrc = ( unsigned char* )&lpPkt[ 8 ];
 lpDst = ( unsigned char* ) lpPkt[ 3 ];

 if ( lpDst ) memcpy ( lpDst, lpSrc, lpPkt[ 1 ] );

}  /* end _read_cb */

int MC_Read ( int aFD, void* apBuff, int aSize ) {

 int retVal = 0;
	
 if ( s_LastCmd ) return retVal;

 s_MCFileCmd.m_FD     = aFD;
 s_MCFileCmd.m_Size   = aSize;
 s_MCFileCmd.m_Buffer = ( int )apBuff;
 s_MCFileCmd.m_Param  = ( int )s_EndParam;

 SifWriteBackDCache ( apBuff,     aSize );
 SifWriteBackDCache ( s_EndParam, 192   );
	
 if (  SifCallRpc (
        &s_Client, MC_RPCCMD_READ, 1, &s_MCFileCmd, 48, s_RecvData, 4, ( SifRpcEndFunc_t )_read_cb, s_EndParam
       ) >= 0
 ) {

  retVal    = 1;
  s_LastCmd = MC_RPCCMD_READ;

 }  /* end if */

 return retVal;

}  /* end MC_Read */

int MC_Close ( int aFD ) {

 int retVal = 0;
	
 if ( s_LastCmd ) return retVal;
	
 s_MCFileCmd.m_FD = aFD;
	
 if (  SifCallRpc (
        &s_Client, MC_RPCCMD_CLOSE, 1, &s_MCFileCmd, 48, s_RecvData, 4, 0, 0
       ) >= 0
 ) {

  retVal    = 1;
  s_LastCmd = MC_RPCCMD_CLOSE;

 }  /* end if */

 return retVal;

}  /* end MC_Close */
