/*
 * PbMatrix.h - Matrix functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbMatrix.h"
#include "PbMath.h"
#include <stdio.h>
#include <stdlib.h>
#include <tamtypes.h>

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixMakeIdentity
///////////////////////////////////////////////////////////////////////////////

void PbMatrixIdentity( PbMatrix* pMatrix )
{
  int i,j;
  
  for( i = 0; i < 4; i++ )
  {
    for( j = 0; j < 4; j++ )
      pMatrix->m_fMatrix[(i<<2)+j] = 0.0f;
    
    pMatrix->m_fMatrix[(i<<2)+i] = 1.0f;
  }
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixMul
///////////////////////////////////////////////////////////////////////////////

void PbMatrixMultiply( PbMatrix* pDest,const PbMatrix* pSrc1,const PbMatrix* pSrc2 )
{
  // taken from jar's "slave of the vu" code

  asm __volatile__(
    "lqc2            vf16,0x00(%1)\n"
    "lqc2            vf17,0x10(%1)\n"
    "lqc2            vf18,0x20(%1)\n"
    "lqc2            vf19,0x30(%1)\n"
    "lqc2            vf20,0x00(%2)\n"
    "lqc2            vf21,0x10(%2)\n"
    "lqc2            vf22,0x20(%2)\n"
    "lqc2            vf23,0x30(%2)\n"
    "vmulax.xyzw     ACC,vf20,vf16\n"
    "vmadday.xyzw    ACC,vf21,vf16\n"
    "vmaddaz.xyzw    ACC,vf22,vf16\n"
    "vmaddw.xyzw     vf16,vf23,vf16\n"
    "vmulax.xyzw     ACC,vf20,vf17\n"
    "vmadday.xyzw    ACC,vf21,vf17\n"
    "vmaddaz.xyzw    ACC,vf22,vf17\n"
    "vmaddw.xyzw     vf17,vf23,vf17\n"
    "vmulax.xyzw     ACC,vf20,vf18\n"
    "vmadday.xyzw    ACC,vf21,vf18\n"
    "vmaddaz.xyzw    ACC,vf22,vf18\n"
    "vmaddw.xyzw     vf18,vf23,vf18\n"
    "vmulax.xyzw     ACC,vf20,vf19\n"
    "vmadday.xyzw    ACC,vf21,vf19\n"
    "vmaddaz.xyzw    ACC,vf22,vf19\n"
    "vmaddw.xyzw     vf19,vf23,vf19\n"
    "sqc2            vf16,0x00(%0)\n"
    "sqc2            vf17,0x10(%0)\n"
    "sqc2            vf18,0x20(%0)\n"
    "sqc2            vf19,0x30(%0)\n"
    : : "r"(pDest), "r"(pSrc1), "r"(pSrc2) : "memory");
}

///////////////////////////////////////////////////////////////////////////////
// void SAMatrix_BuildHeading
///////////////////////////////////////////////////////////////////////////////

void PbMatrixRotateY( PbMatrix* pMatrix, float angle )
{
  float sin_angle = (float)PbSin( angle );
  float cos_angle = (float)PbCos(angle );

  pMatrix->m_fMatrix[(0<<2)+0] = cos_angle;
  pMatrix->m_fMatrix[(0<<2)+1] = 0;
  pMatrix->m_fMatrix[(0<<2)+2] = -sin_angle;
  pMatrix->m_fMatrix[(0<<2)+3] = 0;

  pMatrix->m_fMatrix[(1<<2)+0] = 0;
  pMatrix->m_fMatrix[(1<<2)+1] = 1;
  pMatrix->m_fMatrix[(1<<2)+2] = 0;
  pMatrix->m_fMatrix[(1<<2)+3] = 0;

  pMatrix->m_fMatrix[(2<<2)+0] = sin_angle;
  pMatrix->m_fMatrix[(2<<2)+1] = 0;
  pMatrix->m_fMatrix[(2<<2)+2] = cos_angle;
  pMatrix->m_fMatrix[(2<<2)+3] = 0;

  pMatrix->m_fMatrix[(3<<2)+0] = 0;
  pMatrix->m_fMatrix[(3<<2)+1] = 0;
  pMatrix->m_fMatrix[(3<<2)+2] = 0;
  pMatrix->m_fMatrix[(3<<2)+3] = 1;
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixBuildPitch
///////////////////////////////////////////////////////////////////////////////

void PbMatrixRotateX( PbMatrix* pMatrix, float angle )
{
  float sin_angle = (float)PbSin( angle );
  float cos_angle = (float)PbCos(angle );

  pMatrix->m_fMatrix[(0<<2)+0] = 1;
  pMatrix->m_fMatrix[(0<<2)+1] = 0;
  pMatrix->m_fMatrix[(0<<2)+2] = 0;
  pMatrix->m_fMatrix[(0<<2)+3] = 0;

  pMatrix->m_fMatrix[(1<<2)+0] = 0;
  pMatrix->m_fMatrix[(1<<2)+1] = cos_angle;
  pMatrix->m_fMatrix[(1<<2)+2] = sin_angle;
  pMatrix->m_fMatrix[(1<<2)+3] = 0;

  pMatrix->m_fMatrix[(2<<2)+0] = 0;
  pMatrix->m_fMatrix[(2<<2)+1] = -sin_angle;
  pMatrix->m_fMatrix[(2<<2)+2] = cos_angle;
  pMatrix->m_fMatrix[(2<<2)+3] = 0;

  pMatrix->m_fMatrix[(3<<2)+0] = 0;
  pMatrix->m_fMatrix[(3<<2)+1] = 0;
  pMatrix->m_fMatrix[(3<<2)+2] = 0;
  pMatrix->m_fMatrix[(3<<2)+3] = 1;
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixBuildBank
///////////////////////////////////////////////////////////////////////////////

void PbMatrixRotateZ( PbMatrix* pMatrix, float angle )
{
  float sin_angle = (float)PbSin( angle );
  float cos_angle = (float)PbCos(angle );

  pMatrix->m_fMatrix[(0<<2)+0] = cos_angle;
  pMatrix->m_fMatrix[(0<<2)+1] = sin_angle;
  pMatrix->m_fMatrix[(0<<2)+2] = 0;
  pMatrix->m_fMatrix[(0<<2)+3] = 0;

  pMatrix->m_fMatrix[(1<<2)+0] = -sin_angle;
  pMatrix->m_fMatrix[(1<<2)+1] = cos_angle;
  pMatrix->m_fMatrix[(1<<2)+2] = 0;
  pMatrix->m_fMatrix[(1<<2)+3] = 0;

  pMatrix->m_fMatrix[(2<<2)+0] = 0;
  pMatrix->m_fMatrix[(2<<2)+1] = 0;
  pMatrix->m_fMatrix[(2<<2)+2] = 1;
  pMatrix->m_fMatrix[(2<<2)+3] = 0;

  pMatrix->m_fMatrix[(3<<2)+0] = 0;
  pMatrix->m_fMatrix[(3<<2)+1] = 0;
  pMatrix->m_fMatrix[(3<<2)+2] = 0;
  pMatrix->m_fMatrix[(3<<2)+3] = 1;
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixCreateViewScreen
///////////////////////////////////////////////////////////////////////////////

void PbMatrixViewScreen( PbMatrix* pMatrix, float scrz, float ax, float ay, 
                                            float cx, float cy, float zmin, 
                                            float zmax, float nearz, float farz )
{
  PbMatrix temp2;
  PbMatrix temp1;

  float cz = (-zmax * nearz + zmin * farz) / (-nearz + farz);
  float az  = farz * nearz * (-zmin + zmax) / (-nearz + farz);

  //     | scrz    0  0 0 |
  // m = |    0 scrz  0 0 | 
  //     |    0    0  0 1 |
  //     |    0    0  1 0 |
  
  PbMatrixIdentity( &temp1 ); 
  temp1.m_fMatrix[(0<<2)+0] = scrz;
  temp1.m_fMatrix[(1<<2)+1] = scrz;
  temp1.m_fMatrix[(2<<2)+2] = 0.0f;
  temp1.m_fMatrix[(3<<2)+3] = 0.0f;
  temp1.m_fMatrix[(3<<2)+2] = 1.0f;
  temp1.m_fMatrix[(2<<2)+3] = 1.0f;


  //     | ax  0  0 cx |
  // m = |  0 ay  0 cy | 
  //     |  0  0 az cz |
  //     |  0  0  0  1 |
  
  PbMatrixIdentity( &temp2 ); 
  temp2.m_fMatrix[(0<<2)+0] = ax;
  temp2.m_fMatrix[(1<<2)+1] = ay;
  temp2.m_fMatrix[(2<<2)+2] = az;
  temp2.m_fMatrix[(3<<2)+0] = cx;
  temp2.m_fMatrix[(3<<2)+1] = cy;
  temp2.m_fMatrix[(3<<2)+2] = cz;

  PbMatrixMultiply( pMatrix, &temp1, &temp2 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixPrint
///////////////////////////////////////////////////////////////////////////////

void PbMatrixTranslate( PbMatrix* pMatrix, float x, float y, float z )
{
  pMatrix->m_fMatrix[(3<<2)+0] = x;
  pMatrix->m_fMatrix[(3<<2)+1] = y;
  pMatrix->m_fMatrix[(3<<2)+2] = z;
}

///////////////////////////////////////////////////////////////////////////////
// void PbMatrixPrint
///////////////////////////////////////////////////////////////////////////////

void PbMatrixPrint( const char* pName, PbMatrix* pMatrix )
{
  int i = 0;
  
  //printf( "============== MATRIX: %s ==============\n", pName );
  
  for( i = 0; i < 4; i++)
  {
    //gp_Info->printf( "matrix[(%d<<2)+0] = %f\n", i, pMatrix->m_fMatrix[(i<<2)+0] );
    //gp_Info->printf( "matrix[(%d<<2)+1] = %f\n", i, pMatrix->m_fMatrix[(i<<2)+1] );
    //gp_Info->printf( "matrix[(%d<<2)+2] = %f\n", i, pMatrix->m_fMatrix[(i<<2)+2] );
    //gp_Info->printf( "matrix[(%d<<2)+3] = %f\n", i, pMatrix->m_fMatrix[(i<<2)+3] );
  }
}

