/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_ContainerM3U_H
#define __SMS_ContainerM3U_H

#ifndef __SMS_Container_H
#include "SMS_Container.h"
#endif  /* __SMS_Container_H */

#define SMS_SUBCONTAINER_M4A  0
#define SMS_SUBCONTAINER_OGG  1
#define SMS_SUBCONTAINER_ASF  2
#define SMS_SUBCONTAINER_FLAC 3
#define SMS_SUBCONTAINER_AAC  4
#define SMS_SUBCONTAINER_AC3  5
#define SMS_SUBCONTAINER_MP3  6

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

int SMS_GetContainerM3U ( SMS_Container* );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_ContainerM3U_H */
