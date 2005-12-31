#ifndef __SMS_PlayerControl_H
# define __SMS_PlayerControl_H

# ifndef __GS_H
#  include "GS.h"
# endif  /* __GS_H */

# ifndef __SMS_Player_H
#  include "SMS_Player.h"
# endif  /* __SMS_Player_H */

struct StringList;

# define PC_GSP_SIZE( n ) (  ( n << 2 ) + 6  )

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void                   PlayerControl_Init           ( void                               );
void                   PlayerControl_Destroy        ( void                               );
int                    PlayerControl_Index2Volume   ( void                               );
void                   PlayerControl_AdjustVolume   ( int                                );
int                    PlayerControl_FastForward    ( void                               );
int                    PlayerControl_Rewind         ( void                               );
struct StringListNode* PlayerControl_ChangeLang     ( void                               );
struct StringListNode* PlayerControl_GetLang        ( void                               );
void                   PlayerControl_SwitchSubs     ( void                               );
void                   PlayerControl_DisplayTime    ( int, int64_t, int                  );
void                   PlayerControl_MkTime         ( int64_t, int                       );
void                   PlayerControl_HandleOSD      ( int, int                           );
unsigned int           PlayerControl_GSPLen         ( struct StringList*, unsigned int   );
unsigned int           PlayerControl_GSPacket       ( int, struct StringList*, uint64_t* );
int                    PlayerControl_ScrollBar      ( void ( * ) ( int ), int, int       );
void                   PlayerControl_UpdateDuration ( unsigned int                       );
void                   PlayerControl_UpdateItemNr   ( void                               );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerControl_H */
