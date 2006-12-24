#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
.set noreorder
.set nomacro

.globl ip_cksum

.text

ip_cksum:
    andi    $v1, $a0, 0x3
    move    $t0, $zero
    move    $t1, $zero
    move    $t2, $zero
    beqz    $v1, 1f
    move    $t3, $zero
    slti    $v1, $a1, 3
    bnez    $v1, 2f
    move    $t6, $a1
    andi    $a2, $a0, 0x1
    beqz    $a2, 3f
    andi    $t4, $a0, 0x2
    lbu     $a3, 0($a0)
    addiu   $a1, $a1, -1
    sll     $t2, $a3, 8
    addiu   $a0, $a0, 1
    li      $t3, 1
    andi    $t4, $a0, 0x2
3:
    beqz    $t4, 4f
    slti    $t6, $a1, 72
    lhu     $t5, 0($a0)
    addiu   $a1, $a1, -2
    addu    $t2, $t2, $t5
    addiu   $a0, $a0, 2
1:
    slti    $t6, $a1, 72
4:
    bnez    $t6, 5f
    slti    $a3, $a1, 4
    lw      $a2, 0($a0)
    lw      $a3, 4($a0)
6:
    addu    $v1, $t0, $a2
    lw      $t9, 8($a0)
    srl     $t4, $a2, 16
    addu    $a2, $t1, $t4
    addu    $v0, $v1, $a3
    lw      $t6, 12($a0)
    srl     $t8, $a3, 16
    addu    $t7, $a2, $t8
    srl     $a3, $t9, 16
    addu    $t5, $v0, $t9
    lw      $t9, 16($a0)
    addu    $v0, $t5, $t6
    srl     $t4, $t6, 16
    addu    $t8, $t7, $a3
    lw      $t6, 20($a0)
    addu    $t7, $t8, $t4
    addu    $v1, $v0, $t9
    lw      $t4, 24($a0)
    srl     $a3, $t9, 16
    srl     $a2, $t6, 16
    addu    $t5, $t7, $a3
    addu    $t8, $v1, $t6
    lw      $t7, 28($a0)
    addu    $t6, $t8, $t4
    srl     $t9, $t4, 16
    addu    $a3, $t5, $a2
    lw      $t4, 32($a0)
    addu    $v0, $t6, $t7
    srl     $a2, $t7, 16
    addu    $t5, $a3, $t9
    lw      $t7, 36($a0)
    srl     $t8, $t4, 16
    addu    $t9, $t5, $a2
    addu    $v1, $v0, $t4
    lw      $t5, 40($a0)
    srl     $t6, $t7, 16
    addu    $a2, $t9, $t8
    addu    $t4, $v1, $t7
    lw      $t9, 44($a0)
    addu    $t7, $t4, $t5
    srl     $a3, $t5, 16
    addu    $t8, $a2, $t6
    lw      $t5, 48($a0)
    addu    $v0, $t7, $t9
    srl     $t6, $t9, 16
    addu    $a2, $t8, $a3
    lw      $t8, 52($a0)
    addu    $a3, $a2, $t6
    srl     $t4, $t5, 16
    addu    $v1, $v0, $t5
    lw      $t5, 56($a0)
    addu    $t7, $a3, $t4
    srl     $t9, $t8, 16
    addu    $t4, $v1, $t8
    lw      $t8, 60($a0)
    addu    $a2, $t7, $t9
    srl     $t6, $t5, 16
    addiu   $a1, $a1, -64
    srl     $a3, $t8, 16
    addu    $t9, $a2, $t6
    addu    $v0, $t4, $t5
    slti    $t7, $a1, 72
    addu    $t1, $t9, $a3
    lw      $a2, 64($a0)
    lw      $a3, 68($a0)
    addu    $t0, $v0, $t8
    beqz    $t7, 6b
    addiu   $a0, $a0, 64
    sll     $t7, $t1, 16
    subu    $t6, $t0, $t7
    addu    $t5, $t2, $t6
    addu    $t2, $t5, $t1
    slti    $a3, $a1, 4
5:
    move    $t0, $zero
    bnez    $a3, 7f
    move    $t1, $zero
8:
    lw      $t8, 0($a0)
    addiu   $a1, $a1, -4
    srl     $a2, $t8, 16
    slti    $t9, $a1, 4
    addu    $t1, $t1, $a2
    addiu   $a0, $a0, 4
    beqz    $t9, 8b
    addu    $t0, $t0, $t8
7:
    sll     $t4, $t1, 16
    subu    $v1, $t0, $t4
    addu    $v0, $t2, $v1
    slti    $t0, $a1, 2
    bnez    $t0, 9f
    addu    $t2, $v0, $t1
10:
    lhu     $t5, 0($a0)
    addiu   $a1, $a1, -2
    slti    $t1, $a1, 2
    addu    $t2, $t2, $t5
    beqz    $t1, 10b
    addiu   $a0, $a0, 2
9:
    move    $t6, $a1
2:
    blez    $t6, 11f
    addiu   $a1, $a1, -1
12:
    lbu     $a3, 0($a0)
    move    $t7, $a1
    addu    $t2, $t2, $a3
    addiu   $a0, $a0, 1
    bgtz    $t7, 12b
    addiu   $a1, $a1, -1
11:
    beqz    $t3, 13f
    srl     $t0, $t2, 16
    andi    $v1, $t2, 0xFFFF
    addu    $a2, $v1, $t0
    srl     $t9, $a2, 16
    andi    $t8, $a2, 0xFFFF
    addu    $t3, $t8, $t9
    andi    $a0, $t3, 0xFF
    srl     $a1, $t3, 8
    andi    $v0, $a1, 0xFF
    sll     $v1, $a0, 8
14:
    addu    $t6, $v0, $v1
    srl     $t1, $t6, 16
    andi    $t5, $t6, 0xFFFF
    addu    $t4, $t5, $t1
    jr      $ra
    move    $v0, $t4
13:
    srl     $v1, $t2, 16
    beq     $zero, $zero, 14b
    andi    $v0, $t2, 0xFFFF
