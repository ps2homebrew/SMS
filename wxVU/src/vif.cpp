#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "vif.h"

using namespace std;

int limit(int , int );

static const string tMOD[4] = {
    "No addition processing",
    "Offset mode (Row + dV -> VU Mem)",
    "Difference mode (Row + dV -> Row -> VU Mem)",
    "Undefined"
};
static const string tMII[2] = {
    "Unmask (i bit interrupt enable)",
    "Mask (i bit interrupt disable",
};
static const string tME0[2] = {
    "Unmask (stalls when error occurs.)",
    "Mask (ignores DMAtag Mismatch error.)"
};
static const string tME1[2] = {
    "Unmask (stalls when an error occurs.)",
    "Mask (considered as VIFcode NOP.)"
};

// boa constrictor
VIF::VIF(int num) : SubSystem(num) {
    nREGISTERS = num;
	currentCode = VIF_NOP;
	interrupt = false;
	pos = 0;
	vpu = 0;
	rVIF1_R0 = 0;
	rVIF1_R1 = 0;
	rVIF1_R2 = 0;
	rVIF1_R3 = 0;
	rVIF1_C0 = 0;
	rVIF1_C1 = 0;
	rVIF1_C2 = 0;
	rVIF1_C3 = 0;
	rVIF1_CYCLE = 0;
	rVIF1_MASK = 0;
	rVIF1_MODE = 0;
	rVIF1_ITOP = 0;
	rVIF1_ITOPS = 0;
	rVIF1_BASE = 0;
	rVIF1_OFST = 0;
	rVIF1_TOP = 0;
	rVIF1_TOPS = 0;
	rVIF1_MARK = 0;
	rVIF1_NUM = 0;
	rVIF1_CODE = 0;
	rVIF1_STAT = 0;
	rVIF1_FBRST = 0;
	rVIF1_ERR = 0;

	rVIF0_R0 = 0;
	rVIF0_R1 = 0;
	rVIF0_R2 = 0;
	rVIF0_R3 = 0;
	rVIF0_C0 = 0;
	rVIF0_C1 = 0;
	rVIF0_C2 = 0;
	rVIF0_C3 = 0;
	rVIF0_CYCLE = 0;
	rVIF0_MASK = 0;
	rVIF0_MODE = 0;
	rVIF0_ITOP = 0;
	rVIF0_ITOPS = 0;
	rVIF0_MARK = 0;
	rVIF0_NUM = 0;
	rVIF0_CODE = 0;
}

// de struct or
VIF::~VIF() {
}

//
// VIF Register functions
//

//
// VIF commands
//

void
VIF::cmd_nop(void) {
    pos = pos + 4;
}

void
VIF::cmd_stcycl(void) {
    uint8 WL, CL;
}

void
VIF::cmd_offset(void) {
    uint32 offset;
    // offset = data[pos]&0x3ff;
    // VIF1_OFST = offset;
    // clear DBF flag in VIF1_STAT
    // VIF1_BASE = VIF1_TOPS
}

void
VIF::cmd_base(void) {
    // uint32 base = data[pos]&0x1ff;
    // VIF1_BASE = base;
}

void
VIF::cmd_itop(void) {

}

void
VIF::cmd_stmod(void) {
}

