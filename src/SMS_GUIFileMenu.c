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

#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include <fileXio_rpc.h>
#include <fcntl.h>

#define FA_FLAGS_AVI 0x00000001
#define FA_FLAGS_SUB 0x00000002

static unsigned char s_pPPHDL[] __attribute__(   (  section( ".data" )  )   ) = "PP.HDL.";
static unsigned char s_pHDLdr[] __attribute__(   (  section( ".data" )  )   ) = "HDLoade";

extern FileContext* GUI_MiniBrowser ( FileContext*, char*, void** );

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
 int            m_PD;
 int            m_Active;

} GUIFileMenu;

static GSBitBltPacket s_BitBlt;

static int _filter_item ( SMS_ListNode* apNode ) {

 int lType = ( int )apNode -> m_Param;

 if (  lType == GUICON_PARTITION && (
        (  !( g_Config.m_BrowserFlags & SMS_BF_HDLP ) &&
            (  !strncmp ( apNode -> m_pString, s_pPPHDL, 7 ) ||
               !strncmp ( apNode -> m_pString, s_pHDLdr, 7 )
            )
        ) ||
        (  g_Config.m_BrowserFlags & SMS_BF_SYSP &&
           apNode -> m_pString[ 0 ] == '_' &&
           apNode -> m_pString[ 1 ] == '_'
        )
       )
 ) return 1;

 if ( lType == GUICON_PARTITION || lType == GUICON_FOLDER ) return 0;

 if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) &&
       !(  lType == GUICON_AVI ||
           lType == GUICON_MP3 ||
           lType == GUICON_M3U ||
           lType == GUICON_AVIS
        )
 ) return 1;

 return 0;

}  /* end _filter_item */

static unsigned long* GUIFileMenu_RenderItem ( SMS_ListNode* apNode, int anY, int aWidth, int afDim ) {

 int            lLen;
 int            lDWC;
 unsigned long* lpDMA;

 lLen = strlen ( apNode -> m_pString );

 if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) &&
       (  ( int )apNode -> m_Param == GUICON_AVI ||
          ( int )apNode -> m_Param == GUICON_MP3 ||
          ( int )apNode -> m_Param == GUICON_M3U ||
          ( int )apNode -> m_Param == GUICON_AVIS
       )
 ) lLen -= 4;

 while (  GSFont_WidthEx ( apNode -> m_pString, lLen, -2 ) > aWidth  ) --lLen;

 lDWC = GS_TSP_PACKET_SIZE() + GS_TXT_PACKET_SIZE( lLen );

 lpDMA = GSContext_NewList ( lDWC );
 GUI_DrawIcon (
  ( int )apNode -> m_Param + afDim, 8, anY, GUIcon_Browser, lpDMA
 );
 GSFont_RenderEx (  apNode -> m_pString, lLen, 46, anY, lpDMA + GS_TSP_PACKET_SIZE(), -2, 0  );

 lpDMA[ -1 ] = VIF_DIRECT( lDWC >> 1 );

 return lpDMA;

}  /* end GUIFileMenu_RenderItem */

