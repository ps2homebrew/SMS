#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include "vif.h"
#include "parser.h"
#include "vu.h"
#include "debug.h"

using namespace std;


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

static const wxString tUNPACK_MODES[16] = {
    "S32\n",
    "S16\n",
    "S8\n",
    "",
    "V2_32\n",
    "V2_16\n",
    "V2_8\n",
    "",
    "V3_32\n",
    "V3_16\n",
    "V3_8\n",
    "",
    "V4_32\n",
    "V4_16\n",
    "V4_8\n",
    "V4_5\n"
};

/////////////////////////////// PUBLIC ///////////////////////////////////////
int limit(int a, int max) { return(a > max ? max: a);}

Vif::Vif() : SubSystem(0) {
    Init();
}

Vif::Vif(int num) : SubSystem(num) {
    nREGISTERS = num;
    Init();
}

Vif::Vif(const char *filename, Parser* pParser, Vu* pVu) : SubSystem(0) {
    m_fin->open(filename);
    m_fin->seekg(0, ios::end);
    m_fileLength = m_fin->tellg();
    m_fin->seekg(0, ios::beg);

    m_pParser = pParser;
    m_pVu = pVu;
    Init();
}

Vif::Vif(const char *filename, int num) : SubSystem(num) {
    m_fin->open(filename);
    nREGISTERS = num;
    Init();
}

// de struct or
Vif::~Vif() {
}

const int32
Vif::Open(const char* filename) {
    m_fin = new ifstream(filename, ios::binary);
    return E_OK;
}

const int32
Vif::Close() {
    delete m_fin;
    return E_OK;
}

/////////////////////////////// PRIVATE    ///////////////////////////////////
void
Vif::Init(void) {
	m_interrupt = false;
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
    m_vifCmdNum = 0;
    m_vifCmd = NULL;
    _WL = 0;
    _CL = 0;
    m_memIndex = 0;
    m_codeIndex = 0;
}

// ------------------------------------------------------------------
// VIF commands
const int32
Vif::CmdNop(void) {
    m_vifCmd = VIF_NOP;
    if ( m_trace ) {
        m_pLog->Trace(1, "NOP\n");
    }
    return E_OK;
}

const int32
Vif::CmdStcycl(const int32& data) {
    m_vifCmd = VIF_STCYCL;
    _WL = (data>>8)&0xff;
    _CL = data&0xff;
    if ( m_trace ) {
        m_pLog->Trace(1, "STCYCL\n");
        m_pLog->Trace(2, wxString::Format("WL: %d, CL: %d\n", _WL, _CL));
    }
    return E_OK;
}

const int32
Vif::CmdItop(const int32& data) {
    if ( m_trace ) {
        m_pLog->Trace(1, "ITOP\n");
    }
    m_vifCmd = VIF_ITOP;
    m_vifCmdNum = 0;
    rVIF0_ITOPS = data&0x3ff;
    rVIF1_ITOPS = data&0x3ff;
    return E_OK;
}

const int32
Vif::CmdStmod(const int32& data) {
    if ( m_trace ) {
        m_pLog->Trace(1, "STMOD\n");
    }
    m_vifCmd = VIF_STMOD;
    rVIF0_MODE = data&0x3;
    rVIF1_MODE = data&0x3;
    return E_OK;
}

const int32
Vif::CmdMark(const int32& data) {
    if ( m_trace ) {
        m_pLog->Trace(1, "MARK\n");
    }
    m_vifCmd = VIF_MARK;
    rVIF0_MARK = data&0xFFFF;
    rVIF1_MARK = data&0xFFFF;
    return E_OK;
}

const int32
Vif::CmdFlushE(void) {
    // TODO
    m_vifCmd = VIF_FLUSHE;
    if ( m_trace ) {
        m_pLog->Trace(1, "FLUSHE\n");
    }
    return E_OK;
}

