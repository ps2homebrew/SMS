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
#include "PAD.h"
#include "ExecIOP.h"

#include <kernel.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <libhdd.h>

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

static void _display_handler ( int  );
static void _tvsys_handler   ( int  );
static void _resm_handler    ( int  );
static void _charset_handler ( int  );
static void _blur_handler    ( int  );
static void _alt_handler     ( int  );
static void _art_handler     ( int  );
static void _aup_handler     ( int  );
static void _adn_handler     ( int  );
static void _network_handler ( int  );
static void _netas_handler   ( int  );
static void _netstrt_handler ( int  );
static void _browser_handler ( int  );
static void _skin_handler    ( int  );
static void _sort_handler    ( int  );
static void _avif_handler    ( int  );
static void _hdlp_handler    ( int  );
static void _sysp_handler    ( int  );
static void _abc_handler     ( int  );
static void _ibc_handler     ( int  );
static void _fntc_handler    ( int  );
static void _slbc_handler    ( int  );
static void _sltc_handler    ( int  );
static void _player_handler  ( int  );
static void _volume_handler  ( int  );
static void _scroll_handler  ( int  );
static void _sbpos_handler   ( int  );
static void _sbtim_handler   ( int  );
static void _sbclr_handler   ( int  );
static void _vbclr_handler   ( int  );
static void _aadsp_handler   ( int  );
static void _subttl_handler  ( int  );
static void _alignm_handler  ( int  );
       void _scolor_handler  ( int  );
       void _sbcolr_handler  ( int  );
       void _sicolr_handler  ( int  );
       void _sucolr_handler  ( int  );
       void _powoff_hander   ( int  );
static void _suboff_handler  ( int  );
static void _suboff_enter    ( void );
static void _suboff_leave    ( void );
static void _help_handler    ( int  );
static void _save_handler    ( int  );
static void _exit_handler    ( int  );
static void _finish_handler  ( int  );

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
 { "Soften image:",       NULL, NULL, 0,            _blur_handler    },
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
 { "Filter media files:",      NULL, NULL, 0,              _avif_handler },
 { "Display HDL partitions:",  NULL, NULL, 0,              _hdlp_handler },
 { "Hide system partitions:",  NULL, NULL, 0,              _sysp_handler },
 { "Active border color:",     NULL, NULL, MENU_IF_PALIDX, _abc_handler  },
 { "Inactive border color:",   NULL, NULL, MENU_IF_PALIDX, _ibc_handler  },
 { "Font color:",              NULL, NULL, MENU_IF_PALIDX, _fntc_handler },
 { "Selection bar color:",     NULL, NULL, MENU_IF_PALIDX, _slbc_handler },
 { "Status line text color:",  NULL, NULL, MENU_IF_PALIDX, _sltc_handler },
 { NULL,                       NULL, NULL, 0,              NULL          }
};

static MenuItemData s_SMSMenuPlayer[] __attribute__(   (  section( ".data" )  )   ) = {
 { "Default volume:",           NULL, NULL, MENU_IF_TEXT,   _volume_handler },
 { "Subtitle alignment:",       NULL, NULL, MENU_IF_TEXT,   _alignm_handler },
 { "Subtitle offset:",          NULL, NULL, MENU_IF_TEXT,   _suboff_handler, _suboff_enter, _suboff_leave },
 { "Autoload subtitles:",       NULL, NULL, 0,              _subttl_handler },
 { "Subtitle color:",           NULL, NULL, MENU_IF_PALIDX, _scolor_handler },
 { "Subtitle bold color:",      NULL, NULL, MENU_IF_PALIDX, _sbcolr_handler },
 { "Subtitle italic color:",    NULL, NULL, MENU_IF_PALIDX, _sicolr_handler },
 { "Subtitle underline color:", NULL, NULL, MENU_IF_PALIDX, _sucolr_handler },
 { "Auto power-off:",           NULL, NULL, MENU_IF_TEXT,   _powoff_hander  },
 { "Scroll bar length:",        NULL, NULL, MENU_IF_TEXT,   _scroll_handler },
 { "Scroll bar position:",      NULL, NULL, MENU_IF_TEXT,   _sbpos_handler  },
 { "Display scroll bar time:",  NULL, NULL, 0,              _sbtim_handler  },
 { "Scroll bar color:",         NULL, NULL, MENU_IF_PALIDX, _sbclr_handler  },
 { "Volume bar color:",         NULL, NULL, MENU_IF_PALIDX, _vbclr_handler  },
 { "Audio animation display:",  NULL, NULL, 0,              _aadsp_handler  },
 { NULL,                        NULL, NULL, 0,              NULL            }
};

static MenuContext     s_MenuCtx;
static unsigned int    s_SelIdxRoot;
static BrowserContext* s_pBrowserCtx;

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

