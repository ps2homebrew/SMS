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
#include "SMS_Locale.h"

#include <kernel.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>
#include <fileio.h>

#define STRING( n ) (   (  ( char* )( n )  ) + sizeof ( SMS_DirNode )   )

static SMS_DirNode* SMS_DirAdd ( SMS_Dir* apDir, const char* apString ) {

 SMS_DirNode* lpNode = ( SMS_DirNode* )calloc (  1, sizeof ( SMS_DirNode ) + strlen ( apString ) + 1  );

 if ( lpNode ) {

  strcpy (  STRING( lpNode ), apString );

  if ( apDir -> m_pTail ) {

   apDir -> m_pTail -> m_pNext = lpNode;
   apDir -> m_pTail            = lpNode;

  } else apDir -> m_pHead = apDir -> m_pTail = lpNode;

  return apDir -> m_pTail;

 }  /* end if */

 return NULL;

}  /* end SMS_DirAdd */

SMS_DirTree* SMS_DirTreeInit ( const char* apRoot ) {

 int          lLen;
 char         lRoot[ (  lLen = strlen ( apRoot )  ) + 2 ];
 SMS_DirTree* retVal = ( SMS_DirTree* )calloc (  1, sizeof ( SMS_DirTree )  );

 strcpy ( lRoot, apRoot );
 if ( lRoot[ lLen - 1 ] != '/' )
  strcat ( lRoot, g_SlashStr );
 else --lLen;

 apRoot = lRoot + lLen;

 while ( apRoot > lRoot ) if ( *apRoot-- == '/' && *apRoot == '/' ) (  ( char* )apRoot  )[ 1 ] = '\x00';

 SMS_DirAdd ( &retVal -> m_Root, apRoot );

 return retVal;

}  /* end SMS_DirTreeInit */

void SMS_DirTreeScan ( SMS_DirTree* apTree, SMS_Dir* apRoot, const char* apPath ) {

 int   lLen;
 char* lpPath;
 int   lDD;

 if ( apTree -> m_Error ) return;

 if ( !apRoot ) apRoot = &apTree -> m_Root;

 lpPath = ( char* )malloc (
  (  lLen = strlen ( apPath )  ) + strlen (  STRING( apRoot -> m_pHead )  ) + 2
 );

 if ( !lpPath ) {

  apTree -> m_Error = 1;
  return;

 }  /* end if */

 strcpy ( lpPath, apPath );
 if ( lLen && lpPath[ lLen - 1 ] != '/' ) strcat ( lpPath, g_SlashStr );
 strcat (  lpPath, STRING( apRoot -> m_pHead )  );

 lDD = fioDopen ( lpPath );

 if ( lDD >= 0 ) {

  SMS_DirNode* lpNode;
  fio_dirent_t lEntry;

  if ( apTree -> DirCB ) {
   apTree -> m_Error = apTree -> DirCB ( lpPath );
   if ( apTree -> m_Error ) goto error;
  }  /* end if */

  while (  fioDread ( lDD, &lEntry ) > 0  ) {

   u64           lData;
   SMS_DirNode*  lpNode;

   if ( apTree -> BrkCB ) {
    apTree -> m_Error = apTree -> BrkCB ();
    if ( apTree -> m_Error ) goto error;
   }  /* end if */

   if (  ( lEntry.stat.mode & FIO_SO_IFDIR ) &&
         lEntry.name[ 0 ] == '.' && (
          lEntry.name[ 1 ] == '\x00' || (
           lEntry.name[ 1 ] == '.' && lEntry.name[ 2 ] == '\x00'
          )
         )
   ) continue;

   if ( lEntry.stat.mode & FIO_SO_IFREG ) {
    lData = lEntry.stat.size;
    ++apTree -> m_nFiles;
    apTree -> m_Size += lData;
   } else if ( lEntry.stat.mode & FIO_SO_IFDIR ) {
    SMS_Dir* lpList = ( SMS_Dir* )calloc (  1, sizeof ( SMS_Dir )  );
    if (  !lpList || !SMS_DirAdd ( lpList, lEntry.name )  ) {
     apTree -> m_Error = 1;
error:
     fioDclose ( lDD );
     goto end;
    }  /* end if */
    lEntry.name[ 0 ] = '\x00';
    lData = ( u64           )( unsigned )lpList;
    ++apTree -> m_nDirs;
   } else continue;

   if (   !(  lpNode = SMS_DirAdd ( apRoot, lEntry.name )  )   ) {
    apTree -> m_Error = 1;     
    goto error;
   }  /* end if */

   lpNode -> m_Param = lData;

  }  /* end while */

  fioDclose  ( lDD );

  lpNode = apRoot -> m_pHead -> m_pNext;

  while ( lpNode ) {
   if (  !STRING( lpNode )[ 0 ]  ) SMS_DirTreeScan (  apTree, ( SMS_Dir* )( unsigned )lpNode -> m_Param, lpPath  );
   if ( apTree -> m_Error ) goto end;
   lpNode = lpNode -> m_pNext;
  }  /* end while */

 }  /* end if */
end:
 free ( lpPath );

}  /* end SMS_DirTreeScan */

