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
#include "SMS_DMA.h"
#include "SMS_Timer.h"
#include "SMS_GUIcons.h"
#include "SMS_Config.h"
#include "SMS_VIF.h"
#include "SMS_PAD.h"
#include "SMS_GUIMenu.h"
#include "SMS_Locale.h"
#include "SMS_Codec.h"
#include "SMS_MPEG4.h"
#include "SMS_VideoBuffer.h"
#include "SMS_IPU.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define LOGO_W 17
#define LOGO_H  5

extern void About ( void );

static GSBitBltPacket s_BitBltPrg;

static char s_pSMSkn[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "mc0:SMS/SMS.skn";

static int DrawSkin ( void ) {

 int lFD    = fioOpen ( s_pSMSkn, O_RDONLY ); 
 int retVal = 0;

 if ( lFD >= 0 ) { 

  long           lSize; 
  unsigned char* lpData; 

  lSize = fioLseek ( lFD, 0, SEEK_END ); 
  fioLseek ( lFD, 0, SEEK_SET ); 

  lpData = malloc ( lSize ); 

  if ( lpData ) {

   fioRead ( lFD, lpData, lSize );

   if (  *( unsigned int* )lpData == 0x40534D53  ) {

    int              lWidth   = *( unsigned int* )( lpData + 4 );
    int              lHeight  = *( unsigned int* )( lpData + 8 );
    unsigned int     lVRAMPtr = g_GSCtx.m_VRAMPtr;
    IPUContext*      lpIPUCtx;
    SMS_CodecContext lCodecCtx; 

    g_GSCtx.m_VRAMPtr = g_GSCtx.m_VRAMPtr2 << 5;
    lpIPUCtx          = IPU_InitContext ( lWidth, lHeight );

    lCodecCtx.m_Width     = lWidth;
    lCodecCtx.m_Height    = lHeight;
    lCodecCtx.m_Type      = SMS_CodecTypeVideo;
    lCodecCtx.m_ID        = SMS_CodecID_MPEG4;
    lCodecCtx.m_IntBufCnt = 0; 
    lCodecCtx.m_pIntBuf   = NULL;
    lCodecCtx.m_HurryUp   = 0;

    if (  SMS_CodecOpen ( &lCodecCtx )  ) {

     if (  lCodecCtx.m_pCodec -> Init ( &lCodecCtx )  ) {

      SMS_FrameBuffer* lpFrame = NULL; 

      lCodecCtx.m_pCodec -> Decode ( 
       &lCodecCtx, ( void** )&lpFrame, lpData + 12, lSize - 12
      );

      if ( !lpFrame ) lpFrame = g_MPEGCtx.m_CurPic.m_pBuf;

      if ( lpFrame ) {

       lpIPUCtx -> Display ( lpFrame );
       lpIPUCtx -> Sync ();
       GUI_SetColors ();
       GSFont_Init   ();

       retVal = 1;

      }  /* end if */

      lCodecCtx.m_pCodec -> Destroy ( &lCodecCtx ); 

     }  /* end if */

     SMS_CodecClose ( &lCodecCtx ); 

    }  /* end if */

    lpIPUCtx -> Destroy (); 

    g_GSCtx.m_VRAMPtr = lVRAMPtr;

   }  /* end if */

   free ( lpData );

  }  /* end if */

  fioClose ( lFD );

 }  /* end if */

 return retVal;

}  /* end DrawSkin */

static void Desktop_Render ( GUIObject* apObj, int aCtx ) {

 if ( !apObj -> m_pGSPacket ) {

  GSBitBltPacket lBBPkt;

  if ( g_Config.m_BrowserFlags & SMS_BF_SKIN ) DrawSkin ();

  GUI_LoadIcons ();

  apObj -> m_pGSPacket = GSContext_NewList ( 2 );

  GSContext_InitBitBlt (
   &lBBPkt, g_GSCtx.m_VRAMPtr2, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height,
   0, 0, 0
  );
  GSContext_BitBlt ( &lBBPkt );

  GSContext_InitBitBlt (
   &s_BitBltPrg, 0, 0, g_GSCtx.m_Height - 35, g_GSCtx.m_Width, 34,
   g_GSCtx.m_VRAMPtr2, 0, g_GSCtx.m_Height - 34
  );

 }  /* end if */

 GSContext_NewPacket ( aCtx, 0, GSPaintMethod_Init );

}  /* end Desktop_Render */

static void Desktop_Cleanup ( GUIObject* apObj ) {

 GSContext_DeleteList ( apObj -> m_pGSPacket );
 apObj -> m_pGSPacket = NULL;

}  /* end Desktop_Cleanup */

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

  case SMS_PAD_START: {

   GUI_AddObject (  g_SMSMenuStr, GUI_CreateMenuSMS ()  );
   GUI_Redraw ( GUIRedrawMethod_Redraw );

   retVal = GUIHResult_Handled;

  } break;

  case SMS_PAD_SELECT | SMS_PAD_L1      : _adjleft_handler  ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_R1      : _adjright_handler ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_L2      : _adjup_handler    ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_R2      : _adjdown_handler  ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_SQUARE  : _save_handler     ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_TRIANGLE: _exit_handler     ( NULL, 1 ); retVal = GUIHResult_Handled; break;
  case SMS_PAD_SELECT | SMS_PAD_CIRCLE  : _shutdown_handler ( NULL, 1 ); retVal = GUIHResult_Handled; break;

  case SMS_PAD_R1 | SMS_PAD_L1 | SMS_PAD_R2 | SMS_PAD_L2: About (); retVal = GUIHResult_Handled; break;

 }  /* end switch */

 return retVal;

}  /* end Desktop_HandleEvent */

GUIObject* GUI_CreateDesktop ( void ) {

 GUIObject* retVal = ( GUIObject* )calloc (  1, sizeof ( GUIObject )  );

 retVal -> Render      = Desktop_Render;
 retVal -> HandleEvent = Desktop_HandleEvent;
 retVal -> Cleanup     = Desktop_Cleanup;

 return retVal;

}  /* end GUI_CreateDesktop */

static GSBitBltPacket s_BitBltSL;
static unsigned long* s_pDMASL;
static unsigned int   s_nDMASL;

static void StatusLine_Render ( GUIObject* apObj, int aCtx ) {

 if ( !apObj -> m_pGSPacket ) {

  unsigned long* lpDMA = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpDMA - 2 ),
   0, g_GSCtx.m_Height - 36, g_GSCtx.m_Width - 1, 35, -8,
   g_Palette[ g_Config.m_BrowserIBCIdx - 1 ]
  );

  apObj -> m_pGSPacket = lpDMA;

  GSContext_InitBitBlt (
   &s_BitBltSL, 0, 8, g_GSCtx.m_Height - 34,
   g_GSCtx.m_Width - 16, 33, g_GSCtx.m_VRAMPtr2, 8, g_GSCtx.m_Height - 35
  );

 }  /* end if */

 GSContext_CallList ( aCtx, apObj -> m_pGSPacket );

 if ( s_pDMASL ) GSContext_CallList ( aCtx, s_pDMASL + 2 );

}  /* end StatusLine_Render */

static void StatusLine_Cleanup ( GUIObject* apObj ) {

 Desktop_Cleanup ( apObj );

 free ( s_pDMASL );
 s_pDMASL = NULL;
 s_nDMASL = 0;

}  /* end StatusLine_Cleanup */

GUIObject* GUI_CreateStatusLine ( void ) {

 GUIObject* retVal = ( GUIObject* )calloc (  1, sizeof ( GUIObject )  );

 retVal -> Render  = StatusLine_Render;
 retVal -> Cleanup = StatusLine_Cleanup;

 return retVal;

}  /* end GUI_CreateStatusLine */

void GUI_Status ( unsigned char* apMsg ) {

 int lLen   = strlen ( apMsg );
 int lWidth = g_GSCtx.m_Width - 18;
 int lQWC;
 int lDX = 0;

 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth && lDX >= -12  ) --lDX;
 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth                ) --lLen;

 lWidth  = GS_TXT_PACKET_SIZE( lLen );
 lQWC    = lWidth >> 1;
 lWidth += 2;

 DMA_Wait ( DMAC_VIF1 );

 if ( s_nDMASL < lWidth ) {

  s_pDMASL = ( unsigned long* )realloc (  s_pDMASL, lWidth * sizeof ( unsigned long )  );
  s_nDMASL = lWidth;

 }  /* end if */

 g_GSCtx.m_TextColor = 0;

 s_pDMASL[ 0 ] = 0;
 s_pDMASL[ 1 ] = VIF_DIRECT( lQWC );
 GSFont_RenderEx ( apMsg, lLen, 8, g_GSCtx.m_Height - 34, s_pDMASL + 2, lDX, -2 );
 SyncDCache ( s_pDMASL, s_pDMASL + lWidth );

 GSContext_BitBlt ( &s_BitBltSL );
 DMA_Send ( DMAC_VIF1, s_pDMASL, lQWC + 1 );

}  /* end GUI_Status */

void GUI_Error ( unsigned char* apMsg ) {

 int            lLen   = strlen ( apMsg );
 int            lWidth = g_GSCtx.m_Width - 40;
 unsigned long* lpDMA;
 int            lDX = 0;

 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth && lDX >= -12  ) --lDX;
 while (   GSFont_WidthEx ( apMsg, lLen, lDX ) > lWidth                ) --lLen;

 g_GSCtx.m_TextColor = 0;
 lpDMA = GSContext_NewPacket (  1, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Init  );
 GSFont_RenderEx ( apMsg, lLen, 40, lWidth = g_GSCtx.m_Height - 34, lpDMA, 0, -2 );
 lpDMA = GSContext_NewPacket (  1, GS_TSP_PACKET_SIZE(), GSPaintMethod_Continue  );
 GUI_DrawIcon ( GUICON_ERROR, 8, lWidth, GUIcon_Misc, lpDMA );

 GSContext_BitBlt ( &s_BitBltSL );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );
 GUI_WaitButtons ( SMS_PAD_CROSS, 200 );

 GSContext_NewPacket (  1, 0, GSPaintMethod_Init  );
 GSContext_BitBlt ( &s_BitBltSL );
 g_pStatusLine -> Render ( g_pStatusLine, 1 );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

}  /* end GUI_Error */

void GUI_Progress ( unsigned char* apStr, int aPos ) {

 static unsigned long* s_lpListTxt;
 static unsigned long* s_lpListRRT;
 static int            s_lLen;
 static int            s_lPos;

 if ( apStr ) {

  int lLen   = strlen ( apStr );
  int lWidth = g_GSCtx.m_Width - 16;

  while (   GSFont_Width ( apStr, lLen ) > lWidth  ) --lLen;

  if ( !s_lpListRRT ) s_lpListRRT = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );

  if ( s_lLen != lLen ) {

   GSContext_DeleteList ( s_lpListTxt );
   s_lpListTxt = GSContext_NewList (  GS_TXT_PACKET_SIZE( s_lLen = lLen )  );

   g_GSCtx.m_TextColor = 0;
   GSFont_RenderEx ( apStr, lLen, 8, g_GSCtx.m_Height - 34, s_lpListTxt, 0, -2 );

  }  /* end if */

  if ( aPos < 2 )

   aPos = 2;

  else if ( aPos > 100 ) aPos = 100;

  if ( s_lPos != aPos ) {

   lWidth = (  ( g_GSCtx.m_Width - 4 ) * aPos  ) / 100;
   s_lPos = aPos;

   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( s_lpListRRT - 2 ), 1, g_GSCtx.m_Height - 35, lWidth, 34, 8, 0x20FF8080
   );

   GSContext_NewPacket ( 1, 0, GSPaintMethod_Init        );
   GSContext_CallList  ( 1, g_pStatusLine -> m_pGSPacket );
   GSContext_CallList  ( 1, s_lpListRRT                  );
   GSContext_CallList  ( 1, s_lpListTxt                  );
   GSContext_BitBlt    ( &s_BitBltPrg                    );
   GSContext_Flush     ( 1, GSFlushMethod_KeepLists      );

  }  /* end if */

 } else {

  GSContext_NewPacket ( 1, 0, GSPaintMethod_Init        );
  GSContext_CallList  ( 1, g_pStatusLine -> m_pGSPacket );
  GSContext_BitBlt    ( &s_BitBltPrg                    );
  GSContext_Flush     ( 1, GSFlushMethod_KeepLists      );

  GSContext_DeleteList ( s_lpListRRT );
  GSContext_DeleteList ( s_lpListTxt );

  s_lpListRRT = NULL;
  s_lpListTxt = NULL;

 }  /* end else */

}  /* end GUI_Progress */