void
VIF::get_vifcode(uint32 code) {
	currentCode = (uint32)code>>24;
	interrupt = false;
    switch(currentCode) {
        case VIF_NOP:
            cout << "VIF_NOP" << endl;
            break;
        case VIF_STCYCL:
            cout << "VIF_STCYCL" << endl;
			WL = (code>>8)&0xff;
			CL = (code)&0xff;
			cout << " WL: " << WL << endl;
			cout << " CL: " << CL << endl;
            break;
        case VIF_OFFSET:
            cout << "VIF_OFFSET" << endl;
            break;
        case VIF_BASE:
            cout << "VIF_BASE" << endl;
            break;
        case VIF_ITOP:
            cout << "VIF_ITOP" << endl;
            break;
        case VIF_STMOD:
            cout << "VIF_STMOD" << endl;
            break;
        case VIF_MSKPATH3:
            cout << "VIF_MSKPATH3" << endl;
			if ( (code&0x80) == 1 ) {
				maskpath3 = true;
			} else {
				maskpath3 = false;
			}
            break;
        case VIF_MARK:
            cout << "VIF_MARK" << endl;
			addr = code&0xFFFF;
			if ( vpu == 0 ) {
				rVIF0_MARK = addr;
			} else {
				rVIF1_MARK = addr;
			}
            break;
        case VIF_FLUSHE:
            cout << "VIF_FLUSHE" << endl;
			// end of micro program
            break;
        case VIF_FLUSH:
            cout << "VIF_FLUSH" << endl;
			// end of micro program
			// end of transfer to GIF from PATH1 and PATH2
            break;
        case VIF_FLUSHA:
            cout << "VIF_FLUSHA" << endl;
			// waits no request state from path3
			// end of micro program
			// end of transfer to GIF from PATH1 and PATH2
            break;
        case VIF_MSCAL:
            cout << "VIF_MSCAL" << endl;
			addr = code&0xFFFF;
			// now kick addr
            break;
		case VIF_MSCNT:
            cout << "VIF_MSCNT" << endl;
			// kick another execution after EOMP
			break;
        case VIF_MSCALF:
            cout << "VIF_MSCALF" << endl;
			addr = code&0xFFFF;
			// waits for MP and PATH1/PATH2
            break;
		case VIF_STMASK:
			cout << "VIF_STMASK" << endl;
			if ( vpu == 0 ) {
				rVIF0_MASK = *(data++);
			} else {
				rVIF1_MASK = *(data++);
			}
			pos += 4;
			break;
        case VIF_STROW:
            cout << "VIF_STROW" << endl;
			num = (code>>16)&0xFF;
			addr = code&0xFFFF;
			if ( vpu == 0 ) {
				rVIF0_R0 = *(data++);
				rVIF0_R1 = *(data++);
				rVIF0_R2 = *(data++);
				rVIF0_R3 = *(data++);
			} else {
				rVIF1_R0 = *(data++);
				rVIF1_R1 = *(data++);
				rVIF1_R2 = *(data++);
				rVIF1_R3 = *(data++);
			}
			pos += 16;
            break;
        case VIF_STCOL:
            cout << "VIF_STCOL" << endl;
			num = (code>>16)&0xFF;
			addr = code&0xFFFF;
			if ( vpu == 0 ) {
				rVIF0_C0 = *(data++);
				rVIF0_C1 = *(data++);
				rVIF0_C2 = *(data++);
				rVIF0_C3 = *(data++);
			} else {
				rVIF1_C0 = *(data++);
				rVIF1_C1 = *(data++);
				rVIF1_C2 = *(data++);
				rVIF1_C3 = *(data++);
			}
			pos += 16;
            break;
        case VIF_MPG:
            cout << "VIF_MPG" << endl;
			num = (code>>16)&0xFF;
			addr = code&0xffff;
            break;
		case VIF_DIRECT:
			cout << "VIF_DIRECT" << endl;
			num = code&0xFFFF;
			if (num == 0) {
				num = 65536;
			}
			break;
		case VIF_DIRECTHL:
			cout << "VIF_DIRECTHL" << endl;
			num = code&0xFFFF;
			if (num == 0) {
				num = 65536;
			}
			// this one stalls until path3 is finished
			// let drawGIF get data somehow
			break;
        case VIF_UNPACK:
            cout << "VIF_UNPACK" << endl;
			num = (code>>16)&0xFF;
			flg = (code>>15)&0x1;
			usn = (code>>14)&0x1;
			addr = code&0x3FF;
			if (flg == 1) {
				addr + rVIF1_TOPS;
			}
            break;
    }
}

void
VIF::unpack(void) {
    while(pos<size) {
		vifcode = *(data++);
        get_vifcode(vifcode);
		pos += 4;
    }
}

