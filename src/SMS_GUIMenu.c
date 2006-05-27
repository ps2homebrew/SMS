/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005-2006 bix64
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUIMenu.h"
#include "SMS_GS.h"
#include "SMS_VIF.h"
#include "SMS_GUIcons.h"
#include "SMS_Config.h"
#include "SMS_PAD.h"

#include <malloc.h>
#include <string.h>

static void GUIMenu_Render ( GUIObject* apObj, int aCtx ) {

 GUIMenu*      lpMenu = ( GUIMenu* )apObj;
 int           lY     = lpMenu -> m_Y + 40;
 int           lLastY = lpMenu -> m_Y + lpMenu -> m_Height - 36;
 GUIMenuItem*  lpItem;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )lpMenu -> m_pState -> m_pTail -> m_Param;

 lpState -> m_pLastV = NULL;

 if ( !lpMenu -> m_pGSPacket ) {

  int            lLen  = lpState -> m_pTitle -> m_Len;
  int            lTW   = GSFont_Width ( lpState -> m_pTitle -> m_pStr, lLen );
  int            lTX   = lpMenu -> m_X + (  ( lpMenu -> m_Width - lTW ) >> 1  );
  int            lTDWC = GS_TXT_PACKET_SIZE( lLen );
  unsigned long* lpPkt = GSContext_NewList (   (  GS_RRT_PACKET_SIZE() << 1  ) + 6 + lTDWC   );
  unsigned long* lpDMA = lpPkt + (  GS_RRT_PACKET_SIZE() << 1  );

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )(  lpPkt +  GS_RRT_PACKET_SIZE() - 2  ),
   lpMenu -> m_X, lpMenu -> m_Y, lpMenu -> m_Width, lpMenu -> m_Height, -8, 0x70008000UL
  );
  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpPkt - 2 ), lpMenu -> m_X, lpMenu -> m_Y,
   lpMenu -> m_Width, lpMenu -> m_Height, 8, lpMenu -> m_Color
  );

  lpDMA[ 0 ] = GIF_TAG( 1, 1, 0, 0, GIFTAG_FLG_REGLIST, 4 );
  lpDMA[ 1 ] = GIFTAG_REGS_PRIM | ( GIFTAG_REGS_XYZ2 << 4 ) | ( GIFTAG_REGS_XYZ2 << 8 ) | ( GIFTAG_REGS_NOP << 12 );
  lpDMA[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_LINE, 0, 0, 0, 1, 1, 0, 0, 0 );
  lpDMA[ 3 ] = GS_XYZ( lpMenu -> m_X,                    lpMenu -> m_Y + 36, 0 );
  lpDMA[ 4 ] = GS_XYZ( lpMenu -> m_X + lpMenu ->m_Width, lpMenu -> m_Y + 36, 0 );
  lpDMA[ 5 ] = 0UL;

  g_GSCtx.m_TextColor = 0;
  GSFont_Render ( lpState -> m_pTitle -> m_pStr, lLen, lTX, lpMenu -> m_Y + 2, lpDMA + 6 );

  lpPkt[ -1 ] = VIF_DIRECT(  GS_RRT_PACKET_SIZE() + 3 + ( lTDWC >> 1 )  );

  lpMenu -> m_pGSPacket = lpPkt;

 }  /* end if */

 GSContext_CallList ( aCtx, lpMenu -> m_pGSPacket );

 for ( lpItem = lpState -> m_pFirst; lpItem <= lpState -> m_pLast; ++lpItem ) {

  int            lX     = lpMenu -> m_X      + 8;
  int            lWidth = lpMenu -> m_Width - 16;
  unsigned long* lpDMA;
  int            lLen;

  if ( lpItem -> m_IconLeft ) {

   lpDMA = GSContext_NewPacket (  aCtx, GS_TSP_PACKET_SIZE(), GSPaintMethod_Continue  );
   GUI_DrawIcon ( lpItem -> m_IconLeft, lX, lY, lpMenu -> m_IGroup, lpDMA );
   lX     += 34;
   lWidth -= 34;

  }  /* end for */

  g_GSCtx.m_TextColor = ( lpItem == lpState -> m_pFirst && lpItem != lpState -> m_pItems ) || ( lY >= lLastY - 34 && lpItem != lpState -> m_pLast ) ? 2 : 1;

  lLen = lpItem -> m_pOptionName -> m_Len;

  if ( lpItem -> m_IconRight ) lWidth -= lpMenu -> m_Width / 3;

  while (  GSFont_WidthEx ( lpItem -> m_pOptionName -> m_pStr, lLen, -6 ) > lWidth  ) --lLen;

  lpDMA = GSContext_NewPacket (  aCtx, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Continue  );
  GSFont_RenderEx ( lpItem -> m_pOptionName -> m_pStr, lLen, lX, lY, lpDMA, -7, 0 );

  if (  lpItem == lpState -> m_pCurr && !( lpState -> m_Flags & MENU_FLAGS_TEXT )  ) {

   unsigned long lColor = g_Palette[ g_Config.m_BrowserSCIdx - 1 ];

   lX = lpMenu -> m_X + 4;

   lpDMA = GSContext_NewPacket (  aCtx, GS_RRT_PACKET_SIZE(), GSPaintMethod_Continue  );
   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( lpDMA - 2 ), lX, lY, lpMenu -> m_Width - 8, 32, 8, ( lColor & 0x00FFFFFF ) | 0x10000000
   );
   lpDMA = GSContext_NewPacket (  aCtx, GS_RRT_PACKET_SIZE(), GSPaintMethod_Continue  );
   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( lpDMA - 2 ), lX, lY, lpMenu -> m_Width - 8, 32, -8, lColor
   );

  }  /* end if */

  if ( lpItem -> m_IconRight ) switch ( lpItem -> m_Type & 0x00FFFFFF ) {

   case MENU_ITEM_TYPE_TEXT: {

    int   lUWidth = lpItem -> m_Type >> 24;
    int   lLen    = (  ( SMString* )lpItem -> m_IconRight  ) -> m_Len;
    char* lpStr   = (  ( SMString* )lpItem -> m_IconRight  ) -> m_pStr;
    int   lDX     = -6;
    int   lWidth;
    int   lTxtW;

    if ( lUWidth ) {

     lWidth = lUWidth;
     lTxtW  = lpMenu -> m_Width - ( lWidth << 1 ) - 20;

    } else {

     lWidth = lpMenu -> m_Width / 3;
     lTxtW  = lWidth - 12;

    }  /* end else */

    while (  GSFont_WidthEx ( lpStr, lLen, lDX ) > lTxtW && lDX > -16  ) --lDX;
    while (  GSFont_WidthEx ( lpStr, lLen, lDX ) > lTxtW               ) --lLen;

    lpDMA = GSContext_NewPacket (  aCtx, GS_TXT_PACKET_SIZE( 1 ), GSPaintMethod_Continue  );
    GSFont_RenderEx ( g_ColonStr, 1, lpMenu -> m_X + ( lWidth << 1 ), lY, lpDMA, -6, 0 );

    lpDMA = GSContext_NewPacket (  aCtx, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Continue  );
    GSFont_RenderEx ( lpStr, lLen, lpMenu -> m_X + ( lWidth << 1 ) + 12, lY, lpDMA, lDX, 0 );

   } break;

   case MENU_ITEM_TYPE_PALIDX:

    lpDMA = GSContext_NewPacket (  aCtx, GS_RRT_PACKET_SIZE(), GSPaintMethod_Continue  );
    GS_RenderRoundRect (
     ( GSRoundRectPacket* )( lpDMA - 2 ), lpMenu -> m_X + lpMenu -> m_Width - 48, lY + 4,
     38, 22, 4, g_Palette[ *( unsigned int* )lpItem -> m_IconRight - 1 ]
    );
    lpDMA = GSContext_NewPacket (  aCtx, GS_RRT_PACKET_SIZE(), GSPaintMethod_Continue  );
    GS_RenderRoundRect (
     ( GSRoundRectPacket* )( lpDMA - 2 ), lpMenu -> m_X + lpMenu -> m_Width - 48, lY + 4,
     38, 22, -4, 0x20FFFFFF
    );

   break;

   default:

    lpDMA = GSContext_NewPacket (  aCtx, GS_TSP_PACKET_SIZE(), GSPaintMethod_Continue  );
    GUI_DrawIcon ( lpItem -> m_IconRight, lpMenu -> m_X + lpMenu -> m_Width - 38, lY, GUIcon_Misc, lpDMA );

  }  /* end switch */

  lY += 34;

  if ( lY >= lLastY && lpItem != lpState -> m_pLast ) {

   lpState -> m_pLastV = lpItem;
   break;

  }  /* end if */

 }  /* end for */

}  /* end GUIMenu_Render */

