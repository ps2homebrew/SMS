#include <string>
#include <stdio.h>
#include "intc.h"

const int INTC::nREGISTERS = 2;

const string tINTC_STAT[2] = {
    "No interrupt request exists",
    "An interrupt request exists"
};
const string tINTC_MASK[2] = {
    "Interrupt masked",
    "Interrupt enabled"
};

/////////////////////////////// PUBLIC ///////////////////////////////////////
INTC::INTC() : SubSystem(INTC::nREGISTERS) {
}

INTC::~INTC() {
}

const vector<string>
INTC::GetRegisterText(const int reg) {
    switch(reg) {
        case I_STAT:
            return UnpackIStat(reg);
            break;
        case I_MASK:
            return UnpackIMask(reg);
            break;
        default:
            vector<string> v;
            return v; 
            break;
    }
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
vector<string>
INTC::UnpackIStat(const int reg) {
    vector<string> v;
    v.push_back("GS");
    v.push_back(tINTC_STAT[REGISTERS[reg]&0x1]);
    v.push_back("SBUS");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("VBON");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("VBOF");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("VIF0");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x10)>>4]);
    v.push_back("VIF1");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x20)>>5]);
    v.push_back("VU0");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x40)>>6]);
    v.push_back("VU1");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x80)>>7]);
    v.push_back("IPU");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x100)>>8]);
    v.push_back("TIM0");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x200)>>9]);
    v.push_back("TIM1");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x400)>>10]);
    v.push_back("TIM2");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x800)>>11]);
    v.push_back("TIM3");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x1000)>>12]);
    v.push_back("SFIF0");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x2000)>>13]);
    v.push_back("VU0WD");
    v.push_back(tINTC_STAT[(REGISTERS[reg]&0x4000)>>14]);
    // v.push_back("reserved");
    // v.push_back(tINTC_STAT[(REGISTERS[0]&0x8000)>>15]);
    return v;
}
vector<string>
INTC::UnpackIMask(const int reg) {
    vector<string> v;
    v.push_back("GS");
    v.push_back(tINTC_MASK[REGISTERS[reg]&0x1]);
    v.push_back("SBUS");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("VBON");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("VBOF");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("VIF0");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x10)>>4]);
    v.push_back("VIF1");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x20)>>5]);
    v.push_back("VU0");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x40)>>6]);
    v.push_back("VU1");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x80)>>7]);
    v.push_back("IPU");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x100)>>8]);
    v.push_back("TIM0");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x200)>>9]);
    v.push_back("TIM1");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x400)>>10]);
    v.push_back("TIM2");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x800)>>11]);
    v.push_back("TIM3");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x1000)>>12]);
    v.push_back("SFIF0");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x2000)>>13]);
    v.push_back("VU0WD");
    v.push_back(tINTC_MASK[(REGISTERS[reg]&0x4000)>>14]);
    // v.push_back("reserved");
    // v.push_back(tINTC_MASK[(REGISTERS[1]&0x8000)>>15]);
    return v;
}
