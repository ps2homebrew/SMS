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
#include "SMS_Container.h"
#include "SMS_ContainerAVI.h"
#include "SMS_ContainerMP3.h"
#include "SMS_ContainerM3U.h"
#include "StringList.h"

#include <malloc.h>

typedef int ( *ContainerCreator ) ( SMS_Container* );

static ContainerCreator s_CCreator[] = {
 SMS_GetContainerAVI,
 SMS_GetContainerMP3,
 SMS_GetContainerM3U, NULL
};

static void _DestroyPacket ( SMS_AVPacket* apPkt ) {

 if ( apPkt ) {

  if ( apPkt -> m_pData ) free ( apPkt -> m_pData );

  free ( apPkt );

 }  /* end if */

}  /* end _DestroyPacket */

static void _AllocPacket ( SMS_AVPacket* apPkt, int aSize ) {

 void* lpData = SMS_Realloc ( apPkt -> m_pData, &apPkt -> m_AllocSize, aSize + 8 );

 apPkt -> m_PTS      = SMS_NOPTS_VALUE;
 apPkt -> m_DTS      = SMS_NOPTS_VALUE;
 apPkt -> m_StmIdx   = 0;
 apPkt -> m_Flags    = 0;
 apPkt -> m_Duration = 0;
 apPkt -> m_pData    = lpData; 
 apPkt -> m_Size     = aSize;

}  /* end _AllocPacket */

static SMS_AVPacket* _NewPacket ( SMS_Container* apCont ) {

 SMS_AVPacket* retVal = calloc (  1, sizeof ( SMS_AVPacket )  );

 retVal -> m_pCtx  = apCont;
 retVal -> Destroy = _DestroyPacket;
 retVal -> Alloc   = _AllocPacket;

 return retVal;

}  /* end _NewPacket */

void SMS_DestroyContainer ( SMS_Container* apCont ) {

 uint32_t i;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  if ( !apCont -> m_pStm[ i ] ) continue;

  if ( apCont -> m_pStm[ i ] -> Destroy ) apCont -> m_pStm[ i ] -> Destroy ( apCont -> m_pStm[ i ] );

  if ( apCont -> m_pStm[ i ] -> m_pCodec ) {

   if ( apCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec ) {

    apCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec -> Destroy ( apCont -> m_pStm[ i ] -> m_pCodec );
    free ( apCont -> m_pStm[ i ] -> m_pCodec -> m_pCodec );

   }  /* end if */

   SMS_CodecClose ( apCont -> m_pStm[ i ] -> m_pCodec );

   free ( apCont -> m_pStm[ i ] -> m_pCodec );

  }  /* end if */

  if ( apCont -> m_pStm[ i ] -> m_pName ) free ( apCont -> m_pStm[ i ] -> m_pName );

  free ( apCont -> m_pStm[ i ] );

 }  /* end for */

 if ( apCont -> m_pFileCtx  ) apCont -> m_pFileCtx -> Destroy ( apCont -> m_pFileCtx );
 if ( apCont -> m_pCtx      ) free ( apCont -> m_pCtx );
 if ( apCont -> m_pPlayList ) apCont -> m_pPlayList -> Destroy ( apCont -> m_pPlayList, 1 );

 free ( apCont );

}  /* end _Destroy */

SMS_Container* SMS_GetContainer ( FileContext* apFileCtx, struct GUIContext* apGUICtx ) {

 int            i      = 0;
 SMS_Container* retVal = ( SMS_Container* )calloc (  1, sizeof ( SMS_Container )  );

 retVal -> m_pFileCtx = apFileCtx;
 retVal -> m_pGUICtx  = apGUICtx;

 while ( s_CCreator[ i ] ) {

  if (  s_CCreator[ i++ ] ( retVal )  ) break;

  apFileCtx -> Seek ( apFileCtx, 0 );

 }  /* end while */

 if ( retVal -> m_pName == NULL ) {

  apFileCtx -> Destroy ( apFileCtx );

  free ( retVal );
  retVal = NULL;

 } else {

  if ( !retVal -> NewPacket ) retVal -> NewPacket = _NewPacket;
  if ( !retVal -> Destroy   ) retVal -> Destroy   = SMS_DestroyContainer;

 }  /* end else */

 return retVal;

}  /* end SMS_GetContainer */

void SMSContainer_SetPTSInfo ( SMS_Stream* apStm, int aPTSNum, int aPTSDen ) {

 apStm -> m_TimeBase.m_Num = aPTSNum;
 apStm -> m_TimeBase.m_Den = aPTSDen;

}  /* end SMSContainer_SetPTSInfo */
