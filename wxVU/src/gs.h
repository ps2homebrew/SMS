#ifndef _GS
#define _GS
#include <vector>
#include "datatypes.h"
#include "sub.h"

using namespace std;
enum GS_REGISTERS {
    GS_SIGLBLD, GS_CSR
};
static const char *tGS_REGISTERS[] = {
	"SIGLBLD", "CSR"
};

class GS : public SubSystem {
public:
    GS();
    ~GS();
    static const int    nREGISTERS = 2;
    vector<string>      getRegisterText(const int reg);
private:
    vector<string>      unpack_SIGLBLID(const int reg);
    vector<string>      unpack_CSR(const int reg);

};
#endif
