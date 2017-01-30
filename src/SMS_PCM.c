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
#include "SMS_PCM.h"

#include <malloc.h>
#include <string.h>

extern void* memcpy_swap16 ( void*, const void*, unsigned int );

static int32_t PCM_Init    ( SMS_CodecContext*                                   );
static int32_t PCM_Decode  ( SMS_CodecContext*, SMS_RingBuffer*, SMS_RingBuffer* );
static void    PCM_Destroy ( SMS_CodecContext*                                   );

static void* ( *_Decode ) ( void*, const void*, unsigned int );

void SMS_Codec_PCM_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "pcm16";
 apCtx -> m_pCodec -> Init    = PCM_Init;
 apCtx -> m_pCodec -> Decode  = PCM_Decode;
 apCtx -> m_pCodec -> Destroy = PCM_Destroy;

 if ( apCtx -> m_ID == SMS_CodecID_PCM16LE )
  _Decode = memcpy;
 else if ( apCtx -> m_ID == SMS_CodecID_PCM16BE )
  _Decode = memcpy_swap16;

}  /* end SMS_Codec_PCM_Open */

static int32_t PCM_Init ( SMS_CodecContext* apCtx ) {

 return 0;

}  /* end PCM_Init */

static void PCM_Destroy ( SMS_CodecContext* apCtx ) {


}  /* end PCM_Destroy */

static int32_t PCM_Decode ( SMS_CodecContext* apCtx, SMS_RingBuffer* apOutput, SMS_RingBuffer* apInput ) {

 SMS_AVPacket* lpPkt    = ( SMS_AVPacket* )apInput -> m_pOut;
 uint8_t*      lpBuf    = lpPkt -> m_pData;
 int32_t       lBufSize = lpPkt -> m_Size;
 int           lnParts  = lBufSize / 4096;
 int           lRem     = lBufSize % 4096;
 short*        lpSamples;
 int           i;
 void*         ( *_decode ) ( void*, const void*, unsigned int ) = _Decode;

 for ( i = 0; i < lnParts; ++i ) {

  lpSamples  = ( short* )SMS_RingBufferAlloc ( apOutput, 4096 + 80 );
  lpSamples += 32;
  *( int* )lpSamples = 4096;
  lpSamples += 8;

  _decode ( lpSamples, lpBuf, 4096 );
  apOutput -> UserCB ( apOutput );

  lpBuf += 4096;

  RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );

 }  /* end for */

 if ( lRem ) {

  lpSamples  = ( short* )SMS_RingBufferAlloc ( apOutput, lRem + 80 );
  lpSamples += 32;
  *( int* )lpSamples = lRem;
  lpSamples += 8;

  _decode ( lpSamples, lpBuf, lRem );
  apOutput -> UserCB ( apOutput );

  RotateThreadReadyQueue ( SMS_THREAD_PRIORITY );

 }  /* end if */

 return 0;

}  /* end PCM_Decode */

