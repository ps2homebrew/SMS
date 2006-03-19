/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_MPEG2_H
# define __SMS_MPEG2_H

# ifndef __SMS_DSP_H
#  include "SMS_DSP.h"
# endif  /* __SMS_DSP_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_MPEG2_DCTUnquantizeIntra ( SMS_DCTELEM*, int, int );
void SMS_MPEG2_DCTUnquantizeInter ( SMS_DCTELEM*, int, int );

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */
#endif  /* __SMS_MPEG2_H */
