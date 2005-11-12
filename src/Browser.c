/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# (c) 2005 HOST support by Ronald Andersson (AKA: dlanor)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "Browser.h"
#include "CDDA.h"
#include "CDVD.h"
#include "FileContext.h"
#include "GUI.h"
#include "StringList.h"
#include "Config.h"
#include "Menu.h"

#include <kernel.h>
#include <stdio.h>
#include <string.h>
#include <libhdd.h>
#include <fileXio_rpc.h>
#include <fcntl.h>
#include <libpad.h>

static BrowserContext s_BrowserCtx;
static unsigned int   s_Flags;

static void _check_filter_flag ( void ) {

 if ( g_Config.m_BrowserFlags & SMS_BF_AVIF )

  s_Flags |= SMS_BF_AVIF;

 else s_Flags &= ~SMS_BF_AVIF;

}  /* end _check_filter_flag */

static int _filter_avi ( char* apName ) {

 int lLen   = strlen ( apName );
 int retVal = 0;

 if (  lLen > 4 && !stricmp ( apName + lLen - 4, ".avi"  )  ) {

  apName[ lLen - 4 ] = '\x00';
  retVal             = 1;

 }  /* end if */

 return retVal;

}  /* end _filter_avi */

static int _fill_partition_list ( void ) {

 int          retVal = 0;
 iox_dirent_t lEntry;
 int          lHDDFD;

 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();
 s_BrowserCtx.m_pGUICtx -> Status ( "Reading HDD information..." );

 lHDDFD = fileXioDopen ( "hdd0:" );

 if ( lHDDFD >= 0 ) {

  while (  fileXioDread ( lHDDFD, &lEntry )  ) {

   if (   !(  ( lEntry.stat.attr  & ATTR_SUB_PARTITION ) ||
              ( lEntry.stat.mode == FS_TYPE_EMPTY      )
           )
   ) {

    if (  !( g_Config.m_BrowserFlags & SMS_BF_HDLP ) &&
          (  !strncmp ( lEntry.name, "PP.HDL.", 7 ) ||
             !strncmp ( lEntry.name, "HDLoade", 7 )
          )
    ) continue;

    if (  g_Config.m_BrowserFlags & SMS_BF_SYSP &&
          !strncmp ( lEntry.name, "__", 2 )
    ) continue;

    s_BrowserCtx.m_pGUICtx -> AddFile ( lEntry.name, GUI_FF_PARTITION );
    ++retVal;

   }  /* end if */

  }  /* end while */

  fileXioDclose ( lHDDFD );

 }  /* end if */

 if ( s_BrowserCtx.m_pActivePartition[ 0 ] != '\x00' ) {

  if ( s_Flags & SMS_BF_MENU ) {

   s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );
   s_Flags &= ~SMS_BF_MENU;

  } else s_BrowserCtx.m_pGUICtx -> ActivateFileItem (
          s_BrowserCtx.m_PartIdx, s_BrowserCtx.m_pActivePartition, s_BrowserCtx.m_pFirstPartition
         );

 }  /* end if */

 return retVal;

}  /* end _fill_partition_list */

static int _fill_cdda_directory ( const CDDADirectory* apDir ) {

 int       retVal  = 0;
 CDDAFile* lpFiles = CDDA_GetFileList ( s_BrowserCtx.m_pCDDACtx, apDir );

 CDDA_Stop        ();
 CDDA_Synchronize ();

 if ( lpFiles ) {

  CDDAFile* lpFile = lpFiles;

  while ( lpFile ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpFile -> m_pName, GUI_FF_FILE );

   ++retVal;
   lpFile = lpFile -> m_pNext;

  }  /* end while */

  CDDA_DestroyFileList ( lpFiles );

 }  /* end if */

 _check_filter_flag ();

 return retVal;

}  /* end _fill_cdda_directory */

static int _fill_cdda_list ( void ) {

 int retVal = 0;
 const CDDADirectory* lpDir  = CDDA_DirectoryList ( s_BrowserCtx.m_pCDDACtx );
 const CDDADirectory* lpRoot = NULL;

 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();
 s_BrowserCtx.m_pGUICtx -> Status ( "Reading disk information..." );

 while ( lpDir ) {

  if (  !strcmp ( lpDir -> m_pName, "." )  )

   lpRoot = lpDir;

  else {

   ++retVal;
   s_BrowserCtx.m_pGUICtx -> AddFile ( lpDir -> m_pName, GUI_FF_DIRECTORY );

  }  /* end else */

  lpDir = lpDir -> m_pNext;

 }  /* end while */

 if ( lpRoot ) retVal += _fill_cdda_directory ( lpRoot );

 _check_filter_flag ();

 return retVal;

}  /* end _fill_cdda_list */

