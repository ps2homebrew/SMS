#ifndef _PBVU1_H_
#define _PBVU1_H_

#include <tamtypes.h>

#define VIF1_STAT   ((volatile u32 *)(0x10003c00))
#define VIF1_FBRST  ((volatile u32 *)(0x10003c10))
#define VIF1_ERR    ((volatile u32 *)(0x10003c20))
#define VIF1_MARK   ((volatile u32 *)(0x10003c30))

enum
{
  VU1M_TS_SIMPLE,	// Vu1Micro TriStrip Drawer.
};

void PbVu1_Start( int adress );
void PbVu1_AddToChain( int ProgId,void* pChain,int Adress );

///////////////////////////////////////////////////////////////////////////////
// Internal function(s)
///////////////////////////////////////////////////////////////////////////////

int PbVu1_GetSize( u32* pStart, u32* pEnd );

///////////////////////////////////////////////////////////////////////////////
// Debug function(s)
///////////////////////////////////////////////////////////////////////////////

void PbVu1_DumpMem();
void PbVu1_ShowStats();
void PbVu1_Wait();

#define VU1_MICROMEM ((volatile void*)0x11008000)
#define VU1_MEM      ((volatile void*)0x1100c000)

#endif //_PBVU1_H_

