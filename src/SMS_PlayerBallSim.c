/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) Nick Van Veen (a.k.a Sjeep) 2001
# Adopted for SMS in 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_PlayerBallSim.h"
#include "GUI_Data.h"
#include "GS.h"
#include "DMA.h"

#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>

#define NBALLS 512
#define Z_MAX  100
#define SPEED    2

static GSVertex s_BallPos[ NBALLS ];

static int _ball_rand ( void ) {

 static int s_lSeed = 1;

 return s_lSeed = ( 161140751 * s_lSeed + 13 ) % 219441163;

}  /* end _ball_rand */

static void _create_ball ( int anIdx ) {

 s_BallPos[ anIdx ].m_X = _ball_rand () % ( int )g_GSCtx.m_Width;
 s_BallPos[ anIdx ].m_Y = _ball_rand () % ( int )( g_GSCtx.m_Height / 6 );
 s_BallPos[ anIdx ].m_Z = Z_MAX;

}  /* end CreateBall */

uint64_t* SMS_PlayerBallSim_Init ( uint32_t* apQWC ) {

 int       i;
 uint64_t* lpDMA;
 uint64_t* lpUDMA;

 lpDMA  = ( uint64_t* )malloc (  ( 12 + NBALLS * 4 ) * sizeof ( uint64_t )  );
 lpUDMA = _U( lpDMA );

 FlushCache ( 0 );

 lpUDMA[ 0 ] = DMA_TAG( 6, 1, DMA_CNT, 0, 0, 0 );
 lpUDMA[ 1 ] = 0;
  lpUDMA[ 2 ] = GIF_TAG( 4, 1, 0, 0, 0, 1 );
  lpUDMA[ 3 ] = GIF_AD;
   lpUDMA[  4 ] = GS_SETREG_BITBLTBUF( 0, 0, 0, g_GSCtx.m_IconPtr, 1, GSPSM_32 );
   lpUDMA[  5 ] = GS_BITBLTBUF;
   lpUDMA[  6 ] = GS_SETREG_TRXPOS( 0, 0, 0, 0, 0 );
   lpUDMA[  7 ] = GS_TRXPOS;
   lpUDMA[  8 ] = GS_SETREG_TRXREG( 32, 32 );
   lpUDMA[  9 ] = GS_TRXREG;
   lpUDMA[ 10 ] = GS_SETREG_TRXDIR( 0 );
   lpUDMA[ 11 ] = GS_TRXDIR;
   lpUDMA[ 12 ] = GIF_TAG( 256, 1, 0, 0, 2, 1 );
   lpUDMA[ 13 ] = 0;
  lpUDMA[ 14 ] = DMA_TAG(  256, 1, DMA_REFE, 0, ( u32 )g_ImgBlueLED, 0  );
  lpUDMA[ 15 ] = 0;

 __asm__ __volatile__( "sync\n\t" );

 DMA_SendChainToGIF( lpDMA );
 DMA_Wait ( DMA_CHANNEL_GIF );

 lpUDMA[ 0 ] = GIF_TAG( 3, 0, 0, 0, 0, 1 );
 lpUDMA[ 1 ] = GIF_AD;
 lpUDMA[ 2 ] = GS_SETREG_TEX0( g_GSCtx.m_IconPtr, 1, GSPSM_32, 6, 6, 1, 1, 0, 0, 0, 0, 0 );
 lpUDMA[ 3 ] = GS_TEX0_1;
 lpUDMA[ 4 ] = ALPHA_BLEND_NORMAL;
 lpUDMA[ 5 ] = GS_ALPHA_1;
 lpUDMA[ 6 ] = GS_SETREG_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
 lpUDMA[ 7 ] = GS_PRIM;
 lpUDMA[ 8 ] = GIF_TAG( NBALLS, 1, 0, 0, 1, 4 );
 lpUDMA[ 9 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 for ( i = 0; i < NBALLS; ++i ) {

  lpUDMA[ 10 + i * 4 ] = GS_SETREG_UV(
   g_GSCtx.m_OffsetX << 4, g_GSCtx.m_OffsetX << 4
  );
  lpUDMA[ 12 + i * 4 ] = GS_SETREG_UV(
   ( 32 << 4 ) + ( g_GSCtx.m_OffsetX << 4 ), ( 32 << 4 ) + ( g_GSCtx.m_OffsetX << 4 )
  );

  _create_ball ( i );

 }  /* end for */

 *apQWC = (  ( 10 + NBALLS * 4 ) * sizeof ( uint64_t )  ) >> 4;

 return lpDMA;

}  /* end SMS_PlayerBallSim_Init */

void SMS_PlayerBallSim_Destroy ( uint64_t* apPacket ) {

 if ( apPacket ) free ( apPacket );

}  /* end SMS_PlayerBallSim_Destroy */

void SMS_PlayerBallSim_Update ( uint64_t* apDMA ) {

 int  i, lW = ( int )( g_GSCtx.m_Width  >> 1   ),
         lH = ( int )( g_GSCtx.m_Height / 2.3F );

 for ( i = 0; i < NBALLS; ++i ) {

  int  lX1, lX2;
  int  lY1, lY2;
  u64* lpDMA = ( u64* )_U(  apDMA + 10 + ( i << 2 )  );

  s_BallPos[ i ].m_Z -= SPEED;

  if ( s_BallPos[ i ].m_Z < 2 ) _create_ball ( i );

  lX1 = lW + ( s_BallPos[ i ].m_X << 6 ) / s_BallPos[ i ].m_Z;
  lY1 = lH + ( s_BallPos[ i ].m_Y << 6 ) / s_BallPos[ i ].m_Z;

  if ( lX1 < 0 || lY1 < 0 || lX1 >= ( int )g_GSCtx.m_Width || lY1 >= ( int )g_GSCtx.m_Height - g_GSCtx.m_Height / 3 ) {

   _create_ball ( i );

   lX1 = lW + s_BallPos[ i ].m_X;
   lY1 = lH + s_BallPos[ i ].m_Y;

  }  /* end if */

  lX2 = lX1 + 32;
  lY2 = lY1 + 32;

  lpDMA[ 1 ] = GS_SETREG_XYZ(
   ( lX1 << 4 ) + ( g_GSCtx.m_OffsetX << 4 ), ( lY1 << 3 ) + ( g_GSCtx.m_OffsetY << 4 ), 0
  );
  lpDMA[ 3 ] = GS_SETREG_XYZ(
   ( lX2 << 4 ) + ( g_GSCtx.m_OffsetX << 4 ), ( lY2 << 3 ) + ( g_GSCtx.m_OffsetY << 4 ), 0
  );

 }  /* end for */

 __asm__ __volatile__( "sync\n\t" );

}  /* end SMS_PlayerBallSim_Update */
