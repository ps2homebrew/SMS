/*
 * PbVec.h - Vector functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBVEC_H_
#define _PBVEC_H_

///////////////////////////////////////////////////////////////////////////////
// structs
///////////////////////////////////////////////////////////////////////////////

typedef struct 
{
  float x,y,z,w;
} PbFvec __attribute__((aligned(16)));

typedef struct 
{
  unsigned int x,y,z,w;
} PbIvec __attribute__((aligned(16)));

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void PbFvecAdd( PbFvec* pDest,const PbFvec* pSrc1, const PbFvec* pSrc2 ); 

#endif // _PBVEC_H_






