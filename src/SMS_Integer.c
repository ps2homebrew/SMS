/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2004 Michael Niedermayer <michaelni@gmx.at>
# Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_Integer.h"

#include <string.h>

SMS_Integer SMS_Integer_mul_i ( SMS_Integer anA, SMS_Integer aB ) {

 SMS_Integer retVal;
 int         i, j;
 int         lnA = (  SMS_Integer_log2_i ( anA ) + 16  ) >> 4;
 int         lnB = (  SMS_Integer_log2_i ( aB  ) + 16  ) >> 4;
    
 memset (  &retVal, 0, sizeof( SMS_Integer )  );
    
 for ( i = 0; i < lnA; ++i ) {

  unsigned int lCarry = 0;
        
  if ( anA.m_V[ i ] )

   for ( j = i; j < SMS_INTEGER_SIZE && j - i <= lnB; ++j ) {

    lCarry = ( lCarry >> 16 ) + retVal.m_V[ j ] + anA.m_V[ i ] * aB.m_V[ j - i ];
    retVal.m_V[ j ] = lCarry;

   }  /* end for */

 }  /* end for */

 return retVal;

}  /* end SMS_Integer_mul_i */

SMS_Integer SMS_Integer_int2i ( int64_t anA ) {

 SMS_Integer retVal;
 int         i;
    
 for ( i = 0; i < SMS_INTEGER_SIZE; ++i ) {

  retVal.m_V[ i ] = ( uint16_t )anA;
  anA >>= 16;

 }  /* end for */

 return retVal;

}  /* end SMS_Integer_int2i */

SMS_Integer SMS_Integer_add_i ( SMS_Integer anA, SMS_Integer aB ) {

 int i, lCarry = 0;
    
 for ( i = 0; i < SMS_INTEGER_SIZE; ++i ) {

  lCarry = ( lCarry >> 16 ) + anA.m_V[ i ] + aB.m_V[ i ];
  anA.m_V[ i ] = lCarry;

 }  /* end for */

 return anA;

}  /* end SMS_Integer_add_i */

int64_t SMS_Integer_i2int ( SMS_Integer anA ) {

 int64_t retVal = ( int8_t )anA.m_V[ SMS_INTEGER_SIZE - 1 ];
 int     i;
    
 for ( i = SMS_INTEGER_SIZE - 2; i >= 0; --i )

  retVal = ( retVal << 16 ) + anA.m_V[ i ];

 return retVal;

}  /* end SMS_Integer_i2int */

int SMS_Integer_log2_i ( SMS_Integer anA ) {

 int i;

 for ( i = SMS_INTEGER_SIZE - 1; i >= 0; --i )

  if ( anA.m_V[ i ] ) return SMS_log2 ( anA.m_V[ i ] ) + 16 * i;

 return -1;

}  /* end SMS_Integer_log2_i */

SMS_Integer SMS_Integer_shr_i ( SMS_Integer anA, int aS ) {

 SMS_Integer retVal;
 int         i;

 for ( i = 0; i < SMS_INTEGER_SIZE; ++i ) {

  int          lIdx = i + ( aS >> 4 );
  unsigned int lV   = 0;

  if ( lIdx + 1 < SMS_INTEGER_SIZE && lIdx + 1 >= 0 ) lV  = anA.m_V[ lIdx + 1 ] << 16;
  if ( lIdx     < SMS_INTEGER_SIZE && lIdx     >= 0 ) lV += anA.m_V[ lIdx ];

  retVal.m_V[ i ]= lV >> ( aS & 15 );

 }  /* end for */

 return retVal;

}  /* end SMS_Integer_shr_i */

SMS_Integer SMS_Integer_div_i ( SMS_Integer anA, SMS_Integer aB ) {

 SMS_Integer retVal;

 SMS_Integer_mod_i ( &retVal, anA, aB );

 return retVal;

}  /* end SMS_Integer_div_i */

SMS_Integer SMS_Integer_mod_i ( SMS_Integer* aQuot, SMS_Integer anA, SMS_Integer aB ) {

 SMS_Integer lQuot;
 int         i = SMS_Integer_log2_i ( anA ) - SMS_Integer_log2_i ( aB );

 if ( !aQuot ) aQuot = &lQuot;
    
 if ( i > 0 ) aB = SMS_Integer_shr_i ( aB, -i );
        
 memset (  aQuot, 0, sizeof ( SMS_Integer )  );

 while ( i-- >= 0 ) {

  *aQuot = SMS_Integer_shr_i ( *aQuot, -1 );

  if (  SMS_Integer_cmp_i ( anA, aB ) >= 0  ) {

   anA = SMS_Integer_sub_i ( anA, aB );
   aQuot -> m_V[ 0 ] += 1;

  }  /* end if */

  aB = SMS_Integer_shr_i ( aB, 1 );

 }  /* end while */

 return anA;

}  /* end SMS_Integer_mod_i */

int SMS_Integer_cmp_i ( SMS_Integer anA, SMS_Integer aB ) {

 int i; 
 int lV = ( int16_t )anA.m_V[ SMS_INTEGER_SIZE - 1 ] -
          ( int16_t )aB.m_V [ SMS_INTEGER_SIZE - 1 ];

 if ( lV ) return ( lV >> 16 ) | 1;
    
 for ( i = SMS_INTEGER_SIZE - 2 ; i >= 0; --i ) {

  int lV = anA.m_V[ i ] - aB.m_V[ i ];

  if ( lV ) return ( lV >> 16 ) | 1;

 }  /* end for */

 return 0;

}  /* end SMS_Integer_cmp_i */

SMS_Integer SMS_Integer_sub_i ( SMS_Integer anA, SMS_Integer aB ) {

 int i, lCarry = 0;
    
 for ( i = 0; i < SMS_INTEGER_SIZE; ++i ) {

  lCarry= ( lCarry >> 16 ) + anA.m_V[ i ] - aB.m_V[ i ];
  anA.m_V[ i ] = lCarry;

 }  /* end for */

 return anA;

}  /* end SMS_Integer_sub_i */