MenuItem* Menu_AddItem (  MenuContext* apMenuCtx, char* apOptionName, void* apIconLeft, void* apIconRight, unsigned int aFlags, void ( *aHandler ) ( int )  ) {

 MenuItem* lpItem = ( MenuItem* )calloc (  1, sizeof ( MenuItem )  );

 lpItem -> m_pOptionName = apOptionName;
 lpItem -> m_pIconLeft   = apIconLeft;
 lpItem -> m_pIconRight  = apIconRight;
 lpItem -> m_Flags       = aFlags;
 lpItem -> Handler       = aHandler;

 if ( apMenuCtx -> m_pItems == NULL ) {

  apMenuCtx -> m_pItems =
  apMenuCtx -> m_pCurr  =
  apMenuCtx -> m_pLast  =
  apMenuCtx -> m_pFirst = lpItem;

  lpItem -> m_Y = 0;

  if (  !( apMenuCtx -> m_Flags & MENU_F_TEXT )  ) lpItem -> m_Flags |= MENU_IF_SELECTED;

 } else {

  lpItem -> m_pPrev               = apMenuCtx -> m_pLast;
  apMenuCtx -> m_pLast -> m_pNext = lpItem;
  lpItem -> m_Y                   = apMenuCtx -> m_pLast -> m_Y + 1;
  apMenuCtx -> m_pLast            = lpItem;

 }  /* end else */

 return lpItem;

}  /* end Menu_AddItem */

static void _menu_clear ( MenuContext* apMenuCtx ) {

 MenuItem* lpItem = apMenuCtx -> m_pItems;

 while ( lpItem ) {

  MenuItem* lpNext = lpItem -> m_pNext;

  free ( lpItem );

  lpItem = lpNext;

 }  /* end while */

 apMenuCtx -> m_pItems  =
 apMenuCtx -> m_pFirst  =
 apMenuCtx -> m_pLast   =
 apMenuCtx -> m_pCurr   = NULL;
 apMenuCtx -> m_Offset  = 0;
 apMenuCtx -> m_pSelIdx = NULL;

}  /* end _menu_clear */

void Menu_Fill ( MenuContext* apMenuCtx, char* apName, MenuItemData* apItems ) {

 int i = 0;

 _menu_clear ( apMenuCtx );

 apMenuCtx -> m_pName  = apName;
 apMenuCtx -> m_Flags &= ~MENU_F_TEXT;

 while ( apItems[ i ].m_pName ) {

  MenuItem* lpItem = Menu_AddItem (
   apMenuCtx,
   apItems[ i ].m_pName,      apItems[ i ].m_pIconLeft,
   apItems[ i ].m_pIconRight, apItems[ i ].m_Flags, apItems[ i ].Handler
  );

  lpItem -> Enter = apItems[ i ].Enter;
  lpItem -> Leave = apItems[ i ].Leave;

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

static void _menu_draw_dimmed_item ( MenuContext* apMenuCtx, MenuItem* apItem, int anY ) {

 int lStrLen = strlen ( apItem -> m_pOptionName );
 int lX;

 if ( apItem -> m_pIconLeft ) {

  unsigned char lBuff[ 4096 ] __attribute__(   (  aligned( 16 )  )   );

  _dim_icon ( lBuff, apItem -> m_pIconLeft );

  g_GSCtx.DrawIcon ( apMenuCtx -> m_Left + 8, anY + 2, GSIS_32x32, lBuff );
  lX = apMenuCtx -> m_Left + 44;

 } else lX = apMenuCtx -> m_Left + 8;

 if ( apItem -> m_pIconRight ) {

  if ( apItem -> m_Flags & MENU_IF_TEXT )

   g_GSCtx.DrawText ( apMenuCtx -> m_Left + apMenuCtx -> m_Width - 148, anY + 2, 0, apItem -> m_pIconRight, 0, 0 );

  else if ( apItem -> m_Flags & MENU_IF_PALIDX ) {

   g_GSCtx.m_FillColor = g_Palette[ *( unsigned int* )apItem -> m_pIconRight - 1 ];
   g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x40 );

   g_GSCtx.RoundRect (
    apMenuCtx -> m_Left + apMenuCtx -> m_Width - 40, anY +  4,
    apMenuCtx -> m_Left + apMenuCtx -> m_Width - 10, anY + 30, 1
   );

  } else {

   unsigned char lBuff[ 4096 ] __attribute__(   (  aligned( 16 )  )   );

   _dim_icon ( lBuff, apItem -> m_pIconRight );

   g_GSCtx.DrawIcon ( apMenuCtx -> m_Left + apMenuCtx -> m_Width - 42, anY + 2, GSIS_32x32, lBuff );

  }  /* end else */

 }  /* end if */

 g_GSCtx.DrawText ( lX, anY + 2, 0, apItem -> m_pOptionName, lStrLen, 2 );

}  /* end _menu_draw_dimmed_item */

static void _menu_draw_item ( MenuContext* apMenuCtx, MenuItem* apItem, int anY ) {

 int lStrLen = strlen ( apItem -> m_pOptionName );
 int lX;

 if ( apItem -> m_pIconLeft ) {

  g_GSCtx.DrawIcon ( apMenuCtx -> m_Left + 8, anY + 2, GSIS_32x32, apItem -> m_pIconLeft );
  lX = apMenuCtx -> m_Left + 44;

 } else lX = apMenuCtx -> m_Left + 8;

 if ( apItem -> m_pIconRight ) {

  if ( apItem -> m_Flags & MENU_IF_TEXT )

   g_GSCtx.DrawText ( apMenuCtx -> m_Left + apMenuCtx -> m_Width - 148, anY + 2, 0, apItem -> m_pIconRight, 0, 0 );

  else if ( apItem -> m_Flags & MENU_IF_PALIDX ) {

   g_GSCtx.m_FillColor = g_Palette[ *( unsigned int* )apItem -> m_pIconRight - 1 ];
   g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x40 );

   g_GSCtx.RoundRect (
    apMenuCtx -> m_Left + apMenuCtx -> m_Width - 40, anY +  4,
    apMenuCtx -> m_Left + apMenuCtx -> m_Width - 10, anY + 30, 1
   );

  } else g_GSCtx.DrawIcon ( apMenuCtx -> m_Left + apMenuCtx -> m_Width - 42, anY + 2, GSIS_32x32, apItem -> m_pIconRight );

 }  /* end if */

 g_GSCtx.DrawText ( lX, anY + 2, 0, apItem -> m_pOptionName, lStrLen, 1 );

 if ( apItem -> m_Flags & MENU_IF_SELECTED ) {

  g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x60 );
  g_GSCtx.m_LineColor = g_Palette[ g_Config.m_BrowserSCIdx - 1 ];
  g_GSCtx.RoundRect ( apMenuCtx -> m_Left + 6, anY, apMenuCtx -> m_Left + apMenuCtx -> m_Width - 6, anY + 34, 4 );

 }  /* end if */

}  /* end _menu_draw_item */

