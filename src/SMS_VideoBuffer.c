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
#include "SMS_VideoBuffer.h"

#include <malloc.h>

#define SMS_VIDEO_BUFFER_SIZE 15

#define SMS_RB_SLOT( rb, slot )  (  ( slot ) == ( rb ).m_pEnd ? ( rb ).m_pBeg : ( slot )  )
#define SMS_RB_PUSHSLOT( rb )    (  ( rb ).m_pInp  )
#define SMS_RB_POPSLOT( rb )     (  ( rb ).m_pOut  )
#define SMS_RB_PUSHADVANCE( rb ) (   ( rb ).m_pInp = SMS_RB_SLOT(  ( rb ), ( rb ).m_pInp + 1  )   )
#define SMS_RB_POPADVANCE( rb )  (   ( rb ).m_pOut = SMS_RB_SLOT(  ( rb ), ( rb ).m_pOut + 1  )   )

static SMS_FrameBuffer s_FrameBuffer[ SMS_VIDEO_BUFFER_SIZE ];
static SMS_VideoBuffer s_VideoBuffer;

SMS_FrameBuffer* _sms_video_buffer_alloc ( void ) {

 SMS_FrameBuffer* retVal;

 retVal = SMS_RB_POPSLOT( s_VideoBuffer );
 SMS_RB_POPADVANCE( s_VideoBuffer );

 return retVal;

}  /* end _sms_video_buffer_alloc */

static void _sms_video_buffer_release ( SMS_FrameBuffer* apBuf ) {

 SMS_RB_PUSHSLOT( s_VideoBuffer ) = apBuf;
 SMS_RB_PUSHADVANCE( s_VideoBuffer );

}  /* end _sms_video_buffer_release */

static void _sms_video_buffer_destroy ( void ) {

 int i;

 for ( i = 0; i < SMS_VIDEO_BUFFER_SIZE; ++i ) free ( s_FrameBuffer[ i ].m_pBase );

}  /* end _sms_video_buffer_destroy */

SMS_VideoBuffer* SMS_InitVideoBuffer ( int aWidth, int aHeight ) {

 uint32_t lLinesize;
 uint32_t lHeight = (  ( aHeight + 32 + 15 ) >> 4  ) + SMS_Linesize ( aWidth, &lLinesize );

 s_VideoBuffer.m_Linesize = lLinesize;
 s_VideoBuffer.m_pBeg     =
 s_VideoBuffer.m_pOut     =  s_FrameBuffer;
 s_VideoBuffer.m_pEnd     = &s_FrameBuffer[ SMS_VIDEO_BUFFER_SIZE ];
 s_VideoBuffer.m_pInp     =  s_VideoBuffer.m_pEnd - 1;
 s_VideoBuffer.m_nAlloc   = 0;

 for ( lLinesize = 0; lLinesize < SMS_VIDEO_BUFFER_SIZE; ++lLinesize ) {

  s_FrameBuffer[ lLinesize ].m_pBase = ( SMS_MacroBlock* )calloc (  s_VideoBuffer.m_Linesize * lHeight, sizeof ( SMS_MacroBlock )  );
  s_FrameBuffer[ lLinesize ].m_pData = s_FrameBuffer[ lLinesize ].m_pBase + s_VideoBuffer.m_Linesize + 1;
  s_FrameBuffer[ lLinesize ].m_pBuf  = &s_VideoBuffer;

 }  /* end for */

 s_VideoBuffer.Alloc   = _sms_video_buffer_alloc;
 s_VideoBuffer.Release = _sms_video_buffer_release;
 s_VideoBuffer.Destroy = _sms_video_buffer_destroy;

 return &s_VideoBuffer;

}  /* end SMS_InitVideoBuffer */
