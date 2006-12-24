#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
.set noreorder

.globl _init_ups
.globl _ups_stereo
.globl _ups_mono

.data

s_CnvSrcRate:   .word   0

.text

.ent _init_ups
_init_ups:
    lui     $v0, 0x51EB
    ori     $v0, $v0, 0x851F
    mult    $a0, $v0
    sra     $a0, $a0, 0x1F
    li      $a2, 1920
    move    $a3, $zero
    move    $v1, $zero
    li      $t0, 511
    mfhi    $v0
    sra     $v0, $v0, 0x3
    subu    $v0, $v0, $a0
    sll     $a0, $v0, 0x1
    sw      $a0, s_CnvSrcRate
    subu    $a2, $a2, $a0
2:
    bltz    $a2, 1f
    nop
3:
    addiu   $v1, $v1, 1
    slti    $v0, $v1, 512
    bnez    $v0, 2b
    subu    $a2, $a2, $a0
    addu    $a2, $a2, $a0
    addiu   $a3, $a3, 2
    jr      $ra
    sllv    $v0, $a3, $a1
1:
    beq     $v1, $t0, 3b
    nop
    addiu   $a3, $a3, 1
    beq     $zero, $zero, 3b
    addiu   $a2, $a2, 3840
.end _init_ups

.ent _ups_stereo
_ups_stereo:
    lui      $t2, 0x8888
    lw       $t3, s_CnvSrcRate
    addiu    $a3, $zero, 0x0780
    addu     $t6, $zero, $zero
    addiu    $t4, $zero, 0x0F00
    ori      $t2, $t2, 0x8889
1:
    addu     $a2, $a1, $zero
    addiu    $t1, $zero, 0x00FF
2:
    lw       $v1, 0($a0)
    lw       $t0, 4($a0)
    addu     $t7, $v1, $zero
    addu     $t8, $t0, $zero
    sll	     $v1, $v1, 16
    sra	     $v1, $v1, 16
    mult     $v1, $a3
    subu     $t5, $t4, $a3
    sll	     $t0, $t0, 16
    mflo     $v1
    sra	     $t0, $t0, 16
    mult     $t0, $t5
    addiu    $t1, $t1, -1
    mflo     $t0
    addu     $v1, $v1, $t0
    mult     $v1, $t2
    sra      $t0, $v1, 31
    mfhi     $v0
    addu     $v0, $v0, $v1
    sra	     $v1, $t7, 16
    sra      $v0, $v0, 11
    subu     $v0, $v0, $t0
    sh       $v0, 0($a2)
    mult     $v1, $a3
    subu     $a3, $a3, $t3
    mflo     $v1
    sra	     $t0, $t8, 16
    mult     $t0, $t5
    mflo     $t0
    addu     $v1, $v1, $t0
    sra      $t0, $v1, 31
    mult     $v1, $t2
    addiu    $a2, $a2, 2
    mfhi     $v0
    addu     $v0, $v0, $v1
    sra      $v0, $v0, 11
    subu     $v0, $v0, $t0
    bgez     $a3, 3f
    sh       $v0, 510($a2)
    addiu    $a0, $a0, 4
    addiu    $a3, $a3, 0x0F00
3:
    bgez     $t1, 2b
    nop
    addiu    $t6, $t6, 512
    slti     $v0, $t6, 1024
    bne      $v0, $zero, 1b
    addiu    $a1, $a1, 1024
    jr       $ra
.end _ups_stereo

.ent _ups_mono
_ups_mono:
    lui	    $t3, 0x8888
    lw	    $t2, s_CnvSrcRate
    addu    $t0, $a0, $zero
    addiu   $a3, $zero, 0x0780
    addu    $t5, $zero, $zero
    addiu   $t4, $zero, 0x0F00
    ori	    $t3, $t3, 0x8889
1:
    addu    $a2, $a1, $zero
    addiu   $t1, $zero, 0x00FF
2:
    lh	    $a0, 2($t0)
    subu    $v0, $t4, $a3
    mult    $a0, $v0
    lh	    $v1, 0($t0)
    addiu   $t1, $t1, -1
    mflo    $a0
    mult    $v1, $a3
    subu    $a3, $a3, $t2
    mflo    $v1
    addu    $v1, $v1, $a0
    sra	    $a0, $v1, 31
    mult    $v1, $t3
    mfhi    $v0
    addu    $v0, $v0, $v1
    sra	    $v0, $v0, 11
    subu    $v0, $v0, $a0
    sh	    $v0, 0($a2)
    sh	    $v0, 512($a2)
    bgez    $a3, 3f
    addiu   $a2, $a2, 2
    addiu   $t0, $t0, 2
    addiu   $a3, $a3, 0x0F00
3:
    bgez    $t1, 2b
    nop
    addiu   $t5, $t5, 512
    slti    $v0, $t5, 1024
    bne	    $v0, $zero, 1b
    addiu   $a1, $a1, 1024
    jr	    $ra
    nop
 .end	_ups_mono
