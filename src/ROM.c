/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 Damien Ciabrini (dciabrin), Olivier Parra (yo6)
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
*/
#include "ROM.h"
#include <string.h>

#define PAD16( x ) (   (  ( x ) + 0xF  ) & ~0xF   )

typedef struct ROMDir {

 char           m_Name[ 10 ] __attribute__(  ( packed )  );
 unsigned short m_XInfoSize  __attribute__(  ( packed )  );
 int            m_Size       __attribute__(  ( packed )  );

} ROMDir;

void* ROM_LocateModule ( unsigned int aBase, const char* apName, int* apSize ) {

 static void* lpStart = 0;

 int     i, lOffset;
 ROMDir* lpDir;

 if ( !lpStart ) {

  unsigned char* lpROM = ( unsigned char* )aBase;

  for ( i = 0; i < 16384; ++i ) if (  !memcmp ( lpROM + i, "RESET", 5 )  ) break;

  if ( i == 16384 ) return NULL;

  lpStart = lpROM + i;

 }  /* end if */

 i       = 3;
 lpDir   = ( ROMDir* )lpStart;
 lOffset = PAD16( lpDir[ 1 ].m_Size + lpDir[ 2 ].m_Size );

 lpDir += 3;

 while ( lpDir -> m_Name[ 0 ] ) {

  if (  !strncmp ( apName, lpDir -> m_Name, 10 )  ) break;

  ++lpDir;
  lOffset += PAD16(   (  ( ROMDir* )lpStart  )[ i++ ].m_Size   );

 }  /* end while */

 if ( !lpDir -> m_Name[ 0 ] ) return NULL;

 if ( apSize ) *apSize = lpDir -> m_Size;

 return ( void* )(  ( unsigned int )lpStart + lOffset  );

}  /* end ROM_LocateModule */
