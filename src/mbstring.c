/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2001-2004, ps2dev - http://www.ps2dev.org
# (c) 2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "mbstring.h"

unsigned int _mbstrlen ( const char* apStr ) {

 unsigned int retVal;

 for ( retVal = 0; apStr[ 0 ]; ++retVal, ++apStr ) {
  if ( apStr[ 0 ] < 0 ) {
   if (  !( ++apStr )[ 0 ]  ) break;
  }  /* end if */
 }  /* end for */

 return retVal;

}  /* end _mbstrlen */

char* _mbstrspnp ( const char* apStr, const char* apChrSet ) {

 char* p, *q;

 for (  q = ( char* )apStr; *q; ++q ) {

  for (  p = ( char* )apChrSet; *p; ++p  ) {

   if ( p[ 0 ] < 0 ) {
    if (   (  ( p[ 0 ] == q[ 0 ] ) && ( p[ 1 ] == q[ 1 ] )  ) || p[ 1 ] == '\0'   ) break;
    ++p;
   } else if ( p[ 0 ] == q[ 0 ] ) break;

  }  /* end for */

  if ( p[ 0 ] == '\0' ) break;

  if ( q[ 0 ] < 0 && *++q == '\0' ) break;

 }  /* end for */

 return q[ 0 ] ? q : ( char* )0;

}  /* end _mbstrspnp */

char* _mbstrpbrk ( const char* apStr, const char* apChrSet  ) {

 char* p, *q;

 for (  q = ( char* )apStr; *q; ++q  ) {

  for (  p = ( char* )apChrSet; *p; ++p  ) {

   if ( p[ 0 ] < 0 ) {
    if (   (  ( p[ 0 ] == q[ 0 ] ) && ( p[ 1 ] == q[ 1 ] )  ) || p[ 1 ] == '\0'   ) break;
    ++p;
   } else if ( p[ 0 ] == q[ 0 ] ) break;

  }  /* end for */

  if ( p[ 0 ] != '\0' ) break;

  if ( q[ 0 ] < 0 && *++q == '\0' ) break;

 }  /* end for */

 return q[ 0 ] ? q : ( char* )0;

}  /* end _mbstrpbrk */

char* _mbstrtok ( char* apStr, const char* apSep ) {

 static char* s_lpNextTok;

 char* lpNextSep;

 if ( apStr )
  s_lpNextTok = apStr;
 else if ( !s_lpNextTok ) return ( char* )0;

 if (   (  apStr = _mbstrspnp ( s_lpNextTok, apSep )  ) == ( char* )0   ) return ( char* )0;

 if (  apStr[ 0 ] == '\0' || ( apStr[ 0 ] < 0 && apStr[ 1 ] == '\0' )  ) return ( char* )0;

 lpNextSep = _mbstrpbrk ( apStr, apSep );

 if (  lpNextSep == ( char* )0 || lpNextSep[ 0 ] == '\0'  )
  s_lpNextTok = ( char* )0;
 else {
  if ( lpNextSep[ 0 ] < 0 ) *lpNextSep++ = '\0';
  lpNextSep[ 0 ] = '\0';
  s_lpNextTok = ++lpNextSep;
 }  /* end else */

 return apStr;

}  /* end _mbstrtok */
