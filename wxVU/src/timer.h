#ifndef _TIMER
#define _TIMER
#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;

enum TIMER_REGISTERS {
    T0_MODE, T0_COUNT, T0_COMP, T0_HOLD,
    T1_MODE, T1_COUNT, T1_COMP, T1_HOLD,
    T2_MODE, T2_COUNT, T2_COMP,
    T3_MODE, T3_COUNT, T3_COMP 
};
static const char *tTIMER_REGISTERS[] = {
	"T0_MODE", "T0_COUNT", "T0_COMP", "T0_HOLD",
	"T1_MODE", "T1_COUNT", "T1_COMP", "T1_HOLD",
	"T2_MODE", "T2_COUNT", "T2_COMP",
	"T3_MODE", "T3_COUNT", "T3_COMP"
};

class TIMER : public SubSystem {
public:
    TIMER();
    ~TIMER();
    static const int    nREGISTERS = 14;
    vector<string>  getRegisterText(const int reg);
private:
    enum ENUM_REGISTERS {
        T0_MODE, T0_COUNT, T0_COMP, T0_HOLD,
        T1_MODE, T1_COUNT, T1_COMP, T1_HOLD,
        T2_MODE, T2_COUNT, T2_COMP,
        T3_MODE, T3_COUNT, T3_COMP 
    };
    vector<string>  unpack_Tn_COUNT(const int reg);
    vector<string>  unpack_Tn_MODE(const int reg);
    vector<string>  unpack_Tn_COMP(const int reg);
    vector<string>  unpack_Tn_HOLD(const int reg);
};
#endif
