#ifndef __Config_H
# define __Config_H

typedef struct SMSConfig {

 int  m_Version;
 int  m_DX;
 int  m_DY;
 int  m_DisplayMode;
 char m_Partition[ 256 ];

} SMSConfig;

extern SMSConfig     g_Config;
extern unsigned char g_IconSMS[ 33688 ];

int LoadConfig ( void );
int SaveConfig ( void );
#endif  /* __Config_H */
