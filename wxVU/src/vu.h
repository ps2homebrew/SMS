#ifndef _VU_H
#define _VU_H

#include <vector>
#include <string>

#include "datatypes.h"
#include "sub.h"
#include "VuInstruction.h"
#include "VuFloatReg.h"
#include "VuIntReg.h"
#include "VuReg.h"
#include "VuSymbol.h"
#include "MicroCode.h"

class MemoryPanel;
class VuMem;
class VuRegisterPanel;
class VuParam;

#define UPPER 0
#define LOWER 1
#define VPU0 0
#define VPU1 1
#define VU0_DATA_MEM 0x11004000
#define VU1_DATA_MEM 0x1100C000
#define VU0_CODE_MEM 0x11000000
#define VU1_CODE_MEM 0x11008000
#define VU0_SIZE 4096
#define VU1_SIZE 16384

static const uint32 MAX_VUDATA_SIZE = 1024;
static const uint32 MAX_VUCODE_SIZE = 2048;

static const char *tVU_REGISTERS[] = {
	"cmsar", "fbrst"
};

class Vu : public SubSystem {
public:
    Vu(uint32 vpuType, MemoryPanel* memPanel);
	VuFloatReg      RegFloat[32];       //32 float registers
	VuIntReg        RegInt[16];         //16 integer registers
	VuFloatReg      ACC, I, Q, P, R;    //special registers
	uint16          PC;                 //program counter
	int64           m_clock;            //clock ticks
	VuInstruction   program[MAX_VUCODE_SIZE];   //programm to be executed
	uint32          NInstructions;
	VuSymbol        Labels[MAX_VUCODE_SIZE];
	int             NSymbols;
	VuSymbol        MemDir[MAX_VUCODE_SIZE];
	int             NMemDir;
	uint16          ClipFlag[4];
	int             StatusFlag;
	char            MacZ,MacS,MacU,MacO;
    int32           ReadMemX(uint32 row);
    int32           ReadMemY(uint32 row);
    int32           ReadMemZ(uint32 row);
    int32           ReadMemW(uint32 row);
    void            WriteMemX(uint32 row, uint32 x);
    void            WriteMemY(uint32 row, uint32 y);
    void            WriteMemZ(uint32 row, uint32 z);
    void            WriteMemW(uint32 row, uint32 w);
	const int32     LoadRegisters(const char *file);
    // const int32     LoadMemory(const char *filename);
    // const int32     LoadCode(const char *filename);
    const int32     Run(const int32 _addr);
    const int32     Run(void);

	void            Tic(void);
	void            Reset();
	void            DecStall();
    // KLUDGE
	void            *CallBackObj;
	void            (*CallBackFn)(void *, int, int);
	void            *XGKICKCallBackObj;
	void            (*XGKICKCallBackFn)(void *, int);
	void            SetCallback(void *, void (*)(void *, int, int));
	void            SetXGKICKCallback(void *, void (*)(void *, int));
    // TODO
    const vector<string>    GetRegisterText(const int reg) {
        vector<string> v;
        return v;
    }

    // KLUDGE
    VuRegisterPanel*    m_pVuRegisterPanel;
    // Parser*             m_pParser;

private:
	void            MemVal2(uint16 v2,int16 *v3);
	void            MemVal16(uint16 v2,char pos, int16 *v3);
	void            MemSetVal16(uint16 v2,char pos, int16 v3);
	int             Stalling(VuParam &a);
    void            WriteFloatRegister(const int index, const char *dst,
                        int32 &x, int32 &y, int32 &z, int32 &w);
    void            InitCodeMem();

	//UPPER instructions
	int     VU_ABS(VuInstruction &a);
	int     VU_ADD(VuInstruction &a);
	int     VU_CLIPW(VuInstruction &a);
	int     VU_FTOI(VuInstruction &a, int mode);
	int     VU_ITOF(VuInstruction &a, int mode);
	int     VU_MADD(VuInstruction &a);
	int     VU_MAX(VuInstruction &a);
	int     VU_MIN(VuInstruction &a);
	int     VU_MSUB(VuInstruction &a);
	int     VU_MUL(VuInstruction &a);
	int     VU_NOP(void);
	int     VU_OPMULA(VuInstruction &a);
	int     VU_OPMSUB(VuInstruction &a);
	int     VU_SUB(VuInstruction &a);