static void GUIFileMenu_Render ( GUIObject* apObj, int aCtx ) {

 GUIFileMenu*  lpMenu = ( GUIFileMenu* )apObj;
 SMS_ListNode* lpNode;

 if ( !lpMenu -> m_pGSPacket ) {

  unsigned long* lpDMA   = GSContext_NewList (  GS_RRT_PACKET_SIZE()  );
  int            lHeight = g_GSCtx.m_Height - 101;
  int            lWidth  = g_GSCtx.m_Width  - 48;
  int            lnItems = ( lHeight - 2 ) / 34;
  int            lY      = 62;
  int            lSelY   = lY + 34 * lpMenu -> m_YOffset;
  unsigned long* lpDMAItem;

  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpDMA - 2 ), 0, 60, g_GSCtx.m_Width - 1, lHeight, -8,
   g_Palette[ *lpMenu -> m_pColor - 1 ]
  );
  lpMenu -> m_pGSPacket = lpDMA;

  if (  ( lpNode = lpMenu -> m_pFirst )  ) {

   if ( lpNode -> m_pPrev ) {

    g_GSCtx.m_TextColor = 2;
    lpDMAItem = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, 1 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, lpNode -> m_pString
    ) -> m_Param = ( unsigned int )lpDMAItem;

    lpMenu -> m_pLast = lpNode;
    lY += 34; --lnItems;
    lpNode = lpNode -> m_pNext;

   }  /* end if */

   g_GSCtx.m_TextColor = 1;

   while ( lnItems > 1 && lpNode ) {

    lpDMAItem = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, 0 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, lpNode -> m_pString
    ) -> m_Param = ( unsigned int )lpDMAItem;

    lpMenu -> m_pLast = lpNode;
    lY += 34; --lnItems;
    lpNode = lpNode -> m_pNext;

   }  /* end while */

   if ( lpNode ) {

    int lClr = lpNode -> m_pNext ? 2 : 1;

    g_GSCtx.m_TextColor = lClr;

    lpDMAItem = GUIFileMenu_RenderItem ( lpNode, lY, lWidth, lClr - 1 );

    SMS_ListPushBack (
     lpMenu -> m_pFileList, lpNode -> m_pString
    ) -> m_Param = ( unsigned int )lpDMAItem;
    lpMenu -> m_pLast = lpNode;

   }  /* end if */

  }  /* end if */

  if ( lpMenu -> m_pCurrent ) {

   unsigned long lColor = g_Palette[ g_Config.m_BrowserSCIdx - 1 ];
   unsigned int  lW     = lWidth + 44;

   if ( !lpMenu -> m_pSelRect ) lpMenu -> m_pSelRect = GSContext_NewList (  GS_RRT_PACKET_SIZE() << 1  );

   GS_RenderRoundRect (
    ( GSRoundRectPacket* )(  lpMenu -> m_pSelRect + GS_RRT_PACKET_SIZE() - 2  ),
    2, lSelY, lW, 34, 8, ( lColor & 0x00FFFFFF ) | 0x10000000
   );
   GS_RenderRoundRect (
    ( GSRoundRectPacket* )( lpMenu -> m_pSelRect - 2 ),
    2, lSelY, lW, 34, -8, lColor
   );
   lpMenu -> m_pSelRect[ -1 ] = VIF_DIRECT(  GS_RRT_PACKET_SIZE()  );

  }  /* end if */

  GSContext_InitBitBlt (
   &s_BitBlt, 0, 0, 60, g_GSCtx.m_Width, lHeight, g_GSCtx.m_VRAMPtr2, 0, 60
  );

 }  /* end if */

 GSContext_CallList ( aCtx, lpMenu -> m_pGSPacket );

 lpNode = lpMenu -> m_pFileList -> m_pHead;

 while ( lpNode ) {

  GSContext_CallList (  aCtx, ( unsigned long* )( unsigned int )lpNode -> m_Param  );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 if ( lpMenu -> m_pSelRect && lpMenu -> m_pFileList -> m_Size ) GSContext_CallList ( aCtx, lpMenu -> m_pSelRect );

}  /* end GUIFileMenu_Render */

