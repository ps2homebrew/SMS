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
#ifndef __SMS_H263_H
# define __SMS_H263_H

# ifndef __SMS_DSP_H
#  include "SMS_DSP.h"
# endif  /* __SMS_DSP_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

int      SMS_H263_DecodeMotion       ( int, int               );
int16_t* SMS_H263_PredMotion         ( int, int, int*, int*   );
void     SMS_H263_UpdateMotionVal    ( void                   );
int      SMS_H263_RoundChroma        ( int                    );
void     SMS_H263_DCTUnquantizeIntra ( SMS_DCTELEM*           );
void     SMS_H263_DCTUnquantizeInter ( SMS_DCTELEM*, int, int );

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */
#endif  /* __SMS_H263_H */
