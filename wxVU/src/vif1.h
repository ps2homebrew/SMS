#ifndef __VIF1__
#define __VIF1__

#include <vector>
#include "datatypes.h"

class Vif;

using namespace std;
enum VIF1_REGISTERS {
    VIF1_STAT, VIF1_ERR, VIF1_MARK,
    VIF1_CYCLE, VIF1_MODE, VIF1_NUM, VIF1_MASK,
    VIF1_CODE, VIF1_ITOPS, VIF1_BASE, VIF1_OFST,
    VIF1_TOPS, VIF1_ITOP, VIF1_TOP,
    VIF1_R0, VIF1_R1, VIF1_R2, VIF1_R3,
    VIF1_C0, VIF1_C1, VIF1_C2, VIF1_C3
};
static const char *tVIF1_REGISTERS[] = {
    "VIF1_STAT", "VIF1_ERR", "VIF1_MARK",
    "VIF1_CYCLE", "VIF1_MODE", "VIF1_NUM", "VIF1_MASK",
    "VIF1_CODE", "VIF1_ITOPS", "VIF1_BASE", "VIF1_OFST",
    "VIF1_TOPS", "VIF1_ITOP", "VIF1_TOP",
    "VIF1_R0", "VIF1_R1", "VIF1_R2", "VIF1_R3",
    "VIF1_C0", "VIF1_C1", "VIF1_C2", "VIF1_C3"
};

class Vif1 : public Vif {
public:
    Vif1();
    Vif1(Parser* parser, Vu* vu);
    ~Vif1();
    const int32         Read();
    const int32         Read(ifstream* fin, const uint16 numQuad);
    static const int    nREGISTERS;
    const vector<string>    GetRegisterText(const int reg);
private:
    const int32         DecodeCmd(void);
    const int32         CmdOffset(const int32& data);
    const int32         CmdBase(const int32& data);
    const int32         CmdMaskpath3(const int32& data);
    const int32         CmdFlush(void);
    const int32         CmdFlushA(void);
    const int32         CmdDirect(const int32& data);
    const int32         CmdDirectHl(const int32& data);
    vector<string>      UnpackStat(const int reg);
    vector<string>      UnpackTops(const int reg);
    vector<string>      UnpackTop(const int reg);
    vector<string>      UnpackBase(const int reg);
    vector<string>      UnpackOfst(const int reg);

    bool                m_maskPath3;
};
#endif
