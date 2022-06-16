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
#include "SMS_GUI.h"
#include "SMS_GS.h"
#include "SMS_Config.h"
#include "SMS_List.h"
#include "SMS_PAD.h"
#include "SMS_DMA.h"
#include "SMS_FileDir.h"
#include "SMS_GUIcons.h"
#include "SMS_VIF.h"
#include "SMS_Locale.h"
#include "SMS_CDVD.h"
#include "SMS_FileContext.h"
#include "SMS_SubtitleContext.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_RC.h"
#include "SMS_PgInd.h"
#include "SMS_IOP.h"
#include "SMS_ioctl.h"

#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include <fileio.h>
#include <fcntl.h>

static unsigned char s_pPPHDL[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "PP.HDL.";
static unsigned char s_pHDLdr[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "HDLoade";

extern char g_SMSLng[ 12 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char g_SMSPal[ 13 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );
extern char g_SMSSMB[ 17 ] __attribute__(   (  aligned( 1 ), section( ".data" )  )   );

extern FileContext* GUI_MiniBrowser ( FileContext*, char*, void** );
extern void         GUI_FileCtxMenu ( char*, char*, int, int      );

typedef struct _LevelInfo {

 char* m_pCurrent;
 int   m_YOffset;
 int   m_Pos;

} _LevelInfo;

typedef struct GUIFileMenu {

 DECLARE_GUI_OBJECT()

 unsigned int*  m_pColor;
 SMS_List*      m_pFileList;
 SMS_List*      m_pFiltList;
 SMS_List*      m_pLevels;
 SMS_ListNode*  m_pCurrent;
 SMS_ListNode*  m_pFirst;
 SMS_ListNode*  m_pLast;
 int            m_YOffset;
 unsigned long* m_pSelRect;
 int            m_Active;
 int            m_ActiveY;

} GUIFileMenu;

typedef struct _FileMenuItem {

 unsigned long  m_IconPack[ 32 ];
 unsigned long* m_pTxtPack;

} _FileMenuItem;

static unsigned long s_BitBltPack[ sizeof ( GSLoadImage ) * 9 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );

static int _filter_item ( SMS_ListNode* apNode ) {

 int lType = ( int )apNode -> m_Param;

 if (  lType == GUICON_PARTITION && (
        (  !( g_Config.m_BrowserFlags & SMS_BF_HDLP ) &&
            (  !strncmp (  _STR( apNode ), s_pPPHDL, 7  ) ||
               !strncmp (  _STR( apNode ), s_pHDLdr, 7  )
            )
        ) ||
        (  g_Config.m_BrowserFlags & SMS_BF_SYSP &&
           _STR( apNode )[ 0 ] == '_' &&
           _STR( apNode )[ 1 ] == '_'
        )
       )
 ) return 1;

 if ( lType == GUICON_PARTITION || lType == GUICON_FOLDER || lType == GUICON_SHARE ) return 0;

 if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) &&
       !(  lType == GUICON_AVI     ||
           lType == GUICON_MP3     ||
           lType == GUICON_M3U     ||
           lType == GUICON_AVIS    ||
           lType == GUICON_PICTURE
        )
 ) return 1;

 return 0;

}  /* end _filter_item */

static _FileMenuItem* GUIFileMenu_RenderItem ( SMS_ListNode* apNode, int anY, int aWidth, int afDim ) {

 int            lLen;
 _FileMenuItem* retVal;

 lLen = strlen (  _STR( apNode )  );

 if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) &&
       (  ( int )apNode -> m_Param == GUICON_AVI ||
          ( int )apNode -> m_Param == GUICON_MP3 ||
          ( int )apNode -> m_Param == GUICON_M3U ||
          ( int )apNode -> m_Param == GUICON_AVIS
       )
 ) {

  if (  _STR( apNode )[ lLen - 4 ] == '.'  )
   lLen -= 4;
  else lLen -= 5;

 }  /* end if */

 while (   GSFont_WidthEx (  _STR( apNode ), lLen, -2  ) > aWidth   ) --lLen;

 retVal = ( _FileMenuItem* )SMS_SyncMalloc (  sizeof ( _FileMenuItem )  );

 GUI_DrawIcon (
  ( int )apNode -> m_Param + afDim, 8, anY, GUIcon_Browser,
  UNCACHED_SEG( retVal -> m_IconPack )
 );
 retVal -> m_pTxtPack = GSContext_NewList (  GS_TXT_PACKET_SIZE( lLen )  );
 GSFont_RenderEx (  _STR( apNode ), lLen, 46, anY, retVal -> m_pTxtPack, -2, 0  );

 return retVal;

}  /* end GUIFileMenu_RenderItem */

