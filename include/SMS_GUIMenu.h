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
#ifndef __SMS_GUIMenu_H
#define __SMS_GUIMenu_H

#ifndef __SMS_GUI_H
#include "SMS_GUI.h"
#endif  /* __SMS_GUI_H */

#ifndef __SMS_Locale_H
#include "SMS_Locale.h"
#endif  /* __SMS_Locale_H */

#ifndef __SMS_List_H
#include "SMS_List.h"
#endif  /* __SMS_List_H */

#define MENU_ITEM_TYPE_TEXT   0x00000001
#define MENU_ITEM_TYPE_PALIDX 0x00000002
#define MENU_ITEM_HIDDEN      0x00800000

#define MENU_FLAGS_TEXT 0x00000001

struct GUIMenu;

typedef struct GUIMenuItem {

 unsigned int   m_Type;
 SMString*      m_pOptionName;
 unsigned int   m_IconLeft;
 unsigned int   m_IconRight;

 void ( *Handler ) ( struct GUIMenu*, int  );
 void ( *Enter   ) ( struct GUIMenu*       );
 void ( *Leave   ) ( struct GUIMenu*       );

 unsigned long* m_pIconLeftPack;
 unsigned long* m_pIconRightPack;

} GUIMenuItem;

typedef struct GUIMenuState {

 GUIMenuItem* m_pItems;
 GUIMenuItem* m_pFirst;
 GUIMenuItem* m_pLast;
 GUIMenuItem* m_pCurr;
 GUIMenuItem* m_pLastV;
 SMString*    m_pTitle;
 unsigned int m_Flags;
 unsigned int m_Count;
 void*        m_pUserData;

 void ( *UserDataDestructor ) ( void*                     );
 int  ( *HandleEvent        ) ( GUIObject*, unsigned long );

} GUIMenuState;

typedef struct GUIMenu {

 DECLARE_GUI_OBJECT()

 void ( *Redraw ) ( struct GUIMenu* );

 int           m_X;
 int           m_Y;
 int           m_Width;
 int           m_Height;
 unsigned long m_Color;
 void*         m_pActiveObj;
 SMS_List*     m_pState;
 int           m_IGroup;
 void*         m_pUserData;

} GUIMenu;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

GUIObject* GUI_CreateMenu    ( void );
GUIObject* GUI_CreateMenuSMS ( void );

GUIMenuState* GUI_MenuPushState        ( GUIMenu*              );
int           GUI_MenuPopState         ( GUIMenu*              );
void          GUIMenu_SelectItemByName ( GUIMenu*, const char* );
void          GUIMenu_ShowItem         ( GUIMenu*, int, int    );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_GUIMenu_H */
