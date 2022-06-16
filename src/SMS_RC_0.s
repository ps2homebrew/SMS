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
.set nomacro
.set volatile

.globl g_RCData
.globl RC_ReadDummy
.globl RC_ReadX
.globl RC_ReadI
.globl RC_SetTranslator
.globl RC_TransPAD
.globl RC_TransRC

.section ".sbss"
.align 6
g_RCData:       .space  256

.section ".sdata"
RC_Translator:  .word   RC_DefTranslator

.text

RC_ReadDummy:
    jr      $ra
    xor     $v0, $v0, $v0

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
_rc_def_translator:
    addiu   $t0, $zero, 0x05DF
    andi    $v1, $a0, 0x0FFF
    beql    $v1, $t0, 1f
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

RC_TransPAD:
    pcpyld  $ra, $ra, $ra
    bgezal  $zero, _rc_def_translator
    ori     $a1, $zero, 0x97DF
    ori     $a2, $zero, 0xA7DF
    ori     $a3, $zero, 0xB7DF
    ori     $t0, $zero, 0xC7DF
    beq     $v0, $zero, 1f
    pcpyud  $ra, $ra, $ra
    beql    $v0, $a1, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $a2, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $a3, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $t0, 1f
    xor     $v0, $v0, $v0
1:
    jr      $ra
    nop

RC_TransRC:
    pcpyld  $ra, $ra, $ra
    bgezal  $zero, _rc_def_translator
    or      $a1, $zero, 0x0010
    or      $a2, $zero, 0x0020
    or      $a3, $zero, 0x0040
    or      $t0, $zero, 0x0080
    beq     $v0, $zero, 1f
    pcpyud  $ra, $ra, $ra
    beql    $v0, $a1, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $a2, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $a3, 1f
    xor     $v0, $v0, $v0
    beql    $v0, $t0, 1f
    xor     $v0, $v0, $v0
    ori     $a1, $zero, 0x97DF
    ori     $a2, $zero, 0xA7DF
    ori     $a3, $zero, 0xB7DF
    ori     $t0, $zero, 0xC7DF
    beql    $v0, $a1, 1f
    ori     $v0, $zero, 0x0010
    beql    $v0, $a2, 1f
    ori     $v0, $zero, 0x0040
    beql    $v0, $a3, 1f
    ori     $v0, $zero, 0x0080
    beql    $v0, $t0, 1f
    ori     $v0, $zero, 0x0020
1:
    jr      $ra
    nop
