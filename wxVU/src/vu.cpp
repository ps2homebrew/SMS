#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <iostream>
#include "vu.h"
#include "util.h"

using namespace std;

MicroCode Instr;

VU VUchip;

void VUParam::Reset() {
    type=0;
    index=0;
    sufix[0]=0;
    udata=0;
    data=0;
    label[0]=0;
    stalling=0;
    memdir=0; 
}

void
VUInstruction::Reset() {
    int i;
    addr=0;
    tics=0;
    breakpoint=0;
    SymbolIndex = -1;
    // InstIndex[0]=0; InstIndex[1]=0;
    // flavor[0]=0; flavor[1]=0;
    // dest[0][0]=0; dest[1][0]=0;
    // for (i=0; i<4; i++) {
        // Params[0][i].Reset();
        // Params[1][i].Reset();   
    // }
}

VU::VU() {
    Reset();
}

void VU::Reset() {
    //initialize VU
    uint32 i;

    NInstructions=0;
    NSymbols=0;
    NMemDir=0;
    clock=0;
    ClipFlag[0]=ClipFlag[1]=ClipFlag[2]=ClipFlag[3]=0;
    MacZ=0;
    MacS=0;
    MacU=0;
    MacO=0;
    StatusFlag = 0;
    for(i=0; i<32; i++)
        RegFloat[i].set(0.0);
    RegFloat[0].w(1.0);
    for(i=0; i<16; i++) {
        RegInt[i].value(0);
    }
    ACC.set(0.0);
    I.set(0.0);
    Q.set(0.0);
    P.set(0.0);
    PC = 0;
    clock=0;
    for(i = 0; i < MAX_VUDATA_SIZE; i++) {
        dataMem[i].x=0;
        dataMem[i].y=0;
        dataMem[i].z=0;
        dataMem[i].w=0;
        program[i].SymbolIndex = -1;
        program[i].Reset();
    }
    NInstructions=0;
    NSymbols=0;
    upperRegisterWrite = false;
    lowerRegisterWrite = false;
    specialRegisterWrite = false;
    memoryUpdate = true;
}

int VUReg::stall(){ return vstall;}
int VUReg::lastRead(){ return lRead;}
int VUReg::lastWrite(){ return lWrite;}
void VUReg::stall(int a){ vstall=a;}
void VUReg::decstall(){ if(vstall) vstall--;}
void VUReg::lastRead(int a){ lRead=a;}
void VUReg::lastWrite(int a){ lWrite=a;}
float VFReg::x(){ return data.vf.x;}
float VFReg::y(){ return data.vf.y;}
float VFReg::z(){ return data.vf.z;}
float VFReg::w(){ return data.vf.w;}
void VFReg::set(float a){ data.vf.x=a; data.vf.y=a; data.vf.z=a; data.vf.w=a;}
void VFReg::x(float a){ data.vf.x=a;}
void VFReg::y(float a){ data.vf.y=a;}
void VFReg::z(float a){ data.vf.z=a;}
void VFReg::w(float a){ data.vf.w=a;}
int16 VIReg::value(){ return data.vi;}
void VIReg::value(int16 v){ data.vi=v;}

void VFReg::mcopy(int *dest, int reg)
{
    switch(reg) {
        case 0:
            memcpy((void *)dest, (void *)&(data.vf.x), 4);
            break;
        case 1:
            memcpy((void *)dest, (void *)&(data.vf.y), 4);
            break;
        case 2:
            memcpy((void *)dest, (void *)&(data.vf.z), 4);
            break;
        case 3:
            memcpy((void *)dest, (void *)&(data.vf.w), 4);
            break;
    }
}

void VFReg::mwrite(int *org, int reg)
{
    switch(reg) {
        case 0:
            memcpy( (void *)&(data.vf.x), (void *)org, 4);
            break;
        case 1:
            memcpy( (void *)&(data.vf.y), (void *)org, 4);
            break;
        case 2:
            memcpy( (void *)&(data.vf.z), (void *)org, 4);
            break;
        case 3:
            memcpy( (void *)&(data.vf.w), (void *)org, 4);
            break;
    }
}

void VFReg::xtoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.x),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.x),b,i,15);
            break;
    }
}

void VFReg::ytoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.y),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.y),b,i,15);
            break;
    }
}

void VFReg::ztoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.z),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.z),b,i,15);
            break;
    }
}

void VFReg::wtoi(float v, int a)
{
    int b,i;
    long double c,d;
    b=(int)v; //integer part
    c=(long double)(v-(float)b); //decimal part
    switch (a) {
        case 0:
            memcpy((void *)&(data.vf.w),(void *)&b, 4);
            break;
        case 4:
            d=1.0/16.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,4);
            break;
        case 12:
            d=1.0/4096.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,12);
            break;
        case 15:
            d=1.0/32768.0;
            i=(int) round(c/d);
            setdata((void *)&(data.vf.w),b,i,15);
            break;
    }
}


void VFReg::xtof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.x=(float)b;
            break;
        case 4:
            //i=b&15;
            //d=(1.0/16.0)*(float)i;
            e=(float)b/16.0;
            data.vf.x=e; //d+e;
            break;
        case 12:
            //i=b&4095;
            //d=(1.0/4096.0)*(float)i;
            e=(float)b/4096.0;
            data.vf.x=e; //d+e;
            break;
        case 15:
            //i=b&32767;
            //d=(1.0/32768.0)*(float)i;
            e=(float)b/32768.0;
            data.vf.x=e; //d+e;
            break;
    }
}

void VFReg::ytof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.y=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.y=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.y=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.y=e;
            break;
    }
}

void VFReg::ztof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.z=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.z=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.z=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.z=e;
            break;
    }
}

void VFReg::wtof(int v, int a)
{
    int b;
    float e;
    memcpy((void *)&b,(void *)&v, 4);

    switch (a) {
        case 0:
            data.vf.w=(float)b;
            break;
        case 4:
            e=(float)b/16.0;
            data.vf.w=e;
            break;
        case 12:
            e=(float)b/4096.0;
            data.vf.w=e;
            break;
        case 15:
            e=(float)b/32768.0;
            data.vf.w=e;
            break;
    }
}




void VU::DecStall()
{
    int i;
    for(i=0; i<32; i++) {
        RegFloat[i].decstall();
    }
    for(i=0; i<16; i++) {
        RegInt[i].decstall();
    }
    ACC.decstall();
    I.decstall();
    Q.decstall();
    P.decstall();
}

void MicroCode::DecThroughput()
{
    int i,j;
    //well in fact this is wrong
    //the broadcast flavors are managed separate
    //when they should use the same instruction throughput
    //counter but :) no matter cause throughput for all
    //broacast instrcutions are 1 ;)

    for(i=0; i< nInstructionDef; i++)
        for(j=0;j<15;j++)
            if(Instr[i].lastthr[j])
                Instr[i].lastthr[j]--;
}

void
VU::SetCallback(void *objPtr, void (*fnPtr)(void *objPtr, int a, int b)) {
    CallBackFn = fnPtr;
    CallBackObj = objPtr;
}

void
VU::SetXGKICKCallback(void *objPtr, void (*fnPtr)(void *objPtr, int a)) {
    XGKICKCallBackFn = fnPtr;
    XGKICKCallBackObj = objPtr;
}

void
VU::Tic() {
    int i = 1,j = 1;
    if(program[PC].flg=='E') { //end of program
        CallBackFn(CallBackObj, UPPER, 999);
        return;
    }

    program[PC].tics = 0;

    while(i>0 || j>0) {
        program[PC].tics++;
        clock++;
        DecStall();                     //dec stall registers
        Instr.DecThroughput();          //dec throughput counter
        if(i) {
            i = DoUpper();              //try upper
            // CallBackFn(CallBackObj, UPPER, i);
        }
        if(j) {
            j = DoLower();              //try lower
            // if(j >= 0) {
                // CallBackFn(CallBackObj, LOWER, j);
            // }
        }
    }
    updateRegisters();                  // We update registers as the last
                                        // stage to keep the dual pipeline in
                                        // sync
    PC++;
    if(j >= 0) {
        CallBackFn(CallBackObj, 2, i);
    }
}

