#include <string>
#include <stdio.h>
#include "intc.h"

const string tINTC_STAT[2] = {
    "No interrupt request exists",
    "An interrupt request exists"
};
const string tINTC_MASK[2] = {
    "Interrupt masked",
    "Interrupt enabled"
};

INTC::INTC() : SubSystem(INTC::nREGISTERS) {
}

INTC::~INTC() {
}

vector<string>
INTC::unpack_I_STAT(const int reg) {
    vector<string> v;
    v.push_back("GS");
    v.push_back(tINTC_STAT[REGISTERS[0]&0x1]);
    v.push_back("SBUS");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x2)>>1]);
    v.push_back("VBON");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x4)>>2]);
    v.push_back("VBOF");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x8)>>3]);
    v.push_back("VIF0");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x10)>>4]);
    v.push_back("VIF1");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x20)>>5]);
    v.push_back("VU0");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x40)>>6]);
    v.push_back("VU1");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x80)>>7]);
    v.push_back("IPU");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x100)>>8]);
    v.push_back("TIM0");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x200)>>9]);
    v.push_back("TIM1");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x400)>>10]);
    v.push_back("TIM2");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x800)>>11]);
    v.push_back("TIM3");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x1000)>>12]);
    v.push_back("SFIF0");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x2000)>>13]);
    v.push_back("VU0WD");
    v.push_back(tINTC_STAT[(REGISTERS[0]&0x4000)>>14]);
    // v.push_back("reserved");
    // v.push_back(tINTC_STAT[(REGISTERS[0]&0x8000)>>15]);
    return v;
}
vector<string>
INTC::unpack_I_MASK(const int reg) {
    vector<string> v;
    v.push_back("GS");
    v.push_back(tINTC_MASK[REGISTERS[1]&0x1]);
    v.push_back("SBUS");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x2)>>1]);
    v.push_back("VBON");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x4)>>2]);
    v.push_back("VBOF");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x8)>>3]);
    v.push_back("VIF0");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x10)>>4]);
    v.push_back("VIF1");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x20)>>5]);
    v.push_back("VU0");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x40)>>6]);
    v.push_back("VU1");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x80)>>7]);
    v.push_back("IPU");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x100)>>8]);
    v.push_back("TIM0");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x200)>>9]);
    v.push_back("TIM1");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x400)>>10]);
    v.push_back("TIM2");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x800)>>11]);
    v.push_back("TIM3");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x1000)>>12]);
    v.push_back("SFIF0");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x2000)>>13]);
    v.push_back("VU0WD");
    v.push_back(tINTC_MASK[(REGISTERS[1]&0x4000)>>14]);
    // v.push_back("reserved");
    // v.push_back(tINTC_MASK[(REGISTERS[1]&0x8000)>>15]);
    return v;
}

// public classes
vector<string>
INTC::getRegisterText(const int reg) {
    switch(reg) {
        case I_STAT:
            return unpack_I_STAT(reg);
            break;
        case I_MASK:
            return unpack_I_MASK(reg);
            break;
    }
}
