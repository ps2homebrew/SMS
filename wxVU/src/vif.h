#ifndef _VIF
#define _VIF
#include "datatypes.h"
#include "sub.h"
#include <fstream>

typedef struct vifCode {
    uint32  CMD;
    uint32  NUM;
    uint32  IMMEDIATE;
};

// VU CMD
const uint32 VIF_NOP       = 0x0;
const uint32 VIF_STCYCL    = 0x1;
const uint32 VIF_OFFSET    = 0x2;
const uint32 VIF_BASE      = 0x3;
const uint32 VIF_ITOP      = 0x4;
const uint32 VIF_STMOD     = 0x5;
const uint32 VIF_MSKPATH3  = 0x6;
const uint32 VIF_MARK      = 0x7;
const uint32 VIF_FLUSHE    = 0x10;
const uint32 VIF_FLUSH     = 0x11;
const uint32 VIF_FLUSHA    = 0x13;
const uint32 VIF_MSCAL     = 0x14;
const uint32 VIF_MSCNT     = 0x17;
const uint32 VIF_MSCALF    = 0x15;
const uint32 VIF_STMASK    = 0x20;
const uint32 VIF_STROW     = 0x30;
const uint32 VIF_STCOL     = 0x31;
const uint32 VIF_MPG       = 0x4A;
const uint32 VIF_DIRECT    = 0x50;
const uint32 VIF_DIRECTHL  = 0x51;
const uint32 VIF_UNPACK    = 0x60;
const uint32 CMD_IRQ       = 0x80;
;
// Values for UNPACK;
const uint32 UNPACK_S32    = 0x0;
const uint32 UNPACK_S16    = 0x1;
const uint32 UNPACK_S8     = 0x2;
const uint32 UNPACK_V232   = 0x4;
const uint32 UNPACK_V216   = 0x5;
const uint32 UNPACK_V28    = 0x6;
const uint32 UNPACK_V332   = 0x8;
const uint32 UNPACK_V316   = 0x9;
const uint32 UNPACK_V38    = 0xA;
const uint32 UNPACK_V432   = 0xC;
const uint32 UNPACK_V416   = 0xD;
const uint32 UNPACK_V48    = 0xE;
const uint32 UNPACK_V45    = 0xF;

const uint32 MODE_DIRECT   = 0x0;
const uint32 MODE_ADD      = 0x1;
const uint32 MODE_ADDROW   = 0x2;

class VIF : public SubSystem {
public:
    VIF(int);
    VIF(const char *filename);
    VIF(const char *filename, int numregs);
    ~VIF();
    bool            eof(void);
    bool            valid(void);
    uint32          read(void);
    uint32          cmd(void);
    void            decode_cmd(void);
    void            getVifcode(void);
    void            getFloatVec(void);
    vector<string>  getRegisterText(const int reg) {
        vector<string> v;
        return v;
    };

    vector<string>      unpack_VIF_ITOPS(const int reg);
    vector<string>      unpack_VIF_ITOP(const int reg);
    vector<string>      unpack_VIF_R(const int reg);
    vector<string>      unpack_VIF_C(const int reg);
    vector<string>      unpack_VIF_CYCLE(const int reg);
    vector<string>      unpack_VIF_MODE(const int reg);
    vector<string>      unpack_VIF_MASK(const int reg);
    vector<string>      unpack_VIF_ERR(const int reg);
    vector<string>      unpack_VIF_MARK(const int reg);
    vector<string>      unpack_VIF_NUM(const int reg);
    vector<string>      unpack_VIF_CODE(const int reg);
private:
    // vif command functions
    void            cmd_nop(void);
    void            cmd_offset(void);
    void            cmd_base(void);
    void            cmd_itop(void);
    void            cmd_stmod(void);
    void            cmd_mskpath3(void);
    void            cmd_mark(void);
    void            cmd_flushe(void);
    void            cmd_flush(void);
    void            cmd_flusha(void);
    void            cmd_mscal(void);
    void            cmd_mscnt(void);
    void            cmd_mscalf(void);
    void            cmd_stmask(void);
    void            cmd_strow(void);
    void            cmd_stcol(void);
    void            cmd_mpg(void);
    void            cmd_direct(void);
    void            cmd_directhl(void);
    uint32          cmd_unpack(void);
    void            writeCode(void);
    void            writeRegister(void);
    void            writeData(void);
    void            clearRegisters(void);


    // VIF Registers
    uint32   rVIF1_R0;
    uint32   rVIF1_R1;
    uint32   rVIF1_R2;
    uint32   rVIF1_R3;
    uint32   rVIF1_C0;
    uint32   rVIF1_C1;
    uint32   rVIF1_C2;
    uint32   rVIF1_C3;
    uint32   rVIF1_CYCLE;
    uint32   rVIF1_MASK;
    uint32   rVIF1_MODE;
    uint32   rVIF1_ITOP;
    uint32   rVIF1_ITOPS;
    uint32   rVIF1_BASE;
    uint32   rVIF1_OFST;
    uint32   rVIF1_TOP;
    uint32   rVIF1_TOPS;
    uint32   rVIF1_MARK;
    uint32   rVIF1_NUM;
    uint32   rVIF1_CODE;
    uint32   rVIF1_STAT;
    uint32   rVIF1_FBRST;
    uint32   rVIF1_ERR;

    uint32   rVIF0_R0;
    uint32   rVIF0_R1;
    uint32   rVIF0_R2;
    uint32   rVIF0_R3;
    uint32   rVIF0_C0;
    uint32   rVIF0_C1;
    uint32   rVIF0_C2;
    uint32   rVIF0_C3;
    uint32   rVIF0_CYCLE;
    uint32   rVIF0_MASK;
    uint32   rVIF0_MODE;
    uint32   rVIF0_ITOP;
    uint32   rVIF0_ITOPS;
    uint32   rVIF0_MARK;
    uint32   rVIF0_NUM;
    uint32   rVIF0_CODE;

    // helper vars
    uint32          vifcode;
    uint32          *data;
    bool            interrupt;
    bool            _maskpath3;
    uint32          _flg;
    uint32          _addr;
    uint32          _usn;
    uint32          vpu;
    uint32          nREGISTERS;
    ifstream        _fin;
    uint32          _cmd;
    uint32          _num;
    uint32          _imm;
    uint32          _unpack;
    uint32          _WL;
    uint32          _CL;
    uint32          _length;
    uint32          _memIndex;
    uint32          _codeIndex;
};
#endif
