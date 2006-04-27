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
.text

.set noat
.set volatile
.set noreorder
.set nomacro

.globl DSP_PutQPel16MC10
.globl DSP_PutQPel16MC20
.globl DSP_PutQPel16MC30
.globl DSP_PutQPel16MC01
.globl DSP_PutQPel16MC11
.globl DSP_PutQPel16MC21
.globl DSP_PutQPel16MC31
.globl DSP_PutQPel16MC02
.globl DSP_PutQPel16MC12
.globl DSP_PutQPel16MC22
.globl DSP_PutQPel16MC32
.globl DSP_PutQPel16MC03
.globl DSP_PutQPel16MC13
.globl DSP_PutQPel16MC23
.globl DSP_PutQPel16MC33
.globl DSP_PutNoRndQPel16MC10
.globl DSP_PutNoRndQPel16MC20
.globl DSP_PutNoRndQPel16MC30
.globl DSP_PutNoRndQPel16MC01
.globl DSP_PutNoRndQPel16MC11
.globl DSP_PutNoRndQPel16MC21
.globl DSP_PutNoRndQPel16MC31
.globl DSP_PutNoRndQPel16MC02
.globl DSP_PutNoRndQPel16MC12
.globl DSP_PutNoRndQPel16MC22
.globl DSP_PutNoRndQPel16MC32
.globl DSP_PutNoRndQPel16MC03
.globl DSP_PutNoRndQPel16MC13
.globl DSP_PutNoRndQPel16MC23
.globl DSP_PutNoRndQPel16MC33
.globl DSP_PutQPel816MC10
.globl DSP_PutQPel816MC20
.globl DSP_PutQPel816MC30
.globl DSP_PutQPel816MC01
.globl DSP_PutQPel816MC11
.globl DSP_PutQPel816MC21
.globl DSP_PutQPel816MC31
.globl DSP_PutQPel816MC02
.globl DSP_PutQPel816MC12
.globl DSP_PutQPel816MC22
.globl DSP_PutQPel816MC32
.globl DSP_PutQPel816MC03
.globl DSP_PutQPel816MC13
.globl DSP_PutQPel816MC23
.globl DSP_PutQPel816MC33
.globl DSP_PutNoRndQPel816MC10
.globl DSP_PutNoRndQPel816MC20
.globl DSP_PutNoRndQPel816MC30
.globl DSP_PutNoRndQPel816MC01
.globl DSP_PutNoRndQPel816MC11
.globl DSP_PutNoRndQPel816MC21
.globl DSP_PutNoRndQPel816MC31
.globl DSP_PutNoRndQPel816MC02
.globl DSP_PutNoRndQPel816MC12
.globl DSP_PutNoRndQPel816MC22
.globl DSP_PutNoRndQPel816MC32
.globl DSP_PutNoRndQPel816MC03
.globl DSP_PutNoRndQPel816MC13
.globl DSP_PutNoRndQPel816MC23
.globl DSP_PutNoRndQPel816MC33
.globl DSP_AvgQPel16MC10
.globl DSP_AvgQPel16MC20
.globl DSP_AvgQPel16MC30
.globl DSP_AvgQPel16MC01
.globl DSP_AvgQPel16MC11
.globl DSP_AvgQPel16MC21
.globl DSP_AvgQPel16MC31
.globl DSP_AvgQPel16MC02
.globl DSP_AvgQPel16MC12
.globl DSP_AvgQPel16MC22
.globl DSP_AvgQPel16MC32
.globl DSP_AvgQPel16MC03
.globl DSP_AvgQPel16MC13
.globl DSP_AvgQPel16MC23
.globl DSP_AvgQPel16MC33
.globl DSP_AvgQPel816MC10
.globl DSP_AvgQPel816MC20
.globl DSP_AvgQPel816MC30
.globl DSP_AvgQPel816MC01
.globl DSP_AvgQPel816MC11
.globl DSP_AvgQPel816MC21
.globl DSP_AvgQPel816MC31
.globl DSP_AvgQPel816MC02
.globl DSP_AvgQPel816MC12
.globl DSP_AvgQPel816MC22
.globl DSP_AvgQPel816MC32
.globl DSP_AvgQPel816MC03
.globl DSP_AvgQPel816MC13
.globl DSP_AvgQPel816MC23
.globl DSP_AvgQPel816MC33

_copy16:
    lui     $t1, 0x7000
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    lq      $t6, 0x220($t1)
    lq      $t7, 0x230($t1)
    lq      $t8, 0x250($t1)
    lq      $t5, 0x260($t1)
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 17
    addiu   $at, $zero, 1
    psrlh   $t9, $t8, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $t4, $t4, -1
    addiu   $v0, $v0, 16
    qfsrv   $t2, $t3, $t2
    addiu   $v1, $v1, -1
    beq     $t4, $zero, 2f
    sq      $t2,  0($t1)
    bgtz    $v1, 1b
    addiu   $t1, $t1, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    beq     $zero, $zero, 1b
    addu    $v1, $v1, $at
2:
    jr      $ra
    addiu   $v0, $t1, -256

_put_qpel16:
    addiu   $v1, $zero, 8
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
1:
    lq      $t0,  0($a1)
    lq      $t1, 16($a1)
    lq      $t2,  0($a2)
    lq      $t3, 16($a2)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextub  $t4, $zero, $t0
    pextlb  $t0, $zero, $t0
    pextub  $t5, $zero, $t1
    pextlb  $t1, $zero, $t1
    pextub  $t6, $zero, $t2
    pextlb  $t2, $zero, $t2
    pextub  $t7, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t2
    paddh   $t4, $t4, $t6
    paddh   $t1, $t1, $t3
    paddh   $t5, $t5, $t7
    paddh   $t0, $t0, $t9
    paddh   $t4, $t4, $t9
    paddh   $t1, $t1, $t9
    paddh   $t5, $t5, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t4, $t4, 1
    psrlh   $t1, $t1, 1
    psrlh   $t5, $t5, 1
    ppacb   $t0, $t4, $t0
    ppacb   $t1, $t5, $t1
    sq      $t0,  0($a0)
    sq      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    beq     $s1, $zero, 2f
    nop
    lq      $t0, 0($a1)
    lq      $t1, 0($a2)
    pextub  $t2, $zero, $t0
    pextlb  $t0, $zero, $t0
    pextub  $t3, $zero, $t1
    pextlb  $t1, $zero, $t1
    paddh   $t0, $t0, $t1
    paddh   $t2, $t2, $t3
    paddh   $t0, $t0, $t9
    paddh   $t2, $t2, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t2, $t2, 1
    ppacb   $t0, $t2, $t0
    sq      $t0, 0($a0)
2:
    jr      $ra
    nop

