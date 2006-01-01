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
#include "SMS_PlayerMenu.h"
#include "SMS_Player.h"
#include "SMS_PlayerControl.h"
#include "Menu.h"
#include "IPU.h"
#include "GUI.h"
#include "Config.h"
#include "StringList.h"

#include <stdio.h>

static SMS_Player* s_pPlayer;

static char* s_ModeNames[ 5 ] = {
 "letterbox",
 "pan-scan 1",
 "pan-scan 2",
 "pan-scan 3",
 "fullscreen"
};

extern unsigned char g_IconOn [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconOff[ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );

static void _lang_handler ( int );
static void _disp_handler ( int );
static void _subs_handler ( int );

extern void _scolor_handler ( int );
extern void _sbcolr_handler ( int );
extern void _sicolr_handler ( int );
extern void _sucolr_handler ( int );
extern void _powoff_hander  ( int );

static MenuContext s_MenuCtx;

static MenuItemData s_PlayerMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Language:",          NULL, NULL, MENU_IF_TEXT, _lang_handler  },
 { "Display:",           NULL, NULL, MENU_IF_TEXT, _disp_handler  },
 { "Auto power-off:",    NULL, NULL, MENU_IF_TEXT, _powoff_hander },
 { NULL,                 NULL, NULL, 0,            NULL           }
};

static void _lang_handler ( int aDir ) {

 StringListNode* lpNode = PlayerControl_ChangeLang ();

 s_MenuCtx.m_pCurr -> m_pIconRight = lpNode -> m_pString;
 s_pPlayer -> m_AudioIdx           = ( unsigned int )lpNode -> m_pParam;

}  /* end _lang_handler */

static void _disp_handler ( int aDir ) {

 if ( ++s_pPlayer -> m_PanScan == 5 ) s_pPlayer -> m_PanScan = 0;

 g_Config.m_PlayerFlags &= 0x0FFFFFFF;
 g_Config.m_PlayerFlags |= ( s_pPlayer -> m_PanScan << 28 );

 s_MenuCtx.m_pCurr -> m_pIconRight = s_ModeNames[ s_pPlayer -> m_PanScan ];
 s_pPlayer -> m_pIPUCtx -> ChangeMode ( s_pPlayer -> m_PanScan );

}  /* end _disp_handler */

static void _subs_handler ( int aDir ) {

 if ( s_pPlayer -> m_Flags & SMS_PF_SUBS ) {

  s_pPlayer -> m_Flags &= ~SMS_PF_SUBS;
  s_MenuCtx.m_pCurr -> m_pIconRight = g_IconOff;

 } else {

  s_pPlayer -> m_Flags |= SMS_PF_SUBS;
  s_MenuCtx.m_pCurr -> m_pIconRight = g_IconOn;

 }  /* end else */

}  /* end _subs_handler */

extern char g_PowoffBuffer[ 32 ] __attribute__(   (  section( ".data" )  )   );

extern void _adjust_poweroff ( int );

static void _menu_fill ( void ) {

 s_PlayerMenu[ 0 ].m_pIconRight = PlayerControl_GetLang () -> m_pString;
 s_PlayerMenu[ 1 ].m_pIconRight = s_ModeNames[ s_pPlayer -> m_PanScan ];
 s_PlayerMenu[ 2 ].m_pIconRight = g_PowoffBuffer;

 _adjust_poweroff ( 0 );

 Menu_Fill ( &s_MenuCtx, "Player menu", s_PlayerMenu );

 if ( s_pPlayer -> m_pSubCtx ) {

  Menu_AddItem (
   &s_MenuCtx, "Display subtitles:", NULL,
   s_pPlayer -> m_Flags & SMS_PF_SUBS ? g_IconOn : g_IconOff,
   0, _subs_handler
  );
  Menu_AddItem (
   &s_MenuCtx, "Subtitle color:", NULL, &g_Config.m_PlayerSCNIdx, MENU_IF_PALIDX, _scolor_handler
  );
  Menu_AddItem (
   &s_MenuCtx, "Subtitle bold color:", NULL, &g_Config.m_PlayerSCBIdx, MENU_IF_PALIDX, _sbcolr_handler
  );
  Menu_AddItem (
   &s_MenuCtx, "Subtitle italic color:", NULL, &g_Config.m_PlayerSCIIdx, MENU_IF_PALIDX, _sicolr_handler
  );
  Menu_AddItem (
   &s_MenuCtx, "Subtitle underline color:", NULL, &g_Config.m_PlayerSCUIdx, MENU_IF_PALIDX, _sucolr_handler
  );

 }  /* end if */

}  /* end _menu_fill */

static void _menu_prepaint ( void ) {

 g_GSCtx.VSync    ();
 g_IPUCtx.Repaint ();

}  /* end _menu_prepaint */

static void _menu_run ( void ) {

 unsigned int lColor  = g_Palette[ g_Config.m_BrowserTxtIdx - 1 ] & ~0xFF000000;
 unsigned int lDColor = lColor | 0x20000000;

 lColor |= 0x80000000;

 g_GSCtx.SetTextColor ( 0, 0x80FFFFFF );
 g_GSCtx.SetTextColor ( 1, lColor     );
 g_GSCtx.SetTextColor ( 2, lDColor    );

 _menu_fill ();
 Menu_Draw ( &s_MenuCtx );

 while (   ( int )Menu_Navigate (  &s_MenuCtx, GUI_WaitButton ( 0xFFFF, 0 )  ) != MENU_EV_EXIT   );

 s_pPlayer -> SetColors ();
 s_pPlayer -> m_pIPUCtx -> Repaint ();

}  /* end _menu_run */

MenuContext* PlayerMenu_Init ( SMS_Player* apPlayer ) {

 s_MenuCtx.m_pItems = NULL;
 s_MenuCtx.m_pCurr  = NULL;
 s_MenuCtx.m_pFirst = NULL;
 s_MenuCtx.m_pLast  = NULL;
 s_MenuCtx.m_Offset = 0;
 s_MenuCtx.m_Color  = GS_SETREG_RGBA( 0x20, 0x20, 0x40, 0x20 );
 s_MenuCtx.Run      = _menu_run;
 s_MenuCtx.PrePaint = _menu_prepaint;

 s_pPlayer = apPlayer;

 return &s_MenuCtx;

}  /* end PlayerMenu_Init */
