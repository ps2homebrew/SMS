#ifndef _VIF
#define _VIF
#include "datatypes.h"
#include "sub.h"

typedef struct vifCode {
    uint32  CMD;
    uint32  NUM;
    uint32  IMMEDIATE;
};

// VU CMD
const int VIF_NOP       = 0x0;
const int VIF_STCYCL    = 0x1;
const int VIF_OFFSET    = 0x2;
const int VIF_BASE      = 0x3;
const int VIF_ITOP      = 0x4;
const int VIF_STMOD     = 0x5;
const int VIF_MSKPATH3  = 0x6;
const int VIF_MARK      = 0x7;
const int VIF_FLUSHE    = 0x10;
const int VIF_FLUSH     = 0x11;
const int VIF_FLUSHA    = 0x13;
const int VIF_MSCAL     = 0x14;
const int VIF_MSCNT     = 0x17;
const int VIF_MSCALF    = 0x15;
const int VIF_STMASK    = 0x20;
const int VIF_STROW     = 0x30;
const int VIF_STCOL     = 0x31;
const int VIF_MPG       = 0x4A;
const int VIF_DIRECT    = 0x50;
const int VIF_DIRECTHL  = 0x51;
const int VIF_UNPACK    = 0x60;
const int CMD_IRQ       = 0x80;
;
// Values for UNPACK;
const int UNPACK_S32    = 0x0;
const int UNPACK_S16    = 0x1;
const int UNPACK_S8     = 0x2;
const int UNPACK_V232   = 0x4;
const int UNPACK_V216   = 0x5;
const int UNPACK_V28    = 0x6;
const int UNPACK_V332   = 0x8;
const int UNPACK_V316   = 0x9;
const int UNPACK_V38    = 0xA;
const int UNPACK_V432   = 0xC;
const int UNPACK_V416   = 0xD;
const int UNPACK_V48    = 0xE;
const int UNPACK_V45    = 0xF;

class VIF : public SubSystem {
public:
    VIF(int);
    ~VIF();
    void            unpack(void);
    void            getVifcode(void);
    void            getFloatVec(void);

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
    void            get_vifcode(uint32);
    void            cmd_nop(void);
    void            cmd_stcycl(void);
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
    void            cmd_unpack(uint32);


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
    uint64          dmatag;
    uint32          vifcode;
    uint32          WL;
    uint32          CL;
    uint32          currentCode;
    uint32          *data;
    uint32          pos;
    uint32          size;
    uint32          vumem[1024*16];
    bool            interrupt;
    bool            maskpath3;
    uint32          flg;
    uint32          addr;
    uint32          num;
    uint32          usn;
    uint32          vpu;
    uint32          nREGISTERS;
};
#endif
