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
#include "SMS_ContainerMPEG_PS.h"
#include "SMS_ContainerMP3.h"
#include "SMS_ContainerM3U.h"
#include "SMS_List.h"

#include <malloc.h>
#include <string.h>

typedef int ( *ContainerCreator ) ( SMS_Container* );

static ContainerCreator s_CCreator[] = {
 SMS_GetContainerAVI,
 SMS_GetContainerMPEG_PS,
 SMS_GetContainerMP3,
 SMS_GetContainerM3U, NULL
};

static SMS_AVPacket* _AllocPacket ( SMS_RingBuffer* apRB, int aSize ) {

 static SMS_AVPacket s_EmptyPacket;

 SMS_AVPacket* lpPkt = aSize ? SMS_RingBufferAlloc ( apRB, aSize + 64 ) : &s_EmptyPacket;

 if ( lpPkt ) {

  lpPkt -> m_PTS      = SMS_NOPTS_VALUE;
  lpPkt -> m_DTS      = SMS_NOPTS_VALUE;
  lpPkt -> m_StmIdx   = 0;
  lpPkt -> m_Flags    = 0;
  lpPkt -> m_Duration = 0;
  lpPkt -> m_pData    = (  ( unsigned char* )lpPkt  ) + 64;
  lpPkt -> m_Size     = aSize;

 }  /* end if */

 return lpPkt;

}  /* end _AllocPacket */

void SMS_DestroyContainer ( SMS_Container* apCont, int afAll ) {

 uint32_t i;

 for ( i = 0; i < apCont -> m_nStm; ++i ) {

  SMS_Stream* lpStm = apCont -> m_pStm[ i ];

  if ( !lpStm ) continue;

  apCont -> m_pStm[ i ] = NULL;

  if ( lpStm -> Destroy ) lpStm -> Destroy ( lpStm );

  if ( lpStm -> m_pCodec ) {

   if ( lpStm -> m_pCodec -> m_pCodec ) {

    lpStm -> m_pCodec -> m_pCodec -> Destroy ( lpStm -> m_pCodec );
    free ( lpStm -> m_pCodec -> m_pCodec );

   }  /* end if */

   SMS_CodecClose ( lpStm -> m_pCodec );

   free ( lpStm -> m_pCodec );

  }  /* end if */

  if ( lpStm -> m_pName   ) free ( lpStm -> m_pName );
  if ( lpStm -> m_pPktBuf ) SMS_RingBufferDestroy ( lpStm -> m_pPktBuf );

  free ( lpStm );

 }  /* end for */

 if ( apCont -> m_pCtx ) {
  free ( apCont -> m_pCtx );
  apCont -> m_pCtx = NULL;
 }  /* end if */

 if ( apCont -> m_pPlayList ) {
  SMS_ListDestroy ( apCont -> m_pPlayList, 1 );
  apCont -> m_pPlayList = NULL;
 }  /* end if */

 if ( afAll ) {

  if ( apCont -> m_pFileCtx  ) apCont -> m_pFileCtx -> Destroy ( apCont -> m_pFileCtx );

  free ( apCont );

 }  /* end if */

}  /* end SMS_DestroyContainer */

SMS_Container* SMS_GetContainer ( FileContext* apFileCtx, int aPrefCont ) {

 int            i      = 0;
 int            lSts   = 0;
 SMS_Container* retVal = ( SMS_Container* )calloc (  1, sizeof ( SMS_Container )  );

 retVal -> m_pFileCtx  = apFileCtx;
 retVal -> AllocPacket = _AllocPacket;

 if ( aPrefCont < 0 )

  while ( s_CCreator[ i ] ) {

   if (   (  lSts = s_CCreator[ i++ ] ( retVal )  )   ) break;

   if (  ( int )apFileCtx > 0  ) apFileCtx -> Seek ( apFileCtx, 0 );

  }  /* end while */

 else s_CCreator[ aPrefCont ] ( retVal );

 if ( retVal -> m_pName == NULL ) {

  if (  ( int )apFileCtx > 0  ) apFileCtx -> Destroy ( apFileCtx );

  free ( retVal );
  retVal = NULL;

 } else if ( !retVal -> Destroy ) retVal -> Destroy = SMS_DestroyContainer;

 if ( retVal && lSts < 0 ) {

  retVal -> Destroy ( retVal, 1 );
  retVal = NULL;

 }  /* end if */

 return retVal;

}  /* end SMS_GetContainer */

void SMSContainer_SetPTSInfo ( SMS_Stream* apStm, int aPTSNum, int aPTSDen ) {

 apStm -> m_TimeBase.m_Num = aPTSNum;
 apStm -> m_TimeBase.m_Den = aPTSDen;

}  /* end SMSContainer_SetPTSInfo */

void SMSContainer_CalcPktFields ( SMS_Stream* apStm, SMS_AVPacket* apPkt ) {

 int               lNum, lDen;
 SMS_CodecContext* lpCodecCtx = apStm -> m_pCodec;

 if ( lpCodecCtx -> m_Type == SMS_CodecTypeVideo ) {

  lNum = lpCodecCtx -> m_FrameRateBase;
  lDen = lpCodecCtx -> m_FrameRate;

 } else if ( lpCodecCtx -> m_Type == SMS_CodecTypeAudio ) {

  lNum = lpCodecCtx -> m_FrameSize <= 1 ? ( apPkt -> m_Size * 8 * lpCodecCtx -> m_SampleRate ) / lpCodecCtx -> m_BitRate
                                        : lpCodecCtx -> m_FrameSize;
  lDen = lpCodecCtx -> m_SampleRate;

 } else return;

 if ( lDen && lNum )

  apPkt -> m_Duration = ( int )SMS_Rescale (
   1, lNum * ( int64_t )apStm -> m_TimeBase.m_Den, lDen * ( int64_t )apStm -> m_TimeBase.m_Num
  );

 if ( apPkt -> m_PTS == SMS_NOPTS_VALUE ) {

  if ( apPkt -> m_DTS == SMS_NOPTS_VALUE ) {

   apPkt -> m_PTS = apStm -> m_CurDTS;
   apPkt -> m_DTS = apStm -> m_CurDTS;

  } else {

   apStm -> m_CurDTS = apPkt -> m_DTS;
   apPkt -> m_PTS    = apPkt -> m_DTS;

  }  /* end else */

 } else {

  apStm -> m_CurDTS = apPkt -> m_PTS;
  apPkt -> m_DTS    = apPkt -> m_PTS;

 }  /* end else */

 apStm -> m_CurDTS += apPkt -> m_Duration;

 if ( apPkt -> m_PTS != SMS_NOPTS_VALUE )
  apPkt -> m_PTS = SMS_Rescale (  apPkt -> m_PTS, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den  );

 if ( apPkt -> m_DTS != SMS_NOPTS_VALUE )
  apPkt -> m_DTS = SMS_Rescale ( apPkt -> m_DTS, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den );

 apPkt -> m_Duration = ( int )SMS_Rescale (  apPkt -> m_Duration, SMS_TIME_BASE * ( int64_t )apStm -> m_TimeBase.m_Num, apStm -> m_TimeBase.m_Den  );

}  /* end SMSContainer_CalcPktFields */
