#ifndef __SMS_PlayerMenu_H
# define __SMS_PlayerMenu_H

struct MenuContext;
struct SMS_Player;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

struct MenuContext* PlayerMenu_Init ( struct SMS_Player* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PlayerMenu_H */
