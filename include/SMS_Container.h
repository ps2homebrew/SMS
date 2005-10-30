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
#ifndef __SMS_Container_H
# define __SMS_Container_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# ifndef __FileContext_H
#  include "FileContext.h"
# endif  /* __FileContext_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

struct GUIContext;

# define SMS_CONT_FLAGS_SEEKABLE 0x00000001

typedef struct SMS_Stream {

 SMS_CodecContext m_Codec;
 int32_t          m_SampleRate;
 uint32_t         m_RealFrameRate;
 uint32_t         m_RealFrameRateBase;
 SMS_Rational     m_TimeBase;
 void*            m_pCtx;

 void ( *Destroy ) ( struct SMS_Stream* );

} SMS_Stream;

typedef struct SMS_Container {

 SMS_Stream*        m_pStm[ 32 ];
 uint64_t           m_Duration;
 uint32_t           m_nStm;
 uint32_t           m_Flags;
 FileContext*       m_pFileCtx;
 char*              m_pName;
 void*              m_pCtx;
 struct GUIContext* m_pGUICtx;

 SMS_AVPacket* ( *NewPacket  ) ( struct SMS_Container*                     );
 int           ( *ReadPacket ) ( SMS_AVPacket*                             );
 void          ( *Destroy    ) ( struct SMS_Container*                     );
 int           ( *Seek       ) ( struct SMS_Container*, int, int, uint32_t );

} SMS_Container;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Container* SMS_GetContainer        ( FileContext*, struct GUIContext* );
void           SMSContainer_SetPTSInfo ( SMS_Stream*, int, int            );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Container_H */
