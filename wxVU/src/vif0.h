#ifndef _VIF0
#define _VIF0
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

class VIF0 : public VIF {
public:
    VIF0();
    ~VIF0();
    static const int    nREGISTERS = 18;
    vector<string>      getRegisterText(const int reg);
private:
    vector<string>      unpack_VIF0_STAT(const int reg);
    vector<string>      unpack_VIF0_FBRST(const int reg);
};
#endif
