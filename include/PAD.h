#ifndef __PAD_H
# define __PAD_H

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

# define PAD_LEFT      0x0080
# define PAD_DOWN      0x0040
# define PAD_RIGHT     0x0020
# define PAD_UP        0x0010
# define PAD_START     0x0008
# define PAD_R3        0x0004
# define PAD_L3        0x0002
# define PAD_SELECT    0x0001
# define PAD_SQUARE    0x8000
# define PAD_CROSS     0x4000
# define PAD_CIRCLE    0x2000
# define PAD_TRIANGLE  0x1000
# define PAD_R1        0x0800
# define PAD_L1        0x0400
# define PAD_R2        0x0200
# define PAD_L2        0x0100

# define PAD_STATE_DISCONN  0x00
# define PAD_STATE_FINDPAD  0x01
# define PAD_STATE_FINDCTP1 0x02
# define PAD_STATE_EXECCMD  0x05
# define PAD_STATE_STABLE   0x06
# define PAD_STATE_ERROR    0x07

# define PAD_RSTAT_COMPLETE 0x00
# define PAD_RSTAT_FAILED   0x01
# define PAD_RSTAT_BUSY     0x02

# define PAD_MMODE_DIGITAL   0
# define PAD_MMODE_DUALSHOCK 1

# define PAD_MMODE_UNLOCK 0
# define PAD_MMODE_LOCK   3

typedef struct PadButtonStatus {

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

} PadButtonStatus;

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
#endif  /* __PAD_H */
