#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "vif.h"
#include "vu.h"
#include "parser.h"


using namespace std;

extern VU VUchip;

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

void
VIF::clearRegisters(void) {
	interrupt = false;
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
    _num = 0;
    _cmd = NULL;
    _WL = 0;
    _CL = 0;
    _memIndex = 0;
    _codeIndex = 0;

}

// boa constrictor
VIF::VIF(int num) : SubSystem(num) {
    nREGISTERS = num;
    clearRegisters();
}

VIF::VIF(const char *filename) : SubSystem(0), _fin(filename) {
    clearRegisters();
}

VIF::VIF(const char *filename, int num)
    : SubSystem(num), _fin(filename) {
    nREGISTERS = num;
    clearRegisters();
}

// de struct or
VIF::~VIF() {
}

//
// VIF Register functions
//

// ------------------------------------------------------------------
// VIF commands
void
VIF::cmd_nop(void) {
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
VIF::cmd_stcol(void) {
    char raw[4];
    int32 data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C0 = data;
    rVIF1_C0 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C1 = data;
    rVIF1_C1 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C2 = data;
    rVIF1_C2 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C3 = data;
    rVIF1_C3 = data;
}

void
VIF::cmd_strow(void) {
    char raw[4];
    uint32 data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_R0 = data;
    rVIF1_R0 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_R1 = data;
    rVIF1_R1 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_R2 = data;
    rVIF1_R2 = data;
    _fin.read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_R3 = data;
    rVIF1_R3 = data;
}

void
VIF::cmd_mpg(void) {
    char    raw[4];
    uint32  lower;
    uint32  upper;
    uint32  index;
    char    uppline[64];
    char    lowline[64];
    char    uparam[64];
    char    lparam[64];
    index = _addr;
    while ( _num > 0 ) {
        _fin.read(raw, 4);
        lower = ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            ((unsigned char)raw[1]<<8)+
            (unsigned char)raw[0];
        _fin.read(raw, 4);
        upper = ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            (((unsigned char)raw[1])<<8)+
            ((unsigned char)raw[0]);
        dlower(&lower, lowline, lparam );
        dupper(&upper, uppline, uparam );
        insert(uppline, lowline, uparam, lparam, index);
        index++;
        _num--;
    }

    if ( _num == 0 ) {
        _cmd = NULL;
    }
}

// ------------------------------------------------------------------
bool
VIF::eof() {
    return _fin.eof();
}

bool
VIF::valid() {
    return true;
}

uint32
VIF::read() {
    char    raw[4];
    uint32  data;
    if (_num == 0) {
        decode_cmd();
    } else {
        _fin.read(raw, 4);
        _num--;
        if ( _num == 0 ) {
            _cmd = NULL;
        }
    }
    return data;
}

uint32
VIF::cmd() {
    return _cmd;
}

void
VIF::decode_cmd(void) {
    uint32 data;
    char raw[4];
    uint32 cmd;
    _fin.read(raw, 4);
    data = (raw[3]<<24) +
        (raw[2]<<16) +
        (raw[1]<<8) +
        raw[0];
    if (_fin.eof()) {
        return;
    }
    cmd = data>>24;
    uint32 vl, vn;
    switch(cmd) {
        case VIF_NOP:
            _cmd = VIF_NOP;
            _num = 0;
            cout << "VIF_NOP" << endl;
            break;
        case VIF_STCYCL:
            _cmd = VIF_STCYCL;
            _num = 0;
            _WL = (data>>8)&0xff;
            _CL = data&0xff;
            cout << "VIF_STCYCL: ";
            cout << "WL: " << _WL << ", CL: " << _CL << endl;
            break;
        case VIF_OFFSET:
            _cmd = VIF_OFFSET;
            _num = 0;
            rVIF1_OFST = data&0x3ff;
            // clear DBF flag
            rVIF1_STAT = (rVIF1_STAT&0x1f803f4f);
            rVIF1_TOPS = rVIF1_BASE;
            break;
        case VIF_BASE:
            _cmd = VIF_BASE;
            _num = 0;
            rVIF1_BASE = data&0x3ff;
            cout << "VIF_BASE: " << rVIF1_BASE << endl;
            break;
        case VIF_ITOP:
            _cmd = VIF_ITOP;
            _num = 0;
            rVIF0_ITOPS = data&0x3ff;
            rVIF1_ITOPS = data&0x3ff;
            break;
        case VIF_STMOD:
            _cmd = VIF_STMOD;
            _num = 0;
            rVIF0_MODE = data&0x3;
            rVIF1_MODE = data&0x3;
            break;
        case VIF_MSKPATH3:
            _cmd = VIF_MSKPATH3;
            _num = 0;
            if ( (data&0x8000) == 0x8000 ) {
                _maskpath3 = true;
            } else {
                _maskpath3 = false;
            }
            break;
        case VIF_MARK:
            _cmd = VIF_MARK;
            _num = 0;
            rVIF0_MARK = data&0xFFFF;
            rVIF1_MARK = data&0xFFFF;
            break;
        case VIF_FLUSHE:
            _cmd = VIF_FLUSHE;
            _num = 0;
            cout << "VIF_FLUSHE" << endl;
			// end of micro program
            break;
        case VIF_FLUSH:
            _cmd = VIF_FLUSH;
            _num = 0;
            cout << "VIF_FLUSH" << endl;
			// end of micro program
			// end of transfer to GIF from PATH1 and PATH2
            break;
        case VIF_FLUSHA:
            _cmd = VIF_FLUSHA;
            cout << "VIF_FLUSHA" << endl;
			// waits no request state from path3
			// end of micro program
			// end of transfer to GIF from PATH1 and PATH2
            break;
        case VIF_MSCAL:
            _cmd = VIF_MSCAL;
            cout << "VIF_MSCAL" << endl;
            _addr = data&0xFFFF;
            break;
		case VIF_MSCNT:
            _cmd = VIF_MSCNT;
            cout << "VIF_MSCNT" << endl;
			break;
        case VIF_MSCALF:
            _cmd = VIF_MSCALF;
            cout << "VIF_MSCALF" << endl;
            _addr = data&0xFFFF;
            break;
		case VIF_STMASK:
			cout << "VIF_STMASK" << endl;
            _cmd = VIF_STMASK;
            rVIF0_MASK = data;
            rVIF1_MASK = data;
			break;
        case VIF_STROW:
            cout << "VIF_STROW" << endl;
            cmd_strow();
            break;
        case VIF_STCOL:
            cmd_stcol();
            break;
        case VIF_MPG:
            cout << "VIF_MPG" << endl;
            _cmd = VIF_MPG;
            _num = (data>>16)&0xFF;
            _addr = data&0xffff;
            if ( _num == 0 ) {
                _num = 256;
            }
            cmd_mpg();
            break;
		case VIF_DIRECT:
            // should be redirected to gsexec directly
			cout << "VIF_DIRECT" << endl;
            // num = code&0xFFFF;
            // if (num == 0) {
            //     num = 65536;
            // }
			break;
		case VIF_DIRECTHL:
            // should be redirected to gsexec directly
			cout << "VIF_DIRECTHL" << endl;
            // num = code&0xFFFF;
            // if (num == 0) {
            //     num = 65536;
            // }
			break;
    }

    if ( (cmd&VIF_UNPACK) == VIF_UNPACK) {
        _memIndex = 0;
        cout << "VIF_UNPACK: ";
        _cmd = VIF_UNPACK;
        _num = (data>>16)&0xFF;
        _unpack = (data>>24)&0xF;
        _addr = (data)&0x3FF;
        _usn = (data>>14)&0x1;
        _flg = (data>>15)&0x1;
        vl = (data>>24)&3;
        vn = (data>>26)&3;

        if ( _WL <= _CL ) {
            _length = 1+(((32>>vl)*(vn+1))*_num/32);
        } else {
            uint32 n = _CL*(_num/_WL)+limit(_num%_WL,_CL);
            _length = 1+(((32>>vl)*(vn+1))*n/32);
        }

        _unpack = (vn<<2)+vl;
        cout << "length: " << _length << ", ";
        cout << "usn: " << _usn << ", ";
        cout << "flg: " << _flg << endl;
        if ( _flg == 1 ) {
            _memIndex = rVIF1_TOPS; 
        }
        _memIndex += _addr;
        cmd_unpack();
    }
}

uint32
VIF::cmd_unpack(void) {
    char    rword[4], rhword[2], rbyte[1];
    uint32  word_x, word_y, word_z, word_w;
    uint16  hword_x, hword_y, hword_z, hword_w;
    uint8   byte_x, byte_y, byte_z, byte_w;
    uint32  _cycle = 0;
    uint32  _write = 0;
    uint32  _cnt = _num;
    for (;_num > 0; _num--) {
        if( _CL > _WL ) {
            if ( _write >= _WL ) {
                _memIndex += _CL-_WL;
                _cycle = 0;
                _write = 0;
            }
        } else if ( _CL > _WL ) {
            if ( _write >= _CL ) {
                for(_cnt = 0; _cnt < (_CL-_WL); _cnt++) {
                    VUchip.dataMem[_memIndex].x = rVIF1_C0; 
                    VUchip.dataMem[_memIndex].y = rVIF1_C1; 
                    VUchip.dataMem[_memIndex].z = rVIF1_C2; 
                    VUchip.dataMem[_memIndex].w = rVIF1_C3; 
                    _memIndex++;
                }
                _cycle = 0;
                _write = 0;
            }
        }
        switch(_unpack) {
            case UNPACK_S32:
                cout << "unpacking S32" << endl;
                _fin.read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    VUchip.dataMem[_memIndex].x = word_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = word_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = word_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = word_x+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    VUchip.dataMem[_memIndex].x = word_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = word_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = word_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = word_x+rVIF1_R3;
                    rVIF1_R0 = word_x+rVIF1_R0;
                    rVIF1_R1 = word_x+rVIF1_R1;
                    rVIF1_R2 = word_x+rVIF1_R2;
                    rVIF1_R3 = word_x+rVIF1_R3;
                } else {
                    VUchip.dataMem[_memIndex].x = word_x;
                    VUchip.dataMem[_memIndex].y = word_x;
                    VUchip.dataMem[_memIndex].z = word_x;
                    VUchip.dataMem[_memIndex].w = word_x;
                }
                break;
            case UNPACK_S16:
                cout << "unpacking S16" << endl;
                _fin.read(rhword, 2);
                hword_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    VUchip.dataMem[_memIndex].x = hword_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = hword_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = hword_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = hword_x+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    VUchip.dataMem[_memIndex].x = hword_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = hword_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = hword_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = hword_x+rVIF1_R3;
                    rVIF1_R0 = hword_x+rVIF1_R0;
                    rVIF1_R1 = hword_x+rVIF1_R1;
                    rVIF1_R2 = hword_x+rVIF1_R2;
                    rVIF1_R3 = hword_x+rVIF1_R3;
                } else {
                    VUchip.dataMem[_memIndex].x = hword_x;
                    VUchip.dataMem[_memIndex].y = hword_x;
                    VUchip.dataMem[_memIndex].z = hword_x;
                    VUchip.dataMem[_memIndex].w = hword_x;
                }
                break;
            case UNPACK_S8:
                cout << "unpacking S8" << endl;
                _fin.read(rbyte, 1);
                byte_x = (unsigned char)rbyte[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    VUchip.dataMem[_memIndex].x = byte_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = byte_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = byte_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = byte_x+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    VUchip.dataMem[_memIndex].x = byte_x+rVIF1_R0;
                    VUchip.dataMem[_memIndex].y = byte_x+rVIF1_R1;
                    VUchip.dataMem[_memIndex].z = byte_x+rVIF1_R2;
                    VUchip.dataMem[_memIndex].w = byte_x+rVIF1_R3;
                    rVIF1_R0 = byte_x+rVIF1_R0;
                    rVIF1_R1 = byte_x+rVIF1_R1;
                    rVIF1_R2 = byte_x+rVIF1_R2;
                    rVIF1_R3 = byte_x+rVIF1_R3;
                } else {
                    VUchip.dataMem[_memIndex].x = byte_x;
                    VUchip.dataMem[_memIndex].y = byte_x;
                    VUchip.dataMem[_memIndex].z = byte_x;
                    VUchip.dataMem[_memIndex].w = byte_x;
                }
                break;
            case UNPACK_V232:
                cout << "unpacking V2_32" << endl;
                _fin.read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_y = (rword[3]<<24) +
                    (rword[2]<<16) +
                    (rword[1]<<8) +
                    rword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                break;
            case UNPACK_V216:
                cout << "unpacking V2_16" << endl;
                _fin.read(rword, 2);
                hword_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rword, 2);
                hword_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    word_x = hword_x+rVIF1_R0;
                    word_y = hword_y+rVIF1_R1;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = hword_x+rVIF1_R0;
                    word_y = hword_y+rVIF1_R1;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                } else {
                    word_x = hword_x;
                    word_y = hword_y;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                break;
            case UNPACK_V28:
                cout << "unpacking V2_8" << endl;
                _fin.read(rbyte, 1);
                byte_x = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_y = (unsigned char)rbyte[0];

                if ( rVIF1_MODE == MODE_ADD ) {
                    byte_x = byte_x+rVIF1_R0;
                    byte_y = byte_y+rVIF1_R1;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = byte_x+rVIF1_R0;
                    word_y = byte_y+rVIF1_R1;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                } else {
                    word_x = byte_x;
                    word_y = byte_y;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                break;
            case UNPACK_V332:
                cout << "unpacking V3_32" << endl;
                _fin.read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_y = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_z = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                    word_z = word_z+rVIF1_R2;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                    word_z = word_z+rVIF1_R2;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                    rVIF1_R2 = word_z;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                VUchip.dataMem[_memIndex].z = word_z;
                break;
            case UNPACK_V316:
                cout << "unpacking V3_16" << endl;
                _fin.read(rhword, 2);
                hword_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rhword, 2);
                hword_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rhword, 2);
                hword_z = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    hword_x = hword_x+rVIF1_R0;
                    hword_y = hword_y+rVIF1_R1;
                    hword_z = hword_z+rVIF1_R2;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = hword_x+rVIF1_R0;
                    word_y = hword_y+rVIF1_R1;
                    word_z = hword_z+rVIF1_R2;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                    rVIF1_R2 = word_z;
                } else {
                    word_x = hword_x;
                    word_y = hword_y;
                    word_z = hword_z;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                VUchip.dataMem[_memIndex].z = word_z;
                break;
            case UNPACK_V38:
                cout << "unpacking V3_8" << endl;
                _fin.read(rbyte, 1);
                byte_x = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_y = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_z = (unsigned char)rbyte[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    byte_x = byte_x+rVIF1_R0;
                    byte_y = byte_y+rVIF1_R1;
                    byte_z = byte_z+rVIF1_R2;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = byte_x+rVIF1_R0;
                    word_y = byte_y+rVIF1_R1;
                    word_z = byte_z+rVIF1_R2;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                    rVIF1_R2 = word_z;
                } else {
                    word_x = byte_x;
                    word_y = byte_y;
                    word_z = byte_z;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                VUchip.dataMem[_memIndex].z = word_z;
                break;
            case UNPACK_V432:
                cout << "unpacking V4_32" << endl;
                _fin.read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_y = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_z = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                _fin.read(rword, 4);
                word_w = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                    word_z = word_z+rVIF1_R2;
                    word_w = word_w+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = word_x+rVIF1_R0;
                    word_y = word_y+rVIF1_R1;
                    word_z = word_z+rVIF1_R2;
                    word_w = word_w+rVIF1_R3;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                    rVIF1_R2 = word_z;
                    rVIF1_R3 = word_w;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                VUchip.dataMem[_memIndex].z = word_z;
                VUchip.dataMem[_memIndex].w = word_w;
                break;
            case UNPACK_V416:
                _fin.read(rhword, 2);
                hword_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rhword, 2);
                hword_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rhword, 2);
                hword_z = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                _fin.read(rhword, 2);
                hword_w = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    hword_x = hword_x+rVIF1_R0;
                    hword_y = hword_y+rVIF1_R1;
                    hword_z = hword_z+rVIF1_R2;
                    hword_w = hword_w+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    hword_x = hword_x+rVIF1_R0;
                    hword_y = hword_y+rVIF1_R1;
                    hword_z = hword_z+rVIF1_R2;
                    hword_w = hword_w+rVIF1_R3;
                    rVIF1_R0 = hword_x;
                    rVIF1_R1 = hword_y;
                    rVIF1_R2 = hword_z;
                    rVIF1_R3 = hword_w;
                }
                VUchip.dataMem[_memIndex].x = hword_x;
                VUchip.dataMem[_memIndex].y = hword_y;
                VUchip.dataMem[_memIndex].z = hword_z;
                VUchip.dataMem[_memIndex].w = hword_w;
                break;
            case UNPACK_V48:
                _fin.read(rbyte, 1);
                byte_x = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_y = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_z = (unsigned char)rbyte[0];
                _fin.read(rbyte, 1);
                byte_w = (unsigned char)rbyte[0];
                if ( rVIF1_MODE == MODE_ADD ) {
                    word_x = byte_x+rVIF1_R0;
                    word_y = byte_y+rVIF1_R1;
                    word_z = byte_z+rVIF1_R2;
                    word_w = byte_w+rVIF1_R3;
                } else if ( rVIF1_MODE == MODE_ADDROW ) {
                    word_x = byte_x+rVIF1_R0;
                    word_y = byte_y+rVIF1_R1;
                    word_z = byte_z+rVIF1_R2;
                    word_w = byte_w+rVIF1_R3;
                    rVIF1_R0 = word_x;
                    rVIF1_R1 = word_y;
                    rVIF1_R2 = word_z;
                    rVIF1_R3 = word_w;
                }
                VUchip.dataMem[_memIndex].x = word_x;
                VUchip.dataMem[_memIndex].y = word_y;
                VUchip.dataMem[_memIndex].z = word_z;
                VUchip.dataMem[_memIndex].w = word_w;
                break;
            case UNPACK_V45:
                break;
            default:
                break;
        }
        _memIndex++;
        _cycle++;
        _write++;
    }
    return 0;
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
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
    return v;
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
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
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

// write functions
void
VIF::writeCode(void) {
}

void
VIF::writeData(void) {
}

void
VIF::writeRegister(void) {
}
