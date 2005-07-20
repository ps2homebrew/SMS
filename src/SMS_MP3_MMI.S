/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Copyright (c) ????? (BroadQ maybe)
# I think this code is GPL'ed (thanks JF for pointer).
# Adopted for SMS in 2005 by Eugene Plotnikov.
#
*/
.set noreorder
.set noat
.text
.align 4

#define FRAC_BITS  15
#define WFRAC_BITS 14
#define OUT_SHIFT  ( WFRAC_BITS + FRAC_BITS - 15 )
#define COS3_0     17734
#define COS3_1     42813
#define COS4_0     23170

COS0_n:
.word  16404,  16563,  16890,  17401
.word  18124,  19102,  20398,  22112
.word  24397,  27504,  31869,  38320
.word  48633,  67429, 111661, 333906

COS1_n:
.word  16463,  17121,  18578,  21195
.word  25826,  34756,  56441, 167154
.word -16463, -17121, -18578, -21195
.word -25826, -34756, -56441,-167154

COS2_n:
.word  16705,  19705,  29490,  83982
.word -16705, -19705, -29490, -83982

COS3_n:
.word  17734, 0,  42813, 0
.word -17734, 0, -42813, 0

COS4_n:
.word 23170, 0, -23170, 0

.global ps2_dct32
.global ps2_synth_sum8a
.global ps2_synth_sum8b

.ent ps2_dct32

.MACRO BF a,b,coef
    lq      $t0, \a*4($a1)
    lq      $t1, \b*4($a1)
    mtsah   $zero, 6
    pexew   $t1, $t1
    qfsrv   $t1, $t1, $t1
    psubw   $t2, $t0, $t1
    lq      $t3, \coef*4($a3)
    pextuw  $t4, $zero, $t2
    pextlw  $t2, $zero, $t2
    pextuw  $t5, $zero, $t3
    pextlw  $t3, $zero, $t3
    pmultw  $t2, $t2, $t3
    pmultw  $t4, $t4, $t5
    paddw   $t0, $t0, $t1
    sq      $t0, \a*4($a1)
    psraw   $t2, $t2, FRAC_BITS
    psraw   $t4, $t4, FRAC_BITS
    ppacw   $t2, $zero, $t2
    ppacw   $t4, $t4, $zero
    por     $t2, $t2, $t4
    pexew   $t2, $t2
    qfsrv   $t2, $t2, $t2
    sq      $t2, \b*4($a1)
.ENDM

.MACRO hBF a, coef
    lq      $t0,\a*4($a1)
    pextuw  $t1, $zero, $t0
    pextlw  $t0, $zero, $t0
    pexew   $t1, $t1
    psubw   $t2, $t0, $t1
    lq      $t3, \coef*4($a3)
    pmultw  $t2, $t2, $t3
    paddw   $t0, $t0, $t1
    psraw   $t2, $t2, FRAC_BITS
    pexew   $t2, $t2
    ppacw   $t0, $zero, $t0
    ppacw   $t2, $t2, $zero
    por     $t0, $t0, $t2
    sq      $t0, \a*4($a1)
.ENDM

.MACRO BF1a a
    lq      $t0, \a*4($a1)
    pexcw   $t0, $t0
    pextuw  $t1, $zero, $t0
    pextlw  $t0, $zero, $t0
    psubw   $t2, $t0, $t1
    lq      $t3, ($a3)
    pmultw  $t2, $t2, $t3
    paddw   $t0, $t0, $t1
    psraw   $t2, $t2, FRAC_BITS
    ppacw   $t0, $zero, $t0
    ppacw   $t2, $t2, $zero
    por     $t0, $t0, $t2
    pexcw   $t0, $t0
    sq      $t0, \a*4($a1)
.ENDM

.MACRO BF1 a
    BF1a    \a
    ld      $t0, \a*4+8($a1)
    dsra32  $t1, $t0, 0
    add     $t0, $t1, $t0
    sw      $t0, \a*4+8($a1)
.ENDM

.MACRO BF2 a
    BF1a    \a
    ld      $t2, \a*4+2*4($a1)
    ld      $t0, \a*4($a1)
    dsra32  $t3, $t2, 0
    add     $t2, $t3, $t2
    dsra32  $t1, $t0, 0
    add     $t0, $t0, $t2
    sw      $t0, \a*4($a1)
    add     $t2, $t1, $t2
    sw      $t2, \a*4+2*4($a1)
    add     $t3, $t1, $t3
    sw      $t3, \a*4+1*4($a1)
.ENDM

ps2_dct32:
    la  $a3, COS0_n
    BF  0, 28,  0
    BF  4, 24,  4
    BF  8, 20,  8
    BF 12, 16, 12

    la  $a3, COS1_n
    BF  0, 12,  0
    BF  4,  8,  4
    BF 16, 28,  8
    BF 20, 24, 12

    la  $a3, COS2_n
    BF  0,  4,  0
    BF  8, 12,  4
    BF 16, 20,  0
    BF 24, 28,  4

    la  $a3, COS3_n
    hBF  0, 0
    hBF  4, 4
    hBF  8, 0
    hBF 12, 4
    hBF 16, 0
    hBF 20, 4
    hBF 24, 0
    hBF 28, 4

    la  $a3, COS4_n
    BF1 0
    BF2 4
    BF1 8
    BF2 12
    BF1 16
    BF2 20
    BF1 24
    BF2 28

    jr  $ra
    nop

.end ps2_dct32

