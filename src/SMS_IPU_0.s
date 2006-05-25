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
.set noreorder
.set noat
.set nomacro

.globl IPU_FDEC
.globl IPU_IDEC

.text

IPU_FDEC:
    lui  $at, 0x4000
    lui  $v1, 0x1000
    or   $at, $at, $a0
    sw   $at, 0x2000($v1)
1:
    lw      $at, 0x2010($v1)
    nop
    nop
    nop
    nop
    nop
    bltz    $at, 1b
    nop
    jr      $ra
    lw      $v0, 0x2000($v1)

IPU_IDEC:
    lui     $v1, 0x1000
    sll     $t1, $t1, 27
    sll     $t0, $t0, 26
    or      $a0, $a0, $t1
    sll     $a3, $a3, 25
    or      $a0, $a0, $t0
    sll     $a2, $a2, 24
    or      $a0, $a0, $a3
    sll     $a1, $a1, 16
    or      $a0, $a0, $a2
    or      $a0, $a0, $v1
    or      $a0, $a0, $a1
    sw      $a0, 0x2000($v1)
    lw      $at, 0x2010($v1)
1:
    andi     $v0, $at, 0x4000
    bgtzl    $v0, 1f
    move     $v0, $zero
    andi     $v0, $at, 0x00F0
    bgtzl    $v0, 1f
    srl      $v0, $v0, 4
    b        1b
    lw       $at, 0x2010($v1)
1:
    jr      $ra
    nop