const int32
Vif::CmdMpg(const int32& data) {
    char    raw[4];
    uint32  lower;
    uint32  upper;
    uint32  index;
    char    uppline[64];
    char    lowline[64];
    char    uparam[64];
    char    lparam[64];

    m_vifCmd = VIF_MPG;
    m_vifCmdNum = (data>>16)&0xFF;
    _addr = data&0xffff;
    index = _addr;
    if ( m_vifCmdNum == 0 ) {
        m_vifCmdNum = 256;
    }
    if ( m_trace ) {
        m_pLog->Trace(1, wxString::Format("MPG, Num = %d, Load addr = %d\n",
                m_vifCmdNum, _addr));
    }

    while ( m_vifCmdNum > 0 ) {
        m_fin->read(raw, 4);
        lower = ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            ((unsigned char)raw[1]<<8)+
            (unsigned char)raw[0];
        m_fin->read(raw, 4);
        upper = ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            (((unsigned char)raw[1])<<8)+
            ((unsigned char)raw[0]);
        m_pParser->dlower(&lower, lowline, lparam );
        m_pParser->dupper(&upper, uppline, uparam );
        m_pParser->insert(uppline, lowline, uparam, lparam, index);
        index++;
        m_vifCmdNum--;
    }

    if ( m_vifCmdNum == 0 ) {
        m_vifCmd = NULL;
    }
    return E_OK;
}

