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
.globl DSP_PackMB
.globl DSP_PackAddMB

.sdata
.align  4
s_pVUData:  .half   0x0004, 0x0008, 0x000C, 0x0020
            .half   0x0024, 0x0028, 0x002C, 0x0040
            .half   0x0044, 0x0048, 0x004C, 0x0000
            .half   0x0000, 0x0000, 0x0000, 0x0000

DSP_PackMB:
    lui     $t0, 0x3000
    la      $a1, s_pVUData
    pnor    $a2, $zero, $zero
    mtsah   $zero, 1
    lq      $v1, 0($a1)
    addiu   $at, $zero, 2
    or      $a0, $a0, $t0
    psrlh   $a2, $a2, 8
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
    .word   0x4A0028B8
    ppach   $t0, $t1, $t0
    ppach   $t2, $t3, $t2
    ppach   $t4, $t5, $t4
    ppach   $t6, $t7, $t6
    pminh   $t0, $a2, $t0
    pminh   $t2, $a2, $t2
    pminh   $t4, $a2, $t4
    pminh   $t6, $a2, $t6
    pmaxh   $t0, $zero, $t0
    pmaxh   $t2, $zero, $t2
    pmaxh   $t4, $zero, $t4
    pmaxh   $t6, $zero, $t6
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
    .word   0x4A0028B8
    ppach   $t0, $t1, $t0
    ppach   $t2, $t3, $t2
    ppach   $t4, $t5, $t4
    ppach   $t6, $t7, $t6
    pminh   $t0, $a2, $t0
    pminh   $t2, $a2, $t2
    pminh   $t4, $a2, $t4
    pminh   $t6, $a2, $t6
    pmaxh   $t0, $zero, $t0
    pmaxh   $t2, $zero, $t2
    pmaxh   $t4, $zero, $t4
    pmaxh   $t6, $zero, $t6
    ppacb   $t0, $t4, $t0
    ppacb   $t2, $t6, $t2
    addiu   $v0, $v0, -1
    sq      $t0,  0($a0)
    sq      $t2, 64($a0)
    bgtzl   $v0, 1b
    addiu   $a0, $a0, 16
    jr      $ra

_dsp_pack_addY:  # a0 - apDst, vi01 - vuMem, v0 - 255
    addiu   $at, $zero, 2
1:
    vlqi    $vf01, ($vi01++)
    vlqi    $vf02, ($vi01++)
    vlqi    $vf03, ($vi01++)
    vlqi    $vf04, ($vi01++)
    vlqi    $vf05, ($vi01++)
    vlqi    $vf06, ($vi01++)
    vlqi    $vf07, ($vi01++)
    vlqi    $vf08, ($vi01++)
    qmfc2   $t0, $vf01
    qmfc2   $t1, $vf02
    qmfc2   $t2, $vf03
    qmfc2   $t3, $vf04
    qmfc2   $t4, $vf05
    qmfc2   $t5, $vf06
    qmfc2   $t6, $vf07
    qmfc2   $t7, $vf08
    ppach   $t0, $t1, $t0
    ld      $t1,  0($a0)
    ppach   $t2, $t3, $t2
    pextlb  $t1, $zero, $t1
    ld      $t3, 16($a0)
    ppach   $t4, $t5, $t4
    pextlb  $t3, $zero, $t3
    ld      $t5, 32($a0)
    ppach   $t6, $t7, $t6
    pextlb  $t5, $zero, $t5
    ld      $t7, 48($a0)
    pextlb  $t7, $zero, $t7
    paddh   $t0, $t1, $t0
    paddh   $t2, $t3, $t2
    paddh   $t4, $t5, $t4
    paddh   $t6, $t7, $t6
    pmaxh   $t0, $zero, $t0
    pmaxh   $t2, $zero, $t2
    pmaxh   $t4, $zero, $t4
    pmaxh   $t6, $zero, $t6
    pminh   $t0, $v0, $t0
    pminh   $t2, $v0, $t2
    pminh   $t4, $v0, $t4
    pminh   $t6, $v0, $t6
    ppacb   $t0, $zero, $t0
    ppacb   $t2, $zero, $t2
    ppacb   $t4, $zero, $t4
    ppacb   $t6, $zero, $t6
    sd      $t0,  0($a0)
    sd      $t2, 16($a0)
    addu    $at, $at, -1
    sd      $t4, 32($a0)
    sd      $t6, 48($a0)
    bgtz    $at, 1b
    addiu   $a0, $a0, 64
    jr      $ra

