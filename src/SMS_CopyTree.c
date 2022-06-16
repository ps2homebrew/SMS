/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_DirTree.h"
#include "SMS_FileDir.h"
#include "SMS_GS.h"
#include "SMS_GUI.h"
#include "SMS_Locale.h"
#include "SMS_PAD.h"
#include "SMS_RC.h"
#include "SMS_List.h"
#include "SMS_FileContext.h"
#include "SMS_Timer.h"
#include "SMS_PgInd.h"
#include "SMS_GUIClock.h"
#include "SMS_ioctl.h"
#include "SMS_IOP.h"

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE ( 4096 * 96 )

int SMS_CopyFile ( const char*, FileContext*, u64          , u64*          , void* );

typedef struct SrcDst {
 char*         m_pSrc;
 char*         m_pDst;
 int           m_SrcLen;
 int           m_DstLen;
 u64           m_TotalSize;
 u64           m_DataSize;
 u64           m_CumSize;
 void*         m_pBuf;
} SrcDst;

static int DirCB ( const char* apName ) {

 char* lpSts = malloc (  strlen ( apName ) + strlen ( STR_READING_FMT.m_pStr ) + 1  );

 if ( lpSts ) {

  sprintf ( lpSts, STR_READING_FMT.m_pStr, apName );
  GS_VSync ();
  GUI_Status ( lpSts );
  free ( lpSts );

 }  /* end if */

 return 0;

}  /* end DirCB */

static int BrkCB ( void ) {

 int lBtn   = GUI_ReadButtons ();
 int retVal = ( lBtn == SMS_PAD_TRIANGLE || lBtn == RC_STOP ) ? 2 : 0;

 if ( retVal ) while (  GUI_ReadButtons ()  );

 return retVal;

}  /* end BrkCB */

static void CalcSizeCB ( SMS_DirTree* apTree, const char* apPath, int afDir, unsigned aSize ) {
 if ( !afDir ) {
  (  ( SrcDst* )apTree -> m_pUserData  ) -> m_DataSize  += aSize;
  aSize = ( aSize + 511 ) & ~511;
  (  ( SrcDst* )apTree -> m_pUserData  ) -> m_TotalSize += aSize;
 }  /* end if */
}  /* end CalcSizeCB */

static char* _make_path ( SrcDst* apSrcDst, const char* apPath ) {

 int   lLen   = strlen ( apPath ) - apSrcDst -> m_SrcLen;
 char* retVal = ( char* )malloc (  apSrcDst -> m_DstLen + lLen + 1  );

 if ( retVal ) {

  strcpy ( retVal, apSrcDst -> m_pDst );
  apPath += apSrcDst -> m_SrcLen;
  strcpy ( retVal + apSrcDst -> m_DstLen, apPath );

 }  /* end if */

 return retVal;

}  /* end _make_path */

static void MakeTreeCB ( SMS_DirTree* apTree, const char* apPath, int afDir, unsigned aSize ) {

 if ( afDir ) {

  char* lpDir = _make_path (  ( SrcDst* )apTree -> m_pUserData, apPath  );

  if ( lpDir ) {

   int lSts = fioMkdir ( lpDir );

   free ( lpDir );

   if ( lSts && lSts != -EEXIST )
    apTree -> m_Error = 1;
   else apTree -> m_Error = BrkCB ();

  } else apTree -> m_Error = 1;

 }  /* end if */

}  /* end MakeTreeCB */

static void CopyFileCB ( SMS_DirTree* apTree, const char* apPath, int afDir, unsigned aSize ) {

 if ( !afDir ) {

  SrcDst* lpSrcDst = ( SrcDst* )apTree -> m_pUserData;
  char*   lpPath   = _make_path ( lpSrcDst, apPath );

  if ( lpPath ) {

   FileContext* lpFileCtx = STIO_InitFileContext ( apPath, 0 );

   if ( lpFileCtx )
    apTree -> m_Error = SMS_CopyFile (
     lpPath, lpFileCtx, lpSrcDst -> m_DataSize, &lpSrcDst -> m_CumSize, lpSrcDst -> m_pBuf
    );

   free ( lpPath );

  } else apTree -> m_Error = 1;

 }  /* end if */

}  /* end CopyFileCB */

