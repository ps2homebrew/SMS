/********************************************************************************/
/*                                                                              */
/* CZ80 exec include source file                                                */
/* C Z80 emulator version 0.91                                                  */
/* Copyright 2004-2005 Stéphane Dallongeville                                   */
/*                                                                              */
/********************************************************************************/

s32 FASTCALL Cz80_Exec(cz80_struc *cpu, s32 cycles)
{
#if CZ80_USE_JUMPTABLE
    #include "cz80jmp.inc"
#endif

#if 0
    register cz80_struc *CPU asm ("edi");
    register u32 PC asm ("ebx");
    register s32 CCnt asm ("esi");
//    register u32 Opcode asm ("eax");
    register u32 Opcode;
#elif 0
    register cz80_struc *CPU;
    register u32 PC;
    register s32 CCnt;
    register u32 Opcode;
#else
    cz80_struc *CPU;
    u32 PC;
    s32 CCnt;
    u32 Opcode;
#endif

    CPU = cpu;
    PC = CPU->PC;

    if (CPU->Status & (CZ80_RUNNING | CZ80_DISABLE | CZ80_FAULTED))
    {
        return (CPU->Status | 0x80000000);
    }

#if CZ80_SIZE_OPT
    CPU->CycleToDo = CPU->CycleSup = cycles;
    CCnt = 0;
#else
    CPU->CycleToDo = CCnt = cycles;
    CPU->CycleSup = 0;
#endif
    CPU->Status |= CZ80_RUNNING;
    
#if CZ80_SIZE_OPT
Cz80_Exec_Check:
    if (CCnt <= 0) goto Cz80_Exec_End;
#else
    goto Cz80_Exec_End;
#endif

Cz80_Exec:
    {
        union16 *data = pzHL;
        Opcode = FETCH_BYTE;
        #include "cz80_op.inc"
    }

Cz80_Exec_End:
    CHECK_INT
    if ((CCnt += CPU->CycleSup) > 0)
    {
        CPU->CycleSup = 0;
        if (!(CPU->Status & CZ80_HALTED)) goto Cz80_Exec;

        // CPU halted
        CCnt = 0;
    }

Cz80_Exec_Really_End:
    CPU->Status &= ~CZ80_RUNNING;
    CPU->PC = PC;
    
    // number of executed cycles
    CCnt = CPU->CycleToDo - CCnt;
    
    // update R register
    zR = (zR + (CCnt >> 2)) & 0x7F;

    return CCnt;
}
