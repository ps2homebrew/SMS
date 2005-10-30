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

#define SD_INIT_COLD  0
#define SD_MAX_VOLUME 0x3FFF
#define SD_CORE_0     0
#define SD_CORE_1     1
#define SD_P_AVOLL    (  ( 0x0D << 8 ) + ( 0x01 << 7 )  )
#define SD_P_AVOLR    (  ( 0x0E << 8 ) + ( 0x01 << 7 )  )
#define SD_P_BVOLL    (  ( 0x0F << 8 ) + ( 0x01 << 7 )  )
#define SD_P_BVOLR    (  ( 0x10 << 8 ) + ( 0x01 << 7 )  )
#define SD_P_MVOLL    (  ( 0x09 << 8 ) + ( 0x01 << 7 )  )
#define SD_P_MVOLR    (  ( 0x0A << 8 ) + ( 0x01 << 7 )  )
#define SD_BLOCK_LOOP 0x10

static int     s_SemaPlay;
static int     s_SemaQueue;
static u8      s_SPUBuf [  4096 ] __attribute__(   (  aligned( 64 )  )   );
static u8      s_RingBuf[ 18432 ];
static int     s_ReadPos;
static int     s_WritePos;
static int     s_BufSize;
static UPSFunc s_Upsampler;
static int     s_Advance;

static void SDTransCallback ( void* apArg ) {

 iSignalSema ( s_SemaPlay );

}  /* end SDTransCallback */

static void _SetVolume ( int aCore, int aVol ) {

 sceSdSetParam ( aCore | SD_P_BVOLL, aVol );
 sceSdSetParam ( aCore | SD_P_BVOLR, aVol );

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

   lBlock = 1 - (  sceSdBlockTransStatus ( SD_CORE_1, 0 ) >> 24  );
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

 sceSdInit ( SD_INIT_COLD );

 _Silence   ();

 _SetVolume ( 0, 0       );
 _SetVolume ( 1, lVolume );

 sceSdSetParam ( SD_CORE_1 | SD_P_AVOLL, SD_MAX_VOLUME );
 sceSdSetParam ( SD_CORE_1 | SD_P_AVOLR, SD_MAX_VOLUME );

 sceSdSetParam ( SD_CORE_0 | SD_P_MVOLL, 0 );
 sceSdSetParam ( SD_CORE_0 | SD_P_MVOLR, 0 );

 sceSdSetParam ( SD_CORE_1 | SD_P_MVOLL, SD_MAX_VOLUME );
 sceSdSetParam ( SD_CORE_1 | SD_P_MVOLR, SD_MAX_VOLUME );

 s_Upsampler = SMSAudrv_GetUPS ( lFreq, lBS, lnChan, &s_Advance );
 s_BufSize   = s_Advance * (  sizeof ( s_RingBuf ) / s_Advance  );
 s_WritePos  = 0;
 s_ReadPos   = s_Advance;

 EnableIntr ( SPU_DMA_CHN0_IRQ );
 EnableIntr ( SPU_DMA_CHN1_IRQ );
 EnableIntr ( SPU_IRQ          );

 sceSdSetTransCallback ( SD_CORE_1, SDTransCallback );
 sceSdBlockTrans (  SD_CORE_1, SD_BLOCK_LOOP, s_SPUBuf, sizeof ( s_SPUBuf )  );

 *( int* )apData = s_BufSize;

 return apData;

}  /* end _StartAudio */

static void* _StopAudio ( void* apData, int aSize ) {

 int lRes;

 _SetVolume ( 1, 0 );

 sceSdSetTransCallback ( SD_CORE_1, NULL );

 DisableIntr ( SPU_DMA_CHN0_IRQ, &lRes );
 DisableIntr ( SPU_DMA_CHN1_IRQ, &lRes );
 DisableIntr ( SPU_IRQ,          &lRes );

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

static RPCHandler s_RPC[] = {
 _StartAudio, _StopAudio, _PlayPCM
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

 FlushDcache ();

 lSema.attr    = 0;
 lSema.option  = 0;
 lSema.initial = 0;
 lSema.max     = 1;
 s_SemaPlay  = CreateSema ( &lSema );
 s_SemaQueue = CreateSema ( &lSema );

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
