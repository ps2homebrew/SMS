#ifndef __SMS_PlayerControl_H
# define __SMS_PlayerControl_H

# ifndef __SMS_Player_H
#  include "SMS_Player.h"
# endif  /* __SMS_Player_H */

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void InitPlayerControl ( SMS_Player*      );
int  Index2Volume      ( SMS_Player*      );
void AdjustVolume      ( SMS_Player*, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerControl_H */
