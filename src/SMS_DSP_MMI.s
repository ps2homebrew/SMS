#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# IDCT_Put and IDCT_Add functions are originally provided by Intel at AP-922
# http://developer.intel.com/vtune/cbts/strmsimd/922down.htm
# (See more app notes at http://developer.intel.com/vtune/cbts/strmsimd/appnotes.htm)
# but in a limited edition.
# column code adapted from peter gubanov
# Copyright (c) 2000-2001 Peter Gubanov <peter@elecard.net.ru>
# http://www.elecard.com/peter/idct.shtml
# Rounding trick Copyright (c) 2000 Michel Lespinasse <walken@zoy.org>
# MMI port by Leon van Stuivenberg
# Licensed (like the original ffmpeg source code) under the terms of the
# GNU Lesser General Public License as published by the Free Software Foundation;
# either version 2 of the License, or (at your option) any later version.
#
# DSP_xxx routines (c) 2006 Eugene Plotnikov
# These are licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
.text

.set noat
.set volatile
.set noreorder
.set nomacro

.globl IDCT
.globl DSP_PutPixels16
.globl DSP_PutPixels8
.globl DSP_PutPixels16X
.globl DSP_PutPixels8X
.globl DSP_PutPixels16Y
.globl DSP_PutPixels8Y
.globl DSP_PutPixels16XY
.globl DSP_PutPixels8XY
.globl DSP_PutNoRndPixels16X
.globl DSP_PutNoRndPixels8X
.globl DSP_PutNoRndPixels16Y
.globl DSP_PutNoRndPixels8Y
.globl DSP_PutNoRndPixels16XY
.globl DSP_PutNoRndPixels8XY
.globl DSP_AvgPixels16
.globl DSP_AvgPixels8
.globl DSP_AvgPixels16X
.globl DSP_AvgPixels8X
.globl DSP_AvgPixels16Y
.globl DSP_AvgPixels8Y
.globl DSP_AvgPixels16XY
.globl DSP_AvgPixels8XY
.globl DSP_GMC1_16
.globl DSP_GMC1_8
.globl DSP_PutPixels8_16
.globl DSP_PutPixels8X_16
.globl DSP_PutPixels8Y_16
.globl DSP_PutPixels8XY_16
.globl DSP_PutNoRndPixels8X_16
.globl DSP_PutNoRndPixels8Y_16
.globl DSP_PutNoRndPixels8XY_16
.globl DSP_AvgPixels8_16
.globl DSP_AvgPixels8X_16
.globl DSP_AvgPixels8Y_16
.globl DSP_AvgPixels8XY_16
.globl DSP_GetMB
.globl DSP_PackMB

.equ IDCT_CONST_OFFSET, 0x0400

.sdata
.align  4
s_pVUData:  .half   0x0004, 0x0008, 0x000C, 0x0020
            .half   0x0024, 0x0028, 0x002C, 0x0040
            .half   0x0044, 0x0048, 0x004C, 0x0000
            .half   0x0000, 0x0000, 0x0000, 0x0000

IDCT:
    lui     $t8, 0x7000
    mtc1    $s0, $f00
    lq      $t0, IDCT_CONST_OFFSET +  0($t8)
    lq      $s0,  0($a2)
    mtc1    $s1, $f01
    mtc1    $s2, $f02
    lq      $s1, IDCT_CONST_OFFSET + 32($t8)
    pinth   $s0, $s0, $s0
    mtc1    $s3, $f03
    lq      $s2, IDCT_CONST_OFFSET + 48($t8)
    pinth   $s0, $s0, $s0
    mtc1    $s4, $f04
    lq      $a3, IDCT_CONST_OFFSET + 16($t8)
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 80($t8)
    phmadh  $s2, $s2, $v0
    mtc1    $s5, $f05
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $t0
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    ppach   $t0, $s4, $s1
    lq      $s0,  16($a2)
    prevh   $v0, $t0
    pcpyud  $v0, $v0, $v0
    pinth   $s0, $s0, $s0
    pcpyld  $t0, $v0, $t0