void SMS_CopyTree ( const char* apSrc ) {

 int           lLen;
 u64           lZoneFree;
 u64           lZoneSize;
 SMS_DirTree*  lpTree = SMS_DirTreeInit ( apSrc );
 char*         lpDir  = strrchr ( apSrc, '/' ) + 1;
 SrcDst        lSrcDst; memset (  &lSrcDst, 0, sizeof ( lSrcDst )  );

 lpTree -> DirCB       = DirCB;
 lpTree -> BrkCB       = BrkCB;
 lpTree -> m_pUserData = &lSrcDst;

 SMS_DirTreeScan ( lpTree, NULL, "" );

 if ( !lpTree -> m_Error ) {

  lSrcDst.m_SrcLen = strlen ( apSrc );
  lSrcDst.m_pSrc   = ( char* )malloc ( lSrcDst.m_SrcLen + 2 );

  if ( !lSrcDst.m_pSrc ) goto error;

  strcpy ( lSrcDst.m_pSrc, apSrc );
  lSrcDst.m_pSrc[ lSrcDst.m_SrcLen     ] = '/';
  lSrcDst.m_pSrc[ lSrcDst.m_SrcLen + 1 ] = '\x00';

  lSrcDst.m_DstLen = (  lLen = strlen ( g_HDDWD )  ) + strlen ( lpDir ) + 1;
  lSrcDst.m_pDst   = ( char* )malloc (  lSrcDst.m_DstLen + 2 );

  if ( !lSrcDst.m_pDst ) goto error;

  strcpy ( lSrcDst.m_pDst, g_HDDWD );

  if ( lSrcDst.m_pDst[ lLen - 1 ] != '/' ) {
   strcat ( lSrcDst.m_pDst, g_SlashStr );
   ++lSrcDst.m_DstLen;
  }  /* end if */

  strcat ( lSrcDst.m_pDst, lpDir      );
  strcat ( lSrcDst.m_pDst, g_SlashStr );

  GUI_Status ( STR_CHECKING_FREE_SPACE.m_pStr );

  SMS_PgIndStart ();
  SMS_DirTreeWalk ( lpTree, NULL, NULL, CalcSizeCB );
  SMS_PgIndStop  ();

  if ( !lpTree -> m_Error ) {

   lZoneFree = SMS_IOCtl ( g_pPFS, PFS_IOCTL_GET_ZONE_FREE, NULL );
   lZoneSize = SMS_IOCtl ( g_pPFS, PFS_IOCTL_GET_ZONE_SIZE, NULL );
   lZoneFree = (  ( lZoneFree * lZoneSize ) + 511  ) & ~511;

   if ( lZoneFree > lSrcDst.m_TotalSize ) {

    GUI_Status ( STR_CREATING_DIR_TREE.m_pStr );
    lLen = fioMkdir ( lSrcDst.m_pDst );

    if ( !lLen || lLen == -EEXIST ) {

     SMS_PgIndStart ();
     SMS_DirTreeWalk ( lpTree, NULL, NULL, MakeTreeCB );
     SMS_PgIndStop  ();

     if ( !lpTree -> m_Error ) {

      lSrcDst.m_pBuf = malloc ( BUF_SIZE );

      if ( !lSrcDst.m_pBuf ) goto error;

      SMS_GUIClockSuspend ();
       SMS_DirTreeWalk ( lpTree, NULL, NULL, CopyFileCB );
      SMS_GUIClockResume  ();

      if ( lpTree -> m_Error == 1 ) goto error;
      if ( lpTree -> m_Error == 3 ) goto error_1;

     } else if ( lpTree -> m_Error == 1 ) goto error;

    } else GUI_Error ( STR_ERROR.m_pStr );

   } else error_1: GUI_Error ( STR_NOT_ENOUGH_SPACE.m_pStr );

  }  /* end if */

 } else if ( lpTree -> m_Error == 1 ) error: GUI_Error ( STR_OUT_OF_MEMORY.m_pStr );

 if ( lSrcDst.m_pBuf ) free ( lSrcDst.m_pBuf );
 if ( lSrcDst.m_pSrc ) free ( lSrcDst.m_pSrc );
 if ( lSrcDst.m_pDst ) free ( lSrcDst.m_pDst );

 SMS_DirTreeDestroy ( lpTree );

}  /* end SMS_CopyTree */

static void DeleteFileCB ( SMS_DirTree* apTree, const char* apPath, int afDir, unsigned aSize ) {

 if ( !afDir )
  apTree -> m_Error = fioRemove ( apPath );
 else SMS_ListPush (  ( SMS_List* )apTree -> m_pUserData, apPath  );

 if ( !apTree -> m_Error ) apTree -> m_Error = BrkCB ();

}  /* end DeleteFileCB */

