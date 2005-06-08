#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# Originally provided by Intel at AP-922
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
# Pixel routines by Eugene Plotnikov
# These are licensed under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.


#include "MIPS.h"

.data

.align 4

s_ConstTbl:
.half   0x03FF, 0x0001, 0x03FF, 0x0001, 0x03FF, 0x0001, 0x03FF, 0x0001
.half   0x03FF, 0x0000, 0x03FF, 0x0000, 0x03FF, 0x0000, 0x03FF, 0x0000
.half    16384,  21407, -16384, -21407,  22725,  19266, -22725, -12873
.half     8867,  16384,   8867,  16384,   4520,  12873,  -4520,  19266
.half    16384,  -8867,  16384,  -8867,  12873, -22725,  19266, -22725
.half    21407, -16384, -21407,  16384,  19266,   4520, -12873,   4520
.half    22725,  29692, -22725, -29692,  31521,  26722, -31521, -17855
.half    12299,  22725,  12299,  22725,   6270,  17855,  -6270,  26722
.half    22725, -12299,  22725, -12299,  17855, -31521,  26722, -31521
.half    29692, -22725, -29692,  22725,  26722,   6270, -17855,   6270
.half    21407,  27969, -21407, -27969,  29692,  25172, -29692, -16819
.half    11585,  21407,  11585,  21407,   5906,  16819,  -5906,  25172
.half    21407, -11585,  21407, -11585,  16819, -29692,  25172, -29692
.half    27969, -21407, -27969,  21407,  25172,   5906, -16819,   5906
.half    19266,  25172, -19266, -25172,  26722,  22654, -26722, -15137
.half    10426,  19266,  10426,  19266,   5315,  15137,  -5315,  22654
.half    19266, -10426,  19266, -10426,  15137, -26722,  22654, -26722
.half    25172, -19266, -25172,  19266,  22654,   5315, -15137,   5315
.half     6518,   6518,   6518,   6518,   6518,   6518,   6518,   6518
.half    13573,  13573,  13573,  13573,  13573,  13573,  13573,  13573
.half    21895,  21895,  21895,  21895,  21895,  21895,  21895,  21895
.half    23170,  23170,  23170,  23170,  23170,  23170,  23170,  23170
.half      255,    255,    255,    255,    255,    255,    255,    255

.size   s_ConstTbl, 368

.text

.align 3

.set noat
.set volatile
.set noreorder
.set nomacro

.globl	IDCT_Put
.ent	IDCT_Put

IDCT_Put:
    .set reorder
    .set novolatile
    lui     $t8, %hi(s_ConstTbl)
    subu    $sp, $sp, 24
    sw      $s0,  0($sp)
    sw      $s1,  4($sp)
    addiu   $t8, $t8, %lo(s_ConstTbl)
    sw      $s2,  8($sp)
    sw      $s3, 12($sp)
    sw      $s4, 16($sp)
    sw      $s5, 20($sp)
    lq      $t0,  0($t8)
    lq      $a3, 16($t8)
# DCT_8_INV_ROW1
    lq      $s0,  0($a2)
    lq      $s1, 32($t8)
    lq      $s2, 48($t8)
    prevh   $v0, $s0
    lq      $s3, 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 80($t8)
    phmadh  $s2, $s2, $v0
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
    prevh   $v0, $t0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t0, $v0, $t0
# DCT_8_INV_ROW1
    lq      $s0,  16($a2)
    lq      $s1,  96($t8)
    lq      $s2, 112($t8)
    prevh   $v0, $s0
    lq      $s3, 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 144($t8)
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
    ppach   $t1, $s4, $s1
    prevh   $v0, $t1
    pcpyud  $v0, $v0, $v0
    pcpyld  $t1, $v0, $t1
