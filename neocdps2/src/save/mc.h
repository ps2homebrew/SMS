/**************************************
 ******    MC.C  -  Memory Card   *****
 ****         Header File           ***
**************************************/

#ifndef	MC_H
#define MC_H 


#define SAVE_SIZE 8192

#define MC_SAVE_PATH "mc0:NEOCDPS2"
#define MC_SAVE_FILE "mc0:NEOCDPS2/neocd.save"

extern unsigned char neogeo_memorycard[SAVE_SIZE] __attribute__((aligned(64))) __attribute__ ((section (".bss")));

void initSave();
void writeSave();

#endif