void
VIF::cmd_unpack(uint32 cmd) {
    uint32 vl = cmd&0x0F;
    uint32 vn = (cmd&0xF0)>>4;
    uint32 offset = 0;
    uint32 S1, S2, S3;
    uint32 X1, X2, X3, Y1, Y2, Y3, Z1, Z2, Z3, W1, W2, W3;
    uint32 RGBA1, RGBA2, RGBA3;
    if ( (vpu = 1) && flg) {
        offset = addr + rVIF1_TOPS;
    } else {
        offset = addr;
    }

    if ( WL <= CL ) {
        1+(((32>>vl)*(vn+1))*num/32);
    } else {
        uint32 n = CL*(num/WL)+limit(num%WL,CL);
        1+(((32>>vl)*(vn+1))*n/32);
    }

    switch((vn<<4)+vl) {
        case UNPACK_S32:
            S1 = *(data++);
            S2 = *(data++);
            S3 = *(data++);
            vumem[offset+0] = S1;  
            vumem[offset+1] = S1;  
            vumem[offset+2] = S1;  
            vumem[offset+3] = S1;  

            vumem[offset+5] = S2;
            vumem[offset+6] = S2;
            vumem[offset+7] = S2;
            vumem[offset+8] = S2;

            vumem[offset+9] = S3;  
            vumem[offset+10] = S3;  
            vumem[offset+11] = S3;  
            vumem[offset+12] = S3;  
            offset += 12;
            break;
        case UNPACK_S16:
            S1 = *(data++);
            S2 = (S1>>16);
            S1 = S1&0xFFFF;
            S3 = *(data++);

            vumem[offset+0] = S1;  
            vumem[offset+1] = S1;  
            vumem[offset+2] = S1;  
            vumem[offset+3] = S1;  

            vumem[offset+5] = S2;
            vumem[offset+6] = S2;
            vumem[offset+7] = S2;
            vumem[offset+8] = S2;

            vumem[offset+9] = S3;  
            vumem[offset+10] = S3;  
            vumem[offset+11] = S3;  
            vumem[offset+12] = S3;  
            offset += 12;
            break;
        case UNPACK_S8:
            S1 = *(data++);
            S2 = (S1>>8)&0xFF;
            S3 = (S1>>16)&0xFF;
            S1 = S1&0xFF;

            // fix USN
            vumem[offset+0] = S1;  
            vumem[offset+1] = S1;  
            vumem[offset+2] = S1;  
            vumem[offset+3] = S1;  

            vumem[offset+5] = S2;
            vumem[offset+6] = S2;
            vumem[offset+7] = S2;
            vumem[offset+8] = S2;

            vumem[offset+9] = S3;  
            vumem[offset+10] = S3;  
            vumem[offset+11] = S3;  
            vumem[offset+12] = S3;  
            offset += 12;
            break;
        case UNPACK_V232:
            X1 = *(data++);
            Y1 = *(data++);
            X2 = *(data++);
            Y2 = *(data++);
            X3 = *(data++);
            Y3 = *(data++);

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;

            vumem[offset+9] = X3;
            vumem[offset+10] = Y3;
            offset += 12;
            break;
        case UNPACK_V216:
            X1 = *(data++);
            X2 = *(data++);
            X3 = *(data++);

            Y1 = (X1>>16);
            Y2 = (X2>>16);
            Y3 = (X3>>16);

            X1 = X1&0xFFFF;
            X2 = X2&0xFFFF;
            X3 = X3&0xFFFF;

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;

            vumem[offset+9] = X3;
            vumem[offset+10] = Y3;
            offset += 12;
            break;
        case UNPACK_V28:
            X1 = *(data++);
            X3 = *(data++);

            Y1 = (X1>>8)&0xFF;
            X2 = (X1>>16)&0xFF;
            Y2 = (X2>>24)&0xFF;
            X1 = X1&0xFF;

            Y3 = (X3>>8)&0xFF;
            X3 = X3&0xFF;

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;

            vumem[offset+9] = X3;
            vumem[offset+10] = Y3;
            offset += 12;
            break;
        case UNPACK_V332:
            X1 = *(data++);
            Y1 = *(data++);
            Z1 = *(data++);
            X2 = *(data++);
            Y2 = *(data++);
            Z2 = *(data++);
            Y3 = *(data++);
            Z3 = *(data++);
            X3 = *(data++);

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;
            vumem[offset+2] = Z1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;

            vumem[offset+9] = X3;  
            vumem[offset+10] = Y3;  
            vumem[offset+11] = Z3;  
            offset += 12;
            break;
        case UNPACK_V316:
            X1 = *(data++);
            Z1 = *(data++);
            Y2 = *(data++);
            X3 = *(data++);
            Z3 = *(data++);

            Y1 = (X1>>16)&0xFFFF;
            X2 = (Z1>>16)&0xFFFF;
            Z2 = (Y2>>16)&0xFFFF;
            Y3 = (X3>>16)&0xFFFF;
            Z3 = Z3&0xFFFF;
            X1 = X1&0xFFFF;
            Z1 = Z1&0xFFFF;
            Y2 = Y2&0xFFFF;
            X3 = X3&0xFFFF;
            Z3 = Z3&0xFFFF;
            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;
            vumem[offset+2] = Z1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;

            vumem[offset+9] = X3;  
            vumem[offset+10] = Y3;  
            vumem[offset+11] = Z3;  
            offset += 12;
            break;
        case UNPACK_V38:
            X1 = (*(data))&0xFF;
            Y1 = ((*(data))>>8)&0xFF;
            Z1 = ((*(data))>>16)&0xFF;
            X2 = ((*(data++))>>24)&0xFF;
            Y2 = (*(data))&0xFF;
            Z2 = ((*(data))>>8)&0xFF;
            X3 = ((*(data))>>16)&0xFF;
            Y3 = ((*(data++))>>24)&0xFF;
            Z3 = (*(data))&0xFF;

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;
            vumem[offset+2] = X1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;

            vumem[offset+9] = X3;  
            vumem[offset+10] = Y3;  
            vumem[offset+11] = Z3;  
            offset += 12;
            break;

        case UNPACK_V432:
            X1 = *(data++);
            Y1 = *(data++);
            Z1 = *(data++);
            W1 = *(data++);
            X2 = *(data++);
            Y2 = *(data++);
            Z2 = *(data++);
            W2 = *(data++);
            Y3 = *(data++);
            Z3 = *(data++);
            X3 = *(data++);
            W3 = *(data++);

            vumem[offset+0] = X1;
            vumem[offset+1] = Y2;
            vumem[offset+2] = Z3;
            vumem[offset+3] = W3;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;
            vumem[offset+8] = W2;

            vumem[offset+9] = X3;  
            vumem[offset+10] = Y3;  
            vumem[offset+11] = Z3;  
            vumem[offset+12] = W3;  
            offset += 12;
            break;

        case UNPACK_V416:
            X1 = (*(data))&0xFFFF;
            Y1 = (*(data++)>>16)&0xFFFF;
            Z1 = *(data)&0xFFFF;
            W1 = (*(data++)>>16)&0xFFFF;
            X2 = *(data)&0xFFFF;
            Y2 = (*(data++)>>16)&0xFFFF;
            Z2 = *(data)&0xFFFF;
            W2 = (*(data++)>>16)&0xFFFF;
            X3 = *(data)&0xFFFF;
            Y3 = (*(data++)>>16)&0xFFFF;
            Z3 = *(data)&0xFFFF;
            W3 = (*(data++)>>16)&0xFFFF;

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;
            vumem[offset+2] = Z1;
            vumem[offset+4] = W1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;
            vumem[offset+8] = W2;

            vumem[offset+9] = Z3;  
            vumem[offset+10] = Z3;  
            vumem[offset+11] = Z3;  
            vumem[offset+12] = W3;  
            offset += 12;

            break;
        case UNPACK_V48:
            X1 = (*(data))&0xFF;
            Y1 = (*(data)>>8)&0xFF;
            Z1 = (*(data)>>16)&0xFF;
            W1 = (*(data++)>>24)&0xFF;
            X2 = *(data)&0xFF;
            Y2 = (*(data)>>8)&0xFF;
            Z2 = (*(data)>>16)&0xFF;
            W2 = (*(data++)>>24)&0xFF;
            X3 = *(data)&0xFF;
            Y3 = (*(data)>>8)&0xFF;
            Z3 = (*(data)>>16)&0xFF;
            W3 = (*(data++)>>24)&0xFF;

            vumem[offset+0] = X1;
            vumem[offset+1] = Y1;
            vumem[offset+2] = Z1;
            vumem[offset+3] = W1;

            vumem[offset+5] = X2;
            vumem[offset+6] = Y2;
            vumem[offset+7] = Z2;
            vumem[offset+8] = W2;

            vumem[offset+9] = X3;  
            vumem[offset+10] = Y3;  
            vumem[offset+11] = Z3;  
            vumem[offset+12] = W3;  
            offset += 12;

            break;
        case UNPACK_V45:
            RGBA1 = *(data)&0xFFFF;
            RGBA2 = (*(data++)>>16)&0xFFFF;
            RGBA3 = *(data++)&0xFFFF;

            vumem[offset+0] = (RGBA1&0x1F)<<4;
            vumem[offset+1] = ((RGBA1>>5)&0x1F)<<4;
            vumem[offset+2] = ((RGBA1>>9)&0x1F)<<4;
            vumem[offset+3] = ((RGBA1>>14)&0x1)<<8;

            vumem[offset+5] = (RGBA2&0x1F)<<4;
            vumem[offset+6] = ((RGBA2>>5)&0x1F)<<4;
            vumem[offset+7] = ((RGBA2>>9)&0x1F)<<4;
            vumem[offset+8] = ((RGBA2>>14)&0x1)<<8;

            vumem[offset+9] = (RGBA3&0x1F)<<4;
            vumem[offset+10] = ((RGBA3>>5)&0x1F)<<4;
            vumem[offset+11] = ((RGBA3>>9)&0x1F)<<4;
            vumem[offset+12] = ((RGBA3>>14)&0x1)<<8;

            offset += 12;

            break;
    }
}

