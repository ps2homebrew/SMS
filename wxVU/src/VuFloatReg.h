// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __VUFLOATREG__
#define __VUFLOATREG__

#include "VuReg.h"
#include "util.h"

class VuFloatReg : public VuReg {
public:
	float x();
	float y();
	float z();
	float w();
	void set(float v);
	void x(float v);
	void y(float v);
	void z(float v);
	void w(float v);
	void xtoi(float v, int a);
	void ytoi(float v, int a);
	void ztoi(float v, int a);
	void wtoi(float v, int a);
	void xtof(int v, int a);
	void ytof(int v, int a);
	void ztof(int v, int a);
	void wtof(int v, int a);
	void mcopy(int *dest, int reg);
	void mwrite(int *org, int reg);   
};
#endif
