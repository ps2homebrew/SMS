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
	int i=(v)/(2.0f*M_PI);
	v-=i*2.0f*M_PI;

	fac=1.0f;
	res=0.0f;
	w=1.0f;
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
	int i=(v)/(2.0f*M_PI);
	v-=i*2.0f*M_PI;

	fac=1.0f;
	res=0.0f;
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

///////////////////////////////////////////////////////////////////////////////
// float PbLog
///////////////////////////////////////////////////////////////////////////////

int PbLog( int Value )
{
  int r = 0;
  
  Value--;

  while( Value > 0 )
  {
    Value = Value >> 1;
    r++;
  }
  
  return r;
}

///////////////////////////////////////////////////////////////////////////////
// int PbRand
///////////////////////////////////////////////////////////////////////////////

// taken from D.E. Knuth - The Art of Computer Programming 3rd edition p. 185-186

#define MM 2147483647
#define AA 48271
#define QQ 44488
#define RR 3399

int PbRand()
{
	static int X = 0xfedeabe; // the seed
	X=AA*(X%QQ)-RR*(X/QQ);
	if(X<0) X+=MM;
	return X & PB_RAND_MAX;
}
