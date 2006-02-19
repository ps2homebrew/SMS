#include "SMS_GUIMenu.h"
#include "SMS_IPU.h"
#include "SMS_GS.h"
#include "SMS_Locale.h"
#include "SMS_PAD.h"
#include "SMS_Player.h"
#include "SMS_PlayerControl.h"
#include "SMS_Config.h"
#include "SMS_GUICons.h"
#include "SMS_DMA.h"

#include <kernel.h>
#include <string.h>

extern SMS_Player s_Player;
extern SMString   g_StrPlayer[ 6 ];

static int ( *HandleEventBase ) ( GUIObject*, unsigned long );
static GUIMenu* s_pMenu;

static SMString* s_ModeNames[ 5 ] __attribute__(   (  section( ".data" )  )   ) = {
 &STR_LETTERBOX, &STR_PANSCAN1, &STR_PANSCAN2, &STR_PANSCAN3, &STR_FULLSCREEN
};

static SMString s_Lang;

static void _lang_handler    ( GUIMenu*, int );
static void _disp_handler    ( GUIMenu*, int );
static void _poff_handler    ( GUIMenu*, int );
static void _dsub_handler    ( GUIMenu*, int );
extern void _subclr_handler  ( GUIMenu*, int );
extern void _subbclr_handler ( GUIMenu*, int );
extern void _subiclr_handler ( GUIMenu*, int );
extern void _subu_handler    ( GUIMenu*, int );
extern void _update_poff     ( int           );

static GUIMenuItem s_PlayerMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { MENU_ITEM_TYPE_TEXT,   &STR_LANGUAGE,            0, 0, _lang_handler,    0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_DISPLAY,             0, 0, _disp_handler,    0, 0 },
 { MENU_ITEM_TYPE_TEXT,   &STR_AUTO_POWER_OFF,      0, 0, _poff_handler,    0, 0 },
 { 0,                     &STR_DISPLAY_SUBTITLES,   0, 0, _dsub_handler,    0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_COLOR,      0, 0, _subclr_handler,  0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_BOLD_COLOR, 0, 0, _subbclr_handler, 0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_ITL_COLOR,  0, 0, _subiclr_handler, 0, 0 },
 { MENU_ITEM_TYPE_PALIDX, &STR_SUBTITLE_UND_COLOR,  0, 0, _subu_handler,    0, 0 }
};

static void GUIMenuPlayer_Redraw ( GUIMenu* apMenu ) {

 if ( apMenu ) apMenu -> Cleanup (  ( GUIObject* )apMenu  );

 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
 s_pMenu -> Render (  ( GUIObject* )s_pMenu, 1  );

 GS_VSync ();
 g_IPUCtx.Repaint ();
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

}  /* end GUIMenuPlayer_Redraw */

int GUIMenuPlayer_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case SMS_PAD_TRIANGLE: GUI_PostMessage ( GUI_MSG_QUIT ); return GUIHResult_Handled;

 }  /* end switch */

 return HandleEventBase ( apObj, anEvent );

}  /* end GUIMenuPlayer_HandleEvent */

static void _update_lang ( void ) {

 char* lpLang = PlayerControl_GetLang () -> m_pString;

 s_Lang.m_pStr = lpLang;
 s_Lang.m_Len  = strlen ( lpLang );

}  /* end _update_lang */

static void _lang_handler ( GUIMenu* apMenu, int aDir ) {

 SMS_ListNode* lpNode = PlayerControl_ChangeLang ();

 _update_lang ();

 s_Player.m_AudioIdx = ( unsigned int )lpNode -> m_Param;

 apMenu -> Redraw ( apMenu );

}  /* end _lang_handler */

static void _disp_handler ( GUIMenu* apMenu, int aDir ) {

 if ( ++s_Player.m_PanScan == 5 ) s_Player.m_PanScan = 0;

 g_Config.m_PlayerFlags &= 0x0FFFFFFF;
 g_Config.m_PlayerFlags |= ( s_Player.m_PanScan << 28 );

 s_PlayerMenu[ 1 ].m_IconRight = ( unsigned int )s_ModeNames[ s_Player.m_PanScan ];

 s_Player.m_pIPUCtx -> ChangeMode ( s_Player.m_PanScan );

 apMenu -> Redraw ( apMenu );

}  /* end _disp_handler */

static void _poff_handler ( GUIMenu* apMenu, int aDir ) {

 aDir *= 60000;

 _update_poff ( aDir );

 apMenu -> Redraw ( apMenu );

}  /* end _poff_handler */

