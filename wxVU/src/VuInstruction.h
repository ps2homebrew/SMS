// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#ifndef __VUINSTRUCTION__
#define __VUINSTRUCTION__

#include "VuParam.h"

class VuInstruction {
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
    VuParam Params[2][4];		//parameters
};
#endif
