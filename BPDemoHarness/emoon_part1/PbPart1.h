#ifndef _PBPART1_H_
#define _PBPART1_H_

#include <tamtypes.h>
#include "PbMatrix.h"

void PbPart1_SetInitalTime( float Time );
void PbPart1_Update( float DeltaTime );

void PbPart1_SetupVu1();
void PbPart1_SetupTextures();
void PbPart1_SetupGeneral();

void PbPart1_DrawEnvmapped( PbMatrix* pScreenToView,PbMatrix* pObjectToWorld, void* pChain );

void PbPart1_Main( PbMatrix* pScreenToView, PbMatrix* pCameraMatrix );

void* PbPart1_DrawObject( PbMatrix* pCameraToScreen,PbMatrix* pWorlToCamera,
                          float* pAngles, void* pChain );

///////////////////////////////////////////////////////////////////////////////
// State related functions
///////////////////////////////////////////////////////////////////////////////

enum
{
  PBP1_STATE_INIT_FLASH,
  PBP1_STATE_FLASH,
  PBP1_STATE_INIT_NORMAL,
  PBP1_STATE_NORMAL
};

int  PbPart1_GetState();
void PbPart1_SetState( int State );

///////////////////////////////////////////////////////////////////////////////
// States
///////////////////////////////////////////////////////////////////////////////

void PbPart1_InitFlash();
void PbPart1_Flash( float DeltaTime );
void PbPart1_InitNormal();
void PbPart1_Normal( float DeltaTime );

#endif//_PBPART1_H_

