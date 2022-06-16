/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Based on Hermes's (PS2Reality) example
# (c) 2005-2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
.set noreorder
.set nomacro

.globl g_Timer
.globl SMS_TimerInit
.globl SMS_TimerDestroy
.globl SMS_TimerSet
.globl SMS_iTimerSet
.globl SMS_TimerReset
.globl SMS_iTimerReset
.globl SMS_TimerWait
.globl SMS_SetAlarm

.section ".sbss"

.align 4
g_Timer    : .space 8
s_HandlerID: .space 8
s_SemaID   : .space 8
s_Handlers : .space 16 * 4
s_AlarmID  : .space 4
s_AlarmHdlr: .space 4
s_AlarmParm: .space 4

.text

SMS_TimerInit:
    addiu   $sp, $sp, -48
    lui     $t0, 0x1000
    sw      $ra, 0($sp)
    addiu   $v0, $zero, 0x01C2
    addiu   $v1, $zero, 0x1200
    addiu   $a0, $zero, 0x016F  # 0x0143  # gate enable, falling edge -> 0x016F
    addiu   $a1, $zero, 0x0001
    sd      $zero, g_Timer
    sw      $zero, 0x0000($t0)
    sw      $v0, 0x0010($t0)
    sw      $a0, 0x0810($t0)
    sw      $a1, 0x0820($t0)
    lui     $a1, %hi( _timer_handler )
    sw      $v1, 0x0020($t0)
    addiu   $a1, $a1, %lo( _timer_handler )
    addu    $a2, $zero, $zero
    jal     AddIntcHandler
    addu    $a0, $zero, 9
    sw      $v0, s_HandlerID
    addiu   $a0, $zero, 10
    lui     $a1, %hi( _alarm_handler )
    xor     $a2, $a2, $a2
    jal     AddIntcHandler
    addiu   $a1, $a1, %lo( _alarm_handler )
    sw      $v0, s_AlarmID
    sw      $zero, 12($sp)
    jal     CreateSema
    addu    $a0, $sp, 4
    sw      $v0, s_SemaID
    addu    $a0, $zero, 9
    lw      $ra, 0($sp)
    j       EnableIntc
    addiu   $sp, $sp, 48

SMS_TimerDestroy:
    addiu   $sp, $sp, -16
    sw      $ra, 0($sp)
    jal     DisableIntc
    addiu   $a0, $zero, 9
    jal     DisableIntc
    addiu   $a0, $zero, 10
    jal     DeleteSema
    lw      $a0, s_SemaID
    lw      $a1, s_HandlerID
    jal     RemoveIntcHandler
    addiu   $a0, $zero, 9
    lw      $ra, 0($sp)
    lw      $a1, s_AlarmID
    addiu   $a0, $zero, 10
    j       RemoveIntcHandler
    addiu   $sp, $sp, 16

SMS_TimerSet:
    .set    macro
    la      $t0, s_Handlers
    .set    nomacro
    sll     $a0, $a0, 4
    lui     $v1, 0x0001
    addu    $t0, $t0, $a0
1:
    di
    sync.p
    mfc0    $v0, $12
    and     $v0, $v0, $v1
    bne     $v0, $zero, 1b
    nop
    sw      $a2, 0($t0)
    sw      $a3, 4($t0)
    sd      $a1, 8($t0)
    jr      $ra
    ei

SMS_iTimerSet:
    .set    macro
    la      $t0, s_Handlers
    .set    nomacro
    sll     $a0, $a0, 4
    addu    $t0, $t0, $a0
    sw      $a2, 0($t0)
    sw      $a3, 4($t0)
    jr      $ra
    sd      $a1, 8($t0)

SMS_TimerReset:
    .set    macro
    la      $t0, s_Handlers
    .set    nomacro
    sll     $a0, $a0, 4
    lui     $v1, 0x0001
    addu    $t0, $t0, $a0
1:
    di
    sync.p
    mfc0    $v0, $12
    and     $v0, $v0, $v1
    bne     $v0, $zero, 1b
    nop
    bne     $a1, $zero, 1f
    lw      $v0, 0($t0)
2:
    sd      $zero, 0($t0)
    jr      $ra
    ei
1:
    lw      $v1, 4($t0)
    beq     $zero, $zero, 2b
    sw      $v1, 0($a1)
    
