#ifndef __PARSER__
#define __PARSER__

#include <fstream>
#include "datatypes.h"
#include "opcodes.h"

class Vu;
class VuInstruction;

class Parser {
public:
    Parser(Vu* vpu);
    ~Parser();
    void    dlower(uint32 *lower, char *low, char *lparam);
    void    dupper(uint32 *upper, char *upp, char *uparam);
    int     insert(char *upper, char *lower, char *uparam, char *lparam, uint32 index);
    int     LoadCode(char *file);
    int     LoadInstructions(char *file);
    void    InitCodeMem(void);
private:
    int     IsValidInstruction(char *token, int Mode, int *InstIndex,
                                int *flavor, char *dest, char *flg);
    int     IndexMode(char *a);
    int     VIindex(char *a, char *dest);
    int     VFindex(char *a, char *dest);
    int     SameDest(char *a, char *b);
    int     GetVal(char *a, long *b);
    int     UGetVal(char *a, unsigned long *b, int *md);
    int     FGetVal(char *a, float *b);
    int     SetParam(VuInstruction &inst, int j, int mode, char *token);
    int     ProcessInstruction(char* Line);
    int     MatchSymbol(char* Line);
    int     ProcessLine(char* Line);
    int     ProcessSymbol(char* Line);
    int     LoadSymbol(char* file);
    char*   getLine(char* line, char* data);
    bool    LoadAsciiCode(ifstream *fin, char *code, int size);
    bool    LoadBinaryCode(ifstream *fin, char *data);
    void    get_params(int, uint32, char *, OPCODE *);
    int     get_lower(uint32);
    int     get_upper(uint32);
    int     get_int_reg(uint32, uint32);
    int     get_float_reg(uint32, uint32);
    int     get_imm5(uint32);
    int     get_imm11(uint32);
    int     get_imm12(uint32);
    int     get_imm15(uint32);
    int     get_imm24(uint32);
    int     get_imm32(uint32);
    char*   get_fsf(uint32);
    char*   get_ftf(uint32);

    Vu*     m_pVuCore;
};
#endif
