/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_Container.h"
#include "SMS_ContainerJPG.h"
#include "SMS_List.h"
#include "SMS_JPEG.h"
#include "SMS_Locale.h"
#include "SMS_FileDir.h"
#include "SMS_PgInd.h"
#include "SMS_GUIClock.h"

#include <malloc.h>

static void _Destroy ( SMS_Container* apCont, int afAll ) {

 SMS_ContainerJPEG* lpSubCont = ( SMS_ContainerJPEG* )apCont -> m_pCtx;

 if ( lpSubCont ) {

  SMS_JPEGViewerDestroy ( lpSubCont -> m_pViewer );
  if (  ( int )lpSubCont -> m_pFileList > 0  ) SMS_ListDestroy ( lpSubCont -> m_pFileList, 1 );

 }  /* end if */

 SMS_DestroyContainer ( apCont, 1 );

}  /* end _Destroy */

int SMS_GetContainerJPG ( SMS_Container* apCont ) {

 SMS_List*    lpList    = NULL;
 int          retVal    = 0;
 FileContext* lpFileCtx = apCont -> m_pFileCtx;

 if (  ( int )lpFileCtx < 0  ) {

  lpList = ( SMS_List* )(  ( unsigned int )lpFileCtx & 0x7FFFFFFF  );

  if (   SMS_ContID (  _STR( lpList -> m_pHead )  ) == SMS_CONTAINER_JPG   ) {
   lpList = ( SMS_List* )lpFileCtx;
   retVal = 1;
  }  /* end if */

 } else if (  lpFileCtx && File_GetUShort ( lpFileCtx ) == 0xD8FF  ) {

  lpList = SMS_ListInit ();
  SMS_ListPushBack ( lpList, lpFileCtx -> m_pPath );
  lpFileCtx -> Destroy ( lpFileCtx );

  retVal = 1;

 }  /* end if */

 if ( retVal ) {

  SMS_ContainerJPEG* lpSubCont = ( SMS_ContainerJPEG* )calloc (  1, sizeof ( SMS_ContainerJPEG )  );

  apCont -> m_pFileCtx = NULL;

  if ( g_CMedia == 1 && g_pCDDACtx ) {

   lpSubCont -> m_pOpen      = CDDA_InitFileContext;
   lpSubCont -> m_pOpenParam = g_pCDDACtx;

  } else {

   lpSubCont -> m_pOpen      = STIO_InitFileContext;
   lpSubCont -> m_pOpenParam = NULL;

  }  /* end else */

  SMS_PgIndStop    ();
  SMS_GUIClockStop ();

  lpSubCont -> m_pFileList = lpList;
  lpSubCont -> m_pViewer   = SMS_JPEGViewerInit ();

  apCont -> m_pCtx  = lpSubCont;
  apCont -> m_pName = g_pJPEG;
  apCont -> Destroy = _Destroy;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainerJPG */
