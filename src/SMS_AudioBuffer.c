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
#include "SMS.h"
#include "SMS_AudioBuffer.h"

#include <kernel.h>

extern unsigned char g_DataBuffer[ SMS_DATA_BUFFER_SIZE ];

static SMS_AudioBuffer s_AudioBuffer;
static int             s_Sema;

static unsigned char* _sms_audio_buffer_alloc ( int aSize ) {

 unsigned char* lpPtr;
 int            lSize;

 lSize = ( aSize + 71 ) & 0xFFFFFFC0;
 lpPtr = s_AudioBuffer.m_pInp + lSize;

 if (  SMS_RB_EMPTY( s_AudioBuffer )  ) goto ret;

 while ( 1 ) {

  if ( s_AudioBuffer.m_pInp > s_AudioBuffer.m_pOut )

   if ( lpPtr < s_AudioBuffer.m_pEnd ) {
ret:
    *( int* )s_AudioBuffer.m_pInp             = aSize;
    *( int* )( s_AudioBuffer.m_pInp + lSize ) = -1;

    return s_AudioBuffer.m_pInp + 4;

   } else {

    s_AudioBuffer.m_pInp = s_AudioBuffer.m_pBeg;
    lpPtr                = s_AudioBuffer.m_pBeg + lSize;

   }  /* end else */

  else if ( lpPtr < s_AudioBuffer.m_pOut )

   goto ret;

  else {

   s_AudioBuffer.m_fWait = 1;
   WaitSema ( s_Sema );

  }  /* end else */

 }  /* end while */

}  /* end _sms_audio_buffer_alloc */

int _sms_audio_buffer_release ( void ) {

 int lSize = *( int* )s_AudioBuffer.m_pOut;

 SMS_ADV_ABOUT( &s_AudioBuffer, lSize );

 if (  *( int* )s_AudioBuffer.m_pOut == -1  ) {

  if (  SMS_RB_EMPTY( s_AudioBuffer )  ) return 1;

  s_AudioBuffer.m_pOut = s_AudioBuffer.m_pBeg;

 }  /* end if */

 if ( s_AudioBuffer.m_fWait ) {

  s_AudioBuffer.m_fWait = 0;
  SignalSema ( s_Sema );

 }  /* end if */

 return 0;

}  /* end _sms_audio_buffer_release */

static void _sms_audio_buffer_destroy ( void ) {

 DeleteSema ( s_Sema );

}  /* end _sms_audio_buffer_destroy */

SMS_AudioBuffer* SMS_InitAudioBuffer ( void ) {

 ee_sema_t lSema;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_Sema = CreateSema ( &lSema );

 s_AudioBuffer.m_pInp  =
 s_AudioBuffer.m_pOut  =
 s_AudioBuffer.m_pBeg  = UNCACHED_SEG( g_DataBuffer                          );
 s_AudioBuffer.m_pEnd  = UNCACHED_SEG( &g_DataBuffer[ SMS_DATA_BUFFER_SIZE ] );
 s_AudioBuffer.m_Len   = 0;
 s_AudioBuffer.m_pPos  = NULL;
 s_AudioBuffer.m_fWait = 0;

 s_AudioBuffer.Alloc   = _sms_audio_buffer_alloc;
 s_AudioBuffer.Release = _sms_audio_buffer_release;
 s_AudioBuffer.Destroy = _sms_audio_buffer_destroy;

 return &s_AudioBuffer;

}  /* end SMS_InitAudioBuffer */