static char* _make_path ( char* apBuff ) {

 int             lLen = 0;
 char*           retVal;
 StringListNode* lpNode = s_BrowserCtx.m_pPath -> m_pHead;

 while ( lpNode ) {

  lLen += strlen ( lpNode -> m_pString ) + 1;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 retVal = apBuff ? apBuff : malloc ( ++lLen ); retVal[ 0 ] = '\x00';
 lpNode = s_BrowserCtx.m_pPath -> m_pHead;

 if ( lpNode ) {

  strcpy ( retVal, lpNode -> m_pString );
  lpNode = lpNode -> m_pNext;

  while ( lpNode ) {

   strcat ( retVal, lpNode -> m_pString );
   strcat ( retVal, "/"                 );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

 }  /* end if */

 return retVal;

}  /* end _make_path */

static void _make_bsl_path ( char* apPath ) {

 while ( *apPath ) {

  if ( *apPath == '/' ) *apPath = '\\'; 
  ++apPath;

 }  /* end while */

}  /* end Make_BslPath */

static char* _make_host_path ( void ) {

 int             lLen = 0;
 char*           retVal;
 StringListNode* lpNode = s_BrowserCtx.m_pPath -> m_pHead;

 while ( lpNode ) {

  lLen += strlen ( lpNode -> m_pString ) + 1;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 retVal = malloc ( lLen + 1024 ); retVal[ 0 ] = '\x00';
 lpNode = s_BrowserCtx.m_pPath -> m_pHead;

 if ( lpNode ) {

  strcpy ( retVal, lpNode -> m_pString );
  lpNode = lpNode -> m_pNext;

  while ( lpNode ) {

   strcat ( retVal, lpNode -> m_pString );
   strcat ( retVal, "\\"                );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

 }  /* end if */

 return retVal;

}  /* end _make_host_path */

static void _fill_hdd_list ( void ) {

 iox_dirent_t lEntry;
 char*        lpPath = _make_path ( NULL );
 int          lFD    = fileXioDopen ( lpPath );

 free ( lpPath );

 s_BrowserCtx.m_pGUICtx -> Status ( "Reading directory..." );
 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

 if ( lFD >= 0 ) {

  StringList*     lpDirList  = StringList_Init ();
  StringList*     lpFileList = StringList_Init ();
  StringListNode* lpNode;

  while (  fileXioDread ( lFD, &lEntry )  ) {

   StringList* lpList;

   if ( lEntry.stat.mode & FIO_S_IFDIR ) {

    if (  !strcmp ( lEntry.name, "."  ) ||
          !strcmp ( lEntry.name, ".." )
    ) continue;

    lpList = lpDirList;

   } else if ( lEntry.stat.mode & FIO_S_IFREG ) {

    lpList = lpFileList;

    if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) && !_filter_avi ( lEntry.name )  ) continue;

   } else continue;

   lpList -> PushBack ( lpList, lEntry.name );

  }  /* end while */

  if ( g_Config.m_BrowserFlags & SMS_BF_SORT ) {

   lpDirList  -> Sort ( lpDirList  );
   lpFileList -> Sort ( lpFileList );

  }  /* end if */

  lpNode = lpDirList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_DIRECTORY );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpNode = lpFileList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_FILE );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpDirList  -> Destroy ( lpDirList,  1 );
  lpFileList -> Destroy ( lpFileList, 1 );

  fileXioDclose ( lFD );

 }  /* end if */

 _check_filter_flag ();

}  /* end _fill_hdd_list */

