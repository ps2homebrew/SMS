#ifndef _INTC
#define _INTC
#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;
enum INTC_REGISTERS {
    I_STAT, I_MASK
};
static const char *tINTC_REGISTERS[] = {
	"I_STAT", "I_MASK"
};

class INTC : public SubSystem {
public:
    INTC();
    ~INTC();
    static const int    nREGISTERS;
    const vector<string>    GetRegisterText(const int reg);
private:
    vector<string>      UnpackIStat(const int reg);
    vector<string>      UnpackIMask(const int reg);
};
#endif
