#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <iostream>

#include "wx/string.h"

#include "vu.h"
#include "MemoryPanel.h"
#include "VuMem.h"
#include "VuRegisterPanel.h"
#include "VuParam.h"
#include "util.h"

using namespace std;
MicroCode Instr;

/////////////////////////////// PUBLIC ///////////////////////////////////////
Vu::Vu(uint32 vpuType, MemoryPanel* memPanel) : SubSystem(0) {
    m_vpuType = vpuType;
    m_pMemoryPanel = memPanel;
    if ( m_vpuType == VPU_0 ) {
        m_maxRows = VU0_SIZE/16;
    } else if ( m_vpuType == VPU_1 ) {
        m_maxRows = VU1_SIZE/16;
    } else {
        assert("failed to init vu");
    }
    m_pVuMem = new VuMem(m_maxRows, memPanel);
    // m_pParser = new Parser(this);

    Reset();
}

// initialize VU
void Vu::Reset() {
    ResetCode();
    ResetMemory();

}

void Vu::ResetMemory() {
    uint32 i;
    for(i = 0; i < MAX_VUCODE_SIZE; i++)  {
        program[i].SymbolIndex = -1;
        program[i].Reset();
    }

    for(i = 0; i < MAX_VUDATA_SIZE; i++)  {
      m_pVuMem->WriteVector(i, 0, 0, 0, 0);
		}
    m_pMemoryPanel->Clear();
}

void Vu::ResetCode() {
    uint32 i;
    NMemDir = 0;

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

    InitCodeMem();
    m_upperRegisterWrite = false;
    m_lowerRegisterWrite = false;
    m_specialRegisterWrite = false;
    NSymbols = 0;
    NInstructions = 0;
    m_clock = 0;
    ACC.set(0.0);
    I.set(0.0);
    Q.set(0.0);
    P.set(0.0);
    PC = 0;
}

void Vu::DecStall()
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

void
Vu::SetCallback(void *objPtr, void (*fnPtr)(void *objPtr, int a, int b)) {
    CallBackFn = fnPtr;
    CallBackObj = objPtr;
}

void
Vu::SetXGKICKCallback(void *objPtr, void (*fnPtr)(void *objPtr, int a)) {
    XGKICKCallBackFn = fnPtr;
    XGKICKCallBackObj = objPtr;
}

void
Vu::Tic() {
    int i = 1,j = 1;
    if(program[PC].flg == 'E') {
        CallBackFn(CallBackObj, UPPER, 999);
        return;
    }

    program[PC].tics = 0;

    while(i>0 || j>0) {
        program[PC].tics++;
        m_clock++;
        DecStall();                     //dec stall registers
        Instr.DecThroughput();          //dec throughput counter
        if(i) {
            i = DoUpper();              //try upper
        }
        if(j) {
            j = DoLower();              //try lower
        }
    }
    UpdateRegisters();                  // We update registers as the last
                                        // stage to keep the dual pipeline in
    PC++;
    if(j >= 0) {
        CallBackFn(CallBackObj, 2, i);
    }
}

const int32
Vu::Run(const int32 addr) {
    PC = addr;
    return Run();
}

const int32
Vu::Run(void) {
    if ( m_trace ) {
        m_pLog->Trace(0, "VU\n");
        m_pLog->Trace(1, wxString::Format("PC = %d\n", PC));
    }

    program[PC].tics = 0;
    while((program[PC].flg != 'E') && (PC <= m_maxRows)) {
        int32 i = 1;
        int32 j = 1;
        while(i>0 || j>0) {
            program[PC].tics++;
            m_clock++;
            DecStall();
            Instr.DecThroughput();
            if(i) {
                i = DoUpper();
            }
            if(j) {
                j = DoUpper();
            }
        }
        UpdateRegisters();              // Update registers after doing both
                                        // upper and lower pipeline
        PC++;
    }

    if ( m_trace ) {
        m_pLog->Trace(1, wxString::Format("Stopped ( e bit ), PC = %d\n", PC));
    }
    return E_OK;
}

void
Vu::UpdateRegisters() {
    if ( m_specialRegisterWrite ) {
        switch (m_specialFloatIndex) {
            case 0:
                ACC.x(m_lowerFloatTemp.x());
                ACC.y(m_lowerFloatTemp.y());
                ACC.z(m_lowerFloatTemp.z());
                ACC.w(m_lowerFloatTemp.w());
                break;
            case 1:
                I.x(m_lowerFloatTemp.x());
                I.y(m_lowerFloatTemp.y());
                I.z(m_lowerFloatTemp.z());
                I.w(m_lowerFloatTemp.w());
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
                break;
        }
        m_specialRegisterWrite = false;
    }
}

void
Vu::MemVal16(uint16 v2, char pos, int16 *v3) {
    uint32 val;
    switch(pos) {
        case 'W':
            val = m_pVuMem->ReadW(v2);
            break;
        case 'Z':
            val = m_pVuMem->ReadZ(v2);
            break;
        case 'Y':
            val = m_pVuMem->ReadY(v2);
            break;
        case 'X':
            val = m_pVuMem->ReadX(v2);
            break;
    }
    val &= 65535; //takes 16 lower bits
    memcpy(v3, &val, 2);
}

const int32
Vu::LoadRegisters(const char *file) {
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
    return E_OK;
}

void
Vu::WriteMemX(uint32 row, uint32 x) {
    m_pVuMem->WriteX(row, x);
}
void
Vu::WriteMemY(uint32 row, uint32 y) {
    m_pVuMem->WriteY(row, y);
}
void
Vu::WriteMemZ(uint32 row, uint32 z) {
    m_pVuMem->WriteZ(row, z);
}
void
Vu::WriteMemW(uint32 row, uint32 w) {
    m_pVuMem->WriteW(row, w);
}

int32
Vu::ReadMem(uint32* data, uint32 offset, uint32 size) {
    return m_pVuMem->ReadMem(data, offset, size);
}

int32
Vu::ReadMemX(uint32 row) {
    return m_pVuMem->ReadX(row);
}

int32
Vu::ReadMemY(uint32 row) {
    return m_pVuMem->ReadY(row);
}

int32
Vu::ReadMemZ(uint32 row) {
    return m_pVuMem->ReadZ(row);
}

int32
Vu::ReadMemW(uint32 row) {
    return m_pVuMem->ReadW(row);
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
int
Vu::DoUpper() {
    if(Instr.Instr[program[PC].InstIndex[0]].lastthr[program[PC].flavor[0]]) {
        return 1;
    }
    int i = 1;
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
Vu::DoLower() {
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
Vu::WriteFloatRegister(const int index, const char *dst, int32 &x, int32 &y,
    int32 &z, int32 &w) {

    if(strstr(dst,"X")) {
        RegFloat[index].mwrite(&x, 0);
    }
    if(strstr(dst,"Y")) {
        RegFloat[index].mwrite(&y, 1);
    }
    if(strstr(dst,"Z")) {
        RegFloat[index].mwrite(&z, 2);
    }
    if(strstr(dst,"W")) {
        RegFloat[index].mwrite(&w, 3);
    }
    m_pVuRegisterPanel->WriteFloat(
        index,
        RegFloat[index].x(),
        RegFloat[index].y(),
        RegFloat[index].z(),
        RegFloat[index].w()
        );
}

int
Vu::Stalling(VuParam &a) {
    if ( m_trace ) {
        m_pLog->Trace(1, "Stalling\n");
    }
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
Vu::MemSetVal16(uint16 v2, char pos, int16 v3) {
    uint32 val;
    val = v3;
    switch(pos) {
        case 'W':
            m_pVuMem->WriteW(v2, val);
            // m_pMemoryPanel->WriteW(v2, m_pVuv2);
            break;
        case 'Z':
            m_pVuMem->WriteZ(v2, val);
            // m_pMemoryPanel->WriteZ(v2, dataMem[v2].z);
            break;
        case 'Y':
            m_pVuMem->WriteY(v2, val);
            // m_pMemoryPanel->WriteY(v2, dataMem[v2].y);
            break;
        case 'X':
            m_pVuMem->WriteX(v2, val);
            // m_pMemoryPanel->WriteX(v2, dataMem[v2].x);
            break;
    }
}

void
Vu::MemVal2(uint16 v2,int16 *v3) {
    uint16 a,b,c,d;
    uint32 val;

    a = (uint16)(v2/8);
    b = (uint16)(v2%8);
    c = (uint16)(b/2);
    d = (uint16)(b%2);

    switch(c) {
        case 0:
            val = m_pVuMem->ReadW(a);
            break;
        case 1:
            val = m_pVuMem->ReadZ(a);
            break;
        case 2:
            val = m_pVuMem->ReadY(a);
            break;
        case 3:
            val = m_pVuMem->ReadX(a);
            break;
    }
    if(d) {
        val /= 65536        ; //takes 16 upper bits
    } else {
        val &= 65535        ; //takes 16 lower bits
    }
    memcpy(v3, &val, 2);
}

void
Vu::InitCodeMem(void) {
    unsigned int i;
    for(i = 0; i < m_maxRows; i++) {
        // m_pParser->insert(strdup("nop"), strdup("loi"), strdup(""), strdup("0x0"), i);
    }
    return;
}
