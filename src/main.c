/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2008 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUI.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"
#include "SMS_EE.h"
#include "SMS_RC.h"
#include "SMS_IOP.h"
#include "SMS_DSP.h"
#include "SMS_DMA.h"
#include "SMS_Locale.h"
#include "SMS_Config.h"
#include "SMS_History.h"
#include "SMS_OS.h"
#include "SMS_GS.h"
#include "SMS_PgInd.h"
#include "SMS_CDDA.h"
#include "SMS_VIF.h"
#include <kernel.h>
#include <sys/ioctl.h>

#ifdef DISABLE_EXTRA_TIMERS_FUNCTIONS
DISABLE_EXTRA_TIMERS_FUNCTIONS();
#endif

int main ( int argc, char** argv ) {

 if ( argc > 0 && argv[ 0 ][ 0 ] == 'm' && argv[ 0 ][ 1 ] == 'c' ) {

  char lSlot = argv[ 0 ][ 2 ];

  SMS_SetMCSlot ( lSlot );
  g_pIPConf   [ 2 ] = lSlot;
  g_pBXDATASYS[ 2 ] = lSlot;
  g_pSMSSkn   [ 2 ] = lSlot;
  g_pSMSRMMAN [ 2 ] = lSlot;
  g_pExec0    [ 2 ] = lSlot;
  g_pExec1    [ 2 ] = lSlot;
  g_MCSlot          = lSlot - '0';

 }  /* end if */

 SMS_IOPReset ( 0 );
 SMS_EEInit   ();
 CDVD_Init    ();
 CDDA_Init    ();

 GUI_Initialize ( 1 );
 SMS_PgIndStart ();
 GUI_Status ( STR_INITIALIZING_SMS.m_pStr );
#ifndef EMBEDDED
 if ( g_Config.m_BrowserFlags & SMS_BF_UXH ) SMS_OSInit ( argv[ 0 ] );
#endif  /* EMBEDDED */
 SMS_IOPInit     ();
 SMS_EEPort2Init ();
 CDVD_SetSpeed   ();

 if (  CDDA_DiskType () != DiskType_None  ) CDVD_Stop ();

 SMS_LoadXLT ();
 SMS_EEScanDir ( g_pMC0SMS, g_pExtMBF, g_Config.m_pMBFList );
 SMS_HistoryLoad ();
 GUI_DeleteObject ( g_pVerStr );
 SMS_PgIndStop ();

 GUI_Run ();

 return 0;

}  /* end main */
