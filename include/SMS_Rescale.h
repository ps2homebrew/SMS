/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2009 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_Rescale_H
#define __SMS_Rescale_H

typedef struct _res_param {

 int   m_iDelta[ 2 ];
 float m_fDelta[ 2 ];

} _res_param;

typedef struct SMS_RescaleContext {

 float          m_Scale;          /*  0 */
 unsigned int   m_Width;          /*  4 */
 unsigned int   m_Height;         /*  8 */
 unsigned int   m_NewWidth;       /* 12 */
 unsigned int   m_NewHeight;      /* 16 */
 unsigned short m_StripeH;        /* 20 */
 unsigned short m_Stride;         /* 22 */
 unsigned int   m_nStripes;       /* 24 */
 unsigned int   m_nAlloc;         /* 28 */
 unsigned int   m_nStripeAlloc;   /* 32 */
 void*          m_pStripePtr;     /* 36 */
 void*          m_pStripe;        /* 40 */
 void*          m_pTempPtr;       /* 44 */
 void*          m_pTemp;          /* 48 */
 int            m_Y;              /* 52 */
 _res_param*    m_pCurY;          /* 56 */

 void ( *ProcessStripe ) ( struct SMS_RescaleContext*        ); /* 60 */
 void ( *ProcessBuffer ) ( struct SMS_RescaleContext*, void* ); /* 64 */
 
 _res_param m_XParam[ 1024 ]; /*    68 */
 _res_param m_YParam[ 1024 ]; /* 16452 */

 unsigned int m_Pad;

} SMS_RescaleContext;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

SMS_RescaleContext* SMS_RescaleInit    ( SMS_RescaleContext*, int, int, int, int );
void                SMS_RescaleDestroy ( SMS_RescaleContext*                     );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __SMS_Rescale_H */
