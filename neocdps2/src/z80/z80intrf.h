/************************
*** Z80 CPU Interface ***
***    Header File    ***
************************/

#ifndef	Z80INTRF_H
#define Z80INRTF_H

#include "mz80.h"

void z80_init(void);


extern UINT8		subcpu_memspace[65536]; //__attribute__((aligned(8)));
extern int		sound_code;
extern int		pending_command;
extern int		result_code;
extern int		z80_cycles;

#endif /* Z80INTRF_H */

