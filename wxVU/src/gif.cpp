#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/stat.h>

#include "gif.h"

static const int PRIM      = 0x0;
static const int RGBAQ     = 0x1;
static const int ST        = 0x2;
static const int UV        = 0x3;
static const int XYZF2     = 0x4;
static const int XYZ2      = 0x5;
static const int TEX0_1    = 0x6;
static const int TEX0_2    = 0x7;
static const int CLAMP_1   = 0x8;
static const int CLAMP_2   = 0x9;
static const int FOG       = 0xA;
static const int XYZF3     = 0xC;
static const int XYZ3      = 0xD;
static const int AD        = 0xE;
static const int NOP       = 0xF;

static const string tRegisters[16] = {
    "PRIM", "RGBAQ", "ST", "UV", "XYZF2", "XYZ2", 
    "TEX0_1", "TEX0_2", "CLAMP_1", "CLAMP_2", "FOG",
    "RSRVD", "XYZF3", "XYZ3", "AD", "NOP"
};

static const string tGIFTag[7] = {
    "REGS", "NREG", "FLG", "PRIM", "PRE", "EOP", "NLOOP"
};

static const string tPRIM[7] = {
    "Point", "Line", "Line strip", "Triangle",
    "Triangle strip", "Triangle fan", "Sprite"
};

static const string tIIP[2] = {
    "Flat Shading", "Gouraud Shading"
};

static const string tTME[2] = {
    "Texture mapping OFF", "Texture mapping ON"
};

static const string tFGE[2] = {
    "Fogging OFF", "Fogging ON"
};

static const string tABE[2] = {
    "Alpha Blending OFF", "Alpha Blending ON"
};

static const string tAA1[2] = {
    "Antialiasing OFF", "Antialiasing ON"
};

static const string tFST[2] = {
    "STQ", "UV"
};

static const string tCTXT[2] = {
    "Context 1", "Context 2"
};

static const string tFIX[2] = {
    "Unfixed", "Fixed"
};

// GIF_STAT
static const string tM3R[2] = {
    "Enable ( Initial value )",
    "Disable ( Masked by MR3 flag of GIF_MODE register.)"
};
static const string tM3P[2] = {
    "Enable ( Initial value )",
    "Disable ( Masked by MASKP3 of VIF.)"
};
static const string tIMT[2] = {
    "Continuous transfer mode ( Initial value )",
    "Intermittent transfer mode"
};
static const string tPSE[2] = {
    "Normal ( Initial value )",
    "Temporary stop state by PSE flag of GIF_CTRL register"
};
static const string tIP3[2] = {
    "No interrupted transer via PATH3 (Initial value)",
    "Interrupted transfer via PATH3"
};
static const string tP3Q[2] = {
    "No request to wait for processing in PATH3 (Initial value)",
    "Request to wait for processing in PATH3"
};
static const string tP2Q[2] = {
    "No request to wait for processing in PATH2 (Initial value)",
    "Request to wait for processing in PATH2"
};
static const string tP1Q[2] = {
    "No request to wait for processing in PATH1 (Initial value)",
    "Request to wait for processing in PATH1"
};
static const string tOPH[2] = {
    "Idle state (Initial value)",
    "Outputting data"
};
static const string tAPATH[4] = {
    "Idle state (Initial value)",
    "Transferring data via PATH1",
    "Transferring data via PATH2",
    "Transferring data via PATH3"
};
static const string tDIR[2] = {
    "EE to GS (Initial value)",
    "GS to EE"
};

GIF::GIF() : SubSystem(GIFnREGISTERS) {
}

GIF::GIF(uint32 *data, uint32 size) : SubSystem(GIFnREGISTERS) {
    uint32 i;
    for(i = 0; i < size/16; i++) {
        gifData[i].x = data[i*4+0];
        gifData[i].y = data[i*4+1];
        gifData[i].z = data[i*4+2];
        gifData[i].w = data[i*4+3];
    }
    nloop   = 0;
    eop     = 0;
    nreg    = 0;
    counter = 0;
    curNreg = 0;
    curNreg = 0;
    REGISTERS = new uint32[GIFnREGISTERS];
}

bool
GIF::unpack() {
    unpackNloop();
    unpackNreg();
    unpackFlag();
    unpackPre();
    unpackPrim();
    unpackRegisters();
    counter = counter + 1;
    return validate();
}