int
limit(int a, int max) { return(a > max ? max: a);}

// vif0/vif1 common register unpack functions
vector<string>
VIF::unpack_VIF_R(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("R");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}
vector<string>
VIF::unpack_VIF_C(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("C");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}

vector<string>
VIF::unpack_VIF_ITOPS(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ITOPS");
    sprintf(val, "0x%x", REGISTERS[reg]&0x3FF);
    v.push_back(val);
    return v;
}
vector<string>
VIF::unpack_VIF_ITOP(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ITOP");
    sprintf(val, "0x%x", REGISTERS[reg]&0x3FF);
    v.push_back(val);
    return v;
}

vector<string>
VIF::unpack_VIF_CYCLE(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("CL");
    sprintf(val, "0x%x", REGISTERS[reg]&0xFF);
    v.push_back(val);
    v.push_back("WL");
    sprintf(val, "0x%x", (REGISTERS[reg]&0xFF00)>>8);
    v.push_back(val);
    return v;
}
vector<string>
VIF::unpack_VIF_MODE(const int reg) {
    vector<string> v;
    v.push_back("MODE");
    v.push_back(tMOD[REGISTERS[reg]&0x3]);
    return v;
}
vector<string>
VIF::unpack_VIF_ERR(const int reg) {
    vector<string> v;
    v.push_back("MII");
    v.push_back(tMII[REGISTERS[reg]&0x1]);
    v.push_back("ME0");
    v.push_back(tME0[(REGISTERS[reg]&0x2)>>1]);
    v.push_back("ME1");
    v.push_back(tME1[(REGISTERS[reg]&0x4)>>2]);
    return v;
}
vector<string>
VIF::unpack_VIF_MARK(const int reg) {
    vector<string> v;
    return v;
}
vector<string>
VIF::unpack_VIF_NUM(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
    return v;
}
vector<string>
VIF::unpack_VIF_MASK(const int reg) {
    vector<string> v;
    return v;
}
vector<string>
VIF::unpack_VIF_CODE(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("IMMEDIATE");
    sprintf(val, "0x%x", REGISTERS[reg]&0x0000ffff);
    v.push_back(val);
    v.push_back("NUM");
    sprintf(val, "0x%x", (REGISTERS[reg]&0x00ff0000)>>16);
    v.push_back(val);
    v.push_back("CMD");
    sprintf(val, "0x%x", (REGISTERS[reg]&0xff000000)>>24);
    v.push_back(val);
    return v;
}