static void _fill_cd_list ( void ) {

 char* lpName               = s_BrowserCtx.m_pPath -> m_pHead ? s_BrowserCtx.m_pPath -> m_pHead -> m_pString : ".";
 const CDDADirectory* lpDir = CDDA_DirectoryList ( s_BrowserCtx.m_pCDDACtx );

 s_BrowserCtx.m_pGUICtx -> Status ( "Reading directory..." );
 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

 while ( lpDir ) {

  if (  !strcmp ( lpDir -> m_pName, lpName )  ) break;

  lpDir = lpDir -> m_pNext;

 }  /* end while */

 if ( lpDir ) {

  CDDAFile* lpFiles = CDDA_GetFileList ( s_BrowserCtx.m_pCDDACtx, lpDir );

  if ( lpFiles ) {

   CDDAFile* lpFile = lpFiles;

   while ( lpFile ) {

    s_BrowserCtx.m_pGUICtx -> AddFile ( lpFile -> m_pName, GUI_FF_FILE );
    lpFile = lpFile -> m_pNext;

   }  /* end while */

   CDDA_DestroyFileList ( lpFiles );

  }  /* end if */

 }  /* end if */

 CDDA_Stop        ();
 CDDA_Synchronize ();

 _check_filter_flag ();

}  /* end _fill_cd_list */

static int _fill_dir_list ( char* apDevName ) {

 int          retVal = 0;
 fio_dirent_t lEntry; 
 char*        lpPath;
 int          lFD;

 s_BrowserCtx.m_pGUICtx -> Status ( "Reading directory..." );
 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

 if ( !s_BrowserCtx.m_pPath -> m_pHead ) s_BrowserCtx.m_pPath -> PushBack ( s_BrowserCtx.m_pPath, apDevName );

 lpPath = _make_path ( NULL );

 lFD = fioDopen ( lpPath );

 free ( lpPath );

 if ( lFD >= 0 ) {

  StringList*     lpDirList  = StringList_Init ();
  StringList*     lpFileList = StringList_Init ();
  StringListNode* lpNode;

  while (  fioDread ( lFD, &lEntry ) == 1  )   {

   StringList* lpList;

   if ( lEntry.stat.mode & FIO_SO_IFDIR ) {

    if (  !strcmp ( lEntry.name, "."  ) ||
          !strcmp ( lEntry.name, ".." )
    ) continue;

    lpList = lpDirList;

   } else if ( lEntry.stat.mode & FIO_SO_IFREG ) {

    lpList = lpFileList;

    if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) && !_filter_avi ( lEntry.name )  ) continue;

   } else continue;

   lpList -> PushBack ( lpList, lEntry.name );

  }  /* end while */

  if ( g_Config.m_BrowserFlags & SMS_BF_SORT ) {

   lpDirList  -> Sort ( lpDirList  );
   lpFileList -> Sort ( lpFileList );

  }  /* end if */

  lpNode = lpDirList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_DIRECTORY );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpNode = lpFileList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_FILE );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpDirList  -> Destroy ( lpDirList,  1 );
  lpFileList -> Destroy ( lpFileList, 1 );

  fioDclose ( lFD );

  retVal = 1;

 }  /* end if */

 _check_filter_flag ();

 return retVal;

}  /* end _fill_dir_list */