int VU::Stalling(VUParam &a)
{
    switch(a.type) {
        case 1: //VI
        case 3:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 20:
            a.stalling=RegInt[a.index].stall();
            if(a.stalling)
                return 1;
            return 0;
        case 2: //VF
        case 4:
            a.stalling=RegFloat[a.index].stall();
            if(a.stalling)
                return 1;
            return 0;
        case 11:
            a.stalling=I.stall();
            if(a.stalling)
                return 1;
            return 0;
        case 17://P does not stall but fails, should use waitp
            a.stalling=0;
            if(P.stall())
                return 4;
            return 0;
        case 18: //Q does not stall but fails, should use waitq
            a.stalling=0;
            if(Q.stall())
                return 5;
            return 0;
        case 19:
            a.stalling=R.stall();
            if(a.stalling)
                return 1;
            return 0;
        case 5:
            a.stalling=ACC.stall();
            if(a.stalling)
                return 1;
            return 0;
    }
    return 0;
}

void
VU::updateRegisters() {
    if ( specialRegisterWrite ) {
        switch (specialFloatIndex) {
            case 0:
                ACC.x(lowerFloatTemp.x());
                ACC.y(lowerFloatTemp.y());
                ACC.z(lowerFloatTemp.z());
                ACC.w(lowerFloatTemp.w());
                break;
            case 1:
                I.x(lowerFloatTemp.x());
                I.y(lowerFloatTemp.y());
                I.z(lowerFloatTemp.z());
                I.w(lowerFloatTemp.w());
                break;
        }
        specialRegisterWrite = false;
    }
}

int VU::DoUpper() {
    int i=1;
    if(Instr.Instr[program[PC].InstIndex[0]].lastthr[program[PC].flavor[0]])
        return 1;
    switch(program[PC].InstIndex[0]) {
        case 0:
            i = VU_ABS(program[PC]);
            break;
        case 1:
            i = VU_ADD(program[PC]);
            break;
        case 2:
            i = VU_CLIPW(program[PC]);
            break;
        case 3:
            i = VU_FTOI(program[PC], 0);
            break;
        case 4:
            i = VU_FTOI(program[PC], 4);
            break;
        case 5:
            i = VU_FTOI(program[PC], 12);
            break;
        case 6:
            i = VU_FTOI(program[PC], 15);
            break;
        case 7:
            i = VU_ITOF(program[PC], 0);
            break;
        case 8:
            i = VU_ITOF(program[PC],4);
            break;
        case 9:
            i = VU_ITOF(program[PC],12);
            break;
        case 10:
            i = VU_ITOF(program[PC],15);
            break;
        case 11:
            i = VU_MADD(program[PC]);
            break;
        case 12:
            i = VU_MAX(program[PC]);
            break;
        case 13:
            i = VU_MIN(program[PC]);
            break;
        case 14:
            i = VU_MSUB(program[PC]);
            break;
        case 15:
            i = VU_MUL(program[PC]);
            break;
        case 16:
            i = VU_NOP();
            break;
        case 17:
            i = VU_OPMULA(program[PC]);
            break;
        case 18:
            i = VU_OPMSUB(program[PC]);
            break;
        case 19:
            i = VU_SUB(program[PC]);
            break;
    }
    if(!i)
        Instr.Instr[program[PC].InstIndex[0]].lastthr[program[PC].flavor[0]]=Instr.Instr[program[PC].InstIndex[0]].throughput;
    return i;
}

