/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# (c) 2005 HOST support by Ronald Andersson (AKA: dlanor)
# Special thanks to 'bigboss'/PS2Reality for valuable information
# about SifAddCmdHandler function
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#define NO_DEBUG 1
#include "SMS.h"
#include "SMS_IOP.h"
#include "SMS_Data.h"
#include "SMS_Config.h"
#include "SMS_GUI.h"
#include "SMS_Locale.h"
#include "SMS_SIF.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_RC.h"
#include "SMS_SMB.h"
#include "SMS_FileDir.h"
#include "SMS_ioctl.h"
#include "SMS_Timer.h"

#include <kernel.h>
#include <sifrpc.h>
#include <iopheap.h>
#include <iopcontrol.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <lzma2.h>

#define SMSUTILS_RPC_ID 0x6D737573

extern void* _gp;

unsigned int g_IOPFlags;

static char s_pSIO2MAN[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:SIO2MAN";
static char s_pPADMAN [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:PADMAN";
static char s_pMCMAN  [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:MCMAN";
static char s_pMCSERV [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:MCSERV";
static char s_pUSBD   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "USBD.IRX";

static char s_HDDArgs[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 '-', 'o', '\x00', '2',      '\x00',
 '-', 'n', '\x00', '2', '0', '\x00'
};

static char s_PFSArgs[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = {
 '-', 'm', '\x00', '2',      '\x00',
 '-', 'o', '\x00', '2', '0', '\x00',
 '-', 'n', '\x00', '4', '0', '\x00'
};

static char s_pAudSrv [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "AUDSRV";
static char s_pPS2Dev9[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PS2DEV9";
static char s_pPS2ATAD[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PS2ATAD";
static char s_pPS2HDD [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PS2HDD";
static char s_pPS2FS  [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PS2FS";
static char s_pPS2POff[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "POWEROFF";
static char s_pUDNL   [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:UDNL rom0:EELOADCNF";
static char s_pLIBSD  [] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "rom0:LIBSD";

struct {

 const char* m_pName;
 void*       m_pBuffer;
 int         m_BufSize;
 int         m_nArgs;
 void*       m_pArgs;

} s_LoadParams[ 6 ] __attribute__(   (  section( ".data" )  )   ) = {
 { s_pAudSrv,  &g_DataBuffer[ SMS_AUDSRV_OFFSET   ], SMS_AUDSRV_SIZE,   0,                    NULL      },
 { s_pPS2Dev9, &g_DataBuffer[ SMS_PS2DEV9_OFFSET  ], SMS_PS2DEV9_SIZE,  0,                    NULL      },
 { s_pPS2POff, &g_DataBuffer[ SMS_POWEROFF_OFFSET ], SMS_POWEROFF_SIZE, 0,                    NULL      },
 { s_pPS2ATAD, &g_DataBuffer[ SMS_PS2ATAD_OFFSET  ], SMS_PS2ATAD_SIZE,  0,                    NULL      },
 { s_pPS2HDD,  &g_DataBuffer[ SMS_PS2HDD_OFFSET   ], SMS_PS2HDD_SIZE,   sizeof ( s_HDDArgs ), s_HDDArgs },
 { s_pPS2FS,   &g_DataBuffer[ SMS_PS2FS_OFFSET    ], SMS_PS2FS_SIZE,    sizeof ( s_PFSArgs ), s_PFSArgs }
};

static SifRpcClientData_t s_SMSUClt __attribute__ (   (  aligned( 64 ), section( ".bss" )  )   );

static void ( *s_SMS_SIFCmdHandler[ 6 ] ) ( void* ) __attribute__(   (  section( ".data" )  )   );

void SMS_IOPSetSifCmdHandler (  void ( *apFunc ) ( void* ), int aCmd  ) {

 s_SMS_SIFCmdHandler[ aCmd ] = apFunc;

}  /* end SMS_SetSifCmdHandler */

static void _sif_cmd_handler ( void* apPkt, void* apArg ) {

 s_SMS_SIFCmdHandler[ (  ( SifCmdHeader_t* )apPkt  ) -> opt ] ( apPkt );

}  /* end _sif_cmd_handler */

static void _load_module ( int anIndex, int afStatus ) {

 int lRes, lModRes;

 if ( afStatus ) {

  char lBuff[ 128 ]; sprintf ( lBuff, STR_LOADING.m_pStr, s_LoadParams[ anIndex ].m_pName );

  GUI_Status ( lBuff );

 }  /* end if */

 lRes = SifExecModuleBuffer (
  s_LoadParams[ anIndex ].m_pBuffer, s_LoadParams[ anIndex ].m_BufSize,
  s_LoadParams[ anIndex ].m_nArgs,   s_LoadParams[ anIndex ].m_pArgs, &lModRes
 );

 if ( anIndex == 1 && lRes >= 0 ) g_IOPFlags |= SMS_IOPF_DEV9_IS;

}  /* end _load_module */

#include "SMS_GS.h"
#include <slib.h>
#include <sbv_patches.h>
//#include "s_iop_image.h"

extern slib_exp_lib_list_t _slib_cur_exp_lib_list __attribute__(   (  section( ".data" )  )   );

int SifExecDecompModuleBuffer(void *ptr, u32 size, u32 arg_len, const char *args, int *mod_res)
{
	unsigned char *irx_data;
	int irx_size, ret = -1;
	
	if((irx_size = lzma2_get_uncompressed_size((unsigned char *)ptr, size)) > 0)
	{
		irx_data = (unsigned char *)memalign(64, irx_size);
		ret = lzma2_uncompress((unsigned char *)ptr, size, irx_data, irx_size);
		
		if(ret > 0)
			ret = SifExecModuleBuffer( irx_data, irx_size, arg_len, args, mod_res );
		
		free(irx_data);	
	}

	return ret;
}

void SMS_IOPReset ( int afExit ) {

 static const char* lpModules[ 4 ] = { s_pSIO2MAN, s_pPADMAN, s_pMCMAN, s_pMCSERV };

 int i;
#if NO_DEBUG
 SifInitRpc ( 0 );
 SifExitIopHeap ();
 SifLoadFileExit(); 
 SifExitRpc     (); 

 while(!SifIopReset(s_pUDNL, 0)){};

 FlushCache(0);
 FlushCache(2);

 while (!SifIopSync()) {;}

 SifInitRpc ( 0 );

 _slib_cur_exp_lib_list.tail = NULL;
 _slib_cur_exp_lib_list.head = NULL;
 sbv_patch_enable_lmb           ();
 sbv_patch_disable_prefix_check ();

 //while(!SifIopReset(s_iop_image, 0)){};

 FlushCache(0);
 FlushCache(2);

 while (!SifIopSync()) {;}

 SifInitRpc ( 0 );
 _slib_cur_exp_lib_list.tail = NULL;
 _slib_cur_exp_lib_list.head = NULL;
 sbv_patch_enable_lmb           ();
 sbv_patch_disable_prefix_check ();

 RCX_Load  ();
 RCX_Start ();
 RCX_Open  ();

#if 0
 while ( 1 ) {
  unsigned int lColor = RC_Read ();
  GS_VSync ();
  GS_BGCOLOR() = lColor + 0x00000030;
 }
#endif

#else
 afExit = 1;
#endif  /* NO_DEBUG */
 sbv_patch_disable_prefix_check ();
 sbv_patch_enable_lmb           ();

 SifExecModuleBuffer ( &g_DataBuffer[ SMS_SMSUTILS_OFFSET ], SMS_SMSUTILS_SIZE, 0, NULL, &i );

 if ( !afExit ) SifExecModuleBuffer ( &g_DataBuffer[ SMS_SIO2MAN_OFFSET ], SMS_SIO2MAN_SIZE,  0, NULL, &i );

 for ( i = 1 - afExit; i < 4; ++i ) SifLoadModule ( lpModules[ i ], 0, NULL );

 SIF_BindRPC ( &s_SMSUClt, SMSUTILS_RPC_ID );

 DisableIntc(INTC_TIM0);
 DisableIntc(INTC_TIM1);

}  /* end SMS_IOPReset */

int SMS_IOPStartNet ( int afStatus ) {

 int  i, j;
 char lSMAPArgs[ 80 ];
 int  lSMAPALen;

 if (  !( g_IOPFlags & SMS_IOPF_DEV9_IS )  ) return 0;
 if (  !( g_IOPFlags & SMS_IOPF_DEV9    )  ) {
  SMS_IOCtl ( g_pDEV9X, DEV9CTLINIT, NULL );
  g_IOPFlags |= SMS_IOPF_DEV9;
 }  /* end if */

 if ( afStatus ) GUI_Status ( STR_INITIALIZING_NETWORK.m_pStr );

 memset (  lSMAPArgs, 0, sizeof ( lSMAPArgs )  );
 strncpy ( lSMAPArgs, g_pDefIP, 15 );
 i = strlen ( g_pDefIP ) + 1;
 strncpy ( lSMAPArgs + i, g_pDefMask, 15 );
 i += strlen ( g_pDefMask ) + 1;
 strncpy ( lSMAPArgs + i, g_pDefGW, 15 );
 i += strlen ( g_pDefGW ) + 1;
 lSMAPALen = i;

 j = ( g_Config.m_NetworkFlags >> 8 ) & 3;

 if ( j == 1 )

  j = 0x05E0;

 else if ( j == 2 ) {

  j = 0x0400;

  if ( g_Config.m_NetworkFlags & SMS_DF_HALF ) {

   if ( g_Config.m_NetworkFlags & SMS_DF_10 )
    j |= 0x0020;
   else j |= 0x080;

  } else {

   if ( g_Config.m_NetworkFlags & SMS_DF_10 )
    j |= 0x040;
   else j |= 0x0100;

  }  /* end else */

 } else j = 0;

 sprintf (  &lSMAPArgs[ i ], "%d", j  );
 lSMAPALen += strlen ( &lSMAPArgs[ i ] ) + 1;

 SifExecModuleBuffer ( &g_DataBuffer[ SMS_PS2IP_OFFSET ], SMS_PS2IP_SIZE, 0, NULL, &i );

 if ( i >= 0 ) {

  SifExecModuleBuffer ( &g_DataBuffer[ SMS_PS2SMAP_OFFSET ], SMS_PS2SMAP_SIZE, lSMAPALen, &lSMAPArgs[ 0 ], &i );

  if ( i >= 0 ) {

   void* lpModule;
   int   lSize;
   int   lFlags;

   if ( g_Config.m_NetworkFlags & SMS_DF_SMB ) {
    lpModule = &g_DataBuffer[ SMS_SMB_OFFSET ];
    lSize    = SMS_SMB_SIZE;
    lFlags   = SMS_IOPF_SMB;
   } else {
    lpModule = &g_DataBuffer[ SMS_PS2HOST_OFFSET ];
    lSize    = SMS_PS2HOST_SIZE;
    lFlags   = SMS_IOPF_NET;
   }  /* end else */

   SifExecModuleBuffer ( lpModule, lSize, 0, NULL, &i );

   if ( i >= 0 ) g_IOPFlags |= lFlags;

  }  /* end if */

 }  /* end if */

 return g_IOPFlags & ( SMS_IOPF_NET | SMS_IOPF_SMB );

}  /* end SMS_IOPStartNet */

int SMS_IOPStartUSB ( int afStatus ) {

 int  i;
 char lBuf[ 64 ];

 sprintf ( lBuf, g_pFmt3, g_pMC0SMS, g_SlashStr, s_pUSBD );

 if ( afStatus ) GUI_Status ( STR_LOCATING_USBD.m_pStr );

 i = fioOpen ( lBuf, O_RDONLY );

 if ( i >= 0 ) {
  fioClose ( i );
  i = SifLoadModule ( lBuf, 0, NULL );
 }  /* end if */

 if ( i < 0 ) SifExecDecompModuleBuffer ( &g_DataBuffer[ SMS_USBD_OFFSET ], SMS_USBD_SIZE, 0, NULL, &i );

 g_IOPFlags |= SMS_IOPF_USB;

 lBuf[ strlen ( lBuf ) - 5 ] = 'M';

 i = fioOpen ( lBuf, O_RDONLY );

 if ( i >= 0 ) {
  fioClose ( i );
  i = SifLoadModule ( lBuf, 0, NULL );
 }  /* end if */

 if ( i < 0 ) {
  SifExecDecompModuleBuffer ( &g_DataBuffer[ SMS_USB_MASS_OFFSET ], SMS_USB_MASS_SIZE, 0, NULL, &i );
  g_IOPFlags |= SMS_IOPF_UMS;
  *( int* )g_pUSB = 0x20736D75;
 }  /* end if */

 return g_IOPFlags & SMS_IOPF_USB;

}  /* end SMS_IOPStartUSB */

int SMS_IOPStartHDD ( int afStatus ) {

 int i;

 if (  !( g_IOPFlags & SMS_IOPF_DEV9_IS )  ) return 0;
 if (  !( g_IOPFlags & SMS_IOPF_DEV9    )  ) {
  SMS_IOCtl ( g_pDEV9X, DEV9CTLINIT, NULL );
  g_IOPFlags |= SMS_IOPF_DEV9;
 }  /* end if */

 for ( i = 3; i < 6; ++i ) _load_module ( i, afStatus );

 i = SMS_IOCtl ( g_pHDD, PS2HDD_IOCTL_STATUS, NULL );

 if ( i == 0 || i == 1 ) g_IOPFlags |= SMS_IOPF_HDD;

 return g_IOPFlags & SMS_IOPF_HDD;

}  /* end SMS_IOPStartHDD */

void SMS_IOPSetXLT ( void ) {

 if ( g_IOPFlags & SMS_IOPF_SMBLOGIN ) SMS_IOCtl ( g_pSMBS, SMB_IOCTL_SETCP, g_XLT[ g_Config.m_DisplayCharset ] );

}  /* end SMS_IOPSetXLT */

extern void _exit_handler ( void*, int );

static int  s_PwrOffThreadID;
static char s_PwrOffThreadStack[ 2048 ] __attribute__(   ( aligned( 16 )  )   );

static void _poweroff_thread ( void* apData ) {

 int lFD;

 while ( 1 ) {

  SleepThread ();

  ChangeThreadPriority (  GetThreadId (), 99  );

  if ( g_Config.m_BrowserFlags & SMS_BF_EXIT ) _exit_handler ( NULL, 1 );

  SMS_IOCtl ( g_pPFS, PFS_IOCTL_CLOSE_ALL,      NULL );
  SMS_IOCtl ( g_pHDD, PS2HDD_IOCTL_FLUSH_CACHE, NULL );

  lFD = fioDopen ( g_pDEV9X );

  if ( lFD >= 0 ) fioIoctl ( lFD, DEV9CTLSHUTDOWN, NULL );

  DIntr ();
   ee_kmode_enter ();
    *(  ( unsigned char* )0xBF402017 ) = 0x00;
    *(  ( unsigned char* )0xBF402016 ) = 0x0F;
   ee_kmode_exit ();
  EIntr ();

 }  /* end while */

}  /* end _poweroff_thread */

static void _poweroff_handler ( void* apHdr ) {
 iWakeupThread ( s_PwrOffThreadID );
}  /* end _poff_intr_callback */

void SMS_IOPowerOff ( void ) {
 WakeupThread ( s_PwrOffThreadID );
}  /* end SMS_IOPowerOff */

static SifCmdHandlerData_t handlerdata[32];

void SMS_IOPInit ( void ) {

 int         i, lFD;
 char        lBuff[ 64 ];
 ee_thread_t lThreadParam;

 SifLoadModule ( s_pLIBSD, 0, NULL );

 SMS_IOPDVDVInit ();

 lFD = fioOpen ( g_pIPConf, O_RDONLY );

 if ( lFD >= 0 ) {

  memset (  lBuff, 0, sizeof ( lBuff )  );
  i = fioRead (  lFD, lBuff, sizeof ( lBuff ) - 1  );
  fioClose ( lFD );

  if ( i > 0 ) {

   char lChr;

   lBuff[ i ] = '\x00';

   for (  i = 0; (  ( lChr = lBuff[ i ] ) != '\0'  ); ++i  )

    if (  lChr == ' ' || lChr == '\r' || lChr == '\n' ) lBuff[ i ] = '\x00';

   strncpy ( g_pDefIP, lBuff, 15 );
   i = strlen ( g_pDefIP ) + 1;
   strncpy ( g_pDefMask, lBuff + i, 15 );
   i += strlen ( g_pDefMask ) + 1;
   strncpy ( g_pDefGW, lBuff + i, 15 );

  }  /* end if */

 }  /* end if */

 for ( i = 0; i < 3; ++i ) _load_module ( i, 1 );

 if ( g_IOPFlags & SMS_IOPF_DEV9_IS ) {
#if NO_DEBUG
  if (   !(  g_Config.m_NetworkFlags & ( SMS_DF_AUTO_HDD | SMS_DF_AUTO_NET )  )   )
   SMS_IOCtl ( g_pDEV9X, DEV9CTLSHUTDOWN, NULL );
  else g_IOPFlags |= SMS_IOPF_DEV9;
#else
  g_IOPFlags |= SMS_IOPF_DEV9;
#endif  /* NO_DEBUG */
 }  /* end if */

 SPU_Initialize ();

 if ( g_Config.m_NetworkFlags & SMS_DF_AUTO_HDD ) SMS_IOPStartHDD ( 1 );
 if ( g_Config.m_NetworkFlags & SMS_DF_AUTO_NET ) SMS_IOPStartNet ( 1 );
 if ( g_Config.m_NetworkFlags & SMS_DF_AUTO_USB ) SMS_IOPStartUSB ( 1 );

 GUI_Status ( STR_INITIALIZING_SMS.m_pStr );

 SMS_IOPSetSifCmdHandler ( _poweroff_handler, SMS_SIF_CMD_POWEROFF );

 lThreadParam.initial_priority   = 48;
 lThreadParam.stack_size         = sizeof ( s_PwrOffThreadStack );
 lThreadParam.gp_reg             = &_gp;
 lThreadParam.func               = _poweroff_thread;
 lThreadParam.stack              = s_PwrOffThreadStack;
 StartThread (  s_PwrOffThreadID = CreateThread ( &lThreadParam ), NULL  );

 DI();
  SifSetCmdBuffer(&handlerdata[0], 32);
  SifAddCmdHandler ( 18, _sif_cmd_handler, NULL );
 EI();

 SPU_LoadData (  g_SMSounds, sizeof ( g_SMSounds )  );

 if (  RCX_Load () && RCX_Start ()  )
  g_IOPFlags |= SMS_IOPF_RMMAN2;
 else if (  RC_Load () && RC_Start ()  ) g_IOPFlags |= SMS_IOPF_RMMAN;

 FlushCache ( 0 );

}  /* end SMS_IOPInit */

int SMS_IOPQueryTotalFreeMemSize ( void ) {

 int retVal;

 SifCallRpc ( &s_SMSUClt, 0, 0, NULL, 0, &retVal, 4, 0, 0 );

 return retVal;

}  /* end SMS_IOPQueryTotalFreeMemSize */

int SMS_IOPDVDVInit ( void ) {

 int retVal;

 SifCallRpc ( &s_SMSUClt, 1, 0, NULL, 0, &retVal, 4, 0, 0 );

 return retVal;

}  /* end SMS_IOPDVDVInit */

int SMS_IOCtl ( const char* apDev, int aCmd, void* apData ) {

 int lFD = fioDopen ( apDev );

 if ( lFD >= 0 ) {

  int retVal = fioIoctl ( lFD, aCmd, apData );

  fioDclose ( lFD );

  return retVal;

 } else return lFD;

}  /* end SMS_IOCtl */

int SMS_HDDMount ( const char* aFSys, const char* aPath, int aMode ) {

 PS2HDDMountInfo lMountInfo;

 lMountInfo.m_Mode = aMode;
 strcpy ( lMountInfo.m_Path, aPath );

 return SMS_IOCtl ( aFSys, PFS_IOCTL_MOUNT, &lMountInfo );

}  /* end SMS_HDDMount */
