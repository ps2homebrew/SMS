// 68K CPU interface

#include "cpu68k.h"

#include "../neocd.h"

#ifdef CPU68K_USE_C68K
#include "../c68k/c68k.h"
#endif
#ifdef CPU68K_USE_MUSASHI
#include "../mc68000/m68k.h"
#endif

// MUSASHI DEBUG MEMORY HANDLER

unsigned int m68k_read_disassembler_8(unsigned int address)
{
    return m68k_read_memory_8(address);
}

unsigned int m68k_read_disassembler_16(unsigned int address)
{
    return m68k_read_memory_16(address);
}

unsigned int m68k_read_disassembler_32(unsigned int address)
{
    return m68k_read_memory_32(address);
}

/********************************************/

INLINE void M68K_Init(void)
{
#ifdef CPU68K_USE_MUSASHI
    // MUSASHI
    m68k_set_cpu_type(M68K_CPU_TYPE_68000);
    m68k_pulse_reset();
#endif

#ifdef CPU68K_USE_C68K
    // C68K
    C68k_Init(&C68K, NULL);

    C68k_Set_ReadB(&C68K, m68k_read_memory_8);
    C68k_Set_ReadW(&C68K, m68k_read_memory_16);

    C68k_Set_WriteB(&C68K, m68k_write_memory_8);
    C68k_Set_WriteW(&C68K, m68k_write_memory_16);
    
    C68k_Set_Fetch(&C68K, 0x000000, 0x1FFFFF, (u32) neogeo_prg_memory);
    C68k_Set_Fetch(&C68K, 0xC00000, 0xC7FFFF, (u32) neogeo_rom_memory);
    
    C68k_Reset(&C68K);
#endif
}

INLINE void M68K_Reset(void)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_pulse_reset();
#endif

#ifdef CPU68K_USE_C68K
    C68k_Reset(&C68K);
#endif
}

INLINE void M68K_Exec(s32 cycles)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_execute(cycles);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Exec(&C68K, cycles);
#endif
}

INLINE void M68K_SetIRQ(u32 level)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_irq(level);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_IRQ(&C68K, level);
#endif
}

INLINE u32 M68K_GetOdo(void)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_cycles_run();
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_CycleDone(&C68K);
#endif
}

INLINE void M68K_EndExec(void)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_end_timeslice();
#endif

#ifdef CPU68K_USE_C68K
    C68k_Release_Cycle(&C68K);
#endif
}


INLINE void M68K_SetFetch(u32 low_adr, u32 high_adr, u32 fetch_adr)
{
    // MUSASHI
    
    
    // C68K
#ifdef CPU68K_USE_C68K
    C68k_Set_Fetch(&C68K, low_adr, high_adr, fetch_adr);
#endif
}


INLINE u32 M68K_GetDReg(u32 num)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_D0 + num);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_DReg(&C68K, num);
#endif
}

INLINE u32 M68K_GetAReg(u32 num)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_A0 + num);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_AReg(&C68K, num);
#endif
}

INLINE u32 M68K_GetSP(void)
{
    return M68K_GetAReg(7);
}

INLINE u32 M68K_GetPC(void)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_PC);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_PC(&C68K);
#endif
}

INLINE u32 M68K_GetSR(void)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_SR);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_SR(&C68K);
#endif
}

INLINE u32 M68K_GetMSP(void)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_ISP);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_MSP(&C68K);
#endif
}

INLINE u32 M68K_GetUSP(void)
{
#ifdef CPU68K_USE_MUSASHI
    return m68k_get_reg(NULL, M68K_REG_USP);
#endif

#ifdef CPU68K_USE_C68K
    return C68k_Get_USP(&C68K);
#endif
}

INLINE void M68K_SetDReg(u32 num, u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_D0 + num, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_DReg(&C68K, num, val);
#endif
}

INLINE void M68K_SetAReg(u32 num, u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_A0 + num, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_AReg(&C68K, num, val);
#endif
}

INLINE void M68K_SetSP(u32 val)
{
    M68K_SetAReg(7, val);
}

void M68K_SetPC(u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_PC, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_PC(&C68K, val);
#endif
}

INLINE void M68K_SetSR(u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_SR, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_SR(&C68K, val);
#endif
}

INLINE void M68K_SetMSP(u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_ISP, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_MSP(&C68K, val);
#endif
}

INLINE void M68K_SetUSP(u32 val)
{
#ifdef CPU68K_USE_MUSASHI
    m68k_set_reg(M68K_REG_USP, val);
#endif

#ifdef CPU68K_USE_C68K
    C68k_Set_USP(&C68K, val);
#endif
}
/*
u8 *M68K_Disassemble(u32 *PC)
{
    static u8 debug_buf[256];

    *PC += m68k_disassemble((char*) debug_buf, *PC, M68K_CPU_TYPE_68000);
    return debug_buf;
}
*/

