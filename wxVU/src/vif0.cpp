// -*- C++ -*-
// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <string>
#include <stdio.h>
#include "vif0.h"

const int Vif0::nREGISTERS = 18;

Vif0::Vif0() : Vif(Vif0::nREGISTERS) {
}

Vif0::~Vif0() {
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
Vif0::UnpackStat(const int reg) {
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
const vector<string>
Vif0::GetRegisterText(const int reg) {
    switch(reg) {
        case VIF0_STAT:
            return UnpackStat(reg);
            break;
        // case VIF0_FBRST:
            // return unpack_VIF0_FBRST(reg);
            // break;
        case VIF0_ERR:
            return UnpackErr(reg);
            break;
        case VIF0_MARK:
            return UnpackMark(reg);
            break;
        case VIF0_CYCLE:
            return UnpackCycle(reg);
            break;
        case VIF0_MODE:
            return UnpackMode(reg);
            break;
        case VIF0_NUM:
            return UnpackNum(reg);
            break;
        case VIF0_MASK:
            return UnpackMask(reg);
            break;
        case VIF0_CODE:
            return UnpackCode(reg);
            break;
        case VIF0_R0:
        case VIF0_R1:
        case VIF0_R2:
        case VIF0_R3:
            return UnpackR(reg);
            break;
        case VIF0_C0:
        case VIF0_C1:
        case VIF0_C2:
        case VIF0_C3:
            return UnpackC(reg);
            break;
        default:
            vector<string> v;
            return v;
            break;
    }
}
