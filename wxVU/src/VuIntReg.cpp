// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "VuIntReg.h"

int16 VuIntReg::value(){
    return data.vi;
}

void VuIntReg::value(int16 v){
    data.vi = v;
    return;
}

