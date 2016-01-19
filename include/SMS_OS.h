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
#ifndef __SMS_OS_H
#define __SMS_OS_H
#ifndef EMBEDDED

#  ifdef __cplusplus
extern "C" {
#  endif  /* __cplusplus */

extern unsigned char g_OS[ 32768 ];

void SMS_OSInit ( const char* );

#  ifdef __cplusplus
}
#  endif  /* __cplusplus */
#endif  /* EMBEDDED */
#endif  /* __SMS_OS_H */
