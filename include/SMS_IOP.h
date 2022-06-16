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
#ifndef __SMS_IOP_H
#define __SMS_IOP_H

#define SMS_IOPF_DEV9     0x00000001
#define SMS_IOPF_HDD      0x00000002
#define SMS_IOPF_NET      0x00000004
#define SMS_IOPF_USB      0x00000008
#define SMS_IOPF_RMMAN    0x00000010
#define SMS_IOPF_RMMAN2   0x00000020
#define SMS_IOPF_SMB      0x00000040
#define SMS_IOPF_SMBINFO  0x00000080
#define SMS_IOPF_SMBLOGIN 0x00000100
#define SMS_IOPF_NET_UP   0x00000200
#define SMS_IOPF_EURO     0x00000400
#define SMS_IOPF_DEV9_IS  0x00000800
#define SMS_IOPF_UMS      0x00001000

#define SMS_SIF_CMD_SMB_CONNECT    0
#define SMS_SIF_CMD_USB_CONNECT    1
#define SMS_SIF_CMD_USB_DISCONNECT 2
#define SMS_SIF_CMD_PRINTF         3
#define SMS_SIF_CMD_NETWORK        4
#define SMS_SIF_CMD_POWEROFF       5

extern unsigned int g_IOPFlags;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SMS_IOPReset            ( int                                    );
void SMS_IOPInit             ( void                                   );
void SMS_IOPSetSifCmdHandler (  void ( *apFunc ) ( void* ), int aCmd  );
int  SMS_IOPStartNet         ( int                                    );
int  SMS_IOPStartUSB         ( int                                    );
int  SMS_IOPStartHDD         ( int                                    );
void SMS_IOPSetXLT           ( void                                   );
int  SMS_IOCtl               ( const char*, int, void*                );
void SMS_IOPowerOff          ( void                                   );
int  SMS_HDDMount            ( const char*, const char*, int          );
int  SMS_HDDUMount           ( const char*, const char*               );

int SMS_IOPQueryTotalFreeMemSize ( void );
int SMS_IOPDVDVInit              ( void );

#ifdef __cplusplus
}
#endif  /* __cplusplus */
#endif  /* __SMS_IOP_H */
