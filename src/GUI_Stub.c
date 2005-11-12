#include "GS.h"
#include "GS.h"
#include "SMS_Codec.h"
#include "SMS_MPEG4.h"
#include "SMS_VideoBuffer.h"
#include "IPU.h"
#include "Config.h"

#include <stdio.h>

void GUIStub_DrawBackground ( GSContext* apGSCtx ) {

 if ( g_Config.m_BrowserFlags & SMS_BF_SKIN ) {

  SMS_CodecContext lpCodecCtx; 
  IPUContext*      lpIPUCtx = IPU_InitContext ( apGSCtx, 640, 512 );
  int              retVal   = 0;

  lpCodecCtx.m_Width     = 640; 
  lpCodecCtx.m_Height    = 512; 
  lpCodecCtx.m_Type      = SMS_CodecTypeVideo;
  lpCodecCtx.m_ID        = SMS_CodecID_MPEG4;
  lpCodecCtx.m_IntBufCnt = 0; 
  lpCodecCtx.m_pIntBuf   = NULL;
  lpCodecCtx.m_HurryUp   = 0;
	
  if (  SMS_CodecOpen ( &lpCodecCtx )  ) { 

   if (  lpCodecCtx.m_pCodec -> Init ( &lpCodecCtx )  ) { 

    FILE* lpFile = fopen ( "mc0:SMS/skin.sms", "r" ); 

    if ( lpFile != NULL ) { 

     long  lSize; 
     void* lpData; 

     fseek ( lpFile, 0, SEEK_END ); 
     lSize = ftell ( lpFile ); 
     fseek ( lpFile, 0, SEEK_SET ); 

     lpData = malloc ( lSize ); 

     if ( lpData ) { 

      SMS_FrameBuffer* lpFrame = NULL; 
     
      fread ( lpData, 1, lSize, lpFile ); 

      lpCodecCtx.m_pCodec -> Decode ( 
       &lpCodecCtx, ( void** )&lpFrame, lpData, lSize 
      );

      if ( !lpFrame ) lpFrame = g_MPEGCtx.m_CurPic.m_pBuf;

      if ( lpFrame ) {

       lpIPUCtx -> Display ( lpFrame );
       lpIPUCtx -> Sync ();

       retVal = 1;

      }  /* end if */

      free ( lpData ); 

     }  /* end if */ 

     fclose ( lpFile ); 

    }  /* end if */ 

    lpCodecCtx.m_pCodec -> Destroy ( &lpCodecCtx ); 

   }  /* end if */ 

   SMS_CodecClose ( &lpCodecCtx ); 

  }  /* end if */ 

  lpIPUCtx -> Destroy (); 

 }  /* end if */

}  /* end GUIStub_DrawBackground */
