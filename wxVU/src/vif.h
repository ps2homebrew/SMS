#ifndef __VIF__
#define __VIF__

#include <vector>
#include <string>
#include <fstream>
#include "datatypes.h"
#include "sub.h"

class Parser;
class Vu;

int limit(int, int);

// VU CMD
const uint32 VIF_NOP       = 0x0;
const uint32 VIF_STCYCL    = 0x1;
const uint32 VIF_OFFSET    = 0x2;
const uint32 VIF_BASE      = 0x3;
const uint32 VIF_ITOP      = 0x4;
const uint32 VIF_STMOD     = 0x5;
const uint32 VIF_MSKPATH3  = 0x6;
const uint32 VIF_MARK      = 0x7;
const uint32 VIF_FLUSHE    = 0x10;
const uint32 VIF_FLUSH     = 0x11;
const uint32 VIF_FLUSHA    = 0x13;
const uint32 VIF_MSCAL     = 0x14;
const uint32 VIF_MSCNT     = 0x17;
const uint32 VIF_MSCALF    = 0x15;
const uint32 VIF_STMASK    = 0x20;
const uint32 VIF_STROW     = 0x30;
const uint32 VIF_STCOL     = 0x31;
const uint32 VIF_MPG       = 0x4A;
const uint32 VIF_DIRECT    = 0x50;
const uint32 VIF_DIRECTHL  = 0x51;
const uint32 VIF_UNPACK    = 0x60;
const uint32 CMD_IRQ       = 0x80;
;
// Values for UNPACK;
const uint32 UNPACK_S32    = 0x0;
const uint32 UNPACK_S16    = 0x1;
const uint32 UNPACK_S8     = 0x2;
const uint32 UNPACK_V232   = 0x4;
const uint32 UNPACK_V216   = 0x5;
const uint32 UNPACK_V28    = 0x6;
const uint32 UNPACK_V332   = 0x8;
const uint32 UNPACK_V316   = 0x9;
const uint32 UNPACK_V38    = 0xA;
const uint32 UNPACK_V432   = 0xC;
const uint32 UNPACK_V416   = 0xD;
const uint32 UNPACK_V48    = 0xE;
const uint32 UNPACK_V45    = 0xF;

const uint32 MODE_DIRECT   = 0x0;
const uint32 MODE_ADD      = 0x1;
const uint32 MODE_ADDROW   = 0x2;

class Vif : public SubSystem {
public:
    Vif();
    Vif(int);
    Vif(const char *filename, Parser* pParser, Vu* pVu);
    Vif(const char *filename, int numregs);
    ~Vif();
    
    const int32             Open(const char* filename);
    const int32             Close(void);

    virtual const int32     Read(void) = 0;
    virtual const int32     Read(ifstream* fin, const uint16 numQuad) = 0;
    virtual const vector<string>    GetRegisterText(const int reg) = 0;

protected:
    vector<string>      UnpackItops(const int reg);
    vector<string>      UnpackItop(const int reg);
    vector<string>      UnpackR(const int reg);
    vector<string>      UnpackC(const int reg);
    vector<string>      UnpackCycle(const int reg);
    vector<string>      UnpackMode(const int reg);
    vector<string>      UnpackMask(const int reg);
    vector<string>      UnpackErr(const int reg);
    vector<string>      UnpackMark(const int reg);
    vector<string>      UnpackNum(const int reg);
    vector<string>      UnpackCode(const int reg);
    uint32          m_vifCmdNum;
    uint32          m_vifCmd;
    uint32          m_memIndex;
    ifstream*       m_fin;
    uint32          m_unpack;
    uint32          _WL;
    uint32          _CL;
    uint32          m_length;
    uint32          m_fileLength;
    uint32          m_codeIndex;
    uint32          _flg;
    uint32          _addr;
    uint32          _usn;
    uint32          vpu;
    bool            m_interrupt;
    bool            _mask;
    bool            m_stopped;
// private:
    // vif command functions
    virtual const int32 DecodeCmd(void) = 0;
    const int32     CmdNop(void);
    const int32     CmdStcycl(const int32& data);
    const int32     CmdItop(const int32& data);
    const int32     CmdStmod(const int32& data);
    const int32     CmdMark(const int32& data);
    const int32     CmdFlushE(void);
    const int32     CmdMscal(const int32& data);
    const int32     CmdMscnt(void);
    const int32     CmdMscalf(const int32& data);
    const int32     CmdStmask(void);
    const int32     CmdStrow(void);
    const int32     CmdStcol(void);
    const int32     CmdMpg(const int32& data);
    const int32     CmdUnpack(void);
    void            writeRegister(void);
    void            Init(void);

    // VIF Registers
    uint32   rVIF1_R0;
    uint32   rVIF1_R1;
    uint32   rVIF1_R2;
    uint32   rVIF1_R3;
    uint32   rVIF1_C0;
    uint32   rVIF1_C1;
    uint32   rVIF1_C2;
    uint32   rVIF1_C3;
    uint32   rVIF1_CYCLE;
    uint32   rVIF1_MASK;
    uint32   rVIF1_MODE;
    uint32   rVIF1_ITOP;
    uint32   rVIF1_ITOPS;
    uint32   rVIF1_BASE;
    uint32   rVIF1_OFST;
    uint32   rVIF1_TOP;
    uint32   rVIF1_TOPS;
    uint32   rVIF1_MARK;
    uint32   rVIF1_NUM;
    uint32   rVIF1_CODE;
    uint32   rVIF1_STAT;
    uint32   rVIF1_FBRST;
    uint32   rVIF1_ERR;

    uint32   rVIF0_R0;
    uint32   rVIF0_R1;
    uint32   rVIF0_R2;
    uint32   rVIF0_R3;
    uint32   rVIF0_C0;
    uint32   rVIF0_C1;
    uint32   rVIF0_C2;
    uint32   rVIF0_C3;
    uint32   rVIF0_CYCLE;
    uint32   rVIF0_MASK;
    uint32   rVIF0_MODE;
    uint32   rVIF0_ITOP;
    uint32   rVIF0_ITOPS;
    uint32   rVIF0_MARK;
    uint32   rVIF0_NUM;
    uint32   rVIF0_CODE;

    // helper vars
    uint32          nREGISTERS;
    Parser*         m_pParser;
    Vu*             m_pVu;
};
#endif
