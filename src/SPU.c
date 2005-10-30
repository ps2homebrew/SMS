#include "SPU.h"
#include "SIF.h"

#include <kernel.h>
#include <loadfile.h>
#include <stdio.h>

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

SPUContext* SPU_InitContext ( int anChannels, int aFreq, int aVolume ) {

 if (  !s_ClientDataA.server && !_Init ()  ) return NULL;

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
