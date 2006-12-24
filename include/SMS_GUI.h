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
#ifndef __SMS_GUI_H
# define __SMS_GUI_H

# define GUI_MSG_MOUNT_BIT  0x0000000000100000L
# define GUI_MSG_MOUNT_MASK 0x00000000001F0000L
# define GUI_MSG_PAD_MASK   0x000000000000FFFFL
# define GUI_MSG_USER_MASK  0xFFFFFFFFFFE00000L

# define GUI_MSG_USB    0x0000000000010000L
# define GUI_MSG_CDROM  0x0000000000020000L
# define GUI_MSG_HDD    0x0000000000030000L
# define GUI_MSG_CDDA   0x0000000000040000L
# define GUI_MSG_HOST   0x0000000000050000L
# define GUI_MSG_DVD    0x0000000000060000L
# define GUI_MSG_SMB    0x0000000000070000L
# define GUI_MSG_LOGIN  0x0000000000080000L

# define GUI_MSG_MEDIA_SELECTED 0x8000000000000000L
# define GUI_MSG_MEDIA_REMOVED  0x7000000000000000L
# define GUI_MSG_UPDATE_STATUS  0x6000000000000000L
# define GUI_MSG_REFILL_BROWSER 0x5000000000000000L
# define GUI_MSG_QUIT           0x4000000000000000L
# define GUI_MSG_FILE           0x3000000000000000L
# define GUI_MSG_FOLDER_MP3     0x2000000000000000L

# define DECLARE_GUI_OBJECT()                                \
 void ( *Render      ) ( struct GUIObject*, int           ); \
 int  ( *HandleEvent ) ( struct GUIObject*, unsigned long ); \
 void ( *SetFocus    ) ( struct GUIObject*, int           ); \
 void ( *Cleanup     ) ( struct GUIObject*                ); \
 unsigned long* m_pGSPacket;

typedef enum GUIHResult {

 GUIHResult_Void        = 0,
 GUIHResult_Handled     = 1,
 GUIHResult_ChangeFocus = 2

} GUIHResult;

typedef struct GUIObject {

 DECLARE_GUI_OBJECT()

} GUIObject;

typedef enum GUIRedrawMethod {

 GUIRedrawMethod_InitClearAll = 0,
 GUIRedrawMethod_InitClearObj = 1,
 GUIRedrawMethod_Redraw       = 2,
 GUIRedrawMethod_RedrawClear  = 3

} GUIRedrawMethod;

extern GUIObject* g_pDesktop;
extern GUIObject* g_pStatusLine;
extern GUIObject* g_pDevMenu;
extern GUIObject* g_pFileMenu;
extern void*      g_pActiveNode;
extern int        g_SMBUnit;
extern int        g_SMBError;
extern int        g_SMBServerError;

extern int ( *GUI_ReadButtons  ) ( void );

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void          GUI_Initialize   ( int                      );
int           GUI_WaitButtons  ( int, unsigned*, int      );
void          GUI_AddObject    ( const char*, GUIObject*  );
void          GUI_Redraw       ( GUIRedrawMethod          );
void          GUI_Status       ( unsigned char*           );
void          GUI_Error        ( unsigned char*           );
void          GUI_Progress     ( unsigned char*, int, int );
void          GUI_Run          ( void                     );
void          GUI_Suspend      ( void                     );
void          GUI_Resume       ( void                     );
unsigned long GUI_WaitEvent    ( void                     );
void          GUI_PostMessage  ( unsigned long            );
void          GUI_DeleteObject ( const unsigned char*     );
void          GUI_UpdateStatus ( void                     );
void          GUI_SetColors    ( void                     );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_GUI_H */