void Menu_Draw ( MenuContext* apMenuCtx ) {

 int       lX, lY, lTop;
 MenuItem* lpItem  = apMenuCtx -> m_pFirst;
 int       lW      = ( int )( float )g_GSCtx.m_Width  * 0.65F;
 int       lH      = ( int )( float )g_GSCtx.m_Height * 0.75F;

 apMenuCtx -> m_Left   = ( g_GSCtx.m_Width  - lW ) / 2;
 apMenuCtx -> m_Top    = ( g_GSCtx.m_Height - lH ) / 2;
 apMenuCtx -> m_Width  = lW;
 apMenuCtx -> m_Height = lH;

 lH   = ( apMenuCtx -> m_Height - 76 ) / 36;
 lW   = g_GSCtx.TextWidth ( apMenuCtx -> m_pName, 0 );
 lX   = ( apMenuCtx -> m_Width - lW ) >> 1;
 lY   = 1;
 lTop = apMenuCtx -> m_Top + 40;

 g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0x00, 0xFF, 0x00, 0x00 );
 g_GSCtx.m_FillColor = apMenuCtx -> m_Color;

 if ( !apMenuCtx -> PrePaint ) {

  if (  !( apMenuCtx -> m_Flags & MENU_F_VSYN )  ) g_GSCtx.VSync ();

 } else apMenuCtx -> PrePaint ();

 apMenuCtx -> m_Flags &= ~MENU_F_VSYN;

 g_GSCtx.RoundRect ( apMenuCtx -> m_Left, apMenuCtx -> m_Top,      apMenuCtx -> m_Left + apMenuCtx -> m_Width, apMenuCtx -> m_Top + 34,                    8 );
 g_GSCtx.RoundRect ( apMenuCtx -> m_Left, apMenuCtx -> m_Top + 38, apMenuCtx -> m_Left + apMenuCtx -> m_Width, apMenuCtx -> m_Top + apMenuCtx -> m_Height, 8 );

 g_GSCtx.DrawText ( apMenuCtx -> m_Left + lX, apMenuCtx -> m_Top + 2, 0, apMenuCtx -> m_pName, 0, 0 );

 if ( lpItem ) {

  if ( lpItem != apMenuCtx -> m_pItems )

   _menu_draw_dimmed_item (  apMenuCtx, lpItem, lTop + ( lpItem -> m_Y - apMenuCtx -> m_Offset ) * 34  );

  else _menu_draw_item (  apMenuCtx, lpItem, lTop + ( lpItem -> m_Y - apMenuCtx -> m_Offset ) * 34  );

  lpItem = lpItem -> m_pNext;

  while ( lpItem && lY < lH ) {

   _menu_draw_item (  apMenuCtx, lpItem, lTop + ( lpItem -> m_Y - apMenuCtx -> m_Offset ) * 34  );

   ++lY; lpItem = lpItem -> m_pNext;

  }  /* end while */

  if ( lpItem ) {

   if ( lpItem != apMenuCtx -> m_pLast )

    _menu_draw_dimmed_item (  apMenuCtx, lpItem, lTop + ( lpItem -> m_Y - apMenuCtx -> m_Offset ) * 34  );

   else _menu_draw_item (  apMenuCtx, lpItem, lTop + ( lpItem -> m_Y - apMenuCtx -> m_Offset ) * 34 );

  }  /* end if */

 }  /* end if */

}  /* end Menu_Draw */

