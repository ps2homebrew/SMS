#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#include "wx/string.h"

#include "gif.h"
#include "vif0.h"
#include "vif1.h"
#include "dma.h"

const string tDMA_CTRL[6] = {
    "DMA Enable", "Release signal enable",
    "Memory FIFO drain channel", "Stall Control source channel",
    "Stall control drain channel", "Release Cycle" };

const string tDMAE[2] = {
    "Disables all DMAs",
    "Enables all DMAs"
};

const string tRELE[2] = {
    "Cycle Stealing off",
    "Cycle Stealing on"
};

const string tMFD[4] = {
    "Does not use MFIFO function",
    "RESERVED",
    "VIF1 channel ( ch-1 )",
    "GIF channel ( ch-2 )"
};
const string tSTS[4] = {
    "Non-specified",
    "SIF0 channel (ch-5)",
    "fromSPR channel (ch-8)",
    "fromIPU channel (ch-3)"
};
const string tSTD[4] = {
    "Does not perform stall control",
    "VIF1 channel (ch-1)",
    "GIF channel (ch-2)",
    "SIF1 channel (ch-6)"
};
const string tRCYC[6] = {
    "8", "16", "32", "64", "128", "256"
};

// DMA_STAT
const string tDMA_STAT[25] = {
    "CIS0", "CIS1", "CIS2", "CIS3", "CIS4",
    "CIS5", "CIS6", "CIS7", "CIS8", "CIS9",
    "Stall status", "MFIFO empty status", "BUSERR status",
    "CIM0", "CIM1", "CIM2", "CIM3", "CIM4",
    "CIM5", "CIM6", "CIM7", "CIM8", "CIM9",
    "Stall mask", "MFIFO empty mask"
};

const string tCIS[2] = {
    "in progress",
    "transfer ended"
};

const string tSIS[2] = {
    "Stalled",
    "Not stalled"
};

const string tMEIS[2] = {
    "Not empty",
    "Empty"
};
const string tBEIS[2] = {
    "No error",
    "Error"
};
const string tCIM[2] = {
    "Disable",
    "Enable"
};
const string tSIM[2] = {
    "Disable",
    "Enable"
};
const string tMEIM[2] = {
    "Disable",
    "Enable"
};

// static const string tDMA_PCR[21] = {
//     "CPC0", "CPC1", "CPC2", "CPC3", "CPC4",
//     "CPC5", "CPC6", "CPC7", "CPC8", "CPC9",
//     "CDE0", "CDE1", "CDE2", "CDE3", "CDE4",
//     "CDE5", "CDE6", "CDE7", "CDE8", "CDE9",
//     "Priority Control"
// };
static const string tCPC[2] = {
    "Does not output channel status",
    "Outputs channel status"
};
static const string tCDE[2] = {
    "Disable",
    "Enable"
};
static const string tPCE[2] = {
    "CDEn bit disable",
    "CDEn bit enable"
};

// static const string tDMA_SQWC[2] = {
//     "Skip quadword counter",
//     "Transfer quadword counter"
// };

static const string tSQWC = "Size of the part not transferred(qword): ";
static const string tTQWC = "Size of the part transferred(qword): ";

// static const string *tDMA_RBOR[][] = {
//     "ADDR"
// };
// static const string *tRBOR[][] = { "Ring buffer offset address" };
// //
// // DMA_RBSR
// //
// const int DMA_RBSR = 0x10003000;
// static const string *tDMA_RBSR[][] = {
//     "RMSK"
// };
// static const string *tRMSK = "Ring buffer size mask";

// //
// // DMA_STADR
// //
// const int DMA_STADR = 0x10003000;
// static const string *tDMA_STADR = "ADDR";
// static const string *STADR = "Stall address";
// //
// // DMA_ENABLEW
// //
// // const int DMA_ENABLEHW = 0x10003000;
// // static const string *tDMA_ENABLEHW = "CPND";
static const string tENABLEW_CPND[2] = {
    "Enables all channels(restarts)",
    "Holds all channel transfer(suspends)"
};
// DMA_ENABLER
// const int DMA_ENABLEHR = 0x10003000;
static const string tENABLER_CPND[2] = {
    "All channel transfer enabled",
    "All channel transfer being held"
};

