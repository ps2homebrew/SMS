#include <string>
#include <stdio.h>
#include "timer.h"

const char tMODE_CLKS[4][30] = {
    "BUSCLK (147.456)",
    "1/16th of BUSCLK",
    "1/256th of BUSCLK",
    "External clock (H-BLNK)",
};
const char tMODE_GATE[2][30] = {
    "Gate function not used",
    "Gate function is used"
};
const char tMODE_GATS[2][10] = {
    "H-BLNK",
    "V-BLNK"
};
const char tMODE_GATM[4][70] = {
    "Counts while the gate signal is low",
    "Resets and starts counting at the gate signals rising edge",
    "Resets and starts counting at the gate signals falling edge",
    "Resets and starts counting at both edges of the gate signal",
};
const char tMODE_ZRET[2][90] = {
    "The counter keeps counting, ignoring the reference value",
    "The counter is cleared to 0 when the counter value is equal to the reference value"
};
const char tMODE_CUE[2][30] = {
    "Stops counting",
    "Starts/restarts counting"
};
const char tMODE_CMPE[2][80] = {
    "A compare-interrupt is not generated",
    "An interrupt is generated when the counter is equal to the reference value"
};
const char tMODE_OVFE[2][60] = {
    "An overflow interrupt is not generated",
    "An interrupt is generated when an overflow occurs"
};
const char tMODE_EQUF[2][60] = {
    "The value is set to 1 when a compare-interrupt occurs",
    "Writing 1 clears the equal flag"
};
const char tMODE_OVFF[2][60] = {
    "The value is set to 1 when an overflow-interrupt occurs",
    "Writing 1 clears the equal flag"
};

TIMER::TIMER() : SubSystem(TIMER::nREGISTERS) {
}

TIMER::~TIMER() {
}

vector<string>
TIMER::unpack_Tn_MODE(const int reg) {
    vector<string> v;
    v.push_back("CLKS");
    v.push_back(tMODE_CLKS[(REGISTERS[reg]&0x3)]);
    v.push_back("GATE");
    v.push_back(tMODE_GATE[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("GATS");
    v.push_back(tMODE_GATS[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("GATM");
    v.push_back(tMODE_GATM[(REGISTERS[reg]&0x30)>>4]);
    v.push_back("ZRET");
    v.push_back(tMODE_ZRET[(REGISTERS[reg]&0x40)>>6]);
    v.push_back("CUE");
    v.push_back(tMODE_CUE[(REGISTERS[reg]&0x80)>>7]);
    v.push_back("CMPE");
    v.push_back(tMODE_CMPE[(REGISTERS[reg]&0x100)>>8]);
    v.push_back("OVFE");
    v.push_back(tMODE_OVFE[(REGISTERS[reg]&0x200)>>9]);
    v.push_back("EQUF");
    v.push_back(tMODE_EQUF[(REGISTERS[reg]&0x400)>>10]);
    v.push_back("OVFF");
    v.push_back(tMODE_OVFF[(REGISTERS[reg]&0x800)>>11]);
    return v;
}

vector<string>
TIMER::unpack_Tn_COUNT(const int reg) {
    vector<string> v;
    char val[100];
    sprintf(val, "%d", REGISTERS[reg]&0xFFFF);
    v.push_back("COUNT");
    v.push_back(val);
    return v;
}

vector<string>
TIMER::unpack_Tn_COMP(const int reg) {
    vector<string> v;
    char val[100];
    sprintf(val, "%d", REGISTERS[reg]&0xFFFF);
    v.push_back("COMP");
    v.push_back(val);
    return v;
}

vector<string>
TIMER::unpack_Tn_HOLD(const int reg) {
    vector<string> v;
    char val[100];
    sprintf(val, "%d", REGISTERS[reg]&0xFFFF);
    v.push_back("HOLD");
    v.push_back(val);
    return v;
}

// public
vector<string>
TIMER::getRegisterText(const int reg) {
    switch(reg) {
        case T0_MODE:
        case T1_MODE:
        case T2_MODE:
        case T3_MODE:
            return unpack_Tn_MODE(reg);
            break;
        case T0_COUNT:
        case T1_COUNT:
        case T2_COUNT:
        case T3_COUNT:
            return unpack_Tn_COUNT(reg);
            break;
        case T0_COMP:
        case T1_COMP:
        case T2_COMP:
        case T3_COMP:
            return unpack_Tn_COMP(reg);
            break;
        case T0_HOLD:
        case T1_HOLD:
            return unpack_Tn_HOLD(reg);
            break;
        default:
            cout << "ERROR" << endl;
            break;
    }
}

// uint32
// TIMER::readRegister(const int reg) {
//     return REGISTERS[reg];
// }

// uint32
// TIMER::writeRegister(const int reg, uint32 value) {
//     cout << reg << endl;
//     cout << TIMER_REGISTERS(reg) << endl;
//     REGISTERS[reg] = value;
//     return 0;
// }

// uint32
// TIMER::initRegisters(uint32 *data) {
//     int i;
//     for (i = 0; i < nREGISTERS ; i++) {
//         REGISTERS[i] = *(data++);
//     }
// }
