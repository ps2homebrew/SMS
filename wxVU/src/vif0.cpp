// -*- C++ -*-
// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <string>
#include <stdio.h>
#include "vif0.h"

VIF0::VIF0() : VIF(VIF0::nREGISTERS) {
}

VIF0::~VIF0() {
}
static const string tVPS[4] = {
    "Idle",
    "Waits for the data following VIFcode.",
    "Decoding the VIFcode.",
    "Decompressing/transfering the data following VIFcode."
};
static const string tVEW[2] = {
    "Not-wait",
    "Wait (VU0 is executing a microprogram)"
};
static const string tMRK[2] = {
    "Not-detect",
    "Detect"
};
static const string tVSS[2] = {
    "Not-stall",
    "Stall"
};
static const string tVFS[2] = {
    "Not-stall",
    "Stall"
};
static const string tVIS[2] = {
    "Not-stall",
    "Stall"
};
static const string tINT[2] = {
    "Not-detect",
    "Detect"
};
static const string tER0[2] = {
    "No error",
    "Error (DMAtag has been detected in VIF packet)"
};
static const string tER1[2] = {
    "Not-detect ",
    "Detect (Undefined VIFcode has been detected)"
};

vector<string>
VIF0::unpack_VIF0_STAT(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("VPS");
    v.push_back(tVPS[REGISTERS[reg]&0x00000003]);
    v.push_back("VEW");
    v.push_back(tVEW[(REGISTERS[reg]&0x00000004)>>2]);
    v.push_back("MRK");
    v.push_back(tMRK[(REGISTERS[reg]&0x00000040)>>6]);
    v.push_back("VSS");
    v.push_back(tVSS[(REGISTERS[reg]&0x00000100)>>8]);
    v.push_back("VFS");
    v.push_back(tVFS[(REGISTERS[reg]&0x00000200)>>9]);
    v.push_back("VIS");
    v.push_back(tVIS[(REGISTERS[reg]&0x00000400)>>10]);
    v.push_back("INT");
    v.push_back(tINT[(REGISTERS[reg]&0x00000800)>>11]);
    v.push_back("ER0");
    v.push_back(tER0[(REGISTERS[reg]&0x00001000)>>12]);
    v.push_back("ER1");
    v.push_back(tER1[(REGISTERS[reg]&0x00002000)>>13]);
    v.push_back("FQC");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x0F000000)>>24);
    v.push_back(val);
    return v;
}

// public classes
vector<string>
VIF0::getRegisterText(const int reg) {
    switch(reg) {
        case VIF0_STAT:
            return unpack_VIF0_STAT(reg);
            break;
        // case VIF0_FBRST:
            // return unpack_VIF0_FBRST(reg);
            // break;
        case VIF0_ERR:
            return unpack_VIF_ERR(reg);
            break;
        case VIF0_MARK:
            return unpack_VIF_MARK(reg);
            break;
        case VIF0_CYCLE:
            return unpack_VIF_CYCLE(reg);
            break;
        case VIF0_MODE:
            return unpack_VIF_MODE(reg);
            break;
        case VIF0_NUM:
            return unpack_VIF_NUM(reg);
            break;
        case VIF0_MASK:
            return unpack_VIF_MASK(reg);
            break;
        case VIF0_CODE:
            return unpack_VIF_CODE(reg);
            break;
        case VIF0_ITOPS:
            return unpack_VIF_ITOPS(reg);
            break;
        case VIF0_ITOP:
            return unpack_VIF_ITOP(reg);
            break;
        case VIF0_R0:
        case VIF0_R1:
        case VIF0_R2:
        case VIF0_R3:
            return unpack_VIF_R(reg);
            break;
        case VIF0_C0:
        case VIF0_C1:
        case VIF0_C2:
        case VIF0_C3:
            return unpack_VIF_C(reg);
            break;
        default:
            vector<string> v;
            return v;
            break;
    }
}