static void _dsub_handler ( GUIMenu* apMenu, int aDir ) {

 if ( s_Player.m_Flags & SMS_PF_SUBS ) {

  s_Player.m_Flags             &= ~SMS_PF_SUBS;
  s_PlayerMenu[ 3 ].m_IconRight = GUICON_OFF;

 } else {

  s_Player.m_Flags             |= SMS_PF_SUBS;
  s_PlayerMenu[ 3 ].m_IconRight = GUICON_ON;

 }  /* end else */

 apMenu -> Redraw ( apMenu );

}  /* end _dsub_handler */

void SMS_PlayerMenu ( void ) {

 GUIMenu*      lpMenu   = ( GUIMenu* )GUI_CreateMenu ();
 int           lWidth   = g_GSCtx.m_Width  - ( g_GSCtx.m_Width  >> 2 );
 int           lHeight  = g_GSCtx.m_Height - ( g_GSCtx.m_Height >> 2 );
 unsigned int  lVRAMPtr = g_GSCtx.m_VRAMPtr;
 GUIMenuState* lpState;
 unsigned long lDMA[ 6 ] __attribute__(   (  aligned( 16 )  )   );

 lDMA[ 0 ] = GIF_TAG( 2, 1, 0, 0, 0, 1 );
 lDMA[ 1 ] = GIFTAG_REGS_AD;
 lDMA[ 2 ] = 0;
 lDMA[ 3 ] = GS_TEST_1;
 lDMA[ 4 ] = 0;
 lDMA[ 5 ] = GS_TEST_2;
 SyncDCache ( lDMA, lDMA + 6 );
 DMA_Send ( DMAC_GIF, lDMA, 3 );

 HandleEventBase = lpMenu -> HandleEvent;

 lpMenu -> m_Color      = 0x60301010UL;
 lpMenu -> m_X          = ( g_GSCtx.m_Width  - lWidth  ) >> 1;
 lpMenu -> m_Y          = (  ( g_GSCtx.m_Height - lHeight ) >> 1  ) + 8;
 lpMenu -> m_Width      = lWidth;
 lpMenu -> m_Height     = lHeight;
 lpMenu -> m_pActiveObj = g_pActiveNode;
 lpMenu -> m_pState     = SMS_ListInit ();

 lpMenu -> Redraw      = GUIMenuPlayer_Redraw;
 lpMenu -> HandleEvent = GUIMenuPlayer_HandleEvent;

 lpState = GUI_MenuPushState ( lpMenu );
 lpState -> m_pTitle = &STR_PLAYER_MENU;
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_PlayerMenu;
 lpState -> m_pLast  = &s_PlayerMenu[ s_Player.m_pSubCtx ? 7 : 2 ];

 _update_poff ( 0 );
 _update_lang ();

 s_PlayerMenu[ 0 ].m_IconRight = ( unsigned int )&s_Lang;
 s_PlayerMenu[ 1 ].m_IconRight = ( unsigned int )s_ModeNames[ s_Player.m_PanScan ];
 s_PlayerMenu[ 2 ].m_IconRight = ( unsigned int )&g_StrPlayer[ 3 ];
 s_PlayerMenu[ 3 ].m_IconRight = g_Config.m_PlayerFlags & SMS_PF_SUBS ? GUICON_ON : GUICON_OFF;
 s_PlayerMenu[ 4 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCNIdx;
 s_PlayerMenu[ 5 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCBIdx;
 s_PlayerMenu[ 6 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCIIdx;
 s_PlayerMenu[ 7 ].m_IconRight = ( unsigned int )&g_Config.m_PlayerSCUIdx;

 s_pMenu = lpMenu;

 g_GSCtx.m_VRAMPtr = g_GSCtx.m_VRAMPtr2 << 5;

 GUI_LoadIcons ();
 GUI_SetColors ();
 GUI_AddObject (  STR_PLAYER_MENU.m_pStr, ( GUIObject* )lpMenu  );
 lpMenu -> Redraw ( lpMenu );
 GUI_Run ();
 GUI_DeleteObject ( STR_PLAYER_MENU.m_pStr );
 s_Player.SetColors ();
 g_GSCtx.m_TextColor = 0;

 lDMA[ 2 ] = GS_SET_TEST_1( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 lDMA[ 4 ] = GS_SET_TEST_2( 0, 1, 0x80, 0, 0, 0, 1, 1 );
 SyncDCache ( lDMA, lDMA + 6 );
 DMA_Send ( DMAC_GIF, lDMA, 3 );
 g_IPUCtx.Repaint ();

 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 0 ] );
 GS_SetGC ( &g_GSCtx.m_DrawCtx[ 1 ] );

 g_GSCtx.m_VRAMPtr = lVRAMPtr;

}  /* end SMS_PlayerMenu */
