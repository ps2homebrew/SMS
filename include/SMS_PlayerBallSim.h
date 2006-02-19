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
#ifndef __SMS_PlayerBallSim_H
# define __SMS_PlayerBallSim_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

uint64_t* SMS_PlayerBallSim_Init    ( uint32_t* );
void      SMS_PlayerBallSim_Destroy ( uint64_t* );
void      SMS_PlayerBallSim_Update  ( uint64_t* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerBallSim_H */
