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
# define __SMS_IOP_H

# define SMS_IOPF_DEV9   0x00000001
# define SMS_IOPF_HDD    0x00000002
# define SMS_IOPF_NET    0x00000004
# define SMS_IOPF_USB    0x00000008
# define SMS_IOPF_RMMAN  0x00000010
# define SMS_IOPF_RMMAN2 0x00000020

extern unsigned int g_IOPFlags;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

int  SMS_IOPExec             ( int, void*                             );
void SMS_IOPReset            ( int                                    );
void SMS_IOPInit             ( void                                   );
void SMS_IOPSetSifCmdHandler (  void ( *apFunc ) ( void* ), int aCmd  );
int  SMS_IOPStartNet         ( void                                   );
int  SMS_IOPStartUSB         ( void                                   );
int  SMS_IOPStartHDD         ( void                                   );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_IOP_H */
