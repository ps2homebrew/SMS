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