void
GIF::unpackRegisters(void) {
	uint32 i;
    uint64 regs = ((uint64)gifData[counter].w<<32)+(uint64)gifData[counter].z;
    for(i = 0; i < 16; i++) {
        registers[i] = 0xf;
    }
    for(i = 0; i < nreg; i++) {
        registers[i] = regs&0xf;
        regs = regs >> 4;
    } 
}

bool
GIF::validate(void) {
    uint32 i;
    if (nloop > 1024) {
        return false;
    }
    for(i = 0; i < 16; i++) {
        if(registers[i] == 0xf) {
            if ( i == nreg ) {
                return true;
            } else {
                return false;
            }
        } 
    }
    return false;
}

vector<string>
GIF::NloopData() {
    string tag;
    vector<string> v;
    uint32 i;
    char x[32], y[32], z[32], w[32];
    if ( nloop == 0 ) {
        return v;
    }

    for(i = 0; i < nreg; i++) {
        v.push_back((string)tRegisters[registers[i]]);
        switch (registers[i]) {
            case PRIM:
                sprintf(x, "0x%x", gifData[counter].x);
                v.push_back((string)x);
                break;
            case RGBAQ:
                sprintf(x, "0x%x, ", gifData[counter].x);
                sprintf(y, "0x%x, ", gifData[counter].y);
                sprintf(z, "0x%x, ", gifData[counter].z);
                sprintf(w, "0x%x", gifData[counter].w);
                v.push_back("R: " + (string)x +
                       "G: " + (string)y +
                       "B: " + (string)x +
                       "A: " + (string)w);
                break;
            case ST:
                sprintf(x, "%ff, ", *((float *)&gifData[counter].x));
                sprintf(y, "%ff, ", *((float *)&gifData[counter].y));
                sprintf(z, "%ff\n", *((float *)&gifData[counter].z));
                v.push_back((string)x +
                        (string)y +
                        (string)z);
                break;
            case UV:
                sprintf(x, "%u, ", (gifData[counter].x>>4));
                sprintf(y, "%u\n", (gifData[counter].y>>4));
                v.push_back("U: " + (string)x +
                       "V: " + (string)y);
                break;
            case XYZF2:
                sprintf(x, "%u, ", (gifData[counter].x>>4));
                sprintf(y, "%u, ", (gifData[counter].y>>4));
                sprintf(z, "%u, ", (gifData[counter].z>>4));
                sprintf(w, "%u", ((gifData[counter].w>>4)&0xff));
                v.push_back("X: " + (string)x +
                       "Y: " + (string)y +
                       "Z: " + (string)z +
                       "F: " + (string)w);
                break;
            case XYZ2:
                sprintf(x, "%u, ", (uint32)((gifData[counter].x)>>4)-xoffset);
                sprintf(y, "%u, ", (uint32)((gifData[counter].y)>>4)-yoffset);
                sprintf(z, "%u", gifData[counter].z);
                v.push_back("X: " + (string)x +
                       "Y: " + (string)y +
                       "Z: " + (string)z);
                break;
            case TEX0_1:
                v.push_back("");
                break;
            case TEX0_2:
                v.push_back("");
                break;
            case CLAMP_1:
                v.push_back("");
                break;
            case CLAMP_2:
                v.push_back("");
                break;
            case FOG:
                sprintf(w, "%u, ", (gifData[counter].w&0xFF));
                v.push_back((string)w);
                break;
            case XYZF3:
                sprintf(x, "%d, ", (gifData[counter].x>>4));
                sprintf(y, "%d, ", (gifData[counter].y>>4));
                sprintf(z, "%u, ", (gifData[counter].z>>4));
                sprintf(w, "%u", ((gifData[counter].w>>4)&0xff));
                v.push_back((string)x + " " +
                        (string)y + " " +
                        (string)z + " " +
                        (string)w);
                break;
            case XYZ3:
                sprintf(x, "%d, ", (gifData[counter].x>>4));
                sprintf(y, "%d, ", (gifData[counter].y>>4));
                sprintf(z, "%u", gifData[counter].z);
                v.push_back((string)x + " " +
                        (string)y + " " +
                        (string)z);
                break;
            case AD:
                v.push_back("");
                break;
            case NOP:
                v.push_back("");
                break;
            default:
                break;
        }
        counter = counter+1;
    }
    curNloop = curNloop - 1;
    return v;
}