static int _fill_dir_list_host ( char* apDevName ) {

 int          retVal = 0;
 fio_dirent_t lEntry; 
 char         *lpPath, *lFilename;
 int          lFD, tFD;

 if ( !s_BrowserCtx.m_pPath -> m_pHead ) s_BrowserCtx.m_pPath -> PushBack ( s_BrowserCtx.m_pPath, apDevName );

 lpPath    = _make_host_path ();
 lFD       = fioDopen ( lpPath );
 lFilename = lpPath + strlen ( lpPath );
 
 s_BrowserCtx.m_pGUICtx -> Status ( "Reading directory..." );
 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

 if ( lFD >= 0 ) {

  StringList*     lpDirList  = StringList_Init ();
  StringList*     lpFileList = StringList_Init ();
  StringListNode* lpNode;
  StringList*     lpList;

  char elflist_char, *elflist_text;
  char pathname[ 1025 ], testpath[ 1025 ];
  int  name_pos, i, elflist_size;

  if (  strcmp ( lpPath, "host:" ) ||                                   /* if this is not host: root directory */
        (   (  tFD = fioOpen ( "host:elflist.txt", O_RDONLY )  ) < 0 )  /* or elflist missing                  */
  ) goto no_elflist;

  if (   (  elflist_size = fioLseek ( tFD, 0, SEEK_END )  ) <= 0   ) {  /* if elflist crashed */

   fioClose( tFD );
   goto no_elflist;

  }  /* end if */
/* extract elflist.txt content names here */
  elflist_text = ( char* )malloc ( elflist_size );

  fioLseek ( tFD, 0, SEEK_SET );
  fioRead  ( tFD, elflist_text, elflist_size );
  fioClose ( tFD );

  name_pos = 0;

  for ( i = 0; i <= elflist_size; ++i ) {

   elflist_char = elflist_text[ i ];

   if (  elflist_char == 0x0A || i == elflist_size ) {

    pathname [ name_pos ] = 0;

    if (  pathname[ 0 ] == 0  ) continue;

    snprintf ( testpath, 1024, "%s%s", "host:", pathname );

    if (   (  tFD = fioOpen ( testpath, O_RDONLY )  ) >= 0   ) {

     fioClose ( tFD );
     lpFileList -> PushBack ( lpFileList, pathname );

    } else if (   (  tFD = fioDopen ( testpath )  ) >= 0   ) {

     fioDclose ( tFD );
     lpDirList -> PushBack ( lpDirList, pathname );

    }  /* end if */

    name_pos = 0;

   } else if ( elflist_char != 0x0D ) pathname[ name_pos++ ] = elflist_char;

  } /* end for */

  free ( elflist_text );

  goto extraction_done;
/* This point is only reached if elflist.txt is NOT to be used */
no_elflist:
/* when not dealing with elflist.txt, extract folder content names here */
  while (  fioDread ( lFD, &lEntry ) )   {

   strcpy ( lFilename, lEntry.name );

   if ( ( tFD = fioOpen ( lpPath, O_RDONLY ) ) >= 0 ) {

    fioClose ( tFD );
    lEntry.stat.mode = FIO_SO_IFREG;

   } else if (   (  tFD = fioDopen ( lpPath )  ) >= 0   ) {

    fioDclose ( tFD );
    lEntry.stat.mode = FIO_SO_IFDIR;

   }  /* end if */

   if ( lEntry.stat.mode & FIO_SO_IFDIR ) {

    if (  !strcmp ( lEntry.name, "."  ) ||
          !strcmp ( lEntry.name, ".." )
    ) continue;

    lpList = lpDirList;

   } else if ( lEntry.stat.mode & FIO_SO_IFREG ) {

    lpList = lpFileList;

    if (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) && !_filter_avi ( lEntry.name )  ) continue;

   } else continue;

   lpList -> PushBack ( lpList, lEntry.name );

  }  /* end while */
/* names are already extracted to lpFileList and lpDirList */
/* it is now time to pass them on to the GUI context       */
extraction_done:
  if ( g_Config.m_BrowserFlags & SMS_BF_SORT ) {

   lpDirList  -> Sort ( lpDirList  );
   lpFileList -> Sort ( lpFileList );

  }  /* end if */

  lpNode = lpDirList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_DIRECTORY );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpNode = lpFileList -> m_pHead;

  while ( lpNode ) {

   s_BrowserCtx.m_pGUICtx -> AddFile ( lpNode -> m_pString, GUI_FF_FILE );
   lpNode = lpNode -> m_pNext;

  }  /* end while */

  lpDirList  -> Destroy ( lpDirList,  1 );
  lpFileList -> Destroy ( lpFileList, 1 );

  fioDclose ( lFD );

  retVal = 1;

 }  /* end if */

 free ( lpPath );

 _check_filter_flag ();

 return retVal;

}  /* end _fill_dir_list_host */

