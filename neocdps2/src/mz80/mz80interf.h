/************************
*** Z80 CPU Interface ***
***    Header File    ***
************************/
#ifndef	MZ80INTERF_H
#define MZ80INTERF_H

#include "z80.h" 

#define Z80_MEMSIZE	0x10000 

inline void cpu_z80_init(void);
inline void cpu_z80_nmi(void);
inline void cpu_z80_raise_irq(int l);
inline void cpu_z80_lower_irq(void);
inline void cpu_z80_run(int nbcycle);
inline uint8 z80_port_read(uint16 PortNo);
inline void z80_port_write(uint16 PortNb, uint8 Value);

extern uint8 mame_z80mem[Z80_MEMSIZE]  __attribute__((aligned(64)));
extern int		sound_code;
extern int		pending_command;
extern int		result_code;

#endif /* Z80INTRF_H */

