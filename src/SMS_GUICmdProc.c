/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUI.h"
#include "SMS_FileContext.h"
#include "SMS_Player.h"
#include "SMS_Locale.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "SMS_FileDir.h"
#include "SMS_SubtitleContext.h"

#include <malloc.h>

static void _start_player ( FileContext*, FileContext*, SubtitleFormat );

static void GUICmdProc_Render ( GUIObject* apObj, int aCtx ) {

}  /* end GUICmdProc_Render */

static void GUICmdProc_Cleanup ( GUIObject* apObj ) {

}  /* end GUICmdProc_Cleanup */

extern void RestoreFileDir ( void** );

static int GUICmdProc_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 int           retVal = GUIHResult_Void;
 unsigned long lEvent = anEvent & 0xF000000000000000L;

 if ( lEvent == GUI_MSG_FILE || lEvent == GUI_MSG_FOLDER_MP3 ) {

  void** lpParam = ( void** )( unsigned int )(  ( anEvent & 0x0FFFFFFFFFFFFFFFL ) >> 28  );

  _start_player (  ( FileContext* )lpParam[ 0 ], ( FileContext* )lpParam[ 1 ], ( SubtitleFormat )lpParam[ 2 ]  );

  if ( lEvent == GUI_MSG_FILE )

   free ( lpParam );

  else RestoreFileDir ( lpParam );

  retVal = GUIHResult_Handled;

 }  /* end if */

 return retVal;

}  /* end GUICmdProc_HandleEvent */

GUIObject* GUI_CreateCmdProc ( void ) {

 GUIObject* retVal = ( GUIObject* )calloc (  1, sizeof ( GUIObject )  );

 retVal -> Render      = GUICmdProc_Render;
 retVal -> Cleanup     = GUICmdProc_Cleanup;
 retVal -> HandleEvent = GUICmdProc_HandleEvent;

 return retVal;

}  /* end GUI_CreateCmdProc */

void _start_player ( FileContext* apFileCtx, FileContext* apSubCtx, SubtitleFormat aSubFmt ) {

 SMS_Player* lpPlayer = SMS_InitPlayer ( apFileCtx, apSubCtx, aSubFmt );

 if ( !lpPlayer ) {

  if ( g_CMedia == 1 ) CDVD_Stop ();

  GUI_Error ( STR_UNSUPPORTED_FILE.m_pStr );

 } else {

  lpPlayer -> Play    ();
  lpPlayer -> Destroy ();

  GUI_Initialize ( 0 );

 }  /* end else */

 g_CDDASpeed = 4;

 GUI_UpdateStatus ();

}  /* end _start_player */
