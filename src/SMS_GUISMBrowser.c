/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUIMenu.h"
#include "SMS_Config.h"
#include "SMS_Locale.h"
#include "SMS_FileDir.h"
#include "SMS_IOP.h"
#include "SMS_GUIcons.h"
#include "SMS_PgInd.h"

#include <malloc.h>
#include <string.h>
#include <fileio.h>

typedef struct SMBInfo {

 SMString     m_Title;
 GUIMenuItem* m_pItems;
 SMString*    m_pStrings;
 int          m_CurIdx;

} SMBInfo;

extern void GUIMenuSMS_UpdateStatus ( GUIMenu*                  );
extern int  GUIMenuSMS_HandleEvent  ( GUIObject*, unsigned long );

static int GUIMenuSMB_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 return GUIMenuSMS_HandleEvent ( apObj, anEvent );

}  /* end GUIMenuSMB_HandleEvent */

static void _user_data_destructor ( void* apArg ) {

 SMBInfo* lpInfo = ( SMBInfo* )apArg;

 free ( lpInfo -> m_Title.m_pStr );
 free ( lpInfo -> m_pItems       );
 free ( lpInfo -> m_pStrings     );

}  /* end _user_data_destructor */

static void _smb_handler ( GUIMenu* apMenu, int aDir ) {

 GUIMenuState* lpState = (  ( GUIMenuState* )( unsigned int )apMenu -> m_pState -> m_pTail -> m_Param  );
 SMS_ListNode* lpNode  = SMS_ListFind ( g_Config.m_pSMBList, lpState -> m_pCurr -> m_pOptionName -> m_pStr );
 SMBLoginInfo* lpInfo  = ( SMBLoginInfo* )( unsigned int )lpNode -> m_Param;

 if (  strcmp ( lpInfo -> m_ServerIP, g_Config.m_SMBIP )  ) {

  SMBInfo* lpMenuInfo = ( SMBInfo* )lpState -> m_pUserData;

  lpMenuInfo -> m_pItems[ lpMenuInfo -> m_CurIdx ].m_IconRight = 0;
  lpState -> m_pCurr -> m_IconRight = GUICON_ON;
  lpMenuInfo -> m_CurIdx            = lpState -> m_pCurr - lpMenuInfo -> m_pItems;

  strcpy ( g_Config.m_SMBIP, lpInfo -> m_ServerIP );

  if ( g_IOPFlags & SMS_IOPF_NET_UP ) {

   int lFD = fioDopen ( g_pSMBS );

   if ( lFD >= 0 ) {

    if ( g_IOPFlags & SMS_IOPF_SMBLOGIN ) {
redo:
     fioIoctl ( lFD, SMB_IOCTL_LOGOUT, &g_SMBUnit );
     g_SMBU      = 0x80000000;
     g_IOPFlags &= ~SMS_IOPF_SMBLOGIN;
     GUI_PostMessage ( GUI_MSG_SMB );

    } else {

     int lSts;

     GUI_Status ( STR_SMB_CLOSING.m_pStr );
     SMS_PgIndStart ();
      lSts = fioIoctl ( lFD, SMB_IOCTL_STOPC, &g_SMBUnit );
     SMS_PgIndStop ();
     if ( lSts < 0 ) goto redo;

     GUI_PostMessage ( GUI_MSG_MOUNT_BIT | GUI_MSG_LOGIN );

    }  /* end else */

    fioDclose ( lFD );
    GUIMenuSMS_UpdateStatus ( apMenu );

   }  /* end if */

  }  /* end if */

  apMenu -> Redraw ( apMenu );

 }  /* end if */

}  /* end _smb_handler */

void _smb_menu ( GUIMenu* apMenu ) {

 int           i, lnItems;
 GUIMenuState* lpState = GUI_MenuPushState ( apMenu );
 SMBInfo*      lpInfo  = ( SMBInfo* )malloc (  sizeof ( SMBInfo )  );
 SMS_ListNode* lpNode  = g_Config.m_pSMBList -> m_pHead;

 lnItems = g_Config.m_pSMBList -> m_Size;

 lpInfo -> m_Title.m_pStr = ( char* )calloc ( 1, i = STR_SMB_SERVER.m_Len - 2 );
 strncpy ( lpInfo -> m_Title.m_pStr, STR_SMB_SERVER.m_pStr, --i );
 lpInfo -> m_Title.m_Len  = i;

 lpInfo -> m_pItems   = ( GUIMenuItem* )calloc (  lnItems, sizeof ( GUIMenuItem )  );
 lpInfo -> m_pStrings = ( SMString*    )calloc (  lnItems, sizeof ( SMString    )  );
 lpInfo -> m_CurIdx   = 0;

 for ( i = 0; i < lnItems; ++i, lpNode = lpNode -> m_pNext ) {

  lpInfo -> m_pItems[ i ].m_pOptionName = lpInfo -> m_pStrings + i;
  lpInfo -> m_pItems[ i ].Handler       = _smb_handler;

  lpInfo -> m_pStrings[ i ].m_pStr = _STR( lpNode );
  lpInfo -> m_pStrings[ i ].m_Len  = strlen (  _STR( lpNode )  );

  if (  !strcmp (
          g_Config.m_SMBIP, (  ( SMBLoginInfo* )( unsigned int )lpNode -> m_Param  ) -> m_ServerIP
         )
  ) {
   lpInfo -> m_CurIdx                  = i;
   lpInfo -> m_pItems[ i ].m_IconRight = GUICON_ON;
  }  /* end if */

 }  /* end for */

 if ( !lpInfo -> m_CurIdx ) lpInfo -> m_pItems -> m_IconRight = GUICON_ON;

 lpState -> m_pItems           =
 lpState -> m_pFirst           =
 lpState -> m_pCurr            =  lpInfo -> m_pItems;
 lpState -> m_pLast            =  lpInfo -> m_pItems + lnItems - 1;
 lpState -> m_pTitle           = &lpInfo -> m_Title;
 lpState -> m_pUserData        =  lpInfo;
 lpState -> UserDataDestructor =  _user_data_destructor;

 lpState -> HandleEvent = apMenu -> HandleEvent;
 apMenu  -> HandleEvent = GUIMenuSMB_HandleEvent;

 GUIMenuSMS_UpdateStatus ( apMenu );
 apMenu -> Redraw ( apMenu );

}  /* end _smb_menu */

