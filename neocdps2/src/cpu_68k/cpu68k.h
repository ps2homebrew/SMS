/*
 *  cpu68k.h - cpu 68k Interface
 *  Copyright (C) 2004-2005 Olivier "Evilo" Biot (PS2 Port)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _CPU68K_H
#define _CPU68K_H

#include <tamtypes.h>


#define CPU68K_AUTOVECTOR_BASE_EX   24
#define CPU68K_INT_ACK_AUTOVECTOR   -1


typedef u32  CPU68K_READ(const u32 adr);
typedef void CPU68K_WRITE(const u32 adr, u32 data);

typedef u32   CPU68K_INT_CALLBACK(u32 level);
typedef void  CPU68K_RESET_CALLBACK(void);

void M68K_GetContext(void);
void M68K_SetContext(void);

u32 M68K_GetCurrentCore(void);
void M68K_SetCurrentCore(u32 core);

inline void  M68K_Init(void);
inline void  M68K_Reset(void);

inline void  M68K_Exec(s32 cycles);

inline void M68K_SetIRQ(u32 level);

inline u32  M68K_GetOdo(void);
inline void M68K_EndExec(void);

inline void M68K_SetFetch(u32 low_adr, u32 high_adr, u32 fetch_adr);

inline u32  M68K_GetDReg(u32 num);
inline u32  M68K_GetAReg(u32 num);
inline u32  M68K_GetSP(void);
inline u32  M68K_GetPC(void);
inline u32  M68K_GetSR(void);
inline u32  M68K_GetMSP(void);
inline u32  M68K_GetUSP(void);

inline void M68K_SetAReg(u32 num, u32 val);
inline void M68K_SetDReg(u32 num, u32 val);
inline void M68K_SetSP(u32 val);
inline void M68K_SetPC(u32 val);
inline void M68K_SetSR(u32 val);
inline void M68K_SetMSP(u32 val);
inline void M68K_SetUSP(u32 val);
/*
u8  *M68K_Disassemble(u32 *PC);
*/
#endif	/* CPU68K_H */
