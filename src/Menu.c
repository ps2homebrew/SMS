/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 bix64
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "Menu.h"
#include "Browser.h"
#include "GUI.h"
#include "GS.h"
#include "Config.h"
#include "SMS.h"
#include "ExecIOP.h"

#include <kernel.h>
#include <string.h>
#include <malloc.h>
#include <libpad.h>
#include <stdio.h>
#include <libhdd.h>

typedef struct MenuItemData {

 char*        m_pName;
 void*        m_pIconLeft;
 void*        m_pIconRight;
 unsigned int m_Flags;

 void ( *Handler ) ( void );

} MenuItemData;

extern unsigned char g_IconDisplay[ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconHelp   [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconNetwork[ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconBrowser[ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconPlayer [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconOn     [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconOff    [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconSave   [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconExit   [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );
extern unsigned char g_IconFinish [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );

static void _display_handler ( void );
static void _tvsys_handler   ( void );
static void _resm_handler    ( void );
static void _charset_handler ( void );
static void _alt_handler     ( void );
static void _art_handler     ( void );
static void _aup_handler     ( void );
static void _adn_handler     ( void );
static void _network_handler ( void );
static void _netas_handler   ( void );
static void _netstrt_handler ( void );
static void _browser_handler ( void );
static void _skin_handler    ( void );
static void _sort_handler    ( void );
static void _avif_handler    ( void );
static void _hdlp_handler    ( void );
static void _sysp_handler    ( void );
static void _abc_handler     ( void );
static void _ibc_handler     ( void );
static void _fntc_handler    ( void );
static void _player_handler  ( void );
static void _volume_handler  ( void );
static void _help_handler    ( void );
static void _save_handler    ( void );
static void _exit_handler    ( void );
static void _finish_handler  ( void );

static MenuItemData s_SMSMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Display settings...",  g_IconDisplay, NULL, 0, _display_handler },
 { "Network settings...",  g_IconNetwork, NULL, 0, _network_handler },
 { "Browser settings...",  g_IconBrowser, NULL, 0, _browser_handler },
 { "Player settings...",   g_IconPlayer,  NULL, 0, _player_handler  },
 { "Help...",              g_IconHelp,    NULL, 0, _help_handler    },
 { "Save settings",        g_IconSave,    NULL, 0, _save_handler    },
 { "Shutdown console",     g_IconExit,    NULL, 0, _exit_handler    },
 { "Exit to boot browser", g_IconFinish,  NULL, 0, _finish_handler  },
 { NULL,                   NULL,          NULL, 0, NULL             }
};

static MenuItemData s_SMSMenuDisplay[] __attribute__(   (  section( ".data" )  )   ) = {
 { "TV system:",          NULL, NULL, MENU_IF_TEXT, _tvsys_handler   },
 { "Resolution mode:",    NULL, NULL, MENU_IF_TEXT, _resm_handler    },
 { "Character set:",      NULL, NULL, MENU_IF_TEXT, _charset_handler },
 { "Adjust screen left",  NULL, NULL, 0,            _alt_handler     },
 { "Adjust screen right", NULL, NULL, 0,            _art_handler     },
 { "Adjust screen up",    NULL, NULL, 0,            _aup_handler     },
 { "Adjust screen down",  NULL, NULL, 0,            _adn_handler     },
 { NULL,                  NULL, NULL, 0,            NULL             }
};

static MenuItemData s_SMSMenuNetwork[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Autostart network:", NULL, NULL, 0, _netas_handler },
 { NULL,                 NULL, NULL, 0, NULL           }
};

static MenuItemData s_SMSMenuBrowser[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Use background image:",    NULL, NULL, 0,              _skin_handler },
 { "Sort filesystem objects:", NULL, NULL, 0,              _sort_handler },
 { "Display only AVI files:",  NULL, NULL, 0,              _avif_handler },
 { "Display HDL partitions:",  NULL, NULL, 0,              _hdlp_handler },
 { "Hide system partitions:",  NULL, NULL, 0,              _sysp_handler },
 { "Active border color:",     NULL, NULL, MENU_IF_PALIDX, _abc_handler  },
 { "Inactive border color:",   NULL, NULL, MENU_IF_PALIDX, _ibc_handler  },
 { "Font color:",              NULL, NULL, MENU_IF_PALIDX, _fntc_handler },
 { NULL,                       NULL, NULL, 0,              NULL          }
};

static MenuItemData s_SMSMenuPlayer[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Default volume level:",  NULL, NULL, MENU_IF_TEXT, _volume_handler },
 { NULL,                     NULL, NULL, 0,            NULL            }
};

static MenuContext  s_MenuCtx;
static unsigned int s_SelIdxRoot;

static void ( *FillMenu ) ( void );

static void _switch_flag ( unsigned int* apVar, unsigned int aFlag ) {

 void* lpPtr;

 if ( *apVar & aFlag ) {

  *apVar &= ~aFlag;
  lpPtr   = g_IconOff;

 } else {

  *apVar |= aFlag;
  lpPtr   = g_IconOn;

 }  /* end else */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;

}  /* end _switch_flag */

static void _rotate_palette ( unsigned int* apVal ) {

 if ( ++*apVal == 17 ) *apVal = 1;

}  /* end _rotate_palette */

static void _menu_add_item (  char* apOptionName, void* apIconLeft, void* apIconRight, unsigned int aFlags, void ( *aHandler ) ( void )  ) {

 MenuItem* lpItem = ( MenuItem* )calloc (  1, sizeof ( MenuItem )  );

 lpItem -> m_pOptionName = apOptionName;
 lpItem -> m_pIconLeft   = apIconLeft;
 lpItem -> m_pIconRight  = apIconRight;
 lpItem -> m_Flags       = aFlags;
 lpItem -> Handler       = aHandler;

 if ( s_MenuCtx.m_pItems == NULL ) {

  s_MenuCtx.m_pItems =
  s_MenuCtx.m_pCurr  =
  s_MenuCtx.m_pLast  =
  s_MenuCtx.m_pFirst = lpItem;

  lpItem -> m_Y = 0;

  if ( !s_MenuCtx.m_fText ) lpItem -> m_Flags |= MENU_IF_SELECTED;

 } else {

  lpItem -> m_pPrev            = s_MenuCtx.m_pLast;
  s_MenuCtx.m_pLast -> m_pNext = lpItem;
  lpItem -> m_Y                = s_MenuCtx.m_pLast -> m_Y + 1;
  s_MenuCtx.m_pLast            = lpItem;

 }  /* end else */

}  /* end _menu_add_item */

static void _menu_clear ( void ) {

 MenuItem* lpItem = s_MenuCtx.m_pItems;

 while ( lpItem ) {

  MenuItem* lpNext = lpItem -> m_pNext;

  free ( lpItem );

  lpItem = lpNext;

 }  /* end while */

 s_MenuCtx.m_pItems  =
 s_MenuCtx.m_pFirst  =
 s_MenuCtx.m_pLast   =
 s_MenuCtx.m_pCurr   = NULL;
 s_MenuCtx.m_Offset  = 0;
 s_MenuCtx.m_pSelIdx = NULL;

}  /* end _menu_clear */

static void _menu_fill ( char* apName, MenuItemData* apItems ) {

 int i = 0;

 _menu_clear ();
 s_MenuCtx.m_pName = apName;
 s_MenuCtx.m_fText = 0;

 while ( apItems[ i ].m_pName ) {

  _menu_add_item (
   apItems[ i ].m_pName,      apItems[ i ].m_pIconLeft,
   apItems[ i ].m_pIconRight, apItems[ i ].m_Flags, apItems[ i ].Handler
  );
  ++i;

 }  /* end while */

}  /* end _menu_fill */

static void _dim_icon ( unsigned char* apBuf, unsigned char* apIcon ) {

 int i = 3;

 memcpy ( apBuf, apIcon, 4096 );

 while ( i < 4096 ) {

  apBuf[ i ] >>= 1;
  i += 4;

 }  /* end while */

 SyncDCache ( apBuf, &apBuf[ 4096 ] );

}  /* end _dim_icon */

static void _menu_draw_dimmed_item ( GSContext* apGSCtx, MenuItem* apItem, int anY ) {

 int lStrLen = strlen ( apItem -> m_pOptionName );
 int lX;

 if ( apItem -> m_pIconLeft ) {

  unsigned char lBuff[ 4096 ] __attribute__(   (  aligned( 16 )  )   );

  _dim_icon ( lBuff, apItem -> m_pIconLeft );

  apGSCtx -> DrawIcon ( s_MenuCtx.m_Left + 8, anY + 2, GSIS_32x32, lBuff );
  lX = s_MenuCtx.m_Left + 44;

 } else lX = s_MenuCtx.m_Left + 8;

 if ( apItem -> m_pIconRight ) {

  if ( apItem -> m_Flags & MENU_IF_TEXT )

   apGSCtx -> DrawText ( s_MenuCtx.m_Left + s_MenuCtx.m_Width - 148, anY + 2, 0, apItem -> m_pIconRight, 0, 0 );

  else if ( apItem -> m_Flags & MENU_IF_PALIDX ) {

   apGSCtx -> m_FillColor = g_Palette[ *( unsigned int* )apItem -> m_pIconRight - 1 ];
   apGSCtx -> m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x40 );

   apGSCtx -> RoundRect (
    s_MenuCtx.m_Left + s_MenuCtx.m_Width - 40, anY +  4,
    s_MenuCtx.m_Left + s_MenuCtx.m_Width - 10, anY + 30, 1
   );

  } else {

   unsigned char lBuff[ 4096 ] __attribute__(   (  aligned( 16 )  )   );

   _dim_icon ( lBuff, apItem -> m_pIconRight );

   apGSCtx -> DrawIcon ( s_MenuCtx.m_Left + s_MenuCtx.m_Width - 42, anY + 2, GSIS_32x32, lBuff );

  }  /* end else */

 }  /* end if */

 apGSCtx -> DrawText ( lX, anY + 2, 0, apItem -> m_pOptionName, lStrLen, 2 );

}  /* end _menu_draw_dimmed_item */

static void _menu_draw_item ( GSContext* apGSCtx, MenuItem* apItem, int anY ) {

 int lStrLen = strlen ( apItem -> m_pOptionName );
 int lX;

 if ( apItem -> m_pIconLeft ) {

  apGSCtx -> DrawIcon ( s_MenuCtx.m_Left + 8, anY + 2, GSIS_32x32, apItem -> m_pIconLeft );
  lX = s_MenuCtx.m_Left + 44;

 } else lX = s_MenuCtx.m_Left + 8;

 if ( apItem -> m_pIconRight ) {

  if ( apItem -> m_Flags & MENU_IF_TEXT )

   apGSCtx -> DrawText ( s_MenuCtx.m_Left + s_MenuCtx.m_Width - 148, anY + 2, 0, apItem -> m_pIconRight, 0, 0 );

  else if ( apItem -> m_Flags & MENU_IF_PALIDX ) {

   apGSCtx -> m_FillColor = g_Palette[ *( unsigned int* )apItem -> m_pIconRight - 1 ];
   apGSCtx -> m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x40 );

   apGSCtx -> RoundRect (
    s_MenuCtx.m_Left + s_MenuCtx.m_Width - 40, anY +  4,
    s_MenuCtx.m_Left + s_MenuCtx.m_Width - 10, anY + 30, 1
   );

  } else apGSCtx -> DrawIcon ( s_MenuCtx.m_Left + s_MenuCtx.m_Width - 42, anY + 2, GSIS_32x32, apItem -> m_pIconRight );

 }  /* end if */

 apGSCtx -> DrawText ( lX, anY + 2, 0, apItem -> m_pOptionName, lStrLen, 1 );

 if ( apItem -> m_Flags & MENU_IF_SELECTED ) {

  apGSCtx -> m_FillColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x60 );
  apGSCtx -> m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0x00, 0x00 );
  apGSCtx -> RoundRect ( s_MenuCtx.m_Left + 6, anY, s_MenuCtx.m_Left + s_MenuCtx.m_Width - 6, anY + 34, 4 );

 }  /* end if */

}  /* end _menu_draw_item */

static void _menu_draw ( void ) {

 int                  lX, lY, lTop;
 MenuItem*  lpItem  = s_MenuCtx.m_pFirst;
 GSContext* lpGSCtx = s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx;
 int        lW      = ( int )( float )lpGSCtx -> m_Width  * 0.65F;
 int        lH      = ( int )( float )lpGSCtx -> m_Height * 0.75F;

 s_MenuCtx.m_Left   = ( lpGSCtx -> m_Width  - lW ) / 2;
 s_MenuCtx.m_Top    = ( lpGSCtx -> m_Height - lH ) / 2;
 s_MenuCtx.m_Width  = lW;
 s_MenuCtx.m_Height = lH;

 lH   = ( s_MenuCtx.m_Height - 76 ) / 36;
 lW   = lpGSCtx -> TextWidth ( s_MenuCtx.m_pName, 0 );
 lX   = ( s_MenuCtx.m_Width - lW ) >> 1;
 lY   = 1;
 lTop = s_MenuCtx.m_Top + 40;

 lpGSCtx -> m_LineColor = GS_SETREG_RGBA( 0x00, 0xFF, 0x00, 0x00 );
 lpGSCtx -> m_FillColor = GS_SETREG_RGBA( 0x20, 0x20, 0x40, 0x00 );

 lpGSCtx -> VSync ();

 lpGSCtx -> RoundRect ( s_MenuCtx.m_Left, s_MenuCtx.m_Top,      s_MenuCtx.m_Left + s_MenuCtx.m_Width, s_MenuCtx.m_Top + 34,                 8 );
 lpGSCtx -> RoundRect ( s_MenuCtx.m_Left, s_MenuCtx.m_Top + 38, s_MenuCtx.m_Left + s_MenuCtx.m_Width, s_MenuCtx.m_Top + s_MenuCtx.m_Height, 8 );

 lpGSCtx -> DrawText ( s_MenuCtx.m_Left + lX, s_MenuCtx.m_Top + 2, 0, s_MenuCtx.m_pName, 0, 0 );

 if ( lpItem ) {

  if ( lpItem != s_MenuCtx.m_pItems )

   _menu_draw_dimmed_item (  lpGSCtx, lpItem, lTop + ( lpItem -> m_Y - s_MenuCtx.m_Offset ) * 34  );

  else _menu_draw_item (  lpGSCtx, lpItem, lTop + ( lpItem -> m_Y - s_MenuCtx.m_Offset ) * 34  );

  lpItem = lpItem -> m_pNext;

  while ( lpItem && lY < lH ) {

   _menu_draw_item (  lpGSCtx, lpItem, lTop + ( lpItem -> m_Y - s_MenuCtx.m_Offset ) * 34  );

   ++lY; lpItem = lpItem -> m_pNext;

  }  /* end while */

  if ( lpItem ) {

   if ( lpItem != s_MenuCtx.m_pLast )

    _menu_draw_dimmed_item (  lpGSCtx, lpItem, lTop + ( lpItem -> m_Y - s_MenuCtx.m_Offset ) * 34  );

   else _menu_draw_item (  lpGSCtx, lpItem, lTop + ( lpItem -> m_Y - s_MenuCtx.m_Offset ) * 34 );

  }  /* end if */

 }  /* end if */

}  /* end _menu_draw */

static MenuItem* _menu_navigate ( unsigned long int anEvent ) {

 int       lIdx;
 int       lH     = ( s_MenuCtx.m_Height - 76 ) / 36;
 MenuItem* lpItem = s_MenuCtx.m_pCurr;

 lIdx = lpItem -> m_Y - s_MenuCtx.m_Offset;

 if (  ( anEvent & PAD_DOWN ) && lpItem -> m_pNext  ) {

  lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  if (  !( lIdx < lH - 1 || lpItem -> m_pNext -> m_pNext == NULL ) || s_MenuCtx.m_fText ) {

   ++s_MenuCtx.m_Offset;
   s_MenuCtx.m_pFirst = s_MenuCtx.m_pFirst -> m_pNext;

  }  /* end if */

  if ( !s_MenuCtx.m_fText ) lpItem -> m_pNext -> m_Flags |= MENU_IF_SELECTED;

  s_MenuCtx.m_pCurr = lpItem -> m_pNext;

  if ( s_MenuCtx.m_pSelIdx ) ++*s_MenuCtx.m_pSelIdx;

 } else if (  ( anEvent & PAD_UP ) && lpItem -> m_pPrev  ) {

  lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  if (  !( lIdx > 1 || lpItem -> m_pPrev -> m_pPrev == NULL ) || s_MenuCtx.m_fText  ) {

   --s_MenuCtx.m_Offset;
   s_MenuCtx.m_pFirst = s_MenuCtx.m_pFirst -> m_pPrev;

  }  /* end if */

  if ( !s_MenuCtx.m_fText ) lpItem -> m_pPrev -> m_Flags |= MENU_IF_SELECTED;

  s_MenuCtx.m_pCurr = lpItem -> m_pPrev;

  if ( s_MenuCtx.m_pSelIdx ) --*s_MenuCtx.m_pSelIdx;

 } else if ( anEvent & PAD_TRIANGLE ) {

  return ( MenuItem* )MENU_EV_EXIT;

 } else if (  ( anEvent & PAD_CROSS ) && lpItem -> Handler  ) lpItem -> Handler ();

 _menu_draw ();

 return ( MenuItem* )MENU_EV_CONSUMED;

}  /* end _menu_navigate */

static void _menu_fill_root ( void ) {

 unsigned int lIdx = 0;
 MenuItem*    lpItem;

 _menu_fill ( "SMS menu", s_SMSMenu );

 FillMenu = NULL;
 s_MenuCtx.m_pSelIdx = &s_SelIdxRoot;

 lpItem = s_MenuCtx.m_pItems;

 while ( lpItem ) {

  if ( lIdx++ == *s_MenuCtx.m_pSelIdx && !s_MenuCtx.m_fText ) {

   lpItem -> m_Flags |= MENU_IF_SELECTED;
   s_MenuCtx.m_pCurr  = lpItem;

  } else lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  lpItem = lpItem -> m_pNext;

 }  /* end while */

}  /* end _menu_fill_root */

static unsigned char s_ResBuffer[ 2 ];

static void _display_handler ( void ) {

 FillMenu = _menu_fill_root;

 switch ( g_Config.m_DisplayMode ) {

  case GSDisplayMode_PAL       : s_SMSMenuDisplay[ 0 ].m_pIconRight = "PAL";    break;
  case GSDisplayMode_PAL_I     : s_SMSMenuDisplay[ 0 ].m_pIconRight = "PAL_I";  break;
  case GSDisplayMode_NTSC      : s_SMSMenuDisplay[ 0 ].m_pIconRight = "NTSC";   break;
  case GSDisplayMode_NTSC_I    : s_SMSMenuDisplay[ 0 ].m_pIconRight = "NTSC_I"; break;
  case GSDisplayMode_AutoDetect: s_SMSMenuDisplay[ 0 ].m_pIconRight = "Auto";   break;
  default                      : s_SMSMenuDisplay[ 0 ].m_pIconRight = "n/a";    break;

 }  /* end switch */

 switch ( g_Config.m_DisplayCharset ) {

  case GSCodePage_WinLatin2  : s_SMSMenuDisplay[ 2 ].m_pIconRight = "WinLatin2";   break;
  case GSCodePage_WinCyrillic: s_SMSMenuDisplay[ 2 ].m_pIconRight = "WinCyrillic"; break;
  case GSCodePage_WinLatin1  : s_SMSMenuDisplay[ 2 ].m_pIconRight = "WinLatin1";   break;
  case GSCodePage_WinGreek   : s_SMSMenuDisplay[ 2 ].m_pIconRight = "WinGreek";    break;

 }  /* end switch */

 sprintf ( s_ResBuffer, "%d", g_Config.m_ResMode + 1 );

 s_SMSMenuDisplay[ 1 ].m_pIconRight = s_ResBuffer;

 _menu_fill ( "Display settings", s_SMSMenuDisplay );

}  /* end _display_handler */

static void _tvsys_handler ( void ) {

 void* lpPtr;

 switch ( g_Config.m_DisplayMode ) {

  default                      :
  case GSDisplayMode_PAL       : g_Config.m_DisplayMode = GSDisplayMode_PAL_I;      lpPtr = "PAL_I";  break;
  case GSDisplayMode_PAL_I     : g_Config.m_DisplayMode = GSDisplayMode_NTSC;       lpPtr = "NTSC";   break;
  case GSDisplayMode_NTSC      : g_Config.m_DisplayMode = GSDisplayMode_NTSC_I;     lpPtr = "NTSC_I"; break;
  case GSDisplayMode_NTSC_I    : g_Config.m_DisplayMode = GSDisplayMode_AutoDetect; lpPtr = "Auto";   break;
  case GSDisplayMode_AutoDetect: g_Config.m_DisplayMode = GSDisplayMode_PAL;        lpPtr = "PAL";    break;

 }  /* end switch */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;

 GS_InitContext ( g_Config.m_DisplayMode );
 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 1 );

}  /* end _tvsys_handler */

static void _resm_handler ( void ) {

 if ( ++g_Config.m_ResMode > 1 ) g_Config.m_ResMode = 0;

 sprintf ( s_ResBuffer, "%d", g_Config.m_ResMode + 1 );

 GS_InitContext ( g_Config.m_DisplayMode );
 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 1 );

}  /* end _resm_handler */

static void _charset_handler ( void ) {

 void* lpPtr;

 switch ( g_Config.m_DisplayCharset ) {

  case GSCodePage_WinLatin2  : g_Config.m_DisplayCharset = GSCodePage_WinCyrillic; lpPtr = "WinCyrillic"; break;
  case GSCodePage_WinCyrillic: g_Config.m_DisplayCharset = GSCodePage_WinGreek;    lpPtr = "WinGreek";    break;
  default                    :
  case GSCodePage_WinGreek   : g_Config.m_DisplayCharset = GSCodePage_WinLatin1;   lpPtr = "WinLatin1";   break;
  case GSCodePage_WinLatin1  : g_Config.m_DisplayCharset = GSCodePage_WinLatin2;   lpPtr = "WinLatin2";   break;

 }  /* end switch */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;
 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> SetCodePage ( g_Config.m_DisplayCharset );
 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _charset_handler */

static void _alt_handler ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> AdjustDisplay ( -1, 0 );
 g_Config.m_DX = s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> m_StartX;

}  /* end _alt_handler */

static void _art_handler ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> AdjustDisplay ( 1, 0 );
 g_Config.m_DX = s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> m_StartX;

}  /* end _art_handler */

