/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 2000, 2001, 2002 Fabrice Bellard.
# Copyright (c) 2002 - 2004 Michael Niedermayer <michaelni@gmx.at>
#               2005, 2006  - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#ifndef __SMS_DSP_H
# define __SMS_DSP_H

# ifndef __SMS_H
#  include "SMS.h"
# endif  /* __SMS_H */

typedef void ( *SMS_OpPixFunc  ) ( uint8_t*, const uint8_t*, int, int, int );
typedef void ( *SMS_QPelMCFunc ) ( uint8_t*, const uint8_t*, int, int, int );

typedef struct SMS_DSPGMCData {

 int m_Uo;
 int m_Vo;
 int m_UCo;
 int m_VCo;
 int m_Width;
 int m_Height;
 int m_Accuracy;
 int m_nWarpPts;
 int m_dU[ 2 ];
 int m_dV[ 2 ];

} SMS_DSPGMCData;

typedef struct SMS_DSPContext {

 SMS_QPelMCFunc m_PutQPelPixTab     [ 2 ][ 16 ];
 SMS_QPelMCFunc m_PutNoRndQPelPixTab[ 2 ][ 16 ];
 SMS_QPelMCFunc m_AvgQPelPixTab     [ 2 ][ 16 ];
 SMS_OpPixFunc  m_PutPixTab         [ 3 ][  4 ];
 SMS_OpPixFunc  m_PutNoRndPixTab    [ 3 ][  4 ];
 SMS_OpPixFunc  m_AvgPixTab         [ 3 ][  4 ];

} SMS_DSPContext;

extern const uint8_t  g_SMS_DSP_zigzag_direct            [ 64 ];
extern const uint8_t  g_SMS_DSP_alternate_horizontal_scan[ 64 ];
extern const uint8_t  g_SMS_DSP_alternate_vertical_scan  [ 64 ];
extern SMS_DSPGMCData g_GMCData;
# ifdef __cplusplus
extern "C" {
# endif  /* __cplusplus */

void  SMS_DSP_Init       ( void                             );
void  SMS_DSPContextInit ( SMS_DSPContext*                  );
void  IDCT               ( uint8_t*, int, SMS_DCTELEM*, int );
void  IDCT_ClrBlocks     ( void                             );
void* DSP_FFTInit        ( short*, void*                    );
void  DSP_FFTRun         ( void*                            );
void  DSP_FFTGet         ( void*                            );

void DSP_GetMB  ( void* );
void DSP_PackMB ( void* );

void DSP_GMC1_16 ( void*, void*, int, int, int, int, int, int );
void DSP_GMC1_8  ( void*, void*, int, int, int, int, int, int );

void DSP_GMCn_16 ( uint8_t*, const SMS_MacroBlock*, int, int, int, int );
void DSP_GMCn_8  ( uint8_t*, const SMS_MacroBlock*, int, int, int, int );

void DSP_PutPixels16     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels16X    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels16Y    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels16XY   ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8      ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8X     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8Y     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8XY    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8_16   ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8X_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8Y_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutPixels8XY_16 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_PutNoRndPixels16X    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels16Y    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels16XY   ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8X     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8Y     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8XY    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8X_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8Y_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndPixels8XY_16 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_AvgPixels16     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels16X    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels16Y    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels16XY   ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8      ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8X     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8Y     ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8XY    ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8_16   ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8X_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8Y_16  ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgPixels8XY_16 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_PutQPel16MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel16MC33 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_PutNoRndQPel16MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel16MC33 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_PutQPel816MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutQPel816MC33 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_PutNoRndQPel816MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_PutNoRndQPel816MC33 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_AvgQPel16MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel16MC33 ( uint8_t*, const uint8_t*, int, int, int );

void DSP_AvgQPel816MC10 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC20 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC30 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC01 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC11 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC21 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC31 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC02 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC12 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC22 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC32 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC03 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC13 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC23 ( uint8_t*, const uint8_t*, int, int, int );
void DSP_AvgQPel816MC33 ( uint8_t*, const uint8_t*, int, int, int );
# ifdef __cplusplus
};
# endif  /* __cplusplus */
#endif  /* __SMS_DSP_H */