static void _select_partition ( char* apName ) {

 char lName[ strlen ( apName ) + 6 ]; 

 s_BrowserCtx.m_pGUICtx -> Status ( "Mounting selected partition..." );

 if ( s_BrowserCtx.m_HDDPD != -1 ) fileXioUmount ( "pfs0:" );

 strcpy ( lName, "hdd0:" );
 strcat ( lName, apName  );

 s_BrowserCtx.m_HDDPD = fileXioMount ( "pfs0:", lName, FIO_MT_RDONLY );

 if ( s_BrowserCtx.m_HDDPD >= 0 ) {

  int lLen  = strlen ( s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pCurr  -> m_pFileName );
  int lFLen = strlen ( s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName );

  s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );
  s_BrowserCtx.m_pPath -> PushBack ( s_BrowserCtx.m_pPath, "pfs0:/" );

  s_BrowserCtx.m_PartIdx = s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_Offset;

  if ( lLen > s_BrowserCtx.m_nAlloc ) {

   s_BrowserCtx.m_pActivePartition = ( char* )realloc ( s_BrowserCtx.m_pActivePartition, lLen + 1 );
   s_BrowserCtx.m_nAlloc           = lLen;

  }  /* end if */

  strcpy ( s_BrowserCtx.m_pActivePartition, s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pCurr -> m_pFileName );

  if ( lFLen > s_BrowserCtx.m_nFAlloc ) {

   s_BrowserCtx.m_pFirstPartition = ( char* )realloc ( s_BrowserCtx.m_pFirstPartition, lFLen + 1 );
   s_BrowserCtx.m_nFAlloc         = lLen;

  }  /* end if */

  strcpy ( s_BrowserCtx.m_pFirstPartition, s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName );

  _fill_hdd_list ();

  s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

 } else {

  char lMsg[ 512 ]; sprintf ( lMsg, "Error %d. Press X to continue...", s_BrowserCtx.m_HDDPD );

  s_BrowserCtx.m_pGUICtx -> Status ( lMsg );

  GUI_WaitButton ( PAD_CROSS, 0 );

  s_BrowserCtx.m_HDDPD = -1;

 }  /* end else */

 _check_filter_flag ();

}  /* end _select_partition */

static void _mount_partition ( char* apName ) {

 if (  s_BrowserCtx.m_pGUICtx -> SelectFile ( apName )  ) _select_partition ( apName );

}  /* end _mount_partition */

static void _display_status ( void ) {

 static int s_Detect = 0;

 if ( s_Flags & SMS_BF_DISK ) {

  if ( !s_Detect ) {

   s_BrowserCtx.m_pGUICtx -> Status ( "Detecting disk type..." );
   s_Detect = 1;

  }  /* end if */

 } else {

  char lBuffer[ 2048 ];

  if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems == NULL || s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems -> m_Flags == GUI_DF_CDDA ) {

   strcpy ( lBuffer, "Waiting for media (press \"start\" for menu)..." );

  } else {

   _make_path ( lBuffer );

   if (  !lBuffer[ 0 ] || !strchr ( lBuffer, ':' )  )

    switch ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags ) {

     case GUI_DF_CDROM:

      if ( !lBuffer[ 0 ] )

       strcpy ( lBuffer, "cddafs:/" );

      else {

       memmove (  &lBuffer[ 8 ], lBuffer, strlen ( lBuffer ) + 1  );
       memcpy ( lBuffer, "cddafs:/", 8 );

      }  /* end else */

     break;

     case GUI_DF_HDD:

      strcpy ( lBuffer, "hdd0:/" );

     break;

    }  /* end switch */

  }  /* end else */

  s_BrowserCtx.m_pGUICtx -> Status ( lBuffer );
  s_Detect = 0;

 }  /* end else */

}  /* end _display_status */

static void Browser_Destroy ( void ) {

 s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 1 );

 if ( s_BrowserCtx.m_pCDDACtx    ) CDDA_DestroyContext ( s_BrowserCtx.m_pCDDACtx );
 if ( s_BrowserCtx.m_HDDPD != -1 ) fileXioUmount ( "pfs0:" );

 free ( s_BrowserCtx.m_pActivePartition );
 free ( s_BrowserCtx.m_pFirstPartition  );

}  /* end Browser_Destroy */

static void _handle_unmount ( void ) {

 int lRes = 0;

 s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );
 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

 if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr )

  switch ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags ) {

   case GUI_DF_HDD:

    lRes = _fill_partition_list ();

   break;

   case GUI_DF_USBM:

    lRes = _fill_dir_list ( "mass:/" );

   break;

   case GUI_DF_CDFS:

    lRes = _fill_dir_list ( "cdfs:/" ); CDVD_Stop ();

   break;

   case GUI_DF_CDROM:

    lRes = _fill_cdda_list ();

   break;

   case GUI_DF_HOST:

    lRes = _fill_dir_list_host ( "host:" );

   break;

  }  /* end switch */

 s_BrowserCtx.m_pGUICtx -> ActivateMenu ( lRes );

}  /* end _handle_unmount */

