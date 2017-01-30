/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2007 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Player_H
# define __SMS_Player_H

# ifndef __SMS_Container_H
#  include "SMS_Container.h"
# endif  /* __SMS_Container_H */

# ifndef __SMS_Locale_H
#  include "SMS_Locale.h"
# endif  /* __SMS_Locale_H */

# define SMS_FLAGS_STOP      0x00000001
# define SMS_FLAGS_PAUSE     0x00000002
# define SMS_FLAGS_MENU      0x00000004
# define SMS_FLAGS_EXIT      0x00000008
# define SMS_FLAGS_VSCROLL   0x00000010
# define SMS_FLAGS_ASCROLL   0x00000020
# define SMS_FLAGS_AASCROLL  0x00000040
# define SMS_FLAGS_ABSCROLL  0x00000080
# define SMS_FLAGS_USER_STOP 0x00000100
# define SMS_FLAGS_DISPCTL   0x00000200
# define SMS_FLAGS_SPDIF     0x00000400
# define SMS_FLAGS_AC3       0x00000800
# define SMS_FLAGS_DXSB      0x00001000
# define SMS_FLAGS_SUBS      0x00002000
# define SMS_FLAGS_PDW22     0x00004000
# define SMS_FLAGS_DYNMSK    ( SMS_FLAGS_STOP      | SMS_FLAGS_PAUSE   | SMS_FLAGS_MENU     | SMS_FLAGS_EXIT     | \
                               SMS_FLAGS_VSCROLL   | SMS_FLAGS_ASCROLL | SMS_FLAGS_AASCROLL | SMS_FLAGS_ABSCROLL | \
                               SMS_FLAGS_USER_STOP | SMS_FLAGS_DISPCTL                                             \
                             )

typedef struct SMS_Player {

 SMS_Container*           m_pCont;
 struct FileContext*      m_pFileCtx;
 struct FileContext*      m_pSubFileCtx;
 struct IPUContext*       m_pIPUCtx;
 struct SPUContext*       m_pSPUCtx;
 struct SMS_Codec*        m_pVideoCodec;
 struct SMS_Codec*        m_pAudioCodec;
 struct SMS_CodecContext* m_pAudioCodecCtx;
 struct SubtitleContext*  m_pSubCtx;
 struct SMS_ListNode*     m_pPlayItem;
 unsigned int             m_Flags;
 unsigned int             m_PanScan;
 unsigned int             m_SubFormat;
 unsigned int             m_OSD;
          int             m_VideoIdx;
          int             m_AudioIdx;
          int             m_PrevAudioIdx;
 long     int             m_VideoTime;
 long     int             m_AudioTime;
 unsigned long int*       m_OSDPackets[ 10 ];
 unsigned int             m_OSDQWC    [ 10 ];
 int                      m_AVDelta;
 int                      m_SVDelta;
 int                      m_OSDPLPos;
 int                      m_OSDPLRes;
 int                      m_OSDPLInc;
 int                      m_AudioSampleRate;
 int                      m_AudioChannels;
 int                      m_PlayItemNr;
 int                      m_EOF;
 short*                   m_pAudioSamples;
 unsigned int             m_StartPos;
          int             m_SubIdx;
 SMString*                m_pErrorMsg;
          int             m_PDW22Ratio;
          int             m_PDW22Base;

 int  ( *Play      ) ( void* );
 void ( *Destroy   ) ( void* );
 void ( *SetColors ) ( void* );

} SMS_Player;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Player* SMS_InitPlayer ( struct FileContext*, struct FileContext*, unsigned int );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Player_H */