static void GUIFileMenu_Render ( GUIObject* apObj, int aCtx ) {

 GUIFileMenu*   lpMenu = ( GUIFileMenu* )apObj;
 SMS_ListNode*  lpNode;
 _FileMenuItem* lpPack;

 if ( !lpMenu -> m_pGSPacket ) {

  unsigned long* lpDMA   = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );
  int            lHeight = g_GSCtx.m_Height - 101;
  int            lWidth  = g_GSCtx.m_Width  - 48;
  int            lnItems = ( lHeight - 2 ) / 34;
  int            lY      = 62;
  int            lSelY   = lY + 34 * lpMenu -> m_YOffset;

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpDMA - 2 ), 0, 60, g_GSCtx.m_Width - 1, lHeight, -12,
   g_Palette[ *lpMenu -> m_pColor - 1 ]
  );
  lpMenu -> m_pGSPacket = lpDMA;

  if (  ( lpNode = lpMenu -> m_pFirst )  ) {

   if ( lpNode -> m_pPrev ) {

    g_GSCtx.m_TextColor = 2;
    lpPack = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, 1 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, _STR( lpNode )
    ) -> m_Param = ( unsigned int )lpPack;

    lpMenu -> m_pLast = lpNode;
    lY += 34; --lnItems;
    lpNode = lpNode -> m_pNext;

   }  /* end if */

   g_GSCtx.m_TextColor = 1;

   while ( lnItems > 1 && lpNode ) {

    lpPack = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, 0 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, _STR( lpNode )
    ) -> m_Param = ( unsigned int )lpPack;

    lpMenu -> m_pLast = lpNode;
    lY += 34; --lnItems;
    lpNode = lpNode -> m_pNext;

   }  /* end while */

   if ( lpNode ) {

    int lClr = lpNode -> m_pNext ? 2 : 1;

    g_GSCtx.m_TextColor = lClr;

    lpPack = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, lClr - 1 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, _STR( lpNode )
    ) -> m_Param = ( unsigned int )lpPack;
    lpMenu -> m_pLast = lpNode;

   }  /* end if */

  }  /* end if */

  if ( lpMenu -> m_pCurrent ) {

   unsigned long lColor = g_Palette[ g_Config.m_BrowserSCIdx - 1 ];
   unsigned int  lW     = lWidth + 40;

   if ( !lpMenu -> m_pSelRect ) lpMenu -> m_pSelRect = GSContext_NewList (  GS_RRT_PACKET_SIZE() << 1  );

   GS_RenderRoundRect (
    ( GSRoundRectPacket* )(  lpMenu -> m_pSelRect + GS_RRT_PACKET_SIZE() - 2  ),
    4, lSelY, lW, 34, 12, ( lColor & 0x00FFFFFF ) | 0x10000000
   );
   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( lpMenu -> m_pSelRect - 2 ),
    4, lSelY, lW, 34, -12, lColor
   );
   lpMenu -> m_pSelRect[ -1 ] = VIF_DIRECT(  GS_RRT_PACKET_SIZE()  );
   lpMenu -> m_ActiveY        = lSelY;

  }  /* end if */

  SMS_InitBitBlt ( s_BitBltPack, 9, 58, g_GSCtx.m_Height - 58 - 36 );

 }  /* end if */

 GSContext_CallList ( aCtx, lpMenu -> m_pGSPacket );

 lpNode = lpMenu -> m_pFileList -> m_pHead;

 while ( lpNode ) {

  lpPack = ( _FileMenuItem* )( unsigned int )lpNode -> m_Param;

  GSContext_CallList2 ( aCtx, lpPack -> m_IconPack );
  GSContext_CallList  ( aCtx, lpPack -> m_pTxtPack );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 if ( lpMenu -> m_pSelRect && lpMenu -> m_pFileList -> m_Size ) GSContext_CallList ( aCtx, lpMenu -> m_pSelRect );

}  /* end GUIFileMenu_Render */

