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
#ifndef __SMS_VideoBuffer_H
# define __SMS_VideoBuffer_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

struct SMS_VideoBuffer;

typedef struct SMS_FrameBuffer {

 SMS_MacroBlock*         m_pBase;
 SMS_MacroBlock*         m_pData;
 int                     m_FrameType;
 float                   m_PTS;
 struct SMS_VideoBuffer* m_pBuf;

} SMS_FrameBuffer;

typedef struct SMS_VideoBuffer {

 SMS_FrameBuffer* m_pBeg;
 SMS_FrameBuffer* m_pEnd;
 SMS_FrameBuffer* m_pInp;
 SMS_FrameBuffer* m_pOut;
 int              m_Linesize;
 int              m_nAlloc;

 SMS_FrameBuffer* ( *Alloc   ) ( void             );
 void             ( *Release ) ( SMS_FrameBuffer* );
 void             ( *Destroy ) ( void             );

} SMS_VideoBuffer;

SMS_VideoBuffer* SMS_InitVideoBuffer ( int, int );
#endif  /* __SMS_VideoBuffer_H */
