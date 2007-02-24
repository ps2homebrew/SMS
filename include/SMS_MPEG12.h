/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MPEG12_H
# define __SMS_MPEG12_H

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# define SMS_MPEG12_RESET_DECODER 0x00000001
# define SMS_MPEG12_RESET_QUEUE   0x00000002
# define SMS_MPEG12_RESET_TIME    0x00000004
# define SMS_MPEG12_RESET_RECOVER 0x00000008

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_MPEG12_Open  ( SMS_CodecContext* );
void SMS_Codec_MPEG12_Reset ( unsigned int      );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_MPEG12_H */
