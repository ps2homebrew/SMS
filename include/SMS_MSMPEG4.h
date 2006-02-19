/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MSMPEG4_H
# define __SMS_MSMPEG4_H

# ifndef __SMS_MPEG_H
#  include "SMS_MPEG.h"
# endif  /* __SMS_MPEG_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_MSMPEG4_Open ( SMS_CodecContext* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MSMPEG4_H */
