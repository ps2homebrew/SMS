#ifndef _SIF
#define _SIF

#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;
enum SIF_REGISTERS {
    SB_SMFLAG
};

static const char *tSIF_REGISTERS[] = {
	"SB_SMFLG"
};

class SIF : public SubSystem {
public:
    SIF();
    vector<string>  getRegisterText(const int reg);
    static const int    nREGISTERS;

private:
    vector<string>  unpack_SB_SMFLG(void);
};
const int SIFnREGISTERS = 1;
#endif