//
// Channel Registers
//

static const string tCHCR_DIR[2] = {
    "to Memory",
    "from Memory"
};
static const string tCHCR_MOD[3] = {
    "Normal", "Chain", "Interleave"
};
static const string tCHCR_ASP[3] = {
    "No address pushed by call tag",
    "1 address pushed",
    "2 addresses pushed"
};
static const string tCHCR_TTE[2] = {
    "Does not transfer DMAtag itself",
    "Transfers DMAtag"
};
static const string tCHCR_TIE[2] = {
    "Disables IRQ bit of DMATag",
    "Enables IRQ bit of DMATag"
};
static const string tCHCR_STR[2] = {
    "DMA stopped",
    "DMA running"
};
// static const string DMA_TAG[3][4] = {
//     // Maintains bit 31-16 of the DMAtag read most recently in chain mode
//     "IRQ", "ID", "PCE"
// };

// //
// // Dn_MADR
// //
// static const string *Dn_MADR[][] = {
//     "ADDR", "SPR"
// };
static const string tCHCR_ADDR = "Transfer memory address: ";
static const string tCHCR_SPR[2] = {
    "Memory address",
    "SPR address"
};
// //
// // Dn_TADR
// //
// static const string *Dn_TADR[][] = {
//     "ADDR", "SPR"
// };
// static const string *TADR_ADDR[][] = {
//     "Address of the next tag",
// };
// static const string *TADR_SPR[][] = {
//     "Memory address",
//     "SPR address"
// };
// //
// // Dn_ASR0
// //
// static const string *Dn_ASR0[][] = {
//     "ADDR",
//     "SPR"
// };
// static const string *ASR0_ADDR = "Tag address pushed by call tag";
// static const string *ASR0_SPR[][] = {
//     "aaMemory address",
//     "SPR address"
// };
// //
// // Dn_ASR1
// //
// static const string *Dn_ASR1[][] = {
//     "ADDR",
//     "SPR"
// };
// static const string *ASR1_ADDR = "Tag address pushed by call tag";
// static const string ASR1_SPR[2][20] = {
//     "Memory address",
//     "SPR address"
// };
// //
// // Dn_SADR
// //
// static const string *Dn_SADR = "ADDR";
// static const string *SADR_ADDR = "SPR address";
// //
// // Dn_QWC
// //
// static const string *Dn_QWC = "QWC";
// static const string *SADR_ADDR = "Transfer data size (in qwords) = ";


static const int numRegisters = 9;
// static const int numRegisters = 52;

/////////////////////////////// PUBLIC ///////////////////////////////////////
Dma::Dma() : SubSystem(0) {
    Init();
}

Dma::Dma(const char *filename) : SubSystem(0) {
    m_pFileIn->open(filename, ios::binary);
    Init();
}

Dma::~Dma() {
    Init();
}

void
Dma::Init(void) {
    m_id = -1;
    m_qwc = 0;
    m_channel = VIF1;
    m_pLog = Log::Instance();
    m_trace = false;
    REGISTERS = new uint32[numRegisters];
}

uint32
Dma::NumRegisters() {
    return numRegisters;
}

void
Dma::SetRegisters(uint32 *data, uint32 size) {
    uint32 i;
    for(i = 0; i < size/4; i++) {
        REGISTERS[i] = *(data++);
    }
    return;
}

bool
Dma::Eof(void) {
    return m_pFileIn->eof();
}

const vector<string>
Dma::GetRegisterText(const int reg) {
    switch(reg) {
        case DMA_CTRL:
            return UnpackCtrl(reg);
            break;
        case DMA_STAT:
            return UnpackStat(reg);
            break;
        case DMA_PCR:
            return UnpackPcr(reg);
            break;
        case DMA_SQWC:
            return UnpackSqwc(reg);
            break;
        case DMA_RBOR:
            return UnpackRbor(reg);
            break;
        case DMA_RBSR:
            return UnpackRbsr(reg);
            break;
        case DMA_STADR:
            return UnpackStadr(reg);
            break;
        case DMA_EnableR:
            return UnpackEnabler(reg);
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
            return UnpackDnCHCR(reg);
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
            return UnpackDnMADR(reg);
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
            return UnpackDnQWC(reg);
            break;
        case D0_TADR:
        case D1_TADR:
        case D2_TADR:
        case D4_TADR:
        case D6_TADR:
        case D9_TADR:
            return UnpackDnTADR(reg);
            break;
        case D0_ASR0:
        case D1_ASR0:
        case D2_ASR0:
            return UnpackDnASR0(reg);
            break;
        case D0_ASR1:
        case D1_ASR1:
        case D2_ASR1:
            return UnpackDnASR1(reg);
            break;
        case D8_SADR:
        case D9_SADR:
            return UnpackDnSADR(reg);
            break;
        default:
            vector<string> v;
            return v; 
            break;
    }    
}

