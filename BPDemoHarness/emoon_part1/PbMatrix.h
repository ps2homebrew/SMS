#ifndef _PBMATRIX_H_
#define _PBMATRIX_H_

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

void PbMatrix_MakeIdentity( PbMatrix* pMatrix );
void PbMatrix_Rotate( PbMatrix* pMatrix, float head,float pitch, float bank );
void PbMatrix_Scale( PbMatrix* pMatrix, float xscale, float yscale, float zscale );

void PbMatrix_Multiply( PbMatrix* pDest,const PbMatrix* pSrc1,const PbMatrix* pSrc2 );
void PbMatrix_Invert( PbMatrix* pDest,const PbMatrix* pSrc );
void PbMatrix_Translate( PbMatrix* pMatrix, float x, float y, float z );

void PbMatrix_CreateViewScreen( PbMatrix* pMatrix, float scrz, float ax, float ay, 
																									 float cx, float cy, float zmin, 
																									 float zmax, float nearz, float farz );

///////////////////////////////////////////////////////////////////////////////
// Debug
///////////////////////////////////////////////////////////////////////////////

void PbMatrix_Print( const char* pName, PbMatrix* pMatrix );

///////////////////////////////////////////////////////////////////////////////
// Private
///////////////////////////////////////////////////////////////////////////////

void PbMatrix_BuildHeading( PbMatrix* pMatrix, float angle );
void PbMatrix_BuildPitch( PbMatrix* pMatrix, float angle );
void PbMatrix_BuildBank( PbMatrix* pMatrix, float angle );

#endif // _SAMATRIX_H_
 

