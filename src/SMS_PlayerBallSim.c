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
#include "SMS_GS.h"
#include "SMS_DMA.h"
#include "SMS_IPU.h"
#include "SMS_Sounds.h"

#include <kernel.h>
#include <stdlib.h>
#include <malloc.h>

#define Z_MAX  100
#define SPEED    2

extern unsigned char g_IconBall[ 316 ] __attribute__(   (  aligned( 16 ), section( ".data" )  )   );

typedef struct _ball_pos {

 int m_X;
 int m_Y;
 int m_Z;

} _ball_pos;

# define s_BallPos (  ( _ball_pos* )g_Balls  )

static void _create_ball ( int anIdx ) {

 s_BallPos[ anIdx ].m_X = SMS_rand () % ( int )g_GSCtx.m_Width;
 s_BallPos[ anIdx ].m_Y = SMS_rand () % ( int )( g_GSCtx.m_Height / 6 );
 s_BallPos[ anIdx ].m_Z = Z_MAX;

}  /* end CreateBall */

uint64_t* SMS_PlayerBallSim_Init ( uint32_t* apQWC ) {

 int          i;
 uint64_t*    lpUDMA;
 IPULoadImage lLoadImage;

 g_GSCtx.m_VRAMTexPtr = g_GSCtx.m_VRAMPtr;

 lpUDMA = _U( g_BallPkt );

 IPU_InitLoadImage ( &lLoadImage, 32, 32 );
 IPU_LoadImage (  &lLoadImage, g_IconBall, sizeof ( g_IconBall ), 0, 0, 0, 1, 16  );
 lLoadImage.Destroy ( &lLoadImage );

 lpUDMA[ 0 ] = GIF_TAG( 3, 0, 0, 0, 0, 1 );
 lpUDMA[ 1 ] = GIFTAG_REGS_AD;
 lpUDMA[ 2 ] = GS_SET_TEX0( g_GSCtx.m_VRAMPtr, 1, GSPixelFormat_PSMCT32, 6, 6, 1, 1, 0, 0, 0, 0, 0 );
 lpUDMA[ 3 ] = GS_TEX0_1;
 lpUDMA[ 4 ] = GS_SET_ALPHA( GS_ALPHA_A_CS, GS_ALPHA_B_CD, GS_ALPHA_C_AS, GS_ALPHA_D_CD, 0x00 );
 lpUDMA[ 5 ] = GS_ALPHA_1;
 lpUDMA[ 6 ] = GS_SET_PRIM( GS_PRIM_PRIM_SPRITE, 0, 1, 0, 1, 1, 1, 0, 0 );
 lpUDMA[ 7 ] = GS_PRIM;
 lpUDMA[ 8 ] = GIF_TAG( 512, 1, 0, 0, 1, 4 );
 lpUDMA[ 9 ] = GS_UV | ( GS_XYZ2 << 4 ) | ( GS_UV << 8 ) | ( GS_XYZ2 << 12 );

 for ( i = 0; i < 512; ++i ) {

  lpUDMA[ 10 + i * 4 ] = GS_SET_UV(   8,   8 );
  lpUDMA[ 12 + i * 4 ] = GS_SET_UV( 504, 504 );

  _create_ball ( i );

 }  /* end for */

 *apQWC = (  ( 10 + 512 * 4 ) * sizeof ( uint64_t )  ) >> 4;

 return g_BallPkt;

}  /* end SMS_PlayerBallSim_Init */

void SMS_PlayerBallSim_Update ( uint64_t* apDMA ) {

 int  i, lW = ( int )( g_GSCtx.m_Width  >> 1   ),
         lH = ( int )( g_GSCtx.m_Height / 2.3F );

 for ( i = 0; i < 512; ++i ) {

  int  lX, lY;
  u64* lpDMA = ( u64* )_U(  apDMA + 10 + ( i << 2 )  );

  s_BallPos[ i ].m_Z -= SPEED;

  if ( s_BallPos[ i ].m_Z < 2 ) _create_ball ( i );

  lX = lW + ( s_BallPos[ i ].m_X << 6 ) / s_BallPos[ i ].m_Z;
  lY = lH + ( s_BallPos[ i ].m_Y << 6 ) / s_BallPos[ i ].m_Z;

  if ( lX < 0 || lY < 0 || lX >= ( int )g_GSCtx.m_Width || lY >= ( int )g_GSCtx.m_Height - g_GSCtx.m_Height / 3 ) {

   _create_ball ( i );

   lX = lW + s_BallPos[ i ].m_X;
   lY = lH + s_BallPos[ i ].m_Y;

  }  /* end if */

  lpDMA[ 1 ] = GS_XYZ( lX,      lY,      0 );
  lpDMA[ 3 ] = GS_XYZ( lX + 32, lY + 32, 0 );

 }  /* end for */

}  /* end SMS_PlayerBallSim_Update */
