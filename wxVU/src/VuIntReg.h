// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#ifndef __VUINTREG__
#define __VUINTREG__

#include "VuReg.h"

class VuIntReg : public VuReg {
public:
	int16   value();              //read values from register
	void    value(int16 v);        //sets values on register
private:

};
#endif
