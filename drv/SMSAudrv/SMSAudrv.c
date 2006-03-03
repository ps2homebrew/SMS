/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by gawd (Gil Megidish) (original idea and code)
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
#
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
*/
#include <intrman.h>
#include <loadcore.h>
#include <libsd.h>
#include <sifrpc.h>
#include <sysclib.h>
#include <thbase.h>
#include <thsemap.h>

#include "SMSAudrv_US.h"

#define SMS_AUDIO_RPC_ID  0x41534D53
#define SMS_VOLUME_RPC_ID 0x56534D53

#define MIN( a, b ) (  ( a ) <= ( b )  ) ? ( a ) : ( b )

#define SPU_IRQ           9
#define SPU_DMA_CHN0_IRQ 36
#define SPU_DMA_CHN1_IRQ 40

#define SD_INIT_COLD 0
#define SD_INIT_HOT  1
#define SD_CORE_0    0
#define SD_CORE_1    1

static int     s_SemaPlay;
static int     s_SemaQueue;
static u8      s_SPUBuf [  4096 ] __attribute__(   (  aligned( 64 )  )   );
static u8      s_RingBuf[ 18432 ];
static int     s_ReadPos;
static int     s_WritePos;
static int     s_BufSize;
static UPSFunc s_Upsampler;
static int     s_Advance;
static int     s_Core;

static void SDTransCallback ( void* apArg ) {

 iSignalSema ( s_SemaPlay );

}  /* end SDTransCallback */

static void _Mute0 ( int afMute ) {

 int lVol = afMute ? 0 : 0x3FFF;

 sceSdSetParam ( SD_CORE_0 | SD_PARAM_MVOLL, lVol );
 sceSdSetParam ( SD_CORE_0 | SD_PARAM_MVOLR, lVol );

}  /* end _Mute0 */

static void _SetVolume ( int aCore, int aVol ) {

 sceSdSetParam ( aCore | SD_PARAM_BVOLL, aVol );
 sceSdSetParam ( aCore | SD_PARAM_BVOLR, aVol );

}  /* end _SetVolume */

static int _InAvail ( void ) {

 return s_WritePos <= s_ReadPos ? s_ReadPos - s_WritePos
                                : s_BufSize - ( s_WritePos - s_ReadPos );
}  /* end _InAvail */

static void _WaitAvail ( int aSize ) {

 while ( 1 ) {

  if (  _InAvail () >= aSize ) return;
		
  WaitSema ( s_SemaQueue );

 }  /* end while */

}  /* end _WaitAvail */

static void _PlaybackThread ( void* apParam ) {

 while ( 1 ) {

  int lState;
  int lBlock;
  u8* lpBuf;

  WaitSema (  *( int* )apParam  );

  CpuSuspendIntr ( &lState );

   lBlock = 1 - (  sceSdBlockTransStatus ( s_Core, 0 ) >> 24  );
   lpBuf  = s_SPUBuf + ( lBlock << 11 );

   s_Upsampler ( &s_RingBuf[ s_ReadPos ], lpBuf );
   s_ReadPos += s_Advance;

   if ( s_ReadPos >= s_BufSize ) s_ReadPos = 0;

  CpuResumeIntr  ( lState );

  if (  _InAvail () > ( s_BufSize >> 3 )  ) SignalSema ( s_SemaQueue );

 }  /* end while */

}  /* end _PlaybackThread */

typedef void* ( *RPCHandler ) ( void*, int );

static void _Silence ( void ) {

 memset (  s_SPUBuf,  0, sizeof ( s_SPUBuf  )  );
 memset (  s_RingBuf, 0, sizeof ( s_RingBuf )  );

}  /* end _Silence */

static void* _StartAudio ( void* apData, int aSize ) {

 int lFreq   = (  ( int* )apData  )[ 0 ];
 int lBS     = (  ( int* )apData  )[ 1 ];
 int lnChan  = (  ( int* )apData  )[ 2 ];
 int lVolume = (  ( int* )apData  )[ 3 ];

 _Mute0     ( 1 );
 _Silence   ();
 _SetVolume ( 0, 0 );

 s_Upsampler = SMSAudrv_GetUPS ( lFreq, lBS, lnChan, &s_Advance );
 s_BufSize   = s_Advance * (  sizeof ( s_RingBuf ) / s_Advance  );
 s_WritePos  = 0;
 s_ReadPos   = s_Advance;

 if ( lnChan == 5 ) {

  s_Core  = SD_CORE_0;
  lBS     = SPU_DMA_CHN0_IRQ;
  lVolume = 0;

 } else {

  s_Core = SD_CORE_1;
  lBS    = SPU_DMA_CHN1_IRQ;

 }  /* end else */

 _SetVolume ( 1, lVolume );

 EnableIntr ( lBS );

 sceSdSetTransCallback ( s_Core, SDTransCallback );
 sceSdBlockTrans (  s_Core, SD_TRANS_LOOP, s_SPUBuf, sizeof ( s_SPUBuf )  );

 *( int* )apData = s_BufSize;

 return apData;

}  /* end _StartAudio */

static void* _StopAudio ( void* apData, int aSize ) {

 int lRes;

 _SetVolume ( 1, 0 );

 sceSdSetTransCallback ( s_Core, NULL );

 DisableIntr ( SPU_DMA_CHN1_IRQ, &lRes );

 sceSdSetCoreAttr ( SD_CORE_SPDIF_MODE, 0 );

 return NULL;

}  /* end _StopAudio */