int
VU::DoLower() {
    int i = 0;
    if(Instr.Instr[program[PC].InstIndex[1]].lastthr[program[PC].flavor[1]])
        return 1;
    switch(program[PC].InstIndex[1]) {
        case 20:
            i = VU_B(program[PC]);
            break;
        case 21:
            i = VU_BAL(program[PC]);
            break;
        case 22:
            i = VU_DIV(program[PC]);
            break;
        case 23:
            i = VU_EATAN(program[PC]);
            break;
        case 24:
            i = VU_EATANXY(program[PC]);
            break;
        case 25:
            i = VU_EATANXZ(program[PC]);
            break;
        case 26:
            i = VU_EEXP(program[PC]);
            break;
        case 27:
            i = VU_ELENG(program[PC]);
            break;
        case 28:
            i = VU_ERCPR(program[PC]);
            break;
        case 29:
            i = VU_ERLENG(program[PC]);
            break;
        case 30:
            i = VU_ERSADD(program[PC]);
            break;
        case 31:
            i = VU_ERSQRT(program[PC]);
            break;
        case 32:
            i = VU_ESADD(program[PC]);
            break;
        case 33:
            i = VU_ESIN(program[PC]);
            break;
        case 34:
            i = VU_ESQRT(program[PC]);
            break;
        case 35:
            i = VU_ESUM(program[PC]);
            break;
        case 36:
            i = VU_FCAND(program[PC]);
            break;
        case 37:
            i = VU_FCEQ(program[PC]);
            break;
        case 38:
            i = VU_FCGET(program[PC]);
            break;
        case 39:
            i = VU_FCOR(program[PC]);
            break;
        case 40:
            i = VU_FCSET(program[PC]);
            break;
        case 41:
            i = VU_FMAND(program[PC]);
            break;
        case 42:
            i = VU_FMEQ(program[PC]);
            break;
        case 43:
            i = VU_FMOR(program[PC]);
            break;
        case 44:
            i = VU_FSAND(program[PC]);
            break;
        case 45:
            i = VU_FSEQ(program[PC]);
            break;
        case 46:
            i = VU_FSOR(program[PC]);
            break;
        case 47:
            i = VU_FSSET(program[PC]);
            break;
        case 48:
            i = VU_IADD(program[PC]);
            break;
        case 49:
            i = VU_IADDI(program[PC]);
            break;
        case 50:
            i = VU_IADDIU(program[PC]);
            break;
        case 51:
            i = VU_IAND(program[PC]);
            break;
        case 52:
            i = VU_IBEQ(program[PC]);
            break;
        case 53:
            i = VU_IBGEZ(program[PC]);
            break;
        case 54:
            i = VU_IBGTZ(program[PC]);
            break;
        case 55:
            i = VU_IBLEZ(program[PC]);
            break;
        case 56:
            i = VU_IBLTZ(program[PC]);
            break;
        case 57:
            i = VU_IBNE(program[PC]);
            break;
        case 58:
            i = VU_ILW(program[PC]);
            break;
        case 59:
            i = VU_ILWR(program[PC]);
            break;
        case 60:
            i = VU_IOR(program[PC]);
            break;
        case 61:
            i = VU_ISUB(program[PC]);
            break;
        case 62:
            i = VU_ISUBIU(program[PC]);
            break;
        case 63:
            i = VU_ISW(program[PC]);
            break;
        case 64:
            i = VU_ISWR(program[PC]);
            break;
        case 65:
            i = VU_JARL(program[PC]);
            break;
        case 66:
            i = VU_JR(program[PC]);
            break;
        case 67:
            i = VU_LQ(program[PC]);
            break;
        case 68:
            i = VU_LQD(program[PC]);
            break;
        case 69:
            i = VU_LQI(program[PC]);
            break;
        case 70:
            i = VU_MFIR(program[PC]);
            break;
        case 71:
            i = VU_MFP(program[PC]);
            break;
        case 72:
            i = VU_MOVE(program[PC]);
            break;
        case 73:
            i = VU_MR32(program[PC]);
            break;
        case 74:
            i = VU_MTIR(program[PC]);
            break;
        case 75:
            i = VU_NOP();
            break;
        case 76:
            i = VU_RGET(program[PC]);
            break;
        case 77:
            i = VU_RINIT(program[PC]);
            break;
        case 78:
            i = VU_RNEXT(program[PC]);
            break;
        case 79:
            i = VU_RSQRT(program[PC]);
            break;
        case 80:
            i = VU_RXOR(program[PC]);
            break;
        case 81:
            i = VU_SQ(program[PC]);
            break;
        case 82:
            i = VU_SQD(program[PC]);
            break;
        case 83:
            i = VU_SQI(program[PC]);
            break;
        case 84:
            i = VU_SQRT(program[PC]);
            break;
        case 85:
            i = VU_WAITP();
            break;
        case 86:
            i = VU_WAITQ();
            break;
        case 87:
            i = VU_XGKICK(program[PC]);
            if (i >= 0) {
                XGKICKCallBackFn(XGKICKCallBackObj, i);
            }
            i = 0;
            break;
        case 88:
            i = VU_XITOP(program[PC]);
            break;
        case 89:
            i = VU_XTOP(program[PC]);
            break;
        case 90:
            i = VU_LOI_LOWER(program[PC]);
            break;
    }

    if(!i)
        Instr.Instr[program[PC].InstIndex[1]].lastthr[program[PC].flavor[1]]=Instr.Instr[program[PC].InstIndex[1]].throughput;
    return i;
}

