#ifndef _SubSystem
#define _SubSystem
class SubSystem {
public:
    SubSystem(const int num) {
        int i;
        REGISTERS = new uint32[num];
        nREGISTERS = num;
        for(i = 0; i < nREGISTERS; i++) {
            REGISTERS[i] = 0;
        }
    }
    virtual vector<string>  getRegisterText(const int reg) = 0;
    virtual uint32          readRegister(const int reg) {
        return REGISTERS[reg];
    };
    virtual uint32          writeRegister(const int reg, uint32 value) {
        REGISTERS[reg] = value;
    };
    virtual uint32          initRegisters(uint32 *data) {
        int i;
        for (i = 0; i < nREGISTERS ; i++) {
            REGISTERS[i] = *(data++);
        }
        return 0;
    };
    uint32  *REGISTERS;
private:
    uint32  nREGISTERS;
};
#endif
