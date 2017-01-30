/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_ContainerM3U.h"
#include "SMS_ContainerMP3.h"
#include "SMS_ContainerOGG.h"
#include "SMS_ContainerASF.h"
#include "SMS_ContainerMOV.h"
#include "SMS_ContainerAAC.h"
#include "SMS_ContainerFLAC.h"
#include "SMS_ContainerAC3.h"
#include "SMS_List.h"
#include "SMS_Locale.h"
#include "SMS_FileDir.h"
#include "SMS_Sounds.h"
#include "SMS_Config.h"
#include "SMS_Timer.h"

#include <string.h>
#include <malloc.h>
#include <stdio.h>

#define MYCONT( c ) (  ( _M3UContainer* )c -> m_pCtx                   )
#define MYENTR( e ) (  ( _M3UEntry*     )( unsigned int )e -> m_Param  )

typedef struct _M3UEntry {

 unsigned int m_Duration;
 char*        m_pPath;

} _M3UEntry;

typedef struct _M3UContainer {

 SMS_Container* m_pSubCont;
 int            m_fInit;
 int            m_Active;
 int            m_ContID[ 7 ];
 uint64_t       ( *Probe[ 7 ] ) ( FileContext*, SMS_AudioInfo* );

} _M3UContainer;

#define M3U_MAXLEN 512

static int _SelectFile ( SMS_Container*, _M3UContainer* );