static void* _PlayPCM ( void* apData, int aSize ) {

 void* lpData = (  ( u8* )apData  ) + 4;

 aSize = *( int* )apData;

 _WaitAvail ( aSize );

 aSize = MIN(  aSize, _InAvail ()  );

 while ( aSize > 0 ) {

  int lCopy = aSize;

  if ( s_WritePos >= s_ReadPos ) lCopy = MIN( s_BufSize - s_WritePos, aSize );

  memcpy ( s_RingBuf + s_WritePos, lpData, lCopy );

  lpData   += lCopy;
  aSize    -= lCopy;

  s_WritePos += lCopy;

  if ( s_WritePos >= s_BufSize ) s_WritePos = 0;

 }  /* end while */

 return NULL;

}  /* end _PlayPCM */

static void* _LoadData ( void* apData, int aSize ) {

 void* lpData = ( void* )(  ( unsigned int* )apData  )[ 0 ];
 int   lSize  = (  ( unsigned int* )apData  )[ 1 ];

 EnableIntr ( SPU_DMA_CHN0_IRQ );
  sceSdVoiceTrans (  0, SD_TRANS_WRITE | SD_TRANS_MODE_DMA, lpData, ( u8* )0x5010, lSize  );
  sceSdVoiceTransStatus ( 0, 1 );
 DisableIntr ( SPU_DMA_CHN0_IRQ, &lSize );

 return NULL;

}  /* end _LoadData */

static void* _PlaySound ( void* apData, int aSize ) {

 unsigned int lSound   = (  ( unsigned int* )apData  )[ 0 ];
 unsigned int lVolume  = (  ( unsigned int* )apData  )[ 1 ];
 unsigned int lVVolume = lVolume >> 1;

 for ( aSize = 0; aSize < 2; ++aSize ) {

  sceSdSetParam (  SD_VOICE( aSize, 0 ) | SD_VPARAM_VOLL, lVVolume  );
  sceSdSetParam (  SD_VOICE( aSize, 0 ) | SD_VPARAM_VOLR, lVVolume  );

  _SetVolume ( aSize, lVolume );

 }  /* end for */

 _Mute0 ( 0 );

 sceSdSetAddr (  SD_VOICE( 0, 0 ) | SD_VADDR_SSA, 0x5010 + lSound  );
 sceSdSetSwitch ( SD_CORE_0 | SD_SWITCH_KEYDOWN, 1 );

 return NULL;

}  /* end _PlaySound */

static RPCHandler s_RPC[] = {
 _StartAudio, _StopAudio, _PlayPCM, _LoadData, _PlaySound
};

static void* _RPCServer_SMSA ( int aCmd, void* apData, int aSize ) {

 return s_RPC[ aCmd ] ( apData, aSize );

}  /* end _RPCServer_SMSA */

static void* _RPCServer_SMSV ( int aCmd, void* apData, int aSize ) {

 switch ( aCmd ) {

  case 0: _SetVolume (  1, *( int* )apData  ); break;
  case 1: _Silence   ();                       break;

 }  /* end switch */

 return NULL;

}  /* end _RPCServer_SMSV */

static void _SMS_AudrvA ( void* apParam ) {

 static u8 s_RPCBuffer[ 8192 ] __attribute(   (  aligned( 64 )  )   );

 SifRpcDataQueue_t  lRPCDataQueue;
 SifRpcServerData_t lRPCServData;

 sceSifSetRpcQueue (  &lRPCDataQueue, GetThreadId ()  );
 sceSifRegisterRpc (
  &lRPCServData, SMS_AUDIO_RPC_ID, _RPCServer_SMSA, s_RPCBuffer,
  NULL, NULL, &lRPCDataQueue
 );
 sceSifRpcLoop ( &lRPCDataQueue );

}  /* end _SMS_AudrvA */

static void _SMS_AudrvV ( void* apParam ) {

 static u8 s_RPCBuffer[ 16 ] __attribute(   (  aligned( 64 )  )   );

 SifRpcDataQueue_t  lRPCDataQueue;
 SifRpcServerData_t lRPCServData;

 sceSifSetRpcQueue (  &lRPCDataQueue, GetThreadId ()  );
 sceSifRegisterRpc (
  &lRPCServData, SMS_VOLUME_RPC_ID, _RPCServer_SMSV, s_RPCBuffer,
  NULL, NULL, &lRPCDataQueue
 );
 sceSifRpcLoop ( &lRPCDataQueue );

}  /* end _SMS_AudrvV */

int _start ( int argc, char** argv ) {

 iop_thread_t lThread;
 iop_sema_t   lSema;

 lSema.attr    = 0;
 lSema.option  = 0;
 lSema.initial = 0;
 lSema.max     = 1;
 s_SemaPlay  = CreateSema ( &lSema );
 s_SemaQueue = CreateSema ( &lSema );

 sceSdInit ( SD_INIT_COLD );

 sceSdSetParam ( SD_CORE_1 | SD_PARAM_MVOLL, 0x3FFF );
 sceSdSetParam ( SD_CORE_1 | SD_PARAM_MVOLR, 0x3FFF );
 sceSdSetParam (  SD_VOICE( 0, 0 ) | SD_VPARAM_PITCH, 0x1000  );

 _Mute0 ( 1 );

 lThread.attr      = TH_C;
 lThread.thread    = _PlaybackThread;
 lThread.stacksize = 0x1000;
 lThread.priority  = 0x10;
 StartThread (  CreateThread ( &lThread ), &s_SemaPlay  );

 lThread.thread = _SMS_AudrvA;
 StartThread (  CreateThread ( &lThread ), NULL  );

 lThread.thread = _SMS_AudrvV;
 StartThread (  CreateThread ( &lThread ), NULL  );

 return MODULE_RESIDENT_END;

}  /* end _start */