static void GUIFileMenu_Cleanup ( GUIObject* apObj ) {

 GUIFileMenu*   lpMenu = ( GUIFileMenu* )apObj;
 SMS_ListNode*  lpNode = lpMenu -> m_pFileList -> m_pHead;
 _FileMenuItem* lpPack;

 GUIObject_Cleanup ( apObj );

 while ( lpNode ) {

  lpPack = ( _FileMenuItem* )( unsigned int )lpNode -> m_Param;

  GSContext_DeleteList ( lpPack -> m_pTxtPack );
  free ( lpPack );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 SMS_ListDestroy ( lpMenu -> m_pFileList, 0 );

}  /* end GUIFileMenu_Cleanup */

static void _redraw ( GUIFileMenu* apMenu, int afAll ) {

 GUIFileMenu_Cleanup (  ( GUIObject* )apMenu  );

 if ( !afAll ) {

  GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
  GSContext_CallList2 (  1, ( unsigned long* )&s_BitBltPack  );
  GUIFileMenu_Render (  ( GUIObject* )apMenu, 1  );
  GS_VSync2 ( g_GSCtx.m_DrawDelay );
  GSContext_Flush ( 1, GSFlushMethod_KeepLists );

 } else GUI_Redraw ( GUIRedrawMethod_Redraw );

}  /* end _redraw */

static void GUIFileMenu_SetFocus ( GUIObject* apObj, int afSet ) {

 GUIFileMenu* lpMenu = ( GUIFileMenu* )apObj;

 lpMenu -> m_Active = afSet;
 lpMenu -> m_pColor = &( afSet ? g_Config.m_BrowserABCIdx : g_Config.m_BrowserIBCIdx );

 _redraw ( lpMenu, 0 );

}  /* end GUIFileMenu_SetFocus */

static void _reset ( GUIFileMenu* apMenu, int afAll ) {

 apMenu -> m_pFirst   = NULL;
 apMenu -> m_pLast    = NULL;
 apMenu -> m_pCurrent = NULL;
 apMenu -> m_YOffset  = 0;

 _redraw ( apMenu, afAll );

}  /* end _reset */

static void _push_state ( GUIFileMenu* apMenu ) {

 _LevelInfo* lpInfo = ( _LevelInfo* )malloc (  sizeof ( _LevelInfo )  );

 lpInfo -> m_pCurrent = ( char* )malloc (   strlen (  _STR( apMenu -> m_pCurrent )  ) + 1   );
 lpInfo -> m_YOffset  = apMenu -> m_YOffset;
 lpInfo -> m_Pos      = strlen ( g_CWD );

 strcpy (  lpInfo -> m_pCurrent, _STR( apMenu -> m_pCurrent )  );

 SMS_ListPushBack (
  apMenu -> m_pLevels, _STR( apMenu -> m_pFirst )
 ) -> m_Param = ( unsigned int )lpInfo;

}  /* end _push_state */

static void _pop_state ( GUIFileMenu* apMenu ) {

 _LevelInfo* lpInfo = ( _LevelInfo* )( unsigned int )apMenu -> m_pLevels -> m_pTail -> m_Param;

 free ( lpInfo -> m_pCurrent );
 free ( lpInfo );

 SMS_ListPopBack ( apMenu -> m_pLevels );

}  /* end _pop_state */

static void _fill ( GUIFileMenu* apMenu, char* apPath, int afAll ) {

 SMS_ListNode* lpNode;

 _reset ( apMenu, afAll );

 SMS_PgIndStart ();
 SMS_FileDirInit ( apPath );

 lpNode = g_pFileList -> m_pHead;
 SMS_ListDestroy ( apMenu -> m_pFiltList, 0 );

 while ( lpNode ) {

  if (  !_filter_item ( lpNode )  ) SMS_ListPushBack (
   apMenu -> m_pFiltList, _STR( lpNode )
  ) -> m_Param = lpNode -> m_Param;

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 apMenu -> m_pFirst   =
 apMenu -> m_pCurrent = apMenu -> m_pFiltList -> m_pHead;

 SMS_PgIndStop ();

}  /* end _fill */

void RestoreFileDir ( void** apParam ) {
 
 apParam[ 0 ] = ( void* )(  ( unsigned int )apParam[ 0 ] & 0x3FFFFFFF  );

 SMS_ListDestroy (  ( SMS_List* )apParam[ 0 ], 1  );
 g_pFileList = ( SMS_List* )apParam[ 1 ];
 strcpy (  g_CWD, ( char* )( apParam + 2 )  );
 free ( apParam );

 GUI_UpdateStatus ();

}  /* end RestoreFileDir */

static void _make_path ( char* apBuff, char** appPathEnd, const char* apName ) {

 strcpy ( apBuff, g_CWD );

 if (  apBuff[ strlen ( apBuff ) - 1 ] != '/'  ) strcat ( apBuff, g_SlashStr );

 *appPathEnd = apBuff + strlen ( apBuff );
 strcat ( apBuff, apName );

}  /* end _make_path */

void** SMS_OpenMediaFile ( const char* apName, int aFlags ) {

 char         lPath[ 1024 ];
 char*        lpPathEnd;
 FileContext* lpFileCtx;
 void**       retVal = NULL;

 SMS_PgIndStart ();

 _make_path ( lPath, &lpPathEnd, apName );

 if ( g_CMedia == 1 && g_pCDDACtx )

  lpFileCtx = CDDA_InitFileContext (  &lPath[ 7 ], g_pCDDACtx );

 else lpFileCtx = STIO_InitFileContext ( lPath, NULL );

 if ( lpFileCtx ) {

  unsigned long lEvent = GUI_MSG_FILE;

  retVal = malloc (   (  sizeof ( void* ) << 1  ) + sizeof ( void* )  );

  retVal[ 0 ] = lpFileCtx;
  retVal[ 1 ] = NULL;

  if ( aFlags & SMS_FA_FLAGS_SUB ) {
   SMS_PgIndStop  ();
    retVal[ 1 ] = GUI_MiniBrowser ( lpFileCtx, lPath, &retVal[ 2 ] );
   SMS_PgIndStart ();
  }  /* end if */

  if (  ( aFlags & SMS_FA_FLAGS_AVI ) && !retVal[ 1 ] && !( aFlags & SMS_FA_FLAGS_SUB ) && ( g_Config.m_PlayerFlags & SMS_PF_SUBS )  ) {

   int            i;
   char*          lpDot;
   char           lName[ 1024 ];
   char*          lExt[ 3 ] = { g_pSrtStr,          g_pSubStr,          g_pTxtStr          };
   SubtitleFormat lFmt[ 3 ] = { SubtitleFormat_SRT, SubtitleFormat_SUB, SubtitleFormat_SUB };

   strcpy ( lName, apName );
   lpDot = strrchr ( lName, '.' );

   if ( !lpDot ) {

    lpDot  = lName + strlen ( lName );
    *lpDot = '.';

   }  /* end if */

   ++lpDot;

   for (  i = 0; i < sizeof ( lFmt ) / sizeof ( lFmt[ 0 ] ); ++i  ) {

    SMS_ListNode* lpNode;

    strcpy ( lpDot, lExt[ i ] );

    lpNode = SMS_ListFindI ( g_pFileList, lName );

    if ( lpNode ) {

     FileContext* lpSubFileCtx;
     char*        lpName = g_CMedia == 1 && g_pCDDACtx ? &lPath[ 7 ] : lPath;

     strcpy (  lpPathEnd, _STR( lpNode )  );
     lpSubFileCtx = lpFileCtx -> Open ( lpName, lpFileCtx -> m_pOpenParam );

     if ( lpSubFileCtx ) {

      retVal[ 1 ] = ( void* )lpSubFileCtx;
      retVal[ 2 ] = ( void* )lFmt[ i ];

      break;

     }  /* end if */

    }  /* end if */

   }  /* end for */

  }  /* end if */

  if ( aFlags & SMS_FA_FLAGS_MSG ) {

   lEvent |= (  ( unsigned long )( unsigned int )retVal  ) << 28;
   GUI_PostMessage ( lEvent );

  }  /* end if */

 }  /* end if */

 SMS_PgIndStop ();

 return retVal;

}  /* end SMS_OpenMediaFile */

static void _perform_file_action ( GUIFileMenu* apMenu, int aFlags ) {

 SMS_OpenMediaFile (  _STR( apMenu -> m_pCurrent ), aFlags | SMS_FA_FLAGS_MSG  );

}  /* end _perform_file_action */

static void _action_avi ( GUIFileMenu* apMenu, int afPopup ) {

 int lFlags = SMS_FA_FLAGS_AVI;

 if ( afPopup ) lFlags |= SMS_FA_FLAGS_SUB;

 _perform_file_action ( apMenu, lFlags );

}  /* end _action_avi */

static void _action_file ( GUIFileMenu* apMenu, int afPopup ) {

 _perform_file_action ( apMenu, 0 );

}  /* end _action_file */

static void _action_partition ( GUIFileMenu* apMenu, int afPopup ) {

 int  lPD;
 char lPath[ 2 ] __attribute__(   (  aligned( 4 )  )   );
 char lMountPath[ strlen (  _STR( apMenu -> m_pCurrent )  ) + 6 ];

 strcpy (  lMountPath, g_pDevName[ 2 ]               );
 strcat (  lMountPath, g_ColonStr                    );
 strcat (  lMountPath, _STR( apMenu -> m_pCurrent )  );
 strcpy (  g_CWD,      g_pPFS                        );

 if ( g_PD >= 0 ) g_PD = 0 - !SMS_IOCtl ( g_CWD, PFS_IOCTL_UMOUNT, NULL );

 lPD = SMS_HDDMount ( g_CWD, lMountPath, FIO_MT_RDWR );

 if ( lPD >= 0 ) {

  strcpy (  g_Config.m_Partition, _STR( apMenu -> m_pCurrent )  );

  *( unsigned short* )&lPath[ 0 ] = 0x002F;
  _push_state ( apMenu );
  _fill ( apMenu, lPath, 0 );
  _redraw ( apMenu, 0 );

  g_PD = lPD;

 }  /* end if */

}  /* end _action_partition */

static void _context_action_part ( GUIFileMenu* apMenu, int afPopup ) {

}  /* end _context_action_part */

static void _context_action_file ( GUIFileMenu* apMenu, int afPopup ) {
#ifdef EMBEDDED
 static char sl_SMSsms[] __attribute__(   (  section( ".data" ), aligned( 1 )  )   ) = "SMS.sms";
#endif  /* EMBEDDED */
 int lAction = -1;
 int lfPart  = g_Config.m_Partition[ 0 ] != '\x00';

 if ( afPopup == 3 ) {
  lAction =  9;
 } else if ( afPopup == 4 ) {
  lAction = 10;
 } else if ( afPopup == 2 ) {
  if ( g_CMedia == 2 )  /* HDD */
   lAction = 3;         /* delete tree */
  else if ( g_CMedia == 3 || g_CMedia == 4 ) return;  /* CDDA || HOST */
  else if ( lfPart ) lAction = 1;  /* copy tree to HDD */
 } else if ( afPopup == 1 && g_CMedia != 2 && lfPart ) {
  lAction = 0;  /* copy file to HDD */
 } else if ( g_CMedia == 2 && lfPart ) {  /* HDD */
  lAction = 2;                            /* delete file */
 } else lAction = 100;

 if ( lAction >= 0 ) {

  char  lPath[ 1024 ];
  char* lpPathEnd;
  char* lpCur      = _STR( apMenu -> m_pCurrent );
  char  lRoot[ 2 ] = { '\x01', '\x00' };

  _make_path ( lPath, &lpPathEnd, lpCur );

  if ( lAction == 10 ) lpCur = lRoot;

  if ( lAction == 0 || lAction == 100 ) {
   if (  !strcmp ( lpCur, &g_SMSLng[ 4 ] )  )
    lAction = 4;  /* update language file */
   else if (  !strcmp ( lpCur, &g_SMSPal[ 5 ] )  )
    lAction = 5;  /* update palette file */
#ifdef EMBEDDED
   else if (  !strcmp ( lpCur, sl_SMSsms )  )
    lAction = 6;  /* update SMS image */
#endif  /* EMBEDDED */
   else if (  !strcmp ( lpCur, &g_SMSSMB[ 9 ] )  )
    lAction = 7;  /* update SMB server list */
   else {
    int lLen = strlen ( lpCur );
    if ( lLen > 4 && lpCur[ lLen - 4 ] == '.' &&
                     lpCur[ lLen - 3 ] == 's' &&
                     lpCur[ lLen - 2 ] == 'm' &&
                     lpCur[ lLen - 1 ] == 'i'
    ) lAction = 8;  /* add background image */
   }  /* end else */
  }  /* end if */

  GUI_FileCtxMenu ( lAction == 9 || lAction == 10 ? lpCur : lPath, lpPathEnd, lAction, apMenu -> m_ActiveY );

 }  /* end if */

}  /* end _context_action_file */

static void _action_folder ( GUIFileMenu* apMenu, int afPopup ) {

 if ( !afPopup ) {

  _push_state ( apMenu );
  _fill (  apMenu, _STR( apMenu -> m_pCurrent ), 0  );
  _redraw ( apMenu, 0 );

 } else _context_action_file ( apMenu, 3 );

}  /* end _action_folder */

static void _context_action_fold ( GUIFileMenu* apMenu, int afPopup ) {

 _context_action_file ( apMenu, 2 );

}  /* end _context_action_fold */

static void _action_share ( GUIFileMenu* apMenu, int afPopup ) {

 int          lSD;
 SMBMountInfo lMountInfo;
 char         lPath[ 2 ] __attribute__(   (  aligned( 4 )  )   );
 char*        lpPath = _STR( apMenu -> m_pCurrent );

 lSD = fioDopen ( g_pSMBS );

 if ( lSD >= 0 ) {

  if ( g_SMBU >= 0 ) {

   fioIoctl ( lSD, SMB_IOCTL_UMOUNT, &g_SMBU );
   g_SMBU = 0x80000000;

  }  /* end if */

  lMountInfo.m_Unit = g_SMBUnit;
  strcpy ( lMountInfo.m_Path, lpPath );
  *strchr ( lMountInfo.m_Path, ':' ) = '\x00';

  g_SMBU = fioIoctl ( lSD, SMB_IOCTL_MOUNT, &lMountInfo );
  fioDclose ( lSD );

  if ( g_SMBU > 0 ) {

   *( unsigned int* )g_CWD       = *( int* )g_pSMB;
   g_CWD[ 3 ]                    = '0' + g_SMBU;
   *( unsigned int* )&g_CWD[ 4 ] = 0x0000003A;
   *( unsigned short* )lPath     = 0x002F;

   _push_state ( apMenu );
   _fill ( apMenu, lPath, 0 );
   _redraw ( apMenu, 0 );

  }  /* end if */

 }  /* end if */

}  /* end _action_share */

static void _context_action_share ( GUIFileMenu* apMenu, int afPopup ) {

}  /* end _context_action_share */

static void ( *Action[ 9 ] ) ( GUIFileMenu*, int ) = {
 _action_folder, _action_avi,   _action_file,
 _action_file,   _action_file,  _action_partition,
 _action_avi,    _action_share, _action_file
};

static void ( *ContextAction[ 9 ] ) ( GUIFileMenu*, int ) = {
 _context_action_fold, _context_action_file, _context_action_file,
 _context_action_file, _context_action_file, _context_action_part,
 _context_action_file, _context_action_share, _context_action_file
};

static int _page_scroll (  int ( * ) ( GUIFileMenu*, int ), GUIFileMenu*  );
static int _step_up     ( GUIFileMenu*, int                               );

static int _step_down ( GUIFileMenu* apMenu, int afRoll ) {

 int retVal = 0;

 if ( apMenu -> m_pCurrent ) {

  if ( apMenu -> m_pCurrent -> m_pNext ) {

   apMenu -> m_pCurrent = apMenu -> m_pCurrent -> m_pNext;

   if ( apMenu -> m_pCurrent == apMenu -> m_pLast && apMenu -> m_pCurrent -> m_pNext )
    apMenu -> m_pFirst = apMenu -> m_pFirst -> m_pNext;
   else ++apMenu -> m_YOffset;

   retVal = 1;

  } else if ( afRoll ) while (  _page_scroll ( _step_up, apMenu )  );

 }  /* end if */

 return retVal;

}  /* end _step_down */

static int _step_up ( GUIFileMenu* apMenu, int afRoll ) {

 int retVal = 0;

 if ( apMenu -> m_pCurrent ) {

  if ( apMenu -> m_pCurrent -> m_pPrev ) {

   apMenu -> m_pCurrent = apMenu -> m_pCurrent -> m_pPrev;

   if ( apMenu -> m_pCurrent == apMenu -> m_pFirst && apMenu -> m_pCurrent -> m_pPrev )
    apMenu -> m_pFirst = apMenu -> m_pFirst -> m_pPrev;
   else --apMenu -> m_YOffset;

   retVal = 1;

  } else if ( afRoll ) while (  _page_scroll ( _step_down, apMenu )  );

 }  /* end if */

 return retVal;

}  /* end _step_up */

static int _page_scroll (  int ( *aFunc ) ( GUIFileMenu*, int ), GUIFileMenu* apMenu  ) {

 int           i, j;
 int           lHeight = g_GSCtx.m_Height - 101;
 int           lnItems = ( lHeight - 2 ) / 34;
 int           retVal  = 1;
 SMS_ListNode* lpNode;

 for ( i = 0; i < 10; ++i ) {

  j = lnItems;

  if (  !aFunc ( apMenu, 0 )  ) {

   retVal = 0;
   break;

  }  /* end if */

  lpNode = apMenu -> m_pFirst;

  while ( lpNode && j > 1 ) {

   apMenu -> m_pLast = lpNode;
   --j;
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  if ( lpNode ) apMenu -> m_pLast = lpNode;

 }  /* end for */

 return retVal;

}  /* end _page_scroll */

static int GUIFileMenu_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 int          retVal   = GUIHResult_Void;
 GUIFileMenu* lpMenu   = ( GUIFileMenu* )apObj;
 int          lObjType;

 if ( lpMenu -> m_pCurrent )
  lObjType = ( unsigned int )lpMenu -> m_pCurrent -> m_Param >> 1;
 else lObjType = 0x80000000;

 if ( anEvent & GUI_MSG_PAD_MASK ) {

  int lBtn = anEvent & GUI_MSG_PAD_MASK;

  if ( lBtn == RC_TOP_MENU || lBtn == SMS_PAD_LEFT || lBtn == SMS_PAD_RIGHT ) {

   retVal = GUIHResult_ChangeFocus;

  } else if ( lBtn == RC_PREV || lBtn == SMS_PAD_L1 ) {

   _page_scroll (  _step_up, lpMenu );
   goto redraw;

  } else if ( lBtn == RC_NEXT || lBtn == SMS_PAD_L2 ) {

   _page_scroll (  _step_down, lpMenu );
   goto redraw;

  } else if ( lBtn == SMS_PAD_DOWN ) {

   _step_down ( lpMenu, 1 );
   goto redraw;

  } else if ( lBtn == SMS_PAD_UP ) {

   _step_up ( lpMenu, 1 );
redraw:
   _redraw ( lpMenu, 0 );

   retVal = GUIHResult_Handled;

  } else if (  lBtn == RC_ENTER || ( lObjType > 0 && lBtn == RC_PLAY ) || lBtn == SMS_PAD_CROSS  ) {

   if ( lObjType >= 0 ) {
    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );
    Action[ lObjType ] ( lpMenu, 0 );
   }  /* end if */

   retVal = GUIHResult_Handled;

  } else if (  lBtn == RC_SUBTITLE || lBtn == SMS_PAD_CIRCLE || ( lObjType == 0 && lBtn == RC_PLAY )  ) {

   if ( lObjType >= 0 ) {
    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );
    Action[ lObjType ] ( lpMenu, 1 );
   }  /* end if */

   retVal = GUIHResult_Handled;

  } else if (  lBtn == RC_DISPLAY || lBtn == ( SMS_PAD_R1 | SMS_PAD_CIRCLE )  ) {

   if (  ( g_CMedia == 2 && g_PD     >= 0 && g_CWD[ 0 ] != 'h' ) ||
         ( g_CMedia == 6 && g_SMBU   >  0                      ) ||
         ( g_CMedia != 2 && g_CMedia != 6                      )
   ) _context_action_file ( lpMenu, 4 );

   retVal = GUIHResult_Handled;

  } else if ( lBtn == RC_PROGRAM || lBtn == SMS_PAD_SQUARE ) {

   if ( lObjType >= 0 ) ContextAction[ lObjType ] ( lpMenu, 1 );

   retVal = GUIHResult_Handled;

  } else if ( lBtn == RC_STOP || lBtn == RC_RETURN || lBtn == SMS_PAD_TRIANGLE ) {

   int lLevel = lpMenu -> m_pLevels -> m_Size;

   if ( lLevel ) {

    SMS_ListNode* lpState = lpMenu -> m_pLevels -> m_pTail;
    SMS_ListNode* lpNode;
    _LevelInfo*   lpInfo  = ( _LevelInfo* )( unsigned int )lpState -> m_Param;
    char          lCWD[ sizeof ( g_CWD ) ] __attribute__(   (  aligned( 4 )  )   );
    char*         lpPtr;

    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );

    if ( lLevel > 1 ) {

     _LevelInfo* lpInfoPrev  = ( _LevelInfo* )( unsigned int )lpState -> m_pPrev -> m_Param;
     int         lLen        = lpInfo -> m_Pos - lpInfoPrev -> m_Pos;

     strncpy ( lCWD, &g_CWD[ lpInfoPrev -> m_Pos ], lLen );
     lCWD [ lLen                ] = '\x00';
     g_CWD[ lpInfoPrev -> m_Pos ] = '\x00';
     lpPtr = lCWD;

    } else lpPtr = g_EmptyStr;

    _fill ( lpMenu, lpPtr, 0 );

    lpNode = SMS_ListFind (  lpMenu -> m_pFiltList, _STR( lpState )  );

    if ( lpNode ) {

     lpMenu -> m_pFirst   = lpNode;
     lpMenu -> m_pCurrent = SMS_ListFind ( lpMenu -> m_pFiltList, lpInfo -> m_pCurrent );
     lpMenu -> m_YOffset  = lpInfo -> m_YOffset;

    } else {

     lpMenu -> m_pFirst   =
     lpMenu -> m_pCurrent = lpMenu -> m_pFiltList -> m_pHead;
     lpMenu -> m_YOffset  = 0;

    }  /* end else */

    _pop_state ( lpMenu );
    _redraw ( lpMenu, 0 );

    if ( g_CMedia == 2 && !lpMenu -> m_pLevels -> m_Size ) g_Config.m_Partition[ 0 ] = '\x00';

   }  /* end if */

  }  /* end if */

 } else switch ( anEvent & GUI_MSG_USER_MASK ) {

  case GUI_MSG_MEDIA_SELECTED: {

   static int s_fHDDInit = 0;

   while ( lpMenu -> m_pLevels -> m_Size ) _pop_state ( lpMenu );

   if ( g_CMedia == 1 || g_CMedia == 5 ) CDVD_FlushCache ();

   _fill ( lpMenu, g_EmptyStr, anEvent & GUI_MSG_MENU_BIT );

   if ( g_CMedia == 2 && !s_fHDDInit && g_Config.m_Partition[ 0 ] ) {

    SMS_ListNode* lpMountNode = SMS_ListFind ( lpMenu -> m_pFiltList, g_Config.m_Partition );

    if ( lpMountNode ) {

     while ( lpMenu -> m_pCurrent != lpMountNode ) {

      lpMenu -> m_pCurrent = lpMenu -> m_pCurrent -> m_pNext;

      if ( lpMenu -> m_pCurrent == lpMenu -> m_pLast && lpMenu -> m_pCurrent -> m_pNext )
       lpMenu -> m_pFirst = lpMenu -> m_pFirst -> m_pNext;
      else ++lpMenu -> m_YOffset;

     }  /* end while */

     s_fHDDInit = 1;
     _action_partition ( lpMenu, 0 );

     return GUIHResult_Handled;

    }  /* end if */

   }  /* end if */

   _redraw ( lpMenu, anEvent & GUI_MSG_MENU_BIT );

   retVal = GUIHResult_Handled;

   if ( !lpMenu -> m_Active ) GUI_PostMessage ( SMS_PAD_DOWN );

  } break;

  case GUI_MSG_MEDIA_REMOVED: {

   _reset ( lpMenu, anEvent & GUI_MSG_MENU_BIT );

   retVal = GUIHResult_Handled;

  } break;

  case GUI_MSG_RELOAD_BROWSER:
   _fill ( lpMenu, g_DotStr, 0 );
  break;

  case GUI_MSG_REFILL_BROWSER: {

   if ( g_pFileList ) {

    SMS_ListNode* lpNode = g_pFileList -> m_pHead;
    SMS_ListDestroy ( lpMenu -> m_pFiltList, 0 );

    while ( lpNode ) {

     if (  !_filter_item ( lpNode )  ) SMS_ListPushBack (
      lpMenu -> m_pFiltList, _STR( lpNode )
     ) -> m_Param = lpNode -> m_Param;

     lpNode = lpNode -> m_pNext;

    }  /* end while */

    lpMenu -> m_pFirst   =
    lpMenu -> m_pCurrent = lpMenu -> m_pFiltList -> m_pHead;
    lpMenu -> m_YOffset  = 0;
    lpMenu -> m_pLast    = NULL;

   }  /* end if */

  } break;

 }  /* end switch */

 return retVal;

}  /* end GUIFileMenu_HandleEvent */

GUIObject* GUI_CreateFileMenu ( void ) {

 GUIFileMenu* retVal = ( GUIFileMenu* )calloc (  1, sizeof ( GUIFileMenu )  );

 retVal -> Render      = GUIFileMenu_Render;
 retVal -> HandleEvent = GUIFileMenu_HandleEvent;
 retVal -> SetFocus    = GUIFileMenu_SetFocus;
 retVal -> Cleanup     = GUIFileMenu_Cleanup;
 retVal -> m_pFileList = SMS_ListInit ();
 retVal -> m_pLevels   = SMS_ListInit ();
 retVal -> m_pFiltList = SMS_ListInit ();
 retVal -> m_pColor    = &g_Config.m_BrowserIBCIdx;
 retVal -> m_YOffset   = 0;

 g_PD   = 0x80000000;
 g_SMBU = 0x80000000;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateFileMenu */
