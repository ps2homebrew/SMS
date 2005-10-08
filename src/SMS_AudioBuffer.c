# include "SMS.h"
# include "SMS_AudioBuffer.h"

static SMS_AudioBuffer s_AudioBuffer;

#ifndef _WIN32
# include <kernel.h>
#else
# include <stdio.h>
# define UNCACHED_SEG( a ) ( a )
#endif  /* _WIN32 */

extern unsigned char g_DataBuffer[ SMS_DATA_BUFFER_SIZE ];

static SMS_AudioBuffer s_AudioBuffer;
static int             s_Sema;

#if defined( LOCK_QUEUES ) && !defined( _WIN32 )
static int s_SemaLock;
# define LOCK() WaitSema ( s_SemaLock )
# define UNLOCK() SignalSema ( s_SemaLock )
#else
# define LOCK()
# define UNLOCK()
#endif  /* LOCK_QUEUES */

static unsigned char* _sms_audio_buffer_alloc ( int aSize ) {

 unsigned char* lpPtr;
 int            lSize;

 LOCK(); {

  lSize = ( aSize + 71 ) & 0xFFFFFFC0;
  lpPtr = s_AudioBuffer.m_pInp + lSize;

  if (  SMS_RB_EMPTY( s_AudioBuffer )  ) goto ret;

  while ( 1 ) {

   if ( s_AudioBuffer.m_pInp > s_AudioBuffer.m_pOut )

    if ( lpPtr < s_AudioBuffer.m_pEnd ) {
ret:
     *( int* )s_AudioBuffer.m_pInp             = aSize;
     *( int* )( s_AudioBuffer.m_pInp + lSize ) = -1;

     UNLOCK();

     return s_AudioBuffer.m_pInp + 4;

    } else {

     s_AudioBuffer.m_pInp = s_AudioBuffer.m_pBeg;
     lpPtr                = s_AudioBuffer.m_pBeg + lSize;

    }  /* end else */

   else if ( lpPtr < s_AudioBuffer.m_pOut )

    goto ret;

   else {

    s_AudioBuffer.m_fWait = 1;
#ifndef _WIN32
    UNLOCK();
     WaitSema ( s_Sema );
    LOCK();
#endif  /* _WIN32 */
   }  /* end else */

  }  /* end while */

 } UNLOCK();

}  /* end _sms_audio_buffer_alloc */

int _sms_audio_buffer_release ( void ) {

 LOCK(); {

  int lSize = *( int* )s_AudioBuffer.m_pOut;

  SMS_ADV_ABOUT( &s_AudioBuffer, lSize );

  if (  *( int* )s_AudioBuffer.m_pOut == -1  ) {

   if (  SMS_RB_EMPTY( s_AudioBuffer )  ) {

    UNLOCK();
    return 1;

   }  /* end if */

   s_AudioBuffer.m_pOut = s_AudioBuffer.m_pBeg;

  }  /* end if */

  if ( s_AudioBuffer.m_fWait ) {
#ifndef _WIN32
   s_AudioBuffer.m_fWait = 0;
   SignalSema ( s_Sema );
#endif  /* _WIN32 */
  }  /* end if */

 } UNLOCK();

 return 0;

}  /* end _sms_audio_buffer_release */

static void _sms_audio_buffer_destroy ( void ) {
#ifndef _WIN32
 DeleteSema ( s_Sema );
# ifdef LOCK_QUEUES
 DeleteSema ( s_SemaLock );
# endif  /* LOCK_QUEUES */
#endif  /* _WIN32 */
}  /* end _sms_audio_buffer_destroy */

SMS_AudioBuffer* SMS_InitAudioBuffer ( void ) {
#ifndef _WIN32
 ee_sema_t lSema;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_Sema = CreateSema ( &lSema );
# ifdef LOCK_QUEUES
 lSema.init_count = 1;
 s_SemaLock = CreateSema ( &lSema );
# endif  /* LOCK_QUEUES */
#endif  /* _WIN32 */
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

