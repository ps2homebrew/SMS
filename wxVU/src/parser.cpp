#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include "util.h"
#include "vu.h"
#include "opcodes.h"
#include "parser.h"

using namespace std;

static int get_lower(uint32);
static int get_upper(uint32);
static int get_int_reg(uint32, uint32);
static int get_float_reg(uint32, uint32);
static int get_imm5(uint32);
static int get_imm11(uint32);
static int get_imm12(uint32);
static int get_imm15(uint32);
static int get_imm24(uint32);
static int get_imm32(uint32);
static char *get_fsf(uint32);
static char *get_ftf(uint32);
static void get_params(int, uint32, char *, OPCODE *);
static bool LoadBinaryCode(ifstream *, char*);
static bool LoadAsciiCode(ifstream *, char*, int);
static char *getLine(char *, char *);

static const int vid = 6;
static const int vis = 11;
static const int vit = 16;

static const int vfd = 6;
static const int vfs = 11;
static const int vft = 16;

static uint32 ibit = 0;
static uint32 ebit = 0;
static uint32 mbit = 0;
static uint32 dbit = 0;
static uint32 tbit = 0;

extern MicroCode Instr;
extern VU VUchip;

int counter = 0;

int
LoadInstructions (char *file) {
    /*in a future version this should use
      XML to load data, but by the moment...
      File format is very simple, each instructions has:
      tipe //0 - Micromode Upper
    //1 - Micromode Lower
    //No macromode sorry
    Nemot //String representing instruction
    Suffix //0-No flavors
    //1-3 flavor i.e MAX, MAXi, MAXbc
    //2-8 flavor i.e ADD, ADDi, ADDq, ADDbc, ...
    Dest   //0
    N Operands //Number of operands
    Type 1  //Type of operand,
    Type 2  // 0- VIReg 1- VFReg 2-Integer 3-Float 4-P 5-Q 6-ACC
    ...
    Type n
    Throughput
    Latency
# at begining means line is comentary
     */
    FILE *In;
    char Line[250];
    int part = 0, ope = 0, i;

    Instr.nInstructionDef = 0;
    if((In=fopen(file,"r"))==NULL) {
        return 0;
    }

    while(fgets(Line,250,In)) {
        strtrim(Line);
        if(Line[0]=='#') {
            continue;
        }
        switch(part) {
            case 0:
                Instr.Instr[Instr.nInstructionDef].mode = atoi(Line);
                part=1;
                break;
            case 1:
                strcpy(Instr.Instr[Instr.nInstructionDef].nemot, strupr(Line));
                part=2;
                break;
            case 2:
                Instr.Instr[Instr.nInstructionDef].sufix = atoi(Line);
                part=3;
                break;
            case 3:
                Instr.Instr[Instr.nInstructionDef].operands = atoi(Line);
                if(Instr.Instr[Instr.nInstructionDef].operands==0) {
                    part=5;
                    ope=0;
                } else {
                    part=4;
                    ope=0;
                }
                break;
            case 4:
                strcpy(Instr.Instr[Instr.nInstructionDef].types[ope++],Line);
                if(ope==Instr.Instr[Instr.nInstructionDef].operands) {
                    part=5;
                    ope=0;
                }
                break;
            case 5:
                Instr.Instr[Instr.nInstructionDef].throughput=atoi(Line);
                part=6;
                break;
            case 6:
                Instr.Instr[Instr.nInstructionDef].latency=atoi(Line);
                part=0;
                for(i=0; i<15; i++)
                    Instr.Instr[Instr.nInstructionDef].lastthr[i]=0;
                Instr.nInstructionDef++;
                break;
        }
    }
    if (part!=0)
        return 0;
    return 1;
}


int IsValidInstruction(char *token, int Mode, int *InstIndex, int *flavor, char *dest, char *flg)
{
    //Mode 0-upper 1-lower
    //flavors
    //0 - normal
    //1 - i register
    //2-5 - bc broadcast 2-x 3-y 4-z 5-w
    //6 - q register
    //7 - a acumulator
    //8 - ai acumulator and i register
    //9 - aq acumulator and q register
    //10-13 - abc acumulator broadcast 10-x 11-y 12-z 13-w
    //
    //dest
    //x y z w and combinations

    int i,j,k;
    char root[15],root2[20],defdes[20],teststr[6];

    *flg=0;
    i=0;
    dest[0]=0;
    strcpy(teststr,"XYZW");
    while(token[i]) {                //take the .dest part
        if(token[i]=='.') {
            token[i]=0;
            strcpy(dest,token+i+1);
        } else {
            i++;
        }
    }
    i=0;

    while(token[i]!='[' && token[i]) {
        i++;
    }

    if(token[i]) {
        *flg=token[i+1];
        token[i]=0;
    }

    i=0;
    if(dest[0]) { //check .dest is valid
        for(j=0;j<4;j++) {
            for(k=0;k<(int)strlen(dest);k++) {
                if(dest[k]==teststr[j]) {
                    i++;
                    teststr[j]=0;
                }
            }
        }

    }
    if(i!=(int)strlen(dest)) {
        return 0;
    }
    for (i=0; i<Instr.nInstructionDef; i++) {
        if(Instr.Instr[i].mode==Mode) {
            defdes[0]=0;
            j=0;
            strcpy(root,Instr.Instr[i].nemot);
            while(root[j])
                if(root[j]=='.') {
                    root[j]=0;
                    strcpy(defdes,root+j+1);
                }
                else j++;
                if(dest[0] && !defdes[0])
                    continue; //instruction comes with dest and not allowed
                if(defdes[0] && strcmp(defdes,"DEST")) {
                    if(strcmp(dest,defdes))
                        continue; //dest is not xyzw, instead is fixed
                }

                *InstIndex=i;
                *flavor=0;
                if(!strcmp(root,token)) {
                    //the only exception in all instructions
                    if(!strcmp(token,"CLIPW"))
                        *flavor=5;
                    return 1;
                }
                if(Instr.Instr[i].sufix!=0) {
                    //flavor 1
                    *flavor=1;
                    sprintf(root2,"%s%s",root,"I");
                    if(!strcmp(root2,token)) {
                        return 1;
                    }
                    //flavor 2
                    *flavor=2;
                    sprintf(root2,"%s%s",root,"X");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=3;
                    sprintf(root2,"%s%s",root,"Y");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=4;
                    sprintf(root2,"%s%s",root,"Z");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=5;
                    sprintf(root2,"%s%s",root,"W");
                    if(!strcmp(root2,token))
                        return 1;
                }
                if(Instr.Instr[i].sufix==2) {
                    //flavor 6
                    *flavor=6;
                    sprintf(root2,"%s%s",root,"Q");
                    if(!strcmp(root2,token))
                        return 1;
                    //flavor 7
                    *flavor=7;
                    sprintf(root2,"%s%s",root,"A");
                    if(!strcmp(root2,token))
                        return 1;
                    //flavor 8
                    *flavor=8;
                    sprintf(root2,"%s%s",root,"AI");
                    if(!strcmp(root2,token))
                        return 1;
                    //flavor 9
                    *flavor=9;
                    sprintf(root2,"%s%s",root,"AQ");
                    if(!strcmp(root2,token))
                        return 1;
                    //flavor 10
                    *flavor=10;
                    sprintf(root2,"%s%s",root,"AX");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=11;
                    sprintf(root2,"%s%s",root,"AY");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=12;
                    sprintf(root2,"%s%s",root,"AZ");
                    if(!strcmp(root2,token))
                        return 1;
                    *flavor=13;
                    sprintf(root2,"%s%s",root,"AW");
                    if(!strcmp(root2,token))
                        return 1;
                }
        }
    }
    *InstIndex=0;
    *flavor=0;
    return 0;
}

int
IndexMode(char *a) {
    int i;
    char *Type[]={"VI", "VF", "VIDEST", "VFDEST", "ACC",
        "IMM24", "IMM15", "IMM12", "IMM11", "IMM5", "I",
        "IMM11(VI)", "IMM11(VI)DEST", "(VI)DEST", "(--VI)",
        "(VI++)", "P", "Q", "R", "VI01", "FMM32"};
    for (i=0; i<21; i++) {
        if(!strcmp(strupr(a),Type[i])) {
            return i+1;
        }
    }
    return 0;
}

int
VIindex(char *a, char *dest) {
    int i=2,j;
    if(a[0]!='V' || a[1]!='I') return -1;
    while (a[i]=='0' && a[i]) i++;
    if(!a[i]) return 0;
    j=atoi(a+i);
    while (isdigit(a[i++]));
    if(a[i-1]=='.')
        strcpy(dest,a+i);
    else
        strcpy(dest,a+i-1);
    if (j<0 || j>16) return -1;
    return j;
}

int
VFindex(char *a, char *dest) {
    int i=2,j;
    if(a[0]!='V' || a[1]!='F') return -1;
    while (a[i]=='0' && a[i]) i++;
    if(!a[i]) return 0;
    j=atoi(a+i);
    while (isdigit(a[i++]));
    if(a[i-1]=='.')
        strcpy(dest,a+i);
    else
        strcpy(dest,a+i-1);
    if (j<0 || j>32) return -1;
    return j;
}

int
SameDest(char *a, char *b) {
    //check a nd b are the same destinations
    int i,j;
    if (!a[0]) return 1; //if no dest specified takes default

    i=0;
    while(a[i]) {
        for(j=0; b[j]; j++)
            if(a[i]==b[j])
                break;
        if(!b[j])
            return 0;
        i++;
    }

    i=0;
    while(b[i]) {
        for(j=0; a[j]; j++)
            if(b[i]==a[j])
                break;
        if(!a[j])
            return 0;
        i++;
    }
    return 1;
}

int
GetVal(char *a, long *b) {
    int i=0;
    long offset;
    char auxi[100], auxi2[100];

    strcpy(auxi,a);
    while(auxi[i]!='+' && auxi[i]!='-' && auxi[i]) i++;
    if(auxi[i]=='+')
        offset=atol(auxi+i+1);
    if(auxi[i]=='-')
        offset=atol(auxi+i);
    if(auxi[i]==0)
        offset=0;
    auxi[i]=0;
    for(i=0; i<VUchip.NSymbols; i++) {
        strcpy(auxi2,VUchip.Labels[i].symb);
        if(!strcmp(auxi,strupr(auxi2))) {
            *b=offset+(long)VUchip.Labels[i].Line;
            return 2;
        }
    }
    for(i=0; i<VUchip.NMemDir; i++) {
        strcpy(auxi2,VUchip.MemDir[i].symb);
        if(!strcmp(auxi,strupr(auxi2))) {
            *b=offset+(long)VUchip.MemDir[i].Line;
            return 2;
        }
    }
    i=0;
    while(auxi[i]=='0') i++;
    if(auxi[i]=='X') { //ex format
        *b= offset+(long) htol(auxi+i+1);
        return 1;
    }
    *b=offset+(long)atol(auxi);
    return 1;
}

int
UGetVal(char *a, unsigned long *b, int *md) {
    int i;
    i=strlen(a);
    *md=0;
    if(a[0]=='(' && a[i-1]==')') {
        *md=1;
        a[i-1]=0;
        strcpy(a,a+1);
    }

    for(i=0; i<VUchip.NSymbols; i++)
        if(!strcmp(a,VUchip.Labels[i].symb)) {
            *b=(unsigned long)VUchip.Labels[i].Line;
            return 2;
        }
    for(i=0; i<VUchip.NMemDir; i++)
        if(!strcmp(a,VUchip.MemDir[i].symb)) {
            *b=(unsigned long)VUchip.MemDir[i].Line;
            return 2;
        }
    i=0;
    while(a[i]=='0') i++;
    if(a[i]=='X') { //ex format
        *b = (unsigned long) htol(a+i+1);
        return 1;
    }
    *b = (unsigned long)atol(a);
    return 1;
}

int FGetVal(char *a, float *b)
{
    char auxi[100];

    if(a[0] == '0' && a[1] == 'X') {
#ifndef WIN32
        int q = (int)strtoll(a, (char **)NULL, 16);
#else
		int q = (int)strtol(a, (char **)NULL, 16);
#endif
        sprintf(auxi, "%f", *((float *)&q));
    } else  {
        strcpy(auxi,a);
    }
    *b=atof(auxi);
    return 1;
}

int
SetParam(VUInstruction &inst, int j, int mode, char * token) {
    //check if param is correct and store it in instruction class
    //TYPE OF PARAM VI VF VIdest VFdest ACCdest Imm24 Imm15 Imm12 Imm11 Imm5 I
    //TYPE OF PARAM Imm11(VI) Imm11(VI)dest (VI)dest (--VI) (VI++) P Q R VI01 Fmm32
    int i,k,v,w,NemoMode,md;
    long Imm11, Imm5;
    unsigned long Imm24, Imm15, Imm12;
    float Fmm32;
    char root[100], dest[20];
    char flavors[14][3] = {"","i","x","y","z","w","q","A","Ai","Aq","Ax","Ay","Az","Aw"};

    strcpy(root,strupr(token));
    i = IndexMode(Instr.Instr[inst.InstIndex[mode]].types[j]);
    inst.Params[mode][j].type = i;
    if(inst.flavor[mode]> 6 && j==0) {
        inst.Params[mode][j].type=5; //Acumulator
    }
    if(inst.flavor[mode]==1 && j==2) {
        inst.Params[mode][j].type=11; //I register
    }
    if(inst.flavor[mode]==8 && j==2) {
        inst.Params[mode][j].type=11; //I register
    }
    if(inst.flavor[mode]==6 && j==2) {
        inst.Params[mode][j].type=18; //Q register
    }
    if(inst.flavor[mode]==9 && j==2) {
        inst.Params[mode][j].type=18; //Q register
    }
    inst.Params[mode][j].sufix[0]=0;
    w=0;
    strcpy(dest,Instr.Instr[inst.InstIndex[mode]].nemot);
    strrev(dest);
    if(!strncmp(dest,"TSED.",5))
        NemoMode=1; //dest specified in instruction
    else
        NemoMode=0; //dest not specified in instruction
    switch (i) {
        case 1: //VInn
            k=VIindex(root, dest);
            if(k<0) return 0;
            inst.Params[mode][j].index=k;
            break;
        case 2: //VFnn
            k=VFindex(root, dest);
            if(k<0) return 0;
            inst.Params[mode][j].index=k;
            break;
        case 3: //VIdest
            k=VIindex(root, dest);
            if(k<0) return 0;
            if(NemoMode)
                if(!SameDest(dest,inst.dest[mode]))
                    return 0;
            inst.Params[mode][j].index=k;
            strcpy(inst.Params[mode][j].sufix,dest);
            break;
        case 4: //VFdest
            k=VFindex(root,dest);
            if(k<0) return 0;
            //check for broadcast cause is differen dest testing
            if(NemoMode) {
                if(j==Instr.Instr[inst.InstIndex[mode]].operands-1) {
                    if(inst.flavor[mode]>9) {
                        if(!SameDest(dest, strupr(flavors[inst.flavor[mode]]+1)))
                            return 0;
                    }
                    if(inst.flavor[mode]>1 && inst.flavor[mode]<6) {
                        if(!SameDest(dest, strupr(flavors[inst.flavor[mode]])))
                            return 0;
                    }
                } else {
                    if(!SameDest(dest,inst.dest[mode])) return 0;
                }
            }
            inst.Params[mode][j].index=k;
            strcpy(inst.Params[mode][j].sufix,dest);
            break;
        case 5: //ACCdest
            if(strncmp(root,"ACC",3)) return 0;
            strcpy(dest,root+3);
            if(NemoMode) {
                if(!SameDest(dest,inst.dest[mode])) {
                    return 0;
                }
            }
            inst.Params[mode][j].type=i;
            strcpy(inst.Params[mode][j].sufix,dest);
            break;
        case 6: //Imm24
            if(!UGetVal(root,&Imm24,&md)) return 0;
            inst.Params[mode][j].udata=Imm24;
            inst.Params[mode][j].memdir=md;
            break;
        case 7: //Imm15
            if(!UGetVal(root,&Imm15,&md)) return 0;
            inst.Params[mode][j].udata=Imm15;
            inst.Params[mode][j].memdir=md;
            break;
        case 8: //Imm12
            if(!UGetVal(root,&Imm12,&md)) return 0;
            inst.Params[mode][j].udata=Imm12;
            inst.Params[mode][j].memdir=md;
            break;
        case 9: //Imm11
            v = GetVal(root,&Imm11);
            if(!v) {
                return 0;
            }
            if(v==2)
                strcpy(inst.Params[mode][j].label,root);
            inst.Params[mode][j].data=Imm11;
            break;
        case 10: //Imm5
            if(!GetVal(root,&Imm5)) return 0;
            inst.Params[mode][j].data=Imm5;
            break;
        case 11: //I
            if(root[0] != 'I') {
                return 0;
            }
            break;
        case 12: //Imm11(VI)
            v=0;
            while(root[v]!='(' && root[v]) v++;
            if (!root[v]) return 0;
            root[v]=0;
            w=GetVal(root,&Imm11);
            if(!w) return 0;
            if(w==2)
                strcpy(inst.Params[mode][j].label,root);
            strcpy(root,root+v+1);
            v=0;
            while(root[v]!=')' && root[v]) v++;
            if (!root[v]) return 0;
            root[v]=0;
            k=VIindex(root, dest);
            if(k<0) return 0;
            inst.Params[mode][j].data=Imm11;
            inst.Params[mode][j].index=k;
            break;
        case 13: //Imm11(VI)dest
            v = 0;
            while(root[v]!='(' && root[v]) {
                v++;
            }
            if (!root[v]) {
                return 0;
            }
            root[v]=0;
            w = GetVal(root,&Imm11);
            if(!w) {
                return 0;
            }
            if(w==2) {
                strcpy(inst.Params[mode][j].label,root);
            }
            strcpy(root,root+v+1);
            v = 0;
            while(root[v]!=')' && root[v]) {
                v++;
            }
            if (!root[v]) {
                return 0;
            }
            root[v]=0;
            strcat(root,root+v+1); //cat dest
            k = VIindex(root, dest);
            if(k<0) {
                return 0;
            }
            if(NemoMode) {
                // if(!SameDest(dest, inst.dest[mode])) {
                //     return 0;
                // }
            }
            inst.Params[mode][j].data = Imm11;
            inst.Params[mode][j].index = k;
            // strcpy(inst.Params[mode][j].sufix, dest);
            break;
        case 14: //(VI)dest
            if (root[0]!='(') return 0;
            strcpy(root,root+1);
            v=0;
            while(root[v]!=')' && root[v]) v++;
            if (!root[v]) return 0;
            root[v]=0;
            strcat(root,root+v+1); //cat dest
            k = VIindex(root, dest);
            // if(NemoMode) {
            //     if(!SameDest(dest,inst.dest[mode])) {
            //         cout << "not same dest" << endl;
            //     }
            // }
            inst.Params[mode][j].index = k;
            strcpy(inst.Params[mode][j].sufix,dest);
            break;
        case 15: //(--VI)
            if (strncmp(root,"(--",3)) return 0;
            strcpy(root,root+3);
            v=0;
            while(root[v]!=')' && root[v]) v++;
            if (!root[v]) return 0;
            root[v]=0;
            k=VIindex(root, dest);
            inst.Params[mode][j].index=k;
            break;
        case 16: //(VI++)
            strrev(root);
            if (strncmp(root,")++",3)) return 0;
            strcpy(root,root+3);
            strrev(root);
            strcpy(root,root+1);
            k=VIindex(root, dest);
            inst.Params[mode][j].index=k;
            strcpy(inst.Params[mode][j].sufix,dest);
            break;
        case 17: //P
            if (strcmp(root,"P")) return 0;
            break;
        case 18: //Q
            if (strcmp(root,"Q")) return 0;
            break;
        case 19: //R
            if (strcmp(root,"R")) return 0;
            break;
        case 20: //VI01
            if (strcmp(root,"VI01")) return 0;
            inst.Params[mode][j].index=1;
            break;
        case 21: //Fmm32
            if(!FGetVal(root,&Fmm32)) {
                return 0;
            }
            inst.Params[mode][j].fdata=Fmm32;
            break;
    }
    return 1;
}

int
ProcessInstruction(char * Line) {
    //this functions should process upper and lower instruction line
    //first we spect to find the upper instruction
    int i=0,j,k,InstIndex, flavor=0;
    char token[50], dest[50], flg;

    while (Line[i]!=' ' && Line[i]!='\t' && Line[i]!=0) {
        i++;
    }
    if (!Line[i]) {
        return 0; //no separator found
    }
    strncpy(token,Line,i);
    token[i]=0;
    strupr(token);
    VUchip.program[VUchip.NInstructions].invalid = 0;
    if(!IsValidInstruction(token, UPPER, &InstIndex, &flavor, dest, &flg)) {
        VUchip.program[VUchip.NInstructions].invalid = 1;
        return 0;
    }
    VUchip.program [VUchip.NInstructions].InstIndex[UPPER]=InstIndex;
    strcpy(VUchip.program [VUchip.NInstructions].dest[UPPER],dest);
    VUchip.program[VUchip.NInstructions].flavor[UPPER] = flavor;
    VUchip.program[VUchip.NInstructions].breakpoint = 0;
    VUchip.program[VUchip.NInstructions].flg = flg;
    VUchip.program[VUchip.NInstructions].SymbolIndex = -1;
    for (j=0; j<Instr.Instr[InstIndex].operands; j++) {
        if(j==Instr.Instr[InstIndex].operands-1) { //last operand
            //first eliminate heading blanks
            while ((Line[i]==' ' || Line[i]=='\t') && Line[i]!=0) i++;
            k=i;
            while (Line[i]!=' ' && Line[i]!='\t' && Line[i]!=0) i++;
            strncpy(token,Line+k,i-k);
            token[i-k]=0;  //parameter grabbed in token
            SetParam(VUchip.program[VUchip.NInstructions],j,UPPER,token);
        }
        else { //intermediate operand, search for a ','
            while ((Line[i]==' ' || Line[i]=='\t') && Line[i]!=0) i++;
            k=i;
            while(Line[i]!=',' && Line[i]) i++;
            strncpy(token,Line+k,i-k);
            token[i-k]=0; //parameter grabbed in token
            SetParam(VUchip.program [VUchip.NInstructions],j,UPPER,token);
            if(Line[i]==',') i++;
        }
    }
    if(!Line[i]) {
        return 0;
    }

    while ((Line[i]==' ' || Line[i]=='\t') && Line[i]!=0) i++;
    if(!Line[i]) {
        return 0;
    }
    j=i;
    while (Line[i]!=' ' && Line[i]!='\t' && Line[i]!=0) i++;
    strncpy(token,Line+j,i-j);
    token[i-j]=0;
    strupr(token);

    if(!IsValidInstruction(token, LOWER, &InstIndex, &flavor,dest, &flg)) {
        VUchip.program[VUchip.NInstructions].invalid = 1;
        return 0;
    }
    VUchip.program[VUchip.NInstructions].InstIndex[LOWER] = InstIndex;
    strcpy(VUchip.program[VUchip.NInstructions].dest[LOWER],dest);
    VUchip.program[VUchip.NInstructions].flavor[LOWER] = flavor;
    for (j=0; j<Instr.Instr[InstIndex].operands; j++) {
        if(j==Instr.Instr[InstIndex].operands-1) { //last operand
            //first eliminate heading blanks
            while ((Line[i]==' ' || Line[i]=='\t') && Line[i]!=0) i++;
            k=i;
            while (Line[i]!=' ' && Line[i]!='\t' && Line[i]!=0) i++;
            strncpy(token,Line+k,i-k);
            token[i-k]=0;
            SetParam(VUchip.program [VUchip.NInstructions],j,LOWER,token);
        }
        else { //intermediate operand, search for a ','
            while ((Line[i]==' ' || Line[i]=='\t') && Line[i]!=0) {
                i++;
            }
            k=i;
            while(Line[i]!=',' && Line[i]) {
                i++;
            }
            strncpy(token,Line+k,i-k);
            token[i-k] = 0;
            SetParam(VUchip.program [VUchip.NInstructions],j,LOWER,token);
            if(Line[i]==',') {
                i++;
            }
        }
    }
    VUchip.NInstructions++;
    return 1;
}


