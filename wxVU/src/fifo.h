#ifndef _FIFO
#define _FIFO
#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;
enum FIFO_REGISTERS {
    VIF1_FIFO, IPU_out_FIFO
};
static const char *tFIFO_REGISTERS[] = {
	"VIF1_FIFO", "IPU_out_FIFO"
};

class FIFO : public SubSystem {
public:
    FIFO();
	~FIFO();
    static const int    nREGISTERS = 2;
    vector<string>      getRegisterText(const int reg);
private:
    vector<string>      unpack_VIF1_FIFO(const int reg);
    vector<string>      unpack_IPU_out_FIFO(const int reg);
};
#endif
