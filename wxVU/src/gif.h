#ifndef _GIF
#define _GIF
#include "datatypes.h"
#include "sub.h"

using namespace std;

static const char *tGIF_REGISTERS[] = {
    "GIF STAT", "GIF TAG0", "GIF TAG1",
    "GIF TAG2", "GIF TAG3", "GIF CNT",
    "GIF P3CNT", "GIF P3TAG"
};

enum GIF_REGISTERS {
    GIF_STAT, GIF_TAG0, GIF_TAG1, GIF_TAG2, GIF_TAG3,
    GIF_CNT, GIF_P3CNT, GIF_P3TAG
};

class GIF : public SubSystem {
public:
    GIF();
    GIF(uint32 *, uint32);
    static const int nREGISTERS = 8;
    string          getReadableTag();
    uint32          getNloop();
    uint32          getNreg();
    uint32          getRegs();
    string          RegsAsString();
    vector<string>  TagAsString();
    vector<string>  NloopData();

    vector<string>  getRegisterText(const int reg);

    bool            unpack();
    // variables
    long            xoffset;
    long            yoffset;
private:
    // functions
    void            unpackNloop(void);
    void            unpackNreg(void);
    void            unpackFlag(void);
    void            unpackPrim(void);
    void            unpackPre(void);
    void            unpackEop(void);
    void            unpackRegisters(void);
    void            parsePRIM(vector<string> &);
    bool            validate(void);

    // EE mapped register functions
    vector<string>  unpack_stat(const int reg); 
    vector<string>  unpack_tag0(const int reg); 
    vector<string>  unpack_tag1(const int reg); 
    vector<string>  unpack_tag2(const int reg); 
    vector<string>  unpack_tag3(const int reg); 
    vector<string>  unpack_cnt(const int reg); 
    vector<string>  unpack_p3cnt(const int reg); 
    vector<string>  unpack_p3tag(const int reg); 

    // variables
    unsigned char   registers[16];
    uint32          nloop;
    uint32          eop;
    uint32          nreg;
    uint32          flag;
    uint32          prim;
    uint32          pre;
    ivec            gifData[1024];

    uint32          counter;
    uint32          curNreg;
    uint32          curNloop;
};
#endif
