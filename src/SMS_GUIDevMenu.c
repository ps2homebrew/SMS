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
#include "SMS_GS.h"
#include "SMS_Config.h"
#include "SMS_Locale.h"
#include "SMS_VIF.h"
#include "SMS_List.h"
#include "SMS_GUIcons.h"
#include "SMS_PAD.h"
#include "SMS_DMA.h"
#include "SMS_FileDir.h"
#include "SMS_FileContext.h"
#include "SMS_CDVD.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_RC.h"
#include "SMS_IOP.h"

#include <kernel.h>
#include <malloc.h>
#include <fileio.h>
#include <stdio.h>
#include <string.h>

typedef struct _DevMenuItem {

 int            m_XOffset;
 int            m_DevID;
 unsigned long* m_pGSPacket;

} _DevMenuItem;

typedef struct GUIDevMenu {

 DECLARE_GUI_OBJECT()

 SMS_List*      m_pDevList;
 SMS_ListNode*  m_pActive;
 SMS_ListNode*  m_pSelected;
 int            m_XOffset;
 unsigned long* m_pSelRect;
 unsigned long* m_pActRect;
 unsigned int*  m_pColor;

} GUIDevMenu;

static GSBitBltPacket s_BitBlt;

static void GUIDevMenu_RenderSelRect ( GUIDevMenu* apMenu, _DevMenuItem* apActive, _DevMenuItem* apSelected ) {

 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( apMenu -> m_pActRect - 2 ),
  apActive -> m_XOffset - 1, 4, 48, 48, 12, ( g_Palette[ g_Config.m_BrowserSCIdx - 1 ] & 0x00FFFFFF ) | 0x10000000
 );
 GS_RenderRoundRect (
  ( GSRoundRectPacket* )( apMenu -> m_pSelRect - 2 ),
  apSelected -> m_XOffset - 1, 4, 48, 48, -12, ( g_Palette[ g_Config.m_BrowserSCIdx - 1 ] & 0x00FFFFFF ) | 0x80000000
 );

}  /* end GUIDevMenu_RenderSelRect */

static void GUIDevMenu_Render ( GUIObject* apObj, int aCtx ) {

 GUIDevMenu*   lpMenu = ( GUIDevMenu* )apObj;
 SMS_ListNode* lpNode = lpMenu -> m_pDevList -> m_pHead;
 int           lIdx   = 0;

 if ( !lpMenu -> m_pGSPacket ) {

  int            lPktSize = GS_RRT_PACKET_SIZE() + GS_TXT_PACKET_SIZE( STR_AVAILABLE_MEDIA.m_Len );
  int            lCumSize = lPktSize;
  unsigned long* lpDMA;

  lpMenu -> m_XOffset = GSFont_WidthEx ( STR_AVAILABLE_MEDIA.m_pStr, STR_AVAILABLE_MEDIA.m_Len, 4 );

  if ( !lpNode ) lCumSize += GS_TXT_PACKET_SIZE( STR_NONE.m_Len );

  lpDMA = GSContext_NewList ( lCumSize );

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpDMA - 2 ), 0, 1, g_GSCtx.m_Width - 1, 54, -12,
   g_Palette[ *lpMenu -> m_pColor - 1 ]
  );
  *( lpDMA - 1 ) = VIF_DIRECT( lCumSize >> 1 );
  g_GSCtx.m_TextColor = 0;
  GSFont_RenderEx (
   STR_AVAILABLE_MEDIA.m_pStr, STR_AVAILABLE_MEDIA.m_Len,
   8, 12, lpDMA + GS_RRT_PACKET_SIZE(), 4, 0
  );

  if ( !lpNode ) GSFont_Render (
                  STR_NONE.m_pStr, STR_NONE.m_Len,
                  lpMenu -> m_XOffset, 12, lpDMA + lPktSize
                 );

  lpMenu -> m_pGSPacket = lpDMA;

  if ( lpMenu -> m_pActRect && lpMenu -> m_pActive ) GUIDevMenu_RenderSelRect (  lpMenu, ( _DevMenuItem* )( unsigned int )lpMenu -> m_pActive -> m_Param, ( _DevMenuItem* )( unsigned int )lpMenu -> m_pSelected -> m_Param  );

  GSContext_InitBitBlt (
   &s_BitBlt, 0, 0, 0, g_GSCtx.m_Width, 55,
   g_GSCtx.m_VRAMPtr2, 0, 0
  );

 }  /* end if */

 GSContext_CallList ( aCtx, lpMenu -> m_pGSPacket );

 if ( lpNode ) {

  while ( lpNode ) {

   _DevMenuItem* lpItem = ( _DevMenuItem* )( unsigned int )lpNode -> m_Param;

   lpItem -> m_XOffset = lpMenu -> m_XOffset + 54 * lIdx++;
   GUI_DrawIcon ( lpItem -> m_DevID, lpItem -> m_XOffset, 2, GUIcon_Device, lpItem -> m_pGSPacket );
   GSContext_CallList ( aCtx, lpItem -> m_pGSPacket );

   lpNode = lpNode -> m_pNext;

  }  /* end while */

  GUIDevMenu_RenderSelRect (  lpMenu, ( _DevMenuItem* )( unsigned int )lpMenu -> m_pActive -> m_Param, ( _DevMenuItem* )( unsigned int )lpMenu -> m_pSelected -> m_Param  );
  GSContext_CallList ( aCtx, lpMenu -> m_pActRect );
  GSContext_CallList ( aCtx, lpMenu -> m_pSelRect );

 }  /* end if */

}  /* end GUIDevMenu_Render */

static void GUIDevMenu_Cleanup ( GUIObject* apObj ) {

 GSContext_DeleteList ( apObj -> m_pGSPacket );
 apObj -> m_pGSPacket = NULL;

}  /* end GUIDevMenu_Cleanup */

static int GUIDevMenu_Redraw ( GUIDevMenu* apMenu ) {

 GUIDevMenu_Cleanup (  ( GUIObject* )apMenu  );

 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
 GUIDevMenu_Render (  ( GUIObject* )apMenu, 1  );

 GS_VSync ();
 GSContext_BitBlt ( &s_BitBlt );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

 return GUIHResult_Handled;

}  /* end GUIDevMenu_Redraw */

static int GUIDevMenu_HandleMount ( GUIDevMenu* apMenu, unsigned int aMount ) {

 int       lDevID = (  aMount & ~( GUI_MSG_MOUNT_BIT >> 16 )  ) - 1;
 SMS_List* lpList = apMenu -> m_pDevList;

 if (  aMount & ( GUI_MSG_MOUNT_BIT >> 16 )  ) {

  _DevMenuItem* lpItem;

  if ( lDevID == 6 ) {

   if ( g_SMBUnit < 0 ) {

    char lBuff[ 512 ];

    switch ( g_SMBError )  {

     case SMB_ERROR_NEGOTIATE:
      sprintf ( lBuff, STR_PROT_NEG_ERROR.m_pStr, g_SMBServerError );
     break;

     case SMB_ERROR_LOGIN:
      sprintf ( lBuff, STR_LOGIN_ERROR.m_pStr, g_SMBServerError );
     break;

     default: strcpy ( lBuff, STR_COMM_ERROR.m_pStr );

    }  /* end switch */

    GUI_Error ( lBuff );

    return GUIHResult_Handled;

   } else g_IOPFlags |= SMS_IOPF_SMBLOGIN;

  }  /* end if */

  lpItem = ( _DevMenuItem* )malloc (  sizeof ( _DevMenuItem )  );

  if ( lDevID == 3 ) {

   g_pCDDACtx = CDDA_InitContext ( 0UL );

   if ( g_pCDDACtx ) lDevID = 1;

  }  /* end if */

  SMS_ListPushBack ( lpList, g_pDevName[ lDevID ] ) -> m_Param = ( unsigned int )lpItem;

  lpItem -> m_DevID     = lDevID;
  lpItem -> m_XOffset   = apMenu -> m_XOffset + 54 * ( lpList -> m_Size - 1 );
  lpItem -> m_pGSPacket = GSContext_NewList (  GS_TSP_PACKET_SIZE()  );

  if ( lpList -> m_Size == 1 ) {

   apMenu -> m_pActRect = GSContext_NewList (  GS_RRT_PACKET_SIZE ()  );
   apMenu -> m_pSelRect = GSContext_NewList (  GS_RRT_PACKET_SIZE ()  );

   GUIDevMenu_RenderSelRect ( apMenu, lpItem, lpItem );

   apMenu -> m_pActive   =
   apMenu -> m_pSelected = lpList -> m_pTail;

   g_CMedia = lDevID;

   GUI_PostMessage ( GUI_MSG_MEDIA_SELECTED );

  } else {

   if (  ( lDevID & 1 ) && g_pCDDACtx  ) CDVD_Stop ();

   GUI_Status ( g_CWD );

  }  /* end else */

  if ( lDevID == 0 || lDevID == 4 || lDevID == 6 ) SPU_PlaySound ( SMSound_Mount, g_Config.m_PlayerVolume );

 } else {

  SMS_ListNode* lpNode = lpList -> m_pHead;
  unsigned long lMsg;

  if ( lDevID == 3 && g_pCDDACtx ) {

   CDDA_DestroyContext ( g_pCDDACtx );

   g_pCDDACtx = NULL;
   lDevID     = 1;

  }  /* end if */

  while ( lpNode ) {

   _DevMenuItem* lpItem = ( _DevMenuItem* )( unsigned int )lpNode -> m_Param;

   if ( lpItem -> m_DevID == lDevID ) {

    SMS_ListNode* lpCurNode = lpNode -> m_pNext;

    while ( lpCurNode ) {

     _DevMenuItem* lpCurItem = ( _DevMenuItem* )( unsigned int )lpCurNode -> m_Param;

     lpCurItem -> m_XOffset -= 55;
     GUI_DrawIcon (
      lpCurItem -> m_DevID,
      lpCurItem -> m_XOffset, 2, GUIcon_Device,
      lpCurItem -> m_pGSPacket
     );
     lpCurNode = lpCurNode -> m_pNext;

    }  /* end while */

    if ( apMenu -> m_pActive   == lpNode ) apMenu -> m_pActive   = lpNode -> m_pPrev ? lpNode -> m_pPrev : lpNode -> m_pNext;
    if ( apMenu -> m_pSelected == lpNode ) apMenu -> m_pSelected = lpNode -> m_pPrev ? lpNode -> m_pPrev : lpNode -> m_pNext;

    SMS_ListRemove ( lpList, lpNode );

    GSContext_DeleteList ( lpItem -> m_pGSPacket );
    free ( lpItem );

    if ( apMenu -> m_pActive ) {

     int lNewMedia = (  ( _DevMenuItem* )( unsigned int )apMenu -> m_pActive -> m_Param  ) -> m_DevID;

     if ( g_CMedia != lNewMedia ) {

      lMsg     = GUI_MSG_MEDIA_SELECTED;
      g_CMedia = lNewMedia;

     } else lMsg = 0UL;

     GUIDevMenu_RenderSelRect (  apMenu, ( _DevMenuItem* )( unsigned int )apMenu -> m_pActive -> m_Param, ( _DevMenuItem* )( unsigned int )apMenu -> m_pSelected -> m_Param  );

    } else lMsg = GUI_MSG_MEDIA_REMOVED;

    GUI_PostMessage ( lMsg );

    if ( lDevID == 0 || lDevID == 4 || lDevID == 6 ) SPU_PlaySound ( SMSound_UMount, g_Config.m_PlayerVolume );

    if ( lDevID == 6 ) GUI_PostMessage ( GUI_MSG_MOUNT_BIT | GUI_MSG_LOGIN );

    break;

   }  /* end if */

   lpNode = lpNode -> m_pNext;

  }  /* end while */

  if ( !lpList -> m_Size ) {

   GSContext_DeleteList ( apMenu -> m_pActRect );
   GSContext_DeleteList ( apMenu -> m_pSelRect );

   g_CMedia = -1;
   GUI_Status ( STR_WAITING_FOR_MEDIA.m_pStr );

  }  /* end if */

 }  /* end else */

 return GUIDevMenu_Redraw ( apMenu );

}  /* end GUIDevMenu_HandleMount */

static int GUIDevMenu_HandlePad ( GUIDevMenu* apMenu, unsigned int aPad ) {

 int retVal       = GUIHResult_Void;
 SMS_List* lpList = apMenu -> m_pDevList;

 if ( lpList -> m_Size ) switch ( aPad ) {

  case SMS_PAD_LEFT:

   apMenu -> m_pSelected = apMenu -> m_pSelected -> m_pPrev ? apMenu -> m_pSelected -> m_pPrev : lpList -> m_pTail;

  goto redraw;

  case SMS_PAD_RIGHT:

   apMenu -> m_pSelected = apMenu -> m_pSelected -> m_pNext ? apMenu -> m_pSelected -> m_pNext : lpList -> m_pHead;
redraw:
   GUIDevMenu_RenderSelRect (  apMenu, ( _DevMenuItem* )( unsigned int )apMenu -> m_pActive -> m_Param, ( _DevMenuItem* )( unsigned int )apMenu -> m_pSelected -> m_Param  );
   GUIDevMenu_Redraw ( apMenu );

   retVal = GUIHResult_Handled;

  break;

 }  /* end if */

 switch ( aPad ) {

  case SMS_PAD_UP  :
  case SMS_PAD_DOWN:

   retVal = GUIHResult_ChangeFocus;

  break;

  case RC_PLAY      :
  case RC_ENTER     :
  case SMS_PAD_CROSS:

   if ( apMenu -> m_pSelected ) {

    _DevMenuItem* lpItem = ( _DevMenuItem* )( unsigned int )apMenu -> m_pSelected -> m_Param; 

    apMenu -> m_pActive = apMenu -> m_pSelected;
    GUIDevMenu_RenderSelRect (  apMenu, ( _DevMenuItem* )( unsigned int )apMenu -> m_pActive -> m_Param, ( _DevMenuItem* )( unsigned int )apMenu -> m_pSelected -> m_Param  );
    GUIDevMenu_Redraw ( apMenu );

    g_CMedia = lpItem -> m_DevID;

    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );
    GUI_PostMessage ( GUI_MSG_MEDIA_SELECTED );

    retVal = GUIHResult_Handled;

   }  /* end if */

  break;

 }  /* end switch */

 return retVal;

}  /* end GUIDevMenu_HandlePad */