# DCT_8_INV_ROW1
    lq      $s0,  32($a2)
    lq      $s1, 160($t8)
    lq      $s2, 176($t8)
    prevh   $v0, $s0
    lq      $s3, 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 208($t8)
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
    ppach   $t2, $s4, $s1
    prevh   $v0, $t2
    pcpyud  $v0, $v0, $v0
    pcpyld  $t2, $v0, $t2
# DCT_8_INV_ROW1
    lq      $s0,  48($a2)
    lq      $s1, 224($t8)
    lq      $s2, 240($t8)
    prevh   $v0, $s0
    lq      $s3, 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 272($t8)
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
    ppach   $t3, $s4, $s1
    prevh   $v0, $t3
    pcpyud  $v0, $v0, $v0
    pcpyld  $t3, $v0, $t3
# DCT_8_INV_ROW1
    lq      $s0, 64($a2)
    lq      $s1, 32($t8)
    lq      $s2, 48($t8)
    prevh   $v0, $s0
    lq      $s3, 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 80($t8)
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
    ppach   $t4, $s4, $s1
    prevh   $v0, $t4
    pcpyud  $v0, $v0, $v0
    pcpyld  $t4, $v0, $t4
# DCT_8_INV_ROW1
    lq      $s0,  80($a2)
    lq      $s1, 224($t8)
    lq      $s2, 240($t8)
    prevh   $v0, $s0
    lq      $s3, 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 272($t8)
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
    ppach   $t5, $s4, $s1
    prevh   $v0, $t5
    pcpyud  $v0, $v0, $v0
    pcpyld  $t5, $v0, $t5
# DCT_8_INV_ROW1
    lq      $s0,  96($a2)
    lq      $s1, 160($t8)
    lq      $s2, 176($t8)
    prevh   $v0, $s0
    lq      $s3, 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 208($t8)
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
    ppach   $t6, $s4, $s1
    prevh   $v0, $t6
    pcpyud  $v0, $v0, $v0
    pcpyld  $t6, $v0, $t6
# DCT_8_INV_ROW1
    lq      $s0, 112($a2)
    lq      $s1,  96($t8)
    lq      $s2, 112($t8)
    prevh   $v0, $s0
    lq      $s3, 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 144($t8)
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
    ppach   $t7, $s4, $s1
    prevh   $v0, $t7
    pcpyud  $v0, $v0, $v0
    pcpyld  $t7, $v0, $t7
# DCT_8_INV_COL8
    lq          $v0, 320($t8)
    pmulth      $s1, $t3, $v0
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t5
    pmulth      $s2, $t5, $v0
    psraw       $s2, $s2, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s2, $v1, $s2
    paddh       $s2, $s2, $t3
    lq          $v0, 288($t8)
    pmulth      $s3, $t7, $v0
    psraw       $s3, $s3, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s3, $v1, $s3
    paddh       $s3, $s3, $t1
    pmulth      $s4, $t1, $v0
    psraw       $s4, $s4, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s4, $v1, $s4
    psubh       $s4, $s4, $t7
    psubh       $v1, $s3, $s2
    paddh       $s0, $s4, $s1
    psubh       $at, $s4, $s1
    paddh       $s4, $s3, $s2
    lq          $v0, 336($t8)
    paddh       $s5, $v1, $s0
    psubh       $t9, $v1, $s0
    pmulth      $s5, $s5, $v0
    psraw       $s5, $s5, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s5, $v1, $s5
    pmulth      $t9, $t9, $v0
    psraw       $t9, $t9, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $t9, $v1, $t9
    lq          $v0, 304($t8)
    pmulth      $s1, $t2, $v0
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t6
    pmulth      $s2, $t6, $v0
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

    lq          $t3, 352($t8)