int
MatchSymbol(char *Line) {
    int i;
    i=strlen(Line);
    if(Line[i-1]!=':')
        return 0;
    Line[i-1]=0;
    for(i=0; i<VUchip.NSymbols; i++)
        if(!strcmp(Line,VUchip.Labels[i].symb)) {
            VUchip.program [VUchip.NInstructions].SymbolIndex=i;
            return 1;
        }
    return 0;
}

int
ProcessLine(char *Line) {
    int i;
    i = strlen(Line);
    if(Line[i-1]==':') { 
        return MatchSymbol(Line);
    } else {
        return ProcessInstruction(Line);
    }
}

int
ProcessSymbol(char *Line) {
    //we spect to find here a label
    //or a upper - lower pair
    int i;

    i=strlen(Line);
    if(Line[i-1]!=':') { //it is an instruction
        counter++;
        return 1;
    }
    Line[i-1]=0;
    strcpy(VUchip.Labels[VUchip.NSymbols].symb, Line);
    VUchip.Labels[VUchip.NSymbols].Line=VUchip.NInstructions;
    VUchip.Labels[VUchip.NSymbols].Line=counter;
    VUchip.NSymbols++;
    return 1;
}

int
LoadSymbol(char *file) {
    FILE *In;
    char Line[250];
    int j=0;
    if((In=fopen(file,"r"))==NULL)
        return 0;
    counter=0;
    while(fgets(Line,250,In)) {
        j=0;
        //truncate remarks
        while(Line[j]) {
            if(Line[j]==';')
                Line[j]=0;
            else
                j++;
        }
        strtrim(Line);
        if(Line[0]==';' || Line[0]==0) //comment or empty line
            continue;
        if(!ProcessSymbol(Line)) {
            VUchip.Reset();
            return 0;
        }
    }
    return 1;
}

int
LoadCode(char *file) {
    char Line[250];
    string line;
    int j = 0;
    VUchip.Reset();
    ifstream fin(file, ios::binary);
    struct stat st;
    char *data;

    if (stat(file, &st) != 0 ) {
        return 0;
    }
    if (!isascii(fin.get()) || !isascii(fin.get()) ||
        !isascii(fin.get()) || !isascii(fin.get()) ||
        !isascii(fin.get()) || !isascii(fin.get()) ||
        !isascii(fin.get()) || !isascii(fin.get()) 
        ) {
        fin.seekg(0);
        data = (char *)malloc(st.st_size*50);
        LoadBinaryCode(&fin, data);
    } else {
        if(!LoadSymbol(file)) {
            return 0;
        }
        fin.seekg(0);
        data = (char *)malloc(st.st_size+1);
        memset(data, 0, st.st_size+1);
        LoadAsciiCode(&fin, data, st.st_size);
    }

    while((data = getLine(Line, data))) {
        j = 0;
        while(Line[j]) {
            if(Line[j]==';') {
                Line[j]=0;
            } else {
                j++;
            }
        }
        strtrim(Line);
        if(!strncmp(Line, ".vu", 3)) {
            continue;
        }
        if(!strncmp(Line, ".global", 7)) {
            continue;
        }
        if(!strncmp(Line, ".data", 5)) {
            break;
        }
        if(Line[0]==';' || Line[0]==0 || Line[0]=='#') //comment or empty line
            continue;
        if(!ProcessLine(Line)) {
            return 0;
        }
    }
    fin.close();
    free(data);
    return 1;
}

