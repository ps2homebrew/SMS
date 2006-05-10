/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS_Spectrum.h"
#include "SMS_Sounds.h"
#include "SMS_Config.h"
#include "SMS_GS.h"
#include "SMS_VIF.h"
#include "SMS_Player.h"
#include "SMS_DSP.h"

#include <kernel.h>

extern SMS_Player s_Player;

void SMS_SpectrumInit ( void ) {

 int            lX       = (  ( g_GSCtx.m_Width - 480 ) >> 1  ) - 30;
 int            lYT      = g_GSCtx.m_Height - 196;
 int            lYB      = g_GSCtx.m_Height - 192;
 unsigned long* lpDMA    = UNCACHED_SEG( g_SPCPkt       );
 unsigned long* lpDMAEnd = UNCACHED_SEG( g_SPCPkt + 148 );

 *lpDMA++ = 0L;
 *lpDMA++ = VIF_DIRECT( 73 );
 *lpDMA++ = GIF_TAG( 16, 1, 0, 0, GIFTAG_FLG_REGLIST, 9 );
 *lpDMA++ = GS_PRIM | ( GS_RGBAQ <<  4 ) | ( GS_XYZ2 <<  8 ) | ( GS_RGBAQ << 12 ) | ( GS_XYZ2 << 16 ) | ( GS_RGBAQ << 20 ) | ( GS_XYZ2 << 24 ) | ( GS_RGBAQ << 28 ) | ( GS_XYZ2 << 32 );

 while ( lpDMA < lpDMAEnd ) {

  int lXR;

  lX += 30;
  lXR = lX + 20;

  lpDMA[ 0 ] = GS_SET_PRIM(
   GS_PRIM_PRIM_TRISTRIP, GS_PRIM_IIP_GOURAUD, GS_PRIM_TME_OFF,
   GS_PRIM_FGE_OFF, GS_PRIM_ABE_ON, GS_PRIM_AA1_OFF, GS_PRIM_FST_STQ,
   GS_PRIM_CTXT_1, GS_PRIM_FIX_UNFIXED
  );
  lpDMA[ 1 ] = g_Palette[ g_Config.m_BrowserIBCIdx - 1 ];
  lpDMA[ 2 ] = GS_XYZ( lX, lYT, 0 );
  lpDMA[ 3 ] = GS_SET_RGBAQ( 0x00, 0x00, 0xFF, 0x80, 0x00 );
  lpDMA[ 4 ] = GS_XYZ( lX, lYB, 0 );
  lpDMA[ 5 ] = g_Palette[ g_Config.m_BrowserIBCIdx - 1 ];
  lpDMA[ 6 ] = GS_XYZ( lXR, lYT, 0 );
  lpDMA[ 7 ] = GS_SET_RGBAQ( 0x00, 0x00, 0xFF, 0x80, 0x00 );
  lpDMA[ 8 ] = GS_XYZ( lXR, lYB, 0 );

  lpDMA += 9;

 }  /* end while */

 s_Player.m_OSDQWC    [ 7 ] = 74;
 s_Player.m_OSDPackets[ 7 ] = g_SPCPkt;

}  /* end SMS_SpectrumInit */

void SMS_SpectrumUpdate ( short* apSamples ) {

 static int s_lIdx[ 16 ] = {
  0, 1, 2, 3, 4, 5, 6, 7, 15, 14, 13, 12, 11, 10, 9, 8
 };

 int            i;
 int*           lpH      = ( int* )0x70003800;
 int            lYB      = g_GSCtx.m_Height - 192;
 int            lX       = (  ( g_GSCtx.m_Width - 480 ) >> 1  ) - 30;
 unsigned long* lpDMA    = UNCACHED_SEG( g_SPCPkt + 6 );
 unsigned long* lpDMAEnd = UNCACHED_SEG( g_SPCPkt + 148 );

 if ( !apSamples ) {

  for ( i = 0; i < 16; ++i ) lpH[ i ] = 2;

 } else {

  DSP_FFTInit (  UNCACHED_SEG( apSamples ), g_FFTWsp  );
  DSP_FFTRun ( g_FFTWsp );
  DSP_FFTGet ( g_FFTWsp );

 }  /* end else */

 i = 0;

 while ( lpDMA < lpDMAEnd ) {

  unsigned lYT = lYB - lpH[  s_lIdx[ i++ ]  ];

  lX += 30;

  lpDMA[ 0 ] = GS_XYZ ( lX,      lYT, 0 );
  lpDMA[ 4 ] = GS_XYZ ( lX + 20, lYT, 0 );

  lpDMA += 9;

 }  /* end while */

}  /* end SMS_SpectrumUpdate */
