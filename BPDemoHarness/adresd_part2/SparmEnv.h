#ifndef _PBPART6_H_
#define _PBPART6_H_

#include <tamtypes.h>
#include "PbMatrix.h"

void *PbPart6_DoObjects(PbMatrix *pCameraToScreen, PbMatrix *pWorldToCamera,void *pChain, int time,u32 beat_impulse);
void *PbPart6_Do2D(PbTexture* p_texture,PbTexture* p_texturetemp);

#endif//_PBPART6_H_

