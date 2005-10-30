#include "SMS.h"
#include "SMS_Player.h"
#include "SMS_DSP.h"
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
#include <iopheap.h>

int main ( int argc, char** argv ) {

 int             lfConfig;
 FileContext*    lpFileCtx;
 GSContext*      lpGSCtx;
 GUIContext*     lpGUICtx;
 BrowserContext* lpBrowserCtx;
 SMS_Player*     lpPlayer;
 GSDisplayMode   lDisplayMode;

 SMS_DSP_Init ();
#if RESET_IOP
 SifInitRpc     ( 0 ); 
 SifExitIopHeap (); 
 SifLoadFileExit(); 
 SifExitRpc     (); 
 SifIopReset ( "rom0:UDNL rom0:EELOADCNF", 0 );

 while (  SifIopSync ()  );
#endif  /* RESET_IOP */
 SifInitRpc ( 0 );

 CDDA_Init  ();
 Timer_Init ();

 lDisplayMode = GUI_InitPad ();

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

 g_DVDVSupport = CDVD_QueryDVDV ();
 lpBrowserCtx  = BrowserContext_Init ( lpGUICtx );

 while ( 1 ) {

  lpFileCtx = lpBrowserCtx -> Browse ( lfConfig ? g_Config.m_Partition : NULL );
  lpPlayer  = SMS_InitPlayer ( lpFileCtx, lpGUICtx );

  if ( lpPlayer == NULL ) {

   lpGUICtx -> Status ( "Unsupported file format (press X to continue)" );

   if (  CDDA_DiskType () != DiskType_Unknown  ) CDVD_Stop ();

   GUI_WaitButton ( PAD_CROSS, 0 );

  } else if ( lpPlayer -> Play == NULL ) {

   lpPlayer -> Destroy ();

   if (  CDDA_DiskType () != DiskType_Unknown  ) CDVD_Stop ();

   lpGUICtx -> Status ( "Unsupported codecs (press X to continue)" );
   GUI_WaitButton ( PAD_CROSS, 0 );

  } else {

   lpPlayer -> Play    ();
   lpPlayer -> Destroy ();
   lpGUICtx -> Redraw  ();

  }  /* end else */

  g_CDDASpeed = 4;

 }  /* end while */

 return 0;

}  /* end main */
