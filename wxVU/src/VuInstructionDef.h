// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#ifndef __VUINSTRUCTIONDEF__
#define __VUINSTRUCTIONDEF__
class VUInstructionDef {
public:
        int     mode;               //uppder-lower-macromode
        int     sufix;              //diferent flavors
        char    nemot[15];          //instruction
        int     operands;           //number of operands
        char    types[4][50];       //type of operands (by order)
        int     throughput;         //see VU manual
        int     latency;            //
        int     lastthr[15];        //one for each flavor
        friend class VuInstruction;
};
#endif