void SMS_DirTreeWalk (
      SMS_DirTree* apTree, SMS_Dir* apRoot,
      const char* apParent, void ( *Callback ) ( SMS_DirTree*, const char*, int, unsigned )
     ) {

 int          lnAlloc;
 int          lRootLen;
 SMS_DirNode* lpNode;
 char*        lpPath;
 const char*  lpStr;

 if ( apTree -> m_Error ) return;

 if ( !apRoot ) {
  apRoot   = &apTree -> m_Root;
  apParent = STRING( apRoot -> m_pHead );
  lpStr    = STRING( apRoot -> m_pHead );
 } else lpStr  = apParent;

 lpNode   = apRoot -> m_pHead -> m_pNext;
 lRootLen = strlen ( lpStr );
 lpPath   = ( char* )malloc ( lnAlloc = lRootLen + 3 );

 if ( !lpPath ) {
  apTree -> m_Error = 1;
  return;
 }  /* end if */

 strcpy ( lpPath, lpStr );

 while ( lpNode ) {

  int   lfDir    = !STRING( lpNode )[ 0 ];
  char* lpName   = lfDir ? STRING(   (  ( SMS_Dir* )( unsigned int )lpNode -> m_Param  ) -> m_pHead   )
                         : STRING( lpNode );
  int   lNameLen = strlen ( lpName );
  int   lNewLen  = lNameLen + lRootLen + 2;
   
  if ( lnAlloc < lNewLen ) {

   char* lpNewPath = ( char* )realloc ( lpPath, lNewLen );

   if ( !lpNewPath ) break;

   lpPath  = lpNewPath;
   lnAlloc = lNewLen;

  }  /* end if */

  strcpy ( lpPath + lRootLen, lpName );

  if ( lfDir ) {
   lpPath[ lNameLen + lRootLen     ] = '/';
   lpPath[ lNameLen + lRootLen + 1 ] = '\x00';
  }  /* end if */

  Callback ( apTree, lpPath, lfDir, lpNode -> m_Param );

  if ( apTree -> m_Error ) break;

  if ( lfDir ) SMS_DirTreeWalk (  apTree, (  ( SMS_Dir* )( unsigned int )lpNode -> m_Param  ), lpPath, Callback  );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 free ( lpPath );

}  /* end SMS_DirTreeWalk */

static void SMS_DirDestroy ( SMS_Dir* apDir ) {

 SMS_DirNode* lpNode = apDir -> m_pHead;

 while ( lpNode ) {

  SMS_DirNode* lpNext = lpNode -> m_pNext;

  free ( lpNode );

  lpNode = lpNext;

 }  /* end while */

 free ( apDir );

}  /* end SMS_DirDestroy */

void SMS_DirTreeDestroy ( void* apRoot ) {

 SMS_DirNode* lpNode = (  ( SMS_Dir* )apRoot  ) -> m_pHead;

 while ( lpNode ) {

  if (  !STRING( lpNode )[ 0 ]  ) SMS_DirTreeDestroy (  ( void* )( unsigned )lpNode -> m_Param  );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 SMS_DirDestroy (  ( SMS_Dir* )apRoot  );

}  /* end SMS_DirTreeDestroy */
