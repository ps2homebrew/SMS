/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Player_H
# define __SMS_Player_H

# ifndef __SMS_Container_H
#  include "SMS_Container.h"
# endif  /* __SMS_Container_H */

struct FileContext;
struct GUIContext;
struct IPUContext;
struct SPUContext;
struct SMSCodec;

typedef struct SMS_Player {

 SMS_Container*      m_pCont;
 struct GUIContext*  m_pGUICtx;
 struct FileContext* m_pFileCtx;
 struct IPUContext*  m_pIPUCtx;
 struct SPUContext*  m_pSPUCtx;
 struct SMS_Codec*   m_pVideoCodec;
 struct SMS_Codec*   m_pAudioCodec;
 int                 m_Volume;
 unsigned int        m_VideoIdx;
 unsigned int        m_AudioIdx;
 float               m_VideoTime;
 float               m_AudioTime;

 void ( *Play    ) ( void );
 void ( *Destroy ) ( void );

} SMS_Player;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Player* SMS_InitPlayer ( struct FileContext*, struct GUIContext* );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Player_H */
