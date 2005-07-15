#include "SMS.h"
#include "SMS_AVI.h"
#include "CDDA.h"
#include "GS.h"
#include "GUI.h"
#include "Browser.h"
#include "FileContext.h"
#include "Timer.h"

#include <kernel.h>
#include <libpad.h>
#include <sifrpc.h>

int main ( void ) {

 FileContext*    lpFileCtx;
 GSContext*      lpGSCtx;
 GUIContext*     lpGUICtx;
 BrowserContext* lpBrowserCtx;
 SMS_AVIPlayer*  lpPlayer;

 SifInitRpc ( 0 );

 CDDA_Init  ();
 Timer_Init ();

 lpGSCtx  = GS_InitContext  ( GSDisplayMode_AutoDetect );
 lpGUICtx = GUI_InitContext ( lpGSCtx                  );

 lpGUICtx -> Status ( "Initializing SMS..." );

 SMS_Initialize ( lpGUICtx );

 lpBrowserCtx = BrowserContext_Init ( lpGUICtx );

 while ( 1 ) {

  lpFileCtx = lpBrowserCtx -> Browse ();
  lpPlayer  = SMS_AVIInitPlayer ( lpFileCtx, lpGUICtx );

  if ( lpPlayer == NULL ) {

   lpGUICtx -> Status ( "Unsupported file format (press X to continue)" );
   GUI_WaitButton ( PAD_CROSS );

  } else if ( lpPlayer -> Play == NULL ) {

   lpPlayer -> Destroy ();

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
