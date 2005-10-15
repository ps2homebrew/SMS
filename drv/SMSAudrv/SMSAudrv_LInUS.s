#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
.data
s_CnvSrcRate:
    .word   0

.text
.set noreorder

.globl  _init_ups
.ent	_init_ups
_init_ups:
    lui      $v0, 0x51EB
    ori      $v0, $v0, 0x851F
    mult     $a0, $v0
    sra      $a0, $a0, 31
    addu     $a3, $zero, $zero
    addiu    $a2, $zero, 0x0780
    addiu    $v1, $zero, 0x01FF
    mfhi     $v0
    sra      $v0, $v0, 3
    subu     $v0, $v0, $a0
    sll      $v0, $v0, 1
    sw       $v0, s_CnvSrcRate
    subu     $a2, $a2, $v0
1:
    bltz     $a2, 3f
    addiu    $v1, $v1, -1
2:
    bgez     $v1, 1b
    subu     $a2, $a2, $v0
    addu     $a2, $a2, $v0
    addiu    $v0, $a1, 1
    jr       $ra
    sllv     $v0, $a3, $v0
3:
    addiu    $a3, $a3, 1
    j        2b
    addiu    $a2, $a2, 0x0F00
.end	_init_ups

.globl	_ups_stereo
.ent	_ups_stereo
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
    addiu    $t1, $t1, -1
    sll	     $t0, $t0, 16
    sra	     $t0, $t0, 16
    mflo     $v1
    mult     $t0, $t5
    mflo     $t0
    addu     $v1, $v1, $t0
    sra      $t0, $v1, 31
    mult     $v1, $t2
    mfhi     $v0
    addu     $v0, $v0, $v1
    sra	     $v1, $t7, 16
    sra      $v0, $v0, 11
    subu     $v0, $v0, $t0
    sh       $v0, 0($a2)
    mult     $v1, $a3
    subu     $a3, $a3, $t3
    sra	     $t0, $t8, 16
    mflo     $v1
    mult     $t0, $t5
    mflo     $t0
    addu     $v1, $v1, $t0
    sra      $t0, $v1, 31
    mult     $v1, $t2
    mfhi     $v0
    addu     $v0, $v0, $v1
    sra      $v0, $v0, 11
    subu     $v0, $v0, $t0
    sh       $v0, 512($a2)
    bltz     $a3, 4f
    addiu    $a2, $a2, 2
3:
    bgez     $t1, 2b
    nop
    addiu    $t6, $t6, 512
    slti     $v0, $t6, 1024
    bne      $v0, $zero, 1b
    addiu    $a1, $a1, 1024
    jr       $ra
    nop
4:
    addiu    $a0, $a0, 4
    j        3b
    addiu    $a3, $a3, 0x0F00
.end	_ups_stereo

.globl	_ups_mono
.ent	_ups_mono
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
    bltz    $a3, 4f
    addiu   $a2, $a2, 2
3:
    bgez    $t1, 2b
    nop
    addiu   $t5, $t5, 512
    slti    $v0, $t5, 1024
    bne	    $v0, $zero, 1b
    addiu   $a1, $a1, 1024
    jr	    $ra
    nop
4:
    addiu   $t0, $t0,2
    j	    3b
    addiu   $a3, $a3, 0x0F00
 .end	_ups_mono
