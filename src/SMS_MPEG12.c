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
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_IPU.h"
#include "SMS_VideoBuffer.h"
#include "libmpeg.h"

#include <stdio.h>
#include <kernel.h>
#include <string.h>

typedef struct MPEGState {

 SMS_FrameBuffer   m_Pics[ 6 ];
 int64_t           m_PTS;
 int64_t           m_PrevPTS;
 int64_t           m_StreamPTS;
 int               m_MSPerFrame;
 SMS_RingBuffer*   m_pInput;
 SMS_AVPacket*     m_pPacket;
 MPEGSequenceInfo* m_pInfo;
 int               m_Width;
 int               m_Height;
 int               m_PicIdx;

} MPEGState;

static int32_t MPEG12_Init    ( SMS_CodecContext*                                   );
static int32_t MPEG12_Decode  ( SMS_CodecContext*, SMS_RingBuffer*, SMS_RingBuffer* );
static void    MPEG12_Destroy ( SMS_CodecContext*                                   );

static void* _init_cb ( void*, MPEGSequenceInfo* );
static int   _set_dma ( void*                    );

static MPEGState s_MPEGState;

void SMS_Codec_MPEG12_Open ( SMS_CodecContext* apCtx ) {

 apCtx -> m_pCodec = calloc (  1, sizeof ( SMS_Codec )  );

 apCtx -> m_pCodec -> m_pName = "mpeg12";
 apCtx -> m_pCodec -> Init    = MPEG12_Init;
 apCtx -> m_pCodec -> Decode  = MPEG12_Decode;
 apCtx -> m_pCodec -> Destroy = MPEG12_Destroy;
 apCtx -> m_Flags            |= SMS_CODEC_FLAG_NOCSC | SMS_CODEC_FLAG_UNCACHED | SMS_CODEC_FLAG_IPU;

 IPU_FRST ();
 MPEG_Initialize ( _set_dma, &s_MPEGState, _init_cb, &s_MPEGState, &s_MPEGState.m_StreamPTS );
 memset (  &s_MPEGState, 0, sizeof ( s_MPEGState )  );

}  /* end SMS_Codec_MPEG12_Open */

static int32_t MPEG12_Init ( SMS_CodecContext* apCtx ) {

 s_MPEGState.m_PTS     = SMS_NOPTS_VALUE;
 s_MPEGState.m_PrevPTS = SMS_NOPTS_VALUE;

 return 1;

}  /* end MPEG12_Init */

static void _delete_frames ( void ) {

 int i;

 for (  i = 0; i < sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] ); ++i  )
  if ( s_MPEGState.m_Pics[ i ].m_pBase ) {
   free ( s_MPEGState.m_Pics[ i ].m_pBase );
   s_MPEGState.m_Pics[ i ].m_pBase = NULL;
  }  /* end for */

}  /* end _delete_frames */

static void MPEG12_Destroy ( SMS_CodecContext* apCtx ) {

 MPEG_Destroy ();
 IPU_FRST ();
 ResetEE ( 0x40 );
 _delete_frames ();

}  /* end MPEG12_Destroy */

