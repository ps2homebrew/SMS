#ifndef __SMS_PlayerControl_H
# define __SMS_PlayerControl_H

# ifndef __SMS_Player_H
#  include "SMS_Player.h"
# endif  /* __SMS_Player_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void PlayerControl_Init         ( SMS_Player*      );
int  PlayerControl_Index2Volume ( SMS_Player*      );
void PlayerControl_AdjustVolume ( SMS_Player*, int );
int  PlayerControl_FastForward  ( SMS_Player*      );
int  PlayerControl_Rewind       ( SMS_Player*      );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerControl_H */
