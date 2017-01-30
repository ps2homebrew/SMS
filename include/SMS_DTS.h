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
#ifndef __SMS_DTS_H
# define __SMS_DTS_H

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_DTS_Open ( SMS_CodecContext* );

int DTS_SyncInfo ( uint8_t*, int*, int*, int*, int* );
int DTS_Channels ( int                              );

# ifdef __cplusplus
};
# endif  /* __cplusplus */
#endif  /* __SMS_DTS_H */

