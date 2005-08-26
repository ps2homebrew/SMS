#include "SMS.h"
#include "SMS_AVI.h"
#include "CDDA.h"
#include "CDVD.h"
#include "GS.h"
#include "GUI.h"
#include "Browser.h"
#include "FileContext.h"
#include "Timer.h"
#include "Config.h"

#include <kernel.h>
#include <libpad.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <libmc.h>

int main ( void ) {

 int             lfConfig;
 FileContext*    lpFileCtx;
 GSContext*      lpGSCtx;
 GUIContext*     lpGUICtx;
 BrowserContext* lpBrowserCtx;
 SMS_AVIPlayer*  lpPlayer;
 GSDisplayMode   lDisplayMode;
#if RESET_IOP
 SifIopReset ( "rom0:UDNL rom0:EELOADCNF", 0 );

 while (  SifIopSync ()  );
#endif  /* RESET_IOP */
 SifInitRpc ( 0 );

 CDDA_Init  ();
 Timer_Init ();

 lDisplayMode = GUI_InitPad ();

 SifLoadModule ( "rom0:MCMAN",  0, NULL );
 SifLoadModule ( "rom0:MCSERV", 0, NULL );

 mcInit ( MC_TYPE_MC );

 if (   (  lfConfig = LoadConfig ()  ) && lDisplayMode == GSDisplayMode_AutoDetect  ) lDisplayMode = ( GSDisplayMode )g_Config.m_DisplayMode;

 lpGSCtx = GS_InitContext  ( lDisplayMode );

 if ( lfConfig ) {

  lpGSCtx -> m_StartX = g_Config.m_DX;
  lpGSCtx -> m_StartY = g_Config.m_DY;

 }  /* end if */

 lpGUICtx = GUI_InitContext ( lpGSCtx );

 lpGUICtx -> Status ( "Initializing SMS..." );

 SMS_Initialize ( lpGUICtx );

 CDVD_Init ();

 lpBrowserCtx = BrowserContext_Init ( lpGUICtx );

 while ( 1 ) {

  lpFileCtx = lpBrowserCtx -> Browse ( lfConfig ? g_Config.m_Partition : NULL );
  lpPlayer  = SMS_AVIInitPlayer ( lpFileCtx, lpGUICtx );

  if ( lpPlayer == NULL ) {

   lpGUICtx -> Status ( "Unsupported file format (press X to continue)" );

   if (  CDDA_DiskType () != DiskType_Unknown  ) CDVD_Stop ();

   GUI_WaitButton ( PAD_CROSS );

  } else if ( lpPlayer -> Play == NULL ) {

   lpPlayer -> Destroy ();

   if (  CDDA_DiskType () != DiskType_Unknown  ) CDVD_Stop ();

   lpGUICtx -> Status ( "Unsupported codecs (press X to continue)" );
   GUI_WaitButton ( PAD_CROSS );

  } else {

   lpPlayer -> Play    ();
   lpPlayer -> Destroy ();
   lpGUICtx -> Redraw  ();

  }  /* end else */

  g_CDDASpeed = 4;

 }  /* end while */

 return 0;

}  /* end main */