SMS_iTimerReset:
    .set    macro
    la      $t0, s_Handlers
    .set    nomacro
    sll     $a0, $a0, 4
    addu    $t0, $t0, $a0
    jr      $ra
    sd      $zero, 0($t0)

SMS_TimerWait:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    addu    $a1, $zero, $a0
    addiu   $a0, $zero, 3
    lui     $a2, %hi( iSignalSema )
    addiu   $a2, $a2, %lo( iSignalSema )
    jal     SMS_TimerSet
    lw      $a3, s_SemaID
    jal     WaitSema
    lw      $a0, s_SemaID
    lw      $ra,  0($sp)
    jr      $ra
    addiu   $sp, $sp, 16

_timer_handler:
    lui     $t0, 0x1000
    ld      $v0, g_Timer
    lw      $v1, s_Handlers
    lw	    $a1, 0x0010($t0)
    addiu   $sp, $sp, -16
    daddiu  $v0, $v0, 8
    ori     $a1, $a1, 0x0400
    sw      $ra, 0($sp)
    sw      $a1, 0x0010($t0)
    beq     $v1, $zero, 1f
    sd      $v0, g_Timer
    ld      $a0, s_Handlers + 8
    daddiu  $a0, $a0, -8
    bgtzl   $a0, 1f
    sd      $a0, s_Handlers + 8
    lw      $a0, s_Handlers + 4
    jalr    $v1
    sd      $zero, s_Handlers
1:
    lw      $v1, s_Handlers + 16
    beq     $v1, $zero, 2f
    ld      $a0, s_Handlers + 24
    daddiu  $a0, $a0, -8
    bgtzl   $a0, 2f
    sd      $a0, s_Handlers + 24
    lw      $a0, s_Handlers + 20
    jalr    $v1
    sd      $zero, s_Handlers + 16
2:
    lw      $v1, s_Handlers + 32
    beq     $v1, $zero, 3f
    ld      $a0, s_Handlers + 40
    daddiu  $a0, $a0, -8
    bgtzl   $a0, 3f
    sd      $a0, s_Handlers + 40
    lw      $a0, s_Handlers + 36
    jalr    $v1
    sd      $zero, s_Handlers + 32
3:
    lw      $v1, s_Handlers + 48
    beq     $v1, $zero, 4f
    ld      $a0, s_Handlers + 56
    daddiu  $a0, $a0, -8
    bgtzl   $a0, 4f
    sd      $a0, s_Handlers + 56
    lw      $a0, s_Handlers + 52
    jalr    $v1
    sd      $zero, s_Handlers + 48
4:
    lw      $ra, 0($sp)
    jr      $ra
    addiu   $sp, $sp, 16

_alarm_handler:
    addiu   $sp, $sp, -16
    lw      $v0, s_AlarmHdlr
    sw      $ra, 0($sp)
    jalr    $v0
    lw      $a0, s_AlarmParm
    lui     $t0, 0x1000
    lw      $a0, 0x0810($t0)
    lw      $ra, 0($sp)
    ori     $a0, $a0, 0x0400
    sw      $a0, 0x0810($t0)
    jr      $ra
    addiu   $sp, $sp, 16

SMS_SetAlarm:
    lui     $v1, 0x0001
1:
    di
    sync.p
    mfc0    $v0, $12
    and     $v0, $v0, $v1
    bne     $v0, $zero, 1b
    nop
    lui     $t0, 0x1000
    sw      $a1, s_AlarmHdlr
    sw      $a2, s_AlarmParm
    sw      $zero, 0x0800($t0)
    sw      $a0, 0x0820($t0)
    beqzl   $a1, 1f
    lui     $v0, %hi( DisableIntc )
    lw      $a0, 0x0810($t0)
    lui     $v0, %hi( EnableIntc )
    ori     $a0, $a0, 0x0080
    beq     $zero, $zero, 2f
    addiu   $v0, $v0, %lo( EnableIntc )
1:
    lw      $a0, 0x0810($t0)
    lui     $a2, 0xFFFF
    addiu   $v0, $v0, %lo( DisableIntc )
    ori     $a2, $a2, 0xFF7F
    and     $a0, $a0, $a2
2:
    sw      $a0, 0x0810($t0)
    ei
    jr      $v0
    addiu   $a0, $zero, 10
