#include <string>
#include <stdio.h>
#include "gs.h"

static const string tSIGNAL[2] = {
    "SIGNAL event has not been generated",
    "SIGNAL event has been generated"
};
static const string tFINISH[2] = {
    "FINISH event has not been generated",
    "FINISH event has been generated"
};
static const string tHSINT[2] = {
    "HSync interrupt has not been generated",
    "HSync interrupt has been generated"
};
static const string tVSINT[2] = {
    "VSync interrupt has not been generated",
    "VSync interrupt has been generated"
};
static const string tEDWINT[2] = {
    "Rectangular area write interrupt has not been generated",
    "Rectangular area write interrupt has been generated",
};
static const string tFLUSH[2] = {
    "Not flushed",
    "Flushed"
};

GS::GS() : SubSystem(GS::nREGISTERS) {
}

GS::~GS() {
}

vector<string>
GS::getRegisterText(const int reg) {
    switch(reg) {
        case GS_SIGLBLD:
            return unpack_SIGLBLID(reg);
            break;
        case GS_CSR:
            return unpack_CSR(reg);
            break;
    }
}

vector<string>
GS::unpack_CSR(const int reg) {
    vector<string> v;
    v.push_back("SIGNAL");
    v.push_back(tSIGNAL[(REGISTERS[reg]&0x1)]);
    v.push_back("FINISH");
    v.push_back(tFINISH[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("HSINT");
    v.push_back(tHSINT[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("VSINT");
    v.push_back(tVSINT[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("EDWINT");
    v.push_back(tEDWINT[(REGISTERS[reg]&0x10)>>4]);
    v.push_back("FLUSH");
    v.push_back(tFLUSH[(REGISTERS[reg]&0x20)>>5]);
    return v;
}

vector<string>
GS::unpack_SIGLBLID(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("SIGID");
    sprintf(val, "0x%x", REGISTERS[reg]&0xFFFFFFFF);
    v.push_back(val);
    v.push_back("LBLID");
    // sprintf(val, "0x%x", (GS_REGS[18]&0xFFFFFFFF00000000)>>32);
    v.push_back(val);
    return v;
}