static FileContext* Browser_Browse ( char* apPartName ) {

 FileContext* retVal = NULL;

 if ( s_BrowserCtx.m_HDDPD == -1 && s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems == NULL )

  if (  !hddCheckPresent () && !hddCheckFormatted () && _fill_partition_list ()  ) {

   s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_HDD );
   s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

   if ( apPartName && apPartName[ 0 ] ) _mount_partition ( apPartName );

  }  /* end if */

 _display_status ();

 if ( s_BrowserCtx.m_pGUICtx -> m_pCurrentMenu )

  s_BrowserCtx.m_pGUICtx -> ActivateMenu (
   s_BrowserCtx.m_pGUICtx -> m_pCurrentMenu == ( GUIMenu* )&s_BrowserCtx.m_pGUICtx -> m_DevMenu ? 0 : 1
  );

 while ( 1 ) {

  void* lpItem;
  int   lEvent = s_BrowserCtx.m_pGUICtx -> Run ( &lpItem );

  switch ( lEvent ) {

   case GUI_EV_MENU: {

    s_BrowserCtx.m_pMenuCtx -> Run ();
    s_Flags |= SMS_BF_MENU;
    s_BrowserCtx.m_pGUICtx -> ActivateMenu (
     s_BrowserCtx.m_pGUICtx -> m_pCurrentMenu == ( GUIMenu* )&s_BrowserCtx.m_pGUICtx -> m_DevMenu ? 0 : 1
    );

   } break;

   case GUI_EV_EXIT: {

    SMS_ResetIOP ();
    Exit ( 0 );

   } break;

   case GUI_EV_FILE_SELECT: {

    s_BrowserCtx.m_pActiveItem = ( GUIFileMenuItem* )lpItem;

    switch ( s_BrowserCtx.m_pActiveItem -> m_Flags & 0x0000000F ) {

     case GUI_FF_PARTITION:
     case GUI_FF_DIRECTORY: {

      s_BrowserCtx.Reload ();

     } break;

     case GUI_FF_FILE: {

      char* lpPath = _make_path ( NULL );
      int   lLen;
      char  lFullPath[ (  lLen = strlen ( lpPath )  ) + strlen ( s_BrowserCtx.m_pActiveItem -> m_pFileName ) + (  ( g_Config.m_BrowserFlags & SMS_BF_AVIF ) ? 6 : 2  ) ];

      strcpy ( lFullPath, lpPath ); free ( lpPath );

      if ( lFullPath[ lLen - 1 ] != '/' ) strcat ( lFullPath, "/" );

      strcat ( lFullPath, s_BrowserCtx.m_pActiveItem -> m_pFileName );

      if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM )

       retVal = CDDA_InitFileContext ( s_BrowserCtx.m_pCDDACtx, lFullPath );

      else {

       if ( s_Flags & SMS_BF_AVIF ) strcat ( lFullPath, ".avi" );

       if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HDD )

        STIO_SetIOMode ( STIOMode_Extended );

       else {

        STIO_SetIOMode ( STIOMode_Ordinary );

        if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HOST ) _make_bsl_path ( lFullPath );

       }  /* end else */

       retVal = STIO_InitFileContext ( lFullPath );

      }  /* end else */

     } break;

    }  /* end switch */

   } break;

   case GUI_EV_UPLEVEL:

    if ( s_BrowserCtx.m_pPath -> m_pHead ) {

     char  lName[ strlen ( s_BrowserCtx.m_pPath -> m_pTail -> m_pString    ) + 1 ];
     char* lpState = ( char* )s_BrowserCtx.m_pPath -> m_pTail -> m_pParam;
     char* lpFirst = NULL;
     int   lIdx    = 0;

     strcpy ( lName, s_BrowserCtx.m_pPath -> m_pTail -> m_pString );

     if ( lpState ) {

      lpFirst = ( char* )malloc (  strlen ( lpState + 4 ) + 1  );
      strcpy ( lpFirst, lpState + 4 );
      lIdx = *( int* )lpState;

     }  /* end if */

     free ( s_BrowserCtx.m_pPath -> m_pTail -> m_pParam );

     s_BrowserCtx.m_pPath -> PopBack ( s_BrowserCtx.m_pPath );

     if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HDD ) {

      if ( s_BrowserCtx.m_pPath -> m_pHead ) {

       _fill_hdd_list ();
       s_BrowserCtx.m_pGUICtx -> ActivateFileItem ( lIdx, lName, lpFirst );

      } else {

       fileXioUmount ( "pfs0:" ); s_BrowserCtx.m_HDDPD = -1;
       _fill_partition_list ();

      }  /* end else */

     } else if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM ) {

      _fill_cdda_list ();
      s_BrowserCtx.m_pGUICtx -> ActivateFileItem ( lIdx, lName, lpFirst );

     } else if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_USBM ) {

      _fill_dir_list ( "mass:/" );

      if ( lpFirst )

       s_BrowserCtx.m_pGUICtx -> ActivateFileItem ( lIdx, lName, lpFirst );

      else s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

     } else if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HOST ) {

      _fill_dir_list_host ( "host:" );

      if ( lpFirst )

       s_BrowserCtx.m_pGUICtx -> ActivateFileItem ( lIdx, lName, lpFirst );

      else s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

     } else if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDFS ||
                 s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_DVD
            ) {

      _fill_dir_list ( "cdfs:/" ); CDVD_Stop ();

      if ( lpFirst )

       s_BrowserCtx.m_pGUICtx -> ActivateFileItem ( lIdx, lName, lpFirst );

      else s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

      CDVD_Stop ();

     }  /* end if */

     if ( lpFirst ) free ( lpFirst );

    }  /* end if */

   break;

   case GUI_EV_DEV_SELECT: {

    GUIDeviceMenuItem* lpDevice = ( GUIDeviceMenuItem* )lpItem;

    s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );

    if ( lpDevice -> m_Flags == GUI_DF_HDD ) {

     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_partition_list () ? 1 : 0  );

    } else if ( lpDevice -> m_Flags == GUI_DF_CDROM && s_BrowserCtx.m_pCDDACtx ) {

     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_cdda_list () ? 1 : 0  );

    } else if ( lpDevice -> m_Flags == GUI_DF_USBM ) {

     s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list ( "mass:/" ) ? 1 : 0  );

    } else if ( lpDevice -> m_Flags == GUI_DF_HOST ) {

     s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list_host ( "host:" ) ? 1 : 0  );

    } else if ( lpDevice -> m_Flags == GUI_DF_CDFS || lpDevice -> m_Flags == GUI_DF_DVD ) {

     s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list ( "cdfs:/" ) ? 1 : 0  );

     CDVD_Stop ();

    }  /* end if */

   } break;

   case GUI_EV_CDROM_MOUNT: {

    s_BrowserCtx.m_pGUICtx -> Status ( "Reading disk..." );
    s_BrowserCtx.m_pCDDACtx = CDDA_InitContext ( 0 );

    if ( s_BrowserCtx.m_pCDDACtx ) {

     s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_CDROM );

     if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM ) {

      s_BrowserCtx.m_pPath -> Destroy  ( s_BrowserCtx.m_pPath, 0 );
      s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_cdda_list () ? 1 : 0  );

     }  /* end if */

     s_Flags &= ~SMS_BF_DISK;

     CDDA_Stop        ();
     CDDA_Synchronize ();

    } else {

     s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_CDDA );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 0 );

    }  /* end else */

   } break;

   case GUI_EV_CDROM_UMOUNT: {

    s_BrowserCtx.m_pGUICtx -> Status ( "Unmounting disk..." );

    if ( s_BrowserCtx.m_pCDDACtx ) {

     CDDA_DestroyContext ( s_BrowserCtx.m_pCDDACtx );
     s_BrowserCtx.m_pCDDACtx = NULL;
     s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_CDROM );

    } else s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_CDDA );

    _handle_unmount ();

   } break;

   case GUI_EV_SAVE_CONFIG: {

    s_BrowserCtx.m_pGUICtx -> Status ( "Saving configuration..." );

    g_Config.m_DX          = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_StartX;
    g_Config.m_DY          = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_StartY;
    g_Config.m_DisplayMode = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_DisplayMode;
    strncpy ( g_Config.m_Partition, s_BrowserCtx.m_pActivePartition, 255 );

    if (  !SaveConfig ()  ) {

     s_BrowserCtx.m_pGUICtx -> Status ( "Error. Press X to continue..." );
     GUI_WaitButton ( PAD_CROSS, 0 );

    }  /* end if */

   } break;

   case GUI_EV_USBM_MOUNT: {

    s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_USBM );

    if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_USBM ) {

     s_BrowserCtx.m_pPath -> Destroy  ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list ( "mass:/" ) ? 1 : 0  );

    }  /* end if */

   } break;

   case GUI_EV_USBM_UMOUNT: {

    s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_USBM );
    _handle_unmount ();

   } break;

   case GUI_EV_CDFS_MOUNT: {

    int lDevice;
    int lSetDVDV  = 0;
    int lDiskType = CDDA_DiskType ();

    switch ( lDiskType ) {

     case DiskType_CD  : lDevice = GUI_DF_CDFS; break;
     case DiskType_DVDV: lSetDVDV = 1;
     default           : lDevice = GUI_DF_DVD;  break;

    }  /* end switch */

    CDVD_SetDVDV ( lSetDVDV );

    s_BrowserCtx.m_pGUICtx -> AddDevice ( lDevice );

    if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDFS ||
         s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_DVD
    ) {

     s_BrowserCtx.m_pPath -> Destroy  ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list ( "cdfs:/" ) ? 1 : 0  );

     CDVD_Stop ();

    }  /* end if */

    s_Flags &= ~SMS_BF_DISK;

   } break;

   case GUI_EV_CDFS_UMOUNT: {

    s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_CDFS );
    s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_DVD  );
    _handle_unmount ();
    CDVD_FlushCache ();

   } break;

   case GUI_EV_HOST_MOUNT: {

    s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_HOST );

    if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HOST ) {

     s_BrowserCtx.m_pPath -> Destroy  ( s_BrowserCtx.m_pPath, 0 );
     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_dir_list_host ( "host:" ) ? 1 : 0  );

    }  /* end if */

   } break;

   case GUI_EV_HOST_UMOUNT: {

    s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_HOST );
    _handle_unmount ();

   } break;

   case GUI_EV_DISK_DETECT:

    s_Flags |= SMS_BF_DISK;;

   break;

   case GUI_EV_DISK_NODISK:

    s_Flags &= ~SMS_BF_DISK;;

   break;

  }  /* end switch */

  _display_status ();

  if ( retVal ) break;

 }  /* end while */

 return retVal;

}  /* end Browser_Browse */