static void _aup_handler ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> AdjustDisplay ( 0, -1 );
 g_Config.m_DY = s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> m_StartY;

}  /* end _aup_handler */

static void _adn_handler ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> AdjustDisplay ( 0, 1 );
 g_Config.m_DY = s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> m_pGSCtx -> m_StartY;

}  /* end _adn_handler */

static void _network_handler ( void ) {

 FillMenu = _menu_fill_root;

 s_SMSMenuNetwork[ 0 ].m_pIconRight = g_Config.m_NetworkFlags & SMS_NF_AUTO ? g_IconOn : g_IconOff;

 _menu_fill ( "Network settings", s_SMSMenuNetwork );

 if (  !( g_SMSFlags & SMS_FLAG_NET ) && ( g_SMSFlags & SMS_FLAG_DEV9 )  )

  _menu_add_item (
   "Start network interface now", NULL, NULL, 0, _netstrt_handler
  );

}  /* end _network_handler */

static void _netas_handler ( void ) {

 _switch_flag ( &g_Config.m_NetworkFlags, SMS_NF_AUTO );

}  /* end _netas_handler */

static void _netstrt_handler ( void ) {

 SMS_StartNetwork ( s_MenuCtx.m_pBrowserCtx -> m_pGUICtx );

 if (  !( g_SMSFlags & SMS_FLAG_NET )  ) {

  s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( "Error. Press X to continue..." );
  GUI_WaitButton ( PAD_CROSS, 200 );

 } else _network_handler ();

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( " " );

}  /* end _netstrt_handler */