_put_qpel16_h_lowpass:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s0, 0x270($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
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
    addiu   $v0, $v0, 16
    mtsah   $zero, 1
    paddh   $t5, $t1, $t3
    pmulth  $zero, $t5, $t6
    qfsrv   $at, $t4, $t3
    pmsubh  $zero, $at, $t8
    pcpyh   $t5, $t1
    pcpyld  $t5, $t5, $t5
    mtsah   $zero, -1
    qfsrv   $t5, $t1, $t5
    pmsubh  $zero, $t5, $t8
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $at, $t5, $at
    pmaddh  $zero, $at, $t9
    srl     $t5, $t3, 16
    pcpyh   $t5, $t5
    pcpyld  $t5, $t5, $t5
    qfsrv   $at, $at, $t5
    pmsubh  $zero, $at, $s0
    pnor    $t5, $zero, $zero
    paddh   $zero, $zero, $zero
    mtsah   $zero, 3
    qfsrv   $at, $t2, $t1
    pmaddh  $zero, $at, $t9
    psrlh   $t5, $t5, 8
    pmaddh  $zero, $t7, $s0
    mtsah   $zero, 4
    qfsrv   $at, $t2, $t1
    pmsubh  $zero, $at, $s0
    paddh   $t1, $t2, $t4
    pmfhl.lh    $at
    pmulth  $zero, $t1, $t6
    mtsah   $zero, 6
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    qfsrv   $t5, $t4, $t3
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 0($a0)
    mtsah   $zero, 1
    pmsubh  $zero, $t5, $t8
    prevh   $at, $t4
    pcpyud  $at, $at, $at
    qfsrv   $t5, $at, $t4
    pmsubh  $zero, $t5, $t8
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmaddh  $zero, $t5, $t9
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmsubh  $zero, $t5, $s0
    pcpyud  $at, $t3, $zero
    pmaddh  $zero, $t7, $s0
    pcpyld  $at, $t4, $at
    pcpyud  $t4, $t4, $zero
    pmsubh  $zero, $at, $s0
    qfsrv   $at, $t4, $at
    pmaddh  $zero, $at, $t9
    pnor    $t5, $zero, $t5
    psrlh   $t5, $t5, 8
    pmfhl.lh    $at
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 8($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $v1, $v1, $s1
    addu    $a3, $zero, $zero
    bgtz    $v1, 1b
    addu    $s1, $zero, $zero
    jr      $ra
    nop

_put_qpel16_h_lowpass_copy:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s0, 0x270($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    sq      $t3, 0($s2)
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    pextub  $t4, $zero, $t4
    addiu   $v0, $v0, 16
    mtsah   $zero, 1
    paddh   $t5, $t1, $t3
    pmulth  $zero, $t5, $t6
    qfsrv   $at, $t4, $t3
    pmsubh  $zero, $at, $t8
    pcpyh   $t5, $t1
    pcpyld  $t5, $t5, $t5
    mtsah   $zero, -1
    qfsrv   $t5, $t1, $t5
    pmsubh  $zero, $t5, $t8
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $at, $t5, $at
    pmaddh  $zero, $at, $t9
    srl     $t5, $t3, 16
    pcpyh   $t5, $t5
    pcpyld  $t5, $t5, $t5
    qfsrv   $at, $at, $t5
    pmsubh  $zero, $at, $s0
    pnor    $t5, $zero, $zero
    paddh   $zero, $zero, $zero
    mtsah   $zero, 3
    qfsrv   $at, $t2, $t1
    pmaddh  $zero, $at, $t9
    psrlh   $t5, $t5, 8
    pmaddh  $zero, $t7, $s0
    mtsah   $zero, 4
    qfsrv   $at, $t2, $t1
    pmsubh  $zero, $at, $s0
    paddh   $t1, $t2, $t4
    pmfhl.lh    $at
    pmulth  $zero, $t1, $t6
    mtsah   $zero, 6
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    qfsrv   $t5, $t4, $t3
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 0($a0)
    mtsah   $zero, 1
    pmsubh  $zero, $t5, $t8
    prevh   $at, $t4
    pcpyud  $at, $at, $at
    qfsrv   $t5, $at, $t4
    pmsubh  $zero, $t5, $t8
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmaddh  $zero, $t5, $t9
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmsubh  $zero, $t5, $s0
    addiu   $s2, $s2, 16
    pmaddh  $zero, $t7, $s0
    pcpyud  $at, $t3, $zero
    pcpyld  $at, $t4, $at
    pcpyud  $t4, $t4, $zero
    pmsubh  $zero, $at, $s0
    qfsrv   $at, $t4, $at
    pmaddh  $zero, $at, $t9
    pnor    $t5, $zero, $t5
    psrlh   $t5, $t5, 8
    pmfhl.lh    $at
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 8($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $v1, $v1, $s1
    addu    $a3, $zero, $zero
    bgtz    $v1, 1b
    addu    $s1, $zero, $zero
    jr      $ra
    nop

_put_qpel16_h_lowpass_copy_x:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s0, 0x270($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
    mtsab   $a2, 0
    qfsrv   $t3, $t2, $t1
    qfsrv   $t4, $t1, $t2
    pextlb  $t1, $zero, $t3
    addiu   $v1, $v1, -1
    pextub  $t2, $zero, $t3
    mtsab   $zero, 1
    qfsrv   $t4, $t4, $t3
    pextlb  $t3, $zero, $t4
    sq      $t4, 0($s2)
    pextub  $t4, $zero, $t4
    addiu   $v0, $v0, 16
    mtsah   $zero, 1
    paddh   $t5, $t1, $t3
    pmulth  $zero, $t5, $t6
    qfsrv   $at, $t4, $t3
    pmsubh  $zero, $at, $t8
    pcpyh   $t5, $t1
    pcpyld  $t5, $t5, $t5
    mtsah   $zero, -1
    qfsrv   $t5, $t1, $t5
    pmsubh  $zero, $t5, $t8
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $at, $t5, $at
    pmaddh  $zero, $at, $t9
    srl     $t5, $t3, 16
    pcpyh   $t5, $t5
    pcpyld  $t5, $t5, $t5
    qfsrv   $at, $at, $t5
    pmsubh  $zero, $at, $s0
    pnor    $t5, $zero, $zero
    paddh   $zero, $zero, $zero
    mtsah   $zero, 3
    qfsrv   $at, $t2, $t1
    pmaddh  $zero, $at, $t9
    psrlh   $t5, $t5, 8
    pmaddh  $zero, $t7, $s0
    mtsah   $zero, 4
    qfsrv   $at, $t2, $t1
    pmsubh  $zero, $at, $s0
    paddh   $t1, $t2, $t4
    pmfhl.lh    $at
    pmulth  $zero, $t1, $t6
    mtsah   $zero, 6
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    qfsrv   $t5, $t4, $t3
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 0($a0)
    mtsah   $zero, 1
    pmsubh  $zero, $t5, $t8
    prevh   $at, $t4
    pcpyud  $at, $at, $at
    qfsrv   $t5, $at, $t4
    pmsubh  $zero, $t5, $t8
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmaddh  $zero, $t5, $t9
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmsubh  $zero, $t5, $s0
    addiu   $s2, $s2, 16
    pmaddh  $zero, $t7, $s0
    pcpyud  $at, $t3, $zero
    pcpyld  $at, $t4, $at
    pcpyud  $t4, $t4, $zero
    pmsubh  $zero, $at, $s0
    qfsrv   $at, $t4, $at
    pmaddh  $zero, $at, $t9
    pnor    $t5, $zero, $t5
    psrlh   $t5, $t5, 8
    pmfhl.lh    $at
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    pmaxh   $at, $zero, $at
    ppacb   $at, $zero, $at
    sd      $at, 8($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $v1, $v1, $s1
    addu    $a3, $zero, $zero
    bgtz    $v1, 1b
    addu    $s1, $zero, $zero
    jr      $ra
    nop

_put_qpel16_v_lowpass:
    ld      $a1,  0($v0)
    ld      $a2, 16($v0)
    ld      $a3, 32($v0)
    ld      $at, 48($v0)
    ld      $v1, 64($v0)
    pextlb  $a1, $zero, $a1
    pextlb  $a2, $zero, $a2
    pextlb  $a3, $zero, $a3
    pextlb  $at, $zero, $at
    pextlb  $v1, $zero, $v1
    paddh   $t0, $a1, $a2
    pmulth  $zero, $t0, $t6
    paddh   $t1, $a1, $a3
    paddh   $t2, $a2, $at
    pmfhl.lh    $t0
    pmulth  $zero, $t1, $t8
    paddh   $t0, $t0, $t7
    paddh   $t1, $a3, $v1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psubh   $t0, $t0, $t1
    pmfhl.lh    $t1
    pmulth  $zero, $t3, $t6
    paddh   $t3, $a1, $at
    paddh   $t0, $t0, $t1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t2
    ld      $t4, 80($v0)
    pmulth  $zero, $t3, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $v1
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pextlb  $t4, $zero, $t4
    sd      $t0, 0($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    paddh   $t3, $a2, $t4
    psubh   $t0, $t2, $t0
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t1
    paddh   $t0, $t0, $t1
    paddh   $t1, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t1, $t6
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t2
    pmulth  $zero, $t1, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $t4
    sd      $t0, 16($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    ld      $t1, 96($v0)
    psubh   $t0, $t2, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t2, $t1, $a1
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t2
    paddh   $t0, $t0, $t2
    paddh   $t2, $at, $v1
    pmulth  $zero, $t2, $t6
    paddh   $t2, $a3, $t4
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    paddh   $t2, $a2, $t1
    ppacb   $t0, $zero, $t0
    sd      $t0, 32($a0)
    paddh   $t0, $t3, $t7
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 112($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $a1, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $a1, $t6
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 48($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 128($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 64($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    ld      $a2, 144($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a2, $zero, $a2
    paddh   $t3, $a3, $a2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t1, $t2
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $a3, $t4, $a1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    sd      $t0, 80($a0)
    paddh   $t0, $t3, $t7
    paddh   $a3, $v1, $a2
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t9
    ld      $a3, 160($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a3, $zero, $a3
    paddh   $t3, $at, $a3
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $at, $t1, $a2
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t8
    sd      $t0, 96($a0)
    paddh   $t0, $t3, $t7
    paddh   $at, $t4, $a3
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t9
    ld      $at, 176($v0)
    psubh   $t0, $t0, $t3
    pextlb  $at, $zero, $at
    paddh   $t3, $v1, $at
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a1, $a2
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $v1, $t2, $a3
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $v1, $t8
    sd      $t0, 112($a0)
    paddh   $t0, $t3, $t7
    paddh   $v1, $t1, $at
    pmfhl.lh    $t3
    pmulth  $zero, $v1, $t9
    ld      $v1, 192($v0)
    psubh   $t0, $t0, $t3
    pextlb  $v1, $zero, $v1
    paddh   $t3, $t4, $v1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $t4, $a1, $at
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t4, $t8
    sd      $t0, 128($a0)
    paddh   $t0, $t3, $t7
    paddh   $t4, $t2, $v1
    pmfhl.lh    $t3
    pmulth  $zero, $t4, $t9
    ld      $t4, 208($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t4, $zero, $t4
    paddh   $t3, $t1, $t4
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t1, $t8
    sd      $t0, 144($a0)
    paddh   $t0, $t3, $t7
    paddh   $t1, $a1, $t4
    pmfhl.lh    $t3
    pmulth  $zero, $t1, $t9
    ld      $t1, 224($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t1, $zero, $t1
    paddh   $t3, $t2, $t1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $at, $v1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $t2, $a3, $t4
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    sd      $t0, 160($a0)
    paddh   $t0, $t3, $t7
    paddh   $t2, $a2, $t1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 240($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $t0, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 176($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 256($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 192($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a3, $a1
    psubh   $t0, $t0, $t3
    paddh   $a2, $t1, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t6
    paddh   $t0, $t0, $t3
    paddh   $a3, $t4, $a1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    sd      $t0, 208($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $v1, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    psubh   $t0, $t0, $at
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $at, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $at, $t6
    paddh   $a2, $t1, $a1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 224($a0)
    paddh   $t0, $t3, $t7
    paddh   $t3, $t4, $t2
    psubh   $t0, $t0, $v1
    pmfhl.lh    $at
    pmulth  $zero, $t3, $t9
    psubh   $t0, $t0, $t1
    psubh   $t0, $t0, $at
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    sd      $t0, 240($a0)
    addiu   $a0, $a0, 8
    jr      $ra
    addiu   $v0, $v0, 8

_avg_qpel16:
    ADDIU   $v1, $zero, 8
    PNOR    $t9, $zero, $zero
    PSRLH   $t9, $t9, 15
1:
    lq      $t0,  0($a1)
    lq      $t1, 16($a1)
    lq      $t2,  0($a2)
    lq      $t3, 16($a2)
    lq      $t8,  0($a0)
    lq      $v0, 16($a0)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextub  $t4, $zero, $t0
    pextlb  $t0, $zero, $t0
    pextub  $t5, $zero, $t1
    pextlb  $t1, $zero, $t1
    pextub  $t6, $zero, $t2
    pextlb  $t2, $zero, $t2
    pextub  $t7, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t2
    paddh   $t4, $t4, $t6
    paddh   $t1, $t1, $t3
    paddh   $t5, $t5, $t7
    paddh   $t0, $t0, $t9
    paddh   $t4, $t4, $t9
    paddh   $t1, $t1, $t9
    paddh   $t5, $t5, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t4, $t4, 1
    psrlh   $t1, $t1, 1
    psrlh   $t5, $t5, 1
    pextub  $t2, $zero, $t8
    pextlb  $t8, $zero, $t8
    pextub  $t3, $zero, $v0
    pextlb  $v0, $zero, $v0
    paddh   $t0, $t0, $t8
    paddh   $t4, $t4, $t2
    paddh   $t1, $t1, $v0
    paddh   $t5, $t5, $t3
    paddh   $t0, $t0, $t9
    paddh   $t4, $t4, $t9
    paddh   $t1, $t1, $t9
    paddh   $t5, $t5, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t4, $t4, 1
    psrlh   $t1, $t1, 1
    psrlh   $t5, $t5, 1
    ppacb   $t0, $t4, $t0
    ppacb   $t1, $t5, $t1
    sq      $t0,  0($a0)
    sq      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    jr      $ra
    nop

_avg_qpel16_h_lowpass:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t8, 0x250($v0)
    lq      $s0, 0x270($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
1:
    lq      $t1,   0($v0)
    lq      $t2, 384($v0)
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
    addiu   $v0, $v0, 16
    mtsah   $zero, 1
    paddh   $t5, $t1, $t3
    pmulth  $zero, $t5, $t6
    qfsrv   $at, $t4, $t3
    pmsubh  $zero, $at, $t8
    pcpyh   $t5, $t1
    pcpyld  $t5, $t5, $t5
    mtsah   $zero, -1
    qfsrv   $t5, $t1, $t5
    pmsubh  $zero, $t5, $t8
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $at, $t5, $at
    pmaddh  $zero, $at, $t9
    srl     $t5, $t3, 16
    pcpyh   $t5, $t5
    pcpyld  $t5, $t5, $t5
    qfsrv   $at, $at, $t5
    pmsubh  $zero, $at, $s0
    pnor    $t5, $zero, $zero
    paddh   $zero, $zero, $zero
    mtsah   $zero, 3
    qfsrv   $at, $t2, $t1
    pmaddh  $zero, $at, $t9
    psrlh   $t5, $t5, 8
    pmaddh  $zero, $t7, $s0
    mtsah   $zero, 4
    qfsrv   $at, $t2, $t1
    pmsubh  $zero, $at, $s0
    paddh   $t1, $t2, $t4
    pmfhl.lh    $at
    pmulth  $zero, $t1, $t6
    ld      $t1, 0($a0)
    mtsah   $zero, 6
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    qfsrv   $t5, $t4, $t3
    pextlb  $t1, $zero, $t1
    pmaxh   $at, $zero, $at
    paddh   $at, $at, $t1
    paddh   $at, $at, $s0
    psrlh   $at, $at, 1
    ppacb   $at, $zero, $at
    sd      $at, 0($a0)
    mtsah   $zero, 1
    pmsubh  $zero, $t5, $t8
    prevh   $at, $t4
    pcpyud  $at, $at, $at
    qfsrv   $t5, $at, $t4
    pmsubh  $zero, $t5, $t8
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmaddh  $zero, $t5, $t9
    qfsrv   $at, $at, $at
    qfsrv   $t5, $at, $t5
    pmsubh  $zero, $t5, $s0
    ld      $t1, 8($a0)
    pmaddh  $zero, $t7, $s0
    pcpyud  $at, $t3, $zero
    pcpyld  $at, $t4, $at
    pcpyud  $t4, $t4, $zero
    pmsubh  $zero, $at, $s0
    qfsrv   $at, $t4, $at
    pmaddh  $zero, $at, $t9
    pnor    $t5, $zero, $t5
    pextlb  $t1, $zero, $t1
    psrlh   $t5, $t5, 8
    pmfhl.lh    $at
    psrah   $at, $at, 5
    pminh   $at, $t5, $at
    pmaxh   $at, $zero, $at
    paddh   $at, $at, $t1
    paddh   $at, $at, $s0
    psrlh   $at, $at, 1
    ppacb   $at, $zero, $at
    sd      $at, 8($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $v1, $v1, $s1
    addu    $a3, $zero, $zero
    bgtz    $v1, 1b
    addu    $s1, $zero, $zero
    jr      $ra
    nop

_avg_qpel16_v_lowpass:
    ld      $a1,  0($v0)
    ld      $a2, 16($v0)
    ld      $a3, 32($v0)
    ld      $at, 48($v0)
    ld      $v1, 64($v0)
    pextlb  $a1, $zero, $a1
    pextlb  $a2, $zero, $a2
    pextlb  $a3, $zero, $a3
    pextlb  $at, $zero, $at
    pextlb  $v1, $zero, $v1
    paddh   $t0, $a1, $a2
    pmulth  $zero, $t0, $t6
    paddh   $t1, $a1, $a3
    paddh   $t2, $a2, $at
    pmfhl.lh    $t0
    pmulth  $zero, $t1, $t8
    paddh   $t0, $t0, $t7
    paddh   $t1, $a3, $v1
    pmfhl.lh     $t3
    pmulth  $zero, $t2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psubh   $t0, $t0, $t1
    pmfhl.lh    $t1
    pmulth  $zero, $t3, $t6
    paddh   $t3, $a1, $at
    paddh   $t0, $t0, $t1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t2
    ld      $t4, 80($v0)
    ld      $t1,  0($a0)
    pmulth  $zero, $t3, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $v1
    pextlb  $t1, $zero, $t1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pextlb  $t4, $zero, $t4
    sd      $t0, 0($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    paddh   $t3, $a2, $t4
    psubh   $t0, $t2, $t0
    psubh   $t0, $t0, $t3
    ld      $t2, 16($a0)
    pmfhl.lh    $t1
    paddh   $t0, $t0, $t1
    paddh   $t1, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t1, $t6
    pextlb  $t2, $zero, $t2
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t2
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t2
    pmulth  $zero, $t1, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $t4
    sd      $t0, 16($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    ld      $t1, 96($v0)
    psubh   $t0, $t2, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t2, $t1, $a1
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t2
    paddh   $t0, $t0, $t2
    paddh   $t2, $at, $v1
    pmulth  $zero, $t2, $t6
    paddh   $t2, $a3, $t4
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ld      $s1, 32($a0)
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    pextlb  $s1, $zero, $s1
    paddh   $t2, $a2, $t1
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    sd      $t0, 32($a0)
    paddh   $t0, $t3, $t7
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 112($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $a1, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $a1, $t6
    ld      $s1, 48($a0)
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 48($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 128($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 64($a0)
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 64($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    ld      $a2, 144($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a2, $zero, $a2
    paddh   $t3, $a3, $a2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t1, $t2
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 80($a0)
    paddh   $a3, $t4, $a1
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    sd      $t0, 80($a0)
    paddh   $t0, $t3, $t7
    paddh   $a3, $v1, $a2
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t9
    ld      $a3, 160($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a3, $zero, $a3
    paddh   $t3, $at, $a3
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 96($a0)
    paddh   $at, $t1, $a2
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t8
    sd      $t0, 96($a0)
    paddh   $t0, $t3, $t7
    paddh   $at, $t4, $a3
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t9
    ld      $at, 176($v0)
    psubh   $t0, $t0, $t3
    pextlb  $at, $zero, $at
    paddh   $t3, $v1, $at
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a1, $a2
    psrah   $t0, $t0, 5
    pmulth   $zero, $t3, $t6
    ld      $s1, 112($a0)
    paddh   $v1, $t2, $a3
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $v1, $t8
    sd      $t0, 112($a0)
    paddh   $t0, $t3, $t7
    paddh   $v1, $t1, $at
    pmfhl.lh    $t3
    pmulth  $zero, $v1, $t9
    ld      $v1, 192($v0)
    psubh   $t0, $t0, $t3
    pextlb  $v1, $zero, $v1
    paddh   $t3, $t4, $v1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 128($a0)
    paddh   $t4, $a1, $at
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t4, $t8
    sd      $t0, 128($a0)
    paddh   $t0, $t3, $t7
    paddh   $t4, $t2, $v1
    pmfhl.lh    $t3
    pmulth  $zero, $t4, $t9
    ld      $t4, 208($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t4, $zero, $t4
    paddh   $t3, $t1, $t4
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 144($a0)
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t1, $t8
    sd      $t0, 144($a0)
    paddh   $t0, $t3, $t7
    paddh   $t1, $a1, $t4
    pmfhl.lh    $t3
    pmulth  $zero, $t1, $t9
    ld      $t1, 224($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t1, $zero, $t1
    paddh   $t3, $t2, $t1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $at, $v1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 160($a0)
    paddh   $t2, $a3, $t4
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    sd      $t0, 160($a0)
    paddh   $t0, $t3, $t7
    paddh   $t2, $a2, $t1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 240($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 176($a0)
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $t0, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 176($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 256($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $s1, 192($a0)
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 192($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a3, $a1
    psubh   $t0, $t0, $t3
    paddh   $a2, $t1, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t6
    ld      $s1, 208($a0)
    paddh   $t0, $t0, $t3
    paddh   $a3, $t4, $a1
    pextlb  $s1, $zero, $s1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    sd      $t0, 208($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $v1, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    psubh   $t0, $t0, $at
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $at, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $at, $t6
    ld      $s1, 224($a0)
    paddh   $a2, $t1, $a1
    pminh   $t0, $t5, $t0
    pextlb  $s1, $zero, $s1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 224($a0)
    paddh   $t0, $t3, $t7
    paddh   $t3, $t4, $t2
    psubh   $t0, $t0, $v1
    pmfhl.lh    $at
    pmulth  $zero, $t3, $t9
    psubh   $t0, $t0, $t1
    psubh   $t0, $t0, $at
    ld      $s1, 240($a0)
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    psrah   $t0, $t0, 5
    pextlb  $s1, $zero, $s1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $s1
    paddh   $t0, $t0, $s0
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    sd      $t0, 240($a0)
    addiu   $a0, $a0, 8
    jr      $ra
    addiu   $v0, $v0, 8

_put_no_rnd_qpel16:
    addiu   $v1, $zero, 8
1:
    lq      $t0,  0($a1)
    lq      $t1, 16($a1)
    lq      $t2,  0($a2)
    lq      $t3, 16($a2)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextub  $t4, $zero, $t0
    pextlb  $t0, $zero, $t0
    pextub  $t5, $zero, $t1
    pextlb  $t1, $zero, $t1
    pextub  $t6, $zero, $t2
    pextlb  $t2, $zero, $t2
    pextub  $t7, $zero, $t3
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t2
    paddh   $t4, $t4, $t6
    paddh   $t1, $t1, $t3
    paddh   $t5, $t5, $t7
    psrlh   $t0, $t0, 1
    psrlh   $t4, $t4, 1
    psrlh   $t1, $t1, 1
    psrlh   $t5, $t5, 1
    ppacb   $t0, $t4, $t0
    ppacb   $t1, $t5, $t1
    sq      $t0,  0($a0)
    sq      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    beq     $s1, $zero, 2f
    nop
    lq      $t0, 0($a1)
    lq      $t1, 0($a2)
    pextub  $t2, $zero, $t0
    pextlb  $t0, $zero, $t0
    pextub  $t3, $zero, $t1
    pextlb  $t1, $zero, $t1
    paddh   $t0, $t0, $t1
    paddh   $t2, $t2, $t3
    psrlh   $t0, $t0, 1
    psrlh   $t2, $t2, 1
    ppacb   $t0, $t2, $t0
    sq      $t0, 0($a0)
2:
    jr      $ra
    nop

_copy8_16:
    lui     $t1, 0x7000
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    mtsab   $a2, 0
    lq      $t6, 0x220($t1)
    lq      $t7, 0x230($t1)
    lq      $t8, 0x250($t1)
    lq      $t5, 0x260($t1)
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 9
    psrlh   $t9, $t8, 1
1:
    lq      $t2,   0($v0)
    lq      $t3, 384($v0)
    addiu   $v0, $v0, 16
    addiu   $t4, $t4, -1
    qfsrv   $t2, $t3, $t2
    addiu   $v1, $v1, -1
    beq     $t4, $zero, 2f
    sq      $t2,  0($t1)
    bgtz    $v1, 1b
    addiu   $t1, $t1, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    beq     $zero, $zero, 1b
    addu    $a3, $zero, $zero
2:
    jr      $ra
    addiu   $v0, $t1, -128

_put_qpel8_16:
    addiu   $v1, $zero, 4
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
1:
    ld      $t0,  0($a1)
    ld      $t1, 16($a1)
    ld      $t2,  0($a2)
    ld      $t3, 16($a2)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextlb  $t0, $zero, $t0 
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t2
    paddh   $t1, $t1, $t3
    paddh   $t0, $t0, $t9
    paddh   $t1, $t1, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t1, $t1, 1
    ppacb   $t0, $zero, $t0
    ppacb   $t1, $zero, $t1
    sd      $t0,  0($a0)
    sd      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    beq     $s1, $zero, 2f
    nop
    ld      $t0, 0($a1)
    ld      $t1, 0($a2)
    pextlb  $t0, $zero, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t0, $t0, $t1
    paddh   $t0, $t0, $t9
    psrlh   $t0, $t0, 1
    ppacb   $t0, $t2, $t0
    sd      $t0, 0($a0)
2:
    jr      $ra
    nop

_put_qpel8_16_h_lowpass:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s1, 0x260($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 8
    addu    $t4, $t4, $s0
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
    paddh   $at, $t3, $t1
    mtsah   $zero, -1
    pmulth  $zero, $at, $t6
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $t2, $t3, $at
    prevh   $at, $t1
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t1
    paddh   $at, $t2, $t5
    pmsubh  $zero, $at, $t8
    pcpyh   $at, $t1
    pcpyld  $at, $at, $at
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    prevh   $at, $t3
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t5
    paddh   $at, $t2, $t5
    pmaddh  $zero, $at, $t9
    prevh   $at, $t5
    qfsrv   $t5, $at, $t5
    prevh   $at, $t2
    pmfhl.lh    $t1
    psubh   $t1, $t1, $t5
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    psubh   $t1, $t1, $t2
    paddh   $t1, $t1, $t7
    psrah   $t1, $t1, 5
    pminh   $t1, $s1, $t1
    pmaxh   $t1, $zero, $t1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $a3, $zero, $zero
    beq     $zero, $zero, 1b
    addiu   $v1, $v1, 16
2:
    jr      $ra
    nop

_put_qpel8_16_h_lowpass_copy:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s1, 0x260($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 8
    addu    $t4, $t4, $s0
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
    sd      $t3, 0($s2)
    pextlb  $t3, $zero, $t3
    pextlb  $t1, $zero, $t1
    paddh   $at, $t3, $t1
    mtsah   $zero, -1
    pmulth  $zero, $at, $t6
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $t2, $t3, $at
    prevh   $at, $t1
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t1
    paddh   $at, $t2, $t5
    pmsubh  $zero, $at, $t8
    pcpyh   $at, $t1
    pcpyld  $at, $at, $at
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    prevh   $at, $t3
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t5
    paddh   $at, $t2, $t5
    pmaddh  $zero, $at, $t9
    prevh   $at, $t5
    qfsrv   $t5, $at, $t5
    prevh   $at, $t2
    addiu   $s2, $s2, 16
    pmfhl.lh    $t1
    psubh   $t1, $t1, $t5
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    psubh   $t1, $t1, $t2
    paddh   $t1, $t1, $t7
    psrah   $t1, $t1, 5
    pminh   $t1, $s1, $t1
    pmaxh   $t1, $zero, $t1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $a3, $zero, $zero
    beq     $zero, $zero, 1b
    addiu   $v1, $v1, 16
2:
    jr      $ra
    nop

_put_qpel8_16_h_lowpass_copy_x:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t8, 0x250($v0)
    lq      $s1, 0x260($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 8
    addu    $t4, $t4, $s0
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
    sd      $t1, 0($s2)
    pextlb  $t1, $zero, $t1
    paddh   $at, $t3, $t1
    mtsah   $zero, -1
    pmulth  $zero, $at, $t6
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $t2, $t3, $at
    prevh   $at, $t1
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t1
    paddh   $at, $t2, $t5
    pmsubh  $zero, $at, $t8
    pcpyh   $at, $t1
    pcpyld  $at, $at, $at
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    prevh   $at, $t3
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t5
    paddh   $at, $t2, $t5
    pmaddh  $zero, $at, $t9
    prevh   $at, $t5
    qfsrv   $t5, $at, $t5
    prevh   $at, $t2
    addiu   $s2, $s2, 16
    pmfhl.lh    $t1
    psubh   $t1, $t1, $t5
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    psubh   $t1, $t1, $t2
    paddh   $t1, $t1, $t7
    psrah   $t1, $t1, 5
    pminh   $t1, $s1, $t1
    pmaxh   $t1, $zero, $t1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $a3, $zero, $zero
    beq     $zero, $zero, 1b
    addiu   $v1, $v1, 16
2:
    jr      $ra
    nop

_put_qpel8_16_v_lowpass:
    ld      $a1,  0($v0)
    ld      $a2, 16($v0)
    ld      $a3, 32($v0)
    ld      $at, 48($v0)
    ld      $v1, 64($v0)
    pextlb  $a1, $zero, $a1 
    pextlb  $a2, $zero, $a2 
    pextlb  $a3, $zero, $a3 
    pextlb  $at, $zero, $at 
    pextlb  $v1, $zero, $v1 
    paddh   $t0, $a1, $a2
    pmulth  $zero, $t0, $t6
    paddh   $t1, $a1, $a3
    paddh   $t2, $a2, $at
    pmfhl.lh    $t0
    pmulth  $zero, $t1, $t8
    paddh   $t0, $t0, $t7
    paddh   $t1, $a3, $v1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psubh   $t0, $t0, $t1
    pmfhl.lh    $t1
    pmulth  $zero, $t3, $t6
    paddh   $t3, $a1, $at
    paddh   $t0, $t0, $t1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t2
    ld      $t4, 80($v0)
    pmulth  $zero, $t3, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $v1
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pextlb  $t4, $zero, $t4
    sd      $t0, 0($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    paddh   $t3, $a2, $t4
    psubh   $t0, $t2, $t0
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t1
    paddh   $t0, $t0, $t1
    paddh   $t1, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t1, $t6
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0 
    pmfhl.lh    $t2
    pmulth  $zero, $t1, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $t4
    sd      $t0, 16($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    ld      $t1, 96($v0)
    psubh   $t0, $t2, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t2, $t1, $a1
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t2
    paddh   $t0, $t0, $t2
    paddh   $t2, $at, $v1
    pmulth  $zero, $t2, $t6
    paddh   $t2, $a3, $t4
    psrah   $t0, $t0, 5 
    pminh   $t0, $t5, $t0 
    pmaxh   $t0, $zero, $t0 
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    paddh   $t2, $a2, $t1
    ppacb   $t0, $zero, $t0
    sd      $t0, 32($a0)
    paddh   $t0, $t3, $t7
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 112($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $a1, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $a1, $t6
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 48($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 128($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 64($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a3, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t1, $t2
    psrah   $t0, $t0, 5 
    pmulth  $zero, $t3, $t6
    paddh   $a3, $t4, $a1
    pminh   $t0, $t5, $t0 
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    sd      $t0, 80($a0)
    paddh   $t0, $t3, $t7
    paddh   $a3, $v1, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $at, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    paddh   $at, $t1, $a1
    pminh   $t0, $t5, $t0 
    pmaxh   $t0, $zero, $t0 
    ppacb   $t0, $zero, $t0 
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t8
    sd      $t0, 96($a0)
    paddh   $t0, $t3, $t7
    paddh   $at, $t4, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $v1, $t1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    ppacb   $t0, $zero, $t0
    jr      $ra
    sd      $t0, 112($a0)

_avg_qpel8_16:
    addiu   $v1, $zero, 4
    pnor    $t9, $zero, $zero
    psrlh   $t9, $t9, 15
1:
    ld      $t0,  0($a1)
    ld      $t1, 16($a1)
    ld      $t2,  0($a2)
    ld      $t3, 16($a2)
    ld      $t4,  0($a0)
    ld      $t5, 16($a0)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextlb  $t0, $zero, $t0
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    pextlb  $t3, $zero, $t3
    pextlb  $t4, $zero, $t4
    pextlb  $t5, $zero, $t5
    paddh   $t0, $t0, $t2
    paddh   $t1, $t1, $t3
    paddh   $t0, $t0, $t9
    paddh   $t1, $t1, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t1, $t1, 1
    paddh   $t0, $t0, $t4
    paddh   $t1, $t1, $t5
    paddh   $t0, $t0, $t9
    paddh   $t1, $t1, $t9
    psrlh   $t0, $t0, 1
    psrlh   $t1, $t1, 1
    ppacb   $t0, $zero, $t0
    ppacb   $t1, $zero, $t1
    sd      $t0,  0($a0)
    sd      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    jr      $ra
    nop

_avg_qpel8_16_h_lowpass:
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t8, 0x250($v0)
    lq      $s1, 0x260($v0)
    psrlh   $t9, $t8, 1
    addiu   $v0, $zero, 16
    subu    $v1, $v0, $a3
    sll     $v0, $a3, 4
    addu    $v0, $v0, $a1
    addiu   $t4, $zero, 8
    psrlh   $s0, $t9, 1
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
    paddh   $at, $t3, $t1
    mtsah   $zero, -1
    pmulth  $zero, $at, $t6
    pcpyh   $at, $t3
    pcpyld  $at, $at, $at
    qfsrv   $t2, $t3, $at
    prevh   $at, $t1
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t1
    paddh   $at, $t2, $t5
    pmsubh  $zero, $at, $t8
    pcpyh   $at, $t1
    pcpyld  $at, $at, $at
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    prevh   $at, $t3
    pcpyh   $at, $at
    pcpyud  $at, $at, $at
    mtsah   $zero, 1
    qfsrv   $t5, $at, $t5
    paddh   $at, $t2, $t5
    pmaddh  $zero, $at, $t9
    prevh   $at, $t5
    qfsrv   $t5, $at, $t5
    prevh   $at, $t2
    pmfhl.lh    $t1
    psubh   $t1, $t1, $t5
    mtsah   $zero, -1
    qfsrv   $t2, $t2, $at
    psubh   $t1, $t1, $t2
    ld      $t2, 0($a0)
    paddh   $t1, $t1, $t7
    psrah   $t1, $t1, 5
    pextlb  $t2, $zero, $t2
    pminh   $t1, $s1, $t1
    pmaxh   $t1, $zero, $t1
    paddh   $t1, $t1, $t2
    paddh   $t1, $t1, $s0
    psrlh   $t1, $t1, 1
    ppacb   $t1, $zero, $t1
    beq     $t4, $zero, 2f
    sd      $t1, 0($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 16
    addu    $v0, $a1, $t0
    addu    $v1, $zero, $a3
    addu    $a3, $zero, $zero
    beq     $zero, $zero, 1b
    addiu   $v1, $v1, 16
2:
    jr      $ra
    nop

_avg_qpel8_16_v_lowpass:
    ld      $a1,  0($v0)
    ld      $a2, 16($v0)
    ld      $a3, 32($v0)
    ld      $at, 48($v0)
    ld      $v1, 64($v0)
    pextlb  $a1, $zero, $a1
    pextlb  $a2, $zero, $a2
    pextlb  $a3, $zero, $a3
    pextlb  $at, $zero, $at
    pextlb  $v1, $zero, $v1
    paddh   $t0, $a1, $a2
    pmulth  $zero, $t0, $t6
    paddh   $t1, $a1, $a3
    paddh   $t2, $a2, $at
    pmfhl.lh    $t0
    pmulth  $zero, $t1, $t8
    paddh   $t0, $t0, $t7
    paddh   $t1, $a3, $v1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a2, $a3
    psubh   $t0, $t0, $t1
    pmfhl.lh    $t1
    pmulth  $zero, $t3, $t6
    paddh   $t3, $a1, $at
    paddh   $t0, $t0, $t1
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmfhl.lh    $t2
    ld      $t4, 80($v0)
    ld      $t1,  0($a0)
    pmulth  $zero, $t3, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $v1
    pextlb  $t1, $zero, $t1
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t1
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pextlb  $t4, $zero, $t4
    sd      $t0, 0($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    paddh   $t3, $a2, $t4
    psubh   $t0, $t2, $t0
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t1
    paddh   $t0, $t0, $t1
    paddh   $t1, $a3, $at
    psrah   $t0, $t0, 5
    pmulth  $zero, $t1, $t6
    ld      $t3, 16($a0)
    paddh   $t1, $a2, $v1
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t2
    pmulth  $zero, $t1, $t8
    paddh   $t2, $t2, $t7
    paddh   $t3, $a1, $t4
    sd      $t0, 16($a0)
    pmfhl.lh    $t0
    pmulth  $zero, $t3, $t9
    ld      $t1, 96($v0)
    psubh   $t0, $t2, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t2, $t1, $a1
    psubh   $t0, $t0, $t2
    pmfhl.lh    $t2
    paddh   $t0, $t0, $t2
    paddh   $t2, $at, $v1
    pmulth  $zero, $t2, $t6
    paddh   $t2, $a3, $t4
    ld      $t3, 32($a0)
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t8
    paddh   $t2, $a2, $t1
    ppacb   $t0, $zero, $t0
    sd      $t0, 32($a0)
    paddh   $t0, $t3, $t7
    pmfhl.lh    $t3
    pmulth  $zero, $t2, $t9
    ld      $t2, 112($v0)
    psubh   $t0, $t0, $t3
    pextlb  $t2, $zero, $t2
    paddh   $t3, $a1, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $a1, $v1, $t4
    psrah   $t0, $t0, 5
    pmulth  $zero, $a1, $t6
    ld      $t3, 48($a0)
    paddh   $a1, $at, $t1
    pminh   $t0, $t5, $t0
    pextlb  $t3, $zero, $t3
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t8
    sd      $t0, 48($a0)
    paddh   $t0, $t3, $t7
    paddh   $a1, $a3, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $a1, $t9
    ld      $a1, 128($v0)
    psubh   $t0, $t0, $t3
    pextlb  $a1, $zero, $a1
    paddh   $t3, $a2, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t4, $t1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $t3, 64($a0)
    paddh   $a2, $v1, $t2
    pminh   $t0, $t5, $t0
    pextlb  $t3, $zero, $t3
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t8
    sd      $t0, 64($a0)
    paddh   $t0, $t3, $t7
    paddh   $a2, $at, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a2, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $a3, $a1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t1, $t2
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $t3, 80($a0)
    paddh   $a3, $t4, $a1
    pminh   $t0, $t5, $t0
    pextlb  $t3, $zero, $t3
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t8
    sd      $t0, 80($a0)
    paddh   $t0, $t3, $t7
    paddh   $a3, $v1, $a1
    pmfhl.lh    $t3
    pmulth  $zero, $a3, $t9
    psubh   $t0, $t0, $t3
    paddh   $t3, $at, $t2
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    paddh   $t0, $t0, $t3
    paddh   $t3, $t2, $a1
    psrah   $t0, $t0, 5
    pmulth  $zero, $t3, $t6
    ld      $t3, 96($a0)
    paddh   $at, $t1, $a1
    pminh   $t0, $t5, $t0
    pextlb  $t3, $zero, $t3
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $t3
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t8
    sd      $t0, 96($a0)
    paddh   $t0, $t3, $t7
    paddh   $at, $t4, $t2
    pmfhl.lh    $t3
    pmulth  $zero, $at, $t9
    ld      $at, 112($a0)
    psubh   $t0, $t0, $t3
    paddh   $t3, $v1, $t1
    psubh   $t0, $t0, $t3
    pmfhl.lh    $t3
    pextlb  $at, $zero, $at
    paddh   $t0, $t0, $t3
    psrah   $t0, $t0, 5
    pminh   $t0, $t5, $t0
    pmaxh   $t0, $zero, $t0
    paddh   $t0, $t0, $at
    paddh   $t0, $t0, $s3
    psrlh   $t0, $t0, 1
    ppacb   $t0, $zero, $t0
    jr      $ra
    sd      $t0, 112($a0)

_put_no_rnd_qpel8_16:
    addiu   $v1, $zero, 4
1:
    ld      $t0,  0($a1)
    ld      $t1, 16($a1)
    ld      $t2,  0($a2)
    ld      $t3, 16($a2)
    addiu   $v1, $v1, -1
    addiu   $a1, $a1, 32
    addiu   $a2, $a2, 32
    pextlb  $t0, $zero, $t0
    pextlb  $t1, $zero, $t1
    pextlb  $t2, $zero, $t2
    pextlb  $t3, $zero, $t3
    paddh   $t0, $t0, $t2
    paddh   $t1, $t1, $t3
    psrlh   $t0, $t0, 1
    psrlh   $t1, $t1, 1
    ppacb   $t0, $zero, $t0
    ppacb   $t1, $zero, $t1
    sd      $t0,  0($a0)
    sd      $t1, 16($a0)
    bgtz    $v1, 1b
    addiu   $a0, $a0, 32
    beq     $s1, $zero, 2f
    nop
    ld      $t0, 0($a1)
    ld      $t1, 0($a2)
    pextlb  $t0, $zero, $t0
    pextlb  $t1, $zero, $t1
    paddh   $t0, $t0, $t1
    psrlh   $t0, $t0, 1
    ppacb   $t0, $t2, $t0
    sd      $t0, 0($a0)
2:
    jr      $ra
    nop

DSP_PutQPel16MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    sw      $s1, 8($sp)
    lui     $v1, 0x7000
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($v1)
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutQPel16MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($s2)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC01:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a2, $v0, -16
    addiu   $a1, $a0, -16
    jal     _put_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutQPel16MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC21:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero,  1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lui     $a0,  0x7000
    ori     $a0,  $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel16MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero,  1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    lq      $t6, 0x220($a0)
    lq      $t7, 0x230($a0)
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero,  $zero
    addiu   $a1, $a0, -16
    addiu   $a2, $v0, -16
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC02:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     _copy16
    nop
    jal     _put_qpel16_v_lowpass
    nop
    lw      $ra, 0($sp)
    j       _put_qpel16_v_lowpass
    addiu   $sp, $sp, 4

DSP_PutQPel16MC12:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    ori     $v0, $v0, 0x0110
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC22:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel16MC32:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC03:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal      _copy16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $v0
    addiu   $a1, $a0, -16
    jal     _put_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutQPel16MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addu    $a1, $zero, $v0
    addiu   $a2, $a0, -16
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel16MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lui     $a0,  0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    ori     $a0,  $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addu    $a1, $zero, $v0
    addiu   $a2, $a0, -16
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel16MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero,  1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero,  1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    lq      $t6, 0x220($a0)
    lq      $t7, 0x230($a0)
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a1, $a0, -16
    addu    $a2, $zero, $v0
    jal     _put_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    sw      $s1, 8($sp)
    addiu   $s1, $zero, 0
    lui     $v1, 0x7000
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x240($v1)
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutNoRndQPel16MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero,  0
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x240($s2)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC01:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    psubh   $t7, $zero, $t7
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    psrlh   $t7, $t7, 12
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a2, $v0, -16
    addiu   $a1, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutNoRndQPel16MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x240($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC21:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -272
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    ori     $a0,  $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel16MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x240($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    lq      $t6, 0x220($a0)
    lq      $t7, 0x240($a0)
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero,  $zero
    addiu   $a1, $a0, -16
    addiu   $a2, $v0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC02:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     _copy16
    nop
    psubh   $t7, $zero, $t7
    jal     _put_qpel16_v_lowpass
    psrlh   $t7, $t7, 12
    lw      $ra, 0($sp)
    j       _put_qpel16_v_lowpass
    addiu   $sp, $sp, 4

DSP_PutNoRndQPel16MC12:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x240($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x240($v0)
    lq      $t5, 0x260($v0)
    ori     $v0, $v0, 0x0110
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC22:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -272
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel16MC32:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x240($v0)
    lq      $t5, 0x260($v0)
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC03:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    psubh   $t7, $zero, $t7
    ori     $a0,  $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    psrlh   $t7, $t7, 12
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $v0
    addiu   $a1, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutNoRndQPel16MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x240($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addu    $a1, $zero, $v0
    addiu   $a2, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel16MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -272
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addu    $a1, $zero, $v0
    addiu   $a2, $a0, -16
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel16MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero,  1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x240($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_no_rnd_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    lq      $t6, 0x220($a0)
    lq      $t7, 0x240($a0)
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    addu    $s1, $zero, $zero
    addiu   $a1, $a0, -16
    addu    $a2, $zero, $v0
    jal     _put_no_rnd_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    sw      $s1, 8($sp)
    lui     $v1, 0x7000
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x230($v1)
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutQPel816MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC01:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    jal     _copy8_16
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addu    $a1, $zero, $a0
    addu    $a2, $zero, $v0
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s0, 8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel816MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    psrlh   $t9, $t8, 1
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC21:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    addiu   $a0, $s2, -144
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC02:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     _copy8_16
    nop
    lw      $ra, 0($sp)
    j       _put_qpel8_16_v_lowpass
    addiu   $sp, $sp, 4

DSP_PutQPel816MC12:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    lq      $t5, 0x260($v0)
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC22:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel816MC32:
    addiu   $sp, $sp, -24
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    sw      $s3, 20($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    paddh   $s3, $zero, $t9
    lq      $t5, 0x260($v0)
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s0, 8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    lw      $s3, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

DSP_PutQPel816MC03:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy8_16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addu    $a1, $zero, $a0
    addiu   $a2, $v0, 16
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutQPel816MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    psrlh   $t9, $t8, 1
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addiu   $a1, $v0, 16
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutQPel816MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutQPel816MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addu    $a1, $zero, $a0
    jal     _put_qpel8_16
    addiu   $a2, $s2, -144
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    psrlh   $t9, $t8, 1
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    addu    $s1, $zero, $zero
    jal     _put_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    addu    $s1, $zero, $zero
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    sw      $s1, 8($sp)
    lui     $v1, 0x7000
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x240($v1)
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutNoRndQPel816MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x240($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    addu    $s1, $zero, $zero
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC01:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    jal     _copy8_16
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    ori     $a0, $a0, 0x0110
    psubh   $t7, $zero, $t7
    jal     _put_qpel8_16_v_lowpass
    psrlh   $t7, $t7, 12
    addu    $a1, $zero, $a0
    addu    $a2, $zero, $v0
    addu    $s1, $zero, $zero
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel816MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_no_rnd_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC21:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -128
    addiu   $a0, $s2, -144
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_no_rnd_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC02:
    addiu   $sp, $sp, -4
    sw      $ra, 0($sp)
    jal     _copy8_16
    nop
    psubh   $t7, $zero, $t7
    psrlh   $t7, $t7, 12
    lw      $ra, 0($sp)
    j       _put_qpel8_16_v_lowpass
    addiu   $sp, $sp, 4

DSP_PutNoRndQPel816MC12:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_no_rnd_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    psrlh   $t9, $t8, 1
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC22:
    addiu   $sp, $sp,  -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -128
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel816MC32:
    addiu   $sp, $sp, -24
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    sw      $s3, 20($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_no_rnd_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    paddh   $s3, $zero, $t9
    psrlh   $t9, $t8, 1
    lq      $t5, 0x260($v0)
    jal     _put_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    lw      $s3, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

DSP_PutNoRndQPel816MC03:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy8_16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    ori     $a0, $a0, 0x0110
    psubh   $t7, $zero, $t7
    jal     _put_qpel8_16_v_lowpass
    psrlh   $t7, $t7, 12
    addu    $a1, $zero, $a0
    addiu   $a2, $v0, 16
    addu    $s1, $zero, $zero
    jal     _put_no_rnd_qpel8_16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_PutNoRndQPel816MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_no_rnd_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addiu   $a1, $v0, 16
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_PutNoRndQPel816MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x240($a0)
    addiu   $v0, $a0, -128
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_PutNoRndQPel816MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x240($a0)
    addiu   $a0, $a0, -128
    addu    $a1, $zero, $a0
    jal     _put_no_rnd_qpel8_16
    addiu   $a2, $s2, -144
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    addu    $s1, $zero, $zero
    jal     _put_no_rnd_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    sw      $s1, 8($sp)
    jal     _avg_qpel16_h_lowpass
    addiu   $s1, $zero, 0
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_AvgQPel16MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s1, $zero, $zero
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -256
    addiu   $a2, $s2, -256
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC01:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy16
    sw      $s1, 8($sp)
    lui     $a0, 0x7000
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s1, 8($sp)
    addiu   $a2, $v0, -16
    addiu   $a1, $a0, -16
    jal     _avg_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_AvgQPel16MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC21:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero,  1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lui     $a0, 0x7000
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $v0, -16
    addiu   $a2, $a0, -16
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_AvgQPel16MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    lq      $t6, 0x220($a0)
    lq      $t7, 0x230($a0)
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $a0, -16
    addiu   $a2, $v0, -16
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC02:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    jal     _copy16
    sw      $s1, 8($sp)
    jal     _avg_qpel16_v_lowpass
    psrlh   $s0, $t5, 7
    jal     _avg_qpel16_v_lowpass
    nop
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_AvgQPel16MC12:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    ori     $v0, $v0, 0x0110
    psrlh   $t9, $t8, 1
    jal     _avg_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _avg_qpel16_v_lowpass
    nop
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC22:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lq      $t5, 0x260($v0)
    jal     _avg_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _avg_qpel16_v_lowpass
    nop
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_AvgQPel16MC32:
    addiu   $sp, $sp, -24
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    sw      $s3, 20($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    lui     $v0, 0x7000
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    psrlh   $t9, $t8, 1
    por     $s3, $zero, $s0
    jal     _avg_qpel16_v_lowpass
    lw      $a0, 4($sp)
    jal     _avg_qpel16_v_lowpass
    nop
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    lw      $s3, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

DSP_AvgQPel16MC03:
    addiu   $sp, $sp, -8
    sw      $ra, 0($sp)
    jal     _copy16
    sw      $a0, 4($sp)
    lui     $a0, 0x7000
    jal     _put_qpel16_v_lowpass
    ori     $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    nop
    addu    $a2, $zero, $v0
    addiu   $a1, $a0, -16
    jal     _avg_qpel16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    jr      $ra
    addiu   $sp, $sp, 8

DSP_AvgQPel16MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $a2, -256
    lq      $t6, 0x220($v0)
    lq      $t7, 0x230($v0)
    lq      $t5, 0x260($v0)
    jal     _put_qpel16_v_lowpass
    psrlh   $t9, $t8, 1
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addu    $a1, $zero, $v0
    addiu   $a2, $a0, -16
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel16MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s1, $zero, 1
    jal     _put_qpel16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -272
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    ori     $a0, $a0, 0x0110
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $a0, -16
    addu    $a2, $zero, $v0
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_AvgQPel16MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $a0, 0x7000
    lui     $s2, 0x7000
    ori     $a0, $a0, 0x0110
    addiu   $s1, $zero,  1
    jal     _put_qpel16_h_lowpass_copy_x
    lq      $t7, 0x230($s2)
    addiu   $a0, $a0, -272
    addu    $a1, $zero, $a0
    addiu   $a2, $s2, -272
    jal     _put_qpel16
    addiu   $s1, $zero, 1
    addiu   $v0, $a0, -256
    addiu   $a0, $s2, -272
    psrlh   $t9, $t8, 1
    lq      $t6, 0x220($a0)
    lq      $t7, 0x230($a0)
    jal     _put_qpel16_v_lowpass
    lq      $t5, 0x260($a0)
    jal     _put_qpel16_v_lowpass
    lw      $s0,  8($sp)
    addiu   $a1, $a0, -16
    addu    $a2, $zero, $v0
    jal     _avg_qpel16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC10:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC20:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $s0, 4($sp)
    jal     _avg_qpel8_16_h_lowpass
    sw      $s1, 8($sp)
    lw      $ra, 0($sp)
    lw      $s0, 4($sp)
    lw      $s1, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_AvgQPel816MC30:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addu    $s0, $zero, $zero
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a1, $a0, -112
    addiu   $a2, $s2, -128
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC01:
    addiu   $sp, $sp, -12
    sw      $ra, 0($sp)
    sw      $a0, 4($sp)
    jal     _copy8_16
    sw      $s0, 8($sp)
    lui     $a0, 0x7000
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addu    $a1, $zero, $a0
    addu    $a2, $zero, $v0
    jal     _avg_qpel8_16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    lw      $s0, 8($sp)
    jr      $ra
    addiu   $sp, $sp, 12

DSP_AvgQPel816MC11:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC21:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    addiu   $a0, $s2, -144
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC31:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addu    $a1, $zero, $v0
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC02:
    addiu   $sp, $sp, -8
    sw      $ra, 0($sp)
    jal     _copy8_16
    sw      $s3, 4($sp)
    pnor    $s3, $zero, $zero
    jal     _avg_qpel8_16_v_lowpass
    psrlh   $s3, $s3, 15
    lw      $ra, 0($sp)
    lw      $s3, 4($sp)
    jr      $ra
    addiu   $sp, $sp, 8

DSP_AvgQPel816MC12:
    addiu   $sp, $sp, -24
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    sw      $s3, 20($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    paddh   $s3, $zero, $t9
    psrlh   $t9, $t8, 1
    lq      $t5, 0x260($v0)
    jal     _avg_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    lw      $s3, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

DSP_AvgQPel816MC22:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s3, 16($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    psrlh   $s3, $t7, 4
    lq      $t5, 0x260($v0)
    jal     _avg_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s3, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC32:
    addiu   $sp, $sp, -24
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    sw      $s3, 20($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    paddh   $s3, $zero, $t9
    psrlh   $t9, $t8, 1
    lq      $t5, 0x260($v0)
    jal     _avg_qpel8_16_v_lowpass
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    lw      $s3, 20($sp)
    jr      $ra
    addiu   $sp, $sp, 24

DSP_AvgQPel816MC03:
    addiu   $sp, $sp, -8
    sw      $ra, 0($sp)
    jal     _copy8_16
    sw      $a0, 4($sp)
    lui     $a0, 0x7000
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addu    $a1, $zero, $a0
    addiu   $a2, $v0, 16
    jal     _avg_qpel8_16
    lw      $a0, 4($sp)
    lw      $ra, 0($sp)
    jr      $ra
    addiu   $sp, $sp, 8

DSP_AvgQPel816MC13:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addiu   $a2, $s2, -144
    jal     _put_qpel8_16
    addu    $a1, $zero, $a0
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addu    $s1, $zero, $zero
    addu    $a2, $zero, $a0
    addiu   $a1, $v0, 16
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20

DSP_AvgQPel816MC23:
    addiu   $sp, $sp, -16
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    lui     $a0, 0x7000
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass
    lq      $t7, 0x230($a0)
    addiu   $v0, $a0, -128
    lui     $a0, 0x7000
    lq      $t5, 0x260($a0)
    jal     _put_qpel8_16_v_lowpass
    ori     $a0, $a0, 0x0110
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    jr      $ra
    addiu   $sp, $sp, 16

DSP_AvgQPel816MC33:
    addiu   $sp, $sp, -20
    sw      $ra,  0($sp)
    sw      $a0,  4($sp)
    sw      $s0,  8($sp)
    sw      $s1, 12($sp)
    sw      $s2, 16($sp)
    lui     $s2, 0x7000
    lui     $a0, 0x7000
    ori     $s2, $s2, 0x0110
    addiu   $s0, $zero, 1
    jal     _put_qpel8_16_h_lowpass_copy_x
    lq      $t7, 0x230($a0)
    addiu   $a0, $a0, -128
    addu    $a1, $zero, $a0
    jal     _put_qpel8_16
    addiu   $a2, $s2, -144
    addiu   $v0, $a0, -128
    addiu   $a0, $a2, -128
    psrlh   $t9, $t8, 1
    jal     _put_qpel8_16_v_lowpass
    lq      $t5, 0x260($v0)
    addiu   $a1, $v0, 16
    addu    $a2, $zero, $a0
    jal     _avg_qpel8_16
    lw      $a0,  4($sp)
    lw      $ra,  0($sp)
    lw      $s0,  8($sp)
    lw      $s1, 12($sp)
    lw      $s2, 16($sp)
    jr      $ra
    addiu   $sp, $sp, 20
