#ifndef __Config_H
# define __Config_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# define SMS_BF_SKIN 0x00000001
# define SMS_BF_SORT 0x00000002
# define SMS_BF_AVIF 0x00000004
# define SMS_BF_HDLP 0x00000008
# define SMS_BF_SYSP 0x00000010
# define SMS_BF_MENU 0x00000020
# define SMS_BF_DISK 0x00000040

# define SMS_NF_AUTO 0x00000001

typedef struct SMSConfig {
/* Version 0 fields - 272 bytes */
 int          m_Version;
 int          m_DX;
 int          m_DY;
 int          m_DisplayMode;
 char         m_Partition[ 256 ];
/* Version 1 fields - 32 bytes */
 unsigned int m_BrowserABCIdx;
 unsigned int m_BrowserIBCIdx;
 unsigned int m_BrowserFlags;
 unsigned int m_BrowserTxtIdx;
 unsigned int m_NetworkFlags;
 unsigned int m_DisplayCharset;
 unsigned int m_PlayerVolume;
 unsigned int m_ResMode;

} SMSConfig;

extern SMSConfig     g_Config;
extern unsigned int  g_Palette[ 16 ] __attribute__(   (  section( ".data" )  )   );
extern unsigned char g_IconSMS[ 33688 ];

int LoadConfig ( void );
int SaveConfig ( void );
#endif  /* __Config_H */