static void GUIFileMenu_Cleanup ( GUIObject* apObj ) {

 GUIFileMenu*  lpMenu = ( GUIFileMenu* )apObj;
 SMS_ListNode* lpNode = lpMenu -> m_pFileList -> m_pHead;

 GSContext_DeleteList ( lpMenu -> m_pGSPacket );
 lpMenu -> m_pGSPacket = NULL;

 while ( lpNode ) {

  GSContext_DeleteList (  ( unsigned long* )( unsigned int )lpNode -> m_Param  );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 SMS_ListDestroy ( lpMenu -> m_pFileList, 0 );

}  /* end GUIFileMenu_Cleanup */

static void _redraw ( GUIFileMenu* apMenu ) {

 GUIFileMenu_Cleanup (  ( GUIObject* )apMenu  );
 GSContext_NewPacket ( 1, 0, GSPaintMethod_Init );
 GUIFileMenu_Render (  ( GUIObject* )apMenu, 1  );
 GS_VSync ();
 GSContext_BitBlt ( &s_BitBlt );
 GSContext_Flush ( 1, GSFlushMethod_KeepLists );

}  /* end _redraw */

static void GUIFileMenu_SetFocus ( GUIObject* apObj, int afSet ) {

 GUIFileMenu* lpMenu = ( GUIFileMenu* )apObj;

 lpMenu -> m_Active = afSet;
 lpMenu -> m_pColor = &( afSet ? g_Config.m_BrowserABCIdx : g_Config.m_BrowserIBCIdx );

 _redraw ( lpMenu );

}  /* end GUIFileMenu_SetFocus */

static void _reset ( GUIFileMenu* apMenu ) {

 apMenu -> m_pFirst   = NULL;
 apMenu -> m_pLast    = NULL;
 apMenu -> m_pCurrent = NULL;
 apMenu -> m_YOffset  = 0;

 _redraw ( apMenu );

}  /* end _reset */

static void _push_state ( GUIFileMenu* apMenu ) {

 _LevelInfo* lpInfo = ( _LevelInfo* )malloc (  sizeof ( _LevelInfo )  );

 lpInfo -> m_pCurrent = ( char* )malloc (  strlen ( apMenu -> m_pCurrent -> m_pString ) + 1  );
 lpInfo -> m_YOffset  = apMenu -> m_YOffset;
 lpInfo -> m_Pos      = strlen ( g_CWD );

 strcpy ( lpInfo -> m_pCurrent, apMenu -> m_pCurrent -> m_pString );

 SMS_ListPushBack (
  apMenu -> m_pLevels, apMenu -> m_pFirst -> m_pString
 ) -> m_Param = ( unsigned int )lpInfo;

}  /* end _push_state */

static void _pop_state ( GUIFileMenu* apMenu ) {

 _LevelInfo* lpInfo = ( _LevelInfo* )( unsigned int )apMenu -> m_pLevels -> m_pTail -> m_Param;

 free ( lpInfo -> m_pCurrent );
 free ( lpInfo );

 SMS_ListPopBack ( apMenu -> m_pLevels );

}  /* end _pop_state */

static void _fill ( GUIFileMenu* apMenu, char* apPath ) {

 SMS_ListNode* lpNode;

 _reset ( apMenu );

 SMS_FileDirInit ( apPath );

 lpNode = g_pFileList -> m_pHead;
 SMS_ListDestroy ( apMenu -> m_pFiltList, 0 );

 while ( lpNode ) {

  if (  !_filter_item ( lpNode )  ) SMS_ListPushBack ( apMenu -> m_pFiltList, lpNode -> m_pString ) -> m_Param = lpNode -> m_Param;

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 apMenu -> m_pFirst   =
 apMenu -> m_pCurrent = apMenu -> m_pFiltList -> m_pHead;

}  /* end _fill */

void RestoreFileDir ( void** apParam ) {
 
 apParam[ 0 ] = ( void* )(  ( unsigned int )apParam[ 0 ] & 0x7FFFFFFF  );

 SMS_ListDestroy (  ( SMS_List* )apParam[ 0 ], 1  );
 g_pFileList = ( SMS_List* )apParam[ 1 ];
 strcpy (  g_CWD, ( char* )( apParam + 2 )  );
 free ( apParam );

 GUI_UpdateStatus ();

}  /* end RestoreFileDir */

static void _action_folder ( GUIFileMenu* apMenu, int afPopup ) {

 if ( !afPopup ) {

  _push_state ( apMenu );
  _fill ( apMenu, apMenu -> m_pCurrent -> m_pString );
  _redraw ( apMenu );

 } else {

  void**        lpParam = ( void** )malloc (  strlen ( g_CWD ) + (  sizeof ( SMS_List* ) << 1  ) + 1   );
  SMS_ListNode* lpNode;

  lpParam[ 0 ] = SMS_ListInit ();
  lpParam[ 1 ] = g_pFileList;
  strcpy (  ( char* )( lpParam + 2 ), g_CWD  );

  g_pFileList = NULL;
  SMS_FileDirInit ( apMenu -> m_pCurrent -> m_pString );

  lpNode = g_pFileList -> m_pHead;

  while ( lpNode ) {

   if (  ( int )lpNode -> m_Param == GUICON_MP3  )

    SMS_ListPushBack (  ( SMS_List* )lpParam[ 0 ], lpNode -> m_pString  );

   lpNode = lpNode -> m_pNext;

  }  /* end lpNode */

  SMS_ListDestroy ( g_pFileList, 1 );

  if (   (  ( SMS_List* )lpParam[ 0 ]  ) -> m_Size   ) {

   unsigned long lEvent = GUI_MSG_FOLDER_MP3;

   SMS_ListSort (  ( SMS_List* )lpParam[ 0 ]  );

   lpParam[ 0 ] = ( void* )(  ( unsigned int )lpParam[ 0 ] | 0x80000000  );
   lEvent      |= (  ( unsigned long )( unsigned int )lpParam  ) << 28;

   GUI_PostMessage ( lEvent );

  } else RestoreFileDir ( lpParam );

 }  /* end else */

}  /* end _action_folder */

static void _perform_file_action ( GUIFileMenu* apMenu, int aFlags ) {

 char         lPath[ 1024 ];
 char*        lpPathEnd;
 FileContext* lpFileCtx;

 strcpy ( lPath, g_CWD );

 if (  lPath[ strlen ( lPath ) - 1 ] != '/'  ) SMS_Strcat ( lPath, g_SlashStr );

 lpPathEnd = lPath + strlen ( lPath );
 SMS_Strcat ( lPath, apMenu -> m_pCurrent -> m_pString );

 if ( g_CMedia == 1 && g_pCDDACtx )

  lpFileCtx = CDDA_InitFileContext (  &lPath[ 7 ], g_pCDDACtx );

 else {

  STIO_SetIOMode ( g_CMedia == 2 ? STIOMode_Extended : STIOMode_Ordinary );
  lpFileCtx = STIO_InitFileContext ( lPath, NULL );

 }  /* end else */

 if ( lpFileCtx ) {

  unsigned long lEvent  = GUI_MSG_FILE;
  void**        lpParam = malloc (   (  sizeof ( void* ) << 1  ) + sizeof ( void* )  );

  lpParam[ 0 ] = lpFileCtx;
  lpParam[ 1 ] = NULL;

  if ( aFlags & FA_FLAGS_SUB ) lpParam[ 1 ] = GUI_MiniBrowser ( lpFileCtx, lPath, &lpParam[ 2 ] );

  if (  ( aFlags & FA_FLAGS_AVI ) && !lpParam[ 1 ] && !( aFlags & FA_FLAGS_SUB ) && ( g_Config.m_PlayerFlags & SMS_PF_SUBS )  ) {

   int            i;
   char*          lpDot;
   char           lName[ 1024 ];
   char*          lExt[ 2 ] = { g_pSrtStr,          g_pSubStr          };
   SubtitleFormat lFmt[ 2 ] = { SubtitleFormat_SRT, SubtitleFormat_SUB };

   strcpy ( lName, apMenu -> m_pCurrent -> m_pString );
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

     strcpy ( lpPathEnd, lpNode -> m_pString );
     lpSubFileCtx = lpFileCtx -> Open ( lpName, lpFileCtx -> m_pOpenParam );

     if ( lpSubFileCtx ) {

      lpParam[ 1 ] = ( void* )lpSubFileCtx;
      lpParam[ 2 ] = ( void* )lFmt[ i ];

      break;

     }  /* end if */

    }  /* end if */

   }  /* end for */

  }  /* end if */

  lEvent |= (  ( unsigned long )( unsigned int )lpParam  ) << 28;

  GUI_PostMessage ( lEvent );

 }  /* end if */

}  /* end _perform_file_action */

