#ifndef __SubSystem__
#define __SubSystem__

#include "Log.h"

using namespace std;
class SubSystem {
public:
    SubSystem(const int num) {
        uint32 i;
        REGISTERS = new uint32[num];
        nREGISTERS = num;
        for(i = 0; i < nREGISTERS; i++) {
            REGISTERS[i] = 0;
        }
        m_pLog = Log::Instance();
        m_trace = false;
    }
    virtual ~SubSystem(){};
    virtual const vector<string>  GetRegisterText(const int reg) = 0;
    virtual uint32          GetRegister(const int reg) {
        return REGISTERS[reg];
    };
    virtual uint32          WriteRegister(const int reg, uint32 value) {
        REGISTERS[reg] = value;
        return 0;
    };
    virtual uint32          initRegisters(uint32 *data) {
        uint32 i;
        for (i = 0; i < nREGISTERS ; i++) {
            REGISTERS[i] = *(data++);
        }
        return 0;
    };
    void    Trace(bool mode) {
        m_trace = mode;
    };
    uint32*     REGISTERS;
protected:
    bool        m_trace;
    Log*        m_pLog;
private:
    uint32      nREGISTERS;
};
#endif