_dsp_pack_addC:  # a0 - apDst, vi01 - vuMem, v0 - 255
    addiu   $at, $zero, 2
1:
    vlqi    $vf01, ($vi01++)
    vlqi    $vf02, ($vi01++)
    vlqi    $vf03, ($vi01++)
    vlqi    $vf04, ($vi01++)
    vlqi    $vf05, ($vi01++)
    vlqi    $vf06, ($vi01++)
    vlqi    $vf07, ($vi01++)
    vlqi    $vf08, ($vi01++)
    qmfc2   $t0, $vf01
    qmfc2   $t1, $vf02
    qmfc2   $t2, $vf03
    qmfc2   $t3, $vf04
    qmfc2   $t4, $vf05
    qmfc2   $t5, $vf06
    qmfc2   $t6, $vf07
    qmfc2   $t7, $vf08
    ppach   $t0, $t1, $t0
    ld      $t1,  0($a0)
    ppach   $t2, $t3, $t2
    pextlb  $t1, $zero, $t1
    ld      $t3,  8($a0)
    ppach   $t4, $t5, $t4
    pextlb  $t3, $zero, $t3
    ld      $t5, 16($a0)
    ppach   $t6, $t7, $t6
    pextlb  $t5, $zero, $t5
    ld      $t7, 24($a0)
    pextlb  $t7, $zero, $t7
    paddh   $t0, $t1, $t0
    paddh   $t2, $t3, $t2
    paddh   $t4, $t5, $t4
    paddh   $t6, $t7, $t6
    pmaxh   $t0, $zero, $t0
    pmaxh   $t2, $zero, $t2
    pmaxh   $t4, $zero, $t4
    pmaxh   $t6, $zero, $t6
    pminh   $t0, $v0, $t0
    pminh   $t2, $v0, $t2
    pminh   $t4, $v0, $t4
    pminh   $t6, $v0, $t6
    ppacb   $t0, $zero, $t0
    ppacb   $t2, $zero, $t2
    ppacb   $t4, $zero, $t4
    ppacb   $t6, $zero, $t6
    sd      $t0,  0($a0)
    sd      $t2,  8($a0)
    addu    $at, $at, -1
    sd      $t4, 16($a0)
    sd      $t6, 24($a0)
    bgtz    $at, 1b
    addiu   $a0, $a0, 32
    jr      $ra
   
DSP_PackAddMB:
    lui     $at, 0x1000
1:
    lw      $v0, 0x3800($at)
    nop
    nop
    andi    $v0, $v0, 3
    addu    $a2, $zero, $a0
    bne     $v0, $zero, 1b
    pnor    $v0, $zero, $zero
    addu    $t8, $zero, $ra
    psrlh   $v0, $v0, 8
    lb      $at, 0x0890($a1)
    addiu   $v1, $zero, 16
    qmfc2.i $zero, $vf00
    bgezall $at, _dsp_pack_addY
    ctc2    $v1, $vi01
    lb      $at, 0x0892($a1)
    addiu   $v1, $zero, 32
    addiu   $a0, $a2,   8
    bgezall $at, _dsp_pack_addY
    ctc2    $v1, $vi01
    lb      $at, 0x0894($a1)
    addiu   $v1, $zero, 48
    addiu   $a0, $a2, 128
    bgezall $at, _dsp_pack_addY
    ctc2    $v1, $vi01
    lb      $at, 0x0896($a1)
    addiu   $v1, $zero, 64
    addiu   $a0, $a2, 136
    bgezall $at, _dsp_pack_addY
    ctc2    $v1, $vi01
    lb      $at, 0x0898($a1)
    addiu   $v1, $zero, 80
    addiu   $a0, $a2, 256
    bgezall $at, _dsp_pack_addC
    ctc2    $v1, $vi01
    lb      $at, 0x089A($a1)
    addiu   $v1, $zero, 96
    addiu   $a0, $a2, 320
    bgezall $at, _dsp_pack_addC
    ctc2    $v1, $vi01
    addu    $ra, $zero, $t8
    jr      $ra

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
    nop