static void Browser_Reload ( void ) {

 if (  ( s_BrowserCtx.m_pActiveItem -> m_Flags & 0x0000000F ) == GUI_FF_PARTITION  ) {

  _select_partition ( s_BrowserCtx.m_pActiveItem -> m_pFileName );

 } else {

  char* lpState = ( char* )malloc (  strlen ( s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName ) + 5  );

  *( int* )lpState = s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_Offset;
  strcpy ( lpState + 4, s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName );

  s_BrowserCtx.m_pPath -> PushBack ( s_BrowserCtx.m_pPath, s_BrowserCtx.m_pActiveItem -> m_pFileName );
  s_BrowserCtx.m_pPath -> m_pTail -> m_pParam = lpState;

  if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HDD )

   _fill_hdd_list ();

  else if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM )

   _fill_cd_list ();

  else if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_USBM )

   _fill_dir_list ( "mass:/" );

  else if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HOST )

   _fill_dir_list_host ( "host:" );

  else if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDFS ||
             s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_DVD
       ) {

   _fill_dir_list ( "cdfs:/" ); CDVD_Stop ();

  }  /* end else */

  s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

 }  /* end else */

}  /* end Browser_Reload */

BrowserContext* BrowserContext_Init ( struct GUIContext* apGUICtx ) {

 s_BrowserCtx.m_pGUICtx  = apGUICtx;
 s_BrowserCtx.m_pCDDACtx = NULL;
 s_BrowserCtx.m_HDDPD    = -1;
 s_BrowserCtx.m_PartIdx  =  0;
 s_BrowserCtx.m_pPath    = StringList_Init ();
 s_BrowserCtx.m_CurDev   = -1;
 s_BrowserCtx.m_pMenuCtx = MenuContext_Init ( &s_BrowserCtx );
 s_BrowserCtx.Browse     = Browser_Browse;
 s_BrowserCtx.Destroy    = Browser_Destroy;
 s_BrowserCtx.Reload     = Browser_Reload;

 s_BrowserCtx.m_PartIdx          = 0;
 s_BrowserCtx.m_pActivePartition = ( char* )calloc ( 256, 1 );
 s_BrowserCtx.m_nAlloc           = 255;
 s_BrowserCtx.m_pFirstPartition  = ( char* )calloc ( 256, 1 );
 s_BrowserCtx.m_nFAlloc          = 255;

 s_Flags = g_Config.m_BrowserFlags & SMS_BF_AVIF;

 return &s_BrowserCtx;

}  /* end BrowserContext_Init */