static int GUIDevMenu_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 int         retVal = GUIHResult_Void;
 GUIDevMenu* lpMenu = ( GUIDevMenu* )apObj;

 if ( anEvent & GUI_MSG_MOUNT_MASK ) {

  int lDevID = ( anEvent >> 16 ) & 0xFF;

  if ( lDevID == 0x18 ) {

   int lFD = fioDopen ( g_pSMBS );

   if ( lFD >= 0 ) {

    g_SMBLoginInfo.m_fAsync = 1;
    fioIoctl ( lFD, SMB_IOCTL_LOGIN, &g_SMBLoginInfo );
    fioDclose ( lFD );

   }  /* end if */

   retVal = GUIHResult_Handled;

  } else retVal = GUIDevMenu_HandleMount ( lpMenu, lDevID );

 } else if ( anEvent & GUI_MSG_PAD_MASK )

  retVal = GUIDevMenu_HandlePad ( lpMenu, anEvent & 0xFFFF );

 return retVal;

}  /* end GUIDevMenu_HandleEvent */

static void GUIDevMenu_SetFocus ( GUIObject* apObj, int afSet ) {

 GUIDevMenu* lpMenu  = ( GUIDevMenu* )apObj;

 lpMenu -> m_pColor = &( afSet ? g_Config.m_BrowserABCIdx : g_Config.m_BrowserIBCIdx );

 GUIDevMenu_Redraw ( lpMenu );

}  /* end GUIDevMenu_SetFocus */

GUIObject* GUI_CreateDevMenu ( void ) {

 GUIDevMenu* retVal = ( GUIDevMenu* )calloc (  1, sizeof ( GUIDevMenu )  );

 retVal -> Render      = GUIDevMenu_Render;
 retVal -> HandleEvent = GUIDevMenu_HandleEvent;
 retVal -> SetFocus    = GUIDevMenu_SetFocus;
 retVal -> Cleanup     = GUIDevMenu_Cleanup;
 retVal -> m_pDevList  = SMS_ListInit ();
 retVal -> m_pColor    = &g_Config.m_BrowserIBCIdx;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateDevMenu */
