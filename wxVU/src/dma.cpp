#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/stat.h>
#include "dma.h"

// DMA CTRL
struct DMA_CTRL {
    bool DMAE;
    bool RELE;
    unsigned int MFD;
    unsigned int STS;
    unsigned int STD;
    unsigned int RCYC;
};
const char tDMA_CTRL[6][50] = {
    "DMA Enable", "Release signal enable",
    "Memory FIFO drain channel", "Stall Control source channel",
    "Stall control drain channel", "Release Cycle" };

const char tDMAE[2][18] = {
    "Disables all DMAs",
    "Enables all DMAs"
};

const char tRELE[2][20] = {
    "Cycle Stealing off",
    "Cycle Stealing on"
};

const char tMFD[4][30] = {
    "Does not use MFIFO function",
    "RESERVED",
    "VIF1 channel ( ch-1 )",
    "GIF channel ( ch-2 )"
};
const char tSTS[4][30] = {
    "Non-specified",
    "SIF0 channel (ch-5)",
    "fromSPR channel (ch-8)",
    "fromIPU channel (ch-3)"
};
const char tSTD[4][40] = {
    "Does not perform stall control",
    "VIF1 channel (ch-1)",
    "GIF channel (ch-2)",
    "SIF1 channel (ch-6)"
};
const char tRCYC[6][4] = {
    "8", "16", "32", "64", "128", "256"
};

// DMA_STAT
const char tDMA_STAT[25][20] = {
    "CIS0", "CIS1", "CIS2", "CIS3", "CIS4",
    "CIS5", "CIS6", "CIS7", "CIS8", "CIS9",
    "Stall status", "MFIFO empty status", "BUSERR status",
    "CIM0", "CIM1", "CIM2", "CIM3", "CIM4",
    "CIM5", "CIM6", "CIM7", "CIM8", "CIM9",
    "Stall mask", "MFIFO empty mask"
};

const char tCIS[2][20] = {
    "in progress",
    "transfer ended"
};

const char tSIS[2][20] = {
    "Stalled",
    "Not stalled"
};

const char tMEIS[2][10] = {
    "Not empty",
    "Empty"
};
const char tBEIS[2][10] = {
    "No error",
    "Error"
};
const char tCIM[2][10] = {
    "Disable",
    "Enable"
};
const char tSIM[2][10] = {
    "Disable",
    "Enable"
};
const char tMEIM[2][10] = {
    "Disable",
    "Enable"
};

// //
// // DMA_PCR
// //

// const int DMA_PCR = 0x10003000;

// const char *tDMA_PCR[][] = {
//     "CPC0", "CPC1", "CPC2", "CPC3", "CPC4",
//     "CPC5", "CPC6", "CPC7", "CPC8", "CPC9",
//     "CDE0", "CDE1", "CDE2", "CDE3", "CDE4",
//     "CDE5", "CDE6", "CDE7", "CDE8", "CDE9",
//     "Priority Control"
// };
const char tCPC[2][40] = {
    "Does not output channel status",
    "Outputs channel status"
};
const char tCDE[2][10] = {
    "Disable",
    "Enable"
};
const char tPCE[2][30] = {
    "CDEn bit disable",
    "CDEn bit enable"
};

// //
// // DMA_SQWC
// //
// const int DMA_SQWC = 0x10003000;

// const char *tDMA_SQWC[][] = {
//     "Skip quadword counter",
//     "Transfer quadword counter"
// };

const char *tSQWC = "Size of the part not transferred(qword): ";
const char *tTQWC = "Size of the part transferred(qword): ";

// //
// // DMA_RBOR
// //
// const int DMA_RBOR = 0x10003000;

// const char *tDMA_RBOR[][] = {
//     "ADDR"
// };
// const char *tRBOR[][] = { "Ring buffer offset address" };
// //
// // DMA_RBSR
// //
// const int DMA_RBSR = 0x10003000;
// const char *tDMA_RBSR[][] = {
//     "RMSK"
// };
// const char *tRMSK = "Ring buffer size mask";

// //
// // DMA_STADR
// //
// const int DMA_STADR = 0x10003000;
// const char *tDMA_STADR = "ADDR";
// const char *STADR = "Stall address";
// //
// // DMA_ENABLEW
// //
// // const int DMA_ENABLEHW = 0x10003000;
// // const char *tDMA_ENABLEHW = "CPND";
const char tENABLEW_CPND[2][40] = {
    "Enables all channels(restarts)",
    "Holds all channel transfer(suspends)"
};
//
// DMA_ENABLER
//
// const int DMA_ENABLEHR = 0x10003000;
const char tENABLER_CPND[2][40] = {
    "All channel transfer enabled",
    "All channel transfer being held"
};

// //
// // Channel Registers
// //

// //
// // Dn_CHCR
// //

const char tCHCR_DIR[2][20] = {
    "to Memory",
    "from Memory"
};
const char tCHCR_MOD[3][15] = {
    "Normal", "Chain", "Interleave"
};
const char tCHCR_ASP[3][40] = {
    "No address pushed by call tag",
    "1 address pushed",
    "2 addresses pushed"
};
const char tCHCR_TTE[2][40] = {
    "Does not transfer DMAtag itself",
    "Transfers DMAtag"
};
const char tCHCR_TIE[2][30] = {
    "Disables IRQ bit of DMATag",
    "Enables IRQ bit of DMATag"
};
const char tCHCR_STR[2][12] = {
    "DMA stopped",
    "DMA running"
};
// const char DMA_TAG[3][4] = {
//     // Maintains bit 31-16 of the DMAtag read most recently in chain mode
//     "IRQ", "ID", "PCE"
// };

// //
// // Dn_MADR
// //
// const char *Dn_MADR[][] = {
//     "ADDR", "SPR"
// };
const char *tCHCR_ADDR = "Transfer memory address: ";
const char tCHCR_SPR[2][20] = {
    "Memory address",
    "SPR address"
};
// //
// // Dn_TADR
// //
// const char *Dn_TADR[][] = {
//     "ADDR", "SPR"
// };
// const char *TADR_ADDR[][] = {
//     "Address of the next tag",
// };
// const char *TADR_SPR[][] = {
//     "Memory address",
//     "SPR address"
// };
// //
// // Dn_ASR0
// //
// const char *Dn_ASR0[][] = {
//     "ADDR",
//     "SPR"
// };
// const char *ASR0_ADDR = "Tag address pushed by call tag";
// const char *ASR0_SPR[][] = {
//     "aaMemory address",
//     "SPR address"
// };
// //
// // Dn_ASR1
// //
// const char *Dn_ASR1[][] = {
//     "ADDR",
//     "SPR"
// };
// const char *ASR1_ADDR = "Tag address pushed by call tag";
// const char ASR1_SPR[2][20] = {
//     "Memory address",
//     "SPR address"
// };
// //
// // Dn_SADR
// //
// const char *Dn_SADR = "ADDR";
// const char *SADR_ADDR = "SPR address";
// //
// // Dn_QWC
// //
// const char *Dn_QWC = "QWC";
// const char *SADR_ADDR = "Transfer data size (in qwords) = ";

DMA::DMA() {
    REGISTERS = new uint32[nREGISTERS];
}

DMA::~DMA() {
}

