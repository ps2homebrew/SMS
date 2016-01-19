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
#ifndef __SMS_ContainerOGG_H
#define __SMS_ContainerOGG_H

#ifndef __SMS_Container_H
#include "SMS_Container.h"
#endif  /* __SMS_Container_H */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int      SMS_GetContainerOGG ( SMS_Container*                         );
uint64_t SMS_OGGVProbe       ( FileContext* apFileCtx, SMS_AudioInfo* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SMS_ContainerOGG_H */
