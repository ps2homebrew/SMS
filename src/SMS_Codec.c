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
#include "SMS_MPEG12.h"
#include "SMS_MP123.h"
#include "SMS_AC3.h"
#include "SMS_DTS.h"
#include "SMS_OGG.h"
#include "SMS_WMA.h"
#include "SMS_PCM.h"
#include "SMS_AAC.h"
#include "SMS_FLAC.h"
#include "SMS_VideoBuffer.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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
 { SMS_CodecID_MPEG4,     SMS_MKTAG(  'F',  'M',  'P',  '4' ) },
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '3' ) },
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'M',  'P',  '4',  '3' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'M',  'P',  'G',  '3' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '5' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '6' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'D',  'I',  'V',  '4' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'A',  'P',  '4',  '1' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'C',  'O',  'L',  '1' ) }, 
 { SMS_CodecID_MSMPEG4V3, SMS_MKTAG(  'C',  'O',  'L',  '0' ) },
 { SMS_CodecID_DXSB,      SMS_MKTAG(  'D',  'X',  'S',  'B' ) },
 { SMS_CodecID_NULL,      SMS_MKTAG( '\0', '\0', '\0', '\0' ) }
};

static SMS_CodecTag s_CodecAudioTags[] = {
 { SMS_CodecID_MP2,     0x00000050 },
 { SMS_CodecID_MP3,     0x00000055 },
 { SMS_CodecID_AC3,     0x00002000 },
 { SMS_CodecID_DTS,     0x00002001 },
 { SMS_CodecID_OGGV,    0x0000674F },
 { SMS_CodecID_WMA_V1,  0x00000160 },
 { SMS_CodecID_WMA_V2,  0x00000161 },
 { SMS_CodecID_PCM16LE, 0x00000001 },
 { SMS_CodecID_AAC,     0x6134706D },
 { SMS_CodecID_AAC,     0x000000FF },
 { SMS_CodecID_FLAC,    0x43614C66 },
 { SMS_CodecID_NULL,    0x00000000 }
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

 switch ( apCtx -> m_ID ) {

  case SMS_CodecID_MPEG4    : SMS_Codec_MPEG4_Open   ( apCtx ); break;
  case SMS_CodecID_MSMPEG4V3: SMS_Codec_MSMPEG4_Open ( apCtx ); break;
  case SMS_CodecID_MPEG1    :
  case SMS_CodecID_MPEG2    : SMS_Codec_MPEG12_Open  ( apCtx ); break;

  case SMS_CodecID_DXSB: return 1;

  case SMS_CodecID_MP2    : apCtx -> m_Tag = SMS_MKTAG( 'm', 'p', '2', ' ' ); goto SMS_CodecID_MPA;
  case SMS_CodecID_MP3    : apCtx -> m_Tag = SMS_MKTAG( 'm', 'p', '3', ' ' );
       SMS_CodecID_MPA    :                                                   SMS_Codec_MP123_Open ( apCtx ); break;
  case SMS_CodecID_AC3    : apCtx -> m_Tag = SMS_MKTAG( 'a', 'c', '3', ' ' ); SMS_Codec_AC3_Open   ( apCtx ); break;
  case SMS_CodecID_DTS    : apCtx -> m_Tag = SMS_MKTAG( 'd', 't', 's', ' ' ); SMS_Codec_DTS_Open   ( apCtx ); break;
  case SMS_CodecID_OGGV   : apCtx -> m_Tag = SMS_MKTAG( 'o', 'g', 'g', ' ' ); SMS_Codec_OGGV_Open  ( apCtx ); break;
  case SMS_CodecID_WMA_V1 : apCtx -> m_Tag = SMS_MKTAG( 'w', 'm', 'a', '1' ); goto SMS_CodecID_WMA;
  case SMS_CodecID_WMA_V2 : apCtx -> m_Tag = SMS_MKTAG( 'w', 'm', 'a', '2' );
       SMS_CodecID_WMA    :                                                   SMS_Codec_WMA_Open   ( apCtx ); break;
  case SMS_CodecID_PCM16BE:
  case SMS_CodecID_PCM16LE: apCtx -> m_Tag = SMS_MKTAG( 'p', 'c', 'm', ' ' ); SMS_Codec_PCM_Open   ( apCtx ); break;
  case SMS_CodecID_AAC    : apCtx -> m_Tag = SMS_MKTAG( 'a', 'a', 'c', ' ' ); SMS_Codec_AAC_Open   ( apCtx ); break;
  case SMS_CodecID_FLAC   :                                                   SMS_Codec_FLAC_Open  ( apCtx ); break;

  default: break;

 }  /* end switch */

 return apCtx -> m_pCodec != NULL;

}  /* end SMS_CodecOpen */

void SMS_CodecClose ( SMS_CodecContext* apCtx ) {

 if ( apCtx -> HWCtl       ) apCtx -> HWCtl ( apCtx, SMS_HWC_Destroy );
 if ( apCtx -> m_pUserData ) free ( apCtx -> m_pUserData );

}  /* end SMS_CodecClose */

void SMS_CodecDestroy ( SMS_CodecContext* apCtx ) {

 if ( apCtx ) {

  if ( apCtx -> m_pCodec ) {

   apCtx -> m_pCodec -> Destroy ( apCtx );
   free ( apCtx -> m_pCodec );

  }  /* end if */

  SMS_CodecClose ( apCtx );

  free ( apCtx );

 }  /* end if */

}  /* end SMS_CodecDestroy */

void SMS_CodecGetBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 SMS_VideoBuffer* lpBuf = ( SMS_VideoBuffer* )apCtx -> m_pIntBuf;

 apPic -> m_pBuf  = lpBuf -> m_pFree;
 lpBuf -> m_pFree = lpBuf -> m_pFree -> m_pNext;

}  /* end SMS_CodecGetBuffer */

void SMS_CodecReleaseBuffer ( SMS_CodecContext* apCtx, struct SMS_Frame* apPic ) {

 SMS_VideoBuffer* lpBuf = ( SMS_VideoBuffer* )apCtx -> m_pIntBuf;

 apPic -> m_pBuf -> m_pNext = lpBuf -> m_pFree;
 lpBuf -> m_pFree           = apPic -> m_pBuf;
 apPic -> m_pBuf            = NULL;

}  /* end SMS_CodecReleaseBuffer */