void
VU::MemVal2(uint16 v2,int16 *v3) {
    uint16 a,b,c,d;
    uint32 val;

    a = (uint16)(v2/8);
    b = (uint16)(v2%8);
    c = (uint16)(b/2);
    d = (uint16)(b%2);

    switch(c) {
        case 0:
            memcpy(&val,&(dataMem[a].w),4);
            break;
        case 1:
            memcpy(&val,&(dataMem[a].z),4);
            break;
        case 2:
            memcpy(&val,&(dataMem[a].y),4);
            break;
        case 3:
            memcpy(&val,&(dataMem[a].x),4);
            break;
    }
    if(d) {
        val /= 65536        ; //takes 16 upper bits
    } else {
        val &= 65535        ; //takes 16 lower bits
    }
    memcpy(v3,&val,2);
}

void VU::MemVal16(uint16 v2,char pos, int16 *v3)
{
    uint32 val;
    switch(pos) {
        case 'W':
            memcpy(&val,&(dataMem[v2].w),4);
            break;
        case 'Z':
            memcpy(&val,&(dataMem[v2].z),4);
            break;
        case 'Y':
            memcpy(&val,&(dataMem[v2].y),4);
            break;
        case 'X':
            memcpy(&val,&(dataMem[v2].x),4);
            break;
    }
    val &= 65535; //takes 16 lower bits
    memcpy(v3,&val,2);
}

void
VU::MemSetVal16(uint16 v2, char pos, int16 v3) {
    uint32 val;
    val = v3;
    switch(pos) {
        case 'W':
            memcpy(&(dataMem[v2].w),&val,4);
            break;
        case 'Z':
            memcpy(&(dataMem[v2].z),&val,4);
            break;
        case 'Y':
            memcpy(&(dataMem[v2].y),&val,4);
            break;
        case 'X':
            memcpy(&(dataMem[v2].x),&val,4);
            break;
    }

}

uint32
VU::LoadRegisters(char *file) {
    FILE *fd;
    struct stat st;
    float fdata[512];
    uint32 idata[256];
    unsigned int i;
    fd = fopen(file, "rb"); 
    stat(file, &st);
    // Fill in the floats
    fread(fdata, 512, 1, fd);
    for (i = 0; i < 32; i++ ) {
        RegFloat[i].x(fdata[i*4+0]);
        RegFloat[i].y(fdata[i*4+1]);
        RegFloat[i].z(fdata[i*4+2]);
        RegFloat[i].w(fdata[i*4+3]);
    }
    // Fill in the integers
    fread(idata, 256, 1, fd);
    for (i = 0; i < 16; i++) {
        RegInt[i].value(idata[i*4+0]);
    }
    // Fill in the special registers
    // fread(sdata, 80, 1, fd);
    // for (i = 0; i < 5; i++) {
    //  }

    fclose(fd);
    return 0;
}
