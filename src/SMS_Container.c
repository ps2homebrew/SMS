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

#include <malloc.h>

typedef int ( *ContainerCreator ) ( SMS_Container* );

static ContainerCreator s_CCreator[] = {
 SMS_GetContainerAVI, NULL
};

static void _DestroyPacket ( SMS_AVPacket* apPkt ) {

 if ( apPkt != 0 ) {

  free ( apPkt -> m_pData );
  free ( apPkt );

 }  /* end if */

}  /* end _SMS_DestroyPacket */

static SMS_AVPacket* _NewPacket ( SMS_Container* apCont ) {

 SMS_AVPacket* retVal = calloc (  1, sizeof ( SMS_AVPacket )  );

 retVal -> m_pCtx  = apCont;
 retVal -> Destroy = _DestroyPacket;

 return retVal;

}  /* end _NewPacket */

static void _Destroy ( SMS_Container* apCont ) {

 uint32_t i;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  apCont -> m_pStm[ i ] -> Destroy ( apCont -> m_pStm[ i ] );

  if ( apCont -> m_pStm[ i ] -> m_Codec.m_pCodec ) {

   apCont -> m_pStm[ i ] -> m_Codec.m_pCodec -> Destroy ( &apCont -> m_pStm[ i ] -> m_Codec );
   free ( apCont -> m_pStm[ i ] -> m_Codec.m_pCodec );

   SMS_CodecClose ( &apCont -> m_pStm[ i ] -> m_Codec );

  }  /* end if */

  free ( apCont -> m_pStm[ i ] );

 }  /* end for */

 apCont -> m_pFileCtx -> Destroy ( apCont -> m_pFileCtx );

 free ( apCont -> m_pCtx );
 free ( apCont );

}  /* end _Destroy */

SMS_Container* SMS_GetContainer ( FileContext* apFileCtx ) {

 int            i      = 0;
 SMS_Container* retVal = ( SMS_Container* )calloc (  1, sizeof ( SMS_Container )  );

 retVal -> m_pFileCtx = apFileCtx;

 while ( s_CCreator[ i ] )

  if (  s_CCreator[ i++ ] ( retVal )  ) break;

 if ( retVal -> m_pName == NULL ) {

  apFileCtx -> Destroy ( apFileCtx );

  free ( retVal );
  retVal = NULL;

 } else {

  if ( !retVal -> NewPacket ) retVal -> NewPacket = _NewPacket;
  if ( !retVal -> Destroy   ) retVal -> Destroy   = _Destroy;

 }  /* end else */

 return retVal;

}  /* end SMS_GetContainer */
