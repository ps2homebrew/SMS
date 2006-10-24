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
struct IPUContext;
struct SPUContext;
struct SMSCodec;
struct SubtitleContext;
struct SMS_ListNode;

typedef struct SMS_Player {

 SMS_Container*          m_pCont;
 struct FileContext*     m_pFileCtx;
 struct FileContext*     m_pSubFileCtx;
 struct IPUContext*      m_pIPUCtx;
 struct SPUContext*      m_pSPUCtx;
 struct SMS_Codec*       m_pVideoCodec;
 struct SMS_Codec*       m_pAudioCodec;
 struct SubtitleContext* m_pSubCtx;
 struct SMS_ListNode*    m_pPlayItem;
 unsigned int            m_Flags;
 unsigned int            m_PanScan;
 unsigned int            m_SubFormat;
 unsigned int            m_OSD;
 unsigned int            m_VideoIdx;
 unsigned int            m_AudioIdx;
 unsigned long int       m_VideoTime;
 unsigned long int       m_AudioTime;
 unsigned long int*      m_OSDPackets[ 8 ];
 unsigned int            m_OSDQWC    [ 8 ];
 int                     m_AVDelta;
 int                     m_SVDelta;
 int                     m_OSDPLPos;
 int                     m_OSDPLRes;
 int                     m_OSDPLInc;
 int                     m_AudioSampleRate;
 int                     m_AudioChannels;
 int                     m_PlayItemNr;
 short*                  m_pAudioSamples;
 unsigned int            m_StartPos;

 void ( *Play      ) ( void );
 void ( *Destroy   ) ( void );
 void ( *SetColors ) ( void );

} SMS_Player;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Player* SMS_InitPlayer ( struct FileContext*, struct FileContext*, unsigned int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Player_H */