static void GUIMenu_Cleanup ( GUIObject* apObj ) {

 GUIMenu* lpMenu = ( GUIMenu* )apObj;

 if ( lpMenu -> m_pGSPacket ) {

  GSContext_DeleteList ( lpMenu -> m_pGSPacket );
  lpMenu -> m_pGSPacket = NULL;

 }  /* end if */

}  /* end GUIMenu_Cleanup */

int GUIMenu_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 GUIMenu*      lpMenu  = ( GUIMenu* )apObj;
 GUIMenuState* lpState = ( GUIMenuState* )( unsigned int )lpMenu -> m_pState -> m_pTail -> m_Param;

 switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case SMS_PAD_DOWN:

   if (  ( lpState -> m_Flags & MENU_FLAGS_TEXT ) && lpState -> m_pLastV  ) {

    ++lpState -> m_pFirst;
    lpMenu -> Redraw ( NULL );

   } else if ( lpState -> m_pCurr != lpState -> m_pLast ) {

    if ( lpState -> m_pLastV && lpState -> m_pCurr + 1 == lpState -> m_pLastV ) ++lpState -> m_pFirst;

    if ( lpState -> m_pCurr -> Leave ) lpState -> m_pCurr -> Leave ( lpMenu );

    ++lpState -> m_pCurr;
    lpMenu -> Redraw ( NULL );

    if ( lpState -> m_pCurr -> Enter ) lpState -> m_pCurr -> Enter ( lpMenu );

   }  /* end if */

  break;

  case SMS_PAD_UP:

   if (  ( lpState -> m_Flags & MENU_FLAGS_TEXT ) && lpState -> m_pFirst != lpState -> m_pItems  ) {

    --lpState -> m_pFirst;
    lpMenu -> Redraw ( NULL );

   } else if ( lpState -> m_pCurr != lpState -> m_pItems ) {

    if ( lpState -> m_pCurr - 1 == lpState -> m_pFirst && lpState -> m_pFirst != lpState -> m_pItems ) --lpState -> m_pFirst;

    if ( lpState -> m_pCurr -> Leave ) lpState -> m_pCurr -> Leave ( lpMenu );

    --lpState -> m_pCurr;
    lpMenu -> Redraw ( NULL );

    if ( lpState -> m_pCurr -> Enter ) lpState -> m_pCurr -> Enter ( lpMenu );

   }  /* end if */

  break;

  case SMS_PAD_CROSS:

   if ( lpState -> m_pCurr -> Handler ) lpState -> m_pCurr -> Handler ( lpMenu, 1 );

  break;

  case SMS_PAD_CIRCLE:

   if ( lpState -> m_pCurr -> Handler ) lpState -> m_pCurr -> Handler ( lpMenu, -1 );

  break;

 }  /* end switch */

 return anEvent & GUI_MSG_MOUNT_MASK ? GUIHResult_Void : GUIHResult_Handled;

}  /* end GUIMenu_HandleEvent */

GUIMenuState* GUI_MenuPushState ( GUIMenu* apMenu ) {

 GUIMenuState* retVal = ( GUIMenuState* )calloc (  1, sizeof ( GUIMenuState )  );

 SMS_ListPushBack ( apMenu -> m_pState, g_SlashStr ) -> m_Param = ( unsigned int )retVal;

 return retVal;

}  /* end GUI_MenuPushState */

int GUI_MenuPopState ( GUIMenu* apMenu ) {

 SMS_List* lpState = apMenu -> m_pState;

 free (  ( void* )( unsigned int )lpState -> m_pTail -> m_Param  );

 SMS_ListPopBack ( lpState );

 return lpState -> m_Size;

}  /* end GUI_MenuPopState */

GUIObject* GUI_CreateMenu ( void ) {

 GUIMenu* retVal = ( GUIMenu* )calloc (  1, sizeof ( GUIMenu )  );

 retVal -> Render      = GUIMenu_Render;
 retVal -> Cleanup     = GUIMenu_Cleanup;
 retVal -> HandleEvent = GUIMenu_HandleEvent;
 retVal -> m_IGroup    = GUIcon_Misc;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateMenu */
