/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2001-2004, ps2dev - http://www.ps2dev.org
# (c) 2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GS.h"
#include "SMS_GUI.h"
#include "SMS_PAD.h"
#include "SMS_Timer.h"
#include "SMS_RC.h"
#include "SMS_GUIClock.h"

#include <string.h>

#define NSTR 60

extern char g_AboutData[] __attribute__(   (  section( ".text" )  )   );

static char* s_About[ NSTR ] __attribute__(   (  section( ".bss" )  )   );
static int   s_Unhash;

static void UnHash ( void ) {

 int            i;
 unsigned char  lChr  = 'E';
 unsigned char* lpSrc = g_AboutData;

 for ( i = 0; i < NSTR; ++i ) {

  int            j;
  unsigned char* lpPtr = lpSrc;
  int            lLen  = *lpPtr + 1;

  lpPtr[ 1 ] ^= lChr;
  lChr        = lpPtr[ 1 ];

  for ( j = 2; j < lLen; ++j ) {

   lpPtr[ j ] ^= lChr;

   lChr = lpPtr[ j ];

  }  /* end for */

  s_About[ i ] = lpSrc + 1;
  lpSrc       += lLen  + 1;

 }  /* end for */

}  /* end UnHash */

void About ( void ) {

 int i, lY, lHeight;

 SMS_GUIClockStop ();

 if ( !s_Unhash ) {

  UnHash ();
  s_Unhash = 1;

 }  /* end if */

 lY = lHeight = g_GSCtx.m_Height;
 g_GSCtx.m_TextColor = 0;

 while ( 1 ) {

  int            lLen, lX, lYY = lY;
  unsigned long* lpDMA;
  unsigned int   lBtnCode;

  GSContext_NewPacket ( 1, 0, GSPaintMethod_InitClear );

  for ( i = 0; i < NSTR; ++i, lYY += 32 ) {

   int lDW = 0;

   if ( lYY < -32     ) continue;
   if ( lYY > lHeight ) break;

   lLen = strlen ( s_About[ i ] );

   while ( 1 ) {

    lX = ( int )(  g_GSCtx.m_Width - GSFont_WidthEx ( s_About[ i ], lLen, lDW )  ) >> 1;

    if ( lX >= 0 ) break;

    --lDW;

   }  /* end while */

   lpDMA = GSContext_NewPacket (  1, GS_TXT_PACKET_SIZE( lLen ), GSPaintMethod_Continue  );
   GSFont_RenderEx ( s_About[ i ], lLen, lX, lYY, lpDMA, lDW, 0 );

  }  /* end for */

  lpDMA = GSContext_NewPacket (  1, GS_VGR_PACKET_SIZE(), GSPaintMethod_Continue  );
  GSContext_RenderVGRect (
   lpDMA, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height >> 1,
   0x80600000, 0x00000000
  );
  lpDMA = GSContext_NewPacket (  1, GS_VGR_PACKET_SIZE(), GSPaintMethod_Continue  );
  GSContext_RenderVGRect (
   lpDMA, 0, g_GSCtx.m_Height >> 1, g_GSCtx.m_Width, g_GSCtx.m_Height >> 1,
   0x00000000, 0x80600000
  );

  --lY;

  GS_VSync ();
  GSContext_Flush ( 1, GSFlushMethod_KeepLists );

  lBtnCode = GUI_ReadButtons ();

  if (  lY < NSTR * -32 || lBtnCode == SMS_PAD_TRIANGLE
                        || lBtnCode == RC_RESET
                        || lBtnCode == RC_STOP
                        || lBtnCode == RC_RETURN
  ) break;

  SMS_TimerWait ( 30 );

 }  /* end while */

 while (  GUI_ReadButtons ()  );

 GUI_UpdateStatus ();
 GUI_Redraw ( GUIRedrawMethod_Redraw );

 SMS_GUIClockStart ( &g_Clock );

}  /* end About */
