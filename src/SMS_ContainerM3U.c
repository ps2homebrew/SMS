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
#include "SMS_List.h"
#include "SMS_Locale.h"
#include "SMS_FileDir.h"
#include "SMS_Sounds.h"

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

 SMS_Container* m_pMP3Cont;

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

static SMS_Container* _open_file ( SMS_Container* apCont, SMS_ListNode* apNode, FileContext* ( *apOpen ) ( const char*, void* ), void* apOpenParam  ) {

 char           lFileName[ 1024 ];
 SMS_Container* retVal = NULL;
 FileContext*   lpFileCtx;

 strcpy ( lFileName, g_CWD );
 SMS_Strcat (  lFileName, g_SlashStr                   );
 SMS_Strcat (  lFileName, MYENTR( apNode ) -> m_pPath  );

 lpFileCtx            = apOpen ( lFileName, apOpenParam );
 apCont -> m_pFileCtx = lpFileCtx;

 if ( lpFileCtx ) retVal = SMS_GetContainer ( lpFileCtx );

 return retVal;

}  /* end _open_file */

static int _ReadPacket ( SMS_AVPacket* apPkt ) {

 int            retVal   = -1;
 SMS_Container* lpCont   = ( SMS_Container* )apPkt -> m_pCtx;
 _M3UContainer* lpMyCont = MYCONT( lpCont );

 if (   (  retVal = lpMyCont -> m_pMP3Cont -> ReadPacket ( apPkt )  ) < 0   ) {

  if ( lpCont -> m_pPlayItem -> m_pNext ) {

   lpCont -> m_pPlayItem = lpCont -> m_pPlayItem -> m_pNext;

   if (  _SelectFile ( lpCont, lpMyCont )  ) retVal = lpCont -> ReadPacket ( apPkt );

  }  /* end if */

 }  /* end if */

 return retVal;

}  /* end _ReadPacket */

static int _ReadFirstPacket ( SMS_AVPacket* apPkt ) {

 int            retVal   = 0;
 SMS_Container* lpCont   = ( SMS_Container* )apPkt -> m_pCtx;
 _M3UContainer* lpMyCont = MYCONT( lpCont );

 retVal = lpMyCont -> m_pMP3Cont -> ReadPacket ( apPkt );

 apPkt -> m_PTS       = SMS_STPTS_VALUE;
 lpCont -> m_Duration = MYENTR( lpCont -> m_pPlayItem ) -> m_Duration;
 lpCont -> ReadPacket = _ReadPacket;

 return retVal;

}  /* end _ReadFirstPacket */

static int _SelectFile ( SMS_Container* apCont, _M3UContainer* apMyCont ) {

 FileContext*   ( *lpOpen ) ( const char*, void* ) = apCont -> m_pFileCtx -> Open;
 void*             lpOpenParam                     = apCont -> m_pFileCtx -> m_pOpenParam;
 SMS_CodecContext* lpCodec = apCont -> m_pStm[ 0 ] -> m_pCodec;
 int               retVal = 0;

 apCont -> m_pStm[ 0 ] -> m_pCodec = NULL;

 apMyCont -> m_pMP3Cont -> Destroy ( apMyCont -> m_pMP3Cont );
 apMyCont -> m_pMP3Cont = _open_file ( apCont, apCont -> m_pPlayItem, lpOpen, lpOpenParam );

 if ( apMyCont -> m_pMP3Cont ) {

  SMS_Stream*  lpStm     = apMyCont -> m_pMP3Cont -> m_pStm[ 0 ];
  FileContext* lpFileCtx = apMyCont -> m_pMP3Cont -> m_pFileCtx;

  lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, lpFileCtx -> m_StreamSize );

  apCont -> m_pStm[ 0 ] = lpStm;
  apCont -> m_nStm      = 1;
  apCont -> ReadPacket  = _ReadFirstPacket;

  lpStm -> m_pCodec -> m_pCodec = lpCodec -> m_pCodec;

  retVal = 1;

 } else {

  apCont -> m_pStm[ 0 ] = NULL;
  lpCodec -> m_pCodec -> Destroy ( lpCodec );
  free ( lpCodec -> m_pCodec );
  SMS_CodecClose ( lpCodec );
  free ( lpCodec );

 }  /* end else */

 return retVal;

}  /* end _SelectFile */

static void _Destroy ( SMS_Container* apCont ) {

 _M3UContainer* lpCont = MYCONT( apCont );

 if ( lpCont -> m_pMP3Cont ) {

  lpCont -> m_pMP3Cont -> Destroy ( lpCont -> m_pMP3Cont );
  apCont -> m_pStm[ 0 ] = NULL;
  apCont -> m_pFileCtx  = NULL;

 }  /* end if */

 _m3u_destroy_list ( apCont -> m_pPlayList );
 apCont -> m_pPlayList = NULL;

 SMS_DestroyContainer ( apCont );

}  /* end _Destroy */

