// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#include "VuParam.h"

/////////////////////////////// PUBLIC ///////////////////////////////////////
void VuParam::Reset() {
    type        = 0;
    index       = 0;
    sufix[0]    = 0;
    udata       = 0;
    data        = 0;
    label[0]    = 0;
    stalling    = 0;
    memdir      = 0; 
}
/////////////////////////////// PRIVATE ///////////////////////////////////////