MenuItem* Menu_Navigate ( MenuContext* apMenuCtx, unsigned long int anEvent ) {

 int       lIdx;
 int       lH     = ( apMenuCtx -> m_Height - 76 ) / 36;
 MenuItem* lpItem = apMenuCtx -> m_pCurr;

 lIdx = lpItem -> m_Y - apMenuCtx -> m_Offset;

 if (  ( anEvent & PAD_DOWN ) && lpItem -> m_pNext  ) {

  lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  if ( lpItem -> Leave ) lpItem -> Leave ();

  if (  !( lIdx < lH - 1 || lpItem -> m_pNext -> m_pNext == NULL ) || ( apMenuCtx -> m_Flags & MENU_F_TEXT )  ) {

   ++apMenuCtx -> m_Offset;
   apMenuCtx -> m_pFirst = apMenuCtx -> m_pFirst -> m_pNext;

  }  /* end if */

  if (  !( apMenuCtx -> m_Flags & MENU_F_TEXT )  ) {

   lpItem -> m_pNext -> m_Flags |= MENU_IF_SELECTED;

   if ( lpItem -> m_pNext -> Enter ) lpItem -> m_pNext -> Enter ();

  }  /* end if */

  apMenuCtx -> m_pCurr = lpItem -> m_pNext;

  if ( apMenuCtx -> m_pSelIdx ) ++*apMenuCtx -> m_pSelIdx;

 } else if (  ( anEvent & PAD_UP ) && lpItem -> m_pPrev  ) {

  lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  if ( lpItem -> Leave ) lpItem -> Leave ();

  if (  !( lIdx > 1 || lpItem -> m_pPrev -> m_pPrev == NULL ) || ( apMenuCtx -> m_Flags & MENU_F_TEXT )  ) {

   --apMenuCtx -> m_Offset;
   apMenuCtx -> m_pFirst = apMenuCtx -> m_pFirst -> m_pPrev;

  }  /* end if */

  if (  !( apMenuCtx -> m_Flags & MENU_F_TEXT )  ) {

   lpItem -> m_pPrev -> m_Flags |= MENU_IF_SELECTED;

   if ( lpItem -> m_pPrev -> Enter ) lpItem -> m_pPrev -> Enter ();

  }  /* end if */

  apMenuCtx -> m_pCurr = lpItem -> m_pPrev;

  if ( apMenuCtx -> m_pSelIdx ) --*apMenuCtx -> m_pSelIdx;

 } else if ( anEvent & PAD_TRIANGLE ) {

  if ( lpItem -> Leave ) lpItem -> Leave ();

  return ( MenuItem* )MENU_EV_EXIT;

 } else if (  ( anEvent & PAD_CROSS ) && lpItem -> Handler  ) {

  lpItem -> Handler ( 1 );

 } else if (  ( anEvent & PAD_CIRCLE ) && lpItem -> Handler  ) lpItem -> Handler ( -1 );

 Menu_Draw ( apMenuCtx );

 if ( apMenuCtx -> PostPaint ) apMenuCtx -> PostPaint ();

 return ( MenuItem* )MENU_EV_CONSUMED;

}  /* end _menu_navigate */

static void _menu_fill_root ( void ) {

 unsigned int lIdx = 0;
 MenuItem*    lpItem;

 Menu_Fill ( &s_MenuCtx, "SMS menu", s_SMSMenu );

 FillMenu = NULL;
 s_MenuCtx.m_pSelIdx = &s_SelIdxRoot;

 lpItem = s_MenuCtx.m_pItems;

 while ( lpItem ) {

  if (  lIdx++ == *s_MenuCtx.m_pSelIdx && !( s_MenuCtx.m_Flags & MENU_F_TEXT )  ) {

   lpItem -> m_Flags |= MENU_IF_SELECTED;
   s_MenuCtx.m_pCurr  = lpItem;

  } else lpItem -> m_Flags &= ~MENU_IF_SELECTED;

  lpItem = lpItem -> m_pNext;

 }  /* end while */

}  /* end _menu_fill_root */

static unsigned char s_ResBuffer[ 2 ];

static void _display_handler ( int aDir ) {

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
 s_SMSMenuDisplay[ 3 ].m_pIconRight = g_Config.m_PlayerFlags & SMS_PF_BLUR ? g_IconOn : g_IconOff;

 Menu_Fill ( &s_MenuCtx, "Display settings", s_SMSMenuDisplay );

}  /* end _display_handler */

static void _tvsys_handler ( int aDir ) {

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

 g_Config.m_DX = g_GSCtx.m_StartX;
 g_Config.m_DY = g_GSCtx.m_StartY;

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 1 );

}  /* end _tvsys_handler */

static void _resm_handler ( int aDir ) {

 if ( ++g_Config.m_ResMode > 1 ) g_Config.m_ResMode = 0;

 sprintf ( s_ResBuffer, "%d", g_Config.m_ResMode + 1 );

 GS_InitContext ( g_Config.m_DisplayMode );
 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 1 );

}  /* end _resm_handler */

static void _charset_handler ( int aDir ) {

 void* lpPtr;

 switch ( g_Config.m_DisplayCharset ) {

  case GSCodePage_WinLatin2  : g_Config.m_DisplayCharset = GSCodePage_WinCyrillic; lpPtr = "WinCyrillic"; break;
  case GSCodePage_WinCyrillic: g_Config.m_DisplayCharset = GSCodePage_WinGreek;    lpPtr = "WinGreek";    break;
  default                    :
  case GSCodePage_WinGreek   : g_Config.m_DisplayCharset = GSCodePage_WinLatin1;   lpPtr = "WinLatin1";   break;
  case GSCodePage_WinLatin1  : g_Config.m_DisplayCharset = GSCodePage_WinLatin2;   lpPtr = "WinLatin2";   break;

 }  /* end switch */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;

 g_GSCtx.SetCodePage ( g_Config.m_DisplayCharset );
 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _charset_handler */

static void _blur_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_PlayerFlags, SMS_PF_BLUR );

 GS_InitContext ( g_Config.m_DisplayMode );
 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 1 );

}  /* end _rflick_handler */

static void _alt_handler ( int aDir ) {

 g_GSCtx.AdjustDisplay ( -1, 0 );
 g_Config.m_DX = g_GSCtx.m_StartX;

}  /* end _alt_handler */

static void _art_handler ( int aDir ) {

 g_GSCtx.AdjustDisplay ( 1, 0 );
 g_Config.m_DX = g_GSCtx.m_StartX;

}  /* end _art_handler */

static void _aup_handler ( int aDir ) {

 g_GSCtx.AdjustDisplay ( 0, -1 );
 g_Config.m_DY = g_GSCtx.m_StartY;

}  /* end _aup_handler */

static void _adn_handler ( int aDir ) {

 g_GSCtx.AdjustDisplay ( 0, 1 );
 g_Config.m_DY = g_GSCtx.m_StartY;

}  /* end _adn_handler */

static void _network_handler ( int aDir ) {

 FillMenu = _menu_fill_root;

 s_SMSMenuNetwork[ 0 ].m_pIconRight = g_Config.m_NetworkFlags & SMS_NF_AUTO ? g_IconOn : g_IconOff;

 Menu_Fill ( &s_MenuCtx, "Network settings", s_SMSMenuNetwork );

 if (  !( g_SMSFlags & SMS_FLAG_NET ) && ( g_SMSFlags & SMS_FLAG_DEV9 )  )

  Menu_AddItem (
   &s_MenuCtx, "Start network interface now", NULL, NULL, 0, _netstrt_handler
  );

}  /* end _network_handler */

static void _netas_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_NetworkFlags, SMS_NF_AUTO );

}  /* end _netas_handler */

static void _netstrt_handler ( int aDir ) {

 SMS_StartNetwork ( s_pBrowserCtx -> m_pGUICtx );

 if (  !( g_SMSFlags & SMS_FLAG_NET )  ) {

  s_pBrowserCtx -> m_pGUICtx -> Status ( "Error. Press X to continue..." );
  GUI_WaitButton ( PAD_CROSS, 200 );

 } else _network_handler ( aDir );

 s_pBrowserCtx -> m_pGUICtx -> Status ( " " );

}  /* end _netstrt_handler */

static void _browser_handler ( int aDir ) {

 FillMenu = _menu_fill_root;

 s_SMSMenuBrowser[ 0 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SKIN ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 1 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SORT ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 2 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_AVIF ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 3 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_HDLP ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 4 ].m_pIconRight = g_Config.m_BrowserFlags & SMS_BF_SYSP ? g_IconOn : g_IconOff;
 s_SMSMenuBrowser[ 5 ].m_pIconRight = &g_Config.m_BrowserABCIdx;
 s_SMSMenuBrowser[ 6 ].m_pIconRight = &g_Config.m_BrowserIBCIdx;
 s_SMSMenuBrowser[ 7 ].m_pIconRight = &g_Config.m_BrowserTxtIdx;
 s_SMSMenuBrowser[ 8 ].m_pIconRight = &g_Config.m_BrowserSCIdx;
 s_SMSMenuBrowser[ 9 ].m_pIconRight = &g_Config.m_BrowserSBCIdx;
 Menu_Fill ( &s_MenuCtx, "Browser settings", s_SMSMenuBrowser );

}  /* end _browser_handler */

static void _skin_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SKIN );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _skin_handler */

static void _sort_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SORT );

}  /* end _sort_handler */

static void _avif_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_AVIF );

}  /* end _avif_handler */

static void _hdlp_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_HDLP );

}  /* end _hdlp_handler */

static void _sysp_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_BrowserFlags, SMS_BF_SYSP );

}  /* end _sysp_handler */

static void _abc_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_BrowserABCIdx );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _abc_handler */

static void _ibc_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_BrowserIBCIdx );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _ibc_handler */

static void _fntc_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_BrowserTxtIdx );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _fntc_handler */

static void _slbc_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_BrowserSCIdx );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _slbc_handler */

static void _sltc_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_BrowserSBCIdx );

 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _sltc_handler */

static char s_VolumeBuffer[  5 ] __attribute__(   (  section( ".data" )  )   );
static char s_OffsetBuffer[ 32 ] __attribute__(   (  section( ".data" )  )   );
static char s_ScrollBuffer[  9 ] __attribute__(   (  section( ".data" )  )   );;

char g_PowoffBuffer[ 32 ] __attribute__(   (  section( ".data" )  )   );

void _adjust_poweroff ( int anIncr ) {

 int lTime = ( int )g_Config.m_PowerOff;

 lTime += anIncr;

 if ( lTime < 0 ) {

  strcpy ( g_PowoffBuffer, "auto" );
  lTime = -60000;

 } else if ( lTime == 0 ) {

  strcpy ( g_PowoffBuffer, "off" );

 } else {

  if ( lTime > 5400000 ) lTime = 5400000;

  sprintf ( g_PowoffBuffer, "%d min", lTime / 60000 );

 }  /* end else */

 g_Config.m_PowerOff = lTime;

}  /* end _adjust_poweroff */

static void _player_handler ( int aDir ) {

 char* lpPtr;

 FillMenu = _menu_fill_root;

 s_SMSMenuPlayer[ 0 ].m_pIconRight = s_VolumeBuffer;

 switch ( g_Config.m_PlayerSAlign ) {

  default:
  case 0 : lpPtr = "center"; break;
  case 1 : lpPtr = "left";   break;

 }  /* end switch */

 s_SMSMenuPlayer[ 1 ].m_pIconRight = lpPtr;
 s_SMSMenuPlayer[ 2 ].m_pIconRight = s_OffsetBuffer;
 s_SMSMenuPlayer[ 3 ].m_pIconRight = g_Config.m_PlayerFlags & SMS_PF_SUBS ? g_IconOn : g_IconOff;
 s_SMSMenuPlayer[ 4 ].m_pIconRight = &g_Config.m_PlayerSCNIdx;
 s_SMSMenuPlayer[ 5 ].m_pIconRight = &g_Config.m_PlayerSCBIdx;
 s_SMSMenuPlayer[ 6 ].m_pIconRight = &g_Config.m_PlayerSCIIdx;
 s_SMSMenuPlayer[ 7 ].m_pIconRight = &g_Config.m_PlayerSCUIdx;
 s_SMSMenuPlayer[ 8 ].m_pIconRight = g_PowoffBuffer;
 s_SMSMenuPlayer[ 9 ].m_pIconRight = s_ScrollBuffer;

 _adjust_poweroff ( 0 );

 sprintf (    s_OffsetBuffer, "%d",     g_Config.m_PlayerSubOffset                                                      );
 sprintf (    s_VolumeBuffer, "%d%%",   ( int )(   (  ( float )g_Config.m_PlayerVolume / 24.0F ) * 100.0F + 0.5F   )    );
 sprintf (    s_ScrollBuffer, "%d pts", g_Config.m_ScrollBarNum                                                         );

 switch ( g_Config.m_ScrollBarPos ) {

  case SMScrollBarPos_Top     : lpPtr = "top";    break;
  case SMScrollBarPos_Bottom  : lpPtr = "bottom"; break;
  case SMScrollBarPos_Inactive: lpPtr = "off";    break;

 }  /* end switch */

 s_SMSMenuPlayer[ 10 ].m_pIconRight = lpPtr;
 s_SMSMenuPlayer[ 11 ].m_pIconRight = g_Config.m_PlayerFlags & SMS_PF_TIME ? g_IconOn : g_IconOff;
 s_SMSMenuPlayer[ 12 ].m_pIconRight = &g_Config.m_PlayerSBCIdx;
 s_SMSMenuPlayer[ 13 ].m_pIconRight = &g_Config.m_PlayerVBCIdx;
 s_SMSMenuPlayer[ 14 ].m_pIconRight = g_Config.m_PlayerFlags & SMS_PF_ANIM ? g_IconOn : g_IconOff;

 Menu_Fill ( &s_MenuCtx, "Player settings", s_SMSMenuPlayer );

}  /* end _player_handler */

static void _volume_handler ( int aDir ) {

 if ( aDir > 0 ) {

  if ( ++g_Config.m_PlayerVolume == 25 ) g_Config.m_PlayerVolume = 0;

 } else if ( g_Config.m_PlayerVolume ) --g_Config.m_PlayerVolume;

 sprintf (    s_VolumeBuffer, "%d%%", ( int )(   (  ( float )g_Config.m_PlayerVolume / 24.0F ) * 100.0F + 0.5F  )    );

}  /* end _volume_handler */

static void _subttl_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_PlayerFlags, SMS_PF_SUBS );

}  /* end _subttl_handler */

static void _alignm_handler ( int aDir ) {

 char* lpPtr;

 switch ( g_Config.m_PlayerSAlign ) {

  default:
  case 0 : g_Config.m_PlayerSAlign = 1; lpPtr = "left";   break;
  case 1 : g_Config.m_PlayerSAlign = 0; lpPtr = "center"; break;

 }  /* end switch */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;

}  /* end _alignm_handler */

static void _suboff_handler ( int aDir ) {

 if ( aDir > 0 ) {

  if ( ++g_Config.m_PlayerSubOffset == 128 ) g_Config.m_PlayerSubOffset = 0;

 } else if ( g_Config.m_PlayerSubOffset ) --g_Config.m_PlayerSubOffset;

 sprintf ( s_OffsetBuffer, "%d", g_Config.m_PlayerSubOffset );

 g_GSCtx.VSync ();
 s_pBrowserCtx -> m_pGUICtx -> Redraw ( -1 );

}  /* end _suboff_handler */

static void _subsample_paint ( void ) {

 int lY = g_GSCtx.m_Height - 32 - g_Config.m_PlayerSubOffset;
 int lW = g_GSCtx.TextWidth ( "Sample", 6 );

 g_GSCtx.DrawText ( 10,                        lY, 0, "Sample", 6, 0 );
 g_GSCtx.DrawText ( g_GSCtx.m_Width - lW - 10, lY, 0, "Sample", 6, 0 );

}  /* end _subsample_paint */

static void _dummy ( void ) {

}  /* end _dummy */

static void _suboff_enter ( void ) {

 s_MenuCtx.PrePaint  = _dummy;
 s_MenuCtx.PostPaint = _subsample_paint;

}  /* end _suboff_enter */

static void _suboff_leave ( void ) {

 s_MenuCtx.PostPaint = NULL;
 s_MenuCtx.PrePaint  = NULL;
 s_MenuCtx.m_Flags  |= MENU_F_VSYN;
 g_GSCtx.VSync ();
 s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );

}  /* end _suboff_leave */

void _scolor_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerSCNIdx );

}  /* end _scolor_handler */

void _sbcolr_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerSCBIdx );

}  /* end _sbcolr_handler */

void _sicolr_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerSCIIdx );

}  /* end _sicolr_handler */

void _sucolr_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerSCUIdx );

}  /* end _sucolr_handler */

void _powoff_hander ( int aDir ) {

 aDir *= 60000;

 _adjust_poweroff ( aDir );

}  /* end _powoff_hander */

static void _scroll_handler ( int aDir ) {

 if ( aDir < 0 ) {

  g_Config.m_ScrollBarNum -= 16;
 
  if ( g_Config.m_ScrollBarNum == 16 ) g_Config.m_ScrollBarNum = 128;

 } else {

  g_Config.m_ScrollBarNum += 16;
 
  if ( g_Config.m_ScrollBarNum == 144 ) g_Config.m_ScrollBarNum = 32;

 }  /* end else */

 sprintf ( s_ScrollBuffer, "%d pts", g_Config.m_ScrollBarNum );

}  /* end _scroll_handler */

