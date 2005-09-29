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
#include "SMS_MSMPEG4.h"
#include "SMS_MP3.h"
#include "SMS_AC3.h"
#include "SMS_VideoBuffer.h"

#include <malloc.h>
#include <string.h>

#ifdef _WIN32
# include <ctype.h>
#endif  /* _WIN32 */

# define SMS_INTERNAL_BUFFER_SIZE 32

typedef struct SMS_InternalBuffer {

 int              m_LastPicNr;
 SMS_FrameBuffer* m_pBuf;
 int              m_Linesize;

} SMS_InternalBuffer;

static SMS_VideoBuffer* s_pVideoBuffer;

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
 { SMS_CodecID_AC3,  0x00002000 },
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

 if ( apCtx -> m_Type == SMS_CodecTypeVideo ) {

  s_pVideoBuffer     = ( SMS_VideoBuffer* )SMS_InitVideoBuffer ( apCtx -> m_Width, apCtx -> m_Height );
  apCtx -> m_pIntBuf = calloc (  SMS_INTERNAL_BUFFER_SIZE, sizeof ( SMS_InternalBuffer )  );

 }  /* end if */

 switch ( apCtx -> m_ID ) {

  case SMS_CodecID_MPEG4    : SMS_Codec_MPEG4_Open   ( apCtx ); break;
  case SMS_CodecID_MSMPEG4V3: SMS_Codec_MSMPEG4_Open ( apCtx ); break;

  case SMS_CodecID_MP3: SMS_Codec_MP3_Open ( apCtx ); break;
  case SMS_CodecID_AC3: SMS_Codec_AC3_Open ( apCtx ); break;

  default: return 0;

 }  /* end switch */

 return apCtx -> m_pCodec != 0;

}  /* end SMS_CodecOpen */

void SMS_CodecClose ( SMS_CodecContext* apCtx ) {

 if ( apCtx -> m_pIntBuf != NULL && apCtx -> m_Type == SMS_CodecTypeVideo ) {

  s_pVideoBuffer -> Destroy ();
  free ( apCtx -> m_pIntBuf );

 }  /* end if */

}  /* end SMS_CodecClose */

void SMS_CodecReleaseBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 int                 i;
 SMS_InternalBuffer* lpBuf, *lpLast, lBuf;

 lpBuf = 0;

 for ( i = 0; i < apCtx -> m_IntBufCnt; ++i ) {

  lpBuf = &(  ( SMS_InternalBuffer* )apCtx -> m_pIntBuf )[ i ];

  if ( lpBuf -> m_pBuf == apPic -> m_pBuf ) break;

 }  /* end for */

 --apCtx -> m_IntBufCnt;

 lpLast = &(  ( SMS_InternalBuffer* )apCtx ->m_pIntBuf  )[ apCtx -> m_IntBufCnt ];

 lBuf    = *lpBuf;
 *lpBuf  = *lpLast;
 *lpLast = lBuf;

 s_pVideoBuffer -> Release ( apPic -> m_pBuf );
 apPic -> m_pBuf = 0;

}  /* end SMS_CodecReleaseBuffer */

void SMS_CodecGetBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 SMS_InternalBuffer* lpBuf;
 int*                lpPicNr;

 if ( apCtx -> m_pIntBuf == 0 )

  apCtx -> m_pIntBuf = calloc (
   1, SMS_INTERNAL_BUFFER_SIZE * sizeof ( SMS_InternalBuffer )
  );

 lpBuf   = &(  ( SMS_InternalBuffer* )apCtx -> m_pIntBuf )[ apCtx -> m_IntBufCnt ];
 lpPicNr = &(   (  ( SMS_InternalBuffer* )apCtx -> m_pIntBuf  )[ SMS_INTERNAL_BUFFER_SIZE - 1 ]   ).m_LastPicNr;
 ++*lpPicNr;
    
 if ( lpBuf -> m_pBuf != NULL ) {

  apPic -> m_Age       = *lpPicNr - lpBuf -> m_LastPicNr;
  lpBuf -> m_LastPicNr = *lpPicNr;

 } else {

  lpBuf -> m_LastPicNr = -256 * 256 * 256 * 64;
  apPic -> m_Age       =  256 * 256 * 256 * 64;

 }  /* end else */

 lpBuf -> m_pBuf =
 apPic -> m_pBuf = s_pVideoBuffer -> Alloc ();

 apPic -> m_Linesize = s_pVideoBuffer -> m_Linesize;

 ++apCtx -> m_IntBufCnt;

}  /* end SMS_CodecGetBuffer */
