/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006/7 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUI.h"
#include "SMS_GUIMenu.h"
#include "SMS_FileContext.h"
#include "SMS_Player.h"
#include "SMS_Locale.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "SMS_FileDir.h"
#include "SMS_GUIcons.h"
#include "SMS_SubtitleContext.h"
#include "SMS_GS.h"
#include "SMS_RC.h"
#include "SMS_PAD.h"

#include <malloc.h>
#include <string.h>

extern SMS_Player s_Player;

extern void** SMS_OpenMediaFile ( const char*, int );

static void _start_player   ( FileContext*, FileContext*, SubtitleFormat );
static void _start_playlist ( SMS_List*                                  );

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

  if (   (  ( unsigned int )lpParam[ 0 ] & 0xC0000000  ) != 0xC0000000   )
   _start_player (  ( FileContext* )lpParam[ 0 ], ( FileContext* )lpParam[ 1 ], ( SubtitleFormat )lpParam[ 2 ]  );
  else _start_playlist (
   ( SMS_List* )(   (  ( unsigned int )lpParam[ 0 ] << 2  ) >> 2   )
  );

  g_CDDASpeed = 4;

  GUI_UpdateStatus ();

  if ( lEvent == GUI_MSG_FILE )

   free ( lpParam );

  else RestoreFileDir ( lpParam );

  retVal = GUIHResult_Handled;

  GSFont_Unload ();

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

  GUI_Error ( s_Player.m_pErrorMsg -> m_pStr );

 } else {

  lpPlayer -> Play    ( lpPlayer );
  lpPlayer -> Destroy ( lpPlayer );

  GUI_Initialize ( 0 );

 }  /* end else */

}  /* end _start_player */

static int _playlist_menu ( SMS_List*, SMS_ListNode** );

static void _start_playlist ( SMS_List* apList ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 if (  !_playlist_menu ( apList, &lpNode )  ) {

  while ( lpNode ) {

   int lfUserStop = 0;

   if ( !lpNode -> m_Param ) {

    void**      lpParam  = SMS_OpenMediaFile (  _STR( lpNode ), SMS_FA_FLAGS_AVI  );
    SMS_Player* lpPlayer = SMS_InitPlayer (
     ( FileContext* )lpParam[ 0 ], ( FileContext* )lpParam[ 1 ], ( SubtitleFormat )lpParam[ 2 ]
    );

    if ( lpPlayer ) {

     lfUserStop = lpPlayer -> Play ( lpPlayer );
     lpPlayer -> Destroy ( lpPlayer );

     GUI_Initialize ( -1 );

    }  /* end if */

   }  /* end if */

   if ( lfUserStop && lpNode -> m_pNext ) {
    if (  _playlist_menu ( apList, &lpNode )  ) break;
   } else lpNode = lpNode -> m_pNext;

  }  /* end while */

  GUI_Initialize ( 0 );

 } else GUI_Redraw ( GUIRedrawMethod_Redraw );

}  /* end _start_playlist */

extern int ( *CtxMenu_HandleEventBase ) ( GUIObject*, unsigned long );
extern int CtxMenu_HandleEvent          ( GUIObject*, unsigned long );

static GUIMenu* s_pMenu;

static void _file_handler ( GUIMenu* apMenu, int aDir ) {


}  /* end _file_handler */

static void _playlist_menu_redraw ( GUIMenu* apMenu ) {

 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
 s_pMenu -> Render (  ( GUIObject* )s_pMenu, 1  );

 GS_VSync ();
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

}  /* end _playlist_menu_redraw */

static int _playlist_menu ( SMS_List* apList, SMS_ListNode** appNode ) {

 int           lCount  = 0;
 SMS_ListNode* lpNode  = apList -> m_pHead;
 GUIMenu*      lpMenu  = ( GUIMenu* )GUI_CreateMenu ();
 int           lWidth  = g_GSCtx.m_Width  - ( g_GSCtx.m_Width  >> 2 );
 int           lHeight = g_GSCtx.m_Height - ( g_GSCtx.m_Height >> 2 );
 char*         lpSel   = NULL;
 GUIMenuItem*  lpItems;
 GUIMenuState* lpState;
 SMString*     lpNames;

 CtxMenu_HandleEventBase = lpMenu -> HandleEvent;
 s_pMenu                 = lpMenu;

 lpMenu -> m_Color      = 0x80301010UL;
 lpMenu -> m_X          = ( g_GSCtx.m_Width  - lWidth  ) >> 1;
 lpMenu -> m_Y          = (  ( g_GSCtx.m_Height - lHeight ) >> 1  ) + 8;
 lpMenu -> m_Width      = lWidth;
 lpMenu -> m_Height     = lHeight;
 lpMenu -> m_pActiveObj = g_pActiveNode;
 lpMenu -> m_pState     = SMS_ListInit ();
 lpMenu -> m_IGroup     = GUIcon_Browser;
 lpMenu -> Redraw       = _playlist_menu_redraw;
 lpMenu -> HandleEvent  = CtxMenu_HandleEvent;

 while ( lpNode ) {
  if ( !lpNode -> m_Param ) {
   ++lCount;
   if (   !strcmp (  _STR( lpNode ), _STR( appNode[ 0 ] )  )   ) lpSel = _STR( lpNode );
  }  /* end if */
  lpNode = lpNode -> m_pNext;
 }  /* end while */

 lpNode  = apList -> m_pHead;
 lpItems = ( GUIMenuItem* )calloc (  lCount, sizeof ( GUIMenuItem )  );
 lpNames = ( SMString*    )calloc (  lCount, sizeof ( SMString    )  );
 lCount  = 0;

 while ( lpNode ) {
  if ( !lpNode -> m_Param ) {
   lpNames[ lCount ].m_Len         = strlen (  _STR( lpNode )  );
   lpNames[ lCount ].m_pStr        = _STR( lpNode );
   lpItems[ lCount ].m_pOptionName = &lpNames[ lCount ];
   lpItems[ lCount ].m_IconLeft    = GUICON_AVI;
   lpItems[ lCount ].Handler       = _file_handler;
   ++lCount;
  }  /* end if */
  lpNode = lpNode -> m_pNext;
 }  /* end while */

 lpState = GUI_MenuPushState ( lpMenu );
 lpState -> m_pTitle = &STR_SELECT_FILE;
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = lpItems;
 lpState -> m_pLast  = &lpItems[ lCount - 1 ];

 GUIMenu_SelectItemByName ( lpMenu, lpSel );

 GUI_AddObject (  STR_SELECT_FILE.m_pStr, ( GUIObject* )lpMenu  );

 lpMenu -> Redraw ( lpMenu );

 GUI_Run ();

 appNode[ 0 ] = SMS_ListFind ( apList, lpState -> m_pCurr -> m_pOptionName -> m_pStr );

 GUI_DeleteObject ( STR_SELECT_FILE.m_pStr );

 free ( lpNames );
 free ( lpItems );

 return ( int )lpMenu -> m_pUserData;

}  /* end _playlist_menu */
