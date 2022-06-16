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
#include "SMS_MPEG12.h"
#include "SMS_MPEG.h"
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_IPU.h"
#include "SMS_VideoBuffer.h"
#include "SMS_Locale.h"
#include "libmpeg.h"
#include "libmpeg_internal.h"

#include <stdio.h>
#include <kernel.h>
#include <string.h>

typedef struct MPEGState {

 SMS_FrameBuffer   m_Pics[ 3 ];
 int64_t           m_PTS;
 int64_t           m_PrevPTS;
 int64_t           m_StreamPTS;
 SMS_RingBuffer*   m_pInput;
 SMS_AVPacket*     m_pPacket;
 MPEGSequenceInfo* m_pInfo;
 int               m_Width;
 int               m_Height;
 int               m_PicIdx;
 unsigned char     m_f16;

} MPEGState;

static int32_t MPEG12_Init    ( SMS_CodecContext*                                   );
static int32_t MPEG12_Decode  ( SMS_CodecContext*, SMS_RingBuffer*, SMS_RingBuffer* );
static void    MPEG12_Destroy ( SMS_CodecContext*                                   );

static void* _init_cb ( void*, MPEGSequenceInfo* );
static int   _set_dma ( void*                    );

static MPEGState s_MPEGState;

static void _hwctl ( SMS_CodecContext* apCtx, SMS_CodecHWCtl aCtl ) {

 switch ( aCtl ) {

  case SMS_HWC_Init: {

   unsigned char lf16 = g_IPUCtx.m_TexFmt == GSPixelFormat_PSMCT16;

   s_MPEGState.m_f16 = lf16;
   _MPEG_Set16 ( lf16 );

  } break;

  case SMS_HWC_Reset: {

   int i;

   for (  i = 0; i < sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] ); ++i  )
    s_MPEGState.m_Pics[ i ].m_FrameType = -1;

  } break;

  default: break;

 }  /* end switch */

}  /* end _hwctl */

void SMS_Codec_MPEG12_Open ( SMS_CodecContext* apCtx ) {

 memset (  &g_MPEGCtx, 0, sizeof ( g_MPEGCtx )  );

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = g_pMPEG12;
 apCtx -> m_pCodec -> Init    = MPEG12_Init;
 apCtx -> m_pCodec -> Decode  = MPEG12_Decode;
 apCtx -> m_pCodec -> Destroy = MPEG12_Destroy;
 apCtx -> m_Flags            |= SMS_CODEC_FLAG_NOCSC | SMS_CODEC_FLAG_UNCACHED | SMS_CODEC_FLAG_IPU;
 apCtx -> HWCtl               = _hwctl;

 IPU_FRST ();
 MPEG_Initialize ( _set_dma, &s_MPEGState, _init_cb, &s_MPEGState, &s_MPEGState.m_StreamPTS );
 memset (  &s_MPEGState, 0, sizeof ( s_MPEGState )  );

 _hwctl ( apCtx, SMS_HWC_Reset );

}  /* end SMS_Codec_MPEG12_Open */

static int32_t MPEG12_Init ( SMS_CodecContext* apCtx ) {

 s_MPEGState.m_PTS     = SMS_NOPTS_VALUE;
 s_MPEGState.m_PrevPTS = SMS_NOPTS_VALUE;

 return 1;

}  /* end MPEG12_Init */

void SMS_Codec_MPEG12_Reset ( unsigned int aFlags ) {

 if ( aFlags & SMS_MPEG12_RESET_DECODER ) {

  IPU_FRST ();
  s_MPEGState.m_PTS     = SMS_NOPTS_VALUE;
  s_MPEGState.m_PrevPTS = SMS_NOPTS_VALUE;
  MPEG_Reset ( aFlags );

 }  /* end if */

 if ( aFlags & SMS_MPEG12_RESET_QUEUE ) s_MPEGState.m_pPacket = NULL;
 
}  /* end SMS_Codec_MPEG12_Reset */

static void MPEG12_Destroy ( SMS_CodecContext* apCtx ) {

 MPEG_Destroy ();
 IPU_FRST ();
 ResetEE ( 0x40 );
 SMS_FrameBufferDestroy (
  s_MPEGState.m_Pics, sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] )
 );

}  /* end MPEG12_Destroy */

static void* _init_cb ( void* apParam, MPEGSequenceInfo* apSeqInfo ) {

 MPEGState* lpState = ( MPEGState* )apParam;
 int        lWidth  = apSeqInfo -> m_Width;
 int        lHeight = apSeqInfo -> m_Height;

 if ( lpState -> m_Width != lWidth || lpState -> m_Height != lHeight ) {

  lpState -> m_pInfo  = apSeqInfo;
  lpState -> m_Width  = lWidth;
  lpState -> m_Height = lHeight;

  SMS_FrameBufferDestroy (
   s_MPEGState.m_Pics, sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] )
  );
  SMS_FrameBufferInit (
   s_MPEGState.m_Pics, sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] ),
   lWidth, lHeight, s_MPEGState.m_f16
  );

  FlushCache ( 0 );

 }  /* end if */

 return lpState -> m_Pics[ lpState -> m_PicIdx ].m_pBase;

}  /* end _init_cb */

static int _set_dma ( void* apParam ) {

 MPEGState*    lpState = ( MPEGState* )apParam;
 SMS_AVPacket* lpPkt;

 do {

  if ( lpState -> m_pPacket ) SMS_RingBufferFree ( lpState -> m_pInput, lpState -> m_pPacket -> m_Size + 64 );

  lpPkt = lpState -> m_pPacket = SMS_RingBufferWait ( lpState -> m_pInput );

  if ( !lpPkt ) return 0;

 } while ( !lpPkt -> m_Size );

 DMA_SendA (
  DMAC_TO_IPU, ( void* )(  ( unsigned int )lpPkt -> m_pData & 0x1FFFFFFF  ), ( lpPkt -> m_Size + 15 ) >> 4
 );

 if ( lpPkt -> m_PTS > 0 ) lpState -> m_StreamPTS = lpPkt -> m_PTS;

 return 1;

}  /* end _set_dma */

static int32_t MPEG12_Decode ( SMS_CodecContext* apCtx, SMS_RingBuffer* apOutput, SMS_RingBuffer* apInput ) {

 int64_t          lPTS;
 int              lPicIdx;
 SMS_FrameBuffer* lpFrame;
 int              retVal;

 s_MPEGState.m_pInput = apInput;

 lPicIdx = s_MPEGState.m_PicIdx;
 lpFrame = &s_MPEGState.m_Pics[ lPicIdx ];

 retVal = MPEG_Picture ( lpFrame -> m_pBase, &lPTS );

 if ( retVal ) {

  SMS_FrameBuffer** lppOutput = ( SMS_FrameBuffer** )SMS_RingBufferAlloc ( apOutput, 4 );

  *lppOutput = lpFrame;

  lpFrame -> m_FrameType = 0;

  if ( s_MPEGState.m_PrevPTS == lPTS )
   lPTS = ( s_MPEGState.m_PTS += s_MPEGState.m_pInfo -> m_MSPerFrame );
  else s_MPEGState.m_PrevPTS = s_MPEGState.m_PTS = lPTS;

  lpFrame -> m_StartPTS = lPTS;

  if (  ++lPicIdx == sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] )  ) lPicIdx = 0;

  s_MPEGState.m_PicIdx = lPicIdx;

  SMS_RingBufferPost ( apOutput );

 }  /* end if */

 return retVal;

}  /* end MPEG12_Decode */
