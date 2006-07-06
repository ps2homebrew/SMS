#
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
#
.set noreorder
.set noat
.set nomacro
.set volatile

.globl g_RCData
.globl RC_ReadDummy
.globl RC_ReadX
.globl RC_ReadI
.globl RC_SetTranslator

.section ".sbss"
.align 6
g_RCData:       .space  256

.section ".sdata"
RC_Translator:  .word   RC_DefTranslator

.text

RC_ReadDummy:
    jr      $ra
    addu    $v0, $zero, $zero

RC_ReadX:
    la      $a0, g_RCData
2:
    lui     $v1, 0x2000
    or      $a0, $a0, $v1
    lw      $v0, 4($a0)
    lui     $a0, 0x00FF
    ori     $a0, $a0, 0xFFFF
    beql    $v0, $a0, 1f
    addu    $v0, $zero, $zero
3:
    beq     $v0, $zero, 1f
    andi    $a0, $v0, 0x07FF
    lw      $v1, RC_Translator
    srl     $v0, $v0, 8
    jr      $v1
    or      $a0, $a0, $v0
1:
    jr      $ra
    nop

RC_ReadI:
    la      $a0, g_RCData
    lui     $v1, 0x2000
    or      $a0, $a0, $v1
    lw      $v0, 0($a0)
    beq     $zero, $zero, 3b
    srl     $v0, $v0, 8

RC_DefTranslator:
    addiu   $at, $zero, 0x05DF
    andi    $v1, $a0, 0x05DF
    beql    $v1, $at, 1f
    srl     $a0, $a0, 12
    jr      $ra
    addu    $v0, $zero, $a0
1:
    addiu   $v0, $zero, 1
    jr      $ra
    sllv    $v0, $v0, $a0

RC_SetTranslator:
    lw      $v0, RC_Translator
    jr      $ra
    sw      $a0, RC_Translator
