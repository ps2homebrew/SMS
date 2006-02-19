#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) 200X by Hermes (his code was found at PS2Dev forums posted on Tue Jul 13, 2004 8:06 pm)
# Copyright (c) 2005 by Eugene Plotnikov (T1_XXXX stuff)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#

.set noreorder

.globl g_Timer
.globl g_TimerHandler
.globl T0_Handler

.sdata

g_Timer:        .dword  0
g_TimerHandler: .word   0
                .word   0
                .word   0
.text

.ent T0_Handler
T0_Handler:
    ld      $t0, g_Timer
    subu    $sp, $sp, 16
    daddiu  $t0, $t0, 8
    andi    $t1, $t0, 0x003F
    sd      $t0, g_Timer
    bnez    $t1, 1f
    nop
    sw      $ra, 0($sp)
    lw      $t1, g_TimerHandler + 0
    beqz    $t1, 2f
    nop
    jalr    $t1
    nop
2:
    ld	    $t0, g_Timer
    andi    $t0, $t0, 0x03FF
    bnez    $t0, 4f
    lw      $t1, g_TimerHandler + 4
    beqz    $t1, 3f
    nop
    jalr    $t1
    nop
3:
    lw      $t1, g_TimerHandler + 8
    beqz    $t1, 4f
    nop
    jalr    $t1
    nop
4:
    lw      $ra, 0($sp)
1:
    lui	    $v0, 0x1000
    ori	    $v0, 0x0010
    lw	    $t0, 0($v0)
    addu    $sp, $sp, 16
    ori	    $t0, $t0, 0x0400
    jr      $ra
    sw	    $t0, 0($v0)
.end T0_Handler
