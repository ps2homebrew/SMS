#include <string>
#include "sif.h"

// public
vector<string>
SIF::getRegisterText(const int reg) {
    switch(reg) {
        case SB_SMFLAG:
            return unpack_SB_SMFLG();
            break;
        default:
            vector<string> v;
            return v;
            break;
    }
}

// private
vector<string>
SIF::unpack_SB_SMFLG(void) {
    vector<string> v;
    return v;
}

SIF::SIF() : SubSystem(SIFnREGISTERS) {
}