void
Dma::DecodeTag(void) {
    uint64 data;
    char   raw[8];
    m_pFileIn->read(raw, 8);
    if ( m_pFileIn->peek() == EOF ) {
        // cout << "peek sees EOF" << endl;
    }
    if(m_pFileIn->eof()) {
        return;
    }
    data = (
        (((unsigned char)raw[7])<<56) +
        (((unsigned char)raw[6])<<48) +
        (((unsigned char)raw[5])<<40) +
        (((unsigned char)raw[4])<<32) +
        (((unsigned char)raw[3])<<24) +
        (((unsigned char)raw[2])<<16) +
        (((unsigned char)raw[1])<<8)  +
        raw[0]);
    m_qwc = data&0xffff;
    m_pce = (data>>26)&0x3;
    m_id = (data>>28)&0x7;
    m_irq = (data>>31)&0x1;
    m_num = m_qwc*4;
    m_addr = (data>>32)&0xffffffff;
    if ( m_channel == VIF1 || m_channel == VIF0 ) {
        m_num += 2;
    }
    if ( m_trace ) {
        m_pLog->Trace(1, wxString::Format("QWC: %d, PCE: %d, ID: %d\n", m_qwc,
                m_pce, m_id));
    }
    // TODO
    // all dma now goes automatically to vif1
    m_pVif1->Read(m_pFileIn, m_qwc);
    return;
}

const int32
Dma::Read() {
    if ( m_trace ) {
        m_pLog->Trace(0, wxString("DMA\n"));
    }
    while(!m_pFileIn->eof()) {
        DecodeTag();
    }
    return E_OK;
}

const int32
Dma::Open(const char *filename) {
    m_pFileIn = new ifstream(filename, ios::binary);
    return E_OK;
}

const int32
Dma::Close(void) {
    m_pFileIn->close();
    return E_OK;
}

void
Dma::SetGif(Gif* gif) {
    m_pGif = gif;
}

void
Dma::SetVif0(Vif0* vif) {
    m_pVif0 = vif;
}

void
Dma::SetVif1(Vif1* vif) {
    m_pVif1 = vif;
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
vector<string>
Dma::UnpackCtrl(const int reg) {
    vector<string> v;
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
Dma::UnpackStat(const int reg) {
    vector<string> v;
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
Dma::UnpackPcr(const int reg) {
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
Dma::UnpackSqwc(const int reg) {
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
Dma::UnpackRbor(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}
vector<string>
Dma::UnpackRbsr(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("RMSK");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}
vector<string>
Dma::UnpackStadr(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ADDR");
    sprintf(val, "0x%x", REGISTERS[reg]&0x7FFFFFFF);
    v.push_back(val);
    return v;
}

vector<string>
Dma::UnpackEnabler(const int reg) {
    vector<string> v;
    v.push_back("CPND");
    v.push_back(tENABLER_CPND[(REGISTERS[reg]&0x10000)>>16]);
    return v;
}

// Dma channels
vector<string>
Dma::UnpackDnCHCR(const int reg) {
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
Dma::UnpackDnMADR(const int reg) {
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
Dma::UnpackDnTADR(const int reg) {
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
Dma::UnpackDnASR0(const int reg) {
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
Dma::UnpackDnASR1(const int reg) {
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
Dma::UnpackDnSADR(const int reg) {
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
Dma::UnpackDnQWC(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("QWC");
    sprintf(val, "%d", (REGISTERS[reg]&0xFFFF));
    v.push_back(val);
    return v;
}
