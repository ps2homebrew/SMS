///////////////////////////////////////////////////////////////////////////////
//
// debug stuff
//
//
//

#include "debug.h"
#include "main.h"   // not very nice

#ifdef WIN32
#include <windows.h>
#endif

DebugStream cdbg;

//////////////////////////////////////////////////////////////////////////////
// DebugStream::DebugStream
//////////////////////////////////////////////////////////////////////////////

DebugStream::DebugStream()
{
  memset( m_aBuffer, 0, 1024 ); 
}

//////////////////////////////////////////////////////////////////////////////
// DebugStream::flush
//////////////////////////////////////////////////////////////////////////////

DebugStream& DebugStream::flush() 
{ 
  // not very nice

  VUFrame* p_frame = VUemu::GetVuFrame();

  if( p_frame != NULL )
  {
    if( p_frame->txtDebug != NULL )
    {
      p_frame->txtDebug->AppendText( m_aBuffer );
    }
    else
    {
#ifdef WIN32
      OutputDebugString( m_aBuffer ); 
#else
      printf( m_aBuffer ); 
#endif
    }
  }

  memset( m_aBuffer, 0, 1024 ); 
  return (*this); 
}
