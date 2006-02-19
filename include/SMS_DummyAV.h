/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DummyAV_H
# define __SMS_DummyAV_H

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_Codec_DMA_Open ( SMS_CodecContext* );
void SMS_Codec_DMV_Open ( SMS_CodecContext* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_DummyAV_H */
