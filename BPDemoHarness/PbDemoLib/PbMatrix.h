/*
 * PbMatrix.h - Matrix functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBMATRIX_H_
#define _PBMATRIX_H_

#include "PbVec.h"

///////////////////////////////////////////////////////////////////////////////
// structs
///////////////////////////////////////////////////////////////////////////////

typedef struct st_PbMatrix
{
  float    m_fMatrix[4*4];
} PbMatrix __attribute__((aligned(16)));

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void PbMatrixIdentity( PbMatrix* pMatrix );
void PbMatrixRotate( PbMatrix* pMatrix, float x,float y, float z );
void PbMatrixScale( PbMatrix* pMatrix, float xscale, float yscale, float zscale );

void PbMatrixMultiply( PbMatrix* pDest,const PbMatrix* pSrc1,const PbMatrix* pSrc2 );
void PbMatrixInvert( PbMatrix* pDest,const PbMatrix* pSrc );
void PbMatrixTranslate( PbMatrix* pMatrix, float x, float y, float z );

///////////////////////////////////////////////////////////////////////////////
// Util functions
///////////////////////////////////////////////////////////////////////////////

void PbMatrixViewScreen( PbMatrix* pMatrix, float scrz, float ax, float ay, 
                                            float cx, float cy, float zmin, 
                                            float zmax, float nearz, float farz );
void PbMatrixPrint( const char* pName, PbMatrix* pMatrix );

///////////////////////////////////////////////////////////////////////////////
// Rotation functions
///////////////////////////////////////////////////////////////////////////////

void PbMatrixRotateY( PbMatrix* pMatrix, float angle );
void PbMatrixRotateX( PbMatrix* pMatrix, float angle );
void PbMatrixRotateZ( PbMatrix* pMatrix, float angle );

#endif // _PBMATRIX_H_


