///////////////////////////////////////////////////////////////////////////////
//
// debug stuff
//
//
//

#ifndef __DEBUGB_H
#define __DEBUGB_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////////////////////////////////
// class DebugStream
//////////////////////////////////////////////////////////////////////////////

class DebugStream
{
	public :
								DebugStream();
  virtual			 ~DebugStream() {}
	        			DebugStream& flush();
	inline				DebugStream& operator<<(DebugStream& (DebugStream&));
	inline				DebugStream& operator<<( const unsigned char *);
	inline				DebugStream& operator<<( const signed char *);
	inline				DebugStream& operator<<( const char *);
	inline				DebugStream& operator<<( char *);
	inline				DebugStream& operator<<( char );
	inline				DebugStream& operator<<( unsigned char );
	inline				DebugStream& operator<<( signed char );
	inline				DebugStream& operator<<( short );
	inline				DebugStream& operator<<( unsigned short );
	inline				DebugStream& operator<<( int );
	inline				DebugStream& operator<<( unsigned int );
	inline				DebugStream& operator<<( long );
	inline				DebugStream& operator<<( unsigned long );
	inline				DebugStream& operator<<( float );
	inline				DebugStream& operator<<( double );
	inline				DebugStream& operator<<( long double );
								
	private :

	char					m_aBuffer[1024];
};

DebugStream& DebugStream::operator<<( const unsigned char* p) { char a[256]; a[0] = 0; sprintf( a,"%s", p ); strcat( m_aBuffer, a );  return (*this); }
DebugStream& DebugStream::operator<<( const signed char* p ) { char a[256]; a[0] = 0; sprintf( a,"%s", p ); strcat( m_aBuffer, a );  return (*this); }
DebugStream& DebugStream::operator<<( const char* p ) { char a[256]; a[0] = 0; sprintf( a,"%s", p ); strcat( m_aBuffer, a );  return (*this); }
DebugStream& DebugStream::operator<<( char * p ) { char a[256]; a[0] = 0; sprintf( a,"%s", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( char p ) { char a[256]; a[0] = 0; sprintf( a,"%c", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( unsigned char p ) { char a[256]; a[0] = 0; sprintf( a,"%c", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( signed char p ) { char a[256]; a[0] = 0; sprintf( a,"%c", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( short p ) { char a[256]; a[0] = 0; sprintf( a,"%d", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( unsigned short p ) { char a[256]; a[0] = 0; sprintf( a,"%d", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( int p ) { char a[256]; a[0] = 0; sprintf( a,"%d", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( unsigned int p ) { char a[256]; a[0] = 0; sprintf( a,"%d", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( long p ) { char a[256]; a[0] = 0; sprintf( a,"%ld", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( unsigned long p ) { char a[256]; a[0] = 0; sprintf( a,"%lu", p ); strcat( m_aBuffer, a );  return (*this); }
DebugStream& DebugStream::operator<<( float p ) { char a[256]; a[0] = 0; sprintf( a,"%f", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( double p ) { char a[256]; a[0] = 0; sprintf( a,"%f", p ); strcat( m_aBuffer, a );   return (*this); }
DebugStream& DebugStream::operator<<( long double p ) { char a[256]; a[0] = 0; sprintf( a,"%Lf", p ); strcat( m_aBuffer, a );   return (*this); }

inline DebugStream& DebugStream::operator<<( DebugStream& (* _f)(DebugStream&)) { (*_f)(*this); return *this; }

inline DebugStream& flush( DebugStream& _outs ) { return _outs.flush(); }
inline DebugStream& endl( DebugStream& _outs ) { return (_outs << '\n').flush(); }
inline DebugStream& ends( DebugStream& _outs ) { return _outs << char('\0'); }

extern DebugStream cdbg;
#define cdbgfl cdbg << "file: " << __FILE__ << " line: " << __LINE__

///////////////////////////////////////////////////////////////////////////////

#endif
