#include "math.h"

#define M_PI 3.14159265358979323846

#define __my_math_iter 20

// Taylor series
float PbCos(float v)
{
	float res,w;
	int t;
	float fac;
	int i=(v)/(2*M_PI);
	v-=i*2*M_PI;

	fac=1;
	res=0;
	w=1;
	for(t=0;t<__my_math_iter;)
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

float PbSin(float v)
{
	float res,w;
	int t;
	float fac;
	int i=(v)/(2*M_PI);
	v-=i*2*M_PI;

	fac=1;
	res=0;
	w=v;
	for(t=1;t<__my_math_iter;)
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