# DCT_8_INV_COL8_PMS
    paddh   $v0, $s0, $s4
    psubh   $s4, $s0, $s4
    psrah   $s0, $v0, 6
    psrah   $s4, $s4, 6
    paddh   $v1, $s1, $s5
    psubh   $s5, $s1, $s5
    psrah   $s1, $v1, 6
    psrah   $s5, $s5, 6
    paddh   $v0, $s2, $t9
    psubh   $t9, $s2, $t9
    psrah   $s2, $v0, 6
    psrah   $t9, $t9, 6
    paddh   $v1, $s3, $at
    psubh   $at, $s3, $at
    psrah   $s3, $v1, 6
    psrah   $at, $at, 6
# DCT_8_INV_COL8_PUT
    pminh   $v0, $s0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $s1, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $s2, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $s3, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $at, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $t9, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $s5, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    pminh   $v0, $s4, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    lw      $s0,  0($sp)
    lw      $s1,  4($sp)
    lw      $s2,  8($sp)
    lw      $s3, 12($sp)
    lw      $s4, 16($sp)
    lw      $s5, 20($sp)
    .set noreorder	 
    .set volatile
    jr      $ra
    addu    $sp, $sp, 24
.end IDCT_Put

.align 3

.globl IDCT_Add
.ent   IDCT_Add

IDCT_Add:
    .set reorder
    .set novolatile
    lui     $t8, %hi(s_ConstTbl)
    subu    $sp, $sp, 24
    sw      $s0,  0($sp)
    sw      $s1,  4($sp)
    sw      $s2,  8($sp)
    addiu   $t8, $t8, %lo(s_ConstTbl)
    sw      $s3, 12($sp)
    sw      $s4, 16($sp)
    sw      $s5, 20($sp)
# DCT_8_INV_ROW1
    lq      $t0,  0($t8)
    lq      $a3, 16($t8)
    lq      $s0,  0($a2)
    lq      $s1, 32($t8)
    lq      $s2, 48($t8)
    prevh   $v0, $s0
    lq      $s3, 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 80($t8)
    phmadh  $s2, $s2, $v0
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
    prevh   $v0, $t0
    pcpyud  $v0, $v0, $v0
    pcpyld  $t0, $v0, $t0
# DCT_8_INV_ROW1
    lq      $s0,  16($a2)
    lq      $s1,  96($t8)
    lq      $s2, 112($t8)
    prevh   $v0, $s0
    lq      $s3, 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 144($t8)
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
    ppach   $t1, $s4, $s1
    prevh   $v0, $t1
    pcpyud  $v0, $v0, $v0
    pcpyld  $t1, $v0, $t1
# DCT_8_INV_ROW1
    lq      $s0,  32($a2)
    lq      $s1, 160($t8)
    lq      $s2, 176($t8)
    prevh   $v0, $s0
    lq      $s3, 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 208($t8)
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
    ppach   $t2, $s4, $s1
    prevh   $v0, $t2
    pcpyud  $v0, $v0, $v0
    pcpyld  $t2, $v0, $t2
# DCT_8_INV_ROW1
    lq      $s0,  48($a2)
    lq      $s1, 224($t8)
    lq      $s2, 240($t8)
    prevh   $v0, $s0
    lq      $s3, 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 272($t8)
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
    ppach   $t3, $s4, $s1
    prevh   $v0, $t3
    pcpyud  $v0, $v0, $v0
    pcpyld  $t3, $v0, $t3