string
GIF::getReadableTag() {
    string tag;
    uint32 i;
    char x[32], y[32], z[32], w[32];
    if ( nloop == 0 ) {
        return NULL;
    }

    for(i = 0; i < nreg; i++) {
        tag += (string)tRegisters[registers[i]] + "\t";
        sprintf(x, "%f, ", *((float *)&gifData[counter].x));
        sprintf(y, "%f, ", *((float *)&gifData[counter].y));
        sprintf(z, "%f, ", *((float *)&gifData[counter].z));
        sprintf(w, "%f", *((float *)&gifData[counter].w));
        tag += (string)x + (string)y + (string)z + (string)w + "\n";
        counter = counter+1;
    }
    curNloop = curNloop - 1;
    return tag;
}

string
GIF::RegsAsString(void) {
    string regs;
    uint32 i;
    for(i = 0; i < nreg; i++) {
        regs += (string)tRegisters[registers[i]];
    }
	return regs;
}

uint32
GIF::getNreg(void) {
    return nreg;
}

uint32
GIF::getNloop(void) {
    return nloop;
}

void
GIF::unpackFlag(void) {
    flag = ((gifData[counter].y>>26)&0x3);
}

void
GIF::unpackPrim(void) {
    prim = ((gifData[counter].y>>15)&0x3ff);
}

void
GIF::unpackPre(void) {
    pre = ((gifData[counter].y>>14)&0x1);
}

void
GIF::unpackEop(void) {
    eop = ((gifData[counter].x>>15)&0x1);
}

void
GIF::unpackNloop(void) {
   nloop = (gifData[counter].x&0x7FFF);
   curNloop = nloop;
}

void
GIF::unpackNreg(void) {
    nreg = ((gifData[counter].y>>28));
    curNreg = nreg;
}

vector<string>
GIF::TagAsString(void) {
    string reg;
    vector<string> v;
    char val[100];
    v.push_back(tGIFTag[0]);
	uint32 i;
    // wanker code
    for (i = 15; i > 8; i--) {
        reg += (string)tRegisters[registers[i]] + ", ";
    }
    reg += (string)tRegisters[registers[8]];
    v.push_back(reg);
    reg = "";
    v.push_back(reg);

    for(i = 7; i > 0; i--) {
        reg += (string)tRegisters[registers[i]] + ", ";
    }
    reg += (string)tRegisters[registers[0]];
    v.push_back(reg);

    v.push_back((string)tGIFTag[1]);
    sprintf(val, "%d", nreg);
    v.push_back((string)val);

    v.push_back((string)tGIFTag[2]);
    sprintf(val, "0x%x", flag);
    v.push_back((string)val);

    // prim
    v.push_back((string)tGIFTag[3]);
    // v.push_back("");
    parsePRIM(v);

    v.push_back((string)tGIFTag[4]);
    sprintf(val, "%d", pre);
    v.push_back((string)val);

    v.push_back((string)tGIFTag[5]);
    sprintf(val, "%d", eop);
    v.push_back((string)val);

    v.push_back((string)tGIFTag[6]);
    sprintf(val, "%d", nloop);
    v.push_back((string)val);
    return v;
}

void
GIF::parsePRIM(vector<string> &v) {
    v.push_back((string)tPRIM[prim&0x7]);
    v.push_back("");
    v.push_back(tIIP[(prim&0x8)>>3]);
    v.push_back("");
    v.push_back(tTME[(prim&0x10)>>4]);
    v.push_back("");
    v.push_back((string)tFGE[(prim&0x20)>>5]);
    v.push_back("");
    v.push_back((string)tABE[(prim&0x40)>>6]);
    v.push_back("");
    v.push_back((string)tAA1[(prim&0x80)>>7]);
    v.push_back("");
    v.push_back((string)tFST[(prim&0x100)>>8]);
    v.push_back("");
    v.push_back((string)tCTXT[(prim&0x200)>>9]);
    v.push_back("");
    v.push_back((string)tFIX[(prim&0x400)>>10]);
}

