// (C) 2004 by Khaled Daham, <khaled@w-arts.com>

#include "MicroCode.h"
void MicroCode::DecThroughput()
{
    int i, j;
    //well in fact this is wrong
    //the broadcast flavors are managed separate
    //when they should use the same instruction throughput
    //counter but :) no matter cause throughput for all
    //broacast instrcutions are 1 ;)

    for(i=0; i< nInstructionDef; i++)
        for(j=0;j<15;j++)
            if(Instr[i].lastthr[j])
                Instr[i].lastthr[j]--;
}
