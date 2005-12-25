/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 BraveDog
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SubtitleContext_H
# define __SubtitleContext_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

struct IPUContext;
struct FileContext;
struct GSContext;

typedef struct SubtitlePacket {

 int64_t      m_Begin;
 int64_t      m_End;
 uint64_t*    m_pDMA;
 unsigned int m_QWC;
 unsigned int m_Pad[ 2 ];

} SubtitlePacket;

typedef struct SubtitleContext {

 int64_t         m_Delay;
 unsigned int    m_Idx;
 unsigned int    m_Cnt;
 SubtitlePacket* m_pPackets;
 uint64_t*       m_pDMA;
 unsigned int    m_ErrorCode;
 unsigned int    m_ErrorLine;

 void ( *Prepare ) ( void    );
 void ( *Display ) ( int64_t );
 void ( *Destroy ) ( void    );

} SubtitleContext;

typedef enum SubtitleFormat {
 SubtitleFormat_SRT,
 SubtitleFormat_SUB
} SubtitleFormat;

typedef enum SubtitleError {
 SubtitleError_Format   = 1,
 SubtitleError_Sequence = 2
} SubtitleError;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SubtitleContext* SubtitleContext_Init ( struct FileContext*, SubtitleFormat, float );

# ifdef __cplusplus
}
# endif  /* __cplusplus */
#endif  /* __SubtitleContext_H */
