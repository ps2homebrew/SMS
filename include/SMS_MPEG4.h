/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005-2007 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MPEG4_H
# define __SMS_MPEG4_H

# ifndef __SMS_VideoBuffer_H
#  include "SMS_VideoBuffer.h"
# endif  /* __SMS_VideoBuffer_H */

# ifndef __SMS_MPEG_H
#  include "SMS_MPEG.h"
# endif  /* __SMS_MPEG_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

typedef struct SMS_MPEGCSC {
 SMS_FrameBuffer m_Buf[ 3 ];
 int             m_BufIdx;
} SMS_MPEGCSC;

void SMS_Codec_MPEG4_Open ( SMS_CodecContext* );

void MPEG4_PredAC        ( SMS_DCTELEM*, int, int            );
void MPEG4_CommonInit    ( SMS_CodecContext*                 );
void MPEG4_CommonDestroy ( void                              );
void MPEG4_CommonHWCtl   ( SMS_CodecContext*, SMS_CodecHWCtl );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MPEG4_H */
