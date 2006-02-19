/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 200X ps2dev -> http://www.ps2dev.org
# Adopted for SMS in 2005/6 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_PAD_H
# define __SMS_PAD_H

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

# define SMS_PAD_LEFT      0x0080
# define SMS_PAD_DOWN      0x0040
# define SMS_PAD_RIGHT     0x0020
# define SMS_PAD_UP        0x0010
# define SMS_PAD_START     0x0008
# define SMS_PAD_R3        0x0004
# define SMS_PAD_L3        0x0002
# define SMS_PAD_SELECT    0x0001
# define SMS_PAD_SQUARE    0x8000
# define SMS_PAD_CROSS     0x4000
# define SMS_PAD_CIRCLE    0x2000
# define SMS_PAD_TRIANGLE  0x1000
# define SMS_PAD_R1        0x0800
# define SMS_PAD_L1        0x0400
# define SMS_PAD_R2        0x0200
# define SMS_PAD_L2        0x0100

# define SMS_PAD_STATE_DISCONN  0x00
# define SMS_PAD_STATE_FINDPAD  0x01
# define SMS_PAD_STATE_FINDCTP1 0x02
# define SMS_PAD_STATE_EXECCMD  0x05
# define SMS_PAD_STATE_STABLE   0x06
# define SMS_PAD_STATE_ERROR    0x07

# define SMS_PAD_RSTAT_COMPLETE 0x00
# define SMS_PAD_RSTAT_FAILED   0x01
# define SMS_PAD_RSTAT_BUSY     0x02

# define SMS_PAD_MMODE_DIGITAL   0
# define SMS_PAD_MMODE_DUALSHOCK 1

# define SMS_PAD_MMODE_UNLOCK 0
# define SMS_PAD_MMODE_LOCK   3

typedef struct SMS_PadButtonStatus {

 unsigned char  m_OK           __attribute__(  ( packed )  );
 unsigned char  m_Mode         __attribute__(  ( packed )  );
 unsigned short m_Btns         __attribute__(  ( packed )  );
 unsigned char  m_RJoyH        __attribute__(  ( packed )  );
 unsigned char  m_RJoyV        __attribute__(  ( packed )  );
 unsigned char  m_LJoyH        __attribute__(  ( packed )  );
 unsigned char  m_LJoyV        __attribute__(  ( packed )  );
 unsigned char  m_RightP       __attribute__(  ( packed )  );
 unsigned char  m_LeftP        __attribute__(  ( packed )  );
 unsigned char  m_UpP          __attribute__(  ( packed )  );
 unsigned char  m_DownP        __attribute__(  ( packed )  );
 unsigned char  m_TriangleP    __attribute__(  ( packed )  );
 unsigned char  m_CircleP      __attribute__(  ( packed )  );
 unsigned char  m_CrossP       __attribute__(  ( packed )  );
 unsigned char  m_SquareP      __attribute__(  ( packed )  );
 unsigned char  m_L1P          __attribute__(  ( packed )  );
 unsigned char  m_R1P          __attribute__(  ( packed )  );
 unsigned char  m_L2P          __attribute__(  ( packed )  );
 unsigned char  m_R2P          __attribute__(  ( packed )  );
 unsigned char  m_Unkn16[ 12 ] __attribute__(  ( packed )  );

} SMS_PadButtonStatus;

int PAD_Init ( void );
int PAD_Quit ( void );

int PAD_OpenPort  ( int, int, void* );
int PAD_ClosePort ( int, int        );

unsigned char  PAD_ReqState    ( int, int           );
void           PAD_SetReqState ( int, int, int      );
int            PAD_State       ( int, int           );
unsigned short PAD_Read        ( int, int           );
int            PAD_SetMainMode ( int, int, int, int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_PAD_H */
