#ifndef _DMA
#define _DMA

#include <vector>
#include <string>
#include "datatypes.h"
#include "sub.h"

class Gif;
class Vif0;
class Vif1;
class ifstream;

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

class Dma : public SubSystem {
public:
    Dma();
    Dma(const char *filename);
    Dma(const char *filename, int numregs);
    ~Dma();
    void            Init(void);
    const vector<string>    GetRegisterText(const int reg);
    uint64          ReadRegister(const int reg);
    void            SetRegisters(uint32 *data, uint32 size);
    uint32          NumRegisters(void);

    bool            Eof(void);
    bool            Valid(void);
    const int32     Read(void);
    const int32     Open(const char *filename);
    const int32     Close(void);
    void            Write(uint32);
    void            DecodeTag(void);
    void            SetGif(Gif *gif);
    void            SetVif0(Vif0 *vif0);
    void            SetVif1(Vif1 *vif1);
private:
    vector<string>  UnpackCtrl(const int reg);
    vector<string>  UnpackStat(const int reg);
    vector<string>  UnpackPcr(const int reg);
    vector<string>  UnpackSqwc(const int reg);
    vector<string>  UnpackRbor(const int reg);
    vector<string>  UnpackRbsr(const int reg);
    vector<string>  UnpackStadr(const int reg);
    vector<string>  UnpackEnablew(const int reg);
    vector<string>  UnpackEnabler(const int reg);
    vector<string>  UnpackDnCHCR(const int reg);
    vector<string>  UnpackDnMADR(const int reg);
    vector<string>  UnpackDnTADR(const int reg);
    vector<string>  UnpackDnASR0(const int reg);
    vector<string>  UnpackDnASR1(const int reg);
    vector<string>  UnpackDnSADR(const int reg);
    vector<string>  UnpackDnQWC(const int reg);

    uint32          *REGISTERS;
    ifstream*       m_pFileIn;
    int16           m_id;
    uint16          m_qwc;
    uint16          m_pce;
    uint16          m_irq;
    uint16          m_addr;
    uint16          m_spr;
    uint32          m_num;
    uint32          m_channel;

    // subsystems
    Gif*            m_pGif;
    Vif0*           m_pVif0;
    Vif1*           m_pVif1;
    Log*            m_pLog;
};
#endif