static int _Seek ( SMS_Container* apCont, int anIdx, int aDir, uint32_t aPos ) {

 FileContext* lpFileCtx = MYCONT( apCont ) -> m_pMP3Cont -> m_pFileCtx;

 lpFileCtx -> Stream ( lpFileCtx, lpFileCtx -> m_CurPos, 0 );

 apCont -> m_pPlayItem = ( SMS_ListNode* )aPos;

 return _SelectFile (  apCont, MYCONT( apCont )  );

}  /* end _Seek */

int SMS_GetContainerM3U ( SMS_Container* apCont ) {

 char         lBuf  [ 512 ];
 char         lTitle[ 512 ];
 FileContext* lpFileCtx = apCont -> m_pFileCtx;
 int          retVal    = 0;
 SMS_List*    lpList    = NULL;
 _M3UEntry*   lpEntry;

 if (  ( int )lpFileCtx < 0  ) {

  SMS_List*     lpFileList = ( SMS_List* )(  ( unsigned int )lpFileCtx & 0x7FFFFFFF  );
  SMS_ListNode* lpNode     = lpFileList -> m_pHead;

  lpList = SMS_ListInit ();

  while ( lpNode ) {

   int lLen = strlen ( lpNode -> m_pString );

   lpEntry = ( _M3UEntry* )malloc (  sizeof ( _M3UEntry )  );
   lpEntry -> m_pPath    = ( char* )malloc ( lLen + 1 );
   lpEntry -> m_Duration = 0;

   strcpy ( lpEntry -> m_pPath, lpNode -> m_pString );
   lpNode -> m_pString[ lLen - 4 ] = '\x00';

   SMS_ListPushBack ( lpList, lpNode -> m_pString ) -> m_Param = ( unsigned int )lpEntry;

   lpNode = lpNode -> m_pNext;

  }  /* end while */

  retVal = 1;

  goto start;

 }  /* end if */

 File_GetString ( lpFileCtx, lBuf, M3U_MAXLEN );

 if (  !strcmp ( lBuf, g_pExtM3UStr )  ) {

  lpList = SMS_ListInit ();

  while ( 1 ) {

   char*        lpPtr = &lBuf[ 7 ];
   char*        lpPath;
   unsigned int lDuration;

   File_GetString ( lpFileCtx, lBuf, M3U_MAXLEN );

   if ( lBuf[ 0 ] == '\x00' ) {

    if (  FILE_EOF( lpFileCtx )  ) {

     retVal = 1;
     break;

    }  /* end if */

    continue;

   }  /* end if */

   if (  strncmp ( lBuf, g_pExtInfStr, 7 ) || *lpPtr++ != ':'  ) break;
   if (  !sscanf ( lpPtr, g_pPercDStr, &lDuration )            ) break;

   while ( *lpPtr && *lpPtr != ',' ) ++lpPtr;

   if ( *lpPtr++ != ',' ) break;

   if (  !strlen ( lpPtr )  ) break;

   strcpy ( lTitle, lpPtr );

   File_GetString ( lpFileCtx, lBuf, M3U_MAXLEN );

   if ( lBuf[ 0 ] == '\x00' ) break;

   lpPath = ( char* )malloc (  strlen ( lBuf ) + 1  );
   strcpy ( lpPath, lBuf );

   lpEntry = ( _M3UEntry* )malloc (  sizeof ( _M3UEntry )  );
   lpEntry -> m_Duration = lDuration;
   lpEntry -> m_pPath    = lpPath;

   SMS_ListPushBack ( lpList, lTitle ) -> m_Param = ( unsigned int )lpEntry;

  }  /* end while */

  if ( !retVal || !lpList -> m_Size ) {

   _m3u_destroy_list ( lpList );
   retVal = 0;

  } else {

   _M3UContainer*  lpCont;
   SMS_Container*  lpSubCont;
   FileContext* ( *lpOpen ) ( const char*, void* );
   void*           lpOpenParam;
start:
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

   if (   !(  lpSubCont = _open_file ( apCont, lpList -> m_pHead, lpOpen, lpOpenParam )  )   ) {

    free ( apCont -> m_pCtx );
    _m3u_destroy_list ( lpList );
    apCont -> m_pName = NULL;

    retVal = 0;

   } else {

    lpCont -> m_pMP3Cont  = lpSubCont;
    apCont -> m_pStm[ 0 ] = lpSubCont -> m_pStm[ 0 ];
    apCont -> m_nStm      = 1;
    apCont -> m_Duration  = MYENTR(  apCont -> m_pPlayList -> m_pHead ) -> m_Duration;
    apCont -> m_Flags    |= SMS_CONT_FLAGS_SEEKABLE;

   }  /* end else */

  }  /* end else */

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerM3U */
