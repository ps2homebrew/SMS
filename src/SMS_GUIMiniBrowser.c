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
#include "SMS_GUIMenu.h"
#include "SMS_FileContext.h"
#include "SMS_FileDir.h"
#include "SMS_List.h"
#include "SMS_Locale.h"
#include "SMS_GUIcons.h"
#include "SMS_GS.h"
#include "SMS_PAD.h"
#include "SMS_RC.h"
#include "SMS_SubtitleContext.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_Config.h"

#include <malloc.h>
#include <string.h>

extern void GUIMenuSMS_Redraw ( GUIMenu* );

int ( *CtxMenu_HandleEventBase ) ( GUIObject*, u64           );

static FileContext*   s_pFileCtx;
static FileContext*   s_pFileCtxSub;
static SubtitleFormat s_SubFmt;

int CtxMenu_HandleEvent ( GUIObject* apObj, u64           anEvent ) {

 switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case RC_RETURN       :
  case RC_RESET        :
  case SMS_PAD_TRIANGLE:

   (  ( GUIMenu* )apObj  ) -> m_pUserData = ( void* )1;
quit:
   if (  !GUI_QuitPosted ()  ) GUI_PostMessage ( GUI_MSG_QUIT );

  return GUIHResult_Handled;

  case RC_ENTER      :
  case SMS_PAD_CIRCLE:
  case SMS_PAD_CROSS :

   CtxMenu_HandleEventBase ( apObj, anEvent );

  goto quit;

 }  /* end switch */

 return CtxMenu_HandleEventBase ( apObj, anEvent );

}  /* end CtxMenu_HandleEvent */

static void _handler ( GUIMenu* apMenu, int aDir ) {

 char          lPath[ 1024 ];
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param;
 char*         lpName  = g_CMedia == 1 && g_pCDDACtx ? &lPath[ 7 ] : lPath;

 SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );

 strcpy ( lPath, g_CWD );

 if (  lPath[ strlen ( lPath ) - 1 ] != '/'  ) strcat ( lPath, g_SlashStr );

 strcat ( lPath, lpState -> m_pCurr -> m_pOptionName -> m_pStr );

 s_pFileCtxSub = s_pFileCtx -> Open ( lpName, s_pFileCtx -> m_pOpenParam );

 if ( s_pFileCtxSub ) {

  lpName = lPath + strlen ( lPath ) - 3;

  if (  !strcasecmp ( lpName, g_pSrtStr )  )

   s_SubFmt = SubtitleFormat_SRT;

  else if (  !strcasecmp ( lpName, g_pSubStr ) || !strcasecmp ( lpName, g_pTxtStr )  )

   s_SubFmt = SubtitleFormat_SUB;

 }  /* end if */

}  /* end _handler */

static int _is_sub ( SMS_ListNode* apNode ) {

 int lLen = strlen (  _STR( apNode )  );

 return lLen > 4 && _STR( apNode )[ lLen - 4 ] == '.' &&
        (  !strcasecmp (  _STR( apNode ) + lLen - 3, g_pSubStr  ) ||
           !strcasecmp (  _STR( apNode ) + lLen - 3, g_pSrtStr  ) ||
           !strcasecmp (  _STR( apNode ) + lLen - 3, g_pTxtStr  )
        ) && apNode -> m_Param == GUICON_FILE;

}  /* end _is_sub */

FileContext* GUI_MiniBrowser ( FileContext* apCtx, char* apPath, void** appType ) {

 int           i = 0, lnSubs = 0;
 SMS_ListNode* lpNode = g_pFileList -> m_pHead;
 SMString*     lpNames;
 GUIMenuItem*  lpMenuItems;
 GUIMenu*      lpMenu;
 int           lWidth;
 int           lHeight;
 GUIMenuState* lpState;

 s_pFileCtx    = apCtx;
 s_pFileCtxSub = NULL;

 while ( lpNode ) {

  if (  _is_sub ( lpNode )  ) ++lnSubs;

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 if ( !lnSubs ) return NULL;

 lpMenuItems = ( GUIMenuItem* )calloc (  lnSubs, sizeof ( GUIMenuItem )  );
 lpNames     = ( SMString*    )calloc (  lnSubs, sizeof ( SMString    )  );
 lpNode      = g_pFileList -> m_pHead;

 while ( lpNode ) {

  if (  _is_sub ( lpNode )  ) {

   lpNames[ i ].m_pStr = _STR( lpNode );
   lpNames[ i ].m_Len  = strlen (  _STR( lpNode )  );

   lpMenuItems[ i ].m_IconLeft    = GUICON_FILE;
   lpMenuItems[ i ].Handler       = _handler;
   lpMenuItems[ i ].m_pOptionName = &lpNames[ i ];

   ++i;

  }  /* end if */

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 lpMenu  = ( GUIMenu* )GUI_CreateMenu ();
 lWidth  = g_GSCtx.m_Width  / 1.7F;
 lHeight = g_GSCtx.m_Height / 1.7F;

 CtxMenu_HandleEventBase = lpMenu -> HandleEvent;

 lpMenu -> m_Color      = 0x80301010UL;
 lpMenu -> m_X          = g_GSCtx.m_Width  - lWidth - 32;
 lpMenu -> m_Y          = (  ( g_GSCtx.m_Height - lHeight ) >> 1  ) + 8;
 lpMenu -> m_Width      = lWidth;
 lpMenu -> m_Height     = lHeight;
 lpMenu -> m_pActiveObj = g_pActiveNode;
 lpMenu -> m_pState     = SMS_ListInit ();

 lpMenu -> Redraw      = GUIMenuSMS_Redraw;
 lpMenu -> HandleEvent = CtxMenu_HandleEvent;
 lpMenu -> m_IGroup    = GUIcon_Browser;

 lpState = GUI_MenuPushState ( lpMenu );
 lpState -> m_pTitle = &STR_SELECT_SUBTITLES;
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = lpMenuItems;
 lpState -> m_pLast  = &lpMenuItems[ lnSubs - 1 ];

 GUI_AddObject (  STR_SELECT_SUBTITLES.m_pStr, ( GUIObject* )lpMenu  );
 lpMenu -> Redraw ( lpMenu );
 GUI_Run ();
 GUI_DeleteObject ( STR_SELECT_SUBTITLES.m_pStr );
 GUI_Redraw ( GUIRedrawMethod_Redraw );

 free ( lpNames );

 for ( i = 0; i < lnSubs; ++i )
  if ( lpMenuItems[ i ].m_pIconLeftPack ) free ( lpMenuItems[ i ].m_pIconLeftPack );

 free ( lpMenuItems );

 *appType = ( void* )s_SubFmt;

 return s_pFileCtxSub;

}  /* end GUI_MiniBrowser */