	//LOWER instructions
	int     VU_B(VuInstruction &a);
	int     VU_BAL(VuInstruction &a);
	int     VU_DIV(VuInstruction &a);
	int     VU_EATAN(VuInstruction &A);
	int     VU_EATANXY(VuInstruction &A);
	int     VU_EATANXZ(VuInstruction &A);
	int     VU_EEXP(VuInstruction &A);
	int     VU_ELENG(VuInstruction &A);
	int     VU_ERCPR(VuInstruction &A);
	int     VU_ERLENG(VuInstruction &A);
	int     VU_ERSADD(VuInstruction &A);
	int     VU_ERSQRT(VuInstruction &A);
	int     VU_ESADD(VuInstruction &A);
	int     VU_ESIN(VuInstruction &A);
	int     VU_ESQRT(VuInstruction &A);
	int     VU_ESUM(VuInstruction &A);
	int     VU_FCAND(VuInstruction &A);    
	int     VU_FCEQ(VuInstruction &A);
	int     VU_FCGET(VuInstruction &A);
	int     VU_FCOR(VuInstruction &A);
	int     VU_FCSET(VuInstruction &A);
	int     VU_FMAND(VuInstruction &A);
	int     VU_FMEQ(VuInstruction &A);
	int     VU_FMOR(VuInstruction &A);
	int     VU_FSAND(VuInstruction &A);
	int     VU_FSEQ(VuInstruction &A);
	int     VU_FSOR(VuInstruction &A);
	int     VU_FSSET(VuInstruction &A);
	int     VU_IADD(VuInstruction &A);
	int     VU_IADDI(VuInstruction &A);
	int     VU_IADDIU(VuInstruction &A);
	int     VU_IAND(VuInstruction &A);
	int     VU_IBEQ(VuInstruction &A);
	int     VU_IBGEZ(VuInstruction &A);
	int     VU_IBGTZ(VuInstruction &A);
	int     VU_IBLEZ(VuInstruction &A);
	int     VU_IBLTZ(VuInstruction &A);
	int     VU_IBNE(VuInstruction &A);
	int     VU_ILW(VuInstruction &A);
	int     VU_ILWR(VuInstruction &A);
	int     VU_IOR(VuInstruction &A);
	int     VU_ISUB(VuInstruction &A);
	int     VU_ISUBIU(VuInstruction &A);
	int     VU_ISW(VuInstruction &A);
	int     VU_ISWR(VuInstruction &A);
	int     VU_JARL(VuInstruction &A);
	int     VU_JR(VuInstruction &A);
	int     VU_LQ(VuInstruction &A);
	int     VU_LQD(VuInstruction &A);
	int     VU_LQI(VuInstruction &A);
	int     VU_MFIR(VuInstruction &A);
	int     VU_MFP(VuInstruction &A);
	int     VU_MOVE(VuInstruction &A);
	int     VU_MR32(VuInstruction &A);
	int     VU_MTIR(VuInstruction &A);
	int     VU_RGET(VuInstruction &A);
	int     VU_RINIT(VuInstruction &A);
	int     VU_RNEXT(VuInstruction &A);
	int     VU_RSQRT(VuInstruction &A);
	int     VU_RXOR(VuInstruction &A);
	int     VU_SQ(VuInstruction &A);
	int     VU_SQD(VuInstruction &A);
	int     VU_SQI(VuInstruction &A);
	int     VU_SQRT(VuInstruction &A);
	int     VU_WAITP(void);
	int     VU_WAITQ(void);
	int     VU_XGKICK(VuInstruction &A);
	int     VU_XITOP(VuInstruction &A);
	int     VU_XTOP(VuInstruction &A);
	int     VU_LOI_UPPER(VuInstruction &A);
	int     VU_LOI_LOWER(VuInstruction &A);
	int     DoUpper();
	int     DoLower();
    void    UpdateRegisters();

    // Used for dual pipeline sync
    bool            m_lowerRegisterWrite;
    bool            m_upperRegisterWrite;
    bool            m_specialRegisterWrite;
    int             m_lowerIntIndex;
    int             m_lowerFloatIndex;
    int             m_upperIntIndex;
    int             m_upperFloatIndex;
    int             m_specialFloatIndex;
    VuIntReg        m_lowerIntTemp;
    VuFloatReg      m_lowerFloatTemp;
    VuIntReg        m_upperIntTemp;
    VuFloatReg      m_upperFloatTemp;

    uint32          m_vpuType;
    uint32          m_maxRows;
    VuMem*          m_pVuMem;
    MemoryPanel*    m_pMemoryPanel;
};

enum VpuType {
    VPU_0,
    VPU_1
};
#endif