# DCT_8_INV_ROW1
    lq      $s0, 64($a2)
    lq      $s1, 32($t8)
    lq      $s2, 48($t8)
    prevh   $v0, $s0
    lq      $s3, 64($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 80($t8)
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
    ppach   $t4, $s4, $s1
    prevh   $v0, $t4
    pcpyud  $v0, $v0, $v0
    pcpyld  $t4, $v0, $t4
# DCT_8_INV_ROW1
    lq      $s0, 80($a2)
    lq      $s1, 224($t8)
    lq      $s2, 240($t8)
    prevh   $v0, $s0
    lq      $s3, 256($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 272($t8)
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
    ppach   $t5, $s4, $s1
    prevh   $v0, $t5
    pcpyud  $v0, $v0, $v0
    pcpyld  $t5, $v0, $t5
# DCT_8_INV_ROW1
    lq      $s0,  96($a2)
    lq      $s1, 160($t8)
    lq      $s2, 176($t8)
    prevh   $v0, $s0
    lq      $s3, 192($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 208($t8)
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
    ppach   $t6, $s4, $s1
    prevh   $v0, $t6
    pcpyud  $v0, $v0, $v0
    pcpyld  $t6, $v0, $t6
# DCT_8_INV_ROW1
    lq      $s0, 112($a2)
    lq      $s1, 96($t8)
    lq      $s2, 112($t8)
    prevh   $v0, $s0
    lq      $s3, 128($t8)
    phmadh  $s1, $s1, $s0
    lq      $s4, 144($t8)
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
    ppach   $t7, $s4, $s1
    prevh   $v0, $t7
    pcpyud  $v0, $v0, $v0
    pcpyld  $t7, $v0, $t7
# DCT_8_INV_COL8
    lq          $v0, 320($t8)
    pmulth      $s1, $t3, $v0
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t5
    pmulth      $s2, $t5, $v0
    psraw       $s2, $s2, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s2, $v1, $s2
    paddh       $s2, $s2, $t3
    lq          $v0, 288($t8)
    pmulth      $s3, $t7, $v0
    psraw       $s3, $s3, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s3, $v1, $s3
    paddh       $s3, $s3, $t1
    pmulth      $s4, $t1, $v0
    psraw       $s4, $s4, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s4, $v1, $s4
    psubh       $s4, $s4, $t7
    psubh       $v1, $s3, $s2
    paddh       $s0, $s4, $s1
    psubh       $at, $s4, $s1
    paddh       $s4, $s3, $s2
    lq          $v0, 336($t8)
    paddh       $s5, $v1, $s0
    psubh       $t9, $v1, $s0
    pmulth      $s5, $s5, $v0
    psraw       $s5, $s5, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s5, $v1, $s5
    pmulth      $t9, $t9, $v0
    psraw       $t9, $t9, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $t9, $v1, $t9
    lq          $v0, 304($t8)
    pmulth      $s1, $t2, $v0
    psraw       $s1, $s1, 15
    pmfhl.uw    $v1
    psraw       $v1, $v1, 15
    pinteh      $s1, $v1, $s1
    psubh       $s1, $s1, $t6
    pmulth      $s2, $t6, $v0
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

    lq  $t3, 352($t8)
# DCT_8_INV_COL8_PMS
    paddh   $v0, $s0, $s4
    psubh   $s4, $s0, $s4
    psrah   $s0, $v0, 6
    psrah   $s4, $s4, 6
    paddh   $v1, $s1, $s5
    psubh   $s5, $s1, $s5
    psrah   $s1, $v1, 6
    psrah   $s5, $s5, 6
    paddh   $v0, $s2, $t9
    psubh   $t9, $s2, $t9
    psrah   $s2, $v0, 6
    psrah   $t9, $t9, 6
    paddh   $v1, $s3, $at
    psubh   $at, $s3, $at
    psrah   $s3, $v1, 6
    psrah   $at, $at, 6
# DCT_8_INV_COL8_ADD
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s0
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s1
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s2
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s3
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
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
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0
    ld      $v0, 0($a0)
    pextlb  $v0, $0, $v0
    paddh   $v0, $v0, $s4
    pminh   $v0, $v0, $t3
    pmaxh   $v0, $v0, $0
    ppacb   $v0, $0, $v0
    sd      $v0, 0($a0)
    add     $a0, $a1, $a0

    lw      $s0,  0($sp)
    lw      $s1,  4($sp)
    lw      $s2,  8($sp)
    lw      $s3, 12($sp)
    lw      $s4, 16($sp)
    lw      $s5, 20($sp)
    .set noreorder	 
    .set volatile
    jr      $ra
    addu    $sp, $sp, 24
.end IDCT_Add

.ent	DSP_PutPixels8
.global DSP_PutPixels8
DSP_PutPixels8:
.end    DSP_PutPixels8

.ent    DSP_PutNoRndPixels8
.global DSP_PutNoRndPixels8

DSP_PutNoRndPixels8:
    .set reorder	 
    .set novolatile
1:
    ldr     $t0, 0($a1)
    addiu   $a3, $a3, -1
    ldl     $t0, 7($a1)
    add     $a1, $a1, $a2
    sd      $t0, 0($a0)
    ldr     $t0, 0($a1)
    addiu   $a3, $a3, -1
    ldl     $t0, 7($a1)
    add     $a1, $a1, $a2
    sd      $t0, 8($a0)
    ldr     $t0, 0($a1)
    addiu   $a3, $a3, -1
    ldl     $t0, 7($a1)
    add     $a1, $a1, $a2
    sd      $t0, 16($a0)
    ldr     $t0, 0($a1)
    addiu   $a3, $a3, -1
    ldl     $t0, 7($a1)
    add     $a1, $a1, $a2
    sd      $t0, 24($a0)
    .set noreorder	 
    .set volatile
    bgtzl   $a3, 1b
    addiu   $a0, $a0, 32
    jr      $ra
    nop
.end DSP_PutNoRndPixels8

.ent    DSP_PutPixels16
.global DSP_PutPixels16
DSP_PutPixels16:
.end DSP_PutPixels16

.ent    DSP_PutNoRndPixels16
.global DSP_PutNoRndPixels16

DSP_PutNoRndPixels16:
    .set reorder	 
    .set novolatile
    mtsab   $a1, 0
1:
    lq      $t0,  0($a1)
    lq      $t1, 16($a1)
    add     $a1, $a1, $a2
    qfsrv   $t0, $t1, $t0
    lq      $t2,  0($a1)
    sq      $t0, 0($a0)
    lq      $t3, 16($a1)
    addiu   $a3, $a3, -2
    qfsrv   $t2, $t3, $t2
    add     $a1, $a1, $a2
    sq      $t2, 16($a0)
    .set noreorder	 
    .set volatile
    bgtzl   $a3, 1b
    addiu   $a0, $a0, 32
    jr      $ra
    nop
.end DSP_PutNoRndPixels16

.ent    DSP_AvgPixels16
.global DSP_AvgPixels16
DSP_AvgPixels16:
.end DSP_AvgPixels16

.ent    DSP_AvgNoRndPixels16
.global DSP_AvgNoRndPixels16

DSP_AvgNoRndPixels16:
    .set reorder
    .set novolatile
    ori     $t3, $zero, 0xFEFE
    pcpyh   $t3, $t3
    mtsab   $a1, 0
    pextlh  $t3, $t3, $t3
1:
    lq      $t0,  0($a0)
    lq      $t4, 16($a0)
    lq      $t1,  0($a1)
    lq      $t2, 16($a1)
    add     $a1, $a1, $a2
    lq      $t5,  0($a1)
    qfsrv   $t1, $t2, $t1
    lq      $t6, 16($a1)
    add     $a1, $a1, $a2
    qfsrv   $t5, $t6, $t5
    por     $t2, $t0, $t1
    por     $t6, $t4, $t5
    pxor    $t0, $t0, $t1
    pxor    $t4, $t4, $t5
    pand    $t0, $t0, $t3
    pand    $t4, $t4, $t3
    psrlw   $t0, $t0, 1
    psrlw   $t4, $t4, 1
    psubuw  $t2, $t2, $t0
    psubuw  $t6, $t6, $t4
    sq      $t2,  0($a0)
    addiu   $a3, $a3, -2
    sq      $t6, 16($a0)
    .set noreorder
    .set volatile
    bgtzl   $a3, 1b
    addiu   $a0, $a0, 32
    jr      $ra
    nop
.end DSP_AvgNoRndPixels16
