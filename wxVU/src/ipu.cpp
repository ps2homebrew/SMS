#include <string>
#include <stdio.h>
#include "ipu.h"

static const string tIPU_CMD[2] = {
    "DATA field enable",
    "DATA field disable"
};
static const string tIPU_TOP[2] = {
    "BSTOP field enable",
    "BSTOP field disable"
};
static const string tIPU_ECD[2] = {
    "Not detected",
    "Detected"
};
static const string tIPU_IDP[4] = {
    "8 bits",
    "9 bits",
    "10 bits",
    "Reserved"
};
static const string tIPU_AS[2] = {
    "Zigzag scanning",
    "Alternate scanning"
};
static const string tIPU_IVF[2] = {
    "Mpeg1-compatible 2-dimensional VLC table",
    "2-dimensional VLC table specially for intra macro block"
};
static const string tIPU_QST[2] = {
    "Linear step",
    "Non-line step"
};
static const string tIPU_MP1[2] = {
    "MPEG2 bit stream",
    "MPEG1 bit stream"
};
static const string tIPU_PCT[5] = {
    "Reserved",
    "I-Picture",
    "P-Picture",
    "B-Picture",
    "D-Picture"
};
static const string tIPU_BUSY[2] = {
    "Ready",
    "Busy (command in execution)"
};

vector<string>
IPU::unpack_IPU_CMD(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("DATA");
    sprintf(val, "%d", REGISTERS[reg]&0xffffffff);
    v.push_back(val);
    return v;
}
vector<string>
IPU::unpack_IPU_TOP(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("BSTOP");
    sprintf(val, "%d", REGISTERS[reg]&0xffffffff);
    v.push_back(val);
    return v;
}
vector<string>
IPU::unpack_IPU_CTRL(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("IFC");
    sprintf(val, "%d", REGISTERS[reg]&0xf);
    v.push_back(val);
    v.push_back("OFC");
    sprintf(val, "%d", (REGISTERS[reg]&0xf0)>>4);
    v.push_back(val);
    v.push_back("CBP");
    sprintf(val, "%d", (REGISTERS[reg]&0x3F00)>>8);
    v.push_back(val);
    v.push_back("ECD");
    v.push_back(tIPU_ECD[(REGISTERS[reg]&0x4000)>>14]);
    v.push_back("SCD");
    v.push_back(tIPU_ECD[(REGISTERS[reg]&0x8000)>>15]);
    v.push_back("IDP");
    v.push_back(tIPU_IDP[(REGISTERS[reg]&0x30000)>>16]);
    v.push_back("AS");
    v.push_back(tIPU_AS[(REGISTERS[reg]&0x100000)>>20]);
    v.push_back("IVF");
    v.push_back(tIPU_IVF[(REGISTERS[reg]&0x200000)>>21]);
    v.push_back("QST");
    v.push_back(tIPU_QST[(REGISTERS[reg]&0x400000)>>22]);
    v.push_back("MP1");
    v.push_back(tIPU_MP1[(REGISTERS[reg]&0x800000)>>23]);
    v.push_back("PCT");
    v.push_back(tIPU_PCT[(REGISTERS[reg]&0x7000000)>>24]);
    v.push_back("BUSY");
    v.push_back(tIPU_BUSY[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
IPU::unpack_IPU_BP(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("BP");
    sprintf(val, "%d", (REGISTERS[reg]&0x7f));
    v.push_back(val);
    v.push_back("IFC");
    sprintf(val, "%d", (REGISTERS[reg]&0xf00)>>8);
    v.push_back(val);
    v.push_back("FP");
    sprintf(val, "%d", (REGISTERS[reg]&0x30000)>>16);
    v.push_back(val);
    return v;
}

IPU::IPU() : SubSystem(IPU::nREGISTERS) {
}
IPU::~IPU() {
}

// public classes
vector<string>
IPU::getRegisterText(const int reg) {
    switch(reg) {
        case IPU_CMD:
            return unpack_IPU_CMD(reg);
            break;
        case IPU_TOP:
            return unpack_IPU_TOP(reg);
            break;
        case IPU_CTRL:
            return unpack_IPU_CTRL(reg);
            break;
        case IPU_BP:
            return unpack_IPU_BP(reg);
            break;
    }
}
