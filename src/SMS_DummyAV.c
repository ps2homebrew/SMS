/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_DummyAV.h"
#include "SMS_AudioBuffer.h"
#include "SMS_VideoBuffer.h"

#include <malloc.h>
#include <string.h>

static int32_t DummyAudio_Init    ( SMS_CodecContext*                            );
static int32_t DummyAudio_Decode  ( SMS_CodecContext*, void**, uint8_t*, int32_t );
static void    DummyAudio_Destroy ( SMS_CodecContext*                            );

static int32_t DummyVideo_Init    ( SMS_CodecContext*                            );
static int32_t DummyVideo_Decode  ( SMS_CodecContext*, void**, uint8_t*, int32_t );
static void    DummyVideo_Destroy ( SMS_CodecContext*                            );

static SMS_AudioBuffer* s_pAudioBuffer;
static SMS_Frame        s_Frame;
static int              s_Len;
static int              s_Y;
static int              s_Incr;

void SMS_Codec_DMA_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "dummy_audio";
 apCtx -> m_pCodec -> Init    = DummyAudio_Init;
 apCtx -> m_pCodec -> Decode  = DummyAudio_Decode;
 apCtx -> m_pCodec -> Destroy = DummyAudio_Destroy;

}  /* end SMS_Codec_DMA_Open */

void SMS_Codec_DMV_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "dummy_video";
 apCtx -> m_pCodec -> Init    = DummyVideo_Init;
 apCtx -> m_pCodec -> Decode  = DummyVideo_Decode;
 apCtx -> m_pCodec -> Destroy = DummyVideo_Destroy;

}  /* end SMS_Codec_DMV_Open */

static int32_t DummyAudio_Init ( SMS_CodecContext* apCtx ) {

 s_pAudioBuffer = SMS_InitAudioBuffer ();

 return 1;

}  /* end DummyAudio_Init */

static int32_t DummyAudio_Decode ( SMS_CodecContext* apCtx, void** appData, uint8_t* apBuf, int32_t aBufSize ) {

 SMS_AudioBuffer** lppBuffer = ( SMS_AudioBuffer** )appData;
 short*            lpSamples = ( short* )s_pAudioBuffer -> Alloc ( 16 );

 __asm__ __volatile__ (  "sq $zero, 0(%0)\n\t" :: "r"( lpSamples )  );

 s_pAudioBuffer -> m_Len = 0;

 *lppBuffer = s_pAudioBuffer;

 return 1;

}  /* end DummyAudio_Decode */

static void DummyAudio_Destroy ( SMS_CodecContext* apCtx ) {

 s_pAudioBuffer -> Destroy ();

}  /* end DummyAudio_Destroy */

static int32_t DummyVideo_Init ( SMS_CodecContext* apCtx ) {

 SMS_CodecGetBuffer ( apCtx, &s_Frame );

 s_Y    = 0;
 s_Len  = (   (  ( apCtx -> m_Height + 32 + 15 ) >> 4  ) + 2   ) * s_Frame.m_Linesize;
 s_Incr = 4;

 return 1;

}  /* end DummyVideo_Init */

static int32_t DummyVideo_Decode ( SMS_CodecContext* apCtx, void** appData, uint8_t* apBuf, int32_t aBufSize ) {

 int               i;
 SMS_MacroBlock*   lpMB;
 SMS_FrameBuffer** lpFrame  = ( SMS_FrameBuffer** )appData;

 lpMB = s_Frame.m_pBuf -> m_pBase;

 for ( i = 0; i < s_Len; ++i, ++lpMB ) {

  memset ( lpMB -> m_Y,  s_Y,  256 );
  memset ( lpMB -> m_Cb, 0x80,  64 );
  memset ( lpMB -> m_Cr, 0x80,  64 );

 }  /* end for */

 s_Y += s_Incr;

 if ( s_Y > 128 ) {

  s_Y    = 128;
  s_Incr = -s_Incr;

 } else if ( s_Y < 0 ) {

  s_Y    = 0;
  s_Incr = -s_Incr;

 }  /* end if */

 *lpFrame = s_Frame.m_pBuf;

 return 1;

}  /* end DummyVideo_Decode */

static void DummyVideo_Destroy ( SMS_CodecContext* apCtx ) {

 SMS_CodecReleaseBuffer ( apCtx, &s_Frame );

}  /* end DummyVideo_Destroy */
