/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_ContainerMP3_H
#define __SMS_ContainerMP3_H

#ifndef __SMS_Container_H
#include "SMS_Container.h"
#endif  /* __SMS_Container_H */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int      SMS_GetContainerMP3 ( SMS_Container*               );
uint64_t SMS_MP3Probe        ( FileContext*, SMS_AudioInfo* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_ContainerMP3_H */
