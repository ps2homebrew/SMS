// (C) 2004 by Khaled Daham, <khaled@w-arts.com>
#include "VuReg.h"

int VuReg::stall(){ return vstall;}
int VuReg::lastRead(){ return lRead;}
int VuReg::lastWrite(){ return lWrite;}
void VuReg::stall(int a){ vstall=a;}
void VuReg::decstall(){ if(vstall) vstall--;}
void VuReg::lastRead(int a){ lRead=a;}
void VuReg::lastWrite(int a){ lWrite=a;}


