#include "Browser.h"
#include "CDDA.h"
#include "FileContext.h"
#include "GUI.h"
#include "StringList.h"
#include "Config.h"

#include <stdio.h>
#include <string.h>
#include <libhdd.h>
#include <fileXio_rpc.h>
#include <fcntl.h>
#include <libpad.h>

static BrowserContext s_BrowserCtx;

static int _fill_partition_list ( void ) {

 int          retVal = 0;
 iox_dirent_t lEntry;
 int          lHDDFD;

 s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();
 s_BrowserCtx.m_pGUICtx -> Status ( "Reading HDD information..." );

 lHDDFD = fileXioDopen ( "hdd0:" );

 if ( lHDDFD >= 0 ) {

  while (  fileXioDread ( lHDDFD, &lEntry )  ) {

   if (   !(  ( lEntry.stat.attr  & ATTR_SUB_PARTITION             ) ||
              ( lEntry.stat.mode == FS_TYPE_EMPTY                  ) ||
              ( lEntry.name[ 0 ] == '_' && lEntry.name[ 1 ] == '_' )
           )
   ) {

    s_BrowserCtx.m_pGUICtx -> AddFile ( lEntry.name, GUI_FF_PARTITION );
    ++retVal;

   }  /* end if */

  }  /* end while */

  fileXioDclose ( lHDDFD );

 }  /* end if */

 if ( s_BrowserCtx.m_pActivePartition[ 0 ] != '\x00' )

  s_BrowserCtx.m_pGUICtx -> ActivateFileItem (
   s_BrowserCtx.m_PartIdx, s_BrowserCtx.m_pActivePartition, s_BrowserCtx.m_pFirstPartition
  );

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

 return retVal;

}  /* end _fill_cdda_list */

static char* _make_path ( void ) {

 int             lLen = 0;
 char*           retVal;
 StringListNode* lpNode = s_BrowserCtx.m_pPath -> m_pHead;

 while ( lpNode ) {

  lLen += strlen ( lpNode -> m_pString ) + 1;
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 retVal = malloc ( ++lLen ); retVal[ 0 ] = '\x00';
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

static void _fill_hdd_list ( void ) {

 iox_dirent_t lEntry;
 char*        lpPath = _make_path ();
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

   } else if ( lEntry.stat.mode & FIO_S_IFREG )

    lpList = lpFileList;

   else continue;

   lpList -> PushBack ( lpList, lEntry.name );

  }  /* end while */

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

}  /* end _fill_cd_list */

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

  GUI_WaitButton ( PAD_CROSS );

  s_BrowserCtx.m_HDDPD = -1;

 }  /* end else */

}  /* end _select_partition */

static void _mount_partition ( char* apName ) {

 if (  s_BrowserCtx.m_pGUICtx -> SelectFile ( apName )  ) _select_partition ( apName );

}  /* end _mount_partition */

static void _display_status ( void ) {

 s_BrowserCtx.m_pGUICtx -> Status (
  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems == NULL ||
  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems -> m_Flags == GUI_DF_CDDA ? "Waiting for SMS media disk..." : "Ready"
 );

}  /* end _display_status */

static void Browser_Destroy ( void ) {

 s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 1 );

 if ( s_BrowserCtx.m_pCDDACtx    ) CDDA_DestroyContext ( s_BrowserCtx.m_pCDDACtx );
 if ( s_BrowserCtx.m_HDDPD != -1 ) fileXioUmount ( "pfs0:" );

 free ( s_BrowserCtx.m_pActivePartition );
 free ( s_BrowserCtx.m_pFirstPartition  );

}  /* end Browser_Destroy */

