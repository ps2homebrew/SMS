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
#ifndef __SMS_VideoBuffer_H
# define __SMS_VideoBuffer_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

typedef struct SMS_FrameBuffer {

 long                    m_StartPTS;
 long                    m_EndPTS;
 SMS_MacroBlock*         m_pBase;
 SMS_MacroBlock*         m_pData;
 int                     m_FrameType;
 unsigned long*          m_pCSCDma;
 struct SMS_FrameBuffer* m_pNext;

} SMS_FrameBuffer;

typedef struct SMS_CSCParam {

 short m_nBlk[ 2 ];
 short m_nRem[ 2 ];
 short m_QWCB;
 short m_QWCR;
 int   m_CSCmd;
 int   m_HandlerID;

} SMS_CSCParam;

typedef struct SMS_VideoBuffer {

 SMS_FrameBuffer  m_VFrm[ 3 ];
 SMS_FrameBuffer* m_pFree;
 SMS_CSCParam     m_CSCParam;

} SMS_VideoBuffer;

# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

SMS_VideoBuffer* SMS_VideoBufferInit    ( int, int                             );
void             SMS_VideoBufferDestroy ( SMS_VideoBuffer*                     );
void             SMS_CSCResume          ( void                                 );
void             SMS_CSCSuspend         ( void                                 );
void             SMS_CSCSync            ( void                                 );
void             SMS_CSC                ( SMS_CSCParam*, unsigned long*, void* );
void             SMS_FrameBufferInit    ( SMS_FrameBuffer*, int, int, int, int );
void             SMS_FrameBufferDestroy ( SMS_FrameBuffer*, int                );

# ifdef __cplusplus
}
# endif  /* __cplusplus */

#endif  /* __SMS_VideoBuffer_H */
