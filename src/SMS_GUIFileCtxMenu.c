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
#include "SMS_GUIMenu.h"
#include "SMS_FileContext.h"
#include "SMS_FileDir.h"
#include "SMS_Locale.h"
#include "SMS_Timer.h"
#include "SMS_GS.h"
#include "SMS.h"
#include "SMS_EE.h"
#include "SMS_PAD.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"
#include "SMS_Config.h"
#include "SMS_Sounds.h"
#include "SMS_GUIcons.h"
#include "SMS_GUIClock.h"

#include <tamtypes.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <sys/fcntl.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>

extern void GUIMenuSMS_Redraw           ( GUIMenu*                                                        );
extern int  CtxMenu_HandleEvent         ( GUIObject*, u64                                                 );
extern int ( *CtxMenu_HandleEventBase ) ( GUIObject*, u64                                                 );
extern void SMS_CopyTree                ( const char*                                                     );
extern void SMS_DeleteTree              ( const char*                                                     );
extern int  SMS_CopyFile                ( const char*, FileContext*, u64          , u64*          , void* );
extern void SMS_LoadPalette             ( void                                                            );
extern void SMS_LoadSMBInfo             ( void                                                            );
extern void RestoreFileDir              ( void**                                                          );

extern char g_SMSLng[ 12 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char g_SMSPal[ 13 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char g_SMSSMB[ 17 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );

static char* s_pFileName;
static char* s_pPathEnd;
static int   s_Action;

static void _copy2hdd_handler ( GUIMenu*, int );
static void _delete_handler   ( GUIMenu*, int );
static void _copy_lng_handler ( GUIMenu*, int );
static void _copy_pal_handler ( GUIMenu*, int );
#ifdef EMBEDDED
static void _copy_sms_handler ( GUIMenu*, int );
#endif  /* EMBEDDED */
static void _copy_smb_handler ( GUIMenu*, int );
static void _copy_bim_handler ( GUIMenu*, int );

extern void _audio_handler ( GUIMenu*, int );
extern void _video_handler ( GUIMenu*, int );
extern void _image_handler ( GUIMenu*, int );

static GUIMenuItem s_FileCtxMenu[] __attribute__(   (  section( ".data" )  )   ) = {
 { 0, &STR_COPY_TO_HDD,          0,              0, _copy2hdd_handler, 0, 0 },
 { 0, &STR_DELETE,               0,              0, _delete_handler,   0, 0 },
 { 0, NULL,                      0,              0, NULL,              0, 0 },
 { 0, &STR_ADD_BACKGROUND_IMAGE, 0,              0, _copy_bim_handler, 0, 0 },
 { 0, &STR_AUDIO,                GUICON_M3U,     0, _audio_handler,    0, 0 },
 { 0, &STR_VIDEO,                GUICON_AVI,     0, _video_handler,    0, 0 },
 { 0, &STR_IMAGES,               GUICON_PICTURE, 0, _image_handler,    0, 0 }
};

static void _copy2hdd_handler ( GUIMenu* apMenu, int aDir ) {

 switch ( s_Action ) {

  case 0: {

   FileContext* lpFileCtx;

   if ( g_CMedia == 1 && g_pCDDACtx )
    lpFileCtx = CDDA_InitFileContext ( &s_pFileName[ 7 ], g_pCDDACtx );
   else lpFileCtx = STIO_InitFileContext ( s_pFileName, 0 );

   if ( lpFileCtx ) {

    int   lLen;
    char* lpPath = ( char* )malloc (
     (  lLen = strlen ( g_HDDWD )  ) + strlen ( s_pPathEnd ) + 2
    );

    if ( lpPath ) {

     strcpy ( lpPath, g_HDDWD );
     if ( g_HDDWD[ lLen - 1 ] != '/' ) strcat ( lpPath, g_SlashStr );
     strcat ( lpPath, s_pPathEnd );

     SMS_GUIClockSuspend ();
      SMS_CopyFile ( lpPath, lpFileCtx, 0, NULL, NULL );
     SMS_GUIClockResume  ();

     free ( lpPath );

    } else GUI_Error ( STR_OUT_OF_MEMORY.m_pStr );

   }  /* end if */

   if (  CDDA_DiskType () != DiskType_None  ) CDVD_Stop ();

  } break;

  case 1:

   SMS_CopyTree ( s_pFileName );

  break;

 }  /* end switch */

}  /* end _copy2hdd_handler */

static void _delete_handler ( GUIMenu* apMenu, int aDir ) {

 switch ( s_Action ) {

  case 2:
   fioRemove ( s_pFileName );
  break;

  case 3:
   SMS_DeleteTree ( s_pFileName );
  break;

 }  /* end switch */

 g_pFileMenu -> HandleEvent ( g_pFileMenu, GUI_MSG_RELOAD_BROWSER );

}  /* end _delete_handler */

static int _copy_file_to_mc ( const char* apDst, const char* apSrc ) {

 int retVal = 0;
 int lFDSrc = fioOpen ( apSrc, O_RDONLY );

 if ( lFDSrc >= 0 ) {

  int lFDDst = fioOpen ( apDst, O_CREAT | O_WRONLY );

  if ( lFDDst >= 0 ) {

   s64   lSize = fioLseek ( lFDSrc, 0, SEEK_END );
   char* lpBuf = ( char* )malloc ( lSize );

   if ( lpBuf ) {

    fioLseek ( lFDSrc, 0, SEEK_SET );

    if (  fioRead  ( lFDSrc, lpBuf, lSize ) == lSize &&
          fioWrite ( lFDDst, lpBuf, lSize ) == lSize
    ) retVal = 1;

    free ( lpBuf );

   }  /* end if */

   fioClose ( lFDDst );

  }  /* end if */

  fioClose ( lFDSrc );

 }  /* end if */

 if ( !retVal ) {
  fioRemove ( apDst );
  fioRmdir  ( apDst );
 }  /* end if */

 return retVal;

}  /* end _copy_file_to_mc */

static int _do_sms_action ( const char* apDst ) {

 int  retVal = 0;
 char lBuf[ 32 ];

 lBuf[ 0 ] = 'm';
 lBuf[ 1 ] = 'c';
 lBuf[ 2 ] = g_pIPConf[ 2 ];
 lBuf[ 3 ] = ':';
 lBuf[ 4 ] = '/';
 strcpy ( &lBuf[ 5 ], apDst );

 GUI_Status (  STR_PROCESSING.m_pStr );

 if (  SMS_SaveConfig ()  ) retVal = _copy_file_to_mc ( lBuf, s_pFileName );

 if ( !retVal ) GUI_Error ( STR_ERROR.m_pStr );

 return retVal;

}  /* end _do_sms_action */

static void _copy_lng_handler ( GUIMenu* apMenu, int aDir ) {

 if (  _do_sms_action ( g_SMSLng )  ) {
  SMS_LocaleInit ();
  SMS_LocaleSet  ();
 }  /* end if */

}  /* end _copy_lng_handler */

static void _copy_pal_handler ( GUIMenu* apMenu, int aDir ) {

 if (  _do_sms_action ( &g_SMSPal[ 1 ] )  ) {
  SMS_LoadPalette ();
  GUI_SetColors   ();
  GUI_Redraw      ( GUIRedrawMethod_InitClearObj );
 }  /* end if */

}  /* end _copy_pal_handler */
#ifdef EMBEDDED
static void _copy_sms_handler ( GUIMenu* apMenu, int aDir ) {

 static char sl_Fmt[ 37 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   ) = "mc%d:/B%cEXEC-DVDPLAYER/dvdplayer.dmy";

 int  lFD;
 char lSrc[ 64 ];
 char lDst[ 64 ];

 sprintf ( lSrc, sl_Fmt, g_MCSlot, g_pBXDATASYS[ 6 ] );
 sl_Fmt[ 34 ] = 'b';
 sl_Fmt[ 35 ] = 'u';
 sl_Fmt[ 36 ] = 'p';
 sprintf ( lDst, sl_Fmt, g_MCSlot, g_pBXDATASYS[ 6 ] );
 sl_Fmt[ 34 ] = 'd';
 sl_Fmt[ 35 ] = 'm';
 sl_Fmt[ 36 ] = 'y';

 lFD = fioOpen ( lDst, O_RDONLY );

 if ( lFD < 0 ) {

  fioClose ( lFD );
  GUI_Status ( STR_CREATING_BACKUP.m_pStr );

  if (  !_copy_file_to_mc ( lDst, lSrc )  ) {
   GUI_Error ( STR_ERROR.m_pStr );
   return;
  }  /* end if */

  fioRemove ( lSrc );
  fioRmdir  ( lSrc );

 }  /* end if */

 _do_sms_action ( &lSrc[ 5 ] );

}  /* end _copy_sms_handler */
#endif  /* EMBEDDED */
static void _copy_smb_handler ( GUIMenu* apMenu, int aDir ) {

 if (  _do_sms_action ( &g_SMSSMB[ 5 ] )  ) SMS_LoadSMBInfo ();

}  /* end _copy_smb_handler */

static void _copy_bim_handler ( GUIMenu* apMenu, int aDir ) {

 int lRes = fioMkdir ( g_pSMSSkn );

 if ( lRes == 0 || lRes == -4 ) {

  char  lBuf[ 1024 ];
  char* lpPtr = strrchr ( s_pFileName, '/' );

  strcpy ( lBuf, &g_pSMSSkn[ 5 ] );
  strcat ( lBuf, g_SlashStr );
  strcat ( lBuf, lpPtr + 1  );

  if (  _do_sms_action ( lBuf )  ) SMS_EEScanDir ( g_pSMSSkn, g_pExtSMI, g_Config.m_pSkinList );

 } else GUI_Error ( STR_ERROR.m_pStr );

}  /* end _copy_bim_handler */

static int _is_sub ( const char* apName ) {

 int   retVal = 0;
 char* lpPtr  = strrchr ( apName, '.' );

 if (  lpPtr && strlen ( ++lpPtr ) == 3  ) retVal = !strcmp ( lpPtr, g_pSubStr ) ||
                                                    !strcmp ( lpPtr, g_pSrtStr ) ||
                                                    !strcmp ( lpPtr, g_pTxtStr );
 return retVal;

}  /* end _is_sub */

__attribute__(   ( used )  ) static void _av_handler ( GUIMenu* apMenu, int aDir, int aFileType0, int aFileType1 ) {

 void**        lpParam = ( void** )malloc (  strlen ( g_CWD ) + (  sizeof ( SMS_List* ) << 1  ) + 1   );
 int           lnMedia = 0;
 SMS_ListNode* lpNode;

 lpParam[ 0 ] = SMS_ListInit ();
 lpParam[ 1 ] = g_pFileList;
 strcpy (  ( char* )( lpParam + 2 ), g_CWD  );

 g_pFileList = NULL;
 SMS_FileDirInit ( s_pFileName );

 lpNode = g_pFileList -> m_pHead;

 while ( lpNode ) {

  int lParam = ( int )lpNode -> m_Param;
  int lfSub  = 0;

  if (     lParam == aFileType0 || lParam == aFileType1 || (    aFileType0 != GUICON_MP3 && aFileType0 != GUICON_PICTURE && (   lfSub = _is_sub (  _STR( lpNode )  )   )    )     ) {

   lnMedia += !lfSub;
   SMS_ListPushBack (  ( SMS_List* )lpParam[ 0 ], _STR( lpNode )  ) -> m_Param = lfSub;

  }  /* end if */

  lpNode = lpNode -> m_pNext;

 }  /* end lpNode */

 SMS_ListDestroy ( g_pFileList, 1 );
 g_pFileList = lpParam[ 0 ];

 if ( lnMedia ) {

  u64           lEvent = GUI_MSG_FOLDER_MP3;
  unsigned int  lMask  = ( aFileType0 == GUICON_MP3 || aFileType0 == GUICON_PICTURE ) ? 0x80000000 : 0xC0000000;

  SMS_ListSort (  ( SMS_List* )lpParam[ 0 ]  );

  lpParam[ 0 ] = ( void* )(  ( unsigned int )lpParam[ 0 ] | lMask  );
  lEvent      |= (  ( u64           )( unsigned int )lpParam  ) << 28;

  GUI_PostMessage ( GUI_MSG_QUIT );
  GUI_PostMessage ( lEvent );

 } else RestoreFileDir ( lpParam );

}  /* end _av_handler */

__asm__(
 ".set noreorder\n\t"
 "_audio_handler:\n\t"
 "addiu $a2, $zero, 4\n\t"
 "beq   $zero, $zero, _av_handler\n\t"
 "addiu  $a3, $zero, 4\n\t"
 "_video_handler:\n\t"
 "addiu $a2, $zero, 2\n\t"
 "beq   $zero, $zero, _av_handler\n\t"
 "addiu $a3, $zero, 12\n\t"
 "_image_handler:\n\t"
 "or    $a2, $zero, 16\n\t"
 "beq   $zero, $zero, _av_handler\n\t"
 "or    $a3, $zero, 16\n\t"
 ".set reorder\n\t"
);

void GUI_FileCtxMenu ( char* apFileName, char* apPathEnd, int anAction, int anY ) {

 GUIMenu*      lpMenu;
 GUIMenuState* lpState;
 GUIMenuItem*  lpItemFirst;
 GUIMenuItem*  lpItemLast;
 int           lWidth;
 int           lHeight;
 float         lW      = 1.8F;
 SMString*     lpTitle = &STR_SELECT_ACTION;

 switch ( anAction ) {

  case 0:
  case 1: lpItemFirst = lpItemLast = &s_FileCtxMenu[ 0 ]; break;
  case 2:
  case 3: lpItemFirst = lpItemLast = &s_FileCtxMenu[ 1 ]; break;
  case 4:
   lpItemFirst = lpItemLast = &s_FileCtxMenu[ 2 ];
   s_FileCtxMenu[ 2 ].m_pOptionName = &STR_UPDATE_LANGUAGE_FILE;
   s_FileCtxMenu[ 2 ].Handler       = _copy_lng_handler;
  break;
  case 5:
   lpItemFirst = lpItemLast = &s_FileCtxMenu[ 2 ];
   s_FileCtxMenu[ 2 ].m_pOptionName = &STR_UPDATE_PALETTE_FILE;
   s_FileCtxMenu[ 2 ].Handler       = _copy_pal_handler;
  break;
#ifdef EMBEDDED
  case 6:
   lpItemFirst = lpItemLast = &s_FileCtxMenu[ 2 ];
   s_FileCtxMenu[ 2 ].m_pOptionName = &STR_UPDATE_SMS;
   s_FileCtxMenu[ 2 ].Handler       = _copy_sms_handler;
  break;
#endif  /* EMBEDDED */
  case 7:
   lpItemFirst = lpItemLast = &s_FileCtxMenu[ 2 ];
   s_FileCtxMenu[ 2 ].m_pOptionName = &STR_UPDATE_SMB_FILE;
   s_FileCtxMenu[ 2 ].Handler       = _copy_smb_handler;
  break;
  case 8:
   lpItemFirst = lpItemLast = &s_FileCtxMenu[ 3 ];
  break;
  case  9:
  case 10:
   lpItemFirst = &s_FileCtxMenu[ 4 ];
   lpItemLast  = &s_FileCtxMenu[ 6 ];
   lpTitle     = &STR_PLAY_ALL;
   lW          = 2.8F;
  break;

  default: return;

 }  /* end switch */

 s_pFileName = apFileName;
 s_pPathEnd  = apPathEnd;
 s_Action    = anAction;

 lpMenu  = ( GUIMenu* )GUI_CreateMenu ();
 lWidth  = g_GSCtx.m_Width  / lW;
 lHeight = 148;

 CtxMenu_HandleEventBase = lpMenu -> HandleEvent;

 anY += 16;

 if ( anY + lHeight > g_GSCtx.m_Height - 48 ) anY = g_GSCtx.m_Height - lHeight - 54;

 lpMenu -> m_Color      = 0x80301010UL;
 lpMenu -> m_X          = g_GSCtx.m_Width - lWidth - 32;
 lpMenu -> m_Y          = anY;
 lpMenu -> m_Width      = lWidth;
 lpMenu -> m_Height     = lHeight;
 lpMenu -> m_pActiveObj = g_pActiveNode;
 lpMenu -> m_pState     = SMS_ListInit ();
 lpMenu -> m_IGroup     = GUIcon_Browser;
 lpMenu -> Redraw       = GUIMenuSMS_Redraw;
 lpMenu -> HandleEvent  = CtxMenu_HandleEvent;

 if ( anAction == 10 ) {

  lpMenu -> m_X = ( g_GSCtx.m_Width - lpMenu -> m_Width ) / 2;
  lpMenu -> m_Y = 36;

 }  /* end if */

 lpState = GUI_MenuPushState ( lpMenu );

 lpState -> m_pTitle = lpTitle;
 lpState -> m_pItems =
 lpState -> m_pFirst =
 lpState -> m_pCurr  = lpItemFirst;
 lpState -> m_pLast  = lpItemLast;

 GUI_AddObject (  STR_SELECT_ACTION.m_pStr, ( GUIObject* )lpMenu  );
 lpMenu -> Redraw ( lpMenu );
 GUI_Run ();
 GUI_DeleteObject ( STR_SELECT_ACTION.m_pStr );
 GUI_Redraw ( GUIRedrawMethod_Redraw );
 GUI_UpdateStatus ();

}  /* end GUI_FileCtxMenu */
