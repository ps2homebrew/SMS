#ifndef __VIF0__
#define __VIF0__
#include <vector>
#include "datatypes.h"
#include "vif.h"

using namespace std;
enum VIF0_REGISTERS {
    VIF0_STAT, VIF0_ERR, VIF0_MARK,
    VIF0_CYCLE, VIF0_MODE, VIF0_NUM, VIF0_MASK,
    VIF0_CODE, VIF0_ITOPS, 
    VIF0_ITOP,
    VIF0_R0, VIF0_R1, VIF0_R2, VIF0_R3,
    VIF0_C0, VIF0_C1, VIF0_C2, VIF0_C3
};

static const char *tVIF0_REGISTERS[] = {
    "VIF0_STAT", "VIF0_ERR", "VIF0_MARK",
    "VIF0_CYCLE", "VIF0_MODE", "VIF0_NUM", "VIF0_MASK",
    "VIF0_CODE", "VIF0_ITOPS",
    "VIF0_ITOP",
    "VIF0_R0", "VIF0_R1", "VIF0_R2", "VIF0_R3",
    "VIF0_C0", "VIF0_C1", "VIF0_C2", "VIF0_C3"
};

class Vif0 : public Vif {
public:
    Vif0();
    ~Vif0();
    static const int    nREGISTERS;
    const vector<string>    GetRegisterText(const int reg);
private:
    vector<string>      unpackStat(const int reg);
    vector<string>      unpackFbrst(const int reg);
};
#endif
