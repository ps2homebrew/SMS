/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# (c) 2005 HOST support by Ronald Andersson (AKA: dlanor)
# Special thanks to 'bigboss'/PS2Reality for valuable information
# about SifAddCmdHandler function
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_GUI.h"
#include "SMS_GS.h"
#include "SMS_Timer.h"
#include "SMS_MC.h"
#include "SMS_PAD.h"
#include "SMS_Config.h"
#include "SMS_List.h"
#include "SMS_GUIcons.h"
#include "SMS_DMA.h"
#include "SMS_Locale.h"
#include "SMS_IOP.h"
#include "SMS_CDDA.h"
#include "SMS_CDVD.h"
#include "SMS_FileDir.h"
#include "SMS_SPU.h"
#include "SMS_Sounds.h"
#include "SMS_RC.h"

#include <kernel.h>
#include <loadfile.h>
#include <libhdd.h>
#include <fileio.h>
#include <malloc.h>
#include <sifrpc.h>

#define GUIF_DEV_CHECK 0x00000001

#define DEVF_USB_DISCONNECT 0x00000001
#define DEVF_USB_CONNECT    0x00000002
#define DEVF_CDDA           0x00000004
#define DEVF_CD             0x00000008
#define DEVF_DVD            0x00000010
#define DEVF_CDVD_DETECT    0x00000020
#define DEVF_CDVD_UNKNOWN   0x00000040
#define DEVF_CDVD_MASK      0x0000007C
#define DEVF_HOST           0x00000080
#define DEFV_SMB            0x00000100

GUIObject* g_pStatusLine;
GUIObject* g_pDesktop;
GUIObject* g_pDevMenu;
GUIObject* g_pFileMenu;
void*      g_pActiveNode;

int ( *GUI_ReadButtons ) ( void );

static SMS_List*      s_pObjectList;
static SMS_List*      s_pMsgQueue;
static int            s_EventSema;
static int            s_GUIThreadID;
static int            s_GUIFlags;
static int            s_PrevBtn;
static volatile long  s_Event;
static unsigned long  s_PowerOffTimer;
static unsigned int   s_DevFlags;
static GSBitBltPacket s_BitBlt;
       int            g_SMBUnit;
       int            g_SMBError;
       int            g_SMBServerError;

static unsigned char s_pMsgStr [] __attribute__(   (  section( ".data" )  )   ) = "_POSTED_MESSAGE";

static unsigned char s_PadBuf0[  256 ] __attribute__(   (  aligned( 64 ), section( ".data"  )  )   );
       unsigned char g_PadBuf1[  256 ] __attribute__(   (  aligned( 64 ), section( ".data"  )  )   );
static unsigned char s_Stack  [ 4096 ] __attribute__(   (  aligned( 16 ), section( ".data"  )  )   );

static void ( *QueryPad ) ( void );

static void QueryPad0 ( void );
static void QueryPad1 ( void );

extern void*      _gp;
extern GUIObject* GUI_CreateDesktop    ( void );
extern GUIObject* GUI_CreateStatusLine ( void );
extern GUIObject* GUI_CreateDevMenu    ( void );
extern GUIObject* GUI_CreateFileMenu   ( void );
extern GUIObject* GUI_CreateCmdProc    ( void );

