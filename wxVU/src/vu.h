#ifndef _VU_H
#define _VU_H

#include "datatypes.h"

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

class VUReg {
private:
	int vstall;
	int lRead;
	int lWrite;
protected:
	vec data;
public:
	int stall(); //stall states before can accessed
	int lastRead(); //last instruction in read value
	int lastWrite(); //last instruction in write value
	void stall(int v); //set previously defined values
	void decstall();
	void lastRead(int v);
	void lastWrite(int v);
};

class VFReg:public VUReg {
public:
	float x();
	float y();
	float z();
	float w();
	void set(float v);
	void x(float v);
	void y(float v);
	void z(float v);
	void w(float v);
	void xtoi(float v, int a);
	void ytoi(float v, int a);
	void ztoi(float v, int a);
	void wtoi(float v, int a);
	void xtof(int v, int a);
	void ytof(int v, int a);
	void ztof(int v, int a);
	void wtof(int v, int a);
	void mcopy(int *dest, int reg);
	void mwrite(int *org, int reg);   
};

class VIReg:public VUReg {
public:
	int16 value();              //read values from register
	void value(int16 v);        //sets values on register
};

class VUInstructionDef {
public:
	int     mode;               //uppder-lower-macromode
	int     sufix;              //diferent flavors
	char    nemot[15];          //instruction
	int     operands;           //number of operands
	char    types[4][50];       //type of operands (by order)
	int     throughput;         //see VU manual
	int     latency;            //
	int     lastthr[15];        //one for each flavor
	friend class VUInstruction;
};

class MicroCode
{
public:
	VUInstructionDef Instr[200];
	int nInstructionDef;
	void DecThroughput();  
};

class VUParam {
public:
	void    Reset();
	int     type;           //kind of parameter vf, vi, value, other
	int     index;          //if vf-vi-other index
	char    sufix[5];       //x-y-z-w-combination
	unsigned long udata;    //data value
	long    data;           //data value
	float   fdata;
	char    label[50];
	int     stalling;
	int     memdir;
};

class VUInstruction {
public:
	void    Reset();
	char    flg;
	int     addr;				//memory address
	int     tics;
    int     invalid;
	int     breakpoint;			//do the user toogle to breackpoint?
	int     SymbolIndex;		//Symbol index, only for easy drawing
	int     InstIndex[2];		//pointer to instruction class
	int     flavor[2];
	char    dest[2][50];
	VUParam Params[2][4];		//parameters
};

class Symbol {
public:
	char symb[50];
	int Line;
};

class VU {
public:
    static const int nREGISTERS = 2;
	VFReg           RegFloat[32];       //32 float registers
	VIReg           RegInt[16];         //16 integer registers
	VFReg           ACC, I, Q, P, R;    //special registers
	uint16          PC;                 //program counter
	int64           clock;              //clock ticks
	dataquad        dataMem[MAX_VUDATA_SIZE];   //data memory 16 Kb
	VUInstruction   program[MAX_VUCODE_SIZE];   //programm to be executed
	uint32          NInstructions;
	Symbol          Labels[MAX_VUCODE_SIZE];
	int             NSymbols;
	Symbol          MemDir[MAX_VUCODE_SIZE];
	int             NMemDir;
	uint16          ClipFlag[4];
	int             StatusFlag;
	char            MacZ,MacS,MacU,MacO;

	VU();
	void            Tic(void);
	int             DoUpper();
	int             DoLower();
    void            updateRegisters();
	void            Reset();
	void            DecStall();
	int             Stalling(VUParam &a);
	void            *CallBackObj;
	void            (*CallBackFn)(void *, int, int);
	void            *XGKICKCallBackObj;
	void            (*XGKICKCallBackFn)(void *, int);
	uint32          LoadRegisters(char *file);

	void MemVal2(uint16 v2,int16 *v3);
	void MemVal16(uint16 v2,char pos, int16 *v3);
	void MemSetVal16(uint16 v2,char pos, int16 v3);

	//UPPER instructions
	int VU_ABS(VUInstruction &a);
	int VU_ADD(VUInstruction &a);
	int VU_CLIPW(VUInstruction &a);
	int VU_FTOI(VUInstruction &a, int mode);
	int VU_ITOF(VUInstruction &a, int mode);
	int VU_MADD(VUInstruction &a);
	int VU_MAX(VUInstruction &a);
	int VU_MIN(VUInstruction &a);
	int VU_MSUB(VUInstruction &a);
	int VU_MUL(VUInstruction &a);
	int VU_NOP(void);
	int VU_OPMULA(VUInstruction &a);
	int VU_OPMSUB(VUInstruction &a);
	int VU_SUB(VUInstruction &a);

	//LOWER instructions
	int VU_B(VUInstruction &a);
	int VU_BAL(VUInstruction &a);
	int VU_DIV(VUInstruction &a);
	int VU_EATAN(VUInstruction &A);
	int VU_EATANXY(VUInstruction &A);
	int VU_EATANXZ(VUInstruction &A);
	int VU_EEXP(VUInstruction &A);
	int VU_ELENG(VUInstruction &A);
	int VU_ERCPR(VUInstruction &A);
	int VU_ERLENG(VUInstruction &A);
	int VU_ERSADD(VUInstruction &A);
	int VU_ERSQRT(VUInstruction &A);
	int VU_ESADD(VUInstruction &A);
	int VU_ESIN(VUInstruction &A);
	int VU_ESQRT(VUInstruction &A);
	int VU_ESUM(VUInstruction &A);
	int VU_FCAND(VUInstruction &A);    
	int VU_FCEQ(VUInstruction &A);
	int VU_FCGET(VUInstruction &A);
	int VU_FCOR(VUInstruction &A);
	int VU_FCSET(VUInstruction &A);
	int VU_FMAND(VUInstruction &A);
	int VU_FMEQ(VUInstruction &A);
	int VU_FMOR(VUInstruction &A);
	int VU_FSAND(VUInstruction &A);
	int VU_FSEQ(VUInstruction &A);
	int VU_FSOR(VUInstruction &A);
	int VU_FSSET(VUInstruction &A);
	int VU_IADD(VUInstruction &A);
	int VU_IADDI(VUInstruction &A);
	int VU_IADDIU(VUInstruction &A);
	int VU_IAND(VUInstruction &A);
	int VU_IBEQ(VUInstruction &A);
	int VU_IBGEZ(VUInstruction &A);
	int VU_IBGTZ(VUInstruction &A);
	int VU_IBLEZ(VUInstruction &A);
	int VU_IBLTZ(VUInstruction &A);
	int VU_IBNE(VUInstruction &A);
	int VU_ILW(VUInstruction &A);
	int VU_ILWR(VUInstruction &A);
	int VU_IOR(VUInstruction &A);
	int VU_ISUB(VUInstruction &A);
	int VU_ISUBIU(VUInstruction &A);
	int VU_ISW(VUInstruction &A);
	int VU_ISWR(VUInstruction &A);
	int VU_JARL(VUInstruction &A);
	int VU_JR(VUInstruction &A);
	int VU_LQ(VUInstruction &A);
	int VU_LQD(VUInstruction &A);
	int VU_LQI(VUInstruction &A);
	int VU_MFIR(VUInstruction &A);
	int VU_MFP(VUInstruction &A);
	int VU_MOVE(VUInstruction &A);
	int VU_MR32(VUInstruction &A);
	int VU_MTIR(VUInstruction &A);
	int VU_RGET(VUInstruction &A);
	int VU_RINIT(VUInstruction &A);
	int VU_RNEXT(VUInstruction &A);
	int VU_RSQRT(VUInstruction &A);
	int VU_RXOR(VUInstruction &A);
	int VU_SQ(VUInstruction &A);
	int VU_SQD(VUInstruction &A);
	int VU_SQI(VUInstruction &A);
	int VU_SQRT(VUInstruction &A);
	int VU_WAITP(void);
	int VU_WAITQ(void);
	int VU_XGKICK(VUInstruction &A);
	int VU_XITOP(VUInstruction &A);
	int VU_XTOP(VUInstruction &A);
	int VU_LOI_UPPER(VUInstruction &A);
	int VU_LOI_LOWER(VUInstruction &A);
	void SetCallback(void *, void (*)(void *, int, int));
	void SetXGKICKCallback(void *, void (*)(void *, int));
private:
    bool    lowerRegisterWrite;
    bool    upperRegisterWrite;
    bool    specialRegisterWrite;
    int     lowerIntIndex;
    int     lowerFloatIndex;
    int     upperIntIndex;
    int     upperFloatIndex;
    int     specialFloatIndex;
    VIReg   lowerIntTemp;
    VFReg   lowerFloatTemp;
    VIReg   upperIntTemp;
    VFReg   upperFloatTemp;
    
};
#endif
