#ifndef _IPU
#define _IPU
#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;
enum IPU_REGISTERS {
    IPU_CMD, IPU_TOP, IPU_CTRL, IPU_BP
};
static const char *tIPU_REGISTERS[] = {
	"IPU_CMD", "IPU_TOP", "IPU_CTRL", "IPU_BP"
};

class IPU : public SubSystem {
public:
    IPU();
    ~IPU();
    static const int    nREGISTERS;
    vector<string>      getRegisterText(const int reg);
private:
    vector<string>      unpack_IPU_CMD(const int reg);
    vector<string>      unpack_IPU_TOP(const int reg);
    vector<string>      unpack_IPU_CTRL(const int reg);
    vector<string>      unpack_IPU_BP(const int reg);
};
#endif
