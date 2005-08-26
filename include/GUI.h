/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __GUI_H
# define __GUI_H

# ifndef __GS_H
#  include "GS.h"
# endif /* __GS_H */

# define GUI_EF_CDROM_MOUNT  0x8000000000000000LL
# define GUI_EF_CDROM_UMOUNT 0x4000000000000000LL
# define GUI_EF_USBM_MOUNT   0x2000000000000000LL
# define GUI_EF_USBM_UMOUNT  0x1000000000000000LL
# define GUI_EF_CDFS_MOUNT   0x0800000000000000LL
# define GUI_EF_CDFS_UMOUNT  0x0400000000000000LL
# define GUI_EF_HOST_MOUNT   0x0200000000000000LL
# define GUI_EF_HOST_UMOUNT  0x0100000000000000LL
# define GUI_EF_DISK_DETECT  0x0080000000000000LL
# define GUI_EF_DISK_NODISK  0x0040000000000000LL

# define GUI_EV_FILE_SELECT  0
# define GUI_EV_UPLEVEL      1
# define GUI_EV_DEV_SELECT   2
# define GUI_EV_MENU_QUIT    3
# define GUI_EV_CONSUMED     4
# define GUI_EV_CDROM_MOUNT  5
# define GUI_EV_CDROM_UMOUNT 6
# define GUI_EV_MENU_EMPTY   7
# define GUI_EV_SAVE_CONFIG  8
# define GUI_EV_USBM_MOUNT   9
# define GUI_EV_USBM_UMOUNT 10
# define GUI_EV_CDFS_MOUNT  11
# define GUI_EV_CDFS_UMOUNT 12
# define GUI_EV_HOST_MOUNT  13
# define GUI_EV_HOST_UMOUNT 14
# define GUI_EV_DISK_DETECT 15
# define GUI_EV_DISK_NODISK 16

# define GUI_FF_FILE      0x00000000
# define GUI_FF_DIRECTORY 0x00000001
# define GUI_FF_PARTITION 0x00000002
# define GUI_FF_SELECTED  0x80000000

# define GUI_DF_CDROM 0x00000001
# define GUI_DF_HDD   0x00000002
# define GUI_DF_CDDA  0x00000004
# define GUI_DF_USBM  0x00000008
# define GUI_DF_CDFS  0x00000010
# define GUI_DF_HOST  0x00000020
# define GUI_DF_DVD   0x00000040

# define GUI_MB_INFORMATION 0
# define GUI_MB_WARNING     1
# define GUI_MB_ERROR       2

typedef struct GUIFileMenuItem {

 unsigned int            m_Flags;
 unsigned int            m_Y;
 char*                   m_pFileName;
 struct GUIFileMenuItem* m_pPrev;
 struct GUIFileMenuItem* m_pNext;

} GUIFileMenuItem;

typedef struct GUIDeviceMenuItem {

 unsigned int              m_Flags;
 unsigned char*            m_pImage;
 struct GUIDeviceMenuItem* m_pPrev;
 struct GUIDeviceMenuItem* m_pNext;

} GUIDeviceMenuItem;

typedef struct GUIMenu {

 void* ( *Navigate ) ( unsigned long int );

} GUIMenu;

typedef struct GUIDeviceMenu {

 GUIDeviceMenuItem* ( *Navigate ) ( unsigned long int );

 GUIDeviceMenuItem* m_pItems;
 GUIDeviceMenuItem* m_pCurr;
 GUIDeviceMenuItem* m_pSel;
 int                m_StartX;
 int                m_StartY;

} GUIDeviceMenu;

typedef struct GUIFileMenu {

 GUIFileMenuItem* ( *Navigate ) ( unsigned long int );

 GUIFileMenuItem* m_pItems;
 GUIFileMenuItem* m_pFirst;
 GUIFileMenuItem* m_pLast;
 GUIFileMenuItem* m_pCurr;
 int              m_Height;
 int              m_Width;
 int              m_Offset;

} GUIFileMenu;

typedef struct GUIContext {

 GUIFileMenu   m_FileMenu;
 GUIDeviceMenu m_DevMenu;
 GUIMenu*      m_pCurrentMenu;
 GSContext*    m_pGSCtx;

 int  ( *Run              ) ( void**            );
 void ( *Status           ) ( char*             );
 void ( *AddDevice        ) ( int               );
 void ( *DelDevice        ) ( int               );
 void ( *AddFile          ) ( char*, int        );
 void ( *ClearFileMenu    ) ( void              );
 void ( *Destroy          ) ( void              );
 void ( *ActivateMenu     ) ( int               );
 void ( *Redraw           ) ( void              );
 void ( *ActivateFileItem ) ( int, char*, char* );
 int  ( *SelectFile       ) ( char*             );

} GUIContext;

GSDisplayMode     GUI_InitPad     ( void       );
GUIContext*       GUI_InitContext ( GSContext* );
unsigned long int GUI_WaitEvent   ( void       );
unsigned long int GUI_WaitButton  ( int        );
int               GUI_ReadButtons ( void       );
#endif  /* __GUI_H */
