/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_FLAC_H
#define __SMS_FLAC_H

#ifndef __SMS_Codec_H
#include "SMS_Codec.h"
#endif  /* __SMS_Codec_H */

typedef struct FLACData {
 SMS_AudioInfo m_Info;
 int           m_MinBlockSz;
 int           m_MaxBlockSz;
 int           m_MinFrameSz;
 int           m_MaxFrameSz;
 int           m_BPS;
 unsigned int  m_Duration;
} FLACData;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SMS_Codec_FLAC_Open ( SMS_CodecContext* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_FLAC_H */