static FileContext* Browser_Browse ( char* apPartName ) {

 FileContext* retVal = NULL;

 if ( s_BrowserCtx.m_HDDPD == -1 && s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pItems == NULL )

  if (  !hddCheckPresent () && !hddCheckFormatted () && _fill_partition_list ()  ) {

   s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_HDD );
   s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

   if ( apPartName && apPartName[ 0 ] ) _mount_partition ( apPartName );

  }  /* end if */

 _display_status ();

 while ( 1 ) {

  void* lpItem;
  int   lEvent = s_BrowserCtx.m_pGUICtx -> Run ( &lpItem );

  switch ( lEvent ) {

   case GUI_EV_FILE_SELECT: {

    GUIFileMenuItem* lpFile = ( GUIFileMenuItem* )lpItem;

    switch ( lpFile -> m_Flags & 0x0000000F ) {

     case GUI_FF_PARTITION:

      _select_partition ( lpFile -> m_pFileName );

     break;

     case GUI_FF_DIRECTORY: {

      char* lpState = ( char* )malloc (  strlen ( s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName ) + 5  );

      *( int* )lpState = s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_Offset;
      strcpy ( lpState + 4, s_BrowserCtx.m_pGUICtx -> m_FileMenu.m_pFirst -> m_pFileName );

      s_BrowserCtx.m_pPath -> PushBack ( s_BrowserCtx.m_pPath, lpFile -> m_pFileName );
      s_BrowserCtx.m_pPath -> m_pTail -> m_pParam = lpState;

      if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HDD )

       _fill_hdd_list ();

      else if (  s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM )

       _fill_cd_list ();

      s_BrowserCtx.m_pGUICtx -> ActivateMenu ( 1 );

     } break;

     case GUI_FF_FILE: {

      char* lpPath = _make_path ();
      int   lLen;
      char  lFullPath[ (  lLen = strlen ( lpPath )  ) + strlen ( lpFile -> m_pFileName ) + 2 ];

      strcpy ( lFullPath, lpPath ); free ( lpPath );

      if ( lFullPath[ lLen - 1 ] != '/' ) strcat ( lFullPath, "/" );

      strcat ( lFullPath, lpFile -> m_pFileName );

      if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_HDD )

       retVal = STIO_InitFileContext ( lFullPath );

      else retVal = CDDA_InitFileContext ( s_BrowserCtx.m_pCDDACtx, lFullPath );

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

     }  /* end if */

     if ( lpFirst ) free ( lpFirst );

    }  /* end if */

   break;

   case GUI_EV_DEV_SELECT: {

    GUIDeviceMenuItem* lpDevice = ( GUIDeviceMenuItem* )lpItem;

    s_BrowserCtx.m_pPath -> Destroy ( s_BrowserCtx.m_pPath, 0 );

    if ( lpDevice -> m_Flags == GUI_DF_HDD ) {

     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_partition_list () ? 1 : 0  );

    } else if ( lpDevice -> m_Flags == GUI_DF_CDROM && s_BrowserCtx.m_pCDDACtx )

     s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_cdda_list () ? 1 : 0  );

   } break;

   case GUI_EV_CDROM_MOUNT: {

    s_BrowserCtx.m_pGUICtx -> Status ( "Reading disk..." );
    s_BrowserCtx.m_pCDDACtx = CDDA_InitContext ( 0 );

    if ( s_BrowserCtx.m_pCDDACtx ) {

     s_BrowserCtx.m_pGUICtx -> AddDevice ( GUI_DF_CDROM );

     if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr -> m_Flags & GUI_DF_CDROM ) s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_cdda_list () ? 1 : 0  );

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
     s_BrowserCtx.m_pGUICtx -> ClearFileMenu ();

    } else s_BrowserCtx.m_pGUICtx -> DelDevice ( GUI_DF_CDDA );

    if ( s_BrowserCtx.m_pGUICtx -> m_DevMenu.m_pCurr ) s_BrowserCtx.m_pGUICtx -> ActivateMenu (  _fill_partition_list () ? 1 : 0  );

   } break;

   case GUI_EV_SAVE_CONFIG: {

    s_BrowserCtx.m_pGUICtx -> Status ( "Saving configuration..." );

    g_Config.m_DX          = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_StartX;
    g_Config.m_DY          = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_StartY;
    g_Config.m_DisplayMode = s_BrowserCtx.m_pGUICtx -> m_pGSCtx -> m_DisplayMode;
    strncpy ( g_Config.m_Partition, s_BrowserCtx.m_pActivePartition, 255 );

    if (  !SaveConfig ()  ) {

     s_BrowserCtx.m_pGUICtx -> Status ( "Error. Press X to continue..." );
     GUI_WaitButton ( PAD_CROSS );

    }  /* end if */

   } break;

  }  /* end switch */

  _display_status ();

  if ( retVal ) break;

 }  /* end while */

 return retVal;

}  /* end Browser_Browse */

BrowserContext* BrowserContext_Init ( struct GUIContext* apGUICtx ) {

 s_BrowserCtx.m_pGUICtx  = apGUICtx;
 s_BrowserCtx.m_pCDDACtx = NULL;
 s_BrowserCtx.m_HDDPD    = -1;
 s_BrowserCtx.m_PartIdx  =  0;
 s_BrowserCtx.m_pPath    = StringList_Init ();
 s_BrowserCtx.m_CurDev   = -1;
 s_BrowserCtx.Browse     = Browser_Browse;
 s_BrowserCtx.Destroy    = Browser_Destroy;

 s_BrowserCtx.m_PartIdx          = 0;
 s_BrowserCtx.m_pActivePartition = ( char* )calloc ( 256, 1 );
 s_BrowserCtx.m_nAlloc           = 255;
 s_BrowserCtx.m_pFirstPartition  = ( char* )calloc ( 256, 1 );
 s_BrowserCtx.m_nFAlloc          = 255;

 return &s_BrowserCtx;

}  /* end BrowserContext_Init */