char *
getLine(char *line, char *data){
    int i;
    if ( *data == NULL ) {
        return NULL;
    }
    for(i = 0; *data != '\n' && *data != NULL; data++, i++) {
        *line++ = *data;
    }
    data++;
    *line = '\0';
    return data;
}

bool
LoadAsciiCode(ifstream *fin, char *code, int size) {
    cout << "LoadAsciCode" << endl;
    fin->read(code, size);
    return true;
}

bool
LoadBinaryCode(ifstream *fin, char *data) {
    uint32 ucode, lcode;
    int uidx, lidx;
    char lparam[50];
    char uparam[50];
    string test;

    cout << "LoadBinaryCode" << endl;

    while(!fin->eof() ) {
        fin->read((char *)&lcode, sizeof(uint32));
        fin->read((char *)&ucode, sizeof(uint32));
        // if last 8 bytes are 0 its surely the end
        if ( ucode == 0 ) {
            break;
        }
        if ( lcode == 0 ) {
            break;
        }
        uidx = get_upper(ucode);
        lidx = get_lower(lcode);

        memset(lparam, 0, 50);
        memset(uparam, 0, 50);
        get_params(uidx, ucode, uparam, uopcodes);
        get_params(lidx, lcode, lparam, lopcodes);
        if ( uidx != -1 ) {
            test += uopcodes[uidx].name;
            if ( ebit == 1 ) {
                test += "[e]";
            } else if ( dbit == 1 ) {
                test += "[d]";
            }
            test += " "; 
            test += uparam;
            test += " ";
        }
        if ( lidx != -1 ) {
            test += lopcodes[lidx].name;
            test += " ";
            test += lparam;
        }
        test += "\n";
    }
    strcpy(data, test.c_str());
    return true;
}

