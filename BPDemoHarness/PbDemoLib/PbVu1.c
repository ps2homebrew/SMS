/*
 * PbVu1.h - Vector unit 1 functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbVu1.h"
#include "PbFile.h"
#include "PbDma.h"

///////////////////////////////////////////////////////////////////////////////
// void PbVu1Dump()
///////////////////////////////////////////////////////////////////////////////

void PbVu1Dump()
{
  PbDmaWait01();
  PbVu1Wait();
  PbFileWrite( "host:VU1MICROMEM",((void*)0x11008000),1024*16 );
  PbFileWrite( "host:VU1MEM", ((void*)0x1100c000),1024*16 );
}

///////////////////////////////////////////////////////////////////////////////
// void PbVu1Dump()
///////////////////////////////////////////////////////////////////////////////

void PbVu1Wait()
{
	asm __volatile__(
		"nop\n"
		"nop\n"
"0:\n"
		"bc2t 0b\n"
		"nop\n"
		"nop\n" );
}

