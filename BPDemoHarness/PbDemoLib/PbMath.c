/*
 * PbMatrix.h - Matrix functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include "PbMath.h"

///////////////////////////////////////////////////////////////////////////////
// float PbCos
// Taylor series
///////////////////////////////////////////////////////////////////////////////

float PbCos( float v )
{
	float res,w;
	int t;
	float fac;
	int i=(v)/(2*M_PI);
	v-=i*2*M_PI;

	fac=1;
	res=0;
	w=1;
	for(t=0;t<SIN_ITERATOR;)
	{
		res+=fac*w;
		w*=v*v;
		t++;
		fac/=t;
		t++;
		fac/=t;
		
		res-=fac*w;
		w*=v*v;
		t++;
		fac/=t;
		t++;
		fac/=t;
	}
	return res;
}

///////////////////////////////////////////////////////////////////////////////
// float PbSin
// Taylor series
///////////////////////////////////////////////////////////////////////////////

float PbSin( float v )
{
	float res,w;
	int t;
	float fac;
	int i=(v)/(2*M_PI);
	v-=i*2*M_PI;

	fac=1;
	res=0;
	w=v;
	for(t=1;t<SIN_ITERATOR;)
	{
		res+=fac*w;
		w*=v*v;
		t++;
		fac/=t;
		t++;
		fac/=t;
		
		res-=fac*w;
		w*=v*v;
		t++;
		fac/=t;
		t++;
		fac/=t;
	}
	return res;
}

///////////////////////////////////////////////////////////////////////////////
// float PbSqrt
///////////////////////////////////////////////////////////////////////////////

float PbSqrt(float a)
{
	float b;
	
	asm volatile(
	"sqrt.s %0,%1\n"
	: "=f" (b)
	: "f" (a));
	
	return b;
}

