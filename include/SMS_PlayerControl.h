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
#ifndef __SMS_PlayerControl_H
# define __SMS_PlayerControl_H

# ifndef __SMS_GS_H
#  include "SMS_GS.h"
# endif  /* __SMS_GS_H */

# ifndef __SMS_Player_H
#  include "SMS_Player.h"
# endif  /* __SMS_Player_H */

struct SMS_List;

# define PC_GSP_SIZE( n ) (  ( n << 2 ) + 6  )

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void                 PlayerControl_Init           ( void                             );
void                 PlayerControl_Destroy        ( void                             );
void                 PlayerControl_AdjustVolume   ( int                              );
int                  PlayerControl_FastForward    ( void                             );
int                  PlayerControl_Rewind         ( void                             );
struct SMS_ListNode* PlayerControl_ChangeLang     ( void                             );
struct SMS_ListNode* PlayerControl_GetLang        ( void                             );
void                 PlayerControl_SwitchSubs     ( void                             );
void                 PlayerControl_DisplayTime    ( int, int64_t, int                );
void                 PlayerControl_MkTime         ( int64_t                          );
void                 PlayerControl_HandleOSD      ( int, int                         );
unsigned int         PlayerControl_GSPLen         ( struct SMS_List*, unsigned int   );
unsigned int         PlayerControl_GSPacket       ( int, struct SMS_List*, uint64_t* );
int                  PlayerControl_ScrollBar      ( void ( * ) ( int ), int, int     );
void                 PlayerControl_UpdateDuration ( unsigned int, unsigned int       );
void                 PlayerControl_UpdateItemNr   ( void                             );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerControl_H */