static void _m3u_destroy_list ( SMS_List* apList ) {

 SMS_ListNode* lpNode = apList -> m_pHead;

 while ( lpNode ) {

  free (   (  ( _M3UEntry* )( unsigned int )lpNode -> m_Param  ) -> m_pPath   );
  free (  ( void* )( unsigned int )lpNode -> m_Param  );
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 SMS_ListDestroy ( apList, 1 );

}  /* end _m3u_destroy_list */

static int _check_name ( char* apName ) {

 int i, lLen = strlen ( apName );

 for ( i = 0; i < lLen; ++i ) if ( apName[ i ] < ' ' ) break;

 return i == lLen ? lLen : 0;

}  /* end _check_name */

static FileContext* _open_file (
                     SMS_Container* apCont,
                     SMS_ListNode*  apNode,
                     FileContext* ( *apOpen ) ( const char*, void* ),
                     void* apOpenParam
                    ) {

 char lFileName[ 1024 ];

 strcpy ( lFileName, g_CWD );

 if (  MYENTR( apNode ) -> m_pPath[ 0 ] != '/' &&
       g_CWD[ strlen ( g_CWD ) - 1 ]    != '/'
 ) strcat (  lFileName, g_SlashStr );

 strcat (  lFileName, MYENTR( apNode ) -> m_pPath  );

 return apOpen ( lFileName, apOpenParam );

}  /* end _open_file */

static int _ReadPacket ( SMS_Container* apCont, int* apIdx ) {

 int            retVal   = -1;
 _M3UContainer* lpMyCont = MYCONT( apCont );

 if (   (  retVal = lpMyCont -> m_pSubCont -> ReadPacket ( lpMyCont -> m_pSubCont, apIdx )  ) < 0   ) {

  if ( apCont -> m_pPlayItem -> m_pNext ) {

   apCont -> m_pPlayItem = apCont -> m_pPlayItem -> m_pNext;

   if (  _SelectFile ( apCont, lpMyCont )  ) retVal = apCont -> ReadPacket ( apCont, apIdx );

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _ReadPacket */

static int _ReadFirstPacket ( SMS_Container* apCont, int* apIdx ) {

 int               i, lfDoInit = 0, retVal  = 1;
 _M3UContainer*    lpMyCont   = MYCONT( apCont );
 FileContext*      lpFileCtx  = lpMyCont -> m_pSubCont -> m_pFileCtx;
 SMS_CodecContext* lpCodecCtx = apCont -> m_pStm[ 0 ] -> m_pCodec;
 SMS_AudioInfo     lInfo;

 i = lpMyCont -> m_Active;

 if ( !lpMyCont -> m_fInit ) {

  int lSubContID = SMS_SubContID ( lpFileCtx -> m_pPath );

  i = lSubContID < 0 ? 0 : lSubContID;

  while ( 1 ) {

   for (  ; i < sizeof ( lpMyCont -> Probe ) / sizeof ( lpMyCont -> Probe[ 0 ] ); ++i  )
    if (  lpMyCont -> Probe[ i ] ( lpFileCtx, &lInfo )  ) {
     if ( i == SMS_SUBCONTAINER_M4A ||
          i == SMS_SUBCONTAINER_ASF ||
          i == SMS_SUBCONTAINER_FLAC
     ) lfDoInit = 1;
     break;
    } else RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );

   if (  i == sizeof ( lpMyCont -> Probe ) / sizeof ( lpMyCont -> Probe[ 0 ] ) && lSubContID >= 0  ) {
    i          = 0;
    lSubContID = -1;
   } else break;

  }  /* end while */

  if (  i != sizeof ( lpMyCont -> Probe ) / sizeof ( lpMyCont -> Probe[ 0 ] )  ) {

   if ( i == lpMyCont -> m_Active ) {
    lpCodecCtx -> m_Channels   = lInfo.m_nChannels;
    lpCodecCtx -> m_SampleRate = lInfo.m_SampleRate;
    lpCodecCtx -> m_BitRate    = lInfo.m_BitRate;
   }  /* end if */

   lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 4 );

  } else retVal = 0;

 } else lpMyCont -> m_fInit = 0;

 if ( retVal ) {

  if ( i != lpMyCont -> m_Active || lfDoInit ) {

   SMS_RingBuffer* lpBuf = lpMyCont -> m_pSubCont -> m_pStm[ 0 ] -> m_pPktBuf;

   apCont -> m_pStm[ 0 ] = NULL;

   lpMyCont -> m_pSubCont -> m_pStm[ 0 ] -> m_pPktBuf = NULL;
   lpMyCont -> m_pSubCont -> m_pStm[ 0 ] -> m_pCodec  = NULL;
   lpMyCont -> m_pSubCont -> m_pFileCtx               = NULL;

   lpMyCont -> m_pSubCont -> Destroy ( lpMyCont -> m_pSubCont, 1 );
   lpMyCont -> m_pSubCont = SMS_GetContainer ( lpFileCtx, lpMyCont -> m_ContID[ i ] );

   RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );

   if ( lpMyCont -> m_pSubCont ) {

    SMS_Stream* lpStm = lpMyCont -> m_pSubCont -> m_pStm[ 0 ];

    apCont -> m_pStm[ 0 ]   = lpStm;
    apCont -> m_DefPackSize = lpMyCont -> m_pSubCont -> m_DefPackSize;
    apCont -> m_DefPackIdx  = lpMyCont -> m_pSubCont -> m_DefPackIdx;

    lpStm -> m_pPktBuf = lpBuf;

   } else {

    lpFileCtx -> Destroy ( lpFileCtx );
    SMS_CodecDestroy ( lpCodecCtx );
    SMS_RingBufferDestroy ( lpBuf );
    retVal = 0;
    goto end;

   }  /* end else */

  }  /* end else */

  retVal = lpMyCont -> m_pSubCont -> ReadPacket ( lpMyCont -> m_pSubCont, apIdx );

  (  ( SMS_AVPacket* )( apCont -> m_pStm[ 0 ] -> m_pPktBuf -> m_pPtr )  ) -> m_PTS = SMS_STPTS_VALUE;

  if ( i != lpMyCont -> m_Active || lfDoInit ) {
   (  ( SMS_AVPacket* )( apCont -> m_pStm[ 0 ] -> m_pPktBuf -> m_pPtr )  ) -> m_DTS    = ( int64_t )( unsigned int )apCont -> m_pStm[ 0 ] -> m_pCodec;
   (  ( SMS_AVPacket* )( apCont -> m_pStm[ 0 ] -> m_pPktBuf -> m_pPtr )  ) -> m_Flags |= SMS_PKT_FLAG_NWC;
   lpMyCont -> m_Active = i;
  }  /* end if */

  apCont -> m_Duration = MYENTR( apCont -> m_pPlayItem ) -> m_Duration;
  apCont -> ReadPacket = _ReadPacket;

 }  /* end if */
end:
 return retVal;

}  /* end _ReadFirstPacket */

static int _SelectFile ( SMS_Container* apCont, _M3UContainer* apMyCont ) {

 FileContext* ( *lpOpen ) ( const char*, void* ) = apCont -> m_pFileCtx -> Open;
 void*           lpOpenParam                     = apCont -> m_pFileCtx -> m_pOpenParam;
 int             retVal    = 0;
 SMS_Container*  lpSubCont = apMyCont -> m_pSubCont;

 lpSubCont -> m_pFileCtx -> Destroy ( lpSubCont -> m_pFileCtx );
 lpSubCont -> m_pFileCtx = _open_file ( apCont, apCont -> m_pPlayItem, lpOpen, lpOpenParam );

 if ( lpSubCont -> m_pFileCtx ) {

  apCont -> ReadPacket = _ReadFirstPacket;
  apCont -> m_pFileCtx = lpSubCont -> m_pFileCtx;

  retVal = 1;

 }  /* end if */

 return retVal;

}  /* end _SelectFile */

static void _Destroy ( SMS_Container* apCont, int afAll ) {

 _M3UContainer* lpCont = MYCONT( apCont );

 if ( lpCont -> m_pSubCont ) {

  lpCont -> m_pSubCont -> Destroy ( lpCont -> m_pSubCont, 1 );
  apCont -> m_pStm[ 0 ] = NULL;
  apCont -> m_pFileCtx  = NULL;

 }  /* end if */

 _m3u_destroy_list ( apCont -> m_pPlayList );
 apCont -> m_pPlayList = NULL;

 SMS_DestroyContainer ( apCont, 1 );

}  /* end _Destroy */

static int _Seek ( SMS_Container* apCont, int anIdx, int aDir, uint32_t aPos ) {

 FileContext* lpFileCtx = MYCONT( apCont ) -> m_pSubCont -> m_pFileCtx;

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 0 );

 apCont -> m_pPlayItem = ( SMS_ListNode* )aPos;

 MYCONT( apCont ) -> m_Active = -1;

 return _SelectFile (  apCont, MYCONT( apCont )  );

}  /* end _Seek */

