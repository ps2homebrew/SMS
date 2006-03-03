/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_SPU.h"
#include "SMS_SIF.h"
#include "SMS_Config.h"

#include <kernel.h>
#include <iopheap.h>

#define SMS_AUDIO_RPC_ID  0x41534D53
#define SMS_VOLUME_RPC_ID 0x56534D53

static SPUContext         s_SPUCtx;
static SifRpcClientData_t s_ClientDataA __attribute__(   (  aligned( 64 )  )   );
static SifRpcClientData_t s_ClientDataV __attribute__(   (  aligned( 64 )  )   );
static unsigned int       s_Buffer[ 4 ] __attribute__(   (  aligned( 64 )  )   );
static int                s_SemaPCM;
static int                s_SemaVol;

static int _Init ( void ) {

 ee_sema_t lSema;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_SemaPCM = CreateSema ( &lSema );
 s_SemaVol = CreateSema ( &lSema );

 return SIF_BindRPC ( &s_ClientDataA, SMS_AUDIO_RPC_ID  ) &&
        SIF_BindRPC ( &s_ClientDataV, SMS_VOLUME_RPC_ID );

}  /* end _Init */

static void _audio_callback ( void* apArg ) {

 iSignalSema (  *( int* )apArg  );

}  /* end _audio_callback */

static void SPU_SetVolume ( int aVol ) {

 s_Buffer[ 0 ] = aVol;

 SifCallRpc ( &s_ClientDataV, 0, SIF_RPC_M_NOWAIT, s_Buffer, 4, NULL, 0, _audio_callback, &s_SemaVol );
 WaitSema ( s_SemaVol );

}  /* end SPU_SetVolume */

static void SPU_Silence ( void ) {

 SifCallRpc ( &s_ClientDataV, 1, SIF_RPC_M_NOWAIT, NULL, 0, NULL, 0, _audio_callback, &s_SemaVol );
 WaitSema ( s_SemaVol );

}  /* end Silence */

static void SPU_PlayPCM ( void* apBuf ) {

 SifCallRpc (
  &s_ClientDataA, 2, SIF_RPC_M_NOWBDC | SIF_RPC_M_NOWAIT, apBuf, *( int* )apBuf + 4, NULL, 0, _audio_callback, &s_SemaPCM
 );
 WaitSema ( s_SemaPCM );

}  /* end SPU_PlayPCM */

static void SPU_Destroy ( void ) {

 SifCallRpc ( &s_ClientDataA, 1, SIF_RPC_M_NOWBDC | SIF_RPC_M_NOWAIT, NULL, 0, NULL, 0, _audio_callback, &s_SemaPCM );
 WaitSema ( s_SemaPCM );

}  /* end SPU_Destroy */

void SPU_LoadData ( void* apData, int aSize ) {

 void*            lpIOPData = SifAllocIopHeap ( aSize );
 SifDmaTransfer_t lXfrData;
 int              lID;

 lXfrData.src  = apData;
 lXfrData.dest = lpIOPData;
 lXfrData.size = aSize;
 lXfrData.attr = 0;

 lID = SifSetDma ( &lXfrData, 1 );

 s_Buffer[ 0 ] = ( unsigned int )lpIOPData;
 s_Buffer[ 1 ] = aSize;

 while (  SifDmaStat ( lID ) >= 0  );

 SifCallRpc (
  &s_ClientDataA, 3, SIF_RPC_M_NOWAIT, s_Buffer, 8, NULL, 0, _audio_callback, &s_SemaPCM
 );
 WaitSema ( s_SemaPCM );

 SifFreeIopHeap ( lpIOPData );

}  /* end SPU_LoadData */

int SPU_Index2Volume ( int anIdx ) {

 static unsigned s_lScale[ 25 ] = {
      0,   300,   800,  1120,  1600,
   2140,  2660,  3060,  3460,  4380,
   5100,  6020,  7040,  8160,  8980,
  10500, 11940, 13980, 16000, 18160,
  20900, 24060, 27120, 29880, 32767
 };

 return s_lScale[ SMS_clip ( anIdx, 0, 24 ) ];

}  /* end SPU_Index2Volume */

void SPU_PlaySound ( SMSound* apSound, int aVol ) {

 if ( g_Config.m_BrowserFlags & SMS_BF_SDFX ) {

  s_Buffer[ 0 ] = apSound -> m_Sound;
  s_Buffer[ 1 ] = SPU_Index2Volume ( aVol );

  SifCallRpc (
   &s_ClientDataA, 4, SIF_RPC_M_NOWAIT, s_Buffer, 8, NULL, 0, _audio_callback, &s_SemaPCM
  );
  WaitSema ( s_SemaPCM );

 }  /* end if */

}  /* end SPU_PlaySound */

void SPU_Initialize ( void ) {

 if ( !s_ClientDataA.server ) _Init ();

}  /* end SPU_Initialize */

SPUContext* SPU_InitContext ( int anChannels, int aFreq, int aVolume ) {

 s_Buffer[ 0 ] = aFreq;
 s_Buffer[ 1 ] = 16;
 s_Buffer[ 2 ] = anChannels;
 s_Buffer[ 3 ] = aVolume;

 SifCallRpc ( &s_ClientDataA, 0, 0, s_Buffer, 16, s_Buffer, 4, NULL, NULL );

 s_SPUCtx.m_BufTime = (  ( float )s_Buffer[ 0 ] * 1000.0F  ) / (  ( aFreq << 1 ) * anChannels  );
 s_SPUCtx.PlayPCM   = SPU_PlayPCM;
 s_SPUCtx.SetVolume = SPU_SetVolume;
 s_SPUCtx.Destroy   = SPU_Destroy;
 s_SPUCtx.Silence   = SPU_Silence;

 return &s_SPUCtx;

}  /* end SPU_InitContext */