static void _browser_handler ( void ) {

 FillMenu = _menu_fill_root;

 s_SMSMenuBrowser[ 0 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SKIN ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 1 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SORT ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 2 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_AVIF ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 3 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_HDLP ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 4 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SYSP ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 5 ].m_pIconRight = &g_Config.m_BrowserABCIdx;
 s_SMSMenuBrowser[ 6 ].m_pIconRight = &g_Config.m_BrowserIBCIdx;
 s_SMSMenuBrowser[ 7 ].m_pIconRight = &g_Config.m_BrowserTxtIdx;

 _menu_fill ( "Browser settings", s_SMSMenuBrowser );

}  /* end _browser_handler */

static void _skin_handler ( void ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SKIN );

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _skin_handler */

static void _sort_handler ( void ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SORT );

}  /* end _sort_handler */

static void _avif_handler ( void ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_AVIF );

}  /* end _avif_handler */

static void _hdlp_handler ( void ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_HDLP );

}  /* end _hdlp_handler */

static void _sysp_handler ( void ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SYSP );

}  /* end _sysp_handler */

static void _abc_handler ( void ) {

 _rotate_palette ( &g_Config.m_BrowserABCIdx );

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _abc_handler */

static void _ibc_handler ( void ) {

 _rotate_palette ( &g_Config.m_BrowserIBCIdx );

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _ibc_handler */

static void _fntc_handler ( void ) {

 _rotate_palette ( &g_Config.m_BrowserTxtIdx );

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _fntc_handler */

static char s_VolumeBuffer[ 5 ];

static void _player_handler ( void ) {

 FillMenu = _menu_fill_root;

 s_SMSMenuPlayer[ 0 ].m_pIconRight = s_VolumeBuffer;

 sprintf (    s_VolumeBuffer, "%d%%", ( int )(   (  ( float )g_Config.m_PlayerVolume / 24.0F ) * 100.0F + 0.5F   )    );

 _menu_fill ( "Player settings", s_SMSMenuPlayer );

}  /* end _player_handler */

static void _volume_handler ( void ) {

 if ( ++g_Config.m_PlayerVolume == 25 ) g_Config.m_PlayerVolume = 0;

  sprintf (    s_VolumeBuffer, "%d%%", ( int )(   (  ( float )g_Config.m_PlayerVolume / 24.0F ) * 100.0F + 0.5F  )    );

}  /* end _volume_handler */

static char* s_Help[] = {
 "At startup:",
 "sel+R1 - NTSC mode",
 "sel+R2 - PAL mode",
 "sel+R1+square - NTSC_I mode",
 "sel+R2+Square - PAL_I mode",
 " ",
 "Browser:",
 "triangle - parent directory",
 "left/right - device menu",
 "up/down - navigate directory",
 "cross - action",
 "sel+circle - power off",
 "sel+R1 - adjust screen right",
 "sel+L1 - adjust screen left",
 "sel+R2 - adjust screen down",
 "sel+L2 - adjust screen up",
 "sel+square - save settings",
 "sel+triangle - boot browser",
 " ",
 "Player:",
 "up/down - adjust volume",
 "right/left - FFWD/REW mode",
 "cross - exit FFWD/REW mode",
 "triangle - stop",
 "select - pause",
 "start - resume",
 " ",
 "SMS menu:",
 "cross - action/next level",
 "triangle - level up/exit menu",
 NULL

};

static void _help_handler ( void ) {

 int i = 0;

 _menu_clear ();
 s_MenuCtx.m_pName = "Quick help";
 s_MenuCtx.m_fText = 1;

 FillMenu = _menu_fill_root;
 
 while ( s_Help[ i ] ) _menu_add_item ( s_Help[ i++ ], NULL, NULL, 0, NULL );

}  /* end _help_handler */

static void _save_handler ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( "Saving configuration..." );

 strncpy ( g_Config.m_Partition, s_MenuCtx.m_pBrowserCtx -> m_pActivePartition, 255 );

 if (  !SaveConfig ()  ) {

  s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( "Error. Press X to continue..." );
  GUI_WaitButton ( PAD_CROSS, 200 );

 }  /* end if */

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( " " );

}  /* end _save_handler */

static void _exit_handler ( void ) {

 hddPowerOff ();

}  /* end _exit_handler */

static void _finish_handler ( void ) {

 SMS_ResetIOP ();
 Exit ( 0 );

}  /* end _finish_handler */

static void Menu_Run ( void ) {

 s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Status ( " " );

 _menu_fill_root ();
 _menu_draw      ();

 while ( 1 )

  if (  ( int )_menu_navigate (  GUI_WaitButton ( 0xFFFF, 0 )  ) == MENU_EV_EXIT  ) {

   if ( FillMenu != NULL ) {

    FillMenu ();
    _menu_draw ();

   } else {

    s_MenuCtx.m_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );
    break;

   }  /* end else */

  }  /* end if */

}  /* end Menu_Run */

MenuContext* MenuContext_Init ( BrowserContext* apBrowserCtx ) {

 s_MenuCtx.m_pItems = NULL;
 s_MenuCtx.m_pCurr  = NULL;
 s_MenuCtx.m_pFirst = NULL;
 s_MenuCtx.m_pLast  = NULL;
 s_MenuCtx.m_Offset = 0;

 s_MenuCtx.m_pBrowserCtx = apBrowserCtx;
 s_MenuCtx.Run           = Menu_Run;

 return &s_MenuCtx;

}  /* end MenuContext_Init */