void SMS_DeleteTree ( const char* apPath ) {

 SMS_DirTree* lpTree = SMS_DirTreeInit ( apPath );
 SMS_List*    lpList = SMS_ListInit ();

 lpTree -> DirCB       = DirCB;
 lpTree -> BrkCB       = BrkCB;
 lpTree -> m_pUserData = lpList;

 SMS_DirTreeScan ( lpTree, NULL, "" );

 GUI_Status ( STR_DELETING.m_pStr );

 if ( !lpTree -> m_Error ) {

  SMS_DirTreeWalk ( lpTree, NULL, NULL, DeleteFileCB );

  if ( !lpTree -> m_Error ) {

   SMS_ListNode* lpNode = lpList -> m_pHead;

   while ( lpNode ) {

    if (  BrkCB ()  ) break;

    if (   fioRmdir (  _STR( lpNode )  )   ) break;

    lpNode = lpNode -> m_pNext;

   }  /* end while */

   if ( !lpNode ) fioRmdir ( apPath );

  }  /* end if */

 }  /* end if */

 SMS_ListDestroy ( lpList, 1 );
 SMS_DirTreeDestroy ( lpTree );

}  /* end SMS_DeleteTree */

//#define _CHECK_AUDIO

#ifdef _CHECK_AUDIO
# include "SMS_SPU.h"
#endif  /* _CHECK_AUDIO */

int SMS_CopyFile (
     const char* apDst, FileContext* apFile, u64           aTotalSize, u64*           apCumSize, void* apBuf
    ) {

 int   retVal = 0;
 int   lFD    = fioOpen ( apDst, O_CREAT | O_WRONLY );
 void* lpBuf  = apBuf;
#ifdef _CHECK_AUDIO
 SPUContext* lpSPU = SPU_InitContext (  2, 48000, SPU_Index2Volume ( 24 ), 1, 1  );
#endif  /* _CHECK_AUDIO */
 if ( lFD >= 0 ) {

  if ( !lpBuf ) lpBuf = malloc ( BUF_SIZE );

  if ( lpBuf ) {

   static char s_Fmt[] __attribute__(   (  section( ".data" )  )   ) = "%s (%.1f%s)...";

   char          lSts[ 128 ];
   unsigned int  lnCopied = 0U;
   u64           lCumSize = apCumSize ? *apCumSize : 0UL;
   float         lSpeed   = 0.0F;
   int           lfError  = 0;
   unsigned int  lStart   = g_Timer;
   float         lTotalSize;
   int           lBtn;

   if ( !aTotalSize ) aTotalSize = apFile -> m_Size;

   lTotalSize = ( float )aTotalSize;

   apFile -> Stream ( apFile, 0, 96 );

   while ( 1 ) {

    int lnWrite;
    int lnRead = apFile -> Read ( apFile, lpBuf, BUF_SIZE );

    if ( lnRead <= 0 ) break;

    if (  fioWrite ( lFD, lpBuf, lnRead ) < 0  ) {
error:
     lfError = 1;
     break;
    }  /* end if */

    lCumSize += lnRead;
    lnCopied += lnRead;
    lSpeed    = ( float )lnCopied / ( float )( g_Timer - lStart );

    sprintf ( lSts, s_Fmt, STR_COPYING.m_pStr, lSpeed, STR_KBS.m_pStr );
    GUI_Progress (   lSts, ( int )(  ( float )lCumSize / lTotalSize * 100.0F  ), 1   );
    lBtn = GUI_ReadButtons ();
    lBtn = ( lBtn == SMS_PAD_TRIANGLE || lBtn == RC_STOP ) ? 2 : 0;

    if ( lBtn ) {
     retVal = lBtn;
     GUI_Status ( STR_STOPPING.m_pStr );
     while (  GUI_ReadButtons ()  ); break;
    }  /* end if */

    fioSync ( FIO_WAIT, &lnWrite );

    if ( lnWrite != lnRead ) goto error;

   }  /* end while */

   if ( !apBuf ) free ( lpBuf );

   if ( lfError )
    retVal = 3;
   else if ( apCumSize ) *apCumSize = lCumSize;

  } else retVal = 1;

  fioClose ( lFD );

 } else retVal = -1;
#ifdef _CHECK_AUDIO
 lpSPU -> Destroy ();
#endif  /* _CHECK_AUDIO */
 apFile -> Destroy ( apFile );

 return retVal;

}  /* end SMS_CopyFile */
