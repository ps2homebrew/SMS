/*
 * PbVif.h - VIF functions/defines for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBVIF_H_
#define _PBVIF_H_

#include <tamtypes.h>

///////////////////////////////////////////////////////////////////////////////
// Command defines
///////////////////////////////////////////////////////////////////////////////

#define VIF_NOP				  0x00
#define VIF_STCYL			  0x01 
#define VIF_OFFSET		  0x02
#define VIF_BASE			  0x03	
#define VIF_ITOP        0x04
#define VIF_STMOD			  0x05	
#define VIF_STROW			  0x30	
#define VIF_FLUSH			  0x11	
#define VIF_FLUSHA			0x13	
#define VIF_FLUSHE			0x10	
#define VIF_MSCAL			  0x14	
#define VIF_MSCALF			0x15	
#define VIF_MSCNT			  0x17	
#define VIF_MPG				  0x4A	
#define VIF_DIRECT  	  0x50
#define VIF_DIRECTHL	  0x51	

///////////////////////////////////////////////////////////////////////////////
// Unpack related defines
///////////////////////////////////////////////////////////////////////////////

#define VIF_UNPACK			  0x60
#define VIF_UNPACK_MASK		0x10

#define VIF_UNPACK_DBLBUF	0x8000

#define VIF_UNPACK_S_32		(VIF_UNPACK|0x00)
#define VIF_UNPACK_S_16		(VIF_UNPACK|0x01)
#define VIF_UNPACK_S_8		(VIF_UNPACK|0x02)
#define VIF_UNPACK_V2_32	(VIF_UNPACK|0x04)
#define VIF_UNPACK_V2_16	(VIF_UNPACK|0x05)
#define VIF_UNPACK_V2_8		(VIF_UNPACK|0x06)
#define VIF_UNPACK_V3_32	(VIF_UNPACK|0x08)
#define VIF_UNPACK_V3_16	(VIF_UNPACK|0x09)
#define VIF_UNPACK_V3_8		(VIF_UNPACK|0x0A)
#define VIF_UNPACK_V4_32	(VIF_UNPACK|0x0C)
#define VIF_UNPACK_V4_16	(VIF_UNPACK|0x0D)
#define VIF_UNPACK_V4_8		(VIF_UNPACK|0x0E)
#define VIF_UNPACK_V4_5		(VIF_UNPACK|0x0F)

///////////////////////////////////////////////////////////////////////////////
// Unpack mode defines
///////////////////////////////////////////////////////////////////////////////

#define VIF_MODE_NORMAL     0x00
#define VIF_MODE_OFFSET     0x01
#define VIF_MODE_DIFFERENCE 0x02

///////////////////////////////////////////////////////////////////////////////
// Misc defines
///////////////////////////////////////////////////////////////////////////////

#define VIF_CODE(CMD,NUM,IMMEDIATE) ((((unsigned int)(CMD))<<24) | \
                                     (((unsigned int)(NUM))<<16) | \
                                      ((unsigned int)(IMMEDIATE)))

#define VIF_STAT_PROGRAM_RUNNING  0x04
#define VIF_STAT_UNPACKING		    0x03
#define VIF_STAT_GIF_TRANSFERING  0x08

///////////////////////////////////////////////////////////////////////////////
// Public Functions
///////////////////////////////////////////////////////////////////////////////

void PbVif1UploadPrg( int Dest, void* pStart, void* pEnd );

///////////////////////////////////////////////////////////////////////////////
// Private Functions (mostly used internally by Public functions
///////////////////////////////////////////////////////////////////////////////

int PbVifGetProgSize( u32* pStart, u32* pEnd );

#endif //_PBVIF_H_