// --------------------------------------------------------------------------
// function to extract VIF UNPACK data
const int32
Vif::CmdUnpack(void) {
    char    rword[4], rhword[2], rbyte[1];
    uint32  word_x, word_y, word_z, word_w;
    for (;m_vifCmdNum > 0; m_vifCmdNum--) {
        if (m_fin->peek() == EOF) {
            m_pLog->Warning("Premature EOF\n");
            return E_FILE_EOF;
        }
        uint32 cycle;
        uint32 write = 0;
        if( _CL > _WL ) {
            if ( write >= _WL ) {
                m_memIndex += _CL-_WL;
                cycle = 0;
                write = 0;
            }
        } else if ( _CL > _WL ) {
            if ( write >= _CL ) {
                for(uint32 cnt = 0; cnt < (_CL-_WL); cnt++) {
                    m_pVu->WriteMemX(m_memIndex, rVIF1_C0);
                    m_pVu->WriteMemY(m_memIndex, rVIF1_C1);
                    m_pVu->WriteMemZ(m_memIndex, rVIF1_C2);
                    m_pVu->WriteMemW(m_memIndex, rVIF1_C3);
                    m_memIndex++;
                }
                cycle = 0;
                write = 0;
            }
        }
        if ( m_trace ) {
            m_pLog->Trace(2, tUNPACK_MODES[m_unpack]);
        }
        switch(m_unpack) {
            case UNPACK_S32:
                m_fin->read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                word_y = word_x;
                word_z = word_x;
                word_w = word_x;
                break;
            case UNPACK_S16:
                m_fin->read(rhword, 2);
                word_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                word_y = word_x;
                word_z = word_x;
                word_w = word_x;
                break;
            case UNPACK_S8:
                m_fin->read(rbyte, 1);
                word_x = (unsigned int)rbyte[0];
                word_y = word_x;
                word_z = word_x;
                word_w = word_x;
                break;
            case UNPACK_V232:
                m_fin->read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_y = (rword[3]<<24) +
                    (rword[2]<<16) +
                    (rword[1]<<8) +
                    rword[0];
                // word_z = m_pVu->ReadMemZ(m_memIndex);
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V216:
                m_fin->read(rword, 2);
                word_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rword, 2);
                word_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                // word_z = m_pVu->ReadMemZ(m_memIndex);
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V28:
                m_fin->read(rbyte, 1);
                word_x = (unsigned int)rbyte[0];
                m_fin->read(rbyte, 1);
                word_y = (unsigned int)rbyte[0];
                // word_z = m_pVu->ReadMemZ(m_memIndex);
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V332:
                m_fin->read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_y = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_z = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V316:
                m_fin->read(rhword, 2);
                word_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rhword, 2);
                word_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rhword, 2);
                word_z = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V38:
                m_fin->read(rbyte, 1);
                word_x = (unsigned char)rbyte[0];
                m_fin->read(rbyte, 1);
                word_y = (unsigned char)rbyte[0];
                m_fin->read(rbyte, 1);
                word_z = (unsigned char)rbyte[0];
                // word_w = m_pVu->ReadMemW(m_memIndex);
                break;
            case UNPACK_V432:
                m_fin->read(rword, 4);
                word_x = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_y = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_z = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                m_fin->read(rword, 4);
                word_w = ((unsigned char)rword[3]<<24) +
                    ((unsigned char)rword[2]<<16) +
                    ((unsigned char)rword[1]<<8) +
                    (unsigned char)rword[0];
                break;
            case UNPACK_V416:
                m_fin->read(rhword, 2);
                word_x = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rhword, 2);
                word_y = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rhword, 2);
                word_z = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                m_fin->read(rhword, 2);
                word_w = ((unsigned char)rhword[1]<<8) +
                    (unsigned char)rhword[0];
                break;
            case UNPACK_V48:
                m_fin->read(rbyte, 1);
                word_x = (unsigned int)rbyte[0];
                m_fin->read(rbyte, 1);
                word_y = (unsigned int)rbyte[0];
                m_fin->read(rbyte, 1);
                word_z = (unsigned int)rbyte[0];
                m_fin->read(rbyte, 1);
                word_w = (unsigned int)rbyte[0];
                break;
            case UNPACK_V45:
                break;
            default:
                break;
        }

        if ( rVIF1_MODE == MODE_ADD ) {
            if ( _usn == 0 ) {
                word_x = (int)word_x+(int)rVIF1_R0;
                word_y = (int)word_y+(int)rVIF1_R1;
                word_z = (int)word_z+(int)rVIF1_R2;
                word_w = (int)word_w+(int)rVIF1_R3;
            } else {
                word_x = word_x+rVIF1_R0;
                word_y = word_y+rVIF1_R1;
                word_z = word_z+rVIF1_R2;
                word_w = word_w+rVIF1_R3;
            }
        } else if ( rVIF1_MODE == MODE_ADDROW ) {
            if ( _usn == 0 ) {
                word_x = (int)word_x+(int)rVIF1_R0;
                word_y = (int)word_y+(int)rVIF1_R1;
                word_z = (int)word_z+(int)rVIF1_R2;
                word_w = (int)word_w+(int)rVIF1_R3;
            } else {
                word_x = word_x+rVIF1_R0;
                word_y = word_y+rVIF1_R1;
                word_z = word_z+rVIF1_R2;
                word_w = word_w+rVIF1_R3;
            }
            rVIF1_R0 = word_x;
            rVIF1_R1 = word_y;
            rVIF1_R2 = word_z;
            rVIF1_R3 = word_w;
        }
        cout << "Writing: "
            << (void *)word_x << ", "
            << (void *)word_y << ", "
            << (void *)word_z << ", "
            << (void *)word_w << endl;
        m_pVu->WriteMemX(m_memIndex, word_x);
        m_pVu->WriteMemY(m_memIndex, word_y);
        m_pVu->WriteMemZ(m_memIndex, word_z);
        m_pVu->WriteMemW(m_memIndex, word_w);
        m_memIndex++;
        cycle++;
        write++;
    }
    return E_OK;
}

const int32
Vif::CmdMscal(const int32& data) {
    m_vifCmd = VIF_MSCAL;
    _addr = data&0xffff;
    if ( m_trace ) {
        m_pLog->Trace(1, wxString::Format("MSCAL at %d\n", _addr));
    }
    m_pVu->Run(_addr);
    return E_OK;
}

const int32
Vif::CmdMscalf(const int32& data) {
    // TODO simulate the wait for GIF PATH1/PATH2 and
    // end of microprogram ( ie mpg is running while a new batch is waiting to
    // be transfered from VIF to VU
    m_vifCmd = VIF_MSCALF;
    _addr = data&0xffff;
    if ( m_trace ) {
        m_pLog->Trace(1, wxString::Format("MSCALF at %d\n", _addr));
    }
    m_pVu->Run(_addr);
    return E_OK;
}