static void _sbpos_handler ( int aDir ) {

 void* lpPtr;

 switch ( g_Config.m_ScrollBarPos ) {

  default                     :
  case SMScrollBarPos_Top     : g_Config.m_ScrollBarPos = SMScrollBarPos_Bottom;   lpPtr = "bottom"; break;
  case SMScrollBarPos_Bottom  : g_Config.m_ScrollBarPos = SMScrollBarPos_Inactive; lpPtr = "off";    break;
  case SMScrollBarPos_Inactive: g_Config.m_ScrollBarPos = SMScrollBarPos_Top;      lpPtr = "top";    break;

 }  /* end switch */

 s_MenuCtx.m_pCurr -> m_pIconRight = lpPtr;

}  /* end _sbpos_handler */

static void _sbtim_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_PlayerFlags, SMS_PF_TIME );

}  /* end _sbtim_handler */

static void _sbclr_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerSBCIdx );

}  /* end _sbclr_handler */

static void _vbclr_handler ( int aDir ) {

 _rotate_palette ( &g_Config.m_PlayerVBCIdx );

}  /* end _vbclr_handler */

static void _aadsp_handler ( int aDir ) {

 _switch_flag ( &g_Config.m_PlayerFlags, SMS_PF_ANIM );

}  /* end _aadsp_handler */

static char* s_Help[] = {
 "At startup:",
 "sel+R1 - NTSC mode",
 "sel+R2 - PAL mode",
 "sel+L1 - VESA@60Hz mode",
 "sel+L1 - VESA@75Hz mode",
 "sel+R1+square - NTSC_I mode",
 "sel+R2+square - PAL_I mode",
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
 "L1+L2+R1+R2 - display about",
 " ",
 "Player:",
 "up/down - adjust volume",
 "right/left - FFWD/REW mode",
 "cross - exit FFWD/REW mode",
 "triangle - stop",
 "select - pause/timeline",
 "start - resume/menu",
 "cross - OSD timer",
 "rectangle - pan-scan mode",
 "L1 - pan left",
 "R1 - pan right",
 " ",
 "SMS menu:",
 "cross/circle - action/next level",
 "triangle - level up/exit menu",
 NULL

};

static void _help_handler ( int aDir ) {

 int i = 0;

 _menu_clear ( &s_MenuCtx );
 s_MenuCtx.m_pName  = "Quick help";
 s_MenuCtx.m_Flags |= MENU_F_TEXT;

 FillMenu = _menu_fill_root;
 
 while ( s_Help[ i ] ) Menu_AddItem ( &s_MenuCtx, s_Help[ i++ ], NULL, NULL, 0, NULL );

}  /* end _help_handler */

static void _save_handler ( int aDir ) {

 s_pBrowserCtx -> m_pGUICtx -> Status ( "Saving configuration..." );

 strncpy ( g_Config.m_Partition, s_pBrowserCtx -> m_pActivePartition, 255 );

 if (  !SaveConfig ()  ) {

  s_pBrowserCtx -> m_pGUICtx -> Status ( "Error. Press X to continue..." );
  GUI_WaitButton ( PAD_CROSS, 200 );

 }  /* end if */

 s_pBrowserCtx -> m_pGUICtx -> Status ( " " );

}  /* end _save_handler */

static void _exit_handler ( int aDir ) {

 hddPowerOff ();

}  /* end _exit_handler */

static void _finish_handler ( int aDir ) {

 SMS_ResetIOP ();
 Exit ( 0 );

}  /* end _finish_handler */

static void _menu_run ( void ) {

 s_pBrowserCtx -> m_pGUICtx -> Status ( " " );

 _menu_fill_root ();
 Menu_Draw ( &s_MenuCtx );

 while ( 1 )

  if (  ( int )Menu_Navigate (  &s_MenuCtx, GUI_WaitButton ( 0xFFFF, 0 )  ) == MENU_EV_EXIT  ) {

   if ( FillMenu != NULL ) {

    FillMenu ();
    Menu_Draw ( &s_MenuCtx );

   } else {

    s_pBrowserCtx -> m_pGUICtx -> Redraw ( 0 );
    break;

   }  /* end else */

  }  /* end if */

}  /* end _menu_run */

MenuContext* MenuContext_Init ( BrowserContext* apBrowserCtx ) {

 s_MenuCtx.m_pItems  = NULL;
 s_MenuCtx.m_pCurr   = NULL;
 s_MenuCtx.m_pFirst  = NULL;
 s_MenuCtx.m_pLast   = NULL;
 s_MenuCtx.m_Offset  = 0;
 s_MenuCtx.m_Color   = GS_SETREG_RGBA( 0x20, 0x20, 0x40, 0x00 );
 s_MenuCtx.Run       = _menu_run;
 s_MenuCtx.PostPaint = NULL;

 s_pBrowserCtx = apBrowserCtx;

 return &s_MenuCtx;

}  /* end MenuContext_Init */