static void _action_avi ( GUIFileMenu* apMenu, int afPopup ) {

 int lFlags = FA_FLAGS_AVI;

 if ( afPopup ) lFlags |= FA_FLAGS_SUB;

 _perform_file_action ( apMenu, lFlags );

}  /* end _action_avi */

static void _action_file ( GUIFileMenu* apMenu, int afPopup ) {

 _perform_file_action ( apMenu, 0 );

}  /* end _action_file */

static void _action_partition ( GUIFileMenu* apMenu, int afPopup ) {

 int  lPD;
 char lPath[ 2 ] __attribute__(   (  aligned( 4 )  )   );
 char lMountPath[ strlen ( apMenu -> m_pCurrent -> m_pString ) + 6 ];

 strcpy ( lMountPath, g_pDevName[ 2 ] );
 SMS_Strcat ( lMountPath, g_ColonStr                        );
 SMS_Strcat ( lMountPath, apMenu -> m_pCurrent -> m_pString );

 *( unsigned int* )&g_CWD[ 0 ] = 0x30736670;
 *( unsigned int* )&g_CWD[ 4 ] = 0x0000003A;

 if ( apMenu -> m_PD >= 0 ) {

  fileXioUmount ( g_CWD );
  apMenu -> m_PD = -1;

 }  /* end if */

 lPD = fileXioMount ( g_CWD, lMountPath, FIO_MT_RDONLY );

 if ( lPD >= 0 ) {

  strcpy ( g_Config.m_Partition, apMenu -> m_pCurrent -> m_pString );

  *( unsigned short* )&lPath[ 0 ] = 0x002F;
  _push_state ( apMenu );
  _fill ( apMenu, lPath );
  _redraw ( apMenu );

  apMenu -> m_PD = lPD;

 }  /* end if */

}  /* end _action_partition */

