/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_GUI.h"
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_Timer.h"
#include "SMS_GUIcons.h"
#include "SMS_GUIClock.h"
#include "SMS_Config.h"
#include "SMS_VIF.h"
#include "SMS_PAD.h"
#include "SMS_GUIMenu.h"
#include "SMS_Locale.h"
#include "SMS_IPU.h"
#include "SMS_SPU.h"
#include "SMS_IOP.h"
#include "SMS_Sounds.h"
#include "SMS_MC.h"
#include "SMS_RC.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <libhdd.h>

#define LOGO_W 17
#define LOGO_H  5

extern void About ( void );

static struct {

 int m_X;
 int m_Y;

} s_LogoXY[ 51 ] = {
 {  4, 0 }, {  3, 0 }, {  2, 0 }, {  1, 0 }, {  0, 0 },
 {  0, 1 }, {  0, 2 }, {  1, 2 }, {  2, 2 }, {  3, 2 },
 {  4, 2 }, {  4, 3 }, {  4, 4 }, {  3, 4 }, {  2, 4 },
 {  1, 4 }, {  0, 4 }, {  6, 4 }, {  6, 3 }, {  6, 2 },
 {  6, 1 }, {  6, 0 }, {  7, 0 }, {  8, 0 }, {  8, 1 },
 {  8, 2 }, {  8, 3 }, {  8, 4 }, {  9, 0 }, { 10, 0 },
 { 10, 1 }, { 10, 2 }, { 10, 3 }, { 10, 4 }, { 16, 0 },
 { 15, 0 }, { 14, 0 }, { 13, 0 }, { 12, 0 }, { 12, 1 },
 { 12, 2 }, { 13, 2 }, { 14, 2 }, { 15, 2 }, { 16, 2 },
 { 16, 3 }, { 16, 4 }, { 15, 4 }, { 14, 4 }, { 13, 4 }, { 12, 4 }
};

typedef struct _Version {

 DECLARE_GUI_OBJECT()

} _Version;

static int            s_Init     __attribute__(   (  section( ".data" )  )   ) = 0;
static GSLoadImage    s_BitBltSL;
static void*          s_pSLArea  __attribute__(   (  aligned( 64 )  )   );
static unsigned long* s_pDMASL;
static unsigned int   s_nDMASL;

static void _Version_Render ( GUIObject* apObj, int aCtx ) {

 if ( !apObj -> m_pGSPacket ) {

  int  lX, lY, lW, lLen;
  char lFmt [ 32 ];
  char lBuff[ 64 ];

  lFmt[  0 ] = 'V';
  lFmt[  1 ] = 'e';
  lFmt[  2 ] = 'r';
  lFmt[  3 ] = 's';
  lFmt[  4 ] = 'i';
  lFmt[  5 ] = 'o';
  lFmt[  6 ] = 'n';
  lFmt[  7 ] = ' ';
  lFmt[  8 ] = '%';
  lFmt[  9 ] = '.';
  lFmt[ 10 ] = '1';
  lFmt[ 11 ] = 'f';
  lFmt[ 12 ] = ' ';
  lFmt[ 13 ] = '(';
  lFmt[ 14 ] = 'R';
  lFmt[ 15 ] = 'e';
  lFmt[ 16 ] = 'v';
  lFmt[ 17 ] = '.';
  lFmt[ 18 ] = '%';
  lFmt[ 19 ] = 'd';
  lFmt[ 20 ] = ')';
  lFmt[ 21 ] = '\x00';
  sprintf ( lBuff, lFmt, 2.9F, 4 );

  lLen = strlen ( lBuff );
  lW   = GSFont_WidthEx ( lBuff, lLen, -6 );
  lX   = (  ( g_GSCtx.m_Width  - LOGO_W * 32 ) >> 1  ) + 32 * LOGO_W - lW;
  lY   = (  ( g_GSCtx.m_Height - LOGO_H * 32 ) >> 1  ) + 32 * s_LogoXY[ 50 ].m_Y + 32;

  apObj -> m_pGSPacket = GSContext_NewList (  GS_TXT_PACKET_SIZE( lLen )  );

  g_GSCtx.m_TextColor = 3;
  GSFont_RenderEx ( lBuff, lLen, lX, lY, apObj -> m_pGSPacket, -6, -12 );

 }  /* end if */

 GSContext_CallList ( aCtx, apObj -> m_pGSPacket );

}  /* end _Version_Render */

GUIObject* GUI_CreateVersion ( void ) {

 _Version* retVal = ( _Version* )calloc (  1, sizeof ( _Version )  );

 retVal -> Render  = _Version_Render;
 retVal -> Cleanup = GUIObject_Cleanup;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateVersion */

extern void PowerOf2 ( int, int, int*, int* );

static int DrawSkin ( void ) {

 int  lFD; 
 int  retVal = 0;
 char lPath[ 128 ];

 strcpy ( lPath, g_pSMSSkn + 5       );
 strcat ( lPath, g_SlashStr          );
 strcat ( lPath, g_Config.m_SkinName );
 strcat ( lPath, g_pSMI              );

 lFD = MC_OpenS ( g_MCSlot, 0, lPath, O_RDONLY );

 if ( lFD >= 0 ) { 

  long           lSize; 
  unsigned char* lpData; 

  lSize = MC_SeekS ( lFD, 0, SEEK_END ); 
  MC_SeekS ( lFD, 0, SEEK_SET ); 

  lpData = malloc ( lSize ); 

  if ( lpData ) {

   unsigned short lWidth, lHeight;

   MC_ReadS ( lFD, lpData, lSize );

   lWidth = IPU_ImageInfo ( lpData, &lHeight );

   if ( lWidth ) {

    IPULoadImage   lLoadImg;
    unsigned long* lpDMA;

    g_GSCtx.m_TBW = ( lWidth + 63 ) >> 6;

    g_GSCtx.m_VRAMTexPtr = 0x4000 - (
     (   ( g_GSCtx.m_TBW << 6 ) * (  ( lHeight + 31 ) & ~31  ) * 4   ) >> 8
    );
    PowerOf2 ( lWidth, lHeight, &g_GSCtx.m_TW, &g_GSCtx.m_TH );

    IPU_InitLoadImage ( &lLoadImg, lWidth, lHeight );
    IPU_LoadImage ( &lLoadImg, lpData, lSize, 0, 0, 0, 0, 0 );

    if ( lLoadImg.m_fPal ) SMS_SetPalette ( lLoadImg.m_Pal );

    lLoadImg.Destroy ( &lLoadImg );

    lpDMA = GSContext_NewPacket (  0, GS_TSP_PACKET_SIZE(), GSPaintMethod_Init  );
    GSContext_RenderTexSprite (
     ( GSTexSpritePacket* )( lpDMA - 2 ),
     0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height, 0, 0, lWidth, lHeight
    );

	if (  GS_Params () -> m_GSCRTMode == GSVideoMode_DTV_1920x1080I  ) {

     lpDMA = GSContext_NewPacket (  0, GS_VGR_PACKET_SIZE(), GSPaintMethod_Continue  );
     GSContext_RenderVGRect (
      lpDMA, g_GSCtx.m_Width, 0, 1920, g_GSCtx.m_Height,
      GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0xFF, 0x00 ),
      GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0xFF, 0x00 )
     );

	}  /* end if */

    GSContext_Flush ( 0, GSFlushMethod_DeleteLists );

    GUI_SetColors ();
    GSFont_Init   ();

    retVal = 1;

   }  /* end if */

   free ( lpData );

  }  /* end if */

  MC_CloseS ( lFD );

 }  /* end if */

 return retVal;

}  /* end DrawSkin */

static void Desktop_Render ( GUIObject* apObj, int aCtx ) {

 if ( !apObj -> m_pGSPacket ) {

  int            i, lW, lH;
  unsigned long  lXYXY;
  int            lX    = ( g_GSCtx.m_Width  - LOGO_W * 32 ) >> 1;
  int            lY    = ( g_GSCtx.m_Height - LOGO_H * 32 ) >> 1;
  unsigned long* lpDMA = GSContext_NewPacket (  0, GS_VGR_PACKET_SIZE(), GSPaintMethod_InitClear  );
  GSStoreImage   lSIPkt;

  SMS_SetPalette ( NULL );

  if (   aCtx >= 0 && (  !g_Config.m_SkinName[ 0 ] || !DrawSkin ()  )   ) {

   unsigned long lBP[ 96 ] __attribute__(   (  aligned( 16 )  )   );

   GUI_LoadIcons ();

   GSContext_RenderVGRect (
    lpDMA, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height,
    GS_SET_RGBAQ( 0x00, 0x00, 0x40, 0x80, 0x00 ),
    GS_SET_RGBAQ( 0x00, 0x00, 0x00, 0x80, 0x00 )
   );
   GSContext_Flush ( 0, GSFlushMethod_KeepLists );

   for (  i = 0; i < sizeof ( s_LogoXY ) / sizeof ( s_LogoXY[ 0 ] ) / 3; ++i  ) {

    GSContext_NewPacket ( 0, 0, GSPaintMethod_Init );

    GUI_DrawIcon (
     GUICON_BALL,
     lX + 32 * s_LogoXY[ i ].m_X, lY + 32 * s_LogoXY[ i ].m_Y,
     GUIcon_Misc, &lBP[  0 ]
    );
    GUI_DrawIcon (
     GUICON_BALL,
     lX + 32 * s_LogoXY[ i + 17 ].m_X, lY + 32 * s_LogoXY[ i + 17 ].m_Y,
     GUIcon_Misc, &lBP[ 32 ]
    );
    GUI_DrawIcon (
     GUICON_BALL,
     lX + 32 * s_LogoXY[ i + 34 ].m_X, lY + 32 * s_LogoXY[ i + 34 ].m_Y,
     GUIcon_Misc, &lBP[ 64 ]
    );

    GSContext_CallList2 ( 0, &lBP[  0 ] );
    GSContext_CallList2 ( 0, &lBP[ 32 ] );
    GSContext_CallList2 ( 0, &lBP[ 64 ] );

    SyncDCache ( &lBP[ 0 ], &lBP[ 96 ] );

    GSContext_Flush ( 0, GSFlushMethod_KeepLists );

    if ( !s_Init ) SMS_TimerWait ( 30 );

   }  /* end for */

  } else {

   if ( aCtx < 0 ) aCtx = 0;

   GUI_LoadIcons ();

  }  /* end else */

  apObj -> m_pGSPacket = GSContext_NewList ( 2 );

  GS_InitStoreImage (
   &lSIPkt, 0, 0, 0, g_GSCtx.m_LWidth, g_GSCtx.m_PHeight
  );
  GS_StoreImage ( &lSIPkt, g_GSCtx.m_pDBuf );

  lXYXY = GS_L2P ( 0, g_GSCtx.m_Height - 38, g_GSCtx.m_LWidth, 38 );
  lX = ( lXYXY >>  0 ) & 0xFFFF;
  lY = ( lXYXY >> 16 ) & 0xFFFF;
  lW = ( lXYXY >> 32 ) & 0xFFFF;
  lH = ( lXYXY >> 48 ) & 0xFFFF;
  GS_InitLoadImage (
   UNCACHED_SEG( &s_BitBltSL ), 0, g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.FBW,
   g_GSCtx.m_DrawCtx[ 0 ].m_FRAMEVal.PSM, lX, lY, lW, lH
  );
  s_pSLArea = g_GSCtx.m_pDBuf + g_GSCtx.m_LWidth * lY * g_GSCtx.m_PixSize;

  s_Init = 1;

 }  /* end if */

 GSContext_NewPacket ( aCtx, 0, GSPaintMethod_Init );

}  /* end Desktop_Render */

extern void _adjleft_handler  ( GUIMenu*, int );
extern void _adjright_handler ( GUIMenu*, int );
extern void _adjup_handler    ( GUIMenu*, int );
extern void _adjdown_handler  ( GUIMenu*, int );
extern void _save_handler     ( GUIMenu*, int );
extern void _shutdown_handler ( GUIMenu*, int );
extern void _exit_handler     ( GUIMenu*, int );

static int Desktop_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 int retVal = GUIHResult_Void;

 if ( anEvent & GUI_MSG_PAD_MASK ) switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case RC_MENU      :
  case SMS_PAD_START: {

   GUI_AddObject (  g_SMSMenuStr, GUI_CreateMenuSMS ()  );
   GUI_Redraw ( GUIRedrawMethod_Redraw );
   SMS_GUIClockRedraw ();

   retVal = GUIHResult_Handled;

  } break;

  case SMS_PAD_SELECT | SMS_PAD_L1      : _adjleft_handler  ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_R1      : _adjright_handler ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_L2      : _adjup_handler    ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_R2      : _adjdown_handler  ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_SQUARE  : _save_handler     ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_TRIANGLE: _exit_handler     ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case RC_RESET                         :
  case SMS_PAD_SELECT | SMS_PAD_CIRCLE  :
   g_Config.m_BrowserFlags &= ~SMS_BF_EXIT;
   _shutdown_handler ( NULL, 1 ); retVal = GUIHResult_Handled;
  break;

  case RC_A_B                                           :
  case SMS_PAD_R1 | SMS_PAD_L1 | SMS_PAD_R2 | SMS_PAD_L2: About (); retVal = GUIHResult_Handled; break;

  case RC_ON: SMS_IOPowerOff ();

 }  /* end switch */

 return retVal;

}  /* end Desktop_HandleEvent */

GUIObject* GUI_CreateDesktop ( void ) {

 GUIObject* retVal = ( GUIObject* )calloc (  1, sizeof ( GUIObject )  );

 retVal -> Render      = Desktop_Render;
 retVal -> HandleEvent = Desktop_HandleEvent;
 retVal -> Cleanup     = GUIObject_Cleanup;

 return retVal;

}  /* end GUI_CreateDesktop */

static void StatusLine_Render ( GUIObject* apObj, int aCtx ) {

 unsigned long** lppList = ( unsigned long** )&apObj[ 1 ];

 if ( !apObj -> m_pGSPacket ) {

  unsigned long* lpDMA  = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );
  unsigned long* lpDMA2 = GSContext_NewList ( 6 );
  unsigned int   lX, lY;

  lX = g_GSCtx.m_Width  - 88;
  lY = g_GSCtx.m_Height - 36;

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpDMA - 2 ),
   0, lY, g_GSCtx.m_Width - 1, 34, -12,
   g_Palette[ g_Config.m_BrowserIBCIdx - 1 ]
  );

  lpDMA2[ 0 ] = GIF_TAG( 1, 1, 0, 0, GIFTAG_FLG_REGLIST, 4 );
  lpDMA2[ 1 ] = GIFTAG_REGS_PRIM | ( GIFTAG_REGS_XYZ2 << 4 ) | ( GIFTAG_REGS_XYZ2 << 8 ) | ( GIFTAG_REGS_NOP << 12 );
  lpDMA2[ 2 ] = GS_SET_PRIM( GS_PRIM_PRIM_LINE, 0, 0, 0, 1, 1, 0, 0, 0 );
  lpDMA2[ 3 ] = GS_XYZ( lX,                   lY, 0 );
  lpDMA2[ 4 ] = GS_XYZ( lX, g_GSCtx.m_Height - 1, 0 );
  lpDMA2[ 5 ] = 0UL;

  apObj -> m_pGSPacket = lpDMA;
  lppList[ 0 ]         = lpDMA2;

  *( int* )UNCACHED_SEG(  ( char* )&s_BitBltSL + 132  ) = ( int )s_pSLArea;

 }  /* end if */

 GSContext_CallList2 (  aCtx, ( unsigned long* )&s_BitBltSL  );
 GSContext_CallList ( aCtx, apObj -> m_pGSPacket );
 GSContext_CallList ( aCtx, lppList[ 0 ] );

 if ( s_pDMASL ) GSContext_CallList ( aCtx, s_pDMASL + 2 );

}  /* end StatusLine_Render */

static void StatusLine_Cleanup ( GUIObject* apObj ) {

 unsigned long** lppList = ( unsigned long** )&apObj[ 1 ];

 GUIObject_Cleanup ( apObj );
 GSContext_DeleteList ( lppList[ 0 ] );

 lppList[ 0 ] = 0;

 free ( s_pDMASL );

 s_pDMASL = NULL;
 s_nDMASL = 0;

}  /* end StatusLine_Cleanup */

GUIObject* GUI_CreateStatusLine ( void ) {

 GUIObject* retVal = ( GUIObject* )calloc (  1, sizeof ( GUIObject ) + sizeof ( unsigned long* )  );

 retVal -> Render  = StatusLine_Render;
 retVal -> Cleanup = StatusLine_Cleanup;

 return retVal;

}  /* end GUI_CreateStatusLine */

void GUI_Status ( unsigned char* apMsg ) {

 int lLen   = strlen ( apMsg );
 int lWidth = g_GSCtx.m_Width - 96;
 int lDX    = -2;
 int lQWC;

 while (  GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth && lDX >= -12  ) --lDX;
 while (  GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth                ) --lLen;

 lWidth  = GS_TXT_PACKET_SIZE( lLen );
 lQWC    = lWidth >> 1;
 lWidth += 2;

 DMA_Wait ( DMAC_VIF1 );

 if ( s_nDMASL < lWidth ) {

  s_pDMASL = ( unsigned long* )realloc64 (  s_pDMASL, lWidth * sizeof ( unsigned long )  );
  s_nDMASL = lWidth;

 }  /* end if */

 g_GSCtx.m_TextColor = 0;

 s_pDMASL[ 0 ] = 0;
 s_pDMASL[ 1 ] = VIF_DIRECT( lQWC );
 GSFont_RenderEx ( apMsg, lLen, 8, g_GSCtx.m_Height - 34, s_pDMASL + 2, lDX, -2 );
 SyncDCache ( s_pDMASL, s_pDMASL + lWidth );

 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
 g_pStatusLine -> Render ( g_pStatusLine, 1 );
 SMS_GUIClockSuspend ();
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );
 SMS_GUIClockResume ();

}  /* end GUI_Status */

static int _wait_user ( unsigned char* apMsg, int anIcon, int anBtn, unsigned int* apBtn ) {

 int            lLen   = strlen ( apMsg );
 int            lWidth = g_GSCtx.m_Width - 128;
 int            lDX    = -2;
 unsigned long* lpDMA;
 unsigned long  lIcon[ 32 ] __attribute__(   (  aligned( 16 )  )   );

 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth && lDX >= -12  ) --lDX;
 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth                ) --lLen;

 g_GSCtx.m_TextColor = 0;
 GSContext_NewPacket (  1, 0, GSPaintMethod_Init  );
 GSContext_CallList2 (  1, ( unsigned long* )&s_BitBltSL  );
 lpDMA = GSContext_NewPacket (  1, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Continue  );
 GSFont_RenderEx ( apMsg, lLen, 40, lWidth = g_GSCtx.m_Height - 34, lpDMA, lDX, -2 );
 GUI_DrawIcon ( anIcon, 8, lWidth, GUIcon_Misc, lIcon );
 SyncDCache ( &lIcon[ 0 ], &lIcon[ 32 ] );
 lpDMA = (  ( unsigned long** )&g_pStatusLine[ 1 ]  )[ 0 ];
 GSContext_CallList2 ( 1, lIcon );
 GSContext_CallList  ( 1, g_pStatusLine -> m_pGSPacket );
 GSContext_CallList  ( 1, lpDMA );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );
 SPU_PlaySound ( SMSound_Error, g_Config.m_PlayerVolume );
 SMS_GUIClockStop ();
 SMS_GUIClockStart ( &g_Clock );
 lLen = GUI_WaitButtons ( anBtn, apBtn, 200 );
 GSContext_NewPacket (  1, 0, GSPaintMethod_Init  );
 SMS_GUIClockSuspend ();
 g_pStatusLine -> Render ( g_pStatusLine, 1 );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );
 SMS_GUIClockResume ();

 return lLen;

}  /* end _wait_user */

void GUI_Error ( unsigned char* apMsg ) {

 unsigned int lBtn[ 2 ] = { SMS_PAD_CROSS, RC_ENTER };

 _wait_user ( apMsg, GUICON_ERROR, 2, lBtn );

}  /* end GUI_Error */

int GUI_Question ( unsigned char* apMsg ) {

 unsigned int lBtn[ 4 ] = {
  SMS_PAD_CROSS, SMS_PAD_TRIANGLE, RC_ENTER, RC_RETURN
 };

 int retVal = _wait_user ( apMsg, GUICON_HELP, 4, lBtn );

 return retVal == SMS_PAD_CROSS || retVal == RC_ENTER;

}  /* end GUI_Question */

void GUI_Progress ( unsigned char* apStr, int aPos, int afForceUpdate ) {

 static unsigned long* s_lpListTxt;
 static unsigned long* s_lpListRRT;
 static int            s_lLen;
 static int            s_lPos;

 if ( afForceUpdate ) {

  s_lLen = -1;
  s_lPos = -1;

 }  /* end if */

 if ( apStr ) {

  int lLen   = strlen ( apStr );
  int lWidth = g_GSCtx.m_Width - 16;

  while (   GSFont_WidthEx ( apStr, lLen, -2 ) > lWidth  ) --lLen;

  if ( !s_lpListRRT ) s_lpListRRT = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );

  if ( s_lLen != lLen ) {

   GSContext_DeleteList ( s_lpListTxt );
   s_lpListTxt = GSContext_NewList (  GS_TXT_PACKET_SIZE( s_lLen = lLen )  );

   g_GSCtx.m_TextColor = 0;
   GSFont_RenderEx ( apStr, lLen, 8, g_GSCtx.m_Height - 34, s_lpListTxt, -2, -2 );

  }  /* end if */

  if ( aPos < 2 )

   aPos = 2;

  else if ( aPos > 100 ) aPos = 100;

  if ( s_lPos != aPos ) {

   lWidth = (  ( g_GSCtx.m_Width - 4 ) * aPos  ) / 100;
   s_lPos = aPos;

   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( s_lpListRRT - 2 ), 4, g_GSCtx.m_Height - 35, lWidth - 4, 34, 12, 0x20FF8080
   );

   GSContext_NewPacket (  1, 0, GSPaintMethod_Init          );
   GSContext_CallList2 (  1, ( unsigned long* )&s_BitBltSL  );
   GSContext_CallList  (  1, g_pStatusLine -> m_pGSPacket   );
   GSContext_CallList  (  1, s_lpListRRT                    );
   GSContext_CallList  (  1, s_lpListTxt                    );
   GSContext_Flush     (  1, GSFlushMethod_KeepLists        );

  }  /* end if */

 } else {

  GSContext_NewPacket (  1, 0, GSPaintMethod_Init          );
  GSContext_CallList2 (  1, ( unsigned long* )&s_BitBltSL  );
  GSContext_CallList  (  1, g_pStatusLine -> m_pGSPacket   );
  GSContext_Flush     (  1, GSFlushMethod_KeepLists        );

  GSContext_DeleteList ( s_lpListRRT );
  GSContext_DeleteList ( s_lpListTxt );

  s_lpListRRT = NULL;
  s_lpListTxt = NULL;

 }  /* end else */

}  /* end GUI_Progress */
