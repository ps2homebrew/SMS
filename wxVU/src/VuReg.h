// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#ifndef __VUREG__
#define __VUREG__

#include "datatypes.h"

class VuReg {
private:
	int vstall;
	int lRead;
	int lWrite;
protected:
	vec data;
public:
	int stall(); //stall states before can accessed
	int lastRead(); //last instruction in read value
	int lastWrite(); //last instruction in write value
	void stall(int v); //set previously defined values
	void decstall();
	void lastRead(int v);
	void lastWrite(int v);
};

#endif
