/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001 Fabrice Bellard.
# Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_Bitio.h"

void SMS_InitGetBits ( SMS_BitContext* apCtx, const uint8_t* apBuf, uint32_t aBitSize ) {

 const uint32_t lBufSize = ( aBitSize + 7 ) >> 3;

 apCtx -> m_pBuf     = apBuf;
 apCtx -> m_pBufEnd  = apBuf + lBufSize;
 apCtx -> m_szInBits = aBitSize;
 apCtx -> m_Idx      = 0;

 {  /* begin block */

  SMS_OPEN_READER( re, apCtx )
   SMS_UPDATE_CACHE( re, apCtx )
   SMS_UPDATE_CACHE( re, apCtx )
  SMS_CLOSE_READER( re, apCtx )

 }  /* end block */

}  /* end SMS_InitGetBits */

uint32_t SMS_GetBitsLong ( SMS_BitContext* apCtx, uint32_t aN ) {

 if ( aN <= 17 )

  return SMS_GetBits ( apCtx, aN );

 else {

  int retVal = SMS_GetBits ( apCtx, 16 ) << ( aN - 16 );

  return retVal | SMS_GetBits ( apCtx, aN - 16 );

 }  /* end else */

}  /* end SMS_GetBitsLong */

uint32_t SMS_ShowBitsLong ( SMS_BitContext* apCtx, uint32_t aN ) {

 if ( aN <= 17 )

  return SMS_ShowBits ( apCtx, aN );

 else {

  SMS_BitContext lBitCtx = *apCtx;
  uint32_t       retVal  = SMS_GetBitsLong ( apCtx, aN );

  *apCtx = lBitCtx;

  return retVal;

 }  /* end else */

}  /* end SMS_ShowBitsLong */