int SMS_GetContainerM3U ( SMS_Container* apCont ) {

 char         lBuf  [ 512 ];
 char         lTitle[ 512 ];
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 int          retVal    = 0;
 SMS_List*    lpList    = SMS_ListInit ();
 int          lLen;
 char*        lpPtr;
 _M3UEntry*   lpEntry;
 FileContext* lpSubFileCtx;

 if (  ( int )lpFileCtx < 0  ) {

  SMS_List*     lpFileList = ( SMS_List* )(  ( unsigned int )lpFileCtx & 0x7FFFFFFF  );
  SMS_ListNode* lpNode     = lpFileList -> m_pHead;

  while ( lpNode ) {

   lLen = strlen (  _STR( lpNode )  );

   lpEntry = ( _M3UEntry* )malloc (  sizeof ( _M3UEntry )  );
   lpEntry -> m_pPath    = ( char* )malloc ( lLen + 1 );
   lpEntry -> m_Duration = 0;

   strcpy (  lpEntry -> m_pPath, _STR( lpNode )  );
   _STR( lpNode )[ lLen - 4 ] = '\x00';

   if (  _STR( lpNode )[ lLen - 5 ] == '.'  ) _STR( lpNode )[ lLen - 5 ] = '\x00';

   SMS_ListPushBack (  lpList, _STR( lpNode )  ) -> m_Param = ( unsigned int )lpEntry;

   lpNode = lpNode -> m_pNext;

  }  /* end while */

  retVal = 1;

 } else {

  char         lfExtInf  = 0;
  unsigned int lDuration = 0;

  while ( 1 ) {

   File_GetString ( lpFileCtx, lBuf, M3U_MAXLEN );

   if ( !lBuf[ 0 ] ) {

    if (  FILE_EOF( lpFileCtx )  ) break;

    continue;

   } if ( lBuf[ 0 ] == '#' ) {

    if (  !strcmp ( &lBuf[ 1 ], &g_pExtM3UStr[ 1 ] )  ) {

     continue;

    } else if (  !strncmp ( &lBuf[ 1 ], &g_pExtInfStr[ 1 ], 6 )  ) {

     if (  lBuf[ 7 ] == ':' && sscanf ( &lBuf[ 8 ], g_pPercDStr, &lDuration ) &&
           (  lpPtr = strchr ( &lBuf[ 8 ], ',' )  )
     ) {
      strcpy ( lTitle, lpPtr + 1 );
      lfExtInf = 1;
      continue;
     }  /* end if */

    }  /* end if */

    lfExtInf  = 0;
    lDuration = 0;
    continue;

   }  /* end if */

   if (  !_check_name ( lBuf )  ) break;

   lpEntry = ( _M3UEntry* )malloc (  sizeof ( _M3UEntry )  );
   lpEntry -> m_pPath    = ( char* )malloc (  strlen ( lBuf ) + 1  );
   lpEntry -> m_Duration = lDuration;

   lpPtr = lpEntry -> m_pPath;
   strcpy ( lpEntry -> m_pPath, lBuf );

   while ( lpPtr[ 0 ] ) {
    if ( lpPtr[ 0 ] == '\\' ) lpPtr[ 0 ] = '/';
    ++lpPtr;
   }  /* end while */

   if ( !lfExtInf ) {
    char* lpDot = strrchr ( lpEntry -> m_pPath, '.' );
    if ( lpDot ) lpDot[ 0 ] = '\x00';
    lpPtr = lpEntry -> m_pPath + strlen ( lpEntry -> m_pPath ) - 1;
    while ( lpPtr >= lpEntry -> m_pPath ) {
     if ( lpPtr[ 0 ] == '/' ) {
      ++lpPtr;
      break;
     }  /* end if */
     --lpPtr;
    }  /* end while */
    if ( lpPtr < lpEntry -> m_pPath ) lpPtr = lpEntry -> m_pPath;
    strcpy ( lTitle, lpPtr );
    if ( lpDot ) lpDot[ 0 ] = '.';
   }  /* end else */

   SMS_ListPushBack ( lpList, lTitle ) -> m_Param = ( unsigned int )lpEntry;

   lfExtInf = 0;

  }  /* end while */

  retVal = 1;

 }  /* end else */

 if ( !retVal || !lpList -> m_Size ) {

  if ( lpList )_m3u_destroy_list ( lpList );

  retVal = 0;

 } else {

  _M3UContainer*  lpCont;
  SMS_Container*  lpSubCont;
  FileContext* ( *lpOpen ) ( const char*, void* );
  void*           lpOpenParam;

  retVal = 0;

  if ( g_Config.m_PlayerFlags & SMS_PF_RAND ) {

   SMS_List*    lpNewList = SMS_ListInit ();
   unsigned int lSize     = lpList -> m_Size;

   g_RandSeed = ( int )g_Timer;

   while ( lSize ) {

    unsigned int  lIdx   = SMS_rand () % lSize;
    SMS_ListNode* lpNode = SMS_ListAt ( lpList, lIdx );

    if (   !lpNode || SMS_ListFind (  lpNewList, _STR( lpNode )  )   ) continue;

    SMS_ListPushBack (  lpNewList, _STR( lpNode )  ) -> m_Param = lpNode -> m_Param;
    SMS_ListRemove ( lpList, lpNode );

    --lSize;

   }  /* end while */

   SMS_ListDestroy ( lpList, 1 );
   lpList = lpNewList;

  }  /* end if */

  lpCont = ( _M3UContainer* )(   apCont -> m_pCtx = calloc (  1, sizeof ( _M3UContainer )  )   );

  if ( g_CMedia == 1 && g_pCDDACtx ) {

   lpOpen      = CDDA_InitFileContext;
   lpOpenParam = g_pCDDACtx;

  } else {

   lpOpen      = STIO_InitFileContext;
   lpOpenParam = NULL;

  }  /* end else */

  if (  ( int )apCont -> m_pFileCtx > 0  ) apCont -> m_pFileCtx -> Destroy ( apCont -> m_pFileCtx );

  apCont -> m_pFileCtx = NULL;
  apCont -> m_pName    = g_pM3UStr;
  apCont -> ReadPacket = _ReadFirstPacket;
  apCont -> Destroy    = _Destroy;

  apCont -> m_pPlayList = lpList;
  apCont -> m_pPlayItem = lpList -> m_pHead;
  apCont -> Seek        = _Seek;

  lpSubFileCtx = _open_file ( apCont, lpList -> m_pHead, lpOpen, lpOpenParam );

  if ( lpSubFileCtx ) {

   int i;

   lpCont -> m_ContID[ SMS_SUBCONTAINER_M4A  ] = SMS_CONTAINER_M4A;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_OGG  ] = SMS_CONTAINER_OGG;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_ASF  ] = SMS_CONTAINER_ASF;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_FLAC ] = SMS_CONTAINER_FLAC;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_AAC  ] = SMS_CONTAINER_AAC;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_AC3  ] = SMS_CONTAINER_AC3;
   lpCont -> m_ContID[ SMS_SUBCONTAINER_MP3  ] = SMS_CONTAINER_MP3;
   lpCont -> Probe   [ SMS_SUBCONTAINER_M4A  ] = SMS_M4AProbe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_OGG  ] = SMS_OGGVProbe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_ASF  ] = SMS_WMAProbe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_FLAC ] = SMS_FLACProbe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_AAC  ] = SMS_AACProbe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_AC3  ] = SMS_AC3Probe;
   lpCont -> Probe   [ SMS_SUBCONTAINER_MP3  ] = SMS_MP3Probe;

   for (  i = 0; i < sizeof ( lpCont -> Probe ) / sizeof ( lpCont -> Probe[ 0 ]  ); ++i ) {
    if (   (  lpSubCont = SMS_GetContainer ( lpSubFileCtx, lpCont -> m_ContID[ i ] )  )   ) {
     lpCont -> m_Active = i;
     break;
    }  /* end if */
    lpSubFileCtx -> Seek ( lpSubFileCtx, 0 );
   }  /* end for */

   if ( lpSubCont ) {

    apCont -> m_pFileCtx    = lpSubFileCtx;
    lpCont -> m_pSubCont    = lpSubCont;
    apCont -> m_pStm[ 0 ]   = lpSubCont -> m_pStm[ 0 ];
    apCont -> m_nStm        = 1;
    apCont -> m_DefPackSize = lpSubCont -> m_DefPackSize;
    apCont -> m_DefPackIdx  = lpSubCont -> m_DefPackIdx;
    apCont -> m_Duration    = MYENTR(  apCont -> m_pPlayList -> m_pHead ) -> m_Duration;
    apCont -> m_Flags      |= SMS_CONT_FLAGS_SEEKABLE;

    lpCont -> m_fInit = 1;

    retVal = 1;

   } else lpSubFileCtx -> Destroy ( lpSubFileCtx );

  }  /* end if */

  if ( !retVal ) {

   free ( apCont -> m_pCtx );
   _m3u_destroy_list ( lpList );
   apCont -> m_pName = NULL;

  }  /* end if */

 }  /* end else */

 return retVal;

}  /* end SMS_GetContainerM3U */
