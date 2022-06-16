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
#include "SMS_GS.h"
#include "SMS_GUI.h"
#include "SMS_IOP.h"
#include "SMS_Container.h"
#include "SMS_Config.h"
#include "SMS_VIF.h"
#include "SMS_Locale.h"
#include "SMS_Player.h"
#include "SMS_MPEG.h"

#include <kernel.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

extern SMS_Player s_Player;

typedef struct _Info {

 DECLARE_GUI_OBJECT()

} _Info;

static void _Info_Render ( GUIObject* apObj, int aCtx ) {

 GSContext_CallList ( aCtx, apObj -> m_pGSPacket );

}  /* end _Info_Render */

void SMS_GUInfoDelete ( GUIObject** appInfo ) {

 if ( appInfo[ 0 ] ) {
  appInfo[ 0 ] -> Cleanup ( appInfo[ 0 ] );
  free ( appInfo[ 0 ] );
  appInfo[ 0 ] = NULL;
 }  /* end if */

}  /* end SMS_GUInfoDelete */

GUIObject* SMS_GUInfo ( SMS_Container* apCont ) {

 _Info*            retVal  = ( _Info* )calloc (  1, sizeof ( _Info )  );
 int               lWidth  = ( int )( g_GSCtx.m_Width  * 0.9F );
 int               lHeight = 32 * 4 + 8;
 int               lX      = ( g_GSCtx.m_Width  - lWidth  ) >> 1;
 int               lY      = ( g_GSCtx.m_Height - lHeight ) - ( lHeight >> 3 );
 int               lDWC    = GS_RRT_PACKET_SIZE() << 1;
 int               lLen, lGB, lMB, lKB;
 unsigned long*    lpPkt, *lpDMA;
 char              lFileSize[ 64 ];
 char              lVideo   [ 64 ];
 char              lAudio   [ 64 ];
 char              lMemory  [ 64 ];
 SMS_CodecContext* lpCodec;

 strcpy ( lFileSize, STR_FILE_SIZE.m_pStr );
 strcpy ( lVideo,    STR_VIDEO.m_pStr     );
 strcat ( lVideo,    g_ColonSStr          );
 strcpy ( lAudio,    STR_AUDIO.m_pStr     );
 strcat ( lAudio,    g_ColonSStr          );

 __asm__ __volatile__(
  ".set noat\n\t"
  "lui  $at, 0x4000\n\t"
  "ori  $at, $at, 0x0000\n\t"
  "divu $zero, %3, $at\n\t"
  "lui  $at, 0x0010\n\t"
  "ori  $at, $at, 0x0000\n\t"
  "mflo %0\n\t"
  "mfhi %1\n\t"
  "divu $zero, %1, $at\n\t"
  "ori  $at, $zero, 0x0400\n\t"
  "mflo %1\n\t"
  "mfhi %2\n\t"
  "divu $zero, %2, $at\n\t"
  "mflo %2\n\t"
  "beq  %0, $zero, 1f\n\t"
  "ori  $at, $zero, 0x000A\n\t"
  "divu $zero, %1, $at\n\t"
  "mflo %1\n\t"
  "1:\n\t"
  ".set at\n\t"
  : "=r"( lGB ), "=r"( lMB ), "=r"( lKB ) : "r"( apCont -> m_pFileCtx -> m_Size )
 );

 if ( lGB ) {
  sprintf ( &lFileSize[ STR_FILE_SIZE.m_Len ], STR_FMT_0.m_pStr, lGB, lMB );
  strcat ( lFileSize, STR_GB.m_pStr );
 } else {
  sprintf ( &lFileSize[ STR_FILE_SIZE.m_Len ], STR_FMT_1.m_pStr, lMB, lKB );
  strcat ( lFileSize, STR_MB.m_pStr );
 }  /* end else */

 lLen = strlen ( lFileSize );
 lGB  = strlen ( lVideo    );
 lMB  = strlen ( lAudio    );
 lKB  = s_Player.m_AudioIdx > 0;

 lpCodec = apCont -> m_pStm[ s_Player.m_VideoIdx ] -> m_pCodec;
 sprintf ( &lVideo[ lGB ], g_pFmt2, ( char* )&lpCodec -> m_Tag, lpCodec -> m_Width, lpCodec -> m_Height );

 if ( g_MPEGCtx.m_QuarterSample       ) strcat ( lVideo, g_pQPel );
 if ( g_MPEGCtx.m_VolSpriteUsage == 2 ) strcat ( lVideo, g_pGMC  );

 if ( lKB ) {
  char* lpPtr;
  lpCodec = apCont -> m_pStm[ s_Player.m_AudioIdx ] -> m_pCodec;
  sprintf (  &lAudio[ lMB ], g_pFmt1, ( char* )&lpCodec -> m_Tag );
  lpPtr = &lAudio[ strlen ( lAudio ) ];
  if ( lpPtr[ -1 ] == ' ' ) --lpPtr;
  sprintf ( lpPtr, g_pFmt0, lpCodec -> m_BitRate / 1000, STR_KbS.m_pStr, lpCodec -> m_SampleRate, STR_Hz.m_pStr, lpCodec -> m_Channels, STR_Ch.m_pStr );
 }  /* end if */

 sprintf ( lMemory, STR_AVAILABLE_MEMORY.m_pStr, (  32 * 1024 * 1024 - ( int )ps2_sbrk ( 0 )  ) / ( 1024.0F * 1024.0F ), ( float )SMS_IOPQueryTotalFreeMemSize () / ( 1024.0F * 1024.0F )  );

 lGB   = strlen ( lVideo  );
 lMB   = strlen ( lAudio  );
 lKB   = strlen ( lMemory );
 lDWC += GS_TXT_PACKET_SIZE( lLen );
 lDWC += GS_TXT_PACKET_SIZE( lGB  );
 lDWC += GS_TXT_PACKET_SIZE( lMB  );
 lDWC += GS_TXT_PACKET_SIZE( lKB  );

 retVal -> Render  = _Info_Render;
 retVal -> Cleanup = GUIObject_Cleanup;

 lpPkt = GSContext_NewList ( lDWC );
 lpDMA = lpPkt + (  GS_RRT_PACKET_SIZE() << 1  );
  GS_RenderRoundRect (
   ( GSRoundRectPacket* )(  lpPkt +  GS_RRT_PACKET_SIZE() - 2  ),
   lX, lY, lWidth, lHeight, -12, g_Palette[ g_Config.m_BrowserIBCIdx - 1 ]
  );
  GS_RenderRoundRect (
   ( GSRoundRectPacket* )( lpPkt - 2 ), lX, lY,
   lWidth, lHeight, 12, 0x60301010UL
  );
  g_GSCtx.m_TextColor = 0;
  GSFont_RenderEx ( lFileSize, lLen, lX + 16, lY +  8, lpDMA, -2, 0 );
  lpDMA += GS_TXT_PACKET_SIZE( lLen );
  GSFont_RenderEx ( lVideo,    lGB,  lX + 16, lY + 36, lpDMA, -2, 0 );
  lpDMA += GS_TXT_PACKET_SIZE( lGB );
  GSFont_RenderEx ( lAudio,    lMB,  lX + 16, lY + 64, lpDMA, -2, 0 );
  lpDMA += GS_TXT_PACKET_SIZE( lMB );
  GSFont_RenderEx ( lMemory,   lKB,  lX + 16, lY + 92, lpDMA, -2, 0 );
 lpPkt[ -1 ] = VIF_DIRECT( lDWC >> 1 );

 retVal -> m_pGSPacket = lpPkt;

 SyncDCache ( lpPkt - 2, lpPkt + lDWC );

 return ( GUIObject* )retVal;

}  /* end SMS_GUInfo */
