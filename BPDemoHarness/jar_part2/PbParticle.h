/*
 * PbParticle.h - EE-only (so far) Particle functions for Pb demo library
 *
 * Copyright (c) 2004   jar <dsl123588@vip.cybercity.dk>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 *
 * Makes use of PbDemoLib by emoon
 * Copyright (c) 2004   emoon <daniel@collin.com> 
 */

#ifndef _PBPARTICLE_H_
#define _PBPARTICLE_H_

/*
 * You must call PbParticleSetup() before calling PbParticleDraw()
 * So far all particles use the same color, if needed i'll add an
 * alternative draw function that allows for individual color/alpha/etc.
 * I also have plans of providing a VU1 drawer for the particles, but I
 * want the interface in place first.
 */

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void PbParticleSetup(PbMatrix* pLocalToScreen, PbTexture* pTex, float fParticleMaxSize, float fParticleDistanceFalloff, int iColor);
void PbParticleDraw(PbFvec* pPoints, int iNumPoints);

#endif