vector<string>
GIF::getRegisterText(const int reg) {
    switch(reg) {
        case GIF_STAT:
            return unpack_stat(reg);
            break;
        case GIF_TAG0:
            return unpack_tag0(reg);
            break;
        case GIF_TAG1:
            return unpack_tag1(reg);
            break;
        case GIF_TAG2:
            return unpack_tag2(reg);
            break;
        case GIF_TAG3:
            return unpack_tag3(reg);
            break;
        case GIF_CNT:
            return unpack_cnt(reg);
            break;
        case GIF_P3CNT:
            return unpack_p3cnt(reg);
            break;
        case GIF_P3TAG:
            return unpack_p3tag(reg);
            break;
        default:
            vector<string> v;
            return v;
            break;
    }
}

vector<string>
GIF::unpack_stat(const int reg) {
    vector<string> v;
    v.push_back("M3R");
    v.push_back(tM3R[(REGISTERS[reg]&0x1)]);
    v.push_back("M3P");
    v.push_back(tM3P[(REGISTERS[reg]&0x2)]);
    v.push_back("IMT");
    v.push_back(tIMT[(REGISTERS[reg]&0x4)]);
    v.push_back("PSE");
    v.push_back(tPSE[(REGISTERS[reg]&0x80)]);
    v.push_back("IP3");
    v.push_back(tIP3[(REGISTERS[reg]&0x20)]);
    v.push_back("P3Q");
    v.push_back(tP3Q[(REGISTERS[reg]&0x40)]);
    v.push_back("P2Q");
    v.push_back(tP2Q[(REGISTERS[reg]&0x80)]);
    v.push_back("P1Q");
    v.push_back(tP1Q[(REGISTERS[reg]&0x100)]);
    v.push_back("OPH");
    v.push_back(tOPH[(REGISTERS[reg]&0x200)]);
    v.push_back("APATH");
    v.push_back(tAPATH[(REGISTERS[reg]&0xC00)]);
    v.push_back("DIR");
    v.push_back(tDIR[(REGISTERS[reg]&0x1000)]);
    char val[100];
    sprintf(val, "%d", (REGISTERS[reg]&0x1F000000)>>24);
    v.push_back("FQC");
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_tag0(const int reg) {
    vector<string> v;
    char val[100];
    sprintf(val, "%d", REGISTERS[reg]&0x00007FFF);
    v.push_back("NLOOP");
    v.push_back(val);
    sprintf(val, "%d", (REGISTERS[reg]&0x00008000)>>15);
    v.push_back("EOP");
    v.push_back(val);
    sprintf(val, "0x%x", (REGISTERS[reg]&0xFFFF0000)>>16);
    v.push_back("tag");
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_tag1(const int reg) {
    vector<string> v;
    char val[100];

    v.push_back("tag");
    sprintf(val, "%d", REGISTERS[reg]&0x00003FFF);
    v.push_back(val);
    v.push_back("PRE");
    sprintf(val, "%d", (REGISTERS[reg]&0x00004000)>>14);
    v.push_back(val);
    v.push_back("PRIM");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x03FF8000)>>15);
    v.push_back(val);
    v.push_back("FLG");
    sprintf(val, "%d", (REGISTERS[reg]&0x0C000000)>>26);
    v.push_back(val);
    v.push_back("NREG");
    sprintf(val, "%d", (REGISTERS[reg]&0xF0000000)>>28);
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_tag2(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("REGS");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_tag3(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("REGS");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_cnt(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("LOOPCNT");
    sprintf(val, "%d", REGISTERS[reg]&0x00007FFF);
    v.push_back(val);
    v.push_back("REGCNT");
    sprintf(val, "%d", (REGISTERS[reg]&0x000F0000)>>16);
    v.push_back(val);
    v.push_back("VUADDR");
    sprintf(val, "%d", (REGISTERS[reg]&0x3FF00000)>>20);
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_p3cnt(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("P3CNT");
    sprintf(val, "%d", REGISTERS[reg]&0x00007FFF);
    v.push_back(val);
    return v;
}
vector<string>
GIF::unpack_p3tag(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("LOOPCNT");
    sprintf(val, "%d", REGISTERS[reg]&0x00007FFF);
    v.push_back(val);
    v.push_back("EOP");
    sprintf(val, "%d", (REGISTERS[reg]&0x00008000)>>15);
    v.push_back(val);
    return v;
}