.MACRO sum1 a,j
    lq      $t0, (\a*64 + 0 +  \j)*2($a0)
    lq      $t1, (16 + \j + \a*64)*2($a1)
    pmaddh  $t0, $t1
.ENDM

.MACRO sum2 a,j
    lq      $t0, (\a*64 + 32 +  \j)*2($a0)
    lq      $t1, (48 - 7 - \j + \a*64)*2($a1)
    lq      $t2, (48 + 1 - \j + \a*64)*2($a1)
    mtsah   $zero, 1
    qfsrv   $t1, $t2, $t1
    prevh   $t1, $t1
    pcpyud  $t2, $t1, $t1
    pcpyld  $t1, $t1, $t2
    pmsubh  $t0, $t1
.ENDM

.MACRO out8 ofs
    pmfhl.uw    $t0
    pmfhl.lw    $t1
    paddw       $t0, $t0, $t7
    psraw       $t0, $t0, OUT_SHIFT
    paddw       $t1, $t1, $t7
    psraw       $t1, $t1, OUT_SHIFT
    pminw       $t0, $t9, $t0
    pmaxw       $t0, $t8, $t0
    pminw       $t1, $t9, $t1
    pmaxw       $t1, $t8, $t1
    pinteh      $t0, $t0, $t1
    sq          $t0, \ofs*2($a2)
.ENDM

.ent ps2_synth_sum8a

ps2_synth_sum8a:
    or      $t9, $zero,0x7FFF
    pcpyh   $t9, $t9
    pextlh  $t9, $zero, $t9
    addi    $t8, $zero, 0x8000
    pextlw  $t8, $t8, $t8
    pextlw  $t8, $t8, $t8
    or      $t7, $zero, 1
    pcpyh   $t7, $t7
    pextlh  $t7, $zero, $t7
    psllw   $t7, $t7,OUT_SHIFT - 1
    pmthi   $zero
    pmtlo   $zero
    sum1    0, 0
    sum1    1, 0
    sum1    2, 0
    sum1    3, 0
    sum1    4, 0
    sum1    5, 0
    sum1    6, 0
    sum1    7, 0
    sum2    0, 0
    sum2    1, 0
    sum2    2, 0
    sum2    3, 0
    sum2    4, 0
    sum2    5, 0
    sum2    6, 0
    sum2    7, 0
    out8    0
    pmthi   $zero
    pmtlo   $zero
    sum1    0, 8
    sum1    1, 8
    sum1    2, 8
    sum1    3, 8
    sum1    4, 8
    sum1    5, 8
    sum1    6, 8
    sum1    7, 8
    sum2    0, 8
    sum2    1, 8
    sum2    2, 8
    sum2    3, 8
    sum2    4, 8
    sum2    5, 8
    sum2    6, 8
    sum2    7, 8
    out8    8
    jr      $ra
    nop
.end ps2_synth_sum8a

.MACRO sum1b a, j
    lq      $t0, (\a*64 + 0 + \j  )*2($a0)
    lq      $t1, (\a*64 + 0 + \j+8)*2($a0)
    qfsrv   $t0, $t1, $t0
    lq      $t1, (48 - 7 - (\j) + \a*64)*2($a1)
    prevh   $t1, $t1
    pcpyud  $t2, $t1, $t1
    pcpyld  $t1, $t1, $t2
    pmsubh  $t0, $t1
.ENDM

.MACRO sum2b a,j
    lq      $t0, (\a*64 + 32 + \j  )*2($a0)
    lq      $t1, (\a*64 + 32 + \j+8)*2($a0)
    qfsrv   $t0, $t1, $t0
    lq      $t1, (16 +   + \j + \a*64)*2($a1)
    lq      $t2, (16 + 8 + \j + \a*64)*2($a1)
    qfsrv   $t1, $t2, $t1
    pmsubh  $t0, $t1
.ENDM

.ent ps2_synth_sum8b

ps2_synth_sum8b:
    mtsah   $zero, 1
    or      $t9, $zero, 0x7FFF
    pcpyh   $t9, $t9
    pextlh  $t9, $zero, $t9
    addi    $t8, $zero, 0x8000
    pextlw  $t8, $t8, $t8
    pextlw  $t8, $t8, $t8
    or      $t7, $zero, 1
    pcpyh   $t7, $t7
    pextlh  $t7, $zero, $t7
    psllw   $t7, $t7, OUT_SHIFT - 1
    pmthi   $zero
    pmtlo   $zero
    sum1b   0, 17
    sum1b   1, 17
    sum1b   2, 17
    sum1b   3, 17
    sum1b   4, 17
    sum1b   5, 17
    sum1b   6, 17
    sum1b   7, 17
    sum2b   0, 17
    sum2b   1, 17
    sum2b   2, 17
    sum2b   3, 17
    sum2b   4, 17
    sum2b   5, 17
    sum2b   6, 17
    sum2b   7, 17
    out8    0
    pmthi   $zero
    pmtlo   $zero
    sum1b   0, 25
    sum1b   1, 25
    sum1b   2, 25
    sum1b   3, 25
    sum1b   4, 25
    sum1b   5, 25
    sum1b   6, 25
    sum1b   7, 25
    sum2b   0, 25
    sum2b   1, 25
    sum2b   2, 25
    sum2b   3, 25
    sum2b   4, 25
    sum2b   5, 25
    sum2b   6, 25
    sum2b   7, 25
    out8    8
    jr      $ra
    nop
.end ps2_synth_sum8b
