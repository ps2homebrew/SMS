/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on ffmpeg project (no copyright notes in the original source code)
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_AVI_H
# define __SMS_AVI_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

# ifndef __SMS_Codec_H
#  include "SMS_Codec.h"
# endif  /* __SMS_Codec_H */

# define SMS_PKT_FLAG_KEY 0x00000001

struct FileContext;
struct GSContext;

typedef struct SMS_AVIdxEntry {

 uint32_t m_Flags;
 uint32_t m_Pos;
 uint32_t m_Len;
 uint32_t m_CumLen;

} SMS_AVIdxEntry;

typedef struct SMS_AVIStream {

 int64_t          m_StartTime;
 int64_t          m_Duration;
 int64_t          m_CurDTS;
 SMS_AVIdxEntry*  m_pIdx;
 uint32_t         m_IdxAllocSize;
 uint32_t         m_nIdx;
 int32_t          m_Idx;
 int32_t          m_ID;
 int32_t          m_Rate;
 int32_t          m_Scale;
 int32_t          m_PTSWrapBits;
 int32_t          m_Start;
 int32_t          m_SampleSize;
 int32_t          m_SampleRate;
 uint32_t         m_CumLen;
 uint32_t         m_RealFrameRate;
 uint32_t         m_RealFrameRateBase;
 uint32_t         m_FrameOffset;
 SMS_Rational     m_TimeBase;
 SMS_CodecContext m_Codec;

} SMS_AVIStream;

typedef struct SMS_AVIContext {

 int64_t             m_StartTime;
 int64_t             m_Duration;
 uint32_t            m_BitRate;
 uint32_t            m_RiffEnd;
 uint32_t            m_MoviEnd;
 uint32_t            m_MoviList;
 uint32_t            m_FileSize;
 SMS_AVIStream*      m_pStm[ 2 ];
 uint32_t            m_nStm;
 struct FileContext* m_pFileCtx;
 void                ( *Destroy ) ( struct SMS_AVIContext* );

} SMS_AVIContext;

typedef struct SMS_AVIPacket {

 int64_t         m_PTS;
 int64_t         m_DTS;
 uint8_t*        m_pData;
 uint32_t        m_Size;
 uint32_t        m_AllocSize;
 uint32_t        m_StmIdx;
 uint32_t        m_Flags;
 int32_t         m_Duration;
 SMS_AVIContext* m_pCtx;
 void            ( *Destroy ) ( struct SMS_AVIPacket* );

} SMS_AVIPacket;

typedef struct SMS_AVIPlayer {

 SMS_AVIContext*     m_pAVICtx;
 struct GSContext*   m_pGSCtx;
 struct FileContext* m_pFileCtx;

 void ( *Play    ) ( void );
 void ( *Destroy ) ( void );

} SMS_AVIPlayer;
# ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

SMS_AVIContext* SMS_AVINewContext    ( struct FileContext*                    );
int             SMS_AVIProbeFile     ( SMS_AVIContext*                        );
int             SMS_AVIReadHeader    ( SMS_AVIContext*                        );
void            SMS_AVICalcFrameRate ( SMS_AVIContext*                        );
int             SMS_AVIReadPacket    ( SMS_AVIPacket*                         );
SMS_AVIPacket*  SMS_AVINewPacket     ( SMS_AVIContext*                        );
void            SMS_AVIPrintInfo     ( SMS_AVIContext*                        );
SMS_AVIPlayer*  SMS_AVIInitPlayer    ( struct FileContext*, struct GSContext* );
# ifdef __cplusplus
};
# endif  /* __cplusplus */
#endif  /* __SMS_AVI_H */
