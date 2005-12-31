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
struct StringList;
struct StringListNode;

# define SMS_CONT_FLAGS_SEEKABLE 0x00000001

# define SMS_STRM_FLAGS_AUDIO 0x00000001
# define SMS_STRM_FLAGS_VIDEO 0x00000002

# define SMS_MAX_STREAMS 8

typedef struct SMS_Stream {

 SMS_Rational      m_TimeBase;
 int32_t           m_SampleRate;
 uint32_t          m_RealFrameRate;
 uint32_t          m_RealFrameRateBase;
 uint32_t          m_Flags;
 SMS_CodecContext* m_pCodec;
 char*             m_pName;
 void*             m_pCtx;

 void ( *Destroy ) ( struct SMS_Stream* );

} SMS_Stream;

typedef struct SMS_Container {

 SMS_Stream*            m_pStm[ SMS_MAX_STREAMS ];
 int64_t                m_Duration;
 uint32_t               m_nStm;
 uint32_t               m_Flags;
 FileContext*           m_pFileCtx;
 char*                  m_pName;
 void*                  m_pCtx;
 struct GUIContext*     m_pGUICtx;
 struct StringList*     m_pPlayList;
 struct StringListNode* m_pPlayItem;

 SMS_AVPacket* ( *NewPacket   ) ( struct SMS_Container*                     );
 int           ( *ReadPacket  ) ( SMS_AVPacket*                             );
 void          ( *Destroy     ) ( struct SMS_Container*                     );
 int           ( *Seek        ) ( struct SMS_Container*, int, int, uint32_t );

} SMS_Container;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_Container* SMS_GetContainer        ( FileContext*, struct GUIContext* );
void           SMS_DestroyContainer    ( SMS_Container*                   );
void           SMSContainer_SetPTSInfo ( SMS_Stream*, int, int            );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SMS_Container_H */
