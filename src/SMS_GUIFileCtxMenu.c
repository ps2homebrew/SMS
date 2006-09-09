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
#include "SMS_GUIMenu.h"
#include "SMS_FileContext.h"
#include "SMS_FileDir.h"
#include "SMS_Locale.h"
#include "SMS_Timer.h"
#include "SMS_GS.h"
#include "SMS.h"
#include "SMS_PAD.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"

#include <tamtypes.h>
#include <fileXio_rpc.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/fcntl.h>

extern void GUIMenuSMS_Redraw           ( GUIMenu*                  );
extern int  CtxMenu_HandleEvent         ( GUIObject*, unsigned long );
extern int ( *CtxMenu_HandleEventBase ) ( GUIObject*, unsigned long );

static char* s_pFileName;
static char* s_pPathEnd;

static void _copy2hdd_handler  ( GUIMenu*, int );

static GUIMenuItem s_FileCtxMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, &STR_COPY_TO_HDD, 0, 0, _copy2hdd_handler, 0, 0 },
};

static void _copy2hdd_handler ( GUIMenu* apMenu, int aDir ) {

 FileContext* lpFileCtx;

 if ( g_CMedia == 1 && g_pCDDACtx )

  lpFileCtx = CDDA_InitFileContext ( &s_pFileName[ 7 ], g_pCDDACtx );

 else lpFileCtx = STIO_InitFileContext ( s_pFileName, 0 );

 if ( lpFileCtx ) {

  static char s_Fmt[] __attribute__(   (  section( ".data" )  )   ) = "%s (%.1f%s)...";

  char  lFileName[ 1024 ] __attribute__(   ( aligned( 4 )  )   );
  int   lFD;

  *( unsigned int* )&lFileName[ 0 ] = 0x30736670;
  *( unsigned int* )&lFileName[ 4 ] = 0x0000003A;

  SMS_Strcat ( lFileName, g_SlashStr );
  SMS_Strcat ( lFileName, s_pPathEnd );

  lFD = fileXioOpen ( lFileName, O_CREAT | O_WRONLY, 0666 );

  if ( lFD >= 0 ) {

   char         lSts[ 128 ];
   unsigned int lSize    = 262144;
   unsigned int lnCopied = 0;
   float        lSpeed   = 0.0F;
   char*        lpBuff   = ( char* )malloc ( lSize );
   int          lfError  = 0;
   unsigned int lStart   = g_Timer;

   lpFileCtx -> Stream ( lpFileCtx, 0, 128 );

   while ( 1 ) {

    int lnRead = lpFileCtx -> Read ( lpFileCtx, lpBuff, lSize );

    if ( lnRead <= 0 ) break;

    if (   fileXioWrite ( lFD, lpBuff, lnRead ) <= 0  ) {

     lfError = 1;
     break;

    }  /* end if */

    lnCopied += lnRead;
    lSpeed    = ( float )lnCopied / ( float )( g_Timer - lStart );

    sprintf ( lSts, s_Fmt, STR_COPYING.m_pStr, lSpeed, STR_KBS.m_pStr );
    GUI_Progress (   lSts, ( int )(  ( float )lnCopied / ( float )lpFileCtx -> m_Size * 100.0F  ), 1   );

    if (  GUI_ReadButtons () == SMS_PAD_TRIANGLE  ) {

     GUI_Status ( STR_STOPPING.m_pStr );

     while (  GUI_ReadButtons ()  ); break;

    }  /* end if */

   }  /* end while */

   lpFileCtx -> Destroy ( lpFileCtx );
   fileXioClose ( lFD );
   free ( lpBuff );

   if ( lfError ) goto error;

  } else {

   lpFileCtx -> Destroy ( lpFileCtx );
   goto error;

  }  /* end else */

 } else error: GUI_Error ( STR_ERROR.m_pStr );

 if (  CDDA_DiskType () != DiskType_None  ) CDVD_Stop ();

 GUI_UpdateStatus ();

}  /* end _copy2hdd_handler */

void GUI_FileCtxMenu ( char* apFileName, char* apPathEnd ) {

 GUIMenu*      lpMenu;
 GUIMenuState* lpState;
 int           lWidth;
 int           lHeight;

 s_pFileName = apFileName;
 s_pPathEnd  = apPathEnd;

 lpMenu  = ( GUIMenu* )GUI_CreateMenu ();
 lWidth  = g_GSCtx.m_Width  / 1.7F;
 lHeight = g_GSCtx.m_Height / 1.7F;

 CtxMenu_HandleEventBase = lpMenu -> HandleEvent;

 lpMenu -> m_Color      = 0x80301010UL;
 lpMenu -> m_X          = g_GSCtx.m_Width - lWidth - 32;
 lpMenu -> m_Y          = (  ( g_GSCtx.m_Height - lHeight ) >> 1  ) + 8;
 lpMenu -> m_Width      = lWidth;
 lpMenu -> m_Height     = lHeight;
 lpMenu -> m_pActiveObj = g_pActiveNode;
 lpMenu -> m_pState     = SMS_ListInit ();
 lpMenu -> Redraw       = GUIMenuSMS_Redraw;
 lpMenu -> HandleEvent  = CtxMenu_HandleEvent;

 lpState = GUI_MenuPushState ( lpMenu );

 lpState -> m_pTitle = &STR_SELECT_ACTION;
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = s_FileCtxMenu;
 lpState -> m_pLast  = &s_FileCtxMenu[ sizeof ( s_FileCtxMenu ) / sizeof ( s_FileCtxMenu[ 0 ] ) - 1 ];

 GUI_AddObject (  STR_SELECT_ACTION.m_pStr, ( GUIObject* )lpMenu  );
 lpMenu -> Redraw ( lpMenu );
 GUI_Run ();
 GUI_DeleteObject ( STR_SELECT_ACTION.m_pStr );
 GUI_Redraw ( GUIRedrawMethod_Redraw );
 GUI_UpdateStatus ();

}  /* end GUI_FileCtxMenu */
