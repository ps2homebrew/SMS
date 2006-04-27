/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
.set noat
.set noreorder
.set nomacro

.globl MUL64

.text

MUL64:
    dsll32      $v0, $a0, 0
    dsra32      $a2, $a1, 0
    dsra32      $v0, $v0, 0
    dsll32      $a1, $a1, 0
    mult        $zero, $v0, $a2
    dsra32      $a1, $a1, 0
    multu1      $zero, $v0, $a1
    dsra32      $a0, $a0, 0
    madd        $a0, $a0, $a1
    pmfhl.lw    $at
    pcpyud      $at, $at, $zero
    move        $v0, $at
    dsra32      $a2, $v0, 0
    dsll32      $v0, $v0, 0
    addu        $a2, $a2, $a0
    dsrl32      $v0, $v0, 0
    dsll32      $a2, $a2, 0
    jr          $ra
    or          $v0, $v0, $a2