void
get_params(int idx, uint32 code, char *param, OPCODE *opcodes) {
    switch(opcodes[idx].param) {
        case PARAM_FSFVFS:
            sprintf(param, "P,VF%02d%s", get_float_reg(code, vfs),
                    get_fsf(code));
            break;
        case PARAM_FSFVFS_FTFVFT:
            sprintf(param, "Q,VF%02d%s,VF%02d%s", get_float_reg(code, vfs),
                    get_fsf(code), get_float_reg(code, vft), get_ftf(code));
            break;
        case PARAM_FTFVFT:
            sprintf(param, "Q,VF%02d%s", get_float_reg(code, vft),
                    get_ftf(code));
            break;
        case PARAM_IMM11:
            sprintf(param, "0x%08x", get_imm11(code));
            break;
        case PARAM_IMM12:
            sprintf(param, "0x%08x", get_imm12(code));
            break;
        case PARAM_IMM24:
            sprintf(param, "0x%08x", get_imm24(code));
            break;
        case PARAM_IMM32:
            sprintf(param, "0x%08x", get_imm32(code));
            break;
        case PARAM_NOTHING:
            break;
        case PARAM_VFD_VFS:
            sprintf(param, "VF%02d,VF%02d", get_float_reg(code, vfd),
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFD_VFS_Q:
            sprintf(param, "VF%02d,VF%02d,Q", get_float_reg(code, vfd),
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFT_I:
            sprintf(param, "ACC,VF%02d,I", get_float_reg(code, vft));
            break;
        case PARAM_VFD_VFS_I:
            sprintf(param, "VF%02d,VF%02d,I", get_float_reg(code, vfd),
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFS_I:
            sprintf(param, "ACC,VF%02d,I", get_float_reg(code, vfs));
            break;
        case PARAM_VFD_VFS_VFT:
            sprintf(param, "VF%02d,VF%02d,VF%02d", 
                    get_float_reg(code, vfd),
                    get_float_reg(code, vfs),
                    get_float_reg(code, vft));
            break;
        case PARAM_VFS_Q:
            sprintf(param, "ACC,VF%02d,Q", 
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFS:
            sprintf(param, "P,VF%02d", 
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFS_IMM11_VIT_:
            sprintf(param, "VF%02d,%d(VI%02d)", get_float_reg(code, vfs),
                    get_imm11(code), get_int_reg(code, vit));
            break;
        case PARAM_VFS_VFT:
            sprintf(param, "ACC,VF%02d,VF%02d", get_float_reg(code, vfs),
                    get_float_reg(code, vft));
            break;
        case PARAM_VFS_VIT_DEC:
            sprintf(param, "VF%02d,(--VI%02d)",
                get_float_reg(code, vfs),
                get_int_reg(code, vis));
            break;
        case PARAM_VFS_VIT_INC:
            sprintf(param, "VF%02d,(VI%02d++)",
                get_float_reg(code, vfs),
                get_int_reg(code, vit));
            break;
        case PARAM_VFS_VIT:
            sprintf(param, "VF%02d,%d(VI%02d)", get_float_reg(code, vfs),
                    get_imm11(code), get_int_reg(code, vit));
            break;
        case PARAM_VFT:
            sprintf(param, "VF%02d,P", get_float_reg(code, vft));
            break;
        case PARAM_VFT_VIS_DEC:
            sprintf(param, "VF%02d,(--VI%02d)",
                get_float_reg(code, vft),
                get_int_reg(code, vis));
            break;
        case PARAM_VFT_VIS_INC:
            sprintf(param, "VF%02d,(VI%02d++)",
                get_float_reg(code, vft),
                get_int_reg(code, vis));
            break;
        case PARAM_VFS_VIT_IMM11:
            sprintf(param, "VF%02d,%d(VI%02d)", get_float_reg(code, vfs),
                    get_imm11(code), get_int_reg(code, vit));
            break;
        case PARAM_VFT_IMM11_VIS:
            sprintf(param, "VF%02d,%d(VI%02d)", get_float_reg(code, vft),
                    get_imm11(code), get_int_reg(code, vis));
            break;
        case PARAM_VFT_VFS:
            sprintf(param, "VF%02d,VF%02d", get_float_reg(code, vft),
                    get_float_reg(code, vfs));
            break;
        case PARAM_VFT_VIS:
            sprintf(param, "VF%02d,VI%02d",
                get_float_reg(code, vft),
                get_int_reg(code, vis)
                );
            break;
        case PARAM_VI01_IMM24:
            sprintf(param, "VI01,0x%08x", get_imm24(code));
            break;
        case PARAM_VID_VIS_VIT:
            sprintf(param, "VI%02d,VI%02d,VI%02d", get_int_reg(code, vid),
                get_int_reg(code, vis),
                get_int_reg(code, vit)
                );
            break;
        case PARAM_VIS:
            sprintf(param, "VI%02d", get_int_reg(code, vis));
            break;
        case PARAM_VIS_IMM11:
            sprintf(param, "VI%02d,0x%08x", get_int_reg(code, vis),
                    get_imm11(code));
            break;
        case PARAM_VIT:
            sprintf(param, "VI%02d", get_int_reg(code, vit));
            break;
		case PARAM_VIT_FSFVFS:
            sprintf(param, "VI%02d,VF%02d%s", get_int_reg(code, vit),
                    get_float_reg(code, vfs), get_fsf(code));
            break;
		case PARAM_VIT_IMM11:
            sprintf(param, "VI%02d,0x%08x", get_int_reg(code, vit),
                    get_imm11(code));
            break;
		case PARAM_VIT_IMM11_VIS:
            sprintf(param, "VI%02d,%d(VI%02d)", get_int_reg(code, vit),
                    get_imm11(code), get_int_reg(code, vis));
            break;
		case PARAM_VIT_IMM12:
            sprintf(param, "VI%02d,0x%08x", get_int_reg(code, vit),
                    get_imm12(code));
            break;
		case PARAM_VIT_VIS:
            sprintf(param, "VI%02d,VI%02d", get_int_reg(code, vit),
                    get_int_reg(code, vis));
            break;
		case PARAM_VIT_VIS_IMM11:
            printf("we need to insert a label\n");
            sprintf(param, "L%d:", get_imm11(code));
            printf("label = %s\n", param);
            strcpy(VUchip.Labels[VUchip.NSymbols].symb, param);
            VUchip.Labels[VUchip.NSymbols].Line=1;
            VUchip.Labels[VUchip.NSymbols].Line=1;
            VUchip.program[1].SymbolIndex = 1;
            VUchip.NSymbols++;

            sprintf(param, "VI%02d,VI%02d,L%d", get_int_reg(code, vit),
                    get_int_reg(code, vis), get_imm11(code));
            break;
		case PARAM_VIT_VIS_IMM15:
            sprintf(param, "VI%02d,VI%02d,0x%08x", get_int_reg(code, vit),
                    get_int_reg(code, vis), get_imm15(code));
            break;
		case PARAM_VIT_VIS_IMM5:
            sprintf(param, "VI%02d,VI%02d,0x%08x", get_int_reg(code, vis),
                    get_int_reg(code, vit), get_imm5(code));
            break;
        case PARAM_VFT_R:
            sprintf(param, "VF%02d, R", get_float_reg(code, vft));
            break;
        default:
            break;
    }
}

int
get_int_reg(uint32 code, uint32 reg) {
    return (code>>reg)&0x1f;
}

int
get_float_reg(uint32 code, uint32 reg) {
    return (code>>reg)&0x1f;
}

char *
get_ftf(uint32 code) {
    char *ftf[4] = {"x", "y", "z", "w"};
    return ftf[(code>>23)&0x3];
}

char *
get_fsf(uint32 code) {
    char *fsf[4] = {"x", "y", "z", "w"};
    return fsf[(code>>21)&0x3];
}

int
get_imm5(uint32 code) {
    if ( ((code>>6)&0x10) == 0x10) {
        return (int)(code>>6)|0xfffffff0;
    }
    return (code>>6)&0x1F;
}

int
get_imm11(uint32 code) {
    if ( (code & 0x400) == 1024 ) {
        return (int)code|0xfffff800;
    }
    return (int)code&0x7ff;
}

int
get_imm12(uint32 code) {
    return ((code>>10)&0x0400)+((code)&0x7FF);
}

int
get_imm15(uint32 code) {
    return ((code>>10)&0x3C00)+((code)&0x7FF);
}

int
get_imm24(uint32 code) {
    return (code&0xffffff);
}

int
get_imm32(uint32 code) {
    return code;
}

int
get_lower(uint32 code) {
    int i;
    uint32 hold = code&0xFFE007FF;
    uint32 hold2 = code&0xFFe00000;
    uint32 hold3 = code&0xFFe0003F;
    uint32 hold4 = code&0xFE0007FF;
    if (ibit == 1) {
        hold = 0x0; 
        for(i = 0; i < NUM_LOWER; i++) {
            if ( hold == lopcodes[i].opcode) {
                return i;
            }
        }
    }
    for(i = 0; i < NUM_LOWER; i++) {
        if ( hold == lopcodes[i].opcode ||
             hold2 == lopcodes[i].opcode ||
             hold3 == lopcodes[i].opcode ||
             hold4 == lopcodes[i].opcode
            ) {
            return i;
        }
    }
    hold = code&0x10000000;
    for(i = 0; i < NUM_LOWER; i++) {
        if ( hold == lopcodes[i].opcode){
            return i;
        }
    }
    // insert nop coz it could still be valid data afterwards
    if (code == 0x0) {
        hold = 0x8000033C;
        for(i = 0; i < NUM_LOWER; i++) {
            if ( hold == lopcodes[i].opcode) {
                return i;
            }
        }
    }
    return -1;
}

int
get_upper(uint32 code) {
	int i;
    uint32 hold  = code&0x01E003FF;          // 11 bit opcodes
    uint32 hold2 = code&0x01E003FC;          // 9 bit bc opcodes
    uint32 hold3 = code&0x01E0003F;
    if ( code == 0x8000033C ) {
        hold = 0x000002FF;
        for(i = 0; i < NUM_UPPER; i++) {
            if ( hold == uopcodes[i].opcode) {
                return i;
            }
        }
    }
    ibit = (code&0x80000000)>>31;
    ebit = (code&0x40000000)>>30;
    mbit = (code&0x20000000)>>29;
    dbit = (code&0x10000000)>>28;
    tbit = (code&0x08000000)>>27;
    for(i = 0; i < NUM_UPPER; i++) {
        if ( hold == uopcodes[i].opcode) {
            return i;
        }
    }
    for(i = 0; i < NUM_UPPER; i++) {
        if ( hold2 == uopcodes[i].opcode) {
            return i;
        }
    }
    for(i = 0; i < NUM_UPPER; i++) {
        if ( hold3 == uopcodes[i].opcode ) {
            return i;
        }
    }
    // insert nop coz it could still be valid data afterwards
    if (code == 0x0) {
        hold = 0x000002FF;
        for(i = 0; i < NUM_UPPER; i++) {
            if ( hold == uopcodes[i].opcode) {
                return i;
            }
        }
    }
    return -2;
}

// insert the upper and lower instruction at a given index
int
insert(char *upper, char *lower, char *uparam, char *lparam, uint32 index) {
    int i = 0, j = 0, k, InstIndex, flavor = 0;
    char dest[50], token[50], flg;

    strupr(upper);
    strupr(lower);
    VUchip.program[index].invalid = 0;
    if(!IsValidInstruction(upper, UPPER, &InstIndex, &flavor, dest, &flg)) {
        VUchip.program[index].invalid = 1;
        return 0;
    }
    VUchip.program[index].InstIndex[UPPER] = InstIndex;
    strcpy(VUchip.program[index].dest[UPPER],dest);
    VUchip.program[index].flavor[UPPER] = flavor;
    VUchip.program[index].breakpoint = 0;
    VUchip.program[index].flg = flg;
    VUchip.program[index].SymbolIndex = -1;
    for (j=0; j<Instr.Instr[InstIndex].operands; j++) {
        memset(token, 0, 50);
        if(j==Instr.Instr[InstIndex].operands-1) { //last operand
            //first eliminate heading blanks
            while ((uparam[i]==' ' || uparam[i]=='\t') && uparam[i]!=0) i++;
            k=i;
            while (uparam[i]!=' ' && uparam[i]!='\t' && uparam[i]!=0) i++;
            strncpy(token,uparam+k,i-k);
            token[i-k]=0;  //parameter grabbed in token
            SetParam(VUchip.program[index],j,UPPER,token);
        }
        else { //intermediate operand, search for a ','
            while ((uparam[i]==' ' || uparam[i]=='\t') && uparam[i]!=0) i++;
            k=i;
            while(uparam[i]!=',' && uparam[i]) i++;
            strncpy(token,uparam+k,i-k);
            token[i-k]=0; //parameter grabbed in token
            SetParam(VUchip.program[index],j,UPPER,token);
            if(uparam[i]==',') i++;
        }
    }

    if(!IsValidInstruction(lower, LOWER, &InstIndex, &flavor,dest, &flg)) {
        VUchip.program[index].invalid = 1;
        return 0;
    }
    VUchip.program[index].InstIndex[LOWER] = InstIndex;
    strcpy(VUchip.program[index].dest[LOWER], dest);
    VUchip.program[index].flavor[LOWER] = flavor;
    i = 0;
    for (j=0; j<Instr.Instr[InstIndex].operands; j++) {
        memset(token, 0, 50);
        if(j==Instr.Instr[InstIndex].operands-1) { //last operand
            //first eliminate heading blanks
            while ((lparam[i]==' ' || lparam[i]=='\t') && lparam[i]!=0) i++;
            k=i;
            while (lparam[i]!=' ' && lparam[i]!='\t' && lparam[i]!=0) i++;
            strncpy(token,lparam+k,i-k);
            token[i-k]=0;  //parameter grabbed in token
            SetParam(VUchip.program[index],j,LOWER,token);
        }
        else { //intermediate operand, search for a ','
            while ((lparam[i]==' ' || lparam[i]=='\t') && lparam[i]!=0) i++;
            k=i;
            while(lparam[i]!=',' && lparam[i]) i++;
            strncpy(token,lparam+k,i-k);
            token[i-k]=0; //parameter grabbed in token
            SetParam(VUchip.program[index],j,LOWER,token);
            if(lparam[i]==',') i++;
        }
    }
    return 1;
}

// disasm class
void
dlower(uint32 *lower, char *low, char *lparam ) {
    uint32 lidx;
    bzero(lparam, 50);
    lidx = get_lower(*lower);
    get_params(lidx, *lower, lparam, lopcodes);
    strcpy(low, lopcodes[lidx].name);
}

void
dupper(uint32 *upper, char *upp, char *uparam ) {
    uint32 uidx;
    // char uparam[50];
    bzero(uparam, 50);
    uidx = get_upper(*upper);
    get_params(uidx, *upper, uparam, uopcodes);
    strcpy(upp, uopcodes[uidx].name);
    if ( ebit == 1 ) {
        strcat(upp, "[e]");
    } else if ( dbit == 1 ) {
        strcat(upp, "[d]");
    }
}
