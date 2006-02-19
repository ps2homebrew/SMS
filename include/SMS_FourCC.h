/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on ffmpeg project (no copyright notes in the original source code)
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_FourCC_H
# define __SMS_FourCC_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# define SMS_FourCC_DivX 0x58766944
# define SMS_FourCC_DIVX 0x58564944
# define SMS_FourCC_UMP4 0x34504D55
# define SMS_FourCC_XviD 0x44697658
# define SMS_FourCC_XVID 0x44495658
# define SMS_FourCC_XVIX 0x58495658
# define SMS_FourCC_MP3  0x00000055

# define SMS_MKTAG( a, b, c, d ) (   ( a ) | (  ( b ) << 8  ) | (  ( c ) << 16  ) | (  ( d ) << 24  )   )

static SMS_INLINE unsigned int SMS_GetFourCC ( const char* apID ) {
 return apID[ 0 ] + ( apID[ 1 ] << 8 ) + ( apID[ 2 ] << 16 ) + ( apID[ 3 ] << 24 );
}  /* end SMS_GetFourCC */

#endif  /* __SMS_FourCC_H */
