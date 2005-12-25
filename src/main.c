/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_Player.h"
#include "SMS_DSP.h"
#include "CDDA.h"
#include "CDVD.h"
#include "PAD.h"
#include "GS.h"
#include "GUI.h"
#include "Browser.h"
#include "FileContext.h"
#include "Timer.h"
#include "Config.h"

#include <kernel.h>

int main ( int argc, char** argv ) {

 int             lfConfig;
 FileContext*    lpFileCtx;
 FileContext*    lpSubFileCtx;
 GUIContext*     lpGUICtx;
 BrowserContext* lpBrowserCtx;
 SMS_Player*     lpPlayer;
 GSDisplayMode   lDisplayMode;
 unsigned int    lSubFormat;

 GS_InitContext ( GSDisplayMode_AutoDetect );

 SMS_ResetIOP ();
 SMS_DSP_Init ();
 CDDA_Init    ();
 Timer_Init   ();
 CDVD_Init    ();

 lDisplayMode = GUI_InitPad ();
 lSubFormat   = lDisplayMode != GSDisplayMode_AutoDetect;

 if (   (  lfConfig = LoadConfig ()  ) && !lSubFormat  ) lDisplayMode = ( GSDisplayMode )g_Config.m_DisplayMode;

 GS_InitContext ( lDisplayMode );

 if ( lfConfig && !lSubFormat ) {

  g_GSCtx.m_StartX = g_Config.m_DX;
  g_GSCtx.m_StartY = g_Config.m_DY;

 }  /* end if */

 lpGUICtx = GUI_InitContext ();

 lpGUICtx -> Status ( "Initializing SMS..." );

 SMS_Initialize ( lpGUICtx );

 g_DVDVSupport = CDVD_QueryDVDV ();
 lpBrowserCtx  = BrowserContext_Init ( lpGUICtx );

 while ( 1 ) {

  lpFileCtx = lpBrowserCtx -> Browse ( lfConfig ? g_Config.m_Partition : NULL, &lpSubFileCtx, &lSubFormat );
  lpPlayer  = SMS_InitPlayer ( lpFileCtx, lpGUICtx, lpSubFileCtx, lSubFormat );

  if ( lpPlayer == NULL ) {

   lpGUICtx -> Status ( "Unsupported file format (press X to continue)" );

   if (  CDDA_DiskType () != DiskType_Unknown  ) CDDA_Stop ();

   GUI_WaitButton ( PAD_CROSS, 0 );

  } else {

   lpPlayer -> Play    ();
   lpPlayer -> Destroy ();
   lpGUICtx -> Redraw  ( 1 );

  }  /* end else */

  g_CDDASpeed = 4;

 }  /* end while */

 return 0;

}  /* end main */
