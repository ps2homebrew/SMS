#ifndef _VIF1
#define _VIF1
#include <vector>
#include "datatypes.h"
#include "vif.h"

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

class VIF1 : public VIF {
public:
    VIF1();
    ~VIF1();
    static const int    nREGISTERS;
    vector<string>      getRegisterText(const int reg);
private:
    vector<string>      unpack_VIF1_STAT(const int reg);
    vector<string>      unpack_VIF1_TOPS(const int reg);
    vector<string>      unpack_VIF1_TOP(const int reg);
    vector<string>      unpack_VIF1_BASE(const int reg);
    vector<string>      unpack_VIF1_OFST(const int reg);
};
const int VIF1::nREGISTERS = 22;
#endif
