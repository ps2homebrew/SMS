// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "VuInstruction.h"

/////////////////////////////// PUBLIC ///////////////////////////////////////
void
VuInstruction::Reset() {
    addr        = 0;
    tics        = 0;
    breakpoint  = 0;
    SymbolIndex = -1;
}


/////////////////////////////// PRIVATE    ///////////////////////////////////