# DCT_8_INV_ROW1
    lq      $s1, IDCT_CONST_OFFSET +  96($t8)
    pinth   $s0, $s0, $s0
    lq      $s2, IDCT_CONST_OFFSET + 112($t8)
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 144($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0,  32($a2)
    ppach   $t1, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET + 160($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t1
    lq      $s2, IDCT_CONST_OFFSET + 176($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t1, $v0, $t1
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 208($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0,  48($a2)
    ppach   $t2, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET + 224($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t2
    lq      $s2, IDCT_CONST_OFFSET + 240($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t2, $v0, $t2
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 272($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0, 64($a2)
    ppach   $t3, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET + 32($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t3
    lq      $s2, IDCT_CONST_OFFSET + 48($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t3, $v0, $t3
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 80($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0,  80($a2)
    ppach   $t4, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET + 224($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t4
    lq      $s2, IDCT_CONST_OFFSET + 240($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t4, $v0, $t4
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 272($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0,  96($a2)
    ppach   $t5, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET + 160($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t5
    lq      $s2, IDCT_CONST_OFFSET + 176($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t5, $v0, $t5
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 208($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $s0, 112($a2)
    ppach   $t6, $s4, $s1
    lq      $s1, IDCT_CONST_OFFSET +  96($t8)
    pinth   $s0, $s0, $s0
    prevh   $v0, $t6
    lq      $s2, IDCT_CONST_OFFSET + 112($t8)
    pinth   $s0, $s0, $s0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t6, $v0, $t6
# DCT_8_INV_ROW1
    prevh   $v0, $s0
    lq      $s3, IDCT_CONST_OFFSET + 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, IDCT_CONST_OFFSET + 144($t8)
    phmadh  $s2, $s2, $v0
    phmadh  $s3, $s3, $s0
    phmadh  $s4, $s4, $v0
    paddw   $s1, $s1, $s2
    paddw   $s3, $s3, $s4
    pcpyld  $s2, $s3, $s1
    pcpyud  $s4, $s1, $s3
    paddw   $s2, $s2, $a3
    paddw   $s1, $s2, $s4
    psubw   $s4, $s2, $s4
    psraw   $s1, $s1, 11
    psraw   $s4, $s4, 11
    lq      $v0, IDCT_CONST_OFFSET + 320($t8)
    ppach   $t7, $s4, $s1
    pmulth  $s1, $t3, $v0
    prevh   $v1, $t7
    pcpyud  $v1, $v1, $v1
    pcpyld  $t7, $v1, $t7
# DCT_8_INV_COL8
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    pmulth      $s2, $t5, $v0
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t5
    lq          $v0, IDCT_CONST_OFFSET + 288($t8)
    psraw       $s2, $s2, 15
    pmfhl.uw    $v1
    pmulth      $s3, $t7, $v0
    psraw       $v1, $v1, 15
    pinteh      $s2, $v1, $s2
    paddh       $s2, $s2, $t3
    psraw       $s3, $s3, 15
    pmfhl.uw    $v1
    pmulth      $s4, $t1, $v0
    psraw       $v1, $v1, 15
    pinteh      $s3, $v1, $s3
    paddh       $s3, $s3, $t1
    psraw       $s4, $s4, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s4, $v1, $s4
    psubh       $s4, $s4, $t7
    lq          $v0, IDCT_CONST_OFFSET + 336($t8)
    psubh       $v1, $s3, $s2
    paddh       $s0, $s4, $s1
    paddh       $s5, $v1, $s0
    pmulth      $s5, $s5, $v0
    psubh       $at, $s4, $s1
    paddh       $s4, $s3, $s2
    psubh       $t9, $v1, $s0
    psraw       $s5, $s5, 15
    pmfhl.uw    $v1
    pmulth      $t9, $t9, $v0
    lq          $v0, IDCT_CONST_OFFSET + 304($t8)
    psraw       $v1, $v1, 15
    pinteh      $s5, $v1, $s5
    psraw       $t9, $t9, 15
    pmfhl.uw    $v1
    pmulth      $s1, $t2, $v0
    psraw       $v1, $v1, 15
    pinteh      $t9, $v1, $t9
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    pmulth      $s2, $t6, $v0
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t6
    psraw       $s2, $s2, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s2, $v1, $s2
    paddh       $s2, $s2, $t2
    paddh       $v0, $t0, $t4
    psubh       $v1, $t0, $t4
    paddh       $s0, $v0, $s2
    psubh       $s3, $v0, $s2
    psubh       $s2, $v1, $s1
    paddh       $s1, $v1, $s1
    lq          $t3, IDCT_CONST_OFFSET + 352($t8)
# DCT_8_INV_COL8_PMS
    paddh   $v0, $s0, $s4
    paddh   $v1, $s1, $s5
    psubh   $s4, $s0, $s4
    psubh   $s5, $s1, $s5
    psrah   $s0, $v0, 6
    psrah   $s4, $s4, 6
    psrah   $s1, $v1, 6
    psrah   $s5, $s5, 6
    paddh   $v0, $s2, $t9
    paddh   $v1, $s3, $at
    psubh   $t9, $s2, $t9
    psubh   $at, $s3, $at
    psrah   $s2, $v0, 6
    psrah   $t9, $t9, 6
    psrah   $s3, $v1, 6
    psrah   $at, $at, 6
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s0
    pminh   $v0, $v0, $t3
    mfc1    $s0, $f00
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s1
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    mfc1    $s1, $f01
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s2
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    mfc1    $s2, $f02
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s3
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $zero
    mfc1    $s3, $f03
    ppacb   $v0, $zero, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $at
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $t9
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s5
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    mfc1    $s5, $f05
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s4
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    mfc1    $s4, $f04
    jr      $ra
    sd      $v0, 0($a0)

DSP_PutPixels16:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    addiu   $a0, $a0, 16
    qfsrv   $t2, $t3, $t2
    bgtz    $v1, 1b
    sq      $t2, -16($a0)
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels8:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
1:
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    sd      $t3,  0($a0)
    sd      $t5, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels16X:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    addiu   $a0, $a0, 16
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    paddh   $t1, $t1, $t6
    paddh   $t2, $t2, $t6
    psrlh   $t1, $t1, 1
    psrlh   $t2, $t2, 1
    addiu   $v0, $v0, 16
    ppacb   $t1, $t2, $t1
    bgtz    $v1, 1b
    sq      $t1, -16($a0)
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels8X:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    psrlh   $t8, $t8, 15
1:
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    mtsab   $a2, 0
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    paddh   $t3, $t3, $t4
    paddh   $t5, $t5, $t6
    paddh   $t3, $t3, $t8
    paddh   $t5, $t5, $t8
    psrlh   $t3, $t3, 1
    psrlh   $t5, $t5, 1
    ppacb   $t3, $zero, $t3
    ppacb   $t5, $zero, $t5
    sd      $t3,  0($a0)
    sd      $t5, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels16Y:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextub  $t2, $zero, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $a0, $a0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t3, $t4, $t3
    pextub  $t4, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t7, $t3, $t1
    paddh   $t8, $t4, $t2
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    paddh   $t7, $t7, $t6
    paddh   $t8, $t8, $t6
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $t8, $t7
    bgtz    $v1, 1b
    sq      $t7, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels8Y:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t4, $zero, $t5
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t6, $zero, $t7
    paddh   $t7, $t5, $t3
    paddh   $t8, $t6, $t4
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels16XY:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    psrlh   $t8, $t8, 15
    psllh   $t8, $t8, 1
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    mtsab   $a2, 0
    addiu   $v0, $v0, 16
    addiu   $a0, $a0, 16
    qfsrv   $t5, $t4, $t3
    qfsrv   $t6, $t3, $t4
    pextlb  $t3, $zero, $t5
    addiu   $v1, $v1, -1
    pextub  $t4, $zero, $t5
    mtsab   $zero, 1
    qfsrv   $t6, $t6, $t5
    pextlb  $t5, $zero, $t6
    pextub  $t6, $zero, $t6
    paddh   $t3, $t3, $t5
    paddh   $t4, $t4, $t6
    paddh   $t5, $t1, $t3
    paddh   $t6, $t2, $t4
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    paddh   $t5, $t5, $t8
    paddh   $t6, $t6, $t8
    psrlh   $t5, $t5, 2
    psrlh   $t6, $t6, 2
    ppacb   $t5, $t6, $t5
    bgtz    $v1, 1b
    sq      $t5, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutPixels8XY:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
    psllh   $t9, $t9, 1
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    mtsab   $a2, 0
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    paddh   $t3, $t3, $t4
    paddh   $t4, $t5, $t6
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    mtsab   $a2, 0
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t6, $t5, $t5
    qfsrv   $t8, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t7, $zero, $t7
    pextlb  $t6, $zero, $t6
    pextlb  $t8, $zero, $t8
    paddh   $t5, $t5, $t6
    paddh   $t6, $t7, $t8
    paddh   $t7, $t3, $t5
    paddh   $t8, $t4, $t6
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 2
    psrlh   $t8, $t8, 2
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels16X:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    addiu   $a0, $a0, 16
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    psrlh   $t1, $t1, 1
    psrlh   $t2, $t2, 1
    addiu   $v0, $v0, 16
    ppacb   $t1, $t2, $t1
    bgtz    $v1, 1b
    sq      $t1, -16($a0)
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels8X:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    addu    $v0, $v0, $a1
1:
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    mtsab   $a2, 0
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    paddh   $t3, $t3, $t4
    paddh   $t5, $t5, $t6
    psrlh   $t3, $t3, 1
    psrlh   $t5, $t5, 1
    ppacb   $t3, $zero, $t3
    ppacb   $t5, $zero, $t5
    sd      $t3,  0($a0)
    sd      $t5, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels16Y:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextub  $t2, $zero, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $a0, $a0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t3, $t4, $t3
    pextub  $t4, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t7, $t3, $t1
    paddh   $t8, $t4, $t2
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $t8, $t7
    bgtz    $v1, 1b
    sq      $t7, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels8Y:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t4, $zero, $t5
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t6, $zero, $t7
    paddh   $t7, $t5, $t3
    paddh   $t8, $t6, $t4
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels16XY:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    psrlh   $t8, $t8, 15
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    addiu   $a0, $a0, 16
    qfsrv   $t5, $t4, $t3
    qfsrv   $t6, $t3, $t4
    pextlb  $t3, $zero, $t5
    addiu   $v1, $v1, -1
    pextub  $t4, $zero, $t5
    mtsab   $zero, 1
    qfsrv   $t6, $t6, $t5
    pextlb  $t5, $zero, $t6
    pextub  $t6, $zero, $t6
    paddh   $t3, $t3, $t5
    paddh   $t4, $t4, $t6
    paddh   $t5, $t1, $t3
    paddh   $t6, $t2, $t4
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    paddh   $t5, $t5, $t8
    paddh   $t6, $t6, $t8
    psrlh   $t5, $t5, 2
    psrlh   $t6, $t6, 2
    ppacb   $t5, $t6, $t5
    bgtz    $v1, 1b
    sq      $t5, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_PutNoRndPixels8XY:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    mtsab   $a2, 0
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    paddh   $t3, $t3, $t4
    paddh   $t4, $t5, $t6
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    mtsab   $a2, 0
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t6, $t5, $t5
    qfsrv   $t8, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t7, $zero, $t7
    pextlb  $t6, $zero, $t6
    pextlb  $t8, $zero, $t8
    paddh   $t5, $t5, $t6
    paddh   $t6, $t7, $t8
    paddh   $t7, $t3, $t5
    paddh   $t8, $t4, $t6
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 2
    psrlh   $t8, $t8, 2
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels16:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $a0, $a0, 16
    addiu   $v1, $v1, -1
    lq      $t4, -16($a0)
    qfsrv   $t2, $t3, $t2
    pextub  $t3, $zero, $t2
    pextlb  $t2, $zero, $t2
    pextub  $t5, $zero, $t4
    pextlb  $t4, $zero, $t4
    paddh   $t3, $t3, $t5
    paddh   $t2, $t2, $t4
    paddh   $t3, $t3, $t6
    paddh   $t2, $t2, $t6
    psrlh   $t3, $t3, 1
    psrlh   $t2, $t2, 1
    ppacb   $t2, $t3, $t2
    bgtz    $v1, 1b
    sq      $t2, -16($a0)
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels8:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
1:
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    ld      $t7,   0($a0)
    ld      $t8,  64($a0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t7, $zero, $t7
    pextlb  $t8, $zero, $t8
    paddh   $t3, $t3, $t7
    paddh   $t5, $t5, $t8
    paddh   $t3, $t3, $t9
    paddh   $t5, $t5, $t9
    psrlh   $t3, $t3, 1
    psrlh   $t5, $t5, 1
    ppacb   $t3, $zero, $t3
    ppacb   $t5, $zero, $t5
    sd      $t3,  0($a0)
    sd      $t5, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels16X:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    addiu   $a0, $a0, 16
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    lq      $t7,  -16($a0)
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    pextub  $t8, $zero, $t7
    pextlb  $t7, $zero, $t7
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    paddh   $t1, $t1, $t6
    paddh   $t2, $t2, $t6
    psrlh   $t1, $t1, 1
    psrlh   $t2, $t2, 1
    paddh   $t1, $t1, $t7
    paddh   $t2, $t2, $t8
    paddh   $t1, $t1, $t6
    paddh   $t2, $t2, $t6
    psrlh   $t1, $t1, 1
    psrlh   $t2, $t2, 1
    addiu   $v0, $v0, 16
    ppacb   $t1, $t2, $t1
    bgtz    $v1, 1b
    sq      $t1, -16($a0)
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels8X:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    psrlh   $t8, $t8, 15
1:
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    ld      $t1,   0($a0)
    ld      $t2,  64($a0)
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    mtsab   $a2, 0
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t3, $t4
    paddh   $t5, $t5, $t6
    paddh   $t3, $t3, $t8
    paddh   $t5, $t5, $t8
    psrlh   $t3, $t3, 1
    psrlh   $t5, $t5, 1
    paddh   $t3, $t3, $t1
    paddh   $t5, $t5, $t2
    paddh   $t3, $t3, $t8
    paddh   $t5, $t5, $t8
    psrlh   $t3, $t3, 1
    psrlh   $t5, $t5, 1
    ppacb   $t3, $zero, $t3
    ppacb   $t5, $zero, $t5
    sd      $t3,  0($a0)
    sd      $t5, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels16Y:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextub  $t2, $zero, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    addiu   $a0, $a0, 16
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    lq      $at, -16($a0)
    qfsrv   $t3, $t4, $t3
    pextub  $t4, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t7, $t3, $t1
    paddh   $t8, $t4, $t2
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    pextlb  $t3, $zero, $at
    pextub  $t4, $zero, $at
    paddh   $t7, $t7, $t6
    paddh   $t8, $t8, $t6
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    paddh   $t7, $t7, $t3
    paddh   $t8, $t8, $t4
    paddh   $t7, $t7, $t6
    paddh   $t8, $t8, $t6
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $t8, $t7
    bgtz    $v1, 1b
    sq      $t7, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels8Y:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t4, $zero, $t5
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    ld      $t1,   0($a0)
    ld      $t2,  64($a0)
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t6, $zero, $t7
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    paddh   $t7, $t5, $t3
    paddh   $t8, $t6, $t4
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    paddh   $t7, $t7, $t1
    paddh   $t8, $t8, $t2
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels16XY:
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    psrlh   $t8, $t8, 15
    psllh   $t8, $t8, 1
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    paddh   $t1, $t1, $t3
    paddh   $t2, $t2, $t4
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t3,   0($v0)
    lq      $t4, 384($v0)
    addiu   $a0, $a0, 16
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t5, $t4, $t3
    qfsrv   $t6, $t3, $t4
    pextlb  $t3, $zero, $t5
    addiu   $v1, $v1, -1
    lq      $at, -16($a0)
    pextub  $t4, $zero, $t5
    mtsab   $zero, 1
    qfsrv   $t6, $t6, $t5
    pextlb  $t5, $zero, $t6
    pextub  $t6, $zero, $t6
    paddh   $t3, $t3, $t5
    paddh   $t4, $t4, $t6
    paddh   $t5, $t1, $t3
    paddh   $t6, $t2, $t4
    por     $t1, $zero, $t3
    por     $t2, $zero, $t4
    pextlb  $t3, $zero, $at
    pextub  $t4, $zero, $at
    paddh   $t5, $t5, $t8
    paddh   $t6, $t6, $t8
    psrlh   $t5, $t5, 2
    psrlh   $t6, $t6, 2
    psrlh   $t8, $t8, 1
    paddh   $t5, $t5, $t3
    paddh   $t6, $t6, $t4
    paddh   $t5, $t5, $t8
    paddh   $t6, $t6, $t8
    psllh   $t8, $t8, 1
    psrlh   $t5, $t5, 1
    psrlh   $t6, $t6, 1
    ppacb   $t5, $t6, $t5
    bgtz    $v1, 1b
    sq      $t5, -16($a0)
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra

DSP_AvgPixels8XY:
    addiu   $v0, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 3
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
    psllh   $t9, $t9, 1
    ld      $t3,   0($v0)
    ld      $t5,  64($v0)
    ld      $t4, 384($v0)
    ld      $t6, 448($v0)
    mtsab   $a2, 0
    pcpyld  $t3, $t4, $t3
    pcpyld  $t5, $t6, $t5
    qfsrv   $t3, $t3, $t3
    qfsrv   $t5, $t5, $t5
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t4, $t3, $t3
    qfsrv   $t6, $t5, $t5
    pextlb  $t3, $zero, $t3
    pextlb  $t5, $zero, $t5
    pextlb  $t4, $zero, $t4
    pextlb  $t6, $zero, $t6
    paddh   $t3, $t3, $t4
    paddh   $t4, $t5, $t6
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    ld      $t5,   0($v0)
    ld      $t7,  64($v0)
    ld      $t6, 384($v0)
    ld      $t8, 448($v0)
    ld      $t1,   0($a0)
    ld      $t2,  64($a0)
    mtsab   $a2, 0
    pcpyld  $t5, $t6, $t5
    pcpyld  $t7, $t8, $t7
    qfsrv   $t5, $t5, $t5
    qfsrv   $t7, $t7, $t7
    addiu   $at, $zero, 1
    addiu   $v0, $v0, 8
    addiu   $v1, $v1, -1
    mtsab   $at, 0
    qfsrv   $t6, $t5, $t5
    qfsrv   $t8, $t7, $t7
    pextlb  $t5, $zero, $t5
    pextlb  $t7, $zero, $t7
    pextlb  $t6, $zero, $t6
    pextlb  $t8, $zero, $t8
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    paddh   $t5, $t5, $t6
    paddh   $t6, $t7, $t8
    paddh   $t7, $t3, $t5
    paddh   $t8, $t4, $t6
    por     $t3, $zero, $t5
    por     $t4, $zero, $t6
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psrlh   $t7, $t7, 2
    psrlh   $t8, $t8, 2
    psrlh   $t9, $t9, 1
    paddh   $t7, $t7, $t1
    paddh   $t8, $t8, $t2
    paddh   $t7, $t7, $t9
    paddh   $t8, $t8, $t9
    psllh   $t9, $t9, 1
    psrlh   $t7, $t7, 1
    psrlh   $t8, $t8, 1
    ppacb   $t7, $zero, $t7
    ppacb   $t8, $zero, $t8
    sd      $t7,  0($a0)
    sd      $t8, 64($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 8
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
    jr      $ra
    nop

DSP_GMC1_16:
    mult        $v0, $t0, $t1
    addiu       $at, $zero, 16
    subu        $v1, $at, $t0
    mult1       $t4, $v1, $t1
    subu        $t5, $at, $t1
    pcpyh       $v0, $v0
    mult        $t0, $t0, $t5
    pcpyld      $v0, $v0, $v0
    pcpyh       $t4, $t4
    mult1       $v1, $v1, $t5
    pcpyld      $t4, $t4, $t4
    pcpyh       $t0, $t0
    subu        $at, $at, $a3
    pcpyld      $t0, $t0, $t0
    sll         $t1, $a3, 4
    pcpyh       $v1, $v1
    addu        $t1, $t1, $a1
    pcpyh       $t2, $t2
    lq          $t5,   0($t1)
    lq          $t6, 384($t1)
    mtsab       $a2, 0
    pcpyld      $v1, $v1, $v1
    qfsrv       $t5, $t6, $t5
    qfsrv       $t7, $t5, $t6
    pcpyld      $t2, $t2, $t2
    addiu       $t1, $t1, 16
    addiu       $at, $at, -1
    mtsab       $zero, 1
    qfsrv       $t6, $t7, $t5
    beq         $at, $zero, 2f
    addiu       $a3, $a3, 1
1:
    lq          $t7,   0($t1)
    lq          $t8, 384($t1)
    mtsab       $a2, 0
    qfsrv       $t7, $t8, $t7
    qfsrv       $t9, $t7, $t8
    addiu       $at, $at, -1
    pmtlo       $t2
    pmthi       $t2
    mtsab       $zero, 1
    qfsrv       $t8, $t9, $t7
    pextlb      $t9, $zero, $t5
    pmaddh      $zero, $v1, $t9
    pextlb      $t9, $zero, $t6
    pmaddh      $zero, $t0, $t9
    pextlb      $t9, $zero, $t7
    pmaddh      $zero, $t4, $t9
    pextlb      $t9, $zero, $t8
    pmaddh      $zero, $v0, $t9
    addiu       $t1, $t1, 16
    pmfhl.lh    $t9
    pmtlo       $t2
    psrah       $t9, $t9, 8
    pmthi       $t2
    ppacb       $t9, $zero, $t9
    sd          $t9, 0($a0)
    pextub      $t9, $zero, $t5
    pmaddh      $zero, $v1, $t9
    pextub      $t9, $zero, $t6
    pmaddh      $zero, $t0, $t9
    pextub      $t9, $zero, $t7
    pmaddh      $zero, $t4, $t9
    pextub      $t9, $zero, $t8
    pmaddh      $zero, $v0, $t9
    por         $t5, $zero, $t7
    por         $t6, $zero, $t8
    pmfhl.lh    $t9
    psrah       $t9, $t9, 8
    ppacb       $t9, $zero, $t9
    sd          $t9, 8($a0)
    bgtz        $at, 1b
    addiu       $a0, $a0, 16
2:
    addu        $at, $zero, $a3
    addu        $t1, $a1, $t3
    bgtzl       $at, 1b
    addu        $a3, $zero, $zero
    jr          $ra
    nop

DSP_GMC1_8:
    mult        $v0, $t0, $t1
    addiu       $at, $zero, 16
    subu        $v1, $at, $t0
    mult1       $t4, $v1, $t1
    subu        $t5, $at, $t1
    addiu       $at, $zero, 8
    pcpyh       $v0, $v0
    mult        $t0, $t0, $t5
    pcpyld      $v0, $v0, $v0
    pcpyh       $t4, $t4
    mult1       $v1, $v1, $t5
    pcpyld      $t4, $t4, $t4
    pcpyh       $t0, $t0
    subu        $at, $at, $a3
    pcpyld      $t0, $t0, $t0
    sll         $t1, $a3, 3
    pcpyh       $v1, $v1
    addu        $t1, $t1, $a1
    pcpyh       $t2, $t2
    ld          $t5,   0($t1)
    ld          $t6,  64($t1)
    ld          $t7, 384($t1)
    ld          $t8, 448($t1)
    mtsab       $a2, 0
    pcpyld      $v1, $v1, $v1
    pcpyld      $t5, $t7, $t5
    pcpyld      $t6, $t8, $t6
    qfsrv       $t5, $t5, $t5
    qfsrv       $t6, $t6, $t6
    pcpyld      $t2, $t2, $t2
    addiu       $t1, $t1, 8
    addiu       $at, $at, -1
    mtsab       $zero, 1
    qfsrv       $t7, $t5, $t5
    qfsrv       $t8, $t6, $t6
    pcpyld      $t5, $t7, $t5
    pcpyld      $t6, $t8, $t6
    beq         $at, $zero, 2f
    addiu       $a3, $a3, 1
1:
    ld          $t7,   0($t1)
    ld          $t8, 384($t1)
    mtsab       $a2, 0
    pcpyld      $t7, $t8, $t7
    qfsrv       $t7, $t7, $t7
    ld          $t8,  64($t1)
    ld          $t9, 448($t1)
    pcpyld      $t8, $t9, $t8
    qfsrv       $t8, $t8, $t8
    addiu       $at, $at, -1
    pmtlo       $t2
    pmthi       $t2
    mtsab       $zero, 1
    qfsrv       $t9, $t7, $t7
    pcpyld      $t7, $t9, $t7
    qfsrv       $t9, $t8, $t8
    pcpyld      $t8, $t9, $t8
    pextlb      $t9, $zero, $t5
    pmaddh      $zero, $v1, $t9
    pextub      $t9, $zero, $t5
    pmaddh      $zero, $t0, $t9
    pextlb      $t9, $zero, $t7
    pmaddh      $zero, $t4, $t9
    pextub      $t9, $zero, $t7
    pmaddh      $zero, $v0, $t9
    addiu       $t1, $t1, 8
    pmfhl.lh    $t9
    pmtlo       $t2
    psrah       $t9, $t9, 8
    pmthi       $t2
    ppacb       $t9, $zero, $t9
    sd          $t9, 0($a0)
    pextlb      $t9, $zero, $t6
    pmaddh      $zero, $v1, $t9
    pextub      $t9, $zero, $t6
    pmaddh      $zero, $t0, $t9
    pextlb      $t9, $zero, $t8
    pmaddh      $zero, $t4, $t9
    pextub      $t9, $zero, $t8
    pmaddh      $zero, $v0, $t9
    por         $t5, $zero, $t7
    por         $t6, $zero, $t8
    pmfhl.lh    $t9
    psrah       $t9, $t9, 8
    ppacb       $t9, $zero, $t9
    sd          $t9, 64($a0)
    bgtz        $at, 1b
    addiu       $a0, $a0, 8
2:
    addu        $at, $zero, $a3
    addu        $t1, $a1, $t3
    bgtzl       $at, 1b
    addu        $a3, $zero, $zero
    jr          $ra

DSP_PutPixels8_16:
    addiu   $v0, $zero, 16
    addiu   $t4, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $t4, $t4, -1
    addiu   $v1, $v1, -1
    qfsrv   $t2, $t3, $t2
    beq     $t4, $zero, 2f
    sd      $t2, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra

DSP_PutPixels8X_16:
    addiu   $v0, $zero, 16
    addiu   $t4, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $t4, $t4, -1
    addiu   $v0, $v0, 16
    mtsab   $zero, 1
    qfsrv   $t1, $t3, $t3
    pextlb  $t3, $zero, $t3
    pextlb  $t1, $zero, $t1
    paddh   $t1, $t1, $t3
    paddh   $t1, $t1, $t6
    psrlh   $t1, $t1, 1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra

DSP_PutPixels8Y_16:
    addiu   $v0, $zero, 16
    addiu   $t5, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    addiu   $t5, $t5, -1
    qfsrv   $t2, $t3, $t2
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t2, $t1
    paddh   $t3, $t3, $t6
    por     $t1, $zero, $t2
    psrlh   $t3, $t3, 1
    ppacb   $t3, $zero, $t3
    beq     $t5, $zero, 3f
    sd      $t3, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra

DSP_PutPixels8XY_16:
    addiu   $v0, $zero, 16
    addiu   $t7, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    psrlh   $t8, $t8, 15
    psllh   $t8, $t8, 1
    qfsrv   $t1, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $v0, $v0, 16
    pextlb  $t2, $zero, $t1
    mtsab   $zero, 1
    qfsrv   $t1, $t1, $t1
    pextlb  $t1, $zero, $t1
    paddh   $t1, $t1, $t2
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t2, $t3, $t2
    addiu   $v1, $v1, -1
    addiu   $t7, $t7, -1
    pextlb  $t3, $zero, $t2
    mtsab   $zero, 1
    qfsrv   $t2, $t2, $t2
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t3, $t2
    paddh   $t4, $t3, $t1
    por     $t1, $zero, $t3
    paddh   $t4, $t4, $t8
    psrlh   $t4, $t4, 2
    ppacb   $t4, $zero, $t4
    beq     $t7, $zero, 3f
    sd      $t4, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra
    nop

DSP_PutNoRndPixels8X_16:
    addiu   $v0, $zero, 16
    addiu   $t4, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $t4, $t4, -1
    addiu   $v0, $v0, 16
    mtsab   $zero, 1
    qfsrv   $t1, $t3, $t3
    pextlb  $t3, $zero, $t3
    pextlb  $t1, $zero, $t1
    paddh   $t1, $t1, $t3
    psrlh   $t1, $t1, 1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra

DSP_PutNoRndPixels8Y_16:
    addiu   $v0, $zero, 16
    addiu   $t5, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    addiu   $t5, $t5, -1
    qfsrv   $t2, $t3, $t2
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t2, $t1
    por     $t1, $zero, $t2
    psrlh   $t3, $t3, 1
    ppacb   $t3, $zero, $t3
    beq     $t5, $zero, 3f
    sd      $t3, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra

DSP_PutNoRndPixels8XY_16:
    addiu   $v0, $zero, 16
    addiu   $t7, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    psrlh   $t8, $t8, 15
    qfsrv   $t1, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $v0, $v0, 16
    pextlb  $t2, $zero, $t1
    mtsab   $zero, 1
    qfsrv   $t1, $t1, $t1
    pextlb  $t1, $zero, $t1
    paddh   $t1, $t1, $t2
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t2, $t3, $t2
    addiu   $v1, $v1, -1
    addiu   $t7, $t7, -1
    pextlb  $t3, $zero, $t2
    mtsab   $zero, 1
    qfsrv   $t2, $t2, $t2
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t3, $t2
    paddh   $t4, $t3, $t1
    por     $t1, $zero, $t3
    paddh   $t4, $t4, $t8
    psrlh   $t4, $t4, 2
    ppacb   $t4, $zero, $t4
    beq     $t7, $zero, 3f
    sd      $t4, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra

DSP_AvgPixels8_16:
    addiu   $v0, $zero, 16
    addiu   $t4, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    ld      $t5,   0($a0)
    addiu   $v0, $v0, 16
    addiu   $t4, $t4, -1
    addiu   $v1, $v1, -1
    qfsrv   $t2, $t3, $t2
    pextlb  $t5, $zero, $t5
    pextlb  $t2, $zero, $t2
    paddh   $t2, $t2, $t5
    paddh   $t2, $t2, $t6
    psrlh   $t2, $t2, 1
    ppacb   $t2, $zero, $t2
    beq     $t4, $zero, 2f
    sd      $t2, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra

DSP_AvgPixels8X_16:
    addiu   $v0, $zero, 16
    addiu   $t4, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    ld      $t5,   0($a0)
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $t4, $t4, -1
    addiu   $v0, $v0, 16
    mtsab   $zero, 1
    qfsrv   $t1, $t3, $t3
    pextlb  $t3, $zero, $t3
    pextlb  $t1, $zero, $t1
    pextlb  $t5, $zero, $t5
    paddh   $t1, $t1, $t3
    paddh   $t1, $t1, $t6
    psrlh   $t1, $t1, 1
    paddh   $t1, $t1, $t5
    paddh   $t1, $t1, $t6
    psrlh   $t1, $t1, 1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra

DSP_AvgPixels8Y_16:
    addiu   $v0, $zero, 16
    addiu   $t5, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    addu    $v0, $v0, $a1
    pnor    $t6, $zero, $zero
    psrlh   $t6, $t6, 15
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    qfsrv   $t1, $t2, $t1
    pextlb  $t1, $zero, $t1
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    ld      $t7,   0($a0)
    addiu   $v0, $v0, 16
    addiu   $v1, $v1, -1
    addiu   $t5, $t5, -1
    qfsrv   $t2, $t3, $t2
    pextlb  $t2, $zero, $t2
    pextlb  $t7, $zero, $t7
    paddh   $t3, $t2, $t1
    paddh   $t3, $t3, $t6
    por     $t1, $zero, $t2
    psrlh   $t3, $t3, 1
    paddh   $t3, $t3, $t7
    paddh   $t3, $t3, $t6
    psrlh   $t3, $t3, 1
    ppacb   $t3, $zero, $t3
    beq     $t5, $zero, 3f
    sd      $t3, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra

DSP_AvgPixels8XY_16:
    addiu   $v0, $zero, 16
    addiu   $t7, $zero, 8
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    pnor    $t8, $zero, $zero
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    psrlh   $t8, $t8, 15
    psllh   $t9, $t8, 1
    qfsrv   $t1, $t2, $t1
    addiu   $v1, $v1, -1
    addiu   $v0, $v0, 16
    pextlb  $t2, $zero, $t1
    mtsab   $zero, 1
    qfsrv   $t1, $t1, $t1
    pextlb  $t1, $zero, $t1
    paddh   $t1, $t1, $t2
    beq     $v1, $zero, 2f
    addiu   $a3, $a3, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    ld      $t5,   0($a0)
    addiu   $v0, $v0, 16
    mtsab   $a2, 0
    qfsrv   $t2, $t3, $t2
    addiu   $v1, $v1, -1
    addiu   $t7, $t7, -1
    pextlb  $t3, $zero, $t2
    pextlb  $t5, $zero, $t5
    mtsab   $zero, 1
    qfsrv   $t2, $t2, $t2
    pextlb  $t2, $zero, $t2
    paddh   $t3, $t3, $t2
    paddh   $t4, $t3, $t1
    por     $t1, $zero, $t3
    paddh   $t4, $t4, $t9
    psrlh   $t4, $t4, 2
    paddh   $t4, $t4, $t5
    paddh   $t4, $t4, $t8
    psrlh   $t4, $t4, 1
    ppacb   $t4, $zero, $t4
    beq     $t7, $zero, 3f
    sd      $t4, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
2:
    addu    $v1, $zero, $a3
    addu    $v0, $a1, $t0
    bgtzl   $v1, 1b
    addu    $a3, $zero, $zero
3:
    jr      $ra

DSP_GetMB:
    lui     $a1, 0x7000
    lui     $v1, 0x3000
    ori     $a1, 0x0280
    or      $a0, $a0, $v1
    srl     $v1, $v1, 28
1:
    lq      $t0,   0($a0)
    lq      $t1,  16($a0)
    lq      $t2,  32($a0)
    lq      $t3,  48($a0)
    lq      $t4,  64($a0)
    lq      $t5,  80($a0)
    lq      $t6,  96($a0)
    lq      $t7, 112($a0)
    addiu   $a0, $a0, 128
    addiu   $v1, $v1, -1
    sq      $t0,   0($a1)
    sq      $t1,  16($a1)
    sq      $t2,  32($a1)
    sq      $t3,  48($a1)
    sq      $t4,  64($a1)
    sq      $t5,  80($a1)
    sq      $t6,  96($a1)
    sq      $t7, 112($a1)
    bgtzl   $v1, 1b
    addiu   $a1, $a1, 128
    jr      $ra

DSP_PackMB:
    lui     $t0, 0x3000
    la      $a1, s_pVUData
    mtsah   $zero, 1
    lq      $v1, 0($a1)
    addiu   $at, $zero, 2
    or      $a0, $a0, $t0
2:
    addiu   $at, $at, -1
    addiu   $v0, $zero, 4
1:
    qmfc2.i $t0, $vf01
    ctc2    $v1, $vi06
    qfsrv   $v1, $v1, $v1
    qmfc2   $t1, $vf02
    qmfc2   $t2, $vf03
    qmfc2   $t3, $vf04
    qmfc2   $t4, $vf05
    qmfc2   $t5, $vf06
    qmfc2   $t6, $vf07
    qmfc2   $t7, $vf08
    .word   0x4A0026F8
    ppach   $t0, $t1, $t0
    ppach   $t2, $t3, $t2
    ppach   $t4, $t5, $t4
    ppach   $t6, $t7, $t6
    ppacb   $t0, $t2, $t0
    ppacb   $t4, $t6, $t4
    addiu   $v0, $v0, -1
    sq      $t0,  0($a0)
    sq      $t4, 16($a0)
    bgtz    $v0, 1b
    addiu   $a0, $a0, 32
    bgtzl   $at, 2b
    nop
    lq      $v1, 16($a1)
    addiu   $v0, $zero, 4
1:
    qmfc2.i $t0, vf01
    ctc2    $v1, $vi06
    qfsrv   $v1, $v1, $v1
    qmfc2   $t1, vf02
    qmfc2   $t2, vf03
    qmfc2   $t3, vf04
    qmfc2   $t4, vf05
    qmfc2   $t5, vf06
    qmfc2   $t6, vf07
    qmfc2   $t7, vf08
    .word   0x4A0026F8
    ppach   $t0, $t1, $t0
    ppach   $t2, $t3, $t2
    ppach   $t4, $t5, $t4
    ppach   $t6, $t7, $t6
    ppacb   $t0, $t4, $t0
    ppacb   $t2, $t6, $t2
    addiu   $v0, $v0, -1
    sq      $t0,  0($a0)
    sq      $t2, 64($a0)
    bgtzl   $v0, 1b
    addiu   $a0, $a0, 16
    jr      $ra
    nop
