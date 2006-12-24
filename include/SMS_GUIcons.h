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
#ifndef __SMS_GUIcons_H
# define __SMS_GUIcons_H

# ifndef __SMS_GUI_H
#  include "SMS_GUI.h"
# endif  /* __SMS_GUI_H */

# define GUICON_FOLDER     0
# define GUICON_AVI        2
# define GUICON_MP3        4
# define GUICON_M3U        6
# define GUICON_FILE       8
# define GUICON_PARTITION 10
# define GUICON_AVIS      12
# define GUICON_SHARE     14

# define GUICON_USB   (  ( GUI_MSG_USB   >> 16 ) - 1  )
# define GUICON_CDROM (  ( GUI_MSG_CDROM >> 16 ) - 1  )
# define GUICON_HDD   (  ( GUI_MSG_HDD   >> 16 ) - 1  )
# define GUICON_CDDA  (  ( GUI_MSG_CDDA  >> 16 ) - 1  )
# define GUICON_HOST  (  ( GUI_MSG_HOST  >> 16 ) - 1  )
# define GUICON_DVD   (  ( GUI_MSG_DVD   >> 16 ) - 1  )
# define GUICON_SMB   (  ( GUI_MSG_SMB   >> 16 ) - 1  )

# define GUICON_ERROR    0
# define GUICON_DISPLAY  1
# define GUICON_HELP     2
# define GUICON_NETWORK  3
# define GUICON_BROWSER  4
# define GUICON_PLAYER   5
# define GUICON_ON       6
# define GUICON_OFF      7
# define GUICON_SAVE     8
# define GUICON_EXIT     9
# define GUICON_FINISH  10
# define GUICON_BALL    11

typedef enum GUIcon {

 GUIcon_Browser = 0,
 GUIcon_Misc    = 1,
 GUIcon_Device  = 2

} GUIcon;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void GUI_LoadIcons ( void                                           );
void GUI_DrawIcon  ( unsigned int, int, int, GUIcon, unsigned long* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_GUIcons_H */