vector<string>
DMA::unpack_ctrl(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("DMAE");
    v.push_back(tDMAE[(REGISTERS[reg]&0x1)]);
    v.push_back("RELE");
    v.push_back(tRELE[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("MFD");
    v.push_back(tMFD[(REGISTERS[reg]&0xC)>>2]);
    v.push_back("STS");
    v.push_back(tSTS[(REGISTERS[reg]&0x30)>>4]);
    v.push_back("STD");
    v.push_back(tSTD[(REGISTERS[reg]&0xC0)>>6]);
    v.push_back("RCYC");
    v.push_back(tRCYC[(REGISTERS[reg]&0x700)>>8]);
    return v;
}

vector<string>
DMA::unpack_stat(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("CIS0");
    v.push_back(tCIS[(REGISTERS[reg]&0x1)]);
    v.push_back("CIS1");
    v.push_back(tCIS[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("CIS2");
    v.push_back(tCIS[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("CIS3");
    v.push_back(tCIS[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("CIS4");
    v.push_back(tCIS[(REGISTERS[reg]&0x10)>>4]);
    v.push_back("CIS5");
    v.push_back(tCIS[(REGISTERS[reg]&0x20)>>5]);
    v.push_back("CIS6");
    v.push_back(tCIS[(REGISTERS[reg]&0x40)>>6]);
    v.push_back("CIS7");
    v.push_back(tCIS[(REGISTERS[reg]&0x80)>>7]);
    v.push_back("CIS8");
    v.push_back(tCIS[(REGISTERS[reg]&0x100)>>8]);
    v.push_back("CIS9");
    v.push_back(tCIS[(REGISTERS[reg]&0x200)>>9]);
    //
    v.push_back("SIS");
    v.push_back(tSIS[(REGISTERS[reg]&0x2000)>>13]);
    v.push_back("MEIS");
    v.push_back(tMEIS[(REGISTERS[reg]&0x4000)>>14]);
    v.push_back("BEIS");
    v.push_back(tBEIS[(REGISTERS[reg]&0x8000)>>15]);
    //
    v.push_back("CIM0");
    v.push_back(tCIM[(REGISTERS[reg]&0x10000)>>16]);
    v.push_back("CIM1");
    v.push_back(tCIM[(REGISTERS[reg]&0x20000)>>17]);
    v.push_back("CIM2");
    v.push_back(tCIM[(REGISTERS[reg]&0x40000)>>18]);
    v.push_back("CIM3");
    v.push_back(tCIM[(REGISTERS[reg]&0x80000)>>19]);
    v.push_back("CIM4");
    v.push_back(tCIM[(REGISTERS[reg]&0x100000)>>20]);
    v.push_back("CIM5");
    v.push_back(tCIM[(REGISTERS[reg]&0x200000)>>21]);
    v.push_back("CIM6");
    v.push_back(tCIM[(REGISTERS[reg]&0x400000)>>22]);
    v.push_back("CIM7");
    v.push_back(tCIM[(REGISTERS[reg]&0x800000)>>23]);
    v.push_back("CIM8");
    v.push_back(tCIM[(REGISTERS[reg]&0x1000000)>>24]);
    v.push_back("CIM9");
    v.push_back(tCIM[(REGISTERS[reg]&0x2000000)>>25]);
    return v;
}
vector<string>
DMA::unpack_pcr(const int reg) {
    vector<string> v;
    v.push_back("CPC0");
    v.push_back(tCPC[(REGISTERS[reg]&0x1)]);
    v.push_back("CPC1");
    v.push_back(tCPC[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("CPC2");
    v.push_back(tCPC[(REGISTERS[reg]&0x4)>>2]);
    v.push_back("CPC3");
    v.push_back(tCPC[(REGISTERS[reg]&0x8)>>3]);
    v.push_back("CPC4");
    v.push_back(tCPC[(REGISTERS[reg]&0x10)>>4]);
    v.push_back("CPC5");
    v.push_back(tCPC[(REGISTERS[reg]&0x20)>>5]);
    v.push_back("CPC6");
    v.push_back(tCPC[(REGISTERS[reg]&0x40)>>6]);
    v.push_back("CPC7");
    v.push_back(tCPC[(REGISTERS[reg]&0x80)>>7]);
    v.push_back("CPC8");
    v.push_back(tCPC[(REGISTERS[reg]&0x100)>>8]);
    v.push_back("CPC9");
    v.push_back(tCPC[(REGISTERS[reg]&0x200)>>9]);
    //
    v.push_back("CDE0");
    v.push_back(tCDE[(REGISTERS[reg]&0x10000)>>16]);
    v.push_back("CDE1");
    v.push_back(tCDE[(REGISTERS[reg]&0x20000)>>17]);
    v.push_back("CDE2");
    v.push_back(tCDE[(REGISTERS[reg]&0x40000)>>18]);
    v.push_back("CDE3");
    v.push_back(tCDE[(REGISTERS[reg]&0x80000)>>19]);
    v.push_back("CDE4");
    v.push_back(tCDE[(REGISTERS[reg]&0x100000)>>20]);
    v.push_back("CDE5");
    v.push_back(tCDE[(REGISTERS[reg]&0x200000)>>21]);
    v.push_back("CDE6");
    v.push_back(tCDE[(REGISTERS[reg]&0x400000)>>22]);
    v.push_back("CDE7");
    v.push_back(tCDE[(REGISTERS[reg]&0x800000)>>23]);
    v.push_back("CDE8");
    v.push_back(tCDE[(REGISTERS[reg]&0x1000000)>>24]);
    v.push_back("CDE9");
    v.push_back(tCDE[(REGISTERS[reg]&0x2000000)>>25]);

    v.push_back("PCE");
    v.push_back(tPCE[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_sqwc(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("SQWC");
    sprintf(val, "%d", REGISTERS[reg]&0xFF);
    v.push_back((string)tSQWC + val);
    v.push_back("TQWC");
    sprintf(val, "%d", (REGISTERS[reg]&0xFF0000)>>16);
    v.push_back((string)tTQWC + val);
    return v;
}
vector<string>
DMA::unpack_rbor(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}
vector<string>
DMA::unpack_rbsr(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("RMSK");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}
vector<string>
DMA::unpack_stadr(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}

vector<string>
DMA::unpack_enabler(const int reg) {
    vector<string> v;
    v.push_back("CPND");
    v.push_back(tENABLER_CPND[(REGISTERS[reg]&0x10000)>>16]);
    return v;
}

// Dma channels
vector<string>
DMA::unpack_Dn_CHCR(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("DIR");
    v.push_back(tCHCR_DIR[REGISTERS[reg]&0x1]);
    v.push_back("MOD");
    v.push_back(tCHCR_MOD[(REGISTERS[reg]&0xC)>>2]);
    v.push_back("ASP");
    v.push_back(tCHCR_ASP[(REGISTERS[reg]&0x30)>4]);
    v.push_back("TTE");
    v.push_back(tCHCR_TTE[(REGISTERS[reg]&0x40)>6]);
    v.push_back("TIE");
    v.push_back(tCHCR_TIE[(REGISTERS[reg]&0x80)>7]);
    v.push_back("STR");
    v.push_back(tCHCR_STR[(REGISTERS[reg]&0x100)>8]);
    v.push_back("TAG");
    sprintf(val, "0x%x", (REGISTERS[reg]&0xFFFF0000)>>16);
    v.push_back(val);
    return v;
}
vector<string>
DMA::unpack_Dn_MADR(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x7FFFFFFF));
    v.push_back(val);
    v.push_back("SPR");
    v.push_back(tCHCR_SPR[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_Dn_TADR(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x7FFFFFFF));
    v.push_back(val);
    v.push_back("SPR");
    v.push_back(tCHCR_SPR[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_Dn_ASR0(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x7FFFFFFF));
    v.push_back(val);
    v.push_back("SPR");
    v.push_back(tCHCR_SPR[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_Dn_ASR1(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x7FFFFFFF));
    v.push_back(val);
    v.push_back("SPR");
    v.push_back(tCHCR_SPR[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_Dn_SADR(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x7FFFFFFF));
    v.push_back(val);
    v.push_back("SPR");
    v.push_back(tCHCR_SPR[(REGISTERS[reg]&0x80000000)>>31]);
    return v;
}
vector<string>
DMA::unpack_Dn_QWC(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("QWC");
    sprintf(val, "%d", (REGISTERS[reg]&0xFFFF));
    v.push_back(val);
    return v;
}

vector<string>
DMA::getRegisterText(const int reg) {
    switch(reg) {
        case DMA_CTRL:
            return unpack_ctrl(reg);
            break;
        case DMA_STAT:
            return unpack_stat(reg);
            break;
        case DMA_PCR:
            return unpack_pcr(reg);
            break;
        case DMA_SQWC:
            return unpack_sqwc(reg);
            break;
        case DMA_RBOR:
            return unpack_rbor(reg);
            break;
        case DMA_RBSR:
            return unpack_rbsr(reg);
            break;
        case DMA_STADR:
            return unpack_stadr(reg);
            break;
        case DMA_EnableR:
            return unpack_enabler(reg);
            break;
        case D0_CHCR:
        case D1_CHCR:
        case D2_CHCR:
        case D3_CHCR:
        case D4_CHCR:
        case D5_CHCR:
        case D6_CHCR:
        case D7_CHCR:
        case D8_CHCR:
        case D9_CHCR:
            return unpack_Dn_CHCR(reg);
            break;
        case D0_MADR:
        case D1_MADR:
        case D2_MADR:
        case D3_MADR:
        case D4_MADR:
        case D5_MADR:
        case D6_MADR:
        case D7_MADR:
        case D8_MADR:
        case D9_MADR:
            return unpack_Dn_MADR(reg);
            break;
        case D0_QWC:
        case D1_QWC:
        case D2_QWC:
        case D3_QWC:
        case D4_QWC:
        case D5_QWC:
        case D6_QWC:
        case D7_QWC:
        case D8_QWC:
        case D9_QWC:
            return unpack_Dn_QWC(reg);
            break;
        case D0_TADR:
        case D1_TADR:
        case D2_TADR:
        case D4_TADR:
        case D6_TADR:
        case D9_TADR:
            return unpack_Dn_TADR(reg);
            break;
        case D0_ASR0:
        case D1_ASR0:
        case D2_ASR0:
            return unpack_Dn_ASR0(reg);
            break;
        case D0_ASR1:
        case D1_ASR1:
        case D2_ASR1:
            return unpack_Dn_ASR1(reg);
            break;
        case D8_SADR:
        case D9_SADR:
            return unpack_Dn_SADR(reg);
            break;
        default:
            cout << "ERROR" << endl;
            break;
    }    
}

uint32
DMA::writeRegister(const int reg, uint32 value) {

    return 0;
}

uint64
DMA::readRegister(const int reg) {
    return 0;
}

uint32
DMA::initRegisters(uint32 *data) {
    int i;
    for (i = 0; i < nREGISTERS; i++) {
        REGISTERS[i] = *(data++);
    }
}
