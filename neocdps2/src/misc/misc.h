#ifndef _MISC_H
#define _MISC_H 

void swab( const void* src1, const void* src2, int isize);

uint32 PS2_InitTicks();
uint32 PS2_StartTicks();
uint32 PS2_GetTicks(void);
void PS2_Delay(uint32 ms);
#endif