static void _cleanup ( int afAll ) {

 SMS_ListNode* lpNode = s_pObjectList -> m_pHead;

 if ( !afAll ) lpNode = lpNode -> m_pNext;

 while ( lpNode ) {

  GUIObject* lpObj = ( GUIObject* )( unsigned int )lpNode -> m_Param;

  lpObj -> Cleanup ( lpObj );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

}  /* end _cleanup */

void GUI_Redraw ( GUIRedrawMethod aMethod ) {

 SMS_ListNode* lpNode = s_pObjectList -> m_pHead;
 GUIObject*    lpObj  = ( GUIObject* )( unsigned int )(  ( SMS_ListNode* )g_pActiveNode  ) -> m_Param;

 if ( aMethod == GUIRedrawMethod_InitClearAll ) {

  GSContext_InitBitBlt (
   &s_BitBlt, 0, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height,
   g_GSCtx.m_VRAMPtr2, 0, 0
  );

  _cleanup ( 1 );

 } else if ( aMethod == GUIRedrawMethod_InitClearObj ) _cleanup ( 0 );

 while ( lpNode ) {

  GUIObject* lpObj = ( GUIObject* )( unsigned int )lpNode -> m_Param;

  lpObj -> Render ( lpObj, 0 );

  lpNode = lpNode -> m_pNext;

 }  /* end while */

 GS_VSync ();
 GSContext_BitBlt ( &s_BitBlt );
 GSContext_Flush ( 0, aMethod == GUIRedrawMethod_RedrawClear ? GSFlushMethod_DeleteLists : GSFlushMethod_KeepLists );

 if ( aMethod == aMethod == GUIRedrawMethod_RedrawClear ) _cleanup ( 1 );

 if ( lpObj -> SetFocus ) lpObj -> SetFocus ( lpObj, 1 );

}  /* end GUI_Redraw */

void GUI_AddObject ( const char* apName, GUIObject* apObj ) {

 SMS_ListPushBack ( s_pObjectList, apName );
 s_pObjectList -> m_pTail -> m_Param = ( unsigned int )apObj;
 g_pActiveNode = s_pObjectList -> m_pTail;

}  /* end GUI_AddObject */

int GUI_ReadButtons0 ( void ) {

 int retVal = PAD_Read ( 0, 0 );

 if ( !retVal ) retVal = RC_Read ();

 return retVal;

}  /* end GUI_ReadButtons0 */

int GUI_ReadButtons1 ( void ) {

 int retVal = PAD_Read ( 0, 0 );

 if ( !retVal ) retVal = PAD_Read ( 1, 0 );

 return retVal;

}  /* end GUI_ReadButtons1 */

int GUI_ReadButtons2 ( void ) {

 return PAD_Read ( 0, 0 );

}  /* end GUI_ReadButtons2 */

static void TimerHandler ( void* apArg ) {

 iWakeupThread ( s_GUIThreadID );
 SMS_iTimerSet ( 0, 64, TimerHandler, NULL );

}  /* end TimerHandler */

static void _smb_handler_connect ( void* apHdr ) {

 int* lpParam = &(  ( SifCmdHeader_t* )apHdr  ) -> unknown;

 g_SMBUnit        = lpParam[ 1 ];
 g_SMBError       = lpParam[ 2 ];
 g_SMBServerError = lpParam[ 3 ];

 s_DevFlags |= DEFV_SMB;
 iWakeupThread ( s_GUIThreadID );

}  /* end _smb_handler_connect */

static void _usb_handler_connect ( void* apHdr ) {

 if ( s_DevFlags & DEVF_USB_DISCONNECT )
  s_DevFlags &= ~DEVF_USB_DISCONNECT;
 else s_DevFlags |= DEVF_USB_CONNECT;

 iWakeupThread ( s_GUIThreadID );

}  /* end _usb_handler_connect */

static void _usb_handler_disconnect ( void* apHdr ) {

 if ( s_DevFlags & DEVF_USB_CONNECT )
  s_DevFlags &= ~DEVF_USB_CONNECT;
 else s_DevFlags |= DEVF_USB_DISCONNECT;

 iWakeupThread ( s_GUIThreadID );

}  /* end _usb_handler_disconnect */

static void QueryPad0 ( void ) {

 static int s_Repeat = 0;

 int lBtn = GUI_ReadButtons ();

 if ( lBtn ) {

  if ( lBtn != s_PrevBtn ) {

   s_PrevBtn  = lBtn;
   s_Repeat   = 0;
   s_Event   |= lBtn;
   SignalSema ( s_EventSema );

  } else if ( ++s_Repeat == 8 ) {

   s_Repeat = 0;
   QueryPad = QueryPad1;

  }  /* end if */

 } else {

  s_PrevBtn = 0;
  s_Repeat  = 0;

  if ( g_Config.m_PowerOff > 0 && g_Timer >= s_PowerOffTimer ) hddPowerOff ();

 }  /* end else */

}  /* end QueryPad0 */

static void QueryPad1 ( void ) {

 unsigned int lBtn = GUI_ReadButtons ();

 if ( lBtn ) {

  if ( lBtn != s_PrevBtn ) {

   s_PrevBtn  = lBtn;
   s_Event   |= lBtn;

   QueryPad = QueryPad0;

  } else s_Event |= lBtn;

  SignalSema ( s_EventSema );

 } else s_PrevBtn = 0;

}  /* end QueryPad1 */

void GUI_UpdateStatus ( void ) {

 GUI_Status (  g_CMedia < 0 ? ( char* )STR_WAITING_FOR_MEDIA.m_pStr : g_CWD  );

}  /* end _update_status */

static int _gui_thread ( void* apParam ) {

 static int s_lCntr;

 while ( 1 ) {

  SleepThread ();

  if ( s_GUIFlags & GUIF_DEV_CHECK ) {

   DiskType lDiskType;

   if ( s_DevFlags & DEVF_USB_CONNECT ) {

    s_Event    |= ( GUI_MSG_MOUNT_BIT | GUI_MSG_USB );
    s_DevFlags &= ~DEVF_USB_CONNECT;

    goto raiseEvent;

   } else if ( s_DevFlags & DEVF_USB_DISCONNECT ) {

    s_Event    |= GUI_MSG_USB;
    s_DevFlags &= ~DEVF_USB_DISCONNECT;

    goto raiseEvent;

   } else if ( s_DevFlags & DEFV_SMB ) {

    s_Event    |= ( GUI_MSG_MOUNT_BIT | GUI_MSG_SMB );
    s_DevFlags &= ~DEFV_SMB;

    goto raiseEvent;

   }  /* end if */

   lDiskType = CDDA_DiskType ();

   if (  lDiskType == DiskType_CDDA && !( s_DevFlags & DEVF_CDDA )  ) {

    s_Event    |=  ( GUI_MSG_MOUNT_BIT | GUI_MSG_CDDA );
    s_DevFlags &= ~DEVF_CDVD_DETECT;
    s_DevFlags |=  DEVF_CDDA;

    goto raiseEvent;

   } else if (  lDiskType == DiskType_CD && !( s_DevFlags & DEVF_CD )  ) {

    s_Event    |=  ( GUI_MSG_MOUNT_BIT | GUI_MSG_CDROM );
    s_DevFlags &= ~DEVF_CDVD_DETECT;
    s_DevFlags |=  DEVF_CD;

    CDVD_SetDVDV ( 0 );

    goto raiseEvent;

   } else if (  ( lDiskType == DiskType_DVD || lDiskType == DiskType_DVDV ) && !( s_DevFlags & DEVF_DVD )  ) {

    s_Event    |=  ( GUI_MSG_MOUNT_BIT | GUI_MSG_DVD );
    s_DevFlags &= ~DEVF_CDVD_DETECT;
    s_DevFlags |=  DEVF_DVD;

    CDVD_SetDVDV ( lDiskType == DiskType_DVDV );

    goto raiseEvent;

   } else if (  lDiskType == DiskType_Detect && !( s_DevFlags & DEVF_CDVD_DETECT )  ) {

    s_DevFlags |= DEVF_CDVD_DETECT;
    GUI_Status ( STR_READING_DISK.m_pStr );

    continue;

   } else if (  lDiskType == DiskType_Unknown && !( s_DevFlags & DEVF_CDVD_UNKNOWN )  ) {

    s_DevFlags |=  DEVF_CDVD_UNKNOWN;
    s_DevFlags &= ~DEVF_CDVD_DETECT;
    GUI_Error ( STR_ILLEGAL_DISK.m_pStr );

    GUI_UpdateStatus ();

   } else if ( lDiskType == DiskType_None && ( s_DevFlags & DEVF_CDVD_MASK )  ) {

    if ( s_DevFlags & DEVF_CDVD_UNKNOWN )

     s_DevFlags &= ~DEVF_CDVD_UNKNOWN;

    else if ( s_DevFlags & DEVF_CDDA ) {

     s_DevFlags &= ~DEVF_CDDA;
     s_Event    |=  GUI_MSG_CDDA;

     goto raiseEvent;

    } else if ( s_DevFlags & DEVF_CD ) {

     s_DevFlags &= ~DEVF_CD;
     s_Event    |=  GUI_MSG_CDROM;

     goto raiseEvent;

    } else if ( s_DevFlags & DEVF_DVD ) {

     s_DevFlags &= ~DEVF_DVD;
     s_Event    |=  GUI_MSG_DVD;

     goto raiseEvent;

    } else if ( s_DevFlags & DEVF_CDVD_DETECT ) {

     s_DevFlags &= ~DEVF_CDVD_DETECT;
     GUI_UpdateStatus ();

    }  /* end if */

   }  /* end if */

   if ( g_IOPFlags & SMS_IOPF_NET ) {

    if (  !( s_lCntr++ & 0xF )  ) {

     int lFD = fioDopen ( "host:" );

     if ( lFD >= 0 ) {

      fioDclose ( lFD );

      if (  !( s_DevFlags & DEVF_HOST )  ) {

       s_Event    |= ( GUI_MSG_MOUNT_BIT | GUI_MSG_HOST );
       s_DevFlags |= DEVF_HOST;

       goto raiseEvent;

      }  /* end if */

     } else if ( s_DevFlags & DEVF_HOST ) {

       s_Event    |=  GUI_MSG_HOST;
       s_DevFlags &= ~DEVF_HOST;

       goto raiseEvent;

     }  /* end if */

    }  /* end if */

   } else if ( g_IOPFlags & SMS_IOPF_SMBLOGIN ) {

    if (  !( s_lCntr++ & 0x3F )  ) {

     int lStat, lSD = fioDopen ( g_pSMBS );

     if ( lSD >= 0 ) {

      lStat = fioIoctl ( lSD, SMB_IOCTL_ECHO, &g_SMBUnit );

      if ( lStat < 0 ) fioIoctl ( lSD, SMB_IOCTL_LOGOUT, &g_SMBUnit );

      fioDclose ( lSD );

     }  /* end if */

     if ( lSD < 0 || lStat < 0 ) {

      g_SMBU      = 0x80000000;
      g_IOPFlags &= ~SMS_IOPF_SMBLOGIN;
      s_Event    |=  GUI_MSG_SMB;

      goto raiseEvent;

     }  /* end if */

    }  /* end if */

   }  /* end if */

   goto queryPad;
raiseEvent:
   SignalSema ( s_EventSema );
   continue;

  }  /* end if */
queryPad:
  QueryPad ();

 }  /* end while */

}  /* end _gui_thread */

void GUI_SetColors ( void ) {

 int           i;
 unsigned long lColor[ 4 ];

 lColor[ 0 ] = g_Palette[ g_Config.m_BrowserSBCIdx - 1 ] | 0x80000000;
 lColor[ 1 ] = g_Palette[ g_Config.m_BrowserTxtIdx - 1 ] | 0x80000000;
 lColor[ 2 ] = ( lColor[ 1 ] & 0x00FFFFFF ) | 0x20000000;
 lColor[ 3 ] = 0x50A06060;

 for ( i = 0; i < 4; ++i ) GSContext_SetTextColor ( i, lColor[ i ] );

}  /* end GUI_SetColors */

void _set_dx_dy ( int** appDX, int** appDY ) {

 GSVideoMode lMode = g_Config.m_DisplayMode;

 if ( lMode == GSVideoMode_Default ) lMode = *( volatile char* )0x1FC7FF52 == 'E' ? GSVideoMode_PAL : GSVideoMode_NTSC;

 switch ( lMode ) {

  case GSVideoMode_NTSC:
   *appDX = &g_Config.m_DX;
   *appDY = &g_Config.m_DY;
  break;

  case GSVideoMode_Default:
  case GSVideoMode_PAL:
   *appDX = &g_Config.m_DXPALOther[ 0 ];
   *appDY = &g_Config.m_DYPALOther[ 0 ];
  break;

  case GSVideoMode_DTV_720x480P:
   *appDX = &g_Config.m_DXPALOther[ 1 ];
   *appDY = &g_Config.m_DYPALOther[ 1 ];
  break;

  case GSVideoMode_VESA_60Hz:
   *appDX = &g_Config.m_DXPALOther[ 2 ];
   *appDY = &g_Config.m_DYPALOther[ 2 ];
  break;

  case GSVideoMode_VESA_75Hz:
   *appDX = &g_Config.m_DXPALOther[ 3 ];
   *appDY = &g_Config.m_DYPALOther[ 3 ];
  break;

 }  /* end switch */

}  /* end _set_dx_dy */

void GUI_Initialize ( int afCold ) {

 int* lpDX;
 int* lpDY;

 if ( afCold ) {

  int           i;
  unsigned long lTimer;
  GSVideoMode   lVideoMode = GSVideoMode_Default;
  ee_sema_t     lSema;
  ee_thread_t   lThread;

  lSema.init_count = 0;
  lSema.max_count  = 1;
  s_EventSema = CreateSema ( &lSema );

  SMS_TimerInit ();

  PAD_Init ();
  PAD_OpenPort ( 0, 0, s_PadBuf0 );
  MC_Init  ();

  SMS_LoadConfig ();
  SMS_LocaleInit ();
  SMS_LocaleSet  ();

  i      = PAD_State ( 0, 0 );
  lTimer = g_Timer;

  while (  i != SMS_PAD_STATE_STABLE && i != SMS_PAD_STATE_FINDCTP1 ) {

   i = PAD_State ( 0, 0 );

   if ( g_Timer - lTimer > 2000 ) break;

  }  /* end while */

  PAD_SetMainMode ( 0, 0, SMS_PAD_MMODE_DIGITAL, SMS_PAD_MMODE_LOCK );

  lTimer = g_Timer;

  while ( 1 ) {

   i = GUI_ReadButtons2 ();

   if (   (  i & ( SMS_PAD_SELECT | SMS_PAD_R1 )  ) == ( SMS_PAD_SELECT | SMS_PAD_R1 )   ) {

    lVideoMode = GSVideoMode_NTSC;
    break;

   } else if (   (  i & ( SMS_PAD_SELECT | SMS_PAD_R2 )  ) == ( SMS_PAD_SELECT | SMS_PAD_R2 )   ) {

    lVideoMode = GSVideoMode_PAL;
    break;

   } else if ( g_Timer - lTimer > 1000 ) break;

  }  /* end while */

  if ( lVideoMode != GSVideoMode_Default ) g_Config.m_DisplayMode = lVideoMode;

  g_GSCtx.m_CodePage = g_Config.m_DisplayCharset;
  s_pObjectList      = SMS_ListInit ();
  s_pMsgQueue        = SMS_ListInit ();
  QueryPad           = QueryPad0;

  GUI_AddObject (  g_DesktopStr, g_pDesktop    = GUI_CreateDesktop    ()  );
  GUI_AddObject (  g_pCmdPrcStr,                 GUI_CreateCmdProc    ()  );
  GUI_AddObject (  g_StatuslStr, g_pStatusLine = GUI_CreateStatusLine ()  );
  GUI_AddObject (  g_DevMenuStr, g_pDevMenu    = GUI_CreateDevMenu    ()  );
  GUI_AddObject (  g_FilMenuStr, g_pFileMenu   = GUI_CreateFileMenu   ()  );

  lThread.stack_size       = sizeof ( s_Stack );
  lThread.stack            = s_Stack;
  lThread.gp_reg           = &_gp;
  lThread.initial_priority = 100;
  lThread.func             = _gui_thread;
  StartThread (  s_GUIThreadID = CreateThread ( &lThread ), NULL  );

 }  /* end if */

 if ( g_Config.m_DisplayMode != GSVideoMode_PAL          &&
      g_Config.m_DisplayMode != GSVideoMode_NTSC         &&
      g_Config.m_DisplayMode != GSVideoMode_DTV_720x480P &&
      g_Config.m_DisplayMode != GSVideoMode_VESA_60Hz    &&
      g_Config.m_DisplayMode != GSVideoMode_VESA_75Hz
 ) g_Config.m_DisplayMode = GSVideoMode_Default;

 _set_dx_dy ( &lpDX, &lpDY );

 g_GSCtx.m_OffsetX  = *lpDX;
 g_GSCtx.m_OffsetY  = *lpDY;
 g_GSCtx.m_CodePage = g_Config.m_DisplayCharset;

 GS_VSync ();
 GSContext_Init ( g_Config.m_DisplayMode, GSZTest_Off, GSDoubleBuffer_On );
 GS_VSync ();

 GUI_SetColors ();
 GUI_Redraw ( GUIRedrawMethod_InitClearAll );

}  /* end GUI_Initialize */

void GUI_Suspend ( void ) {

 SMS_TimerReset ( 0, NULL );
 SuspendThread ( s_GUIThreadID );

}  /* end GUI_Suspend */

void GUI_Resume ( void ) {

 ResumeThread ( s_GUIThreadID );
 SMS_TimerSet ( 0, 64, TimerHandler, NULL );

}  /* end GUI_Resume */

unsigned long GUI_WaitEvent ( void ) {

 unsigned long retVal = 0UL;

 GUI_Resume ();

 while ( !retVal ) {

  s_PowerOffTimer = g_Timer + g_Config.m_PowerOff;

  WaitSema ( s_EventSema );

  if ( s_Event & GUI_MSG_MOUNT_MASK ) {

   retVal   = s_Event & GUI_MSG_MOUNT_MASK;
   s_Event &= ~GUI_MSG_MOUNT_MASK;

  } else if ( s_pMsgQueue -> m_Size ) {

   retVal = s_pMsgQueue -> m_pHead -> m_Param;
   SMS_ListPop ( s_pMsgQueue );

  } else if ( s_Event & GUI_MSG_PAD_MASK ) {

   retVal   = s_Event & GUI_MSG_PAD_MASK;
   s_Event &= ~GUI_MSG_PAD_MASK;

  }  /* end if */

 }  /* end while */

 GUI_Suspend ();

 return retVal;

}  /* end GUI_WaitEvent */

int GUI_WaitButtons ( int anButtons, unsigned* apButtons, int aTimeout ) {

 int           i, retVal;
 unsigned long lTime;

 lTime = g_Timer + aTimeout;

 while ( 1 ) {

  retVal = GUI_ReadButtons ();

  if ( g_Timer < lTime ) continue;

  for ( i = 0; i < anButtons; ++i ) if ( retVal == apButtons[ i ] ) goto end;

 }  /* end while */
end:
 while (  GUI_ReadButtons ()  );

 return retVal;

}  /* end GUI_WaitButtons */

void GUI_PostMessage ( unsigned long aMsg ) {

 SMS_ListPushBack ( s_pMsgQueue, s_pMsgStr ) -> m_Param = aMsg;

 SignalSema ( s_EventSema );

}  /* end GUI_PostMessage */

void GUI_DeleteObject ( const unsigned char* apName ) {

 SMS_ListNode* lpNode = SMS_ListFind ( s_pObjectList, apName );

 if ( lpNode ) {

  GUIObject* lpObj = ( GUIObject* )( unsigned int )lpNode -> m_Param;

  lpObj -> Cleanup ( lpObj );
  free ( lpObj );

  SMS_ListRemove ( s_pObjectList, lpNode );

  if ( g_pActiveNode == lpNode ) g_pActiveNode = s_pObjectList -> m_pTail;

 }  /* end if */

}  /* end GUI_DeleteObject */

void GUI_Run ( void ) {

 static int s_lLevel = 0;

 unsigned long lEvent;

 if ( !s_lLevel++ ) {

  s_GUIFlags |= GUIF_DEV_CHECK;
  g_CMedia    = -1;

  SMS_IOPSetSifCmdHandler ( _smb_handler_connect,    0 );
  SMS_IOPSetSifCmdHandler ( _usb_handler_connect,    1 );
  SMS_IOPSetSifCmdHandler ( _usb_handler_disconnect, 2 );

  GUI_UpdateStatus ();

  if ( g_IOPFlags & SMS_IOPF_HDD ) GUI_PostMessage ( GUI_MSG_MOUNT_BIT | GUI_MSG_HDD   );
  if ( g_IOPFlags & SMS_IOPF_SMB ) GUI_PostMessage ( GUI_MSG_MOUNT_BIT | GUI_MSG_LOGIN );

 } else s_GUIFlags &= ~GUIF_DEV_CHECK;

 while ( 1 ) {

  GUIHResult lHRes = GUIHResult_Void;
  GUIObject* lpObj = ( GUIObject* )( unsigned int )(  ( SMS_ListNode* )g_pActiveNode  ) -> m_Param;

  if (   (  lEvent = GUI_WaitEvent ()  ) == GUI_MSG_QUIT   ) break;

  if ( lpObj -> HandleEvent ) lHRes = lpObj -> HandleEvent ( lpObj, lEvent );

  if ( lHRes == GUIHResult_Handled )

   continue;

  else if ( lHRes == GUIHResult_ChangeFocus ) {

   SMS_ListNode* lpNode = (  ( SMS_ListNode* )g_pActiveNode  ) -> m_pNext;

   if ( !lpNode ) lpNode = s_pObjectList -> m_pHead;

   lpObj -> SetFocus ( lpObj, 0 );

   while ( lpNode != g_pActiveNode ) {

    lpObj = ( GUIObject* )( unsigned int )lpNode -> m_Param;

    if ( lpObj -> SetFocus ) {

     lpObj -> SetFocus ( lpObj, 1 );
     g_pActiveNode = lpNode;

     break;

    }  /* end if */

    lpNode = lpNode -> m_pNext;

    if ( !lpNode ) lpNode = s_pObjectList -> m_pHead;

   }  /* end while */

  } else {

   SMS_ListNode* lpNode = (  ( SMS_ListNode* )g_pActiveNode  ) -> m_pNext;

   if ( !lpNode ) lpNode = s_pObjectList -> m_pHead;

   while ( lpNode != g_pActiveNode ) {

    lpObj = ( GUIObject* )( unsigned int )lpNode -> m_Param;

    if (  lpObj -> HandleEvent && lpObj -> HandleEvent ( lpObj, lEvent ) == GUIHResult_Handled  ) break;

    lpNode = lpNode -> m_pNext;

    if ( !lpNode ) lpNode = s_pObjectList -> m_pHead;

   }  /* end while */

  }  /* end else */

 }  /* end while */

 if ( --s_lLevel == 1 ) s_GUIFlags |= GUIF_DEV_CHECK;

}  /* end GUI_Run */
