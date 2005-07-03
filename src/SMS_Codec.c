/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2001 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
*/
#include "SMS_Codec.h"
#include "SMS_FourCC.h"
#include "SMS_MPEG4.h"
#include "SMS_MP3.h"
#include "SMS_VideoBuffer.h"

#include <malloc.h>
#include <string.h>

#ifdef _WIN32
# include <ctype.h>
#endif  /* _WIN32 */

# define SMS_INTERNAL_BUFFER_SIZE 32

static SMS_CodecTag s_CodecVideoTags[] = {
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'D',  'I',  'V',  'X' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'D',  'X',  '5',  '0' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'X',  'V',  'I',  'D' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'M',  'P',  '4',  'S' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'M',  '4',  'S',  '2' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG( '\4', '\0', '\0', '\0' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'D',  'I',  'V',  '1' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'B',  'L',  'Z',  '0' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'm',  'p',  '4',  'v' ) },
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'U',  'M',  'P',  '4' ) },
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '3' ) },
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'M',  'P',  '4',  '3' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'M',  'P',  'G',  '3' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '5' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '6' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '4' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'A',  'P',  '4',  '1' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'C',  'O',  'L',  '1' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'C',  'O',  'L',  '0' ) },
 { SMS_CodecID_NULL,      SMS_MKTAG( '\0', '\0', '\0', '\0' ) }
};

static SMS_CodecTag s_CodecAudioTags[] = {
 { SMS_CodecID_MP3,  0x00000055 },
 { SMS_CodecID_NULL, 0x00000000 }
};

SMS_CodecID SMS_CodecGetID ( SMS_CodecType aType, uint32_t aTag ) {

 SMS_CodecTag* lpTag = aType == SMS_CodecTypeVideo ? s_CodecVideoTags : s_CodecAudioTags;

 while ( lpTag -> m_ID != SMS_CodecID_NULL ) {

  if (   toupper (  ( aTag >>  0 ) & 0xFF  ) == toupper (  ( lpTag -> m_Tag >>  0 ) & 0xFF  ) &&
         toupper (  ( aTag >>  8 ) & 0xFF  ) == toupper (  ( lpTag -> m_Tag >>  8 ) & 0xFF  ) &&
         toupper (  ( aTag >> 16 ) & 0xFF  ) == toupper (  ( lpTag -> m_Tag >> 16 ) & 0xFF  ) &&
         toupper (  ( aTag >> 24 ) & 0xFF  ) == toupper (  ( lpTag -> m_Tag >> 24 ) & 0xFF  )
  ) return lpTag -> m_ID;

  ++lpTag;

 }  /* end while */

 return SMS_CodecID_NULL;

}  /* end SMS_Codec_GetID */

int SMS_CodecOpen ( SMS_CodecContext* apCtx ) {

 if ( apCtx -> m_Type == SMS_CodecTypeVideo )

  apCtx -> m_pIntBuf = ( SMS_VideoBuffer* )SMS_InitVideoBuffer ( apCtx -> m_Width, apCtx -> m_Height );

 switch ( apCtx -> m_ID ) {

  case SMS_CodecID_MPEG4:

   SMS_Codec_MPEG4_Open ( apCtx );

  break;

  case SMS_CodecID_MP3:

   SMS_Codec_MP3_Open ( apCtx );

  break;

  default: return 0;

 }  /* end switch */

 return apCtx -> m_pCodec != 0;

}  /* end SMS_CodecOpen */

void SMS_CodecClose ( SMS_CodecContext* apCtx ) {

 if ( apCtx -> m_pIntBuf != NULL && apCtx -> m_Type == SMS_CodecTypeVideo )

  (  ( SMS_VideoBuffer* )apCtx -> m_pIntBuf  ) -> Destroy ();

}  /* end SMS_CodecClose */

void SMS_CodecReleaseBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 SMS_VideoBuffer* lpBuf = ( SMS_VideoBuffer* )apCtx -> m_pIntBuf;

 lpBuf -> Release ( apPic -> m_pBuf );
 apPic -> m_pBuf = NULL;

}  /* end SMS_CodecReleaseBuffer */

void SMS_CodecGetBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 SMS_VideoBuffer* lpBuf = ( SMS_VideoBuffer* )apCtx -> m_pIntBuf;

 apPic -> m_pBuf     = lpBuf -> Alloc ();
 apPic -> m_Linesize = lpBuf -> m_Linesize;

}  /* end SMS_CodecGetBuffer */
