// -*- C++ -*-
// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include <string>
#include <stdio.h>
#include "vif1.h"

const int VIF1::nREGISTERS = 22;

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
static const string tVGW[2] = {
    "Not-wait",
    "Wait (Stopped status with one of FLUSH/FLUSHA, DIRECT/DIRECTHL)"
};
static const string tMRK[2] = {
    "Not-detect",
    "Detect"
};
static const string tDBF[2] = {
    "TOPS = BASE",
    "TOPS = BASE+OFFSET"
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
static const string tFDR[2] = {
    "Main memory/SPRAM -> VIF1",
    "VIF1 -> Main memory/SPRAM"
};

VIF1::VIF1() : VIF(VIF1::nREGISTERS) {
}

VIF1::~VIF1() {
}

// public classes
vector<string>
VIF1::getRegisterText(const int reg) {
    switch(reg) {
        case VIF1_STAT:
            return unpack_VIF1_STAT(reg);
            break;
        // case VIF1_FBRST:
            // return unpack_VIF1_FBRST(reg);
            // break;
        case VIF1_ERR:
            return unpack_VIF_ERR(reg);
            break;
        case VIF1_MARK:
            return unpack_VIF_MARK(reg);
            break;
        case VIF1_CYCLE:
            return unpack_VIF_CYCLE(reg);
            break;
        case VIF1_MODE:
            return unpack_VIF_MODE(reg);
            break;
        case VIF1_NUM:
            return unpack_VIF_NUM(reg);
            break;
        case VIF1_MASK:
            return unpack_VIF_MASK(reg);
            break;
        case VIF1_CODE:
            return unpack_VIF_CODE(reg);
            break;
        case VIF1_ITOPS:
            return unpack_VIF_ITOPS(reg);
            break;
        case VIF1_ITOP:
            return unpack_VIF_ITOP(reg);
            break;
        case VIF1_BASE:
            return unpack_VIF1_BASE(reg);
            break;
        case VIF1_OFST:
            return unpack_VIF1_OFST(reg);
            break;
        case VIF1_TOP:
            return unpack_VIF1_TOP(reg);
            break;
        case VIF1_TOPS:
            return unpack_VIF1_TOPS(reg);
            break;
        case VIF1_R0:
        case VIF1_R1:
        case VIF1_R2:
        case VIF1_R3:
            return unpack_VIF_R(reg);
            break;
        case VIF1_C0:
        case VIF1_C1:
        case VIF1_C2:
        case VIF1_C3:
            return unpack_VIF_C(reg);
            break;
    }
}

vector<string>
VIF1::unpack_VIF1_STAT(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("VPS");
    v.push_back(tVPS[REGISTERS[reg]&0x00000003]);
    v.push_back("VEW");
    v.push_back(tVEW[(REGISTERS[reg]&0x00000004)>>2]);
    v.push_back("VGW");
    v.push_back(tVGW[(REGISTERS[reg]&0x00000008)>>3]);
    v.push_back("MRK");
    v.push_back(tMRK[(REGISTERS[reg]&0x00000040)>>6]);
    v.push_back("DBF");
    v.push_back(tDBF[(REGISTERS[reg]&0x00000080)>>7]);
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
    v.push_back("FDR");
    v.push_back(tFDR[(REGISTERS[reg]&0x00800000)>>23]);
    v.push_back("FQC");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x1F000000)>>24);
    v.push_back(val);
    return v;
}
vector<string>
VIF1::unpack_VIF1_TOPS(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("TOPS");
    sprintf(val, "%d", REGISTERS[reg]&0x3ff);
    v.push_back(val);
    return v;
}
vector<string>
VIF1::unpack_VIF1_TOP(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("TOP");
    sprintf(val, "%d", REGISTERS[reg]&0x3ff);
    v.push_back(val);
    return v;
}
vector<string>
VIF1::unpack_VIF1_BASE(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("BASE");
    sprintf(val, "%d", REGISTERS[reg]&0x3ff);
    v.push_back(val);
    return v;
}
vector<string>
VIF1::unpack_VIF1_OFST(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("OFFSET");
    sprintf(val, "%d", REGISTERS[reg]&0x3ff);
    v.push_back(val);
    return v;
}