static void ( *Action[ 7 ] ) ( GUIFileMenu*, int ) = {
 _action_folder, _action_avi,  _action_file,
 _action_file,   _action_file, _action_partition,
 _action_avi
};

static int GUIFileMenu_HandleEvent ( GUIObject* apObj, unsigned long anEvent ) {

 int          retVal = GUIHResult_Void;
 GUIFileMenu* lpMenu = ( GUIFileMenu* )apObj;

 if ( anEvent & GUI_MSG_PAD_MASK ) switch ( anEvent & GUI_MSG_PAD_MASK ) {

  case SMS_PAD_LEFT :
  case SMS_PAD_RIGHT:

   retVal = GUIHResult_ChangeFocus;

  break;

  case SMS_PAD_DOWN:

   if ( lpMenu -> m_pCurrent && lpMenu -> m_pCurrent -> m_pNext ) {

    lpMenu -> m_pCurrent = lpMenu -> m_pCurrent -> m_pNext;

    if ( lpMenu -> m_pCurrent == lpMenu -> m_pLast && lpMenu -> m_pCurrent -> m_pNext )

     lpMenu -> m_pFirst = lpMenu -> m_pFirst -> m_pNext;

    else ++lpMenu -> m_YOffset;

    goto redraw;

   }  /* end if */

  goto redraw;

  case SMS_PAD_UP:

   if ( lpMenu -> m_pCurrent && lpMenu -> m_pCurrent -> m_pPrev ) {

    lpMenu -> m_pCurrent = lpMenu -> m_pCurrent -> m_pPrev;

    if ( lpMenu -> m_pCurrent == lpMenu -> m_pFirst && lpMenu -> m_pCurrent -> m_pPrev )

     lpMenu -> m_pFirst = lpMenu -> m_pFirst -> m_pPrev;

    else --lpMenu -> m_YOffset;
redraw:
    _redraw ( lpMenu );

    retVal = GUIHResult_Handled;

   }  /* end if */

  break;

  case SMS_PAD_CROSS:

   if ( lpMenu -> m_pCurrent ) {

    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );
    Action[ ( unsigned int )lpMenu -> m_pCurrent -> m_Param >> 1 ] ( lpMenu, 0 );

   }  /* end if */

   retVal = GUIHResult_Handled;

  break;

  case SMS_PAD_CIRCLE:

   if ( lpMenu -> m_pCurrent ) {

    SPU_PlaySound ( SMSound_PAD, g_Config.m_PlayerVolume );
    Action[ ( unsigned int )lpMenu -> m_pCurrent -> m_Param >> 1 ] ( lpMenu, 1 );

   }  /* end if */

   retVal = GUIHResult_Handled;

  break;

  case SMS_PAD_TRIANGLE: {

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

    _fill ( lpMenu, lpPtr );

    lpNode = SMS_ListFind ( lpMenu -> m_pFiltList, lpState -> m_pString );

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
    _redraw    ( lpMenu );

    if ( g_CMedia == 2 && !lpMenu -> m_pLevels -> m_Size ) g_Config.m_Partition[ 0 ] = '\x00';

   }  /* end if */

  } break;

 } else switch ( anEvent ) {

  case GUI_MSG_MEDIA_SELECTED: {

   static int s_fHDDInit = 0;

   while ( lpMenu -> m_pLevels -> m_Size ) _pop_state ( lpMenu );

   if ( g_CMedia == 1 || g_CMedia == 5 ) CDVD_FlushCache ();

   _fill ( lpMenu, g_EmptyStr );

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

   _redraw ( lpMenu );

   retVal = GUIHResult_Handled;

   if ( !lpMenu -> m_Active ) GUI_PostMessage ( SMS_PAD_DOWN );

  } break;

  case GUI_MSG_MEDIA_REMOVED: {

   _reset ( lpMenu );

   retVal = GUIHResult_Handled;

  } break;

  case GUI_MSG_REFILL_BROWSER: {

   if ( g_pFileList ) {

    SMS_ListNode* lpNode = g_pFileList -> m_pHead;
    SMS_ListDestroy ( lpMenu -> m_pFiltList, 0 );

    while ( lpNode ) {

     if (  !_filter_item ( lpNode )  ) SMS_ListPushBack ( lpMenu -> m_pFiltList, lpNode -> m_pString ) -> m_Param = lpNode -> m_Param;

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
 retVal -> m_PD        = 0x80000000;

 return ( GUIObject* )retVal;

}  /* end GUI_CreateFileMenu */
