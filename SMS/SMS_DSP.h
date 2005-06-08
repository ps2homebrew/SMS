/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DSP_H
# define __SMS_DSP_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

typedef void ( *SMS_OpPixFunc  ) ( uint8_t*, const uint8_t*, int, int   );
typedef void ( *SMS_QPelMCFunc ) ( uint8_t*, uint8_t*, int              );

extern const uint8_t g_SMS_DSP_zigzag_direct            [ 64 ];
extern const uint8_t g_SMS_DSP_alternate_horizontal_scan[ 64 ];
extern const uint8_t g_SMS_DSP_alternate_vertical_scan  [ 64 ];

typedef struct SMS_DSPGMCData {

 uint8_t*       m_pDst;
 const uint8_t* m_pSrc;
 int            m_Stride;
 int            m_H;
 int            m_OX;
 int            m_OY;
 int            m_DxX;
 int            m_DxY;
 int            m_DyX;
 int            m_DyY;
 int            m_Shift;
 int            m_R;
 int            m_Width;
 int            m_Height;
 int            m_DeltaX;
 int            m_DeltaY;
 int            m_Rounder;

} SMS_DSPGMCData;

typedef struct SMS_DSPContext {

 uint8_t        m_Permutation       [ 64 ];
 SMS_QPelMCFunc m_PutQPelPixTab     [ 3 ][ 16 ];
 SMS_QPelMCFunc m_PutNoRndQPelPixTab[ 3 ][ 16 ];
 SMS_QPelMCFunc m_AvgQPelPixTab     [ 3 ][ 16 ];
 SMS_OpPixFunc  m_PutPixTab         [ 3 ][  4 ];
 SMS_OpPixFunc  m_PutNoRndPixTab    [ 3 ][  4 ];
 SMS_OpPixFunc  m_AvgPixTab         [ 3 ][  4 ];
 SMS_OpPixFunc  m_AvgNoRndPixTab    [ 3 ][  4 ];

 void ( *IDCT_Put       ) ( uint8_t*, int, SMS_DCTELEM*                 );
 void ( *IDCT_Add       ) ( uint8_t*, int, SMS_DCTELEM*                 );
 void ( *IDCT_ClrBlocks ) ( SMS_DCTELEM*                                );
 void ( *GMC            ) ( const SMS_DSPGMCData*                       );
 void ( *GMC1           ) ( uint8_t*, uint8_t*, int, int, int, int, int );

} SMS_DSPContext;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void SMS_DSP_Init        ( void            );
void SMS_DSPContext_Init ( SMS_DSPContext* );

# ifdef __cplusplus
};
# endif  /* __cplusplus */
#endif  /* __SMS_DSP_H */
