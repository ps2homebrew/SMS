#ifndef _DMA
#define _DMA
#include "datatypes.h"

using namespace std;

enum DMA_REGISTERS {
    DMA_CTRL, DMA_STAT, DMA_PCR, DMA_SQWC,
    DMA_RBOR, DMA_RBSR, DMA_STADR, DMA_EnableR,
    D0_CHCR, D0_MADR, D0_QWC, D0_TADR, D0_ASR0, D0_ASR1,
    D1_CHCR, D1_MADR, D1_QWC, D1_TADR, D1_ASR0, D1_ASR1,
    D2_CHCR, D2_MADR, D2_QWC, D2_TADR, D2_ASR0, D2_ASR1,
    D3_CHCR, D3_MADR, D3_QWC,
    D4_CHCR, D4_MADR, D4_QWC, D4_TADR,
    D5_CHCR, D5_MADR, D5_QWC,
    D6_CHCR, D6_MADR, D6_QWC, D6_TADR,
    D7_CHCR, D7_MADR, D7_QWC,
    D8_CHCR, D8_MADR, D8_QWC, D8_SADR,
    D9_CHCR, D9_MADR, D9_QWC, D9_TADR, D9_SADR
};

enum DMA_CHANNELS {
    VIF0, VIF1, GIF1, IPU, IPU2, SIF0, SIF1, SIF2, SPR, SPR2
};

static const char *tDMA_REGISTERS[] = {
    "DMA Ctrl", "DMA Stat", "DMA PCR",
    "DMA SQWC", "DMA RBOR", "DMA RBSR",
    "DMA STADR", "DMA EnableR",
    // VIF 0
    // VIF 1
    // GIF
    // IPU
    // IPU 2
    // SIF 0
    // SIF 1
    // SIF 2
    // SPR
    // SPR 2
};

class DMA {
public:
    DMA();
    DMA(const char *filename);
    DMA(const char *filename, int numregs);
    ~DMA();
    vector<string>  getRegisterText(const int reg);
    uint64          readRegister(const int reg);
    uint32          writeRegister(const int reg, uint32 value);
    uint32          initRegisters(uint32 *data);

    bool            eof(void);
    bool            valid(void);
    uint32          read(void);
    void            write(uint32);
    void            decode_tag(void);
    // void            getDmaTag(void);
private:
    vector<string>  unpack_ctrl(const int reg);
    vector<string>  unpack_stat(const int reg);
    vector<string>  unpack_pcr(const int reg);
    vector<string>  unpack_sqwc(const int reg);
    vector<string>  unpack_rbor(const int reg);
    vector<string>  unpack_rbsr(const int reg);
    vector<string>  unpack_stadr(const int reg);
    vector<string>  unpack_enablew(const int reg);
    vector<string>  unpack_enabler(const int reg);
    vector<string>  unpack_Dn_CHCR(const int reg);
    vector<string>  unpack_Dn_MADR(const int reg);
    vector<string>  unpack_Dn_TADR(const int reg);
    vector<string>  unpack_Dn_ASR0(const int reg);
    vector<string>  unpack_Dn_ASR1(const int reg);
    vector<string>  unpack_Dn_SADR(const int reg);
    vector<string>  unpack_Dn_QWC(const int reg);

    uint32          *REGISTERS;
    ifstream        _fin;
    int16           _id;
    uint16          _qwc;
    uint16          _pce;
    uint16          _irq;
    uint16          _addr;
    uint16          _spr;
    uint32          _num;
    uint32          _channel;
};
const int DMAnREGISTERS = 52;
#endif