static void* _init_cb ( void* apParam, MPEGSequenceInfo* apSeqInfo ) {

 MPEGState* lpState = ( MPEGState* )apParam;
 int        lWidth  = apSeqInfo -> m_Width;
 int        lHeight = apSeqInfo -> m_Height;

 if ( lpState -> m_Width != lWidth || lpState -> m_Height != lHeight ) {

  unsigned int   i, lSize = ( lWidth * lHeight ) << 2;
  unsigned int   lMBW = lWidth  >> 4;
  unsigned int   lMBH = lHeight >> 4;
  unsigned long* lpDMA;

  lpState -> m_pInfo  = apSeqInfo;
  lpState -> m_Width  = lWidth;
  lpState -> m_Height = lHeight;

  _delete_frames ();

  for (  i = 0; i < sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] ); ++i  ) {

   int            lX, lY;
   unsigned char* lpPic = ( unsigned char* )memalign (   64, lSize + (  ( 8 + 12 * lMBW * lMBH ) << 3  )   );

   lpState -> m_Pics[ i ].m_pBase = ( SMS_MacroBlock* )lpPic;
   lpState -> m_Pics[ i ].m_pData = ( SMS_MacroBlock* )(  lpDMA = ( unsigned long* )( lpPic + lSize )  );

   lpDMA[ 0 ] = DMA_TAG( 3, 0, DMATAG_ID_CNT, 0, 0, 0 );
   lpDMA[ 1 ] = 0LL;
   lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
   lpDMA[ 3 ] = GIFTAG_REGS_AD;
   lpDMA[ 4 ] = GS_SET_TRXREG( 16, 16 );
   lpDMA[ 5 ] = GS_TRXREG;
   lpDMA[ 6 ] = GS_SET_BITBLTBUF( 0, 0, GSPixelFormat_PSMCT32, g_IPUCtx.m_VRAM, g_IPUCtx.m_TBW, GSPixelFormat_PSMCT32 );
   lpDMA[ 7 ] = GS_BITBLTBUF;
   lpDMA     += 8;

   for ( lY = 0; lY < lHeight; lY += 16 )
    for ( lX = 0; lX < lWidth; lX += 16 ) {
     lpDMA[ 0 ] = DMA_TAG( 4, 0, DMATAG_ID_CNT, 0, 0, 0 );
     lpDMA[ 1 ] = 0LL;
      lpDMA[ 2 ] = GIF_TAG( 2, 0, 0, 0, 0, 1 );
      lpDMA[ 3 ] = GIFTAG_REGS_AD;
      lpDMA[ 4 ] = GS_SET_TRXPOS( 0, 0, lX, lY, GS_TRXPOS_DIR_LR_UD );
      lpDMA[ 5 ] = GS_TRXPOS;
      lpDMA[ 6 ] = GS_SET_TRXDIR( GS_TRXDIR_HOST_TO_LOCAL );
      lpDMA[ 7 ] = GS_TRXDIR;
      lpDMA[ 8 ] = GIF_TAG( 64, 0, 0, 0, 2, 0 );
      lpDMA[ 9 ] = 0LL;
     lpDMA[ 10 ] = DMA_TAG(  64, 0, DMATAG_ID_REF, 0, ( unsigned int )lpPic, 0  );
     lpDMA[ 11 ] = 0LL;
     lpDMA +=   12;
     lpPic += 1024;
    }  /* end for */

   lpDMA[ -4 ] = GIF_TAG( 64, 1, 0, 0, 2, 0 );
   lpDMA[ -2 ] = DMA_TAG(  64, 0, DMATAG_ID_REFE, 0, ( unsigned int )( lpPic - 1024 ), 0  );

  }  /* end for */

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
 retVal  = MPEG_Picture ( lpFrame -> m_pBase, &lPTS );

 if ( retVal ) {

  SMS_FrameBuffer** lppOutput = ( SMS_FrameBuffer** )SMS_RingBufferAlloc ( apOutput, 4 );

  *lppOutput = lpFrame;

  if ( s_MPEGState.m_PrevPTS == lPTS )
   lPTS = ( s_MPEGState.m_PTS += s_MPEGState.m_pInfo -> m_MSPerFrame );
  else s_MPEGState.m_PrevPTS = s_MPEGState.m_PTS = lPTS;

  lpFrame -> m_PTS  =
  lpFrame -> m_SPTS = lPTS;

  if (  ++lPicIdx == sizeof ( s_MPEGState.m_Pics ) / sizeof ( s_MPEGState.m_Pics[ 0 ] )  ) lPicIdx = 0;

  s_MPEGState.m_PicIdx = lPicIdx;

  SMS_RingBufferPost ( apOutput );

 }  /* end if */

 return retVal;

}  /* end MPEG12_Decode */