const int32
Vif::CmdMscnt(void) {
    if ( m_trace ) {
        m_pLog->Trace(1, "MSCNT\n");
    }
    m_pVu->Run();
    return E_OK;
}

const int32
Vif::CmdStmask(void) {
    char raw[4];
    uint32 data;
    m_fin->read(raw, 4);
    data = ((unsigned char)raw[3]<<24) +
        ((unsigned char)raw[2]<<16) +
        ((unsigned char)raw[1]<<8) +
        (unsigned char)raw[0];
    m_vifCmd = VIF_STMASK;
    rVIF0_MASK = data;
    rVIF1_MASK = data;
    if ( m_trace ) {
        m_pLog->Trace(1, "STMASK\n");
    }
    return E_OK;
}

const int32
Vif::CmdStrow(void) {
    char raw[4];
    uint32 data;
    if ( m_trace ) {
        m_pLog->Trace(1, "STROW\n");
    }
    m_fin->read(raw, 4);
    data =  ((unsigned char)raw[3]<<24) +
            ((unsigned char)raw[2]<<16) +
            ((unsigned char)raw[1]<<8)  +
            (unsigned char)raw[0];
    rVIF0_R0 = data;
    rVIF1_R0 = data;
    m_fin->read(raw, 4);
    data =  ((unsigned char)raw[3]<<24) +
            ((unsigned char)raw[2]<<16) +
            ((unsigned char)raw[1]<<8)  +
            (unsigned char)raw[0];
    rVIF0_R1 = data;
    rVIF1_R1 = data;
    m_fin->read(raw, 4);
    data =  ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            ((unsigned char)raw[1]<<8)+
            (unsigned char)raw[0];
    rVIF0_R2 = data;
    rVIF1_R2 = data;
    m_fin->read(raw, 4);
    data =  ((unsigned char)raw[3]<<24)+
            ((unsigned char)raw[2]<<16)+
            ((unsigned char)raw[1]<<8)+
            (unsigned char)raw[0];
    rVIF0_R3 = data;
    rVIF1_R3 = data;
    return E_OK;
}

const int32
Vif::CmdStcol(void) {
    char raw[4];
    int32 data;
    m_fin->read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C0 = data;
    rVIF1_C0 = data;
    m_fin->read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C1 = data;
    rVIF1_C1 = data;
    m_fin->read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C2 = data;
    rVIF1_C2 = data;
    m_fin->read(raw, 4);
    data = (raw[3]<<24)+
        (raw[2]<<16)+
        (raw[1]<<8)+
        raw[0];
    rVIF0_C3 = data;
    rVIF1_C3 = data;
    return E_OK;
}

// --------------------------------------------------------------------------
// vif0/vif1 common register to text functions
vector<string>
Vif::UnpackR(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("R");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}
vector<string>
Vif::UnpackC(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("C");
    sprintf(val, "0x%x", REGISTERS[reg]);
    v.push_back(val);
    return v;
}

vector<string>
Vif::UnpackItops(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ITOPS");
    sprintf(val, "0x%x", REGISTERS[reg]&0x3FF);
    v.push_back(val);
    return v;
}
vector<string>
Vif::UnpackItop(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("ITOP");
    sprintf(val, "0x%x", REGISTERS[reg]&0x3FF);
    v.push_back(val);
    return v;
}

vector<string>
Vif::UnpackCycle(const int reg) {
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
Vif::UnpackMode(const int reg) {
    vector<string> v;
    v.push_back("MODE");
    v.push_back(tMOD[REGISTERS[reg]&0x3]);
    return v;
}
vector<string>
Vif::UnpackErr(const int reg) {
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
Vif::UnpackMark(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
    return v;
}
vector<string>
Vif::UnpackNum(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
    return v;
}
vector<string>
Vif::UnpackMask(const int reg) {
    vector<string> v;
    char val[100];
    v.push_back("NUM");
    sprintf(val, "0x%x", REGISTERS[reg]&0x000000ff);
    v.push_back(val);
    return v;
}
vector<string>
Vif::UnpackCode(const int reg) {
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

void
Vif::writeRegister(void) {
}
